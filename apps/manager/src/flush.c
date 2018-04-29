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
#ifdef CONFIG_MANAGER_CACHE_FLUSH 

static bench_thread_t flush_thread, idle_thread; 

/*ep for reply*/
static vka_object_t reply_ep, syn_ep, idle_ep;

static void print_bench_cache_flush(void *user_log_vaddr, 
        void *kernel_log_vaddr) {

    uint32_t count; 
    struct bench_cache_flush *ulogs = user_log_vaddr; 

    printf("cost in kernel : \n");
 
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
        printf(" %u", duration); 
    }
    printf("\n");

#endif

#ifdef CONFIG_KERNEL_SWITCH_COST_BENCH 

    printf("measurment overhead: \n");
   /*skip the warmup rounds*/
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {
        seL4_BenchmarkGetKSCostPair(count + BENCH_WARMUPS); 
 
        printf(" %lu", seL4_GetMR(3)); 
    }
    printf("\n");
    printf("switching cost: \n"); 

   /*skip the warmup rounds*/
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {
        seL4_BenchmarkGetKSCostPair(count + BENCH_WARMUPS); 

        printf(" %lu", seL4_GetMR(2)); 
    }

    printf("\n");
#endif 

    printf("cost in user-level : \n");
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {
        printf(" "CCNT_FORMAT" ", ulogs->costs[count]); 
    }
    printf("\n");

    printf("overhead for user-level measurement: "CCNT_FORMAT" \n", ulogs->overhead);

    printf("done covert benchmark\n");
}


void launch_bench_single (m_env_t *env) {

    int ret; 
    uint32_t n_p = (sizeof (struct bench_cache_flush) / BENCH_PAGE_SIZE) + 1;
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

    idle_thread.kernel_prio = 0; 
    flush_thread.kernel_prio = 0;

    /*the kernel INT masking is not involved, as the cost is linear with 
     the INT enabled in a domain.*/

    flush_thread.test_num = BENCH_FLUSH_THREAD_NUM; 
    idle_thread.test_num = BENCH_CACHE_FLUSH_IDLE;  

    /*initing the thread*/
    create_thread(&flush_thread); 
    create_thread(&idle_thread); 


    printf("creating recording frames for benchmarking thread.\n"); 
    map_r_buf(env, n_p, &flush_thread);

    launch_thread(&flush_thread); 
    launch_thread(&idle_thread); 

    info = seL4_Recv(reply_ep.cptr, NULL);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault); 
    
    printf("benchmark result ready\n");
 
    /*processing record*/
    print_bench_cache_flush(env->record_vaddr, env->kernel_log_vaddr);

}

#endif 
