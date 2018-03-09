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

#define INTERRUPT_PERIOD_NS (10 * NS_IN_MS)



int timer_high(bench_env_t *env) {

    bench_args_t *args = env->args; 
    int error; 
    seL4_CPtr timer_signal = env->ntfn.cptr; 
    seL4_Word badge; 
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

    error = ltimer_reset(&env->timer.ltimer);
    assert(error == 0); 

    error = ltimer_set_timeout(&env->timer.ltimer, INTERRUPT_PERIOD_NS, TIMEOUT_PERIODIC);
    assert(error == 0); 

    SEL4BENCH_READ_CCNT(start); 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        /* wait for irq */
        seL4_Wait(timer_signal, &badge);
        /* record result */
        SEL4BENCH_READ_CCNT(end);
        sel4platsupport_handle_timer_irq(&env->timer, badge);
        results[i] = end - start;

        start = end; 
    }

    printf("timer intervas are: \n"); 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        printf("%u\n", results[i]); 
    }

    free(results); 

    return 0;
}


int timer_low(bench_env_t *env) {

    return 0; 
}
