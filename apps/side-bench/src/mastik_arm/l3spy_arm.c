/*this file contains the covert channel attack using the LLC on sabre*/
/*cache line size 32 bytes, direct mapped to 16 way associativity, way size 64 KB, replacement policy round-robin, total 16 colours , 1MB. 2048 sets.*/

/*the attack currently only works for single core: 
 on single core: sender probes 0-2048 sets, receiver probes 2048 sets*/

#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>
#include "../mastik_common/low.h"
#include "l3_arm.h"



int l3_trojan_single(bench_env_t *env) {

    uint32_t secret;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 

    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);

#ifdef CONFIG_DEBUG_BUILD
    printf("trojan: Got %d sets\n", nsets);
#endif

    /*receive the shared address to record the secret*/
    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 

    uint16_t *results = malloc(sizeof (uint16_t) * nsets); 
    assert(results);

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*ready to do the test*/
    seL4_Send(args->ep, info);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();

        secret = random() % (nsets + 1); 

        l3_unmonitorall(l3);

        /*l3_monitor already touch the cache lines*/
        for (int s = 0; s < secret; s++) {
            l3_monitor(l3, s);
        }
        /*update the secret read by low*/ 
        *share_vaddr = secret; 

    }

    while (1);
    
    return 0;
}


int l3_spy_single(bench_env_t *env) {
    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args;  
      
    struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 
    
    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);

#ifdef CONFIG_DEBUG_BUILD
    printf("spy: Got %d sets\n", nsets);
#endif

    uint16_t *results = malloc(sizeof (uint16_t) * nsets); 
    assert(results);

    l3_unmonitorall(l3);

    /*monitor all sets*/
    for (int s = 0; s < nsets; s++) {
        l3_monitor(l3, s);
    }

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();
        
        l3_probe(l3, results);
        r_addr->result[i] = 0; 
        for (int s = 0; s < nsets; s++) {
            r_addr->result[i] += results[s];
        }

        /*result is the total probing cost
          secret is updated by trojan in the previous system tick*/
        r_addr->sec[i] = *secret; 
    }
    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
} 


/*using the shared kernel image as a timing channel: receiver*/
int l3_spy(bench_env_t *env) {

    kernel_timing_caps_t caps;

    bench_args_t *args = env->args; 
    
    caps.async_ep = args->notification_ep; 
    caps.fake_tcb = args->fake_tcb; 
    caps.self_tcb = simple_get_tcb(&env->simple); 

    seL4_CPtr reply_ep = args->r_ep; 

    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 

    bench_llc_kernel_probe_result_t *probe_result = 
        (bench_llc_kernel_probe_result_t*)args->record_vaddr; 

    seL4_Word badge;
    seL4_MessageInfo_t info;
    ccnt_t tick_start;
    
    //Hold probing set for each API
    vlist_t probed[timing_api_num];
    
    //Find the L3 eviction buffer
    l3pp_t l3 = l3_prepare();
    assert(l3); 

#ifdef CONFIG_DEBUG_BUILD
    int nsets = l3_getSets(l3);
    printf("Spy: Got %d sets\n", nsets);
#endif

    /*search the probing buffer for the seL4 APIs*/
    for (int i = 0; i < timing_api_num; i++)
        probed[i] = search(l3, i, &caps);
#if 1
    remove_same_probesets(probed[timing_signal], probed[timing_tcb]); 
    remove_same_probesets(probed[timing_tcb], probed[timing_poll]); 
    remove_same_probesets(probed[timing_signal], probed[timing_poll]);
#endif 

#ifdef CONFIG_DEBUG_BUILD
    printf("Spy: probe sets for signal %d\n", vl_len(probed[timing_signal]));
    printf("Spy: probe sets for tcb  %d\n", vl_len(probed[timing_tcb]));
    printf("Spy: probe sets for poll %d\n", vl_len(probed[timing_poll]));
#endif

    probe_result->probe_sets[timing_signal] = vl_len(probed[timing_signal]); 
    probe_result->probe_sets[timing_tcb] = vl_len(probed[timing_tcb]); 
    probe_result->probe_sets[timing_poll] = vl_len(probed[timing_poll]); 

    //Signal root ready
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(reply_ep, info);
    
    /*root give global time*/
    info = seL4_Recv(reply_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
    tick_start = (ccnt_t)seL4_GetMR(0);
    
    /*waiting for the trojan for sync msg*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
    
    SEL4BENCH_READ_CCNT(tick_start);  

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        newTimeSlice(); 

        /*probing on each API list*/
        probe_result->probe_results[i][timing_signal] = 
            probing_sets(probed[timing_signal]);  

        probe_result->probe_results[i][timing_tcb] = 
            probing_sets(probed[timing_tcb]);  

        probe_result->probe_results[i][timing_poll] = 
            probing_sets(probed[timing_poll]);  

        probe_result->probe_seq[i] = *secret;
    }

    //Signal root we finished
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(reply_ep, info);

    while (1);

    return 0;
}


/*using the shared kernel image as a timing channel: sender*/
int l3_trojan(bench_env_t *env) {

    kernel_timing_caps_t caps; 
    
    bench_args_t *args = env->args; 

    caps.async_ep = args->notification_ep; 
    caps.fake_tcb = args->fake_tcb; 

    caps.self_tcb = simple_get_tcb(&env->simple); 


    seL4_CPtr reply_ep = args->r_ep; 
    ccnt_t tick_start; 
    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 
  
    seL4_Word badge;
    seL4_MessageInfo_t info;

    /*root give global time*/
    info = seL4_Recv(reply_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
    tick_start = (ccnt_t)seL4_GetMR(0);

    /*syn with the spy*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);

    SEL4BENCH_READ_CCNT(tick_start);  
 

    for(int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        newTimeSlice(); 
       
        if (random() % 2) { 
            /*do nothing*/
            *share_vaddr = 0; 
        }
        else {
            /*call 3 APIs, in order to create a large footprint*/
            do_timing_api(timing_tcb, &caps); 
            do_timing_api(timing_signal, &caps); 
            do_timing_api(timing_poll, &caps); 

            *share_vaddr = 1;
        }
    }

    while (1); 
    return 0;
}

