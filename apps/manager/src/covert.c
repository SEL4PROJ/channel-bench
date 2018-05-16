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
#include "ipc.h"
#include "bench_types.h"

#ifdef  CONFIG_MANAGER_COVERT_BENCH

/*start of the elf file*/
extern char __executable_start;

/*the benchmarking enviornment for two threads*/
static bench_thread_t trojan, spy;

/*ep for syn and reply*/
static vka_object_t syn_ep, t_ep, s_ep; 

/*init covert bench for single core*/
void init_single(m_env_t *env) {

    uint32_t n_p = (sizeof (struct bench_l1) / BENCH_PAGE_SIZE) + 1;

    /*over write according to the benchmark*/
#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL_SCHEDULE
    n_p = (sizeof (struct bench_kernel_schedule) / BENCH_PAGE_SIZE) + 1;
#endif 
#ifdef CONFIG_BENCH_COVERT_TIMER 
    n_p = (sizeof (struct bench_timer_online) / BENCH_PAGE_SIZE) + 1;
#endif 
    uint32_t share_phy; 
    int error; 

    printf("creating trojan\n"); 
    create_thread(&trojan);  
    
    printf("creating spy\n"); 
    create_thread(&spy);

    printf("creating shared frames between spy and trojan\n"); 
    /*creating shared frame between trojan and spy*/
    map_shared_buf(&spy, &trojan, NUM_L1D_SHARED_PAGE, &share_phy);

    printf("creating recording frames for spy\n"); 
    map_r_buf(env, n_p, &spy);

    printf("copy the timer obj to trojan\n"); 
    /* initialise timers for benchmark environment */
    error = sel4utils_copy_timer_caps_to_process(&trojan.bench_args->to, 
            trojan.to, trojan.root_vka, &trojan.process);
    assert(error == 0); 
    trojan.bench_args->timer_enabled = true; 

#ifdef CONFIG_MULTI_KERNEL_IMAGES 
    /*associate the timer irq with the kernel image*/
    error = sel4utils_set_timer_caps_to_kernel(trojan.to, trojan.kernel); 
    assert(error == 0); 
#endif 

#ifdef CONFIG_MANAGER_HUGE_PAGES
    printf("creating huge pages for spy and trojan\n"); 
    create_huge_pages(&trojan, BENCH_MORECORE_HUGE_SIZE); 
    create_huge_pages(&spy, BENCH_MORECORE_HUGE_SIZE); 
#endif 
     
    /*copy the notification cap*/ 
    trojan.bench_args->notification_ep = 
        sel4utils_copy_cap_to_process(&trojan.process, 
                trojan.vka, trojan.notification_ep.cptr);
    assert(trojan.bench_args->notification_ep); 

    spy.bench_args->notification_ep= 
        sel4utils_copy_cap_to_process(&spy.process, 
                spy.vka, spy.notification_ep.cptr);
    assert(spy.bench_args->notification_ep); 

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
    
    r_d =  (struct bench_l1 *)env->record_vaddr;
    printf("probing time start\n");
    
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
        printf("%d %u\n", r_d->sec[i], r_d->result[i]);

    }
    printf("probing time end\n");

#ifdef CONFIG_MANAGER_PMU_COUNTER 
    printf("enabled %d pmu counters\n", BENCH_PMU_COUNTERS);
    for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++) {

        /*print out the pmu counter one by one */
        printf("pmu counter %d start\n",  counter); 
        for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
            printf("%d %u\n", r_d->sec[i], r_d->pmu[i][counter]);
        }
        printf("pmu counter %d end\n", counter);
    }
#endif 

    printf("done covert benchmark\n");
    return BENCH_SUCCESS; 
}


int run_single_llc_kernel(m_env_t *env) {

    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 0); 
    printf("start covert benchmark, single core, LLC through a shared kernel with seL4_Poll\n"); 

    printf("Starting L3 spy setup\n");
    seL4_Send(s_ep.cptr, tag);
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("Starting L3 Trojan setup\n");
    seL4_Send(t_ep.cptr, tag);

    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("Starting test 1\n");
    seL4_Send(s_ep.cptr, tag);
    seL4_Send(t_ep.cptr, tag);
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("Starting test 2\n");
    seL4_Send(s_ep.cptr, tag);
    seL4_Send(t_ep.cptr, tag);
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("done covert benchmark\n");
    return BENCH_SUCCESS; 
}


