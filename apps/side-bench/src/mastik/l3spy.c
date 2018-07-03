#include <autoconf.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <simple/simple.h>
#include "vlist.h"
#include "cachemap.h"
#include "pp.h"
#include "low.h"
#include "search.h"
#include "bench_common.h"
#include "bench_types.h"
#include "bench_helper.h"


/*the following variable is used for side channel
 triggering the syscall thus leaving the cache footprint*/
seL4_CPtr async_ep;        
seL4_CPtr fake_tcb;                            
seL4_Word sender;                          
seL4_CPtr self_tcb; 

#define TEST_COUNT 10


static void inline do_timing_api(enum timing_api api_no) {
    
    switch (api_no) {
        case timing_signal: 
            seL4_Signal(async_ep);
            break; 
        case timing_tcb: 
            seL4_TCB_SetPriority(fake_tcb, self_tcb, 0);
            break; 
        case timing_poll: 
            seL4_Poll(async_ep, &sender);
            break;
        case timing_api_num: 
        default: 
            break;
    }
}



int __attribute__((noinline)) test(pp_t pp, int recv, enum timing_api api_no) {
  
    int count = 0;

    pp_probe(pp);
    pp_probe(pp);
    pp_probe(pp);


    for (int i = 0; i < TEST_COUNT; i++) {
        
        if (recv){
            do_timing_api(api_no);
        }
        
        count <<= 1;
        if (pp_probe(pp) != 0)
            count++;
    }
    // If at most one is set - this is not a collision
    int c = count & (count - 1);
    c = c & (c - 1);
    if ((c & (c - 1))== 0)
        return 0;
    
    return 1;
}


int scan(pp_t pp, int offset, enum timing_api api_no) {
    if (test(pp, 0, api_no))
        return 0;
    /*if do not have conflict with the test code, 
      try to detect the conflict with seL4 Poll*/
    return test(pp, 1, api_no);
}

vlist_t search(cachemap_t cm, enum timing_api api_no) {

    vlist_t probed = vl_new();
    for (int line = 0; line < 4096; line += 64) {
        //printf("Searching in line 0x%02x\n", line / 64);
        // For every page...
        /*preparing the probing set 
          find one that do not have conflict with the test code
          but have conflict with the seL4 Poll service*/
        for (int p  = 0; p < cm->nsets; p++) {
            pp_t pp = pp_prepare(cm->sets[p] , L3_ASSOCIATIVITY, line);
            int t = scan(pp, line, api_no);
            if (t) {
                /*record target probing set*/
                vl_push(probed, pp);

#ifdef CONFIG_DEBUG_BUILD
                printf("Probed at %3d.%03x (%08p)\n", p, line, pp);
#endif

            }
        }
  }
  return probed;
}

/*remove any data from p_b if p_a has the same*/
static void remove_same_probesets(vlist_t p_a, vlist_t p_b) {

    for (int a = 0; a < vl_len(p_a); a++) {

        for (int b = 0; b < vl_len(p_b); b++) {

            if (vl_get(p_a, a) == vl_get(p_b, b)) 
                vl_del(p_b, b);
        }
    }
}


static inline uint32_t probing_sets(vlist_t set_list) {

    uint32_t result = 0; 

    for (int j = 0; j < vl_len(set_list); j++){
       result += pp_probe((pp_t)vl_get(set_list, j));
    }

    return result; 

}


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
  
    bench_args_t *args = env->args; 
    async_ep = args->notification_ep; 
    fake_tcb = args->fake_tcb; 

    self_tcb = simple_get_tcb(&env->simple); 

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

        probed[i] = search(cm, i);
    }


    remove_same_probesets(probed[timing_signal], probed[timing_tcb]); 
    remove_same_probesets(probed[timing_tcb], probed[timing_poll]); 
    remove_same_probesets(probed[timing_signal], probed[timing_poll]);


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

        probe_result->probe_seq[i] = *secret;
        /*probing on each API list*/
        probe_result->probe_results[i][timing_signal] = 
            probing_sets(probed[timing_signal]);  

        probe_result->probe_results[i][timing_tcb] = 
            probing_sets(probed[timing_tcb]);  

        probe_result->probe_results[i][timing_poll] = 
            probing_sets(probed[timing_poll]);  
        
    }

    //Signal root we finished
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(reply_ep, info);


    while (1);

    return 0;
}

int l3_trojan(bench_env_t *env) {
    bench_args_t *args = env->args; 

    async_ep = args->notification_ep; 
    fake_tcb = args->fake_tcb; 

    self_tcb = simple_get_tcb(&env->simple); 


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

        do_timing_api(current_api); 
        *share_vaddr = current_api; 
    }

    while (1); 
    return 0;
}

