/*ipc benchmarks*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>

#ifdef CONFIG_BENCH_IPC 

#define NOPS ""
#include "ipc.h"


/*interfaces*/
seL4_Word ipc_rt_reply_wait_func(seL4_CPtr ep, seL4_CPtr result_ep); 
seL4_Word ipc_rt_call_func(seL4_CPtr ep, seL4_CPtr result_ep); 


#define MEASURE_OVERHEAD(op, dest, decls) do { \
    uint32_t i; \
    for (i = 0; i < IPC_OVERHEAD_RETRIES; i++) { \
        uint32_t j; \
        for (j = 0; j < IPC_RUNS; j++) { \
            uint32_t k; \
            decls; \
            ccnt_t start, end; \
            FENCE(); \
            for (k = 0; k < IPC_WARMUPS; k++) { \
                READ_COUNTER_BEFORE(start); \
                op; \
                READ_COUNTER_AFTER(end); \
            } \
            FENCE(); \
            dest[j] = end - start; \
        } \
        if (results_stable(dest)) break; \
    } \
} while(0)


#undef IPC_BENCH_PRINTOUT
static ipc_rt_result_t *rt_v;

/*list of functions*/
ipc_bench_func ipc_funs[IPC_ALL] = {
ipc_rt_call_func, 
ipc_rt_reply_wait_func};

struct ipc_overhead_t ipc_overhead_num; 

void print_pmu_results(ccnt_t *r) {


    for (int j = 0; j < BENCH_PMU_COUNTERS; j++) 
        printf("\t"CCNT_FORMAT, r[j]);

    printf("\n"); 
}



static ccnt_t results_min(ccnt_t *array) {
    uint32_t i;
    ccnt_t min = array[0];
    for (i = 1; i < IPC_RUNS; i++) {
        if (array[i] < min)
            min = array[i];
    }
    return min;
}


static inline  void print_all(ccnt_t *array) {
    uint32_t i;
    for (i = 0; i < IPC_RUNS; i++) {
        printf("\t"CCNT_FORMAT"\n", array[i]);
    }
}


static int results_stable(ccnt_t *array) {
    uint32_t i;
    for (i = 1; i < IPC_RUNS; i++) {
        if (array[i] != array[i - 1]) {
            return 0;
        }
    }
    return 1;
}

static int check_overhead(struct ipc_overhead_t *results) {
    ccnt_t call_overhead, reply_wait_overhead;
    if (!results_stable(results->call_overhead)) {
#ifdef IPC_BENCH_PRINTOUT
 
        printf("Benchmarking overhead of a call is not stable! Cannot continue\n");
       print_all(results->call_overhead);
#endif 
#ifndef ALLOW_UNSTABLE_OVERHEAD
        return 0;
#endif
    }
    if (!results_stable(results->reply_wait_overhead)) {
#ifdef IPC_BENCH_PRINTOUT

        printf("Benchmarking overhead of a reply wait is not stable! Cannot continue\n");
        print_all(results->reply_wait_overhead);
#endif 
#ifndef ALLOW_UNSTABLE_OVERHEAD
        return 0;
#endif
    }
    call_overhead = results_min(results->call_overhead);
    reply_wait_overhead = results_min(results->reply_wait_overhead);
    /* Take the smallest overhead to be our benchmarking overhead */
    if (call_overhead < reply_wait_overhead) {
        results->call_reply_wait_overhead = call_overhead;
    } else {
        results->call_reply_wait_overhead = reply_wait_overhead;
    }
    
    return 1;
}

void ipc_measure_overhead(seL4_CPtr reply_ep, struct ipc_overhead_t *results) {
 
    MEASURE_OVERHEAD(DO_NOP_CALL(0, tag),
                     results->call_overhead,
                     seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0));
    MEASURE_OVERHEAD(DO_NOP_REPLY_RECV(0, tag, 0),
                     results->reply_wait_overhead,
                     seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0));
 
    if (check_overhead(results)) {
        COMPILER_BARRIER; 
        rt_v->call_reply_wait_overhead = results->call_reply_wait_overhead;
        COMPILER_BARRIER;
    }

}

/*round trip ipc performance testing, only report at the client side
 (call)*/
seL4_Word ipc_rt_call_func(seL4_CPtr ep, seL4_CPtr result_ep) {

    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0); 
#ifdef CONFIG_BENCH_PMU_COUNTER
    ccnt_t pmu_start[BENCH_PMU_COUNTERS] = {0}; 
    ccnt_t pmu_end[BENCH_PMU_COUNTERS] = {0}; 
#endif 
    FENCE(); 
    for (int i  = 0; i < IPC_RUNS; i++) {
        
        for (int j = 0; j < IPC_WARMUPS; j++) { 
#ifdef CONFIG_BENCH_PMU_COUNTER 
            sel4bench_get_counters(BENCH_PMU_BITS, pmu_start);  
#endif
            READ_COUNTER_BEFORE(start); 
            DO_REAL_CALL(ep, tag);
            READ_COUNTER_AFTER(end); 
#ifdef CONFIG_BENCH_PMU_COUNTER 
            sel4bench_get_counters(BENCH_PMU_BITS, pmu_end);  
#endif
        } 

        DO_REAL_SEND(ep, tag); 
        COMPILER_BARRIER; 
        rt_v->call_rt_time[i] = end - start; 
#ifdef CONFIG_BENCH_PMU_COUNTER 
        for (int pc = 0; pc < BENCH_PMU_COUNTERS; pc++) 
            rt_v->pmu_value[i][pc] = pmu_end[pc] - pmu_start[pc];  
#endif
 
        COMPILER_BARRIER; 
    }
    FENCE();
    /*ping the root task, test is done*/
    seL4_Send(result_ep, tag); 
    return 0; 
}

/*round trip ipc performance testing*/
seL4_Word ipc_rt_reply_wait_func(seL4_CPtr ep, seL4_CPtr result_ep) { 
    
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0); 
    seL4_Word badge UNUSED; 
    FENCE();
    
    for (int i = 0; i < IPC_RUNS; i++) {
    
        DO_REAL_RECV(ep, badge); 
        for (int j = 0; j < IPC_WARMUPS; j++) { 
            DO_REAL_REPLY_RECV(ep, tag, badge);
        } 
    }
    FENCE(); 
    return 0; 
}

seL4_Word ipc_bench(seL4_CPtr result_ep, seL4_CPtr test_ep, int test_n,
        void *record_vaddr) {
    
    rt_v = (ipc_rt_result_t *)record_vaddr; 

    if (test_n == IPC_RT_CALL) {
        ipc_measure_overhead(result_ep, &ipc_overhead_num); 
    }

    assert(test_n < IPC_ALL);
    assert(ipc_funs[test_n]);

    ipc_funs[test_n](test_ep, result_ep);
 
    return BENCH_SUCCESS;
}
#endif 
