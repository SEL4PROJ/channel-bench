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
#define BENCH_OVERHEAD_RETRIES 4

/* The fence is designed to try and prevent the compiler optimizing across code boundaries
   that we don't want it to. The reason for preventing optimization is so that things like
   overhead calculations aren't unduly influenced */
#define FENCE() asm volatile("" ::: "memory")


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
#ifdef CONFIG_BENCH_SPLASH
static inline bool measure_splash_overhead(ccnt_t *overhead) {

    /*measure the overhead of benchmarking method*/
    uint64_t start, end; 

    for (int i = 0; i < BENCH_OVERHEAD_RETRIES; i++) {
        for (int j = 0; j < BENCH_OVERHEAD_RUNS; j++) { 
            FENCE(); 
            for (int k = 0; k < BENCH_WARMUPS; k++) { 
#ifdef CONFIG_PLAT_IMX6
            start = seL4_GlobalTimer(); 
            end = seL4_GlobalTimer(); 
#else 

                start = sel4bench_get_cycle_count(); 
                end = sel4bench_get_cycle_count(); 
#endif 
                FENCE(); 
                local_overhead[j] = (ccnt_t)(end - start);
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
#endif 
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

#define ALLOW_UNSTABLE_OVERHEAD 

#ifdef CONFIG_ARCH_X86

static inline uint32_t rdtscp() {
  uint32_t rv;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}
#endif /*CONFIG_ARCH_X86*/

#ifdef CONFIG_ARCH_RISCV

static inline uint32_t rdtime() {
  uint32_t rv;
  asm volatile ("rdcycle %0": "=r" (rv) ::);
  return rv;
}

static inline uint32_t rdtscp() { return rdtime(); }
#endif /* CONFIG_ARCH_RISCV */

static inline int wait_init_msg_from(seL4_CPtr endpoint) {

    seL4_Word badge; 
    seL4_MessageInfo_t info; 

    info = seL4_Recv(endpoint, &badge); 

    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault); 
    assert(seL4_MessageInfo_get_length(info) == 1); 

    if (seL4_GetMR(0) == BENCH_INIT_MSG) 
        return BENCH_SUCCESS; 
    else 
        return BENCH_FAILURE; 
}
