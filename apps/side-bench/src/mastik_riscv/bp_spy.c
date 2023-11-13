#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>


extern uint32_t bp_probe(uint32_t secret);


int bp_trojan(bench_env_t *env) {

  uint32_t secret;
  seL4_MessageInfo_t info;
  bench_args_t *args = (bench_args_t *)env->args; 


  /*receive the shared address to record the secret*/
  volatile uint32_t *share_vaddr = (uint32_t*)args->shared_vaddr; 

  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(args->r_ep, info);

  seL4_Send(args->ep, info);
  
  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      
      if (i % 1000 == 0 || (i - 1) % 1000 == 0 || i == (CONFIG_BENCH_DATA_POINTS - 1)) printf("TROJAN: Data point %d\n", i);

      /*waiting for a system tick*/
      newTimeSlice();

      secret = random() % (BHT_ENTRIES + 1);
      /*update the secret read by low*/ 
      *share_vaddr = secret; 

      for (int i = 0; i < 16; i++) {
          bp_probe(secret);
      }
      
  }
  while (1);
 
  return 0;
}

int bp_spy(bench_env_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  bench_args_t *args = env->args; 


  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
  /*the shared address*/
  volatile uint32_t *secret = (uint32_t*)args->shared_vaddr;

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      if (i % 1000 == 0 || (i - 1) % 1000 == 0 || i == (CONFIG_BENCH_DATA_POINTS - 1)) printf("SPY: Data point %d\n", i);

      newTimeSlice();
   
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = bp_probe(0); 
      r_addr->sec[i] = *secret;

      /* Prime (make sure all saturation counters are reset) */
      for (int i = 0; i < 16; i++) {
          bp_probe(0);
      }

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);

  return 0;
}
