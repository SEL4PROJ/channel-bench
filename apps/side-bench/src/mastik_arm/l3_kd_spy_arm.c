/*this file contains the covert channel attack using the 
  kernel context switching time*/

/*the way this attack work: 
 trying to build contentions on L1 I and D caches
 observing the cost of context switching by timestamping on the cycle counter*/

#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "../../../bench_common.h"
#include "../mastik_common/low.h"
#include "../mastik_common/l1i.h"
#include "../ipc_test.h"
#include "l3_arm.h"
#include "bench_types.h"


/*256 cache sets on L1 I and L1 Data*/
//#define KD_TROJAN_LINES     256
/*the 256 K buffer to access*/
#define KD_TROJAN_LINES 256


uint32_t prev_sec[CONFIG_BENCH_DATA_POINTS];
uint32_t cur_sec[CONFIG_BENCH_DATA_POINTS];
uint32_t starts[CONFIG_BENCH_DATA_POINTS];
uint32_t curs[CONFIG_BENCH_DATA_POINTS];
uint32_t prevs[CONFIG_BENCH_DATA_POINTS];

/*accessing N number of L1 D cache sets*/
void data_access(char *buf, uint32_t sets) {

    for (int s = 0; s < sets; s++) {
        for (int i = 0; i < L1_ASSOCIATIVITY; i++) {

            access(buf + s * L1_CACHELINE + i * L1_STRIDE);
        }
    }

} 

int l3_kd_trojan(bench_env_t *env) {

    uint32_t secret;
    uint32_t cur, prev;
    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 

    uint64_t  monitored_mask[I_MONITOR_MASK] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};
    
    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
    assert(l1i_1); 

    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);

#ifdef CONFIG_DEBUG_BUILD
    printf("trojan: Got %d sets\n", nsets);
#endif

    uint16_t *results = malloc(sizeof (uint16_t) * L1I_SETS); 
    assert(results);
    char *l1d_buf = malloc(L1_PROBE_BUFFER);
    assert(l1d_buf);
    uint16_t *l3_kd_results = malloc(sizeof (uint16_t) * nsets); 
    assert(l3_kd_results);

    /*receive the shared address to record the secret*/
    uint32_t volatile *share_vaddr = args->shared_vaddr; 

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*ready to do the test*/
    seL4_Send(args->ep, info);

    secret = 0; 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        READ_COUNTER_ARMV7(cur);
        prev = cur;
       
        while (cur - prev < KERNEL_SCHEDULE_TICK_LENGTH) {
            prev = cur; 

            READ_COUNTER_ARMV7(cur); 
        }
        
#ifndef CONFIG_BENCH_DATA_SEQUENTIAL 
        secret = random() % (KD_TROJAN_LINES + 1); 
#endif
        /*using the LLC to probe*/
        l3_unmonitorall(l3);
        for (int s = 0; s < secret; s++) {
            /*avoid probing on the syn line*/
            if (s % l3->groupsize == 0) 
                continue; 
            l3_monitor(l3, s);
        }
        /*do simple probe, monitor already touches the caches by linking the cache lines*/
        //l3_probe(l3, l3_kd_results); 

        /*update the secret read by low*/ 
        *share_vaddr = secret; 
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
        if (++secret == KD_TROJAN_LINES + 1)
            secret = 0; 
#endif 
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
    uint32_t volatile *secret = args->shared_vaddr; 

    starts[0] = 1;
    curs[0] = 1;
    prevs[0] = 1;
    uint32_t start, cur; 
    READ_COUNTER_ARMV7(start);
    uint32_t prev = start;
    uint32_t prev_s = *secret; 

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
       
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS;) {
         FENCE(); 


        READ_COUNTER_ARMV7(cur);
        if (cur - prev > KERNEL_SCHEDULE_TICK_LENGTH) {
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


