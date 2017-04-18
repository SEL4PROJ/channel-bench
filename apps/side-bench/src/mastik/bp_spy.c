#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "../../../bench_common.h"

#define WARMUP_ROUNDS 0x1000 

#define TS_THRESHOLD 100000

#define X_4(a) a a a a 
#define X_64(a) X_4(X_4(X_4(a)))

extern uint32_t bp_probe(uint32_t secret);


static void  newTimeSlice(){
  asm("");
  uint32_t prev = rdtscp();
  for (;;) {
    uint32_t cur = rdtscp();
    if (cur - prev > TS_THRESHOLD)
      return;
    prev = cur;
  }
}



int bp_trojan(bench_covert_t *env) {

  uint32_t total_sec = 512, secret;
  seL4_Word badge;
  seL4_MessageInfo_t info;

  srandom(rdtscp());

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  /*receive the shared address to record the secret*/
  volatile uint32_t *share_vaddr = (uint32_t *)seL4_GetMR(0);
  *share_vaddr = SYSTEM_TICK_SYN_FLAG; 
  
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);

  /*warm up the platform*/
  for (int i = 0; i < WARMUP_ROUNDS; i++) {
      secret = random() & 1;
      
      X_64(bp_probe(secret);)
  }
  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
  
  /*giving some number of system ticks to syn trojan and spy */

  for (int i = 0; i < 100; i++) {
      secret = random() % total_sec;
      newTimeSlice();
      X_64(bp_probe(secret);)
      
  }

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      secret = (random() / 2) & 1;
      
      /*update the secret read by low*/ 
      *share_vaddr = secret; 

  asm("");
  uint32_t prev = rdtscp();
  for (;;) {
    uint32_t cur = rdtscp();
    X_64(bp_probe(secret & 1);)
    if (cur - prev > TS_THRESHOLD)
      break;
    prev = cur;
  }
      
  }
  while (1);
 
  return 0;
}

int bp_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
  /*the shared address*/
  volatile uint32_t *secret = (uint32_t *)seL4_GetMR(1);

  /*syn with trojan*/
  info = seL4_Recv(env->syn_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  /*waiting for a start*/
  while (*secret == SYSTEM_TICK_SYN_FLAG) ;


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      newTimeSlice();
   
      //yval
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = bp_probe(0); 
      r_addr->sec[i] = *secret; 

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);


  return 0;
}
