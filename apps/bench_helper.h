/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */


#pragma once

#include <sel4bench/sel4bench.h>
#include "bench_common.h"

#define BENCH_WARMUPS 10
#define BENCH_OVERHEAD_RUNS 10
#define BENCH_OVERHEAD_RUNS 10
#define BENCH_OVERHEAD_RETRIES 4


static ccnt_t local_overhead[BENCH_OVERHEAD_RUNS]; 

static inline bool overhead_stable(ccnt_t *array) {

    for (int i = 1; i < BENCH_OVERHEAD_RUNS; i++) {
        if (array[i] != array[i - 1]) {
            return false;
        }
    }
    return true;
}

static inline ccnt_t find_min_overhead(ccnt_t *array) {

    ccnt_t min = array[0]; 

    for (int i = 1; i < BENCH_OVERHEAD_RUNS; i++) {
        if (array[i] < min)
            min = array[i]; 
    }
    return min; 
}


static inline bool measure_overhead(ccnt_t *overhead) {

    /*measure the overhead of benchmarking method*/
    ccnt_t start, end; 

    for (int i = 0; i < BENCH_OVERHEAD_RETRIES; i++) {
        for (int j = 0; j < BENCH_OVERHEAD_RUNS; j++) { 
            FENCE(); 
            for (int k = 0; k < BENCH_WARMUPS; k++) { 

                start = sel4bench_get_cycle_count(); 
                end = sel4bench_get_cycle_count(); 

                FENCE(); 
                local_overhead[j] = end - start;
            }
        }

        if (overhead_stable(local_overhead)) {
            *overhead = local_overhead[0];
            return true;
        }
    }

    *overhead = find_min_overhead(local_overhead);
    return false;
}

