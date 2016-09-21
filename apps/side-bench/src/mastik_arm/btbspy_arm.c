/*this file contains the covert channel attack using the 
  branch target buffer*/

/*there are two way branch target cache, 2 way 256 entries, total 512 entries*/

/*the way that the attack work:
  trojan send signal with 0-512 branches, but the spy uses 256 branches all 
  the time*/


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

//#define BTAC_ENTRIES  1024  /*512 entries but each probe does 2 branches*/

/*total 256 sets */
#define BTAC_TROJAN_SETS    256
/*one sets contain 4 branch*/
#define BTAC_SPY_SETS       256

#if 0
extern void arm_branch_lines(void); 


static void branch_probe(uint32_t n) {

    /*probe on N number of branch instructions then return*/
    if (!n)
        return; 
    asm volatile (
            "mov r1, %0\n"
            "bl arm_branch_lines" : : "r" (n): "lr", "r0", "r1"); 

}
#endif 

int btb_trojan(bench_covert_t *env) {

    uint32_t secret, shadow, index;
    seL4_Word badge;
    seL4_MessageInfo_t info;
    
    uint64_t  monitored_mask[I_MONITOR_MASK] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};
    
    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
    assert(l1i_1); 

    uint16_t *results = malloc(sizeof (uint16_t) * L1I_SETS); 
    assert(results);

    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    /*receive the shared address to record the secret*/
    uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
    uint32_t volatile *syn_vaddr = share_vaddr + 4;
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
        secret = random() % (BTAC_TROJAN_SETS + 1); 
#endif
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

        //branch_probe(secret);
       /*update the secret read by low*/ 
        *share_vaddr = secret; 
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
        if (++secret == BTAC_TROJAN_SETS + 1)
            secret = 0; 
#endif 
        /*wait until spy set the flag*/
        *syn_vaddr = SPY_SYN_FLAG;

    }

    FENCE(); 

    while (1);

    return 0;
}

int btb_spy(bench_covert_t *env) {

    seL4_Word badge;
    seL4_MessageInfo_t info;
    //uint32_t start, after;

    uint64_t  monitored_mask[I_MONITOR_MASK] = {0};

    uint32_t index = 0, shadow = 0; 
    
    while (shadow < BTAC_SPY_SETS) {
        monitored_mask[index] |= 1ULL << (shadow % 64); 

        if (!(++shadow % 64)) 
            index++; 
    }

    l1iinfo_t l1i_1 = l1i_prepare(monitored_mask);
    assert(l1i_1); 

    uint16_t *results = malloc(sizeof (uint16_t) * L1I_SETS); 
    assert(results);

    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    /*the record address*/
    struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);
    uint32_t volatile *syn = secret + 4;
    *syn = TROJAN_SYN_FLAG;

    /*syn with trojan*/
    info = seL4_Recv(env->syn_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 

        while (*syn != SPY_SYN_FLAG) 
        {
            ;
        }

        FENCE(); 
        /*reset the counter to zero*/
        sel4bench_reset_cycle_count();
        l1i_probe(l1i_1, results); 
        r_addr->result[i] = 0; 
        for (int s = 0; s < BTAC_SPY_SETS; s++) {
            r_addr->result[i] += results[s];
        }
    //    READ_COUNTER_ARMV7(start);
        /*probing on half of the branch buffer*/
      //  branch_probe(BTAC_SPY_ENTRIES);

     //   READ_COUNTER_ARMV7(after);
        //r_addr->result[i] = after - start; 
        
        /*result is the total probing cost
          secret is updated by trojan in the previous system tick*/
        r_addr->sec[i] = *secret; 
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

