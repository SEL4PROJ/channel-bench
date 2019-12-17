/*this file contains the covert channel attack using the 
  kernel context switching time*/

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
#include "../mastik_common/l1i.h"
#include "l3_arm.h"


int l3_kd_trojan(bench_env_t *env) {

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
    uint32_t *share_vaddr = (uint32_t*)args->shared_vaddr; 

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*syn with SPY: ready to do the test*/
    seL4_Send(args->ep, info);

    secret = 0; 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();
        
        secret = random() % (nsets + 1); 
        
        /*using the LLC to probe
         probing range: 0 sets to N sets total N sets*/
        l3_unmonitorall(l3);

        for (int s = 0; s < secret; s++) {
            /*do simple probe, 
              l3_monitor already touches the caches by linking the cache lines
             pushing number of cache lines in a set (associativity)*/
            l3_monitor(l3, s);
        }

        /*update the secret read by low*/ 
        *share_vaddr = secret;

    }

    FENCE(); 

    while (1);

    return 0;
}



int l3_kd_spy(bench_env_t *env) {

    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 
 
    struct bench_kernel_schedule *r_addr = (struct bench_kernel_schedule *)args->record_vaddr; 
    /*the shared address*/
    uint32_t *secret = (uint32_t *)args->shared_vaddr; 

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
    
    
    uint32_t start, cur; 
    uint32_t prev_s = *secret; 
    
    SEL4BENCH_READ_CCNT(start);
    uint32_t prev = start;

      
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS;) {
         FENCE(); 


        SEL4BENCH_READ_CCNT(cur);
        if (cur - prev >= TS_THRESHOLD) {
            /*a new tick*/
            r_addr->prevs[i] = prev;
            r_addr->starts[i] = start;
            r_addr->curs[i] = cur;
            /*prev secret affect online time (prevs - starts)*/
            r_addr->prev_sec[i] = prev_s;
            /*at the beginning of this tick, update the secret*/
            /*cur secrete affect offline time (curs - prevs)*/
            prev_s = r_addr->cur_sec[i] = *secret;
            start = cur;
            i++;

        }

        prev = cur; 
        FENCE(); 
    }
        

    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}

/*the cache flushing benchmark on LLC: 
 testing the cost of flushing the LLC cache*/
int llc_attack_flush(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    bench_args_t *args = env->args; 
    
    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;
    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);
    l3_unmonitorall(l3);

    /*monitor all sets*/
    for (int s = 0; s < nsets; s++) {
        l3_monitor(l3, s);
    }
    
    uint16_t *results = malloc(sizeof (uint16_t) * nsets); 
    assert(results);

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);

 
    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {
        /*waiting for a system tick*/
        newTimeSlice();
        /*start measuing*/
        seL4_SetMR(100, 0x12345678); 

        start = sel4bench_get_cycle_count(); 
        l3_probe(l3, results);

        end = sel4bench_get_cycle_count(); 
        r_addr->costs[i] = end - start - overhead; 
    }
    newTimeSlice();
    seL4_SetMR(100, 0); 
 
    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}

