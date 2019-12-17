/*this file contains the covert channel attack using the 
  branch target buffer*/

/*there are two way branch target cache, 2 way 256 entries, total 512 entries*/

/*the way that the attack work:
  trojan send signal with 0-512 branches, but the spy uses 256 branches all 
  the time*/


#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_helper.h>
#include <channel-bench/bench_types.h>
#include "../mastik_common/low.h"

/*using branch instructions to do the probe, 4 bytes aligned
  defined in branch_probe.S 
 */

/*attack: trojan probs on 0 - N number of branch instructions
          spy probs on N numbers of branch instructions 
          N == branch target buffer entries */

extern void arm_branch_lines(void); 


void branch_probe_lines(uint32_t n) {

    /*n == 0, no probing */
    if (!n)
        return;

    uint32_t start = (uint32_t)arm_branch_lines; 

    /*calculate where to start to probing if just one line, 
     starting from arm_branch_line + 511 * 4*/
    start += (BTAC_ENTRIES - n ) * INSTRUCTION_LENGTH; 

#ifdef CONFIG_ARCH_AARCH64
    asm volatile ("blr %0" : : "r" (start): "x30"); 
#else
    asm volatile ("blx %0" : : "r" (start): "lr"); 
#endif
}

int btb_trojan(bench_env_t *env) {

    uint32_t secret;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 

    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 
    *share_vaddr = 0; 

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*ready to do the test*/
    seL4_Send(args->ep, info);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();

        secret = random() % (BTAC_ENTRIES  + 1); 

        /*using the instructions to probe*/
        branch_probe_lines(secret);

        /*update the secret read by low*/ 
        *share_vaddr = secret; 
    }

    while (1);

    return 0;
}

int btb_spy(bench_env_t *env) {

    seL4_Word badge;
    seL4_MessageInfo_t info;
    uint32_t start, after;
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    ccnt_t volatile pmu_start; 
    ccnt_t volatile pmu_end; 
#endif
    bench_args_t *args = env->args; 

    struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();

        //sel4bench_reset_cycle_count();
#ifdef CONFIG_MANAGER_PMU_COUNTER 
        pmu_start = sel4bench_get_counter(0);  
#endif 
        SEL4BENCH_READ_CCNT(start); 

        /*using instrcutions to probe*/
        branch_probe_lines(BTAC_ENTRIES);

        SEL4BENCH_READ_CCNT(after);

#ifdef CONFIG_MANAGER_PMU_COUNTER 
        pmu_end = sel4bench_get_counter(0);  
#endif 

        r_addr->result[i] = after - start; 

#ifdef CONFIG_MANAGER_PMU_COUNTER 
        /*loading the pmu counter value */
        for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
            r_addr->pmu[i][counter] = pmu_end - pmu_start; 

#endif 
        /*result is the total probing cost
          secret is updated by trojan in the previous system tick*/
        r_addr->sec[i] = *secret; 
    }


    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}

