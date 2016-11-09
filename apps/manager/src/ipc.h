#ifndef _IPC_H_
#define _IPC_H_ 

#include "../../bench_common.h"

#ifdef CONFIG_ARCH_X86
#define CCNT64BIT
#define CCNT_FORMAT "%llu"
typedef uint64_t ccnt_t;
#else
#define CCNT32BIT
typedef uint32_t ccnt_t;
#define CCNT_FORMAT "%u"
#endif

#if 0
/*benchmark running sequence number*/
#define IPC_BENCH_CALL_START 0 
#define IPC_BENCH_CALL_END   3 
#define IPC_BENCH_REPLY_WAIT_START 4 
#define IPC_BENCH_REPLY_WAIT_END   7
#define IPC_WAIT        8 
#define IPC_SEND        9 
#define IPC_OVERHEAD    10
#define EEMBC_TEST      11
#define COVERT_SENDER   12 
#define COVERT_RECEIVER1 13 
#define COVERT_RECEIVER2  14
#endif 


#define READ_COUNTER_X86(var) do { \
    uint32_t low, high; \
    asm volatile( \
        "movl $0, %%eax \n" \
        "movl $0, %%ecx \n" \
        "cpuid \n" \
        "rdtsc \n" \
        "movl %%edx, %0 \n" \
        "movl %%eax, %1 \n" \
        "movl $0, %%eax \n" \
        "movl $0, %%ecx \n" \
        "cpuid \n" \
        : \
         "=r"(high), \
         "=r"(low) \
        : \
        : "eax", "ebx", "ecx", "edx" \
    ); \
    (var) = (((uint64_t)high) << 32ull) | ((uint64_t)low); \
} while(0)

#define READ_COUNTER_ARMV7(var) do { \
    asm volatile("mrc p15, 0, %0, c9, c13, 0\n" \
        : "=r"(var) \
    ); \
} while(0)


#if defined(CONFIG_ARCH_ARM_V6)
#define READ_COUNTER_BEFORE READ_COUNTER_ARMV6
#define READ_COUNTER_AFTER  READ_COUNTER_ARMV6
#elif defined(CONFIG_ARCH_ARM_V7A) || defined (CONFIG_ARCH_ARM_V8A)
#define READ_COUNTER_BEFORE READ_COUNTER_ARMV7
#define READ_COUNTER_AFTER  READ_COUNTER_ARMV7
#elif defined(CONFIG_ARCH_X86)
#ifdef CONFIG_X86_64
#define READ_COUNTER_BEFORE READ_COUNTER_X64_BEFORE
#define READ_COUNTER_AFTER  READ_COUNTER_X64_AFTER
#define ALLOW_UNSTABLE_OVERHEAD
#else
#define READ_COUNTER_BEFORE READ_COUNTER_X86
#define READ_COUNTER_AFTER  READ_COUNTER_X86
#define ALLOW_UNSTABLE_OVERHEAD
#endif
#else
#error Unsupported arch
#endif



struct bench_results {
    /* Raw results from benchmarking. These get checked for sanity */
    ccnt_t call_overhead[IPC_RUNS];
    ccnt_t reply_wait_overhead[IPC_RUNS];
    ccnt_t call_10_overhead[IPC_RUNS];
    ccnt_t reply_wait_10_overhead[IPC_RUNS];
    ccnt_t send_overhead[IPC_RUNS];
    ccnt_t wait_overhead[IPC_RUNS];
    ccnt_t call_time_inter[IPC_RUNS];
    ccnt_t reply_wait_time_inter[IPC_RUNS];
    ccnt_t call_time_intra[IPC_RUNS];
    ccnt_t reply_wait_time_intra[IPC_RUNS];
    ccnt_t call_time_inter_low[IPC_RUNS];
    ccnt_t reply_wait_time_inter_high[IPC_RUNS];
    ccnt_t call_time_inter_high[IPC_RUNS];
    ccnt_t reply_wait_time_inter_low[IPC_RUNS];
    ccnt_t send_time_inter[IPC_RUNS];
    ccnt_t wait_time_inter[IPC_RUNS];
    ccnt_t call_10_time_inter[IPC_RUNS];
    ccnt_t reply_wait_10_time_inter[IPC_RUNS];
    /* A worst case overhead */
    ccnt_t call_reply_wait_overhead;
    ccnt_t call_reply_wait_10_overhead;
    ccnt_t send_wait_overhead;
    /* Calculated results to print out */
    ccnt_t call_cycles_inter;
    ccnt_t reply_wait_cycles_inter;
    ccnt_t call_cycles_intra;
    ccnt_t reply_wait_cycles_intra;
    ccnt_t call_cycles_inter_low;
    ccnt_t reply_wait_cycles_inter_high;
    ccnt_t call_cycles_inter_high;
    ccnt_t reply_wait_cycles_inter_low;
    ccnt_t send_cycles_inter;
    ccnt_t wait_cycles_inter;
    ccnt_t call_10_cycles_inter;
    ccnt_t reply_wait_10_cycles_inter;
};


#endif 


