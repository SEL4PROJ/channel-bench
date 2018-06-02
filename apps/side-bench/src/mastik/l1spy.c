#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "low.h"
#include "l1.h"
#include "bench_common.h"
#include "bench_types.h"
#include "bench_helper.h"



int l1_trojan(bench_env_t *env) {

    seL4_MessageInfo_t info;

    int secret = 0; 

    bench_args_t *args = env->args; 
    
    /*buffer size 32K L1 cache size
      512 cache lines*/
    char *data = malloc(L1_PROBE_BUFFER);
    assert(data); 
    data = (char*)ALIGN_PAGE_SIZE(data);

    uint32_t *share_vaddr = args->shared_vaddr; 

    /*manager: trojan is ready*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*syn with spy*/
    seL4_Send(args->ep, info);


    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        /*waiting for a system tick*/
        newTimeSlice();
        secret = random() % (L1_SETS + 1);

        l1d_data_access(data, secret);

        /*update the secret read by low*/ 
        *share_vaddr = secret; 

    }
    while (1);

    return 0;
}

int l1_spy(bench_env_t *env) {
    seL4_Word badge;
    seL4_MessageInfo_t info;

#ifdef CONFIG_BENCH_COVERT_L1D 
    uint64_t monitored_mask[MONITOR_MASK] = {~0LLU};
#else 
    /*for the L2 cache attack, with larger cache set*/
    uint64_t monitored_mask[MONITOR_MASK] = {~0LLU, ~0LLU,~0LLU, ~0LLU,
        ~0LLU, ~0LLU,~0LLU, ~0LLU,};
#endif 

    ccnt_t UNUSED pmu_start[BENCH_PMU_COUNTERS]; 
    ccnt_t UNUSED pmu_end[BENCH_PMU_COUNTERS]; 

    bench_args_t *args = env->args; 

    l1info_t l1_1 = l1_prepare(monitored_mask);
    uint16_t *results = malloc(l1_nsets(l1_1)*sizeof(uint16_t));


    /*the record address*/
    struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr;
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        newTimeSlice();

#ifdef CONFIG_MANAGER_PMU_COUNTER
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_start);  
#endif 

        l1_probe(l1_1, results);

#ifdef CONFIG_MANAGER_PMU_COUNTER 
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_end);
        /*loading the pmu counter value */
        for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
            r_addr->pmu[i][counter] = pmu_end[counter] - pmu_start[counter]; 

#endif
        /*result is the total probing cost
          secret is updated by trojan in the previous system tick*/
        r_addr->result[i] = 0; 
        r_addr->sec[i] = *secret; 

        for (int j = 0; j < l1_nsets(l1_1); j++) 
            r_addr->result[i] += results[j];

    }

    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}


int l1_cache_flush(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    uint64_t monitored_mask[1] = {~0LLU};

    bench_args_t *args = env->args; 

    l1info_t l1_1 = l1_prepare(monitored_mask);

    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);

    /*warming up*/
    for (int i = 0; i < BENCH_WARMUPS; i++) {
        start = sel4bench_get_cycle_count(); 
        l1_prime(l1_1); 
        end = sel4bench_get_cycle_count(); 

        seL4_Yield();
    }
    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {

        start = sel4bench_get_cycle_count(); 
        l1_prime(l1_1); 
        end = sel4bench_get_cycle_count(); 

        /*ping kernel for taking the measurements in kernel
          a context switch is invovled, switching to the idle user-level thread*/
        seL4_Yield(); 
        r_addr->costs[i] = end - start - overhead; 
    }

    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}
