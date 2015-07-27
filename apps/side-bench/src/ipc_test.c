/*ipc benchmarks*/
#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include "../../bench_common.h"
#include "ipc_test.h"

#ifdef CONFIG_BENCH_IPC 
#if defined(CCNT32BIT)
static void send_result(seL4_CPtr ep, ccnt_t result) {
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, result);
    seL4_Send(ep, tag);
}
#elif defined(CCNT64BIT)
static void send_result(seL4_CPtr ep, ccnt_t result) {
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, (uint32_t)(result >> 32ull));
    seL4_SetMR(1, (uint32_t)(result & 0xFFFFFFFF));
    seL4_Send(ep, tag);
}
#else
#error Unknown ccnt size
#endif

#undef IPC_BENCH_PRINTOUT
#ifdef CONFIG_BENCH_PMU_COUNTER
static ipc_pmu_t *pmu_v; 
#endif 
/*list of functions*/
ipc_bench_func ipc_funs[IPC_ALL] = {

ipc_call_func, ipc_call_func2, ipc_call_10_func, ipc_call_10_func2,

ipc_reply_wait_func, ipc_reply_wait_func2, ipc_reply_wait_10_func, ipc_reply_wait_10_func2, 
ipc_wait_func, 
ipc_send_func, 
NULL};

struct bench_results results; 

#if 0
ipc_bench_func ipc_bench_call[4] = {ipc_call_func, ipc_call_func2, ipc_call_10_func, ipc_call_10_func2};

ipc_bench_func ipc_bench_reply_wait[4] = {ipc_reply_wait_func, ipc_reply_wait_func2, ipc_reply_wait_10_func, ipc_reply_wait_10_func2};
#endif 

void print_pmu_results(sel4bench_counter_t *r) {


        for (int j = 0; j < BENCH_PMU_COUNTERS; j++) 
            printf("\t"CCNT_FORMAT, r[j]);

        printf("\n"); 
}

seL4_Word ipc_call_func(seL4_CPtr ep, seL4_CPtr result_ep) {

    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0); 
    dummy_seL4_Call(ep, tag);
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) { 
        READ_COUNTER_BEFORE(start); 
        DO_REAL_CALL(ep, tag); 
        READ_COUNTER_AFTER(end); 
#ifdef CONFIG_BENCH_PMU_COUNTER 
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_v->pmuc[IPC_CALL]); 
#endif
    } 
    FENCE();
    send_result(result_ep, end); 
    seL4_Send(ep, tag); 
    return 0; 

}



seL4_Word ipc_call_func2(seL4_CPtr ep, seL4_CPtr result_ep) {

    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0); 
    seL4_Call(ep, tag);
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) { 
#ifdef CONFIG_BENCH_PMU_COUNTER 
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_v->pmuc[IPC_CALL2]); 
#endif
        READ_COUNTER_BEFORE(start);
        DO_REAL_CALL(ep, tag); 
        READ_COUNTER_AFTER(end);
    } 
    FENCE();
    send_result(result_ep, start); 
    dummy_seL4_Send(ep, tag); 
    return 0; 

}


seL4_Word ipc_call_10_func(seL4_CPtr ep, seL4_CPtr result_ep) {

    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 10); 
    dummy_seL4_Call(ep, tag);
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) { 
        READ_COUNTER_BEFORE(start); 
        DO_REAL_CALL_10(ep, tag); 
        READ_COUNTER_AFTER(end); 
#ifdef CONFIG_BENCH_PMU_COUNTER 
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_v->pmuc[IPC_CALL_10]); 
#endif

    } 
    FENCE(); 
    send_result(result_ep, end); 
    seL4_Send(ep, tag); 
    return 0; 

}

seL4_Word ipc_call_10_func2(seL4_CPtr ep, seL4_CPtr result_ep) {

    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 10); 
    seL4_Call(ep, tag);
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) {
#ifdef CONFIG_BENCH_PMU_COUNTER 
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_v->pmuc[IPC_CALL2_10]); 
#endif

        READ_COUNTER_BEFORE(start); 
        DO_REAL_CALL_10(ep, tag); 
        READ_COUNTER_AFTER(end); 
    } 
    FENCE(); 
    send_result(result_ep, start); 
    dummy_seL4_Send(ep, tag); 
    return 0; 
}



