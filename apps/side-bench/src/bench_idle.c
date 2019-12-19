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
#include <stdio.h>
#include <sys/mman.h>
#include <sel4/sel4.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_common.h>
#include "mastik_common/low.h"
#include "mastik_common/l1i.h"

static void access_llc_buffer(void *buffer) {

    for (int i = 0; i < L3_SIZE; i += L1_CACHELINE)
        low_access(buffer + i); 
}



int bench_idle(bench_env_t *env) {
    seL4_MessageInfo_t info;
    seL4_Word badge;

    bench_args_t *args = env->args; 

#ifdef CONFIG_MANAGER_SPLASH_BENCH_SWITCH

    char *buf = (char *)mmap(NULL, L3_SIZE + 4096 * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    assert(buf); 
    /*page aligned the buffer*/
    uintptr_t buf_switch = (uintptr_t) buf; 
    buf_switch &= ~(0xfffULL); 
    buf_switch += 0x1000; 
    buf = (char *) buf_switch; 
    int mask_size = I_MONITOR_MASK; 

    uint64_t monitored_mask[mask_size]; 

    for (int m = 0; m < mask_size; m++)
        monitored_mask[m] = ~0LLU; 
    
    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);

#endif

    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

#ifdef CONFIG_BENCH_SPLASH_SWITCH_IDLE
    /*starting to spin, consuming CPU cycles only*/
    while (1) {

#ifdef CONFIG_KERNEL_SWITCH_COST_BENCH 
 
        /* flag of measuring the domain switching cost*/
        //        seL4_SetMR(100, 0x12345678); 

        seL4_Yield(); 
#else 
        ;
#endif 
    }
#endif 


#ifdef CONFIG_MANAGER_SPLASH_BENCH_SWITCH
      /*constantly probing on the LLC buffer
        and the L1-I, polluting caches*/
    while (1) {
        l1i_prime(l1i_1);
        access_llc_buffer(buf);            
    }
#else
    
    /*starting to spin, consuming CPU cycles only*/
    while (1); 
#endif 
    return BENCH_SUCCESS; 
}
