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

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <sel4utils/mapping.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <simple/simple.h>

#include "manager.h"
#include <channel-bench/bench_types.h>

#ifdef  CONFIG_MANAGER_COVERT_BENCH

/*the benchmarking enviornment for two threads*/
static bench_thread_t trojan, spy;

/*ep for syn and reply*/
static vka_object_t syn_ep, t_ep, s_ep;


/*init covert bench thread then letting them run*/
void init_timing_threads(m_env_t *env) {

    uint32_t n_p = (sizeof (struct bench_l1) / BENCH_PAGE_SIZE) + 1;

    /*over write according to the benchmark*/
#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL_SCHEDULE
    n_p = (sizeof (struct bench_kernel_schedule) / BENCH_PAGE_SIZE) + 1;
#endif 
#ifdef CONFIG_BENCH_COVERT_TIMER 
    n_p = (sizeof (struct bench_timer_online) / BENCH_PAGE_SIZE) + 1;
#endif 

#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL 
    n_p = (sizeof (bench_llc_kernel_probe_result_t) / BENCH_PAGE_SIZE) + 1;
#endif 

    uintptr_t share_phy; 
    int error; 

    printf("creating trojan\n"); 
    create_thread(&trojan, 1);  
    
    printf("creating spy\n"); 
    create_thread(&spy, 2);

    printf("creating shared frames between spy and trojan\n"); 
    /*creating shared frame between trojan and spy*/
    map_shared_buf(&trojan, &spy, NUM_L1D_SHARED_PAGE, &share_phy);

    printf("creating recording frames for spy\n"); 
    map_r_buf(env, n_p, &spy);

#ifdef CONFIG_ARCH_ARM 
    /*assign a priority number to all the IRQ associated with 
     the timer*/
    printf("Set a priority number to the timer IRQs\n"); 

    error = sel4utils_set_timer_caps_priority(trojan.to, TROJAN_TIMER_IRQ_PRIORITY);
    assert(error == 0); 
#endif 

    printf("copy the timer obj to trojan\n"); 
    /* initialise timers for benchmark environment */
    error = sel4utils_copy_timer_caps_to_process(&trojan.bench_args->to, 
            trojan.to, trojan.root_vka, &trojan.process);
    assert(error == 0); 
    trojan.bench_args->timer_enabled = true; 


#ifdef CONFIG_MULTI_KERNEL_IMAGES
    /*associate the timer irq with the kernel image*/
    error = sel4utils_set_timer_caps_to_kernel(trojan.to, trojan.kernel, true); 
    assert(error == 0); 
#endif 

#ifdef CONFIG_MANAGER_HUGE_PAGES
    printf("creating huge pages for spy and trojan\n"); 
    create_huge_pages(&trojan, BENCH_MORECORE_HUGE_SIZE); 
    create_huge_pages(&spy, BENCH_MORECORE_HUGE_SIZE); 
#endif 

    /*notification ep*/
    trojan.bench_args->notification_ep = 
        sel4utils_copy_cap_to_process(&trojan.process, 
                trojan.vka, trojan.notification_ep.cptr);
    assert(trojan.bench_args->notification_ep); 

    spy.bench_args->notification_ep=  
        sel4utils_copy_cap_to_process(&spy.process, 
                spy.vka, spy.notification_ep.cptr);
    assert(spy.bench_args->notification_ep); 

    /*fake tcb*/
    trojan.bench_args->fake_tcb = 
        sel4utils_copy_cap_to_process(&trojan.process, 
                trojan.vka, trojan.fake_tcb.cptr);
    assert(trojan.bench_args->fake_tcb);

    spy.bench_args->fake_tcb = 
        sel4utils_copy_cap_to_process(&spy.process, 
                spy.vka, spy.fake_tcb.cptr);
    assert(spy.bench_args->fake_tcb);


    /*lanuch the benchmarking thread*/
    launch_thread(&trojan); 
    launch_thread(&spy);
}


