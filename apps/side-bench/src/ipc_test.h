

#ifndef _IPC_TEST_H_
#define _IPC_TEST_H_


#include "../../bench_common.h"

typedef seL4_Word (*ipc_bench_func)(seL4_CPtr ep, seL4_CPtr result_ep); 


#ifdef CONFIG_ARCH_X86
#define CCNT64BIT
#define CCNT_FORMAT "%llu"
typedef uint64_t ccnt_t;
#else
#define CCNT32BIT
typedef uint32_t ccnt_t;
#define CCNT_FORMAT "%u"
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


#ifndef UNUSED
    #define UNUSED __attribute__((unused))
#endif

#define __SWINUM(x) ((x) & 0x00ffffff)


#define NOPS ""

#define DO_CALL_X86(ep, tag, sys) do { \
    uint32_t ep_copy = ep; \
    asm volatile( \
        "movl %%esp, %%ecx \n"\
        "leal 1f, %%edx \n"\
        "1: \n" \
        sys" \n" \
        : \
         "+S" (tag), \
         "+b" (ep_copy) \
        : \
         "a" (seL4_SysCall) \
        : \
         "ecx", \
         "edx" \
    ); \
} while(0)
#define DO_CALL_10_X86(ep, tag, sys) do { \
    uint32_t ep_copy = ep; \
    asm volatile( \
        "pushl %%ebp \n"\
        "movl %%esp, %%ecx \n"\
        "leal 1f, %%edx \n"\
        "1: \n" \
        sys" \n" \
        "popl %%ebp \n"\
        : \
         "+S" (tag), \
         "+b" (ep_copy) \
        : \
         "a" (seL4_SysCall) \
        : \
         "ecx", \
         "edx", \
         "edi" \
    ); \
} while(0)
#define DO_SEND_X86(ep, tag, sys) do { \
    uint32_t ep_copy = ep; \
    uint32_t tag_copy = tag.words[0]; \
    asm volatile( \
        "movl %%esp, %%ecx \n"\
        "leal 1f, %%edx \n"\
        "1: \n" \
        sys" \n" \
        : \
         "+S" (tag_copy), \
         "+b" (ep_copy) \
        : \
         "a" (seL4_SysSend) \
        : \
         "ecx", \
         "edx" \
    ); \
} while(0)
#define DO_REPLY_WAIT_X86(ep, tag, sys) do { \
    uint32_t ep_copy = ep; \
    asm volatile( \
        "movl %%esp, %%ecx \n"\
        "leal 1f, %%edx \n"\
        "1: \n" \
        sys" \n" \
        : \
         "+S" (tag), \
         "+b" (ep_copy) \
        : \
         "a" (seL4_SysReplyRecv) \
        : \
         "ecx", \
         "edx" \
    ); \
} while(0)
#define DO_REPLY_WAIT_10_X86(ep, tag, sys) do { \
    uint32_t ep_copy = ep; \
    asm volatile( \
        "pushl %%ebp \n"\
        "movl %%esp, %%ecx \n"\
        "leal 1f, %%edx \n"\
        "1: \n" \
        sys" \n" \
        "popl %%ebp \n"\
        : \
         "+S" (tag), \
         "+b" (ep_copy) \
        : \
         "a" (seL4_SysReplyRecv) \
        : \
         "ecx", \
         "edx", \
         "edi" \
    ); \
} while(0)
#define DO_WAIT_X86(ep, sys) do { \
    uint32_t ep_copy = ep; \
    asm volatile( \
        "movl %%esp, %%ecx \n"\
        "leal 1f, %%edx \n"\
        "1: \n" \
        sys" \n" \
        : \
         "+b" (ep_copy) \
        : \
         "a" (seL4_SysWait) \
        : \
         "ecx", \
         "edx", \
         "esi" \
    ); \
} while(0)
#define DO_CALL_ARM(ep, tag, swi) do { \
    register seL4_Word dest asm("r0") = (seL4_Word)ep; \
    register seL4_MessageInfo_t info asm("r1") = tag; \
    register seL4_Word scno asm("r7") = seL4_SysCall; \
    asm volatile(NOPS swi NOPS \
        : "+r"(dest), "+r"(info) \
        : [swi_num] "i" __SWINUM(seL4_SysCall), "r"(scno) \
    ); \
} while(0)
#define DO_CALL_10_ARM(ep, tag, swi) do { \
    register seL4_Word dest asm("r0") = (seL4_Word)ep; \
    register seL4_MessageInfo_t info asm("r1") = tag; \
    register seL4_Word scno asm("r7") = seL4_SysCall; \
    asm volatile(NOPS swi NOPS \
        : "+r"(dest), "+r"(info) \
        : [swi_num] "i" __SWINUM(seL4_SysCall), "r"(scno) \
        : "r2", "r3", "r4", "r5" \
    ); \
} while(0)
#define DO_SEND_ARM(ep, tag, swi) do { \
    register seL4_Word dest asm("r0") = (seL4_Word)ep; \
    register seL4_MessageInfo_t info asm("r1") = tag; \
    register seL4_Word scno asm("r7") = seL4_SysSend; \
    asm volatile(NOPS swi NOPS \
        : "+r"(dest), "+r"(info) \
        : [swi_num] "i" __SWINUM(seL4_SysSend), "r"(scno) \
    ); \
} while(0)
#define DO_REPLY_WAIT_10_ARM(ep, tag, swi) do { \
    register seL4_Word src asm("r0") = (seL4_Word)ep; \
    register seL4_MessageInfo_t info asm("r1") = tag; \
    register seL4_Word scno asm("r7") = seL4_SysReplyRecv; \
    asm volatile(NOPS swi NOPS \
        : "+r"(src), "+r"(info) \
        : [swi_num] "i" __SWINUM(seL4_SysReplyRecv), "r"(scno) \
        : "r2", "r3", "r4", "r5" \
    ); \
} while(0)
#define DO_REPLY_WAIT_ARM(ep, tag, swi) do { \
    register seL4_Word src asm("r0") = (seL4_Word)ep; \
    register seL4_MessageInfo_t info asm("r1") = tag; \
    register seL4_Word scno asm("r7") = seL4_SysReplyRecv; \
    asm volatile(NOPS swi NOPS \
        : "+r"(src), "+r"(info) \
        : [swi_num] "i" __SWINUM(seL4_SysReplyRecv), "r"(scno) \
    ); \
} while(0)
#define DO_WAIT_ARM(ep, swi) do { \
    register seL4_Word src asm("r0") = (seL4_Word)ep; \
    register seL4_MessageInfo_t info asm("r1"); \
    register seL4_Word scno asm("r7") = seL4_SysWait; \
    asm volatile(NOPS swi NOPS \
        : "+r"(src), "=r"(info) \
        : [swi_num] "i" __SWINUM(seL4_SysWait), "r"(scno) \
    ); \
} while(0)