int run_single_llc_kernel_schedule(m_env_t *env) {

    seL4_MessageInfo_t info;
    struct bench_kernel_schedule *r_d;

    printf("starting covert channel benchmark, LLC, kernel deterministic scheduling\n");

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

    for (int i = 3; i < CONFIG_BENCH_DATA_POINTS; i++) {

        printf("%d "CCNT_FORMAT"\n", 
                r_d->prev_sec[i], r_d->prevs[i] - r_d->starts[i]);

    }
    printf("online time end\n");

    printf("offline time start\n");
    for (int i = 3; i < CONFIG_BENCH_DATA_POINTS; i++) {
        printf("%d "CCNT_FORMAT"\n", 
                r_d->cur_sec[i], r_d->curs[i] - r_d->prevs[i]);
    }
    printf("offline time end\n");

    /*printf("Trojan: %llu %llu %llu -> %llu %llu %llu\n", r_d->prevs[i], r_d->starts[i], r_d->curs[i], r_d->prevs[i] - r_d->starts[i], r_d->curs[i] - r_d->prevs[i], r_d->curs[i] - r_d->starts[i]);
     */

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

    for (int i = 3; i < CONFIG_BENCH_DATA_POINTS; i++) {

        printf(CCNT_FORMAT"\n", 
                r_d->prevs[i] - r_d->starts[i]);
    }

    printf("done covert benchmark\n");
    return BENCH_SUCCESS;
}


/*running the single core attack*/ 
int run_single(m_env_t *env) {

    printf("starting covert channel benchmark\n");

    printf("data points %d with random sequence\n", CONFIG_BENCH_DATA_POINTS);

#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL 
    return run_single_llc_kernel(env); 
#endif

#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL_SCHEDULE
    return run_single_llc_kernel_schedule(env); 
#endif 

#ifdef CONFIG_BENCH_COVERT_TIMER 
    return run_single_timer(env); 
#endif

    return run_single_l1(env); 
}

int run_multi(m_env_t *env) {

    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 0); 
    
    printf("start multicore side channel benchmark\n"); 
    
    seL4_Send(s_ep.cptr, tag);
    
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
/*init the covert benchmark for multicore attack*/
void init_multi(m_env_t *env) {


    spy.affinity  = 0;
    trojan.affinity = 1; 

    /*one trojan, one spy thread*/ 
    create_thread(&trojan); 
    create_thread(&spy); 

    launch_thread(&trojan); 
    launch_thread(&spy); 

    if (run_multi(env) != BENCH_SUCCESS) 
        printf("running multi benchmark failed\n");

}
#ifdef CONFIG_ARCH_X86
static inline uint32_t rdtscp() {
  uint32_t rv;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}
#endif 


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

#ifdef CONFIG_MANAGER_MITIGATION 
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



#if 0
    /*setting the kernel sensitivity*/
#ifdef CONFIG_COVERT_TROJAN_SENSITIVE 
    seL4_KernelImage_Sensitive(trojan.kernel); 
    seL4_KernelImage_HoldTime(trojan.kernel, CONFIG_COVERT_TROJAN_HOLD_TIME, 0);
#ifdef CONFIG_ARCH_X86 
    srandom(rdtscp());
#else 
    srandom(sel4bench_get_cycle_count());
#endif 
   
#ifdef CONFIG_ARCH_X86
    for (int i = 0; i < 512; i ++)
        seL4_KernelImage_Random(trojan.kernel, i, random());
#endif  /*CONFIG_ARCH_X86*/
#endif  /*CONFIG COVERT TROJAN SENSITIVE*/

#ifdef CONFIG_COVERT_SPY_SENSITIVE 

    seL4_KernelImage_Sensitive(spy.kernel);                                    
    seL4_KernelImage_HoldTime(spy.kernel, CONFIG_COVERT_SPY_HOLD_TIME, 0); 
#ifdef CONFIG_ARCH_X86 
    srandom(rdtscp());
#else 
    srandom(sel4bench_get_cycle_count());
#endif
#ifdef CONFIG_ARCH_X86
     for (int i = 0; i < 512; i ++)
        seL4_KernelImage_Random(spy.kernel, i, random());
#endif   /*CONFIG_ARCH_X86*/
#endif  /*CONFIG_COVERT_SPY_SENSITIVE*/
#endif 

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
 
    trojan.prio = 100;
    spy.prio = 100; 

    spy.kernel_prio = 3;
    trojan.kernel_prio = 0;

    /*set the actual testing num in bench_common.h*/
    spy.test_num = BENCH_COVERT_SPY;
    trojan.test_num = BENCH_COVERT_TROJAN; 

#ifdef CONFIG_BENCH_COVERT_SINGLE
    init_single(env);
    /*run bench*/
    ret = run_single(env);
    assert(ret == BENCH_SUCCESS);
#endif /*covert single*/ 

#ifdef CONFIG_MANAGER_COVERT_MULTI 
    init_multi(env); 
#endif 
}
#endif  /*CONFIG_MANAGER_COVERT_BENCH*/