int run_single_l1(m_env_t *env) {
    
    seL4_MessageInfo_t info;
    struct bench_l1 *r_d;

   
    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
       return BENCH_FAILURE;
    printf("trojan is ready\n");
    
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    printf("benchmark result ready\n");

#ifdef CONFIG_FLUSH_CORE_STATES 
    /*trun off the flag for cache flush, making the printing faster*/
    seL4_Yield();
#endif

    r_d =  (struct bench_l1 *)env->record_vaddr;
    printf("probing time start\n");
    
    for (int i = BENCH_TIMING_WARMUPS; i < CONFIG_BENCH_DATA_POINTS; i++) {
        printf("%d %u\n", r_d->sec[i], r_d->result[i]);

    }
    printf("probing time end\n");

#ifdef CONFIG_MANAGER_PMU_COUNTER 
    printf("enabled %d pmu counters\n", BENCH_PMU_COUNTERS);
    for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++) {

        /*print out the pmu counter one by one */
        printf("pmu counter %d start\n",  counter); 
        for (int i = BENCH_TIMING_WARMUPS; i < CONFIG_BENCH_DATA_POINTS; i++) {
            printf("%d %u\n", r_d->sec[i], r_d->pmu[i][counter]);
        }
        printf("pmu counter %d end\n", counter);
    }
#endif 

    printf("done covert benchmark\n");
    return BENCH_SUCCESS; 
}


int run_covert_llc_kernel(m_env_t *env) {

    bench_llc_kernel_probe_result_t *probe_result = NULL; 

    ccnt_t attack_start;
    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag;
    uint32_t lines = 0; 
    enum timing_api seq; 

    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("spy is ready\n");

    tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
   
#ifdef CONFIG_ARCH_X86
    attack_start = rdtscp_64();
#else 
    SEL4BENCH_READ_CCNT(attack_start);  
 
#endif 
    seL4_SetMR(0, attack_start);
    seL4_Send(t_ep.cptr, tag);

    seL4_SetMR(0, attack_start);
    seL4_Send(s_ep.cptr, tag);

    printf("waiting for benchmark result\n");
    
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    probe_result = (bench_llc_kernel_probe_result_t *)env->record_vaddr;
    printf("benchmark result ready\n");

    printf("Spy: probe sets for signal %d\n", probe_result->probe_sets[timing_signal]);
    printf("Spy: probe sets for tcb  %d\n", probe_result->probe_sets[timing_tcb]);
    printf("Spy: probe sets for poll %d\n", probe_result->probe_sets[timing_poll]);


    printf("probing time start\n");

    for (int i = BENCH_TIMING_WARMUPS; i < CONFIG_BENCH_DATA_POINTS; i++) {

        seq = probe_result->probe_seq[i]; 

        printf("%d", seq); 

        for (int j = 0; j < timing_api_num; j++) {
            //    printf(" %d", probe_result->probe_results[i][j]);

            lines += probe_result->probe_results[i][j]; 
        }

        printf(" %d\n", lines);

        lines = 0; 
    }
    printf("Probing end\n");

    printf("done covert benchmark\n");
    return BENCH_SUCCESS; 
}


int run_single_llc_kernel_schedule(m_env_t *env) {

    seL4_MessageInfo_t info;
    struct bench_kernel_schedule *r_d;

    printf("starting covert channel benchmark, LLC, kernel deterministic schedueling\n");

    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
       return BENCH_FAILURE;
    printf("trojan is ready\n");

    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    printf("benchmark result ready\n");

    r_d =  (struct bench_kernel_schedule *)env->record_vaddr;
    printf("online time start\n");

    for (int i = BENCH_TIMING_WARMUPS; i < CONFIG_BENCH_DATA_POINTS; i++) {

        printf("%d "CCNT_FORMAT"\n", 
                r_d->prev_sec[i], r_d->prevs[i] - r_d->starts[i]);

    }
    printf("online time end\n");

    printf("offline time start\n");
    for (int i = BENCH_TIMING_WARMUPS; i < CONFIG_BENCH_DATA_POINTS; i++) {
        printf("%d "CCNT_FORMAT"\n", 
                r_d->cur_sec[i], r_d->curs[i] - r_d->prevs[i]);
    }
    printf("offline time end\n");

    printf("done covert benchmark\n");
    return BENCH_SUCCESS;
}


int run_single_timer(m_env_t *env) {

    seL4_MessageInfo_t info;
    struct bench_timer_online *r_d = (struct bench_timer_online *)env->record_vaddr;
    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
       return BENCH_FAILURE;
    printf("HIGH is ready\n");

    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    printf("benchmark result ready\n");
    printf("probing time start\n");
 
    for (int i = BENCH_TIMING_WARMUPS; i < CONFIG_BENCH_DATA_POINTS; i++) {

        printf("%d "CCNT_FORMAT"\n", r_d->sec[i],
                r_d->prevs[i] - r_d->starts[i]);
    }
    printf("probing time end\n");


    printf("done covert benchmark\n");
    return BENCH_SUCCESS;
}

