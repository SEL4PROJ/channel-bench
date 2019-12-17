#include <stdio.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>
#include "../mastik_common/low.h"


int l3_kd_spy(bench_env_t *env) {
 
    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 
    struct bench_kernel_schedule *r_addr = 
        (struct bench_kernel_schedule*)args->record_vaddr;
   
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr;


    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    uint32_t prev_s = *secret; 
    

    ccnt_t start = rdtscp_64();

    uint64_t prev = start;
    
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS;) {
        ccnt_t cur = rdtscp_64(); 
        /*at the begining of the current tick*/
        if (cur - prev >= TS_THRESHOLD) {
            r_addr->prevs[i] = prev;
            r_addr->starts[i] = start;
            r_addr->curs[i] = cur;
            /*prev secret affect online time (prevs - starts)*/
            r_addr->prev_sec[i] = prev_s;
            /*at the beginning of this tick, update the secret*/
            /*cur secrete affect offline time (curs - prevs)*/
            prev_s = r_addr->cur_sec[i] = *secret;
            start = cur;
            i++;
        }
        prev = cur;
    }


    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    for (;;) 
    {
    }

  
}