seL4_Word ipc_reply_wait_func(seL4_CPtr ep, seL4_CPtr result_ep) { 
    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0); 
    seL4_Wait(ep, NULL); 
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) { 
#ifdef CONFIG_BENCH_PMU_COUNTER 
        sel4bench_get_counters(BENCH_PMU_BITS, 
                pmu_v->pmuc[IPC_REPLY_WAIT]); 
#endif
        READ_COUNTER_BEFORE(start); 
        DO_REAL_REPLY_WAIT(ep, tag); 
        READ_COUNTER_AFTER(end); 
    } 
    FENCE(); 
    send_result(result_ep, start); 
    dummy_seL4_Reply(tag); 
    return 0; 
}

seL4_Word ipc_reply_wait_func2(seL4_CPtr ep, seL4_CPtr result_ep) { 
    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0); 
    seL4_Wait(ep, NULL); 
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) { 
        READ_COUNTER_BEFORE(start);
        DO_REAL_REPLY_WAIT(ep, tag); 
        READ_COUNTER_AFTER(end); 
#ifdef CONFIG_BENCH_PMU_COUNTER 
        sel4bench_get_counters(BENCH_PMU_BITS, 
                pmu_v->pmuc[IPC_REPLY_WAIT2]); 
#endif
    } 
    FENCE(); 
    send_result(result_ep, end); 
    seL4_Reply(tag); 
    return 0; 
}


seL4_Word ipc_reply_wait_10_func(seL4_CPtr ep, seL4_CPtr result_ep) { 
    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 10); 
    seL4_Wait(ep, NULL); 
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) { 
        READ_COUNTER_BEFORE(start); 
        DO_REAL_REPLY_WAIT_10(ep, tag); 
        READ_COUNTER_AFTER(end); 
    } 
    FENCE(); 
    send_result(result_ep, start); 
    dummy_seL4_Reply(tag); 
    return 0; 
}

seL4_Word ipc_reply_wait_10_func2(seL4_CPtr ep, seL4_CPtr result_ep) { 
    uint32_t i; 
    ccnt_t start UNUSED, end UNUSED; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 10); 
    seL4_Wait(ep, NULL); 
    FENCE(); 
    for (i = 0; i < IPC_WARMUPS; i++) { 
        READ_COUNTER_BEFORE(start); 
        DO_REAL_REPLY_WAIT_10(ep, tag); 
        READ_COUNTER_AFTER(end); 
    } 
    FENCE(); 
    send_result(result_ep, end); 
    seL4_Reply(tag); 
    return 0; 
}


uint32_t ipc_wait_func(seL4_CPtr ep, seL4_CPtr result_ep) {
    uint32_t i;
    ccnt_t start UNUSED, end UNUSED;
    FENCE();
    for (i = 0; i < IPC_WARMUPS; i++) {
        READ_COUNTER_BEFORE(start);
        DO_REAL_WAIT(ep);
        READ_COUNTER_AFTER(end);
    }
    FENCE();
    DO_REAL_WAIT(ep);
    send_result(result_ep, end);
    return 0;
}