#define DO_CALL_X64(ep, tag, sys) do { \
    uint64_t ep_copy = ep; \
    asm volatile(\
            "movq   %%rsp, %%rcx \n" \
            "leaq   1f, %%rdx \n" \
            "1: \n" \
            sys" \n" \
            : \
            "+S" (tag), \
            "+b" (ep_copy) \
            : \
            "a" (seL4_SysCall) \
            : \
            "rcx", "rdx"\
            ); \
} while (0)

#define DO_CALL_10_X64(ep, tag, sys) do {\
    uint64_t ep_copy = ep; \
    seL4_Word mr0 = 0; \
    seL4_Word mr1 = 1; \
    register seL4_Word mr2 asm ("r8") = 2;  \
    register seL4_Word mr3 asm ("r9") = 3;  \
    register seL4_Word mr4 asm ("r10") = 4; \
    register seL4_Word mr5 asm ("r11") = 5; \
    register seL4_Word mr6 asm ("r12") = 6; \
    register seL4_Word mr7 asm ("r13") = 7; \
    register seL4_Word mr8 asm ("r14") = 8; \
    register seL4_Word mr9 asm ("r15") = 9; \
    asm volatile(                           \
            "push   %%rbp \n"               \
            "movq   %%rcx, %%rbp \n"        \
            "movq   %%rsp, %%rcx \n"        \
            "leaq   1f, %%rdx \n"           \
            "1: \n"                         \
            sys" \n"                        \
            "movq   %%rbp, %%rcx \n"        \
            "pop    %%rbp \n"               \
            :                               \
            "+S" (tag),                     \
            "+b" (ep_copy),                 \
            "+D" (mr0), "+c" (mr1), "+r" (mr2), "+r" (mr3), \
            "+r" (mr4), "+r" (mr5), "+r" (mr6), "+r" (mr7), \
            "+r" (mr8), "+r" (mr9)          \
            :                               \
            "a" (seL4_SysCall)              \
            :                               \
            "rdx"                           \
            );                              \
} while (0)

