/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */




/*The common header file shared by benchmark application 
 and root task (manager process). Mainly containing: 
 data format and hardware configurations for testing.*/


#ifndef _BENCH_COMMON_H 
#define _BENCH_COMMON_H 


#include <stdint.h>
#include <sel4bench/sel4bench.h>
/*elf file name of benchmark thread*/ 
#define CONFIG_BENCH_THREAD_NAME    "side-bench"

/*name, ep, option*/
/*num of arguments passed to benchmark tests*/
#define CONFIG_BENCH_ARGS    3

/*num of digits used for maximum unsigned int*/
#define CONFIG_BENCH_MAX_UNIT   10 

/*benchmark thread priority*/
#define CONFIG_BENCH_PRIORITY    100 


/*benchmark communication protocol*/ 
#define BENCH_INIT_MSG      0x11

/*number of shared frames for benchmark recording*/

/*TODO: currently only suits the L1 data benchmark
 which is 16(msg length) * 256 (maximum 8 bits integer) 
 * 64 (number of cache sets) * 8 (timing measurement) = 0x200000 */
#define CONFIG_BENCH_RECORD_PAGES 0x200 

#define BENCH_FAILURE (-1)
#define BENCH_SUCCESS 0

/*cache line size*/
#define CL_SIZE    64 
/**************************************************/
/*prime + probe attack on L1 D cache, in dcache.c*/
/*AES attack, synchrous*/

/*number of probes*/
#define N_D_TESTS   900000
/*all zero bits for aes key value*/
#define AES_KEY_ALL_ZERO 

/*L1 d cache size 32K*/
#define L1D_SIZE  0x8000

/*number of l1 d cache sets: 64*/
#define N_L1D_SETS   64
/*number of l1 d cache ways*/
#define N_L1D_WAYS   8 
/*bit masks used for recording connected cache lines*/
#define N_L1D_WAY_BITMASK 0xff 

/*size of an entire L1 D set: 4096 */
#define L1D_SET_SIZE  0x1000
/*AES msg size, 128 bits 
 key size 128 bits*/
#define AES_MSG_SIZE  16
#define AES_KEY_SIZE  16
/*aes service size 128 bits block */
#define AES_BITS      128

/*number of possible plaintext values in each byte*/
#define N_PT_B 256   

/* The fence is designed to try and prevent the compiler optimizing across code boundaries
      that we don't want it to. The reason for preventing optimization is so that things like
         overhead calculations aren't unduly influenced */
#define FENCE() asm volatile("" ::: "memory")
/*for cache flushing benchmark*/
#define WARMUPS 16
#define OVERHEAD_RUNS 10
#define OVERHEAD_RETRIES 4

/*ipc benchmark, same as sel4bench*/
#define IPC_RUNS    16
#define IPC_WARMUPS 16
#define IPC_OVERHEAD_RETRIES  4 

#define IPC_PROCESS_PRIO_LOW 50 
#define IPC_PROCESS_PRIO_HIGH 100 

#define IPC_PROCESS_PRIO 100 

/*maximum number of benchmarking thread in the system*/ 
#define MAX_BENCH_THREADS  2


/*dividing cache colours into security domains*/
#define CC_NUM_DOMAINS     2
#ifdef CONFIG_ARCH_X86 
#ifdef CONFIG_MANAGER_COVERT_SINGLE 
#define CC_DIV             4    /*spliting the L2 caches, 8 colours in total*/
#else
#define CC_DIV             16
#endif 
#endif 
#ifdef CONFIG_PLAT_IMX6
#define CC_DIV             8
#endif
#ifdef CONFIG_PLAT_EXYNOS4
#define CC_DIV             8
#endif
#ifdef CONFIG_PLAT_EXYNOS54XX
#define CC_DIV             8
#endif


/*running the capacity benchmark*/ 
#define BENCH_CAP_SINGLE  1  /*running the benchmark on single core*/
#define BENCH_CAP_SLAVE 1

#if 0
#define IPC_BENCH_CALL_START 0 
#define IPC_BENCH_CALL_END   3 
#define IPC_BENCH_REPLY_WAIT_START 4 
#define IPC_BENCH_REPLY_WAIT_END   7
#define IPC_WAIT        8 
#define IPC_SEND        9 
#define IPC_OVERHEAD    10
#endif 

/*ipc bench serial number*/
enum ipc_funs{
    IPC_CALL, 
    IPC_CALL2, 
    IPC_CALL_10, 
    IPC_CALL2_10, 
    IPC_REPLY_WAIT, 
    IPC_REPLY_WAIT2, 
    IPC_REPLY_WAIT_10, 
    IPC_REPLY_WAIT2_10, 
    IPC_WAIT, 
    IPC_SEND, 
    IPC_RT_CALL, 
    IPC_RT_REPLY_WAIT,
    IPC_OVERHEAD, 
    IPC_ALL
};

