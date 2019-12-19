#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include "mastik_common/vlist.h"
#include "mastik_common/low.h"
#include "mastik_common/l1i.h"
#include "mastik_common/l1.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>


static void access_llc_buffer(void *buffer) {

    for (int i = 0; i < L3_SIZE; i += L1_CACHELINE)
        low_access(buffer + i); 
}


/*the cache flushing benchmark on LLC: 
 testing the cost of flushing the LLC cache*/
int llc_cache_flush(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    bench_args_t *args = env->args; 
    char *buf = (char *)mmap(NULL, L3_SIZE + 4096 * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    assert(buf); 
    /*page aligned the buffer*/
    uintptr_t buf_switch = (uintptr_t) buf; 
    buf_switch &= ~(0xfffULL); 
    buf_switch += 0x1000; 
    buf = (char *) buf_switch; 

    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);


    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {
        /*waiting for a system tick*/
        newTimeSlice();
        /*start measuing*/
        seL4_SetMR(100, 0x12345678); 

        start = sel4bench_get_cycle_count(); 
        access_llc_buffer(buf);            
 
        end = sel4bench_get_cycle_count(); 

        r_addr->costs[i] = end - start - overhead; 
    }
    newTimeSlice();
    seL4_SetMR(100, 0); 

    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}

/*the cache flushing benchmark on LLC: 
 testing the cost of flushing the LLC cache*/
int llc_cache_flush_only(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    bench_args_t *args = env->args; 
    char *buf = (char *)mmap(NULL, L3_SIZE + 4096 * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    assert(buf); 
    /*page aligned the buffer*/
    uintptr_t buf_switch = (uintptr_t) buf; 
    buf_switch &= ~(0xfffULL); 
    buf_switch += 0x1000; 
    buf = (char *) buf_switch; 

    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);


    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {
        /*waiting for a system tick*/
        newTimeSlice();

        start = sel4bench_get_cycle_count(); 
        access_llc_buffer(buf);            
 
        end = sel4bench_get_cycle_count(); 

        r_addr->costs[i] = end - start - overhead; 

        /*yielding to the idle thread*/
        seL4_Yield(); 
    }
    newTimeSlice();

    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}


int l1_cache_nothing(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    bench_args_t *args = env->args; 

    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);


    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {
        /*waiting for a system tick*/
        newTimeSlice();
        seL4_SetMR(100, 0x12345678); 
        start = sel4bench_get_cycle_count(); 

        /*doing nothing*/
        end = sel4bench_get_cycle_count(); 

        /*ping kernel for taking the measurements in kernel
          a context switch is invovled, switching to the idle user-level thread*/
        r_addr->costs[i] = end - start - overhead; 
    }
    newTimeSlice();
    seL4_SetMR(100, 0); 
    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}


int l1_cache_flush_only(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    int mask_size = MONITOR_MASK; 
#ifdef CONFIG_BENCH_CACHE_FLUSH_L1_CACHES_INSTRUCTION
    mask_size = I_MONITOR_MASK;
#endif 

    uint64_t monitored_mask[mask_size]; 

    for (int m = 0; m < mask_size; m++)
        monitored_mask[m] = ~0LLU; 

    bench_args_t *args = env->args; 

#ifdef CONFIG_BENCH_CACHE_FLUSH_L1_CACHES_INSTRUCTION
    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
#else 
    l1info_t l1_1 = l1_prepare(monitored_mask);
#endif 
    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);

    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {
        /*waiting for a system tick*/
        newTimeSlice();
        start = sel4bench_get_cycle_count(); 

#ifdef CONFIG_BENCH_CACHE_FLUSH_L1_CACHES_INSTRUCTION
        l1i_prime(l1i_1);
#else 
        l1_prime(l1_1); 
#endif 
        end = sel4bench_get_cycle_count(); 

        /*ping kernel for taking the measurements in kernel
          a context switch is invovled, switching to the idle user-level thread*/
        r_addr->costs[i] = end - start - overhead; 
        /*yielding to the idle thread*/
        seL4_Yield(); 
    }
    newTimeSlice();
    
    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}

int l1_cache_flush(bench_env_t *env) {
    seL4_MessageInfo_t info;
    ccnt_t overhead, start, end;

    int mask_size = MONITOR_MASK; 
#ifdef CONFIG_BENCH_CACHE_FLUSH_L1_CACHES_INSTRUCTION
    mask_size = I_MONITOR_MASK;
#endif 

    uint64_t monitored_mask[mask_size]; 

    for (int m = 0; m < mask_size; m++)
        monitored_mask[m] = ~0LLU; 

    bench_args_t *args = env->args; 

#ifdef CONFIG_BENCH_CACHE_FLUSH_L1_CACHES_INSTRUCTION
    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
#else 
    l1info_t l1_1 = l1_prepare(monitored_mask);
#endif 
    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);


    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {
        /*waiting for a system tick*/
        newTimeSlice();
        seL4_SetMR(100, 0x12345678); 
        start = sel4bench_get_cycle_count(); 

#ifdef CONFIG_BENCH_CACHE_FLUSH_L1_CACHES_INSTRUCTION
        l1i_prime(l1i_1);
#else 
        l1_prime(l1_1); 
#endif 
        end = sel4bench_get_cycle_count(); 

        /*ping kernel for taking the measurements in kernel
          a context switch is invovled, switching to the idle user-level thread*/
        r_addr->costs[i] = end - start - overhead; 
    }
    newTimeSlice();
    seL4_SetMR(100, 0); 
    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}