#define DO_SEND_X64(ep, tag, sys) do { \
    uint64_t ep_copy = ep; \
    uint32_t tag_copy = tag.words[0]; \
    asm volatile( \
            "movq   %%rsp, %%rcx \n" \
            "leaq   1f, %%rdx \n" \
            "1: \n" \
            sys" \n" \
            : \
            "+S" (tag_copy), \
            "+b" (ep_copy) \
            : \
            "a" (seL4_SysSend) \
            : \
            "rcx", "rdx" \
            ); \
} while (0)

#define DO_REPLY_WAIT_X64(ep, tag, sys) do { \
    uint64_t ep_copy = ep; \
    asm volatile( \
            "movq   %%rsp, %%rcx \n" \
            "leaq   1f, %%rdx \n" \
            "1: \n" \
            sys" \n" \
            : \
            "+S" (tag), \
            "+b" (ep_copy) \
            : \
            "a"(seL4_SysReplyRecv) \
            : \
            "rcx", "rdx" \
            ); \
} while (0)

#define DO_REPLY_WAIT_10_X64(ep, tag, sys) do { \
    uint64_t ep_copy = ep;                      \
    seL4_Word mr0 = 0;                          \
    seL4_Word mr1 = -1;                         \
    register seL4_Word mr2 asm ("r8") = -2;     \
    register seL4_Word mr3 asm ("r9") = -3;     \
    register seL4_Word mr4 asm ("r10") = -4;    \
    register seL4_Word mr5 asm ("r11") = -5;    \
    register seL4_Word mr6 asm ("r12") = -6;    \
    register seL4_Word mr7 asm ("r13") = -7;    \
    register seL4_Word mr8 asm ("r14") = -8;    \
    register seL4_Word mr9 asm ("r15") = -9;    \
    asm volatile(                               \
            "push   %%rbp \n"                   \
            "movq   %%rcx, %%rbp \n"            \
            "movq   %%rsp, %%rcx \n"            \
            "leaq   1f, %%rdx \n"               \
            "1: \n"                             \
            sys" \n"                            \
            "movq   %%rbp, %%rcx \n"            \
            "pop    %%rbp \n"                   \
            :                                   \
            "+S" (tag),                         \
            "+b" (ep_copy),                     \
            "+D" (mr0), "+c" (mr1), "+r" (mr2), \
            "+r" (mr3), "+r" (mr4), "+r" (mr5), \
            "+r" (mr6), "+r" (mr7), "+r" (mr8), \
            "+r" (mr9)                          \
            :                                   \
            "a" (seL4_SysReplyRecv)             \
            :                                   \
            "rdx"                               \
            );                                  \
} while (0)

#define DO_WAIT_X64(ep, sys) do { \
    uint64_t ep_copy = ep; \
    uint64_t tag = 0; \
    asm volatile( \
            "movq   %%rsp, %%rcx \n" \
            "leaq   1f, %%rdx \n" \
            "1: \n" \
            sys" \n" \
            : \
            "+S" (tag) ,\
            "+b" (ep_copy) \
            : \
            "a" (seL4_SysWait) \
            :  "rcx", "rdx" \
            ); \
} while (0)

#define READ_COUNTER_ARMV6(var) do { \
    asm volatile("b 2f\n\
                1:mrc p15, 0, %[counter], c15, c12," SEL4BENCH_ARM1136_COUNTER_CCNT "\n\
                  bx lr\n\
                2:sub r8, pc, #16\n\
                  .word 0xe7f000f0" \
        : [counter] "=r"(var) \
        : \
        : "r8", "lr"); \
} while(0)

#define READ_COUNTER_ARMV7(var) do { \
    asm volatile("mrc p15, 0, %0, c9, c13, 0\n" \
        : "=r"(var) \
    ); \
} while(0)

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

#define READ_COUNTER_X64_BEFORE(var) do { \
    uint32_t low, high; \
    asm volatile( \
            "cpuid \n" \
            "rdtsc \n" \
            "movl %%edx, %0 \n" \
            "movl %%eax, %1 \n" \
            : "=r"(high), "=r"(low) \
            : \
            : "%rax", "%rbx", "%rcx", "%rdx"); \
    (var) = (((uint64_t)high) << 32ull) | ((uint64_t)low); \
} while (0)

