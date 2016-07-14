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
#include "./mastik/low.h"
#include "./mastik/l1.h"
#include "../../bench_common.h"
#include "bench.h"

#ifdef CONFIG_BENCH_CACHE_FLUSH 

static volatile uint32_t flush_buf[CONFIG_BENCH_CACHE_BUFFER] __attribute__((aligned (4096)));
static volatile char user_flush[32768] __attribute__((aligned (4096)));

static uint32_t probe_size = CONFIG_BENCH_FLUSH_START;
static sel4bench_counter_t measure_overhead[OVERHEAD_RUNS];
static volatile uint32_t readc, readt; 
static uint16_t *l1_probe_result; 
static l1info_t l1_1, l1_2, l1_3;


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

                //start = sel4bench_get_cycle_count(); 
               // end = sel4bench_get_cycle_count(); 
                start = rdtscp(); 
                end = rdtscp();
            }
            FENCE(); 
            measure_overhead[j] = end - start;
        }

        if (overhead_stable(measure_overhead)) break; 
    }

}

static inline void flush_buffer(void) {

    for (int i = 0; i < 32768; i+= 64)  {
        readc =  user_flush[i];
        asm volatile("" ::"r"(readc):"memory");
    }

}

static inline void prepare_buffer_sequential(void) {

    for (int i = 0; i < probe_size; i+= CONFIG_BENCH_FLUSH_STEPPING ) {
        flush_buf[i] = i; 
    }

}

static inline void prepare_buffer(void) {


    for (int i = 0; i < probe_size; i+= CONFIG_BENCH_FLUSH_STEPPING ) {
        flush_buf[i] = i; 
    }
    for (int i = 0; i < probe_size; i+= CONFIG_BENCH_FLUSH_STEPPING) {
        
        int temp = random() % (probe_size - i) + i ;
        temp = temp / CONFIG_BENCH_FLUSH_STEPPING * CONFIG_BENCH_FLUSH_STEPPING;

        int temp_buf = flush_buf[temp]; 
        flush_buf[temp] = i; 
        flush_buf[i] = temp_buf; 
    }
}

static inline void prepare_probe_buffer(void) {

    l1_1 = l1_prepare(~0LLU);
    l1_2 = l1_prepare(~0LLU);
    l1_3 = l1_prepare(~0LLU);

    l1_probe_result = malloc(l1_nsets(l1_2)*sizeof(uint16_t));
    assert(l1_probe_result != NULL);  
    /*using only half of the L1 for the L1_1*/
    l1_set_monitored_set(l1_1, 0xffffffffLLU);

}

static inline sel4bench_counter_t walk_probe_buffer(l1info_t p_buf) {


    l1_probe(p_buf, l1_probe_result); 

    return l1_probe_result[0];
}

static inline sel4bench_counter_t walk_buffer(void) {

    sel4bench_counter_t start, end; 
    uint32_t lines = 0; 
    readc = 0; 

    FENCE();

    start = sel4bench_get_cycle_count(); 

#if 0

    for (int i = 0; i < probe_size; i += CONFIG_BENCH_FLUSH_STEPPING)
    {


        readc = flush_buf[i];
        asm volatile("" ::"r"(readc):"memory");
    }
#endif 
#if 1
    while (lines < probe_size / CONFIG_BENCH_FLUSH_STEPPING) {


#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
 
        flush_buf[readc + 1] ^= 0xff;  
#endif
        readc = flush_buf[readc];
        
        lines++;
    }
#endif 
    end = sel4bench_get_cycle_count(); 
    FENCE();

    return end - start;
}



seL4_Word bench_flush(void *record) {

    /*recording the result at buffer, only return the result*/
    sel4bench_counter_t *r_buf = (sel4bench_counter_t*)record; 
    uint32_t n_flush = 0, n_runs = CONFIG_BENCH_FLUSH_RUNS + WARMUPS;

    /*measure the overhead of read timestamp*/
    overhead();

    prepare_probe_buffer(); 

    while (probe_size <= CONFIG_BENCH_CACHE_BUFFER) {

        /*prepare the buffer, randomise it*/
        //prepare_buffer();
        /*prepare buffer sequential*/
        //prepare_buffer_sequential();
        FENCE();

        while (n_flush++ < n_runs) {

#ifndef CONFIG_BENCH_CACHE_FLUSH_NONE 
            /*using seL4 yield to jump into kernel
              seL4 will do the cache flush. according to the configured type
             because the L1 cache is small, only jumping into the kernel may be
             enough to pollute the cache*/
            seL4_Yield();
#endif
           // flush_buffer();

            /*walk the sequential buffer*/
            //*r_buf = walk_buffer(); 
            /*walk the probe buffer*/
            if (probe_size == CONFIG_BENCH_FLUSH_START) {
                *r_buf = walk_probe_buffer(l1_1);
            }
            else if (probe_size == CONFIG_BENCH_FLUSH_START * 2) {
                *r_buf = walk_probe_buffer(l1_2); 
            } else {

                *r_buf = walk_probe_buffer(l1_2); 
                *r_buf += walk_probe_buffer(l1_3);
            }

            r_buf++;

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
#endif 
