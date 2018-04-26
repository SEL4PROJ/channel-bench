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

static bench_thread_t flush_thread;

/*ep for reply*/
static vka_object_t reply_ep, syn_ep;

static void print_bench_cache_flush(void *user_log_vaddr, 
        void *kernel_log_vaddr) {

    uint32_t count, nlog_kernel;
    seL4_Word duration; 

    kernel_log_entry_t *klogs = kernel_log_vaddr; 
    struct bench_cache_flush *ulogs = user_log_vaddr; 

#ifdef CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER
    /*finalise the log in kernel*/
    nlog_kernel = seL4_BenchmarkFinalizeLog(); 
    assert(nlog_kernel != (BENCH_CACHE_FLUSH_RUNS + BENCH_WARMUPS)); 

    printf("cost in kernel : \n");
    /*skip the warmup rounds*/
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {
        duration = kernel_logging_entry_get_data(klogs + BENCH_WARMUPS + count); 
        printf(" %u", duration); 
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
  
    flush_thread.image = CONFIG_BENCH_THREAD_NAME;

    flush_thread.vspace = &env->vspace;

    flush_thread.name = "flush"; 

#ifdef CONFIG_MANAGER_MITIGATION 
    /*using sperate kernels*/
    flush_thread.kernel = env->kimages[0].ki.cptr; 

#else 
    /*by default the kernel is shared*/
    flush_thread.kernel = env->kernel;
#endif 

#ifdef CONFIG_MANAGER_MITIGATION 
    flush_thread.vka = &env->vka_colour[0]; 
    env->ipc_vka = &env->vka_colour[0];
#else 
    flush_thread.vka = &env->vka; 
    env->ipc_vka = &env->vka;
#endif

    flush_thread.root_vka = &env->vka;
    flush_thread.simple = &env->simple;

    /*ep for communicate*/
    ret = vka_alloc_endpoint(env->ipc_vka, &reply_ep);
    assert(ret == 0);

    ret = vka_alloc_endpoint(env->ipc_vka, &syn_ep);
    assert(ret == 0);
 
    flush_thread.ipc_vka = env->ipc_vka; 
    flush_thread.reply_ep = reply_ep;
    flush_thread.ep = syn_ep; 

    flush_thread.prio = 100;

    flush_thread.kernel_prio = 0;

    flush_thread.test_num = BENCH_FLUSH_THREAD_NUM; 

    /*initing the thread*/
    printf("Flushing cache benchmark, creating thread...");
    create_thread(&flush_thread); 

    printf("done. \n");

    printf("creating recording frames for benchmarking thread.\n"); 
    map_r_buf(env, n_p, &flush_thread);

    launch_thread(&flush_thread); 

    info = seL4_Recv(reply_ep.cptr, NULL);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault); 
    
    printf("benchmark result ready\n");
 
    /*processing record*/
    print_bench_cache_flush(env->record_vaddr, env->kernel_log_vaddr);

}


