#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "l1i.h"
#include "../../../bench_common.h"

#define TS_THRESHOLD 10000
extern void nop_spy(void);
extern void nop_trojan_1(void);
extern void nop_trojan_2(void);
extern void nop_trojan_3(void);
extern void nop_trojan_4(void);
extern void nop_trojan_5(void);
extern void nop_trojan_6(void);
extern void nop_trojan_7(void);
extern void nop_trojan_8(void);


static void  newTimeSlice(){
  asm("");
  uint64_t  prev = rdtscp_64();
  for (;;) {
    uint64_t cur = rdtscp_64();
    if (cur - prev > TS_THRESHOLD)
      return;
    prev = cur;
  }
}

static void l1i_trojan_nop_select(uint32_t n) {

    switch(n) {

        case 0: 
            break;
        case 1:
            nop_trojan_1(); 
            for (int p = 0; p < 4096 + 4096; p += 64)
                clflush(nop_trojan_1 + p); 
            break;
        case 2:
            nop_trojan_2(); 
            for (int p = 0; p < 4096 * 2 + 4096; p += 64)
                clflush(nop_trojan_2 + p); 
            break;
        case 3:
            nop_trojan_3(); 
            for (int p = 0; p < 4096 * 3 + 4096; p += 64)
                clflush(nop_trojan_3 + p); 
            break;
        case 4:
   
            nop_trojan_4(); 
            for (int p = 0; p < 4096 * 4 + 4096; p += 64)
                clflush(nop_trojan_4 + p); 
            break;

        case 5:
            nop_trojan_5(); 
            for (int p = 0; p < 4096 * 5 + 4096; p += 64)
                clflush(nop_trojan_5 + p); 

            break;
        case 6:
            nop_trojan_6();
            for (int p = 0; p < 4096 * 6 + 4096; p += 64)
                clflush(nop_trojan_6 + p); 

            break;
        case 7:
            nop_trojan_7();
            for (int p = 0; p < 4096 * 7 + 4096; p += 64)
                clflush(nop_trojan_7 + p); 

            break;
        case 8:
            nop_trojan_8();
            for (int p = 0; p < 4096 * 8 + 4096; p += 64)
                clflush(nop_trojan_8 + p); 

            break;
        default:
            break;
    }

}
#if 0
int l1i_trojan(bench_covert_t *env) {

  uint32_t total_sec = 9, secret;
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
  for (int i = 0; i < NUM_L1I_WARMUP_ROUNDS; i++) {
      secret = random() % total_sec; 

      /*do the probe*/
      l1i_trojan_nop_select(secret);
  }
  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
  
  /*giving some number of system ticks to syn trojan and spy */

  for (int i = 0; i < 100; i++) {
      secret = random() % total_sec;
      newTimeSlice();
      
      /*do the probe*/
      l1i_trojan_nop_select(secret);
  }

for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      secret = random() % total_sec; 
      
      /*waiting for a system tick*/
      newTimeSlice();
      l1i_trojan_nop_select(secret);
      /*update the secret read by low*/ 
      *share_vaddr = secret; 
    
      
  }
  while (1);
 
  return 0;
}
#endif 
#if 1
int l1i_trojan(bench_covert_t *env) {

  uint16_t results[64];
  l1iinfo_t l1i_1 = l1i_prepare(~0LLU);
  uint32_t total_sec = L1I_SETS, secret;
  uint64_t monitored_sets;  
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
  for (int i = 0; i < NUM_L1I_WARMUP_ROUNDS; i++) {
      secret = random() % total_sec;
      
      monitored_sets = 0ull;
      for (int n = 0; n < secret; n++) 
          monitored_sets |= 1ull << n;

      /*set up the l1i buffer according to the monitored sets*/
      l1i_set_monitored_set(l1i_1, monitored_sets); 
      /*do the probe*/
      l1i_probe(l1i_1, results);
  }
  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
  
  /*giving some number of system ticks to syn trojan and spy */

  for (int i = 0; i < 100; i++) {
      secret = random() % total_sec;
      newTimeSlice();
      
      monitored_sets = 0ull;
      for (int n = 0; n < secret; n++) 
          monitored_sets |= 1ull << n;

      /*set up the l1i buffer according to the monitored sets*/
      l1i_set_monitored_set(l1i_1, monitored_sets); 
      /*do the probe*/
      l1i_probe(l1i_1, results);
     
  }

for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      secret = random() % total_sec; 
      
      /*waiting for a system tick*/
      newTimeSlice();
      /*update the secret read by low*/ 
      *share_vaddr = secret; 

      monitored_sets = 0ull;
      for (int n = 0; n < secret; n++) 
          monitored_sets |= 1ull << n;

      /*set up the l1i buffer according to the monitored sets*/
      l1i_set_monitored_set(l1i_1, monitored_sets); 
      /*do the probe*/
      l1i_probe(l1i_1, results);
     // for (int p = 0; p < 32768; p += 64) 
       //   clflush(l1i_1->memory + p);
      
  }
  while (1);
 
  return 0;
}
#endif
#if 0
int l1i_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;

  /*spy using different virtual address to establish the probing buffer*/


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
  
      /*using the nops to test the benchmark*/
      r_addr->result[i] = l1i_probe_nop();
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->sec[i] = *secret; 
     
      for (int p = 0; p < 32768 + 4096; p += 64) 
          clflush(nop_spy + p);
  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);


  return 0;
}
#endif 
#if 1
int l1i_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;

  /*spy using different virtual address to establish the probing buffer*/
  l1iinfo_t l1i_1 = l1i_prepare(~0LLU);
  l1i_1 = l1i_prepare(~0LLU);
  l1i_1 = l1i_prepare(~0LLU);
  l1i_1 = l1i_prepare(~0LLU);



  uint16_t *results = malloc(l1i_nsets(l1i_1)*sizeof(uint16_t));


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
  
      /*using the nops to test the benchmark*/
      l1i_probe_1(l1i_1, results);
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = 0; 
      r_addr->sec[i] = *secret; 

      for (int j = 0; j < l1i_nsets(l1i_1); j++) 
          r_addr->result[i] += results[j];

      for (int p = 0; p < 32768; p += 64) 
          clflush(l1i_1->memory + p);
      
  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);


  return 0;
}
#endif 
