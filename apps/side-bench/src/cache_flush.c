#include <autoconf.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include "vlist.h"
#include "low.h"
#include "l1.h"
#include "bench_common.h"
#include "bench_types.h"
#include "bench_helper.h"


static void access_llc_buffer(void *buffer) {

    for (int i = 0; i < L3_SIZE; i += L1_CACHELINE)
        access(buffer + i); 
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

    /*warming up*/
    for (int i = 0; i < BENCH_WARMUPS; i++) {
 
        start = sel4bench_get_cycle_count(); 
        access_llc_buffer(buf);            
        end = sel4bench_get_cycle_count(); 
 
        seL4_Yield();
    }
 
    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {
       
        start = sel4bench_get_cycle_count(); 
        access_llc_buffer(buf);    
        end = sel4bench_get_cycle_count(); 
 
        /*ping kernel for taking the measurements in kernel
          a context switch is invovled, switching to the idle user-level thread*/
        seL4_Yield(); 
        r_addr->costs[i] = end - start - overhead; 
    }
 
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

    uint64_t monitored_mask[MONITOR_MASK]; 

    for (int m = 0; m < MONITOR_MASK; m++)
        monitored_mask[m] = ~0LLU; 

    bench_args_t *args = env->args; 

    l1info_t l1_1 = l1_prepare(monitored_mask);

    /*the record address*/
    struct bench_cache_flush *r_addr = (struct bench_cache_flush *)args->record_vaddr;

    /*measuring the overhead: reading the timestamp counter*/
    measure_overhead(&overhead);
    r_addr->overhead = overhead; 

    /*syn with the idle thread */
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->ep, info);

    /*warming up*/
    for (int i = 0; i < BENCH_WARMUPS; i++) {
        start = sel4bench_get_cycle_count(); 
        l1_prime(l1_1); 
        end = sel4bench_get_cycle_count(); 

        seL4_Yield();
    }
    /*running benchmark*/
    for (int i = 0; i < BENCH_CACHE_FLUSH_RUNS; i++) {

        start = sel4bench_get_cycle_count(); 
        l1_prime(l1_1); 
        end = sel4bench_get_cycle_count(); 

        /*ping kernel for taking the measurements in kernel
          a context switch is invovled, switching to the idle user-level thread*/
        seL4_Yield(); 
        r_addr->costs[i] = end - start - overhead; 
    }

    /*send result to manager, benchmarking is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}
