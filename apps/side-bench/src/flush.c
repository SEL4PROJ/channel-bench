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

static sel4bench_counter_t walk_buffer(void) {

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
    uint32_t n_flush = 0;

    while (probe_size <= CONFIG_BENCH_CACHE_BUFFER) {

        while (n_flush++ < CONFIG_BENCH_FLUSH_RUNS) {

            *r_buf = walk_buffer(); 
           
            r_buf++;
            /*using seL4 yield to triger a context switch, 
              seL4 will do the cache flush.*/
            seL4_Yield(); 
        }

        n_flush = 0;
        probe_size *= 2;
    }
    return BENCH_SUCCESS;

}
