/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <sel4/sel4.h>
#include <sel4platsupport/timer.h>
#include <sel4bench/sel4bench.h>
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include "bench_support.h"
#include "mastik_common/low.h"


#define TIMER_DETECT_INTERVAL_NS (1200)

#define SIGNAL_RANGE   9

int timer_high(bench_env_t *env) {

    bench_args_t *args = env->args; 
    int error; 
    seL4_CPtr timer_signal = env->ntfn.cptr; 
    seL4_Word badge = 0;  
    seL4_MessageInfo_t info;
    uint32_t secret; 
    uint32_t *share_vaddr = (uint32_t*)args->shared_vaddr; 

    assert(args->timer_enabled); 
    
    error = timing_benchmark_init_timer(env);
    assert(error == BENCH_SUCCESS); 


    /*tell manager that we are ready*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);
    
    error = ltimer_reset(&env->timer.ltimer);
    assert(error == 0); 
    
    
    /*msg LOW: HIGH is ready*/
    seL4_Send(args->ep, info);

    /*actual range 11-19*/
    secret = random() % SIGNAL_RANGE + 11;
    
    error = ltimer_set_timeout(&env->timer.ltimer, secret * NS_IN_MS, TIMEOUT_RELATIVE);
    assert(error == 0); 

    /*update the secret read by low*/ 
    *share_vaddr = secret; 

   
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
        
        /*waiting for a system tick*/
        newTimeSlice();
        badge = 0;
        do {
            seL4_Poll(timer_signal, &badge);
            /*assuming the interrupt is received while the low is running*/
        }  while(!badge); 
        
        sel4platsupport_handle_timer_irq(&env->timer, badge);
        /*actual range 11-19*/
        secret = random() % SIGNAL_RANGE + 11;
        /*set the timer again*/
        error = ltimer_set_timeout(&env->timer.ltimer, secret * NS_IN_MS, TIMEOUT_RELATIVE);
        assert(error == 0); 

        /*update the secret read by low*/ 
        *share_vaddr = secret; 

       }

    /*waiting on ep never return*/
    seL4_Wait(args->ep, &badge);

    return 0;
}


int timer_low(bench_env_t *env) {

    /*the low detect the timer counter jump recording as the 
     online time*/
    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 
    struct bench_timer_online *r_addr; 
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 

    ccnt_t  start; 
    ccnt_t  cur; 
    ccnt_t  prev; 
    
    /*the record address*/
    r_addr = (struct bench_timer_online *)args->record_vaddr;

    /*syn with HIGH*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
    

#ifdef CONFIG_ARCH_ARM
    SEL4BENCH_READ_CCNT(start);  
#elif defined CONFIG_ARCH_RISCV
    start = rdtime();
#else

    start = rdtscp_64(); 
#endif 

    prev = start;

    uint32_t volatile prev_s = *secret; 
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS;) {
        FENCE(); 

#ifdef CONFIG_ARCH_ARM
        SEL4BENCH_READ_CCNT(cur);  
#elif defined CONFIG_ARCH_RISCV
        cur = rdtime();
#else        
        cur = rdtscp_64();
#endif 
        FENCE(); 

        /*at the begining of the current tick*/
        if (unlikely(cur - prev >= TIMER_DETECT_INTERVAL_NS)) {
            /*online time (prevs - starts)*/
            r_addr->prevs[i] = prev;
            r_addr->starts[i] = start;
            /*prev secret affect online time (prevs - starts)*/
            r_addr->sec[i] = prev_s;
            prev_s = *secret;
            start = cur;
            i++;
        }
        prev = cur;
        FENCE(); 

    }

    /*send result to manager, benchmark is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    return 0; 
}
