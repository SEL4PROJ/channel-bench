/*this file contains the covert channel attack using the 
  kernel context switching time*/

#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "bench_common.h"
#include "bench_types.h"
#include "bench_helper.h"
#include "low.h"
#include "l1i.h"
#include "l3_arm.h"


uint32_t prev_sec[CONFIG_BENCH_DATA_POINTS];
uint32_t cur_sec[CONFIG_BENCH_DATA_POINTS];
uint32_t starts[CONFIG_BENCH_DATA_POINTS];
uint32_t curs[CONFIG_BENCH_DATA_POINTS];
uint32_t prevs[CONFIG_BENCH_DATA_POINTS];


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
            prevs[i] = prev;
            starts[i] = start;
            curs[i] = cur;
            /*prev secret affect online time (prevs - starts)*/
            prev_sec[i] = prev_s;
            /*at the beginning of this tick, update the secret*/
            /*cur secrete affect offline time (curs - prevs)*/
            prev_s = cur_sec[i] = *secret;
            start = cur;
            i++;

        }

        prev = cur; 
        FENCE(); 
    }
        
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        r_addr->prevs[i] = prevs[i]; 
        r_addr->starts[i] = starts[i]; 
        r_addr->curs[i] = curs[i]; 
        r_addr->prev_sec[i] = prev_sec[i];
        r_addr->cur_sec[i] = cur_sec[i];

    }  

    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}