#if 0
enum ipc_benchmarks {
    INTRA_CALL, 
    INTRA_REPLYWAIT, 
    INTER_CALL, 
    INTER_REPLYWAIT, 
    INTER_CALL_LOW_TO_HIGH, 
    INTER_REPLYWAIT_LOW_TO_HIGH, 
    INTER_CALL_HIGH_TO_LOW, 
    INTER_REPLYWAIT_HIGH_TO_LOW, 
    INTER_SEND, 
    INTER_CALL_10,
    INTER_REPLYWAIT_10, 
    NIPCBENCHMARKS
}

enum ipc_overhead_benchmarks {
    CALL_OVERHEAD,
    REPLY_WAIT_OVERHEAD,
    SEND_OVERHEAD,
    WAIT_OVERHEAD,
    CALL_10_OVERHEAD,
    REPLY_WAIT_10_OVERHEAD,
    /******/
    NOVERHEADBENCHMARKS
};

enum ipc_overheads {
    CALL_REPLY_WAIT_OVERHEAD,
    CALL_REPLY_WAIT_10_OVERHEAD,
    SEND_WAIT_OVERHEAD,
    /******/
    NOVERHEADS
};


struct ipc_results {
    /* Raw results from benchmarking. These get checked for sanity */
    sel4bench_counter_t overhead_benchmarks[NOVERHEADBENCHMARKS][IPC_RUNS];
    sel4bench_counter_t benchmarks[NIPCBENCHMARKS][IPC_RUNS];
    /* A worst case overhead */
    sel4bench_counter_t overheads[NOVERHEADS];
    /* Calculated results to print out */
    sel4bench_counter_t results[NIPCBENCHMARKS];

};
#endif 

#ifdef CONFIG_ARCH_X86 
#define BENCH_PMU_BITS 0xf
#define BENCH_PMU_COUNTERS 4 
#endif 

#ifdef CONFIG_ARM_CORTEX_A9
#define BENCH_PMU_BITS 0x3f 
#define BENCH_PMU_COUNTERS 6 
#endif 

#ifdef CONFIG_ARM_CORTEX_A15
#define BENCH_PMU_BITS 0x3
#define BENCH_PMU_COUNTERS 2 
#endif 


#define BENCH_PMU_PAGES    1
#define BENCH_COVERT_TIME_PAGES 21  /*ts structure defined in timestats.c*/
#define BENCH_COVERT_BUF_PAGES  4096 /*trojan/probe buffers*/

/*pmu counter result structure*/
typedef struct {

    sel4bench_counter_t pmuc[IPC_ALL][BENCH_PMU_COUNTERS]; 
} ipc_pmu_t;  

typedef struct {
    /* Raw results from benchmarking. These get checked for sanity */
    sel4bench_counter_t call_time_inter[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t reply_wait_time_inter[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t call_time_inter_low[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t reply_wait_time_inter_high[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t call_time_inter_high[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t reply_wait_time_inter_low[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t send_time_inter[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t wait_time_inter[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t call_10_time_inter[IPC_RUNS][BENCH_PMU_COUNTERS];
    sel4bench_counter_t reply_wait_10_time_inter[IPC_RUNS][BENCH_PMU_COUNTERS];
} ipc_test_pmu_t; 

typedef struct {
    /* Raw results from benchmarking. rt only */
   volatile sel4bench_counter_t call_rt_time[IPC_RUNS];
   volatile sel4bench_counter_t call_reply_wait_overhead; 
} ipc_rt_result_t; 
           




/*data maxtix for probing result (x,y) = (Plain text 
  value, set number)*/
//typedef uint64_t d_time_t [N_PT_B][N_L1D_SETS];
typedef struct {

    uint64_t t[N_PT_B][N_L1D_SETS];
} d_time_t; 

/*running enviorment for bench covert 
 capacity*/
typedef struct bench_covert {
    void *p_buf; /*private, contiguous buffer, spy/trojan buffers*/
    void *ts_buf;    /*time statistic, ts_alloc*/
    seL4_CPtr syn_ep; /*comm trojan and receiver, tr_start_slave*/ 
    seL4_CPtr r_ep; /*comm between receiver and manager*/
    int opt;        /*running option, trojan, probe, etc*/
}bench_covert_t; 


/*opt for running covert channel bench*/
#define BENCH_COVERT_TROJAN_SINGLE  1 
#define BENCH_COVERT_SPY_SINGLE   2
#define BENCH_COVERT_MSG_LEN  2 /*msg len for init env*/

#define BENCH_PAGE_SIZE    4096  /*page size */

#define COMPILER_BARRIER do { asm volatile ("" ::: "memory"); } while(0);


#endif 



