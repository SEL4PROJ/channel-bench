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
#include "../mastik_common/vlist.h"
#include "cachemap.h"
#include "pp.h"
#include "../mastik_common/low.h"
#include "search.h"

#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>


static void visit_page(cachemap_t cm, int secret) {
    
    for (int i = 0; i < secret; i++) {

        /*the upper bits select the page*/
        vlist_t vl = cm->sets[i >> 6]; 
        
        /*16 is the associativity*/
        int l = 16; 
        if (l > vl_len(vl)) 
            l = vl_len(vl); 

        /*touch the page*/
        for (int j = 0; j < l; j++) {

            char *p = (char *)vl_get(vl, j); 
            /*the lower bits slect the set within a page*/
            p[(i & 0x3f) << 6] = 0; 
        }
    }
}

      
static void warmup(cachemap_t cm) {

    /*warming up the testing platform with randomized 
      cache accessing sequence*/
    int total_sec = cm->nsets * 64 + 1;
    int secret; 
   
    for (int i = 0; i < NUM_KD_WARMUP_ROUNDS; i++) {
        secret = random() % total_sec; 

        visit_page(cm, secret);
    }
}




int l3_kd_trojan(bench_env_t *env) {
    seL4_MessageInfo_t info;
    int total_sec, secret;
    bench_args_t *args = env->args;

    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 

#ifdef CONFIG_BENCH_COVERT_L2_KERNEL_SCHEDULE 
    return l1_trojan(env); 
#endif  /*using the L2 probing attack for the benchmark*/ 

    cachemap_t cm = map();
    assert(cm); 

//    printf("cm nsets %d\n", cm->nsets);
    /*to root task*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    warmup(cm);

    /*to SPY*/
    seL4_Send(args->ep, info);

    /*nprobing sets is 64 (16 colour * 4 cores) on a coloured platform
     and 128 (32 colour * 4 cores) on a non-coloured platform
     secret represents the number of cache sets in total, 64 sets in a 
     page, all together nsets * 64.*/

    total_sec = cm->nsets * 64 + 1;


    for (int n = 0; n < CONFIG_BENCH_DATA_POINTS; n++) {

        newTimeSlice();

        secret = random() % total_sec;  
        visit_page(cm, secret);

        /*update the secret read by low*/ 
        *share_vaddr = secret; 

    }

    for(;;) {
        /*never return*/

    }
 
}

/*the cache flushing benchmark on LLC: 
 testing the cost of flushing the LLC cache*/
int llc_attack_flush(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    bench_args_t *args = env->args; 

    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    cachemap_t cm = map();
    assert(cm); 
    
    uint32_t secret = cm->nsets * 64; 


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
        visit_page(cm, secret);

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

