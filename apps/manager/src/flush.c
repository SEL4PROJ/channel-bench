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
#include <simple/simple.h>
#include <sel4bench/kernel_logging.h>

#include "manager.h"
#include "bench_types.h"
#include "bench_helper.h"
#include "bench_common.h"



#if defined(CONFIG_MANAGER_CACHE_FLUSH) || defined (CONFIG_MANAGER_IPC)

static bench_thread_t flush_thread, idle_thread; 

/*ep for reply*/
static vka_object_t reply_ep, syn_ep, idle_ep;

#ifdef CONFIG_MANAGER_CACHE_FLUSH 
static void print_bench_cache_flush(void *user_log_vaddr, 
        void *kernel_log_vaddr) {

    uint32_t count; 
    struct bench_cache_flush *ulogs = user_log_vaddr; 

 
#ifdef CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER
    seL4_Word duration; 
    uint32_t nlog_kernel;

    kernel_log_entry_t *klogs = kernel_log_vaddr; 
 
    /*finalise the log in kernel*/
    nlog_kernel = seL4_BenchmarkFinalizeLog(); 
    assert(nlog_kernel != (BENCH_CACHE_FLUSH_RUNS + BENCH_WARMUPS)); 

   /*skip the warmup rounds*/
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {
        duration = kernel_logging_entry_get_data(klogs + BENCH_WARMUPS + count); 
        printf(" "CCNT_FORMAT" ", duration); 
    }
    printf("\n");

#endif

#ifdef CONFIG_KERNEL_SWITCH_COST_BENCH 

    printf("kernel switching cost: \n"); 
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {

        /*skip the warmup rounds*/
        seL4_BenchmarkGetKSCostPair(count + BENCH_WARMUPS); 
        /*the cost = measure - overhead*/
        printf(" "CCNT_FORMAT" ", seL4_GetMR(2) - seL4_GetMR(3)); 

    }
    printf("\n");

#endif 

    printf("cost in user-level : \n");
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {
        printf(" "CCNT_FORMAT" ", ulogs->costs[count]); 
    }
    printf("\n");

}

#endif 

static void print_ipc_result(void *record_vaddr) {

    ipc_rt_result_t *rt_v = (ipc_rt_result_t*)record_vaddr;

    printf("call reply wait overhead "CCNT_FORMAT"\n", rt_v->call_reply_wait_overhead); 
    printf("round trip IPC  after overhead is taken off: \n"); 

    for (int i = 0; i < IPC_RUNS; i++ ) {
        printf(" "CCNT_FORMAT" ", rt_v->call_rt_time[i]); 
    }
    printf("\n");
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    /*get pmu counter value*/
    for (int i = 0; i < BENCH_PMU_COUNTERS; i++) {

        assert(pmu_v->pmuc[IPC_REPLY_WAIT2][i] >=
                pmu_v->pmuc[IPC_CALL2][i]);

        pmu_counters[i] = pmu_v->pmuc[IPC_REPLY_WAIT2][i] - 
            pmu_v->pmuc[IPC_CALL2][i];  

    }
#endif
}


void launch_bench_flush (m_env_t *env) {

    int ret; 

#ifdef CONFIG_MANAGER_IPC 
    uint32_t n_p = (sizeof (ipc_rt_result_t) / BENCH_PAGE_SIZE) + 1; 
#else
    uint32_t n_p = (sizeof (struct bench_cache_flush) / BENCH_PAGE_SIZE) + 1;
#endif

    seL4_MessageInfo_t info;
  
    idle_thread.image = flush_thread.image = CONFIG_BENCH_THREAD_NAME;

    flush_thread.vspace = &env->vspace;
    idle_thread.vspace = &env->vspace;

    flush_thread.name = "flush"; 
    idle_thread.name = "user_idle"; 

#ifdef CONFIG_MANAGER_MITIGATION 
    /*using sperate kernels*/
    flush_thread.kernel = env->kimages[0].ki.cptr; 
    idle_thread.kernel = env->kimages[1].ki.cptr; 
#else 
    /*by default the kernel is shared*/
    flush_thread.kernel = env->kernel;
    idle_thread.kernel = env->kernel; 
#endif 

#ifdef CONFIG_MANAGER_MITIGATION 
    flush_thread.vka = &env->vka_colour[0]; 
    idle_thread.vka = &env->vka_colour[1]; 
    env->ipc_vka = &env->vka_colour[0];
#else 
    flush_thread.vka = &env->vka;
    idle_thread.vka = &env->vka; 
    env->ipc_vka = &env->vka;
#endif

#ifdef CONFIG_MANAGER_IPC_SAME_COLOUR 
    idle_thread.kernel = env->kimages[0].ki.cptr; 
    idle_thread.vka = &env->vka_colour[0]; 
#endif 
    
    idle_thread.root_vka = flush_thread.root_vka = &env->vka;
    idle_thread.simple = flush_thread.simple = &env->simple;

    /*ep for communicate*/
    ret = vka_alloc_endpoint(env->ipc_vka, &reply_ep);
    assert(ret == 0);

    ret = vka_alloc_endpoint(env->ipc_vka, &syn_ep);
    assert(ret == 0);
    
    ret = vka_alloc_endpoint(env->ipc_vka, &idle_ep);
    assert(ret == 0);
 
    idle_thread.ipc_vka = flush_thread.ipc_vka = env->ipc_vka; 
    flush_thread.reply_ep = reply_ep;
    idle_thread.reply_ep = idle_ep; 

    idle_thread.ep = flush_thread.ep = syn_ep; 

    idle_thread.prio = flush_thread.prio = 100;

#ifdef CONFIG_ARCH_X86
    idle_thread.kernel_prio = 0; 
    flush_thread.kernel_prio = 0;
#else 
    /*the 0xf0 is the default INT priority*/
    idle_thread.kernel_prio = 0xf0; 
    flush_thread.kernel_prio = 0xf0;
#endif 
    /*the kernel INT masking is not involved, as the cost is linear with 
     the INT enabled in a domain.*/

    flush_thread.test_num = BENCH_FLUSH_THREAD_NUM; 
    idle_thread.test_num = BENCH_IDLE_THREAD_NUM;  

    /*initing the thread*/
    create_thread(&flush_thread); 
    create_thread(&idle_thread); 


    printf("creating recording frames for benchmarking thread.\n"); 
    map_r_buf(env, n_p, &flush_thread);

    printf("launching threads\n");
    launch_thread(&flush_thread); 
    launch_thread(&idle_thread); 
    printf("waiting for result\n");
    info = seL4_Recv(reply_ep.cptr, NULL);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault); 
    
    printf("benchmark result ready\n");
 
    /*processing record*/
#ifdef CONFIG_MANAGER_IPC 

    print_ipc_result(env->record_vaddr);

#else 
    print_bench_cache_flush(env->record_vaddr, env->kernel_log_vaddr);
#endif 

    printf("done covert benchmark\n");
}
#endif 