#define READ_COUNTER_X64_AFTER(var) do { \
    uint32_t low, high; \
    asm volatile( \
            "rdtscp \n" \
            "movl %%edx, %0 \n" \
            "movl %%eax, %1 \n" \
            "cpuid \n" \
            : "=r"(high), "=r"(low) \
            : \
            : "%rax", "rbx", "%rcx", "%rdx"); \
    (var) = (((uint64_t)high) << 32ull) | ((uint64_t)low); \
} while (0)

#if defined(CONFIG_ARCH_ARM)
#define DO_REAL_CALL(ep, tag) DO_CALL_ARM(ep, tag, "swi %[swi_num]")
#define DO_NOP_CALL(ep, tag) DO_CALL_ARM(ep, tag, "nop")
#define DO_REAL_REPLY_WAIT(ep, tag) DO_REPLY_WAIT_ARM(ep, tag, "swi %[swi_num]")
#define DO_NOP_REPLY_WAIT(ep, tag) DO_REPLY_WAIT_ARM(ep, tag, "nop")
#define DO_REAL_CALL_10(ep, tag) DO_CALL_10_ARM(ep, tag, "swi %[swi_num]")
#define DO_NOP_CALL_10(ep, tag) DO_CALL_10_ARM(ep, tag, "nop")
#define DO_REAL_REPLY_WAIT_10(ep, tag) DO_REPLY_WAIT_10_ARM(ep, tag, "swi %[swi_num]")
#define DO_NOP_REPLY_WAIT_10(ep, tag) DO_REPLY_WAIT_10_ARM(ep, tag, "nop")
#define DO_REAL_SEND(ep, tag) DO_SEND_ARM(ep, tag, "swi %[swi_num]")
#define DO_NOP_SEND(ep, tag) DO_SEND_ARM(ep, tag, "nop")
#define DO_REAL_WAIT(ep) DO_WAIT_ARM(ep, "swi %[swi_num]")
#define DO_NOP_WAIT(ep) DO_WAIT_ARM(ep, "nop")
#elif defined(CONFIG_ARCH_X86)

#ifdef CONFIG_X86_64
#define DO_REAL_CALL(ep, tag) DO_CALL_X64(ep, tag, "sysenter")
#define DO_NOP_CALL(ep, tg) DO_CALL_X64(ep, tag, ".byte 0x90")
#define DO_REAL_REPLY_WAIT(ep, tag) DO_REPLY_WAIT_X64(ep, tag, "sysenter")
#define DO_NOP_REPLY_WAIT(ep, tag) DO_REPLY_WAIT_X64(ep, tag, ".byte 0x90")
#define DO_REAL_CALL_10(ep, tag) DO_CALL_10_X64(ep, tag, "sysenter")
#define DO_NOP_CALL_10(ep, tag) DO_CALL_10_X64(ep, tag, ".byte 0x90")
#define DO_REAL_REPLY_WAIT_10(ep, tag) DO_REPLY_WAIT_10_X64(ep, tag, "sysenter")
#define DO_NOP_REPLY_WAIT_10(ep, tag) DO_REPLY_WAIT_10_X64(ep, tag, ".byte 0x90")
#define DO_REAL_SEND(ep, tag) DO_SEND_X64(ep, tag, "sysenter")
#define DO_NOP_SEND(ep, tag) DO_SEND_X64(ep, tag, ".byte 0x90")
#define DO_REAL_WAIT(ep) DO_WAIT_X64(ep, "sysenter")
#define DO_NOP_WAIT(ep) DO_WAIT_X64(ep, ".byte 0x90")
#else
#define DO_REAL_CALL(ep, tag) DO_CALL_X86(ep, tag, "sysenter")
#define DO_NOP_CALL(ep, tag) DO_CALL_X86(ep, tag, ".byte 0x66\n.byte 0x90")
#define DO_REAL_REPLY_WAIT(ep, tag) DO_REPLY_WAIT_X86(ep, tag, "sysenter")
#define DO_NOP_REPLY_WAIT(ep, tag) DO_REPLY_WAIT_X86(ep, tag, ".byte 0x66\n.byte 0x90")
#define DO_REAL_CALL_10(ep, tag) DO_CALL_10_X86(ep, tag, "sysenter")
#define DO_NOP_CALL_10(ep, tag) DO_CALL_10_X86(ep, tag, ".byte 0x66\n.byte 0x90")
#define DO_REAL_REPLY_WAIT_10(ep, tag) DO_REPLY_WAIT_10_X86(ep, tag, "sysenter")
#define DO_NOP_REPLY_WAIT_10(ep, tag) DO_REPLY_WAIT_10_X86(ep, tag, ".byte 0x66\n.byte 0x90")
#define DO_REAL_SEND(ep, tag) DO_SEND_X86(ep, tag, "sysenter")
#define DO_NOP_SEND(ep, tag) DO_SEND_X86(ep, tag, ".byte 0x66\n.byte 0x90")
#define DO_REAL_WAIT(ep) DO_WAIT_X86(ep, "sysenter")
#define DO_NOP_WAIT(ep) DO_WAIT_X86(ep, ".byte 0x66\n.byte 0x90")
#endif
#else
#error Unsupported arch
#endif

