#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <simple/simple.h>
#include "../mastik_common/vlist.h"
#include "cachemap.h"
#include "pp.h"
#include "../mastik_common/low.h"
#include "search.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>


/*the following variable is used for side channel
 triggering the syscall thus leaving the cache footprint*/


#define SYN_TICK_MULTI_TROJAN          500000 
#define SYN_TICK_MULTI_SPY_OFFSET      50000

static inline void waiting_probe(ccnt_t start, int tick, bool spy) {

#if CONFIG_MAX_NUM_NODES > 1

    /*Probe on an offset (for this tick), which is relative to the start 
     of the attack. 
     the prime (trojan) is on every multiple tick length
     the probe (spy) is on 50000 after the prime*/

    ccnt_t offset = (tick + 1) * SYN_TICK_MULTI_TROJAN; 
    if (spy) 
        offset += SYN_TICK_MULTI_SPY_OFFSET;  

    ccnt_t current = rdtscp_64();

    while (current - start < offset) 
        current = rdtscp_64(); 
#else 
    newTimeSlice();
#endif

}


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

    //Find the L3 eviction set
    cachemap_t cm = map();

    //Individually find the probing set for each api
    for(int i = 0; i < timing_api_num; i++){

        probed[i] = search(cm, i, &caps);
    }


    remove_same_probesets(probed[timing_signal], probed[timing_tcb]); 
    remove_same_probesets(probed[timing_tcb], probed[timing_poll]); 
    remove_same_probesets(probed[timing_signal], probed[timing_poll]);

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
    tick_start = rdtscp_64();

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        waiting_probe(tick_start, i , true); 

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

    enum timing_api current_api; 
    
    /*root give global time*/
    info = seL4_Recv(reply_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
    tick_start = (ccnt_t)seL4_GetMR(0);

    /*syn with the spy*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);
    tick_start = rdtscp_64();

    for(int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        waiting_probe(tick_start, i , false); 

        current_api = random() % (timing_api_num + 1);  

        do_timing_api(current_api, &caps); 
        *share_vaddr = current_api; 
    }

    while (1); 
    return 0;
}

