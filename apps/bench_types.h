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

#include <autoconf.h>
#include <vka/vka.h>
#include <allocman/allocman.h>
#include <sel4utils/vspace.h>
#include <simple/simple.h>
#include <sel4platsupport/timer.h>
#include <sel4bench/sel4bench.h>
#include "bench_common.h"

struct bench_l1 {
    /*L1 data/instruction cache 64 sets, the result contains the 
     total cost on probing L1 D/I cache*/
    uint32_t result[CONFIG_BENCH_DATA_POINTS];
    uint32_t sec[CONFIG_BENCH_DATA_POINTS];
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    uint32_t pmu[CONFIG_BENCH_DATA_POINTS][BENCH_PMU_COUNTERS]; 
#endif 
};

struct bench_kernel_schedule {
    ccnt_t prevs[CONFIG_BENCH_DATA_POINTS];
    ccnt_t starts[CONFIG_BENCH_DATA_POINTS];
    ccnt_t curs[CONFIG_BENCH_DATA_POINTS];
    uint32_t prev_sec[CONFIG_BENCH_DATA_POINTS];
    uint32_t cur_sec[CONFIG_BENCH_DATA_POINTS];
};


struct bench_timer_online {
    /*online time: prevs = starts*/
    ccnt_t  prevs[CONFIG_BENCH_DATA_POINTS];
    ccnt_t  starts[CONFIG_BENCH_DATA_POINTS];
};

struct bench_cache_flush {
    ccnt_t overhead; 
    ccnt_t costs[BENCH_CACHE_FLUSH_RUNS];
};

/*the argument passes to the benchmarking thread*/
typedef struct {

    int test_num; 

    uintptr_t shared_vaddr; /*pages shared between benchmarking thread*/ 
    size_t shared_pages;

    uintptr_t record_vaddr; /*pages used for recording result*/
    size_t record_pages; 
    
    uintptr_t stack_vaddr; /*pages used as stack*/
    size_t stack_pages;

    uintptr_t hugepage_vaddr; /*a huge page used for benchmark*/
    size_t hugepage_size;     /*size in bytes*/

    seL4_CPtr ep;   /*communicate between benchmarking threads(spy&trojan)*/
    seL4_CPtr r_ep;  /*reply to root task*/
    seL4_CPtr notification_ep; /*notification ep used only within a domain*/ 
   
    /*the cspace allocation start from here*/
    seL4_CPtr first_free;
    /*untyped allocated by the root task*/
    seL4_CPtr untyped_cptr[CONFIG_BENCH_UNTYPE_COUNT];
    /*timer object shared by the root task*/
    timer_objects_t to;
    /*the flag of an equiped timer*/
    bool timer_enabled; 

} bench_args_t;

/* benchmarking environment set up according to the info passed by root*/
typedef struct bench_env {
    /* vka interface */
    vka_t vka;
    /* vspace interface for managing virtual memory in the benchmark */
    vspace_t vspace;
    /* intialised allocman that backs the vka interface */
    allocman_t *allocman;
    /* minimal simple implementation that backs the default timer */
    simple_t simple;
    /* static data to initialise vspace */
    sel4utils_alloc_data_t data;
    /* seL4 ltimer wrapper */
    seL4_timer_t timer;
    /* has the timer been initialised? */
    bool timer_initialised;
    /* notification that is bound to both timer irq handlers */
    vka_object_t ntfn;
    /* args we started with */
    bench_args_t *args;
} bench_env_t;