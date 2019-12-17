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
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_helper.h>
#include <channel-bench/bench_types.h>
#include "../mastik_common/low.h"
#include "../mastik_common/l1i.h"

int l1i_trojan(bench_env_t *env) {

    uint32_t secret, index, shadow;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 

    uint64_t  monitored_mask[I_MONITOR_MASK] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};

    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
    assert(l1i_1); 

    uint16_t *results = malloc(sizeof (uint16_t) * L1I_SETS); 
    assert(results);


    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 
    *share_vaddr = 0; 

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*ready to do the test*/
    seL4_Send(args->ep, info);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();

        secret = random() % (L1I_SETS  + 1); 

        /*using L1 I cache sets to probe */
        for (int s = 0; s < I_MONITOR_MASK; s++ ) {
            monitored_mask[s] = 0;        
        }

        index = shadow = 0; 
        while (shadow < secret) {

            monitored_mask[index] |= 1ULL << (shadow % 64); 

            if (!(++shadow % 64)) 
                index++; 
        }

        l1i_set_monitored_set(l1i_1, monitored_mask);
        l1i_probe(l1i_1, results); 
        /*update the secret read by low*/ 
        *share_vaddr = secret; 
#ifdef CONFIG_BENCH_COVERT_L1I_REWRITE
        l1i_rewrite(l1i_1);
#endif 

    }

    while (1);

    return 0;

}


int l1i_spy(bench_env_t *env) {

    seL4_Word badge;
    seL4_MessageInfo_t info;
    uint32_t start, after;
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    ccnt_t volatile pmu_start; 
    ccnt_t volatile pmu_end; 
#endif
    bench_args_t *args = env->args; 

    uint64_t  monitored_mask[I_MONITOR_MASK] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};

    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
    assert(l1i_1); 

    uint16_t *results = malloc(sizeof (uint16_t) * L1I_SETS); 
    assert(results);

    struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();

#ifdef CONFIG_MANAGER_PMU_COUNTER 
        pmu_start = sel4bench_get_counter(0);  
#endif 
        SEL4BENCH_READ_CCNT(start); 

        l1i_probe(l1i_1, results); 

        SEL4BENCH_READ_CCNT(after);

#ifdef CONFIG_MANAGER_PMU_COUNTER 
        pmu_end = sel4bench_get_counter(0);  
#endif 

        r_addr->result[i] = after - start; 

#ifdef CONFIG_MANAGER_PMU_COUNTER 
        /*loading the pmu counter value */
        for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
            r_addr->pmu[i][counter] = pmu_end - pmu_start; 

#endif 
        /*result is the total probing cost
          secret is updated by trojan in the previous system tick*/
        r_addr->sec[i] = *secret; 
    }

#ifdef CONFIG_BENCH_COVERT_L1I_REWRITE
    l1i_rewrite(l1i_1);
#endif 

    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;

}
