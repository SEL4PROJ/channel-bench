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
#include "l3_arm.h"


/*256 cache sets on L1 I and L1 Data*/
//#define KD_TROJAN_LINES     256
/*the 256 K buffer to access*/
#define KD_TROJAN_LINES 256


/*accessing N number of L1 D cache sets*/
void data_access(char *buf, uint32_t sets) {

    for (int s = 0; s < sets; s++) {
        for (int i = 0; i < L1_ASSOCIATIVITY; i++) {

            access(buf + s * L1_CACHELINE + i * L1_STRIDE);
        }
    }

} 


int l3_kd_trojan(bench_covert_t *env) {

    uint32_t secret, shadow, index;
    seL4_Word badge;
    seL4_MessageInfo_t info;
    
    uint64_t  monitored_mask[I_MONITOR_MASK] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};
    
    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
    assert(l1i_1); 

    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);

#ifdef CONFIG_DEBUG_BUILD
    printf("trojan: Got %d sets\n", nsets);
#endif


    uint16_t *results = malloc(sizeof (uint16_t) * L1I_SETS); 
    assert(results);
    char *l1d_buf = malloc(L1_PROBE_BUFFER);
    assert(l1d_buf);
    uint16_t *l3_kd_results = malloc(sizeof (uint16_t) * nsets); 
    assert(l3_kd_results);

 
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

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 

        while (*syn_vaddr != TROJAN_SYN_FLAG) {
            ;
        }
        FENCE();

#ifndef CONFIG_BENCH_DATA_SEQUENTIAL 
        secret = random() % (KD_TROJAN_LINES + 1); 
#endif
#if 1 
        /*using the LLC to probe*/
        l3_unmonitorall(l3);
        for (int s = 0; s < secret; s++) {
            l3_monitor(l3, s);
        }
        /*do simple probe, monitor already touches the caches by linking the cache lines*/
        //l3_probe(l3, l3_kd_results); 
 
#endif
#if 0
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
#endif 
        //branch_probe(secret);
       /*update the secret read by low*/ 
        *share_vaddr = secret; 
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
        if (++secret == KD_TROJAN_LINES + 1)
            secret = 0; 
#endif 
        /*spy go*/
        *syn_vaddr = SPY_SYN_FLAG;

    }

    FENCE(); 

    while (1);

    return 0;


}



int l3_kd_spy(bench_covert_t *env) {

    seL4_Word badge;
    seL4_MessageInfo_t info;
    uint32_t volatile cur;
    uint32_t volatile prev;
 
    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    /*the record address*/
    struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);
    uint32_t volatile *syn = secret + 1;


    *syn = TROJAN_SYN_FLAG;

 
    /*syn with trojan*/
    info = seL4_Recv(env->syn_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
     /*reset the counter to zero*/
    sel4bench_reset_cycle_count();


    READ_COUNTER_ARMV7(prev);
       
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE();
#if 1
        /*measure the offline time*/
         while (*syn != SPY_SYN_FLAG) {
             ;
         }

        FENCE();
        READ_COUNTER_ARMV7(cur);
        /*reset the counter to zero*/
    //    sel4bench_reset_cycle_count();

#endif   
        if (prev > cur) {
            uint64_t this = (uint64_t)cur + (1llu << 32);
            r_addr->result[i] = this - prev;
        }
        else 
            r_addr->result[i] = cur - prev; 
        
        r_addr->sec[i] = *secret; 
        prev = cur; 

        /*spy set the flag*/
        *syn = TROJAN_SYN_FLAG; 

    }


    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(env->r_ep, info);

    while (1);

    return 0;



}