uint32_t ipc_send_func(seL4_CPtr ep, seL4_CPtr result_ep) {
    uint32_t i;
    ccnt_t start UNUSED, end UNUSED;
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    FENCE();
    for (i = 0; i < IPC_WARMUPS; i++) {
        READ_COUNTER_BEFORE(start);
        DO_REAL_SEND(ep, tag);
        READ_COUNTER_AFTER(end);
    }
    FENCE();
    send_result(result_ep, start);
    DO_REAL_SEND(ep, tag);
    return 0;

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

static int check_overhead(struct bench_results *results) {
    ccnt_t call_overhead, reply_wait_overhead;
    ccnt_t call_10_overhead, reply_wait_10_overhead;
    ccnt_t send_overhead, wait_overhead;
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
    if (!results_stable(results->send_overhead)) {
#ifdef IPC_BENCH_PRINTOUT

        printf("Benchmarking overhead of a send is not stable! Cannot continue\n");
        print_all(results->send_overhead);
#endif 
#ifndef ALLOW_UNSTABLE_OVERHEAD
        return 0;
#endif
    }
    if (!results_stable(results->wait_overhead)) {
#ifdef IPC_BENCH_PRINTOUT

        printf("Benchmarking overhead of a wait is not stable! Cannot continue\n");
        print_all(results->wait_overhead);
#endif 
#ifndef ALLOW_UNSTABLE_OVERHEAD
        return 0;
#endif
    }
    if (!results_stable(results->call_10_overhead)) {
#ifdef IPC_BENCH_PRINTOUT
        printf("Benchmarking overhead of a call is not stable! Cannot continue\n");
        print_all(results->call_10_overhead);
#endif 
#ifndef ALLOW_UNSTABLE_OVERHEAD
        return 0;
#endif
    }
    if (!results_stable(results->reply_wait_10_overhead)) {
#ifdef IPC_BENCH_PRINTOUT

        printf("Benchmarking overhead of a reply wait is not stable! Cannot continue\n");
        print_all(results->reply_wait_10_overhead);
#endif 
#ifndef ALLOW_UNSTABLE_OVERHEAD
        return 0;
#endif
    }
    call_overhead = results_min(results->call_overhead);
    reply_wait_overhead = results_min(results->reply_wait_overhead);
    call_10_overhead = results_min(results->call_10_overhead);
    reply_wait_10_overhead = results_min(results->reply_wait_10_overhead);
    send_overhead = results_min(results->send_overhead);
    wait_overhead = results_min(results->wait_overhead);
    /* Take the smallest overhead to be our benchmarking overhead */
    if (call_overhead < reply_wait_overhead) {
        results->call_reply_wait_overhead = call_overhead;
    } else {
        results->call_reply_wait_overhead = reply_wait_overhead;
    }
    if (send_overhead < wait_overhead) {
        results->send_wait_overhead = send_overhead;
    } else {
        results->send_wait_overhead = wait_overhead;
    }
    if (call_10_overhead < reply_wait_10_overhead) {
        results->call_reply_wait_10_overhead = call_10_overhead;
    } else {
        results->call_reply_wait_10_overhead = reply_wait_10_overhead;
    }

    return 1;
}

void ipc_measure_overhead(seL4_CPtr reply_ep, struct bench_results *results) {
    MEASURE_OVERHEAD(DO_NOP_CALL(0, tag),
                     results->call_overhead,
                     seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0));
    MEASURE_OVERHEAD(DO_NOP_REPLY_WAIT(0, tag),
                     results->reply_wait_overhead,
                     seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0));
    MEASURE_OVERHEAD(DO_NOP_SEND(0, tag),
                     results->send_overhead,
                     seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0));
    MEASURE_OVERHEAD(DO_NOP_WAIT(0),
                     results->wait_overhead,
                     {});
    MEASURE_OVERHEAD(DO_NOP_CALL_10(0, tag10),
                     results->call_10_overhead,
                     seL4_MessageInfo_t tag10 = seL4_MessageInfo_new(0, 0, 0, 10));
    MEASURE_OVERHEAD(DO_NOP_REPLY_WAIT_10(0, tag10),
                     results->reply_wait_10_overhead,
                     seL4_MessageInfo_t tag10 = seL4_MessageInfo_new(0, 0, 0, 10));
    if (check_overhead(results)) {

        send_result(reply_ep,  results->call_reply_wait_overhead);
        send_result(reply_ep, results->send_wait_overhead );
        send_result(reply_ep, results->call_reply_wait_10_overhead);
    }

}
seL4_Word ipc_bench(seL4_CPtr result_ep, seL4_CPtr test_ep, int test_n,
        void *record_vaddr) {


    if (test_n == IPC_OVERHEAD) {
        ipc_measure_overhead(result_ep, &results); 
        return BENCH_SUCCESS;
    }

#ifdef CONFIG_BENCH_PMU_COUNTER 

    pmu_v = record_vaddr; 
#endif 

    if (!ipc_funs[test_n])
        return BENCH_FAILURE;

    ipc_funs[test_n](test_ep, result_ep);
 
    return BENCH_SUCCESS;
}

#endif
