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
#define WARMUPS 10
#define OVERHEAD_RUNS 10
#define OVERHEAD_RETRIES 4

#define NLATENCY  16 


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
#ifdef CONFIG_BENCH_COVERT_SINGLE 
#define CC_DIV             4    /*spliting the L2 caches, 8 colours in total*/
#else
#define CC_DIV             16
#endif 
#endif  /*x86*/


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
    IPC_LATENCY_CALL, 
    IPC_LATENCY_REPLY_WAIT,
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
//#define BENCH_PMU_BITS 0x3f 
//#define BENCH_PMU_COUNTERS 6 
#define BENCH_PMU_BITS 0x1f 
#define BENCH_PMU_COUNTERS 5 
#endif 

#ifdef CONFIG_ARM_CORTEX_A15
#define BENCH_PMU_BITS 0x3
#define BENCH_PMU_COUNTERS 2 
#endif 

/*currently do not support any pmu debugging on v8a*/
#ifdef CONFIG_ARCH_ARM_V8A
#define BENCH_PMU_BITS  0x1
#define BENCH_PMU_COUNTERS 1 
#endif 


#define BENCH_PMU_PAGES    1
#define BENCH_COVERT_TIME_PAGES 21  /*ts structure defined in timestats.c*/
#define BENCH_COVERT_BUF_PAGES  4096 /*trojan/probe buffers*/

#define BENCH_MORECORE_HUGE_SIZE  (16 * 1024 * 1024) /* huge pages created by master for benchmarking thread */


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
   volatile sel4bench_counter_t call_time[IPC_RUNS]; 
   volatile sel4bench_counter_t reply_wait_time[IPC_RUNS];
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
    seL4_CPtr notification_ep; /*notification ep used only within a domain*/ 
    int opt;        /*running option, trojan, probe, etc*/
}bench_covert_t; 

#define BENCH_PAGE_SIZE  4096

/*opt for running covert channel bench*/
#define BENCH_COVERT_L2_TROJAN  1 
#define BENCH_COVERT_L2_SPY   2
#define BENCH_MASTIK_TEST    3
#define BENCH_MASTIK_VICTIM  4
#define BENCH_MPI_VICTIM     5
#define BENCH_MASTIK_SPY     6
#define BENCH_COVERT_L1D_TROJAN    7 
#define BENCH_COVERT_L1D_SPY       8 
#define BENCH_COVERT_L1I_TROJAN    9 
#define BENCH_COVERT_L1I_SPY       10
#define BENCH_COVERT_LLC_KERNEL_TROJAN   11 
#define BENCH_COVERT_LLC_KERNEL_SPY      12 
#define BENCH_COVERT_LLC_KD_TROJAN       13   /*kernel determinisitic scheduling*/
#define BENCH_COVERT_LLC_KD_SPY          14 
#define BENCH_COVERT_TLB_TROJAN          15 
#define BENCH_COVERT_TLB_SPY             16
#define BENCH_COVERT_BRANCH_TROJAN       17 
#define BENCH_COVERT_BRANCH_SPY          18
#define BENCH_COVERT_LLC_SINGLE_TROJAN   19 
#define BENCH_COVERT_LLC_SINGLE_SPY      20
#define BENCH_COVERT_FUNS                21

#define BENCH_COVERT_MSG_LEN  3 /*msg len for init env*/
/*matching the test number according to the config*/ 
#ifdef CONFIG_BENCH_COVERT_L1D 
#define BENCH_COVERT_TROJAN    BENCH_COVERT_L1D_TROJAN 
#define BENCH_COVERT_SPY       BENCH_COVERT_L1D_SPY 
#endif 
#ifdef CONFIG_BENCH_COVERT_L1I
#define BENCH_COVERT_TROJAN    BENCH_COVERT_L1I_TROJAN 
#define BENCH_COVERT_SPY       BENCH_COVERT_L1I_SPY 
#endif 
#ifdef CONFIG_BENCH_COVERT_L2 
#define BENCH_COVERT_TROJAN    BENCH_COVERT_L2_TROJAN 
#define BENCH_COVERT_SPY       BENCH_COVERT_L2_SPY 
#endif 
#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL  /*LLC channel through shared kernel*/
#define BENCH_COVERT_TROJAN    BENCH_COVERT_LLC_KERNEL_TROJAN 
#define BENCH_COVERT_SPY       BENCH_COVERT_LLC_KERNEL_SPY 
#endif 

/*LLC covert channel, single core*/
#ifdef CONFIG_BENCH_COVERT_LLC 
#define BENCH_COVERT_TROJAN    BENCH_COVERT_LLC_SINGLE_TROJAN 
#define BENCH_COVERT_SPY       BENCH_COVERT_LLC_SINGLE_SPY 
#endif 


