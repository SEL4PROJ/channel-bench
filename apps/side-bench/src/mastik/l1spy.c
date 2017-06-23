#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "../mastik_common/l1.h"
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

static void access_buffer(char *buffer, uint32_t sets) {

    uint32_t offset; 

    for (int i = 0; i < sets; i++) {
        for (int j = 0; j < L1_ASSOCIATIVITY; j++) { 
            offset = i * L1_CACHELINE + j * L1_STRIDE; 
            access(buffer + offset);
        }
    }

}

int l1_trojan(bench_covert_t *env) {
  /*buffer size 32K L1 cache size
   512 cache lines*/
  char *data = malloc(L1_PROBE_BUFFER);
  int secret = 0; 


  seL4_Word badge;
  seL4_MessageInfo_t info;

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

  /*receive the shared address to record the secret*/
  uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
  *share_vaddr = SYSTEM_TICK_SYN_FLAG; 
  
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);

  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
  

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

     /*waiting for a system tick*/
      newTimeSlice();
      secret = random() % (L1_SETS + 1);
     
      access_buffer(data, secret);
      
      /*update the secret read by low*/ 
      *share_vaddr = secret; 
     
  }
  while (1);

  return 0;
}

int l1_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;

#ifdef CONFIG_BENCH_COVERT_L1D 
  uint64_t monitored_mask[1] = {~0LLU};
#else 
  uint64_t monitored_mask[8] = {~0LLU, ~0LLU,~0LLU, ~0LLU,
      ~0LLU, ~0LLU,~0LLU, ~0LLU,};
#endif 
  l1info_t l1_1 = l1_prepare(monitored_mask);

  uint16_t *results = malloc(l1_nsets(l1_1)*sizeof(uint16_t));
  
  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
  /*the shared address*/
  uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);

  /*syn with trojan*/
  info = seL4_Recv(env->syn_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

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
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
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