int run_multi(m_env_t *env) {

    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 0); 
    
    printf("start multicore side channel benchmark\n"); 
    
    seL4_Send(s_ep.cptr, tag);
    
    printf("spy is alive\n"); 
    

    info = seL4_Recv(s_ep.cptr, NULL); 
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE; 
    
    printf("spy ready\n");

    seL4_Send(s_ep.cptr, tag);
        
    info = seL4_Recv(s_ep.cptr, NULL); 
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE; 

    printf("done covert benchmark\n"); 
    /*do not return*/
    for (;;)
    return BENCH_SUCCESS; 

}




/*running the single core attack*/ 
int run_timing_threads(m_env_t *env) {

    printf("starting covert channel benchmark\n");

    printf("data points %d with random sequence\n", CONFIG_BENCH_DATA_POINTS);

#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL
    return run_covert_llc_kernel(env); 
#endif 

#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL_SCHEDULE
    return run_single_llc_kernel_schedule(env); 
#endif 

#ifdef CONFIG_BENCH_COVERT_TIMER 
    return run_single_timer(env); 
#endif

#ifdef CONFIG_MASTIK_ATTACK_SIDE 
    return run_multi(env); 
#endif 

    return run_single_l1(env); 
}


/*entry point of covert channel benchmark*/
void launch_bench_covert (m_env_t *env) {

    int ret; 
    trojan.image = spy.image = CONFIG_BENCH_THREAD_NAME;
    trojan.vspace = spy.vspace = &env->vspace;
    trojan.name = "trojan"; 
    spy.name = "spy";
#ifdef CONFIG_MANAGER_MITIGATION 
    /*using sperate kernels*/
    trojan.kernel = env->kimages[0].ki.cptr; 
    spy.kernel = env->kimages[1].ki.cptr;
#else 
    /*by default the kernel is shared*/
    trojan.kernel = spy.kernel = env->kernel;
#endif  

#if defined (CONFIG_MANAGER_MITIGATION) || defined (CONFIG_BENCH_COVERT_LLC_KERNEL)
    trojan.vka = &env->vka_colour[0]; 
    spy.vka = &env->vka_colour[1]; 
    env->ipc_vka = &env->vka_colour[0];
#else 
    spy.vka = trojan.vka = &env->vka; 
    env->ipc_vka = &env->vka;
#endif


    spy.root_vka = trojan.root_vka = &env->vka;

    spy.simple = trojan.simple = &env->simple;

    /*sharing the timer object*/
    spy.to = trojan.to = &env->to; 

    /*ep for communicate*/
    ret = vka_alloc_endpoint(env->ipc_vka, &syn_ep);
    assert(ret == 0);
    /*ep for spy to manager*/
    ret = vka_alloc_endpoint(env->ipc_vka, &s_ep); 
    assert(ret == 0);
    /*ep for trojan to manager*/
    ret = vka_alloc_endpoint(env->ipc_vka, &t_ep);
    assert(ret == 0);

    trojan.ipc_vka = spy.ipc_vka = env->ipc_vka; 
    spy.ep  = trojan.ep = syn_ep; 
    spy.reply_ep = s_ep;
    trojan.reply_ep = t_ep; 

    /*create a notification endpoint, used only within a domain*/
    ret = vka_alloc_notification(trojan.vka, &trojan.notification_ep); 
    assert(ret == 0); 
    
    ret = vka_alloc_notification(spy.vka, &spy.notification_ep); 
    assert(ret == 0); 

    /*Create TCB objects for syscall timing channel*/
    ret = vka_alloc_tcb(trojan.vka, &trojan.fake_tcb);
    assert(ret == 0);
    ret = vka_alloc_tcb(spy.vka, &spy.fake_tcb);
    assert(ret == 0);

    trojan.prio = 100;
    spy.prio = 100; 

    /*set the actual testing num in bench_common.h*/
    spy.test_num = BENCH_COVERT_SPY;
    trojan.test_num = BENCH_COVERT_TROJAN; 

#if CONFIG_MAX_NUM_NODES > 1
    spy.affinity  = 0;
    trojan.affinity = 1; 
#endif 

    init_timing_threads(env);

    ret = run_timing_threads(env);
    assert(ret == BENCH_SUCCESS);

}
#endif  /*CONFIG_MANAGER_COVERT_BENCH*/
