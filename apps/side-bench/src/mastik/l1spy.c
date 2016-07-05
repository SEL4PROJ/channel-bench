#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "low.h"
#include "l1.h"
#include "../../../bench_common.h"


#define TS_THRESHOLD 10000

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



int l1_trojan(bench_covert_t *env) {
  /*buffer size 32K L1 cache size
   512 cache lines*/
  char *data = malloc(4096*8);
  int total_sec = 512, secret = 0; 


  seL4_Word badge;
  seL4_MessageInfo_t info;

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*receive the shared address to record the secret*/
  uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
  *share_vaddr = SYSTEM_TICK_SYN_FLAG; 
  
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);

  /*warm up the platform*/
  for (int i = 0; i < NUM_L1D_WARMUP_ROUNDS; i++) {
      secret = random() % total_sec;
      
      for (int n = 0; n < secret; n+=64) 
	access(data+i);

  }
  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);

  /*giving some number of system ticks to syn trojan and spy */

  for (int i = 0; i < 100; i++) {
      secret = random() % total_sec;
       /*waiting for a system tick*/
      newTimeSlice();
      
      for (int n = 0; n < secret; n+=64) 
	access(data+i);

  }

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      secret = random() % total_sec; 
      
      /*waiting for a system tick*/
      newTimeSlice();
      /*update the secret read by low*/ 
      *share_vaddr = secret; 
       
      for (int n = 0; n < secret; n+=64) 
	access(data+i);
  }
  while (1);

  return 0;
}

int l1_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;

  l1info_t l1_1 = l1_prepare(~0LLU);

  uint16_t *results = malloc(l1_nsets(l1_1)*sizeof(uint16_t));
  
  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
  /*the shared address*/
  uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);

  /*syn with trojan*/
  info = seL4_Recv(env->syn_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*waiting for a start*/
  while (*secret == SYSTEM_TICK_SYN_FLAG) ;


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

    newTimeSlice();
    l1_probe(l1_1, results);

    /*result is the total probing cost
     secret is updated by trojan in the previous system tick*/
    r_addr->result[i] = 0; 
    r_addr->sec[i] = *secret; 

    for (int j = 0; j < l1_nsets(l1_1); j++) 
        r_addr->result[i] += results[j];

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);

#if 0
  /*print out the result*/
  for (int i = 1; i < 100; i++) {
      /*the variance of the probing time*/
    //printf("%3d: %10u %10u", i, times[i], times[i] - times[i-1]);
    for (int j = 0; j < l1_nsets(l1_1); j++)
        /*variance of that cache set*/
      printf("%4d ", results[i*l1_nsets(l1_1) + j]);
    printf("\n");
  }
#endif
   return 0;
}
