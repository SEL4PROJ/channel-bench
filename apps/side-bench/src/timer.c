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
#include <sel4/sel4.h>
#include <sel4platsupport/timer.h>
#include <sel4bench/sel4bench.h>
#include "bench_common.h"
#include "bench_types.h"
#include "bench_support.h"



#define INTERRUPT_PERIOD_NS (1000 * NS_IN_US)
#define TIMER_DETECT_INTERVAL_NS (500)


int timer_high(bench_env_t *env) {

    bench_args_t *args = env->args; 
    int error; 
    seL4_CPtr timer_signal = env->ntfn.cptr; 
    seL4_Word badge = 0;  
    ccnt_t start, end; 
    seL4_MessageInfo_t info;

    uint32_t *results = malloc(sizeof (uint32_t) * CONFIG_BENCH_DATA_POINTS); 
    assert(results); 

    assert(args->timer_enabled); 
    
    error = benchmark_init_timer(env);
    assert(error == BENCH_SUCCESS); 


    /*tell manager that we are ready*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);
    
    /*msg LOW: HIGH is ready*/
    seL4_Send(args->ep, info);


    error = ltimer_reset(&env->timer.ltimer);
    assert(error == 0); 
    
    error = ltimer_set_timeout(&env->timer.ltimer, INTERRUPT_PERIOD_NS, TIMEOUT_PERIODIC);
    assert(error == 0); 
    //for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
    while(1) {
        badge = 0; 

#ifdef CONFIG_ARCH_ARM 
        SEL4BENCH_READ_CCNT(start);  
#else 
        start = rdtscp_64();
#endif 
        /* wait for irq */
        //seL4_Wait(timer_signal, &badge);
        /*polling for the int until the int is received*/
        do {
            seL4_Poll(timer_signal, &badge);
        } while (!badge);

        /* record result */
#ifdef CONFIG_ARCH_ARM
        SEL4BENCH_READ_CCNT(end);  
#else 
        end = rdtscp_64();
#endif 
//        results[i] = end - start;
        sel4platsupport_handle_timer_irq(&env->timer, badge);
    }
#if 0
    printf("timer intervas are: \n"); 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        printf("%u\n", results[i]); 
    }
#endif 
    free(results); 

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

    ccnt_t start, cur, prev; 
    
    /*the record address*/
    r_addr = (struct bench_timer_online *)args->record_vaddr;

    /*syn with HIGH*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

#ifdef CONFIG_ARCH_ARM
    SEL4BENCH_READ_CCNT(start);  
#else 

    start = rdtscp_64(); 
#endif 
    prev = start;
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS;) {

#ifdef CONFIG_ARCH_ARM
        SEL4BENCH_READ_CCNT(cur);  
#else        
        cur = rdtscp_64();
#endif 

        /*at the begining of the current tick*/
        if (cur - prev > TIMER_DETECT_INTERVAL_NS) {
            
            /*online time (prevs - starts)*/
            r_addr->prevs[i] = prev;
            r_addr->starts[i] = start;
            start = cur;
            i++;
        }
        prev = cur;
    }

    /*send result to manager, benchmark is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    return 0; 
}
