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


#ifndef _BENCH_TYPES_H
#define _BENCH_TYPES_H 

/*FIXME: prepare for deletion*/
/*running enviorment for bench covert 
 capacity*/
typedef struct bench_covert {
    void *p_buf; /*private, contiguous buffer, spy/trojan buffers*/
    void *ts_buf;    /*time statistic, ts_alloc*/
    seL4_CPtr syn_ep; /*comm trojan and receiver, tr_start_slave*/ 
    seL4_CPtr r_ep; /*comm between receiver and manager*/
    seL4_CPtr notification_ep; /*notification ep used only within a domain*/ 
    seL4_Word opt;        /*running option, trojan, probe, etc*/
}bench_covert_t; 


struct bench_l1 {
    /*L1 data/instruction cache 64 sets, the result contains the 
     total cost on probing L1 D/I cache*/
    uint32_t volatile result[CONFIG_BENCH_DATA_POINTS];
    uint32_t volatile sec[CONFIG_BENCH_DATA_POINTS];
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    uint32_t volatile pmu[CONFIG_BENCH_DATA_POINTS][BENCH_PMU_COUNTERS]; 
#endif 
};



/* benchmarking environment set up by root task */
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
    /* virtual address to write benchmark results to */
    void *results;
    /* seL4 ltimer wrapper */
    seL4_timer_t timer;
    /* has the timer been initialised? */
    bool timer_initialised;
    /* notification that is bound to both timer irq handlers */
    vka_object_t ntfn;
    /* args we started with */
    benchmark_args_t *args;
} bench_env_t;


/*the argument passes to the benchmarking thread*/
typedef struct {

    int test_num; 

    uintptr_t shared_vaddr; /*pages shared between benchmarking thread*/ 
    size_t shared_pages;

    uintptr_t record_vaddr; /*pages used for recording result*/
    size_t record_pages; 

    uintptr_t hugepage_vaddr; /*a huge page used for benchmark*/
    size_t hugepage_size;     /*size in bytes*/

    void *p_buf; /*private, contiguous buffer, spy/trojan buffers*/
    size_t p_buf_pages; 

    void *ts_buf;    /*time statistic, ts_alloc*/
    size_t ts_buf_pages; 

    uintptr_t stack_vaddr;
    size_t stack_pages;

    seL4_CPtr ep;   /*communicate between benchmarking threads(spy&trojan)*/
    seL4_CPtr r_ep;  /*reply to root task*/
    seL4_CPtr notification_ep; /*notification ep used only within a domain*/ 
   
    /*the cspace allocation start from here*/
    seL4_CPtr first_free;
    /*untyped allocated by the root task*/
    seL4_CPtr untyped_cptr[CONFIG_BENCH_UNTYPE_COUNT];
    /*timer object shared by the root task*/
    timer_objects_t to;
} benchmark_args_t;

#endif   /*_BENCH_TYPES_H*/
~
