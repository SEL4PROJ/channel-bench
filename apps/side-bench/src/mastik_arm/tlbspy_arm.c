/*using tlb entries for covert channels on sabre*/
/*there are two levels of tlb on cortexA9, the micro tlbs (I and D) 
 have 32 entries, fully associative, and flushed during context switch as 
 a result of update the contextID register
 the unified tlb has 2 * 64 entreis, 2 way associative*/
#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>
#include "../mastik_common/low.h"

static inline void tlb_access(char *buf, uint32_t s) {

    /*access s tlb entries by visiting one line in each page
     randomly select a line to visit in a page*/
    /*the attack range is 0 - s pages*/

    for (int i = 0; i < s; i++) { 
        
        char *addr = buf + i * PAGE_SIZE + random() % PAGE_SIZE; 
 
        /*align to a cache line size*/
        addr = (void *)((uintptr_t)addr & ~(L1_CACHELINE - 1)); 
        low_access(addr);
    }

}

int tlb_trojan(bench_env_t *env) {

    uint32_t secret;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 
    
    char *buf = malloc((TLB_ENTRIES * PAGE_SIZE) + PAGE_SIZE);
    assert(buf); 

    buf = (char*)ALIGN_PAGE_SIZE(buf); 

    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);
    

    /*ready to do the test*/
    seL4_Send(args->ep, info);
    secret = 0; 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();

        secret = random() % (TLB_ENTRIES + 1); 

        tlb_access(buf, secret);

        /*update the secret read by low*/ 
        *share_vaddr = secret; 
    }

    while (1);

    return 0;
}

int tlb_spy(bench_env_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  ccnt_t start, after;
#ifdef CONFIG_MANAGER_PMU_COUNTER 
  uint32_t pmu_start[BENCH_PMU_COUNTERS]; 
  uint32_t pmu_end[BENCH_PMU_COUNTERS]; 
#endif 
  bench_args_t *args = env->args; 

  char *buf = malloc ((TLB_ENTRIES  * PAGE_SIZE) + PAGE_SIZE);
  assert(buf); 

  buf = (char*)ALIGN_PAGE_SIZE(buf); 

  struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
  /*the shared address*/
  uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      FENCE(); 
      newTimeSlice();

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_start);  
#endif 
      SEL4BENCH_READ_CCNT(start);  

      tlb_access(buf, TLB_PROBE_PAGES);
      
      SEL4BENCH_READ_CCNT(after);  
     
#ifdef CONFIG_MANAGER_PMU_COUNTER 
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_end);  
#endif 

      r_addr->result[i] = after - start; 
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->sec[i] = *secret;

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      /*loading the pmu counter value */
      for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
          r_addr->pmu[i][counter] = pmu_end[counter] - pmu_start[counter]; 
#endif 
  
  }
  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);

  return 0;
}

