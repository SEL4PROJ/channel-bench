#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "../../../bench_common.h"
#include "bench_types.h"


#define WARMUP_ROUNDS 0x1000 

#define TS_THRESHOLD 400000

#define X_4(a) a a a a 
#define X_64(a) X_4(X_4(X_4(a)))

extern uint32_t bp_probe(uint32_t secret);


static void  newTimeSlice(){
  asm("");
  uint32_t cur, prev; 

  READ_COUNTER_ARMV7(prev);

  for (;;) {
      READ_COUNTER_ARMV7(cur);
    if (cur - prev > TS_THRESHOLD)
      return;
    prev = cur;
  }
}



int bp_trojan(bench_env_t *env) {

  uint32_t secret;
  seL4_Word badge;
  seL4_MessageInfo_t info;

  uint32_t cur, prev; 
  bench_args_t *args = env->args; 
  
  READ_COUNTER_ARMV7(prev);

  srandom(prev);
  volatile uint32_t *share_vaddr = args->shared_vaddr; 
  *share_vaddr = SYSTEM_TICK_SYN_FLAG; 
  
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(args->r_ep, info);

  /*warm up the platform*/
  for (int i = 0; i < WARMUP_ROUNDS; i++) {
      secret = random() & 1;
      
      X_64(bp_probe(secret);)
  }
  /*ready to do the test*/
  seL4_Send(args->ep, info);
  
  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      FENCE(); 


      secret = random() % 2;
      
      /*update the secret read by low*/ 
      *share_vaddr = secret; 

      asm("");
      READ_COUNTER_ARMV7(prev);

      for (;;) {
          READ_COUNTER_ARMV7(cur);
          X_64(bp_probe(secret);)
              if (cur - prev > TS_THRESHOLD)
                  break;
          prev = cur;
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
  volatile uint32_t *secret = args->shared_vaddr; 

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  /*waiting for a start*/
  while (*secret == SYSTEM_TICK_SYN_FLAG) ;


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