/*the tlb covert channel*/
#ifdef CONFIG_BENCH_COVERT_TLB
#define BENCH_COVERT_TROJAN    BENCH_COVERT_TLB_TROJAN 
#define BENCH_COVERT_SPY       BENCH_COVERT_TLB_SPY 
#endif 

/*the branch covert channel*/
#ifdef CONFIG_BENCH_COVERT_BTB
#define BENCH_COVERT_TROJAN    BENCH_COVERT_BRANCH_TROJAN 
#define BENCH_COVERT_SPY       BENCH_COVERT_BRANCH_SPY 
#endif 


/*LLC channel through kernel scheduling*/
#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL_SCHEDULE 
#define BENCH_COVERT_TROJAN      BENCH_COVERT_LLC_KD_TROJAN
#define BENCH_COVERT_SPY         BENCH_COVERT_LLC_KD_SPY
#endif

#ifdef CONFIG_MASTIK_ATTACK_COVERT
#define BENCH_COVERT_SPY       BENCH_MASTIK_TEST
#define BENCH_COVERT_TROJAN    BENCH_MASTIK_VICTIM 
#endif
    /*the demo*/
#ifdef CONFIG_MASTIK_ATTACK_SIDE 
#define BENCH_COVERT_SPY       BENCH_MASTIK_SPY
#define BENCH_COVERT_TROJAN    BENCH_MPI_VICTIM
#endif 


/*used by kernel determinsitic scheduling benchmark*/
#ifdef CONFIG_CACHE_COLOURING
/*if cache colouring enabled, the number of cache sets that high uses doubles*/
#define NUM_LLC_CACHE_SETS   4096
#else
#define NUM_LLC_CACHE_SETS   2048 
#endif
/*number of cache sets visits for trojan to warm up the platform
 total 4096 cache sets, trying 0x16 rounds on each sets*/
#define NUM_KD_WARMUP_ROUNDS   0x16000

#ifndef CONFIG_BENCH_DATA_POINTS 
#define CONFIG_BENCH_DATA_POINTS  1
#endif 

#define NUM_KERNEL_SCHEDULE_DATA (CONFIG_BENCH_DATA_POINTS)
#define SYSTEM_TICK_SYN_FLAG   0xffff
#define SPY_SYN_FLAG           0x87654321
#define TROJAN_SYN_FLAG        0x12345678


/*one system tick is 1ms, 3400000 cycles, 3.4GHZ sandybridge machine*/
#define KERNEL_SCHEDULE_TICK_LENTH   1000000
#define NUM_KERNEL_SCHEDULE_SHARED_PAGE  1
#define NUM_L1D_SHARED_PAGE  1
/*ARM V7 tick length 1ms 800 MHZ*/
#define KERNEL_SCHEDULE_TICK_LENGTH    10000


#ifdef CONFIG_ARCH_X86
struct bench_kernel_schedule {
    uint64_t volatile prevs[NUM_KERNEL_SCHEDULE_DATA];
    uint64_t volatile starts[NUM_KERNEL_SCHEDULE_DATA];
    uint64_t volatile curs[NUM_KERNEL_SCHEDULE_DATA];
    uint32_t volatile prev_sec[NUM_KERNEL_SCHEDULE_DATA];
    uint32_t volatile cur_sec[NUM_KERNEL_SCHEDULE_DATA];
};
#else
/*arm platform*/
struct bench_kernel_schedule {
    uint32_t volatile prevs[NUM_KERNEL_SCHEDULE_DATA];
    uint32_t volatile starts[NUM_KERNEL_SCHEDULE_DATA];
    uint32_t volatile curs[NUM_KERNEL_SCHEDULE_DATA];
    uint32_t volatile prev_sec[NUM_KERNEL_SCHEDULE_DATA];
    uint32_t volatile cur_sec[NUM_KERNEL_SCHEDULE_DATA];
};
#endif 
struct bench_l1 {
    /*L1 data/instruction cache 64 sets, the result contains the 
     total cost on probing L1 D/I cache*/
    uint32_t volatile result[CONFIG_BENCH_DATA_POINTS];
    uint32_t volatile sec[CONFIG_BENCH_DATA_POINTS];
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    uint32_t volatile pmu[CONFIG_BENCH_DATA_POINTS][BENCH_PMU_COUNTERS]; 
#endif 
};

#ifdef CONFIG_ARCH_X86
static inline uint64_t rdtscp_64(void) {
    uint32_t low, high;

    asm volatile ( 
            "rdtscp          \n"
            "movl %%edx, %0 \n"
            "movl %%eax, %1 \n"
            : "=r" (high), "=r" (low)
            :
            : "eax", "ecx", "edx");

    return ((uint64_t) high) << 32llu | (uint64_t) low;
}

#endif


/*sandy bridge machine frequency 3.4GHZ*/
#define MASTIK_FEQ  (3400000000ull)


#define COMPILER_BARRIER do { asm volatile ("" ::: "memory"); } while(0);

#endif 



