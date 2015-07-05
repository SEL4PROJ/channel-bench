/*
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#
*/

/*This file contains the benchmarks for measuring the indirect cost
 of flushing caches. The working set size of this thread is configured 
 at Kconfig.*/

#include <autoconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4bench/sel4bench.h>

#include "../../bench_common.h"
#include "bench.h"


static char flush_buf[CONFIG_BENCH_CACHE_BUFFER]; 
static uint32_t probe_size = CONFIG_BENCH_FLUSH_START;
static sel4bench_counter_t measure_overhead[OVERHEAD_RUNS];

static inline int overhead_stable(sel4bench_counter_t *array) {
    for (int i = 1; i < OVERHEAD_RUNS; i++) {
        if (array[i] != array[i - 1]) {
            return 0;
        }
    }
    return 1;
}


static inline void overhead(void) {

    /*measure the overhead of benchmarking method*/
    sel4bench_counter_t start, end; 

    for (int i = 0; i < OVERHEAD_RETRIES; i++) {
        for (int j = 0; j < OVERHEAD_RUNS; j++) { 
            FENCE(); 
            for (int k = 0; k < WARMUPS; k++) { 
                start = sel4bench_get_cycle_count(); 
                end = sel4bench_get_cycle_count(); 
            }
            FENCE(); 
            measure_overhead[j] = end - start;
        }

        if (overhead_stable(measure_overhead)) break; 
    }

}

static inline sel4bench_counter_t walk_buffer(void) {

    sel4bench_counter_t start, end; 

    start = sel4bench_get_cycle_count(); 

    for (int i = 0; i < probe_size; i += 
            CONFIG_BENCH_CACHE_LINE_SIZE) {

        flush_buf[i] ^= 0xff; 
    }
    end = sel4bench_get_cycle_count(); 
    

    return end - start;
}



seL4_Word bench_flush(void *record) {

    /*recording the result at buffer, only return the result*/
    sel4bench_counter_t *r_buf = (sel4bench_counter_t*)record; 
    uint32_t n_flush = 0, n_runs = CONFIG_BENCH_FLUSH_RUNS + WARMUPS;

    /*measure the overhead of read timestamp*/
    overhead();

    while (probe_size <= CONFIG_BENCH_CACHE_BUFFER) {

        FENCE();

        while (n_flush++ < n_runs) {

            *r_buf = walk_buffer(); 

            r_buf++;
            /*using seL4 yield to triger a context switch, 
              seL4 will do the cache flush.*/
            seL4_Yield(); 
        }

        FENCE();
        n_flush = 0;
        probe_size *= 2;
    }

    /*report the overhead of measure timestamp*/
    for (int i = 0; i < OVERHEAD_RUNS; i++) {
        *r_buf = measure_overhead[i];
        r_buf++;
    }
    return BENCH_SUCCESS;

}
