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
#include <simple/simple.h>

#include "manager.h"
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>
#include <channel-bench/bench_common.h>


#ifdef CONFIG_MANAGER_SPLASH_BENCH

static bench_thread_t flush_thread, idle_thread; 

/*ep for reply*/
static vka_object_t reply_ep, syn_ep, idle_ep;


static void print_result (m_env_t *env) {
    
    uint64_t takes; 

    splash_bench_result_t *result =(splash_bench_result_t*)env->record_vaddr; 

    takes = result->overall - result->overhead; 
#ifdef CONFIG_ARCH_X86
    printf("raw cost: %ld, overhead %ld\n", result->overall, result->overhead); 

    printf("Total cost: %ld\n", takes); 

#else 
    printf("raw cost: %lld, overhead %lld\n", result->overall, result->overhead); 

    printf("Total cost: %lld\n", takes); 
#endif 


#ifdef CONFIG_KERNEL_SWITCH_COST_BENCH 

    int count; 
    /*reading what has been recorded by kernel, currently 100 paris*/
    printf("kernel switching cost: \n"); 
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {

        /*the first log number is the cost of switching from the attack thread
          to the idle thread
         the second log number is the cost of switching back from the
         idle thread to the attack thread
         skip the cost of switching from the idle thread*/
        seL4_BenchmarkGetKSCostPair(count); 
        /*the cost = measure*/
        printf(" "CCNT_FORMAT" \n", seL4_GetMR(2)); 
    }
    
    printf("kernel measurement overhead: \n"); 
    for (count = 0; count < BENCH_CACHE_FLUSH_RUNS; count++) {

        /*the first log number is the cost of switching from the attack thread
          to the idle thread
         the second log number is the cost of switching back from the
         idle thread to the attack thread
         skip the cost of switching from the idle thread*/
        seL4_BenchmarkGetKSCostPair(count); 
        /*the overhead*/
        printf(" "CCNT_FORMAT" \n", seL4_GetMR(3)); 

    }

#endif 

}

void launch_bench_splash(m_env_t *env) {



    int ret; 

    uint32_t n_p = (sizeof (splash_bench_result_t) / BENCH_PAGE_SIZE) + 1;

    seL4_MessageInfo_t info;
  
    idle_thread.image = flush_thread.image = CONFIG_BENCH_THREAD_NAME;

    flush_thread.vspace = &env->vspace;
    idle_thread.vspace = &env->vspace;

    flush_thread.name = "splash"; 
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

#ifdef CONFIG_MANAGER_UNCOLOUR_KERNEL 
    flush_thread.kernel = env->kimages[0].ki.cptr; 
    idle_thread.kernel = env->kimages[1].ki.cptr; 
#endif  /*UNCOLOUR_KERNEL*/
 
#ifdef CONFIG_MANAGER_MITIGATION 
    flush_thread.vka = &env->vka_colour[0]; 
    idle_thread.vka = &env->vka_colour[1]; 
    env->ipc_vka = &env->vka_colour[0];
#else 
    flush_thread.vka = &env->vka;
    idle_thread.vka = &env->vka; 
    env->ipc_vka = &env->vka;
#endif

#ifdef CONFIG_MANAGER_COLOUR_ONLY 
    flush_thread.vka = &env->vka_colour[0]; 
    idle_thread.vka = &env->vka_colour[1]; 
    env->ipc_vka = &env->vka_colour[0];
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

    /*the kernel INT masking is not involved, as the cost is linear with 
     the INT enabled in a domain.*/

    flush_thread.test_num = BENCH_SPLASH_TEST_NUM; 
    idle_thread.test_num = BENCH_SPLASH_IDLE_NUM;  
    /*don't allocate any untypes*/
    idle_thread.untype_none = true; 

    /*initing the thread*/
    printf("creating splash thread.\n"); 

    create_thread(&flush_thread, 0); 
    printf("creating idle thread.\n"); 

    create_thread(&idle_thread, 0); 

    printf("creating recording frames for benchmarking thread.\n"); 
    map_r_buf(env, n_p, &flush_thread);

#ifdef CONFIG_BENCH_SPLASH_MORECORE
    printf("creating extra morecore frames for benchmarking thread.\n"); 
    map_morecore_buf(SPLASH_MORECORE_SIZE, &flush_thread);
#endif  

    printf("launching splash thread\n");
    launch_thread(&flush_thread);

#ifdef CONFIG_MANAGER_SPLASH_BENCH_SWITCH
 
    /*testing the cost of context switch*/
    printf("lanching the idle thread\n");
    launch_thread(&idle_thread); 
#endif

    printf("waiting for result\n");
    info = seL4_Recv(reply_ep.cptr, NULL);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault); 
    
    printf("benchmark result ready\n");

    print_result(env); 


    printf("done covert benchmark\n");
}
#endif  /*CONFIG_MANAGER_SPLASH_BENCH*/
