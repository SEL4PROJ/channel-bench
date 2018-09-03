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

#include "manager.h"
#include "bench_types.h"
#include "bench_helper.h"
#include "bench_common.h"


#ifdef CONFIG_MANAGER_SPLASH_BENCH

static bench_thread_t flush_thread, idle_thread; 

/*ep for reply*/
static vka_object_t reply_ep, syn_ep, idle_ep;


static void print_result (m_env_t *env) {
    
    ccnt_t takes; 

    splash_bench_result_t *result =(splash_bench_result_t*)env->record_vaddr; 

    takes = result->overall - result->overhead; 

    printf("cost before scale up : %d, overhead %d\n", result->overall, result->overhead); 

    printf("Total cost in CPU ticks: %lld\n", takes * 64ULL); 

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

#ifdef CONFIG_MANAGER_CACHE_DIV_UNEVEN 
    /*cannot create a kerne image from domain 1 */
    idle_thread.kernel = env->kimages[0].ki.cptr; 
#else 
    idle_thread.kernel = env->kimages[1].ki.cptr; 
#endif /*UNEVEN*/

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
    idle_thread.test_num = BENCH_SPLASH_TEST_NUM;  

    /*initing the thread*/
    create_thread(&flush_thread); 

#ifndef CONFIG_MANAGER_CACHE_DIV_UNEVEN 
    /*cannot creat a thread with just tiny colour*/
    create_thread(&idle_thread); 
#endif 

    printf("creating recording frames for benchmarking thread.\n"); 
    map_r_buf(env, n_p, &flush_thread);

    printf("launching threads\n");
    launch_thread(&flush_thread);

    /*not launching the second thread*/
//    launch_thread(&idle_thread); 
    printf("waiting for result\n");
    info = seL4_Recv(reply_ep.cptr, NULL);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault); 
    
    printf("benchmark result ready\n");

    print_result(env); 


    printf("done covert benchmark\n");
}
#endif  /*CONFIG_MANAGER_SPLASH_BENCH*/
