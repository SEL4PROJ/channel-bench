/*the l1i channel for arm platforms*/
#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "../../../bench_common.h"
#include "../ipc_test.h"

extern void arm_branch_spy(void); 
extern void arm_branch_trojan_1(void); 
extern void arm_branch_trojan_2(void); 
extern void arm_branch_trojan_3(void); 
extern void arm_branch_trojan_4(void); 
extern void arm_branch_trojan_5(void); 
extern void arm_branch_trojan_6(void); 
extern void arm_branch_trojan_7(void); 
extern void arm_branch_trojan_8(void); 



static inline void l1i_trojan_branch_select(uint32_t n) {

    if (n == 1) {
        asm volatile ("bl arm_branch_trojan_1" :::"lr"); 
            //arm_branch_trojan_1();
     } else if (n == 2)
            //arm_branch_trojan_2();
            asm volatile ("bl arm_branch_trojan_2" :::"lr"); 
    else if (n == 3)       
//     arm_branch_trojan_3();
            asm volatile ("bl arm_branch_trojan_3" :::"lr"); 
    else if (n == 4)        
  //      arm_branch_trojan_4(); 
            asm volatile ("bl arm_branch_trojan_4" :::"lr"); 
    else if (n == 5)
            //arm_branch_trojan_5();
           asm volatile ("bl arm_branch_trojan_5" :::"lr"); 
    else if (n == 6) 
        //arm_branch_trojan_6();
            asm volatile ("bl arm_branch_trojan_6" :::"lr"); 
    else if (n == 7)        
        //arm_branch_trojan_7();
            asm volatile ("bl arm_branch_trojan_7" :::"lr"); 
    else if (n == 8)        
        //arm_branch_trojan_8();
        asm volatile ("bl arm_branch_trojan_8":::"lr");

    else 
        ;
}

int l1i_trojan(bench_covert_t *env) {

  uint32_t total_sec = 9, secret;
  seL4_Word badge;
  seL4_MessageInfo_t info;
  
  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*receive the shared address to record the secret*/
  uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
  uint32_t volatile *syn_vaddr = share_vaddr + 4;
  *share_vaddr = 0; 
  
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);
  

  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS / total_sec; i++) {

      for (secret = 0; secret < total_sec; secret++) {

          FENCE(); 

          while (*syn_vaddr != TROJAN_SYN_FLAG) {
              ;
          }
          FENCE();

          l1i_trojan_branch_select(secret);
          /*update the secret read by low*/ 
          *share_vaddr = secret; 
          /*wait until spy set the flag*/
          *syn_vaddr = SPY_SYN_FLAG;
      }
      
  }

 FENCE(); 
#else 
 for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {



     FENCE(); 

     while (*syn_vaddr != TROJAN_SYN_FLAG) {
         ;
     }
     FENCE();
     secret = random() % total_sec; 

     l1i_trojan_branch_select(secret);
     /*update the secret read by low*/ 
     *share_vaddr = secret; 
     /*wait until spy set the flag*/
     *syn_vaddr = SPY_SYN_FLAG;

      
  }

 FENCE(); 
#endif 
 
  while (1);
 
  return 0;
}

#if 0
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
int l1i_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint32_t start, after;
  
 
  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
  
  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
  /*the shared address*/
  uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);
  uint32_t volatile *syn = secret + 4;
  *syn = TROJAN_SYN_FLAG;

  /*syn with trojan*/
  info = seL4_Recv(env->syn_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

 

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      
      FENCE(); 
    
      while (*syn != SPY_SYN_FLAG) 
      {
          ;
      }
     
      FENCE(); 
      /*reset the counter to zero*/
      sel4bench_reset_cycle_count();
      start = sel4bench_get_cycle_count();

      asm volatile ("bl arm_branch_spy"::: "lr"); 
      /*using the nops to test the benchmark*/
      after = sel4bench_get_cycle_count();
      r_addr->result[i] = after - start; 
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->sec[i] = *secret; 
      /*spy set the flag*/
      *syn = TROJAN_SYN_FLAG; 
     
  }
 

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);


  return 0;
}
#if 0
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