#if defined(CONFIG_ARCH_ARM_V6)
#define READ_COUNTER_BEFORE READ_COUNTER_ARMV6
#define READ_COUNTER_AFTER  READ_COUNTER_ARMV6
#elif defined(CONFIG_ARCH_ARM_V7A) || defined(CONFIG_ARCH_ARM_V8A)
#define READ_COUNTER_BEFORE READ_COUNTER_ARMV7
#define READ_COUNTER_AFTER  READ_COUNTER_ARMV7
#define ALLOW_UNSTABLE_OVERHEAD
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

/* The fence is designed to try and prevent the compiler optimizing across code boundaries
   that we don't want it to. The reason for preventing optimization is so that things like
   overhead calculations aren't unduly influenced */
#define FENCE() asm volatile("" ::: "memory")


#define MEASURE_OVERHEAD(op, dest, decls) do { \
    uint32_t i; \
    for (i = 0; i < IPC_OVERHEAD_RETRIES; i++) { \
        uint32_t j; \
        for (j = 0; j < IPC_RUNS; j++) { \
            uint32_t k; \
            decls; \
            ccnt_t start, end; \
            FENCE(); \
            for (k = 0; k < IPC_WARMUPS; k++) { \
                READ_COUNTER_BEFORE(start); \
                op; \
                READ_COUNTER_AFTER(end); \
            } \
            FENCE(); \
            dest[j] = end - start; \
        } \
        if (results_stable(dest)) break; \
    } \
} while(0)


static inline void dummy_seL4_Send(seL4_CPtr ep, seL4_MessageInfo_t tag) {
    (void)ep;
    (void)tag;
}

static inline void dummy_seL4_Call(seL4_CPtr ep, seL4_MessageInfo_t tag) {
    (void)ep;
    (void)tag;
}

static inline void dummy_seL4_Wait(seL4_CPtr ep, void *badge) {
    (void)ep;
    (void)badge;
}

static inline void dummy_seL4_Reply(seL4_MessageInfo_t tag) {
    (void)tag;
}

seL4_Word ipc_call_func(seL4_CPtr ep, seL4_CPtr result_ep);
seL4_Word ipc_call_func2(seL4_CPtr ep, seL4_CPtr result_ep);
seL4_Word ipc_call_10_func(seL4_CPtr ep, seL4_CPtr result_ep);
seL4_Word ipc_call_10_func2(seL4_CPtr ep, seL4_CPtr result_ep);


seL4_Word ipc_reply_wait_func(seL4_CPtr ep, seL4_CPtr result_ep); 

seL4_Word ipc_reply_wait_func2(seL4_CPtr ep, seL4_CPtr result_ep); 

seL4_Word ipc_reply_wait_10_func(seL4_CPtr ep, seL4_CPtr result_ep); 

seL4_Word ipc_reply_wait_10_func2(seL4_CPtr ep, seL4_CPtr result_ep); 

seL4_Word ipc_rt_reply_wait_func(seL4_CPtr ep, seL4_CPtr result_ep); 
seL4_Word ipc_rt_call_func(seL4_CPtr ep, seL4_CPtr result_ep); 
seL4_Word ipc_latency_reply_wait_func(seL4_CPtr ep, seL4_CPtr result_ep); 
seL4_Word ipc_latency_call_func(seL4_CPtr ep, seL4_CPtr result_ep); 


/*interfaces*/
void ipc_measure_overhead(seL4_CPtr reply_ep, struct bench_results *results); 
uint32_t ipc_wait_func(seL4_CPtr ep, seL4_CPtr result_ep); 
uint32_t ipc_send_func(seL4_CPtr ep, seL4_CPtr result_ep);

void run_covert_benchmark(void *share_vaddr, uint32_t no); 


#endif 
