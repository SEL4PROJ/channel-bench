/*The L1 Data cache attack on sabre platform 
sabre L1 D cache, 32B cache line, 4 ways, physically indexed, physically tagged
 256 sets in total*/

#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "bench_common.h"
#include "bench_types.h"
#include "../mastik_common/low.h"
#include "../mastik_common/l1.h"
#include "../ipc_test.h"

/*accessing N number of L1 D cache sets*/
static void data_access(char *buf, uint32_t sets) {

    for (int s = 0; s < sets; s++) {
        for (int i = 0; i < L1_ASSOCIATIVITY; i++) {

            access(buf + s * L1_CACHELINE + i * L1_STRIDE);
        }
    }
}

int l1_trojan(bench_env_t *env) {

    uint32_t  secret;
    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args;

    /*buffer size 32K L1 cache size
      1024 cache lines*/
    char *data = malloc(4096 * 8);
    assert(data != NULL);

    /*receive the shared address to record the secret*/
    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr;
    uint32_t volatile *syn_vaddr = share_vaddr + 1;
    *share_vaddr = 0; 

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*ready to do the test*/
    seL4_Send(args->ep, info);
    secret = 0; 

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 

        while (*syn_vaddr != TROJAN_SYN_FLAG) {
            ;
        }
        FENCE();

        secret = random() % (L1_SETS + 1); 

        data_access(data, secret);

        *share_vaddr = secret; 
        
        /*wait until spy set the flag*/
        *syn_vaddr = SPY_SYN_FLAG;

    }

    FENCE(); 

    while (1);

    return 0;
}

int l1_spy(bench_env_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  bench_args_t *args = env->args;
  
  uint32_t UNUSED start, after;
  uint32_t UNUSED pmu_start[BENCH_PMU_COUNTERS]; 
  uint32_t UNUSED pmu_end[BENCH_PMU_COUNTERS]; 

  uint64_t  monitored_mask[4] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};

  l1info_t l1_1 = l1_prepare(monitored_mask);

  uint16_t *results = malloc(l1_nsets(l1_1)*sizeof(uint16_t));
 
  /*the record address*/
  struct bench_l1 *r_addr = args->record_vaddr; 

  /*the shared address*/
  uint32_t volatile *secret = args->shared_vaddr; 
  uint32_t volatile *syn = secret + 1;
  *syn = TROJAN_SYN_FLAG;

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      
      FENCE(); 
    
      while (*syn != SPY_SYN_FLAG) 
      {
          ;
      }
     
      FENCE(); 

      /*reset the counter to zero*/
      //sel4bench_reset_cycle_count();

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_start);  
#endif 
      //READ_COUNTER_ARMV7(start);
      /*start */
      l1_probe(l1_1, results);

     // READ_COUNTER_ARMV7(after);
#ifdef CONFIG_MANAGER_PMU_COUNTER 
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_end);  
#endif
      //r_addr->result[i] = after - start; 

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      /*loading the pmu counter value */
      for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
          r_addr->pmu[i][counter] = pmu_end[counter] - pmu_start[counter]; 

#endif 

      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = 0; 
      r_addr->sec[i] = *secret; 

      for (int j = 0; j < l1_nsets(l1_1); j++) 
          r_addr->result[i] += results[j];

      /*spy set the flag*/
      *syn = TROJAN_SYN_FLAG; 
     
  }
 
  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);
  
  return 0;
}
