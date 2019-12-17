#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>
#include "../mastik_common/low.h"

extern uint32_t bp_probe(uint32_t secret);


int bp_trojan(bench_env_t *env) {

  uint32_t secret;
  seL4_MessageInfo_t info;

  bench_args_t *args = env->args; 
  
  volatile uint32_t *share_vaddr = (uint32_t *)args->shared_vaddr; 
  
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(args->r_ep, info);

  /*ready to do the test*/
  seL4_Send(args->ep, info);
  
  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      FENCE();
      newTimeSlice();

      secret = random() % 2;

      X_64(bp_probe(secret);)
      /*update the secret read by low*/ 
      *share_vaddr = secret; 
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
  volatile uint32_t *secret = (uint32_t *)args->shared_vaddr; 

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      newTimeSlice();
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = bp_probe(0); 
      r_addr->sec[i] = *secret; 

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);

  return 0;
}
