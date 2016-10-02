/*this file contains the covert channel attack using the 
  kernel context switching time*/

/*the way this attack work: 
 trying to build contentions on L1 I and D caches
 observing the cost of context switching by timestamping on the cycle counter*/

#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "../../../bench_common.h"
#include "../mastik_common/low.h"
#include "../mastik_common/l1i.h"
#include "../ipc_test.h"


/*256 cache sets on L1 I and L1 Data*/
#define KD_TROJAN_LINES     256
#define TIME_TICK_BREAK_CYCLES     10000


/*accessing N number of L1 D cache sets*/
void static data_access(char *buf, uint32_t sets) {

    for (int s = 0; s < sets; s++) {
        for (int i = 0; i < L1_ASSOCIATIVITY; i++) {

            access(buf + s * L1_CACHELINE + i * L1_STRIDE);
        }
    }

} 


uint32_t static timer_tick(void) {

    /*waiting until a timer tick occured*/
    uint32_t start, after;
    uint64_t end, begin; 

    do 
    { 
        READ_COUNTER_ARMV7(start);
        READ_COUNTER_ARMV7(after); 

        begin = start;
        end = after;

        /*overflow*/
        if (start > after) 
            end += 1LLU << 32; 
        
        /*a preemption occured*/
        if (end - begin > TIME_TICK_BREAK_CYCLES) 
            return (uint32_t)(end - begin);

    } while (1);


}


int l3_kd_trojan(bench_covert_t *env) {

    uint32_t secret, shadow, index;
    seL4_Word badge;
    seL4_MessageInfo_t info;
    
    uint64_t  monitored_mask[I_MONITOR_MASK] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};
    
    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
    assert(l1i_1); 

    uint16_t *results = malloc(sizeof (uint16_t) * L1I_SETS); 
    assert(results);
    char *l1d_buf = malloc(L1_PROBE_BUFFER);
    assert(l1d_buf);

    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    /*receive the shared address to record the secret*/
    uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
    uint32_t volatile *syn_vaddr = share_vaddr + 1;
    *share_vaddr = 0; 

    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(env->r_ep, info);


    /*ready to do the test*/
    seL4_Send(env->syn_ep, info);


    secret = 0; 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS + 10; i++) {

        FENCE(); 

        /*wait for a new tick*/
        timer_tick(); 

#ifndef CONFIG_BENCH_DATA_SEQUENTIAL 
        secret = random() % (KD_TROJAN_LINES + 1); 
#endif

       /*using L1 I cache sets to probe */
        for (int s = 0; s < I_MONITOR_MASK; s++ ) {
            monitored_mask[s] = 0;        
        }

        index = shadow = 0; 
        while (shadow < secret) {

            monitored_mask[index] |= 1ULL << (shadow % 64); 

            if (!(++shadow % 64)) 
                index++; 
        }

        l1i_set_monitored_set(l1i_1, monitored_mask);
        l1i_probe(l1i_1, results); 
        /*L1 D cache access*/ 
        data_access(l1d_buf, secret); 

        //branch_probe(secret);
       /*update the secret read by low*/ 
        //*share_vaddr = secret; 
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
        if (++secret == KD_TROJAN_LINES + 1)
            secret = 0; 
#endif 
    }

    FENCE(); 

    while (1);

    return 0;


}



int l3_kd_spy(bench_covert_t *env) {

    seL4_Word badge;
    seL4_MessageInfo_t info;
    uint32_t sec = 0;
    
 
    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    /*the record address*/
    struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);
    uint32_t volatile *syn = secret + 1;


    /*syn with trojan*/
    info = seL4_Recv(env->syn_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
    
    timer_tick(); 

    
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE();
        /*measure the offline time*/
        r_addr->result[i] = timer_tick();
        r_addr->sec[i] = sec; 
        
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
        if (++sec == KD_TROJAN_LINES + 1)
            sec = 0; 
#endif 

    }


    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(env->r_ep, info);

    while (1);

    return 0;



}


