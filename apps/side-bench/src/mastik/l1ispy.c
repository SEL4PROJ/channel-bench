#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "../mastik_common/l1i.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>


int l1i_trojan(bench_env_t *env) {

  uint16_t results[64];
  uint64_t l1i_prepare_sets = ~0LLU; 

  l1iinfo_t l1i_1 = l1i_prepare(&l1i_prepare_sets);
  uint32_t secret;
  uint64_t monitored_sets;  
  
  seL4_MessageInfo_t info;
  bench_args_t *args = env->args; 

  uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 
  
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(args->r_ep, info);

  
  /*ready to do the test*/
  seL4_Send(args->ep, info);
  

for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      
      /*waiting for a system tick*/
      newTimeSlice();
     
      secret = random() % (L1I_SETS + 1); 
      monitored_sets = 0ull;
      /*the range is 0 -- L1I_SETS*/
      for (int n = 0; n < secret; n++) 
          monitored_sets |= 1ull << n;

      /*set up the l1i buffer according to the monitored sets*/
      l1i_set_monitored_set(l1i_1, &monitored_sets); 

      /*do the probe*/
      l1i_probe(l1i_1, results);

      /*update the secret read by low*/ 
      *share_vaddr = secret; 

#ifdef CONFIG_BENCH_COVERT_L1I_REWRITE
      l1i_rewrite(l1i_1);
#endif 
  }
  while (1);
 
  return 0;
}

int l1i_spy(bench_env_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint64_t prepare_sets = ~0LLU; 
  bench_args_t *args = env->args; 

  ccnt_t volatile UNUSED pmu_start; 
  ccnt_t volatile UNUSED pmu_end; 

  /*spy using different virtual address to establish the probing buffer*/

#ifdef CONFIG_BENCH_COVERT_L1I_PROB_VADDR 
  l1i_prepare(&prepare_sets);

  /*creating the pbuf again therefore using the different vaddr*/
  l1i_prepare(&prepare_sets); 

#endif 

  l1iinfo_t l1i_1 = l1i_prepare(&prepare_sets);

  uint16_t *results = malloc(l1i_nsets(l1i_1)*sizeof(uint16_t));

  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
  /*the shared address*/
  uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      newTimeSlice();
#ifdef CONFIG_MANAGER_PMU_COUNTER
      pmu_start = sel4bench_get_counter(0);  
#endif 
 
      l1i_probe(l1i_1, results);

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      /*loading the pmu counter value */
      pmu_end = sel4bench_get_counter(0);  
      r_addr->pmu[i][0] = pmu_end - pmu_start; 

#endif
      
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = 0; 
      r_addr->sec[i] = *secret; 

      for (int j = 0; j < l1i_nsets(l1i_1); j++) 
          r_addr->result[i] += results[j];

#ifdef CONFIG_BENCH_COVERT_L1I_REWRITE
      l1i_rewrite(l1i_1);
#endif 

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);

  return 0;
}
