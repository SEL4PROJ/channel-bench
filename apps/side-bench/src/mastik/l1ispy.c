#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "../mastik_common/l1i.h"
#include "../../../bench_common.h"

#define TS_THRESHOLD 10000

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


int l1i_trojan(bench_covert_t *env) {

  uint16_t results[64];
  uint64_t l1i_prepare_sets = ~0LLU; 

  l1iinfo_t l1i_1 = l1i_prepare(&l1i_prepare_sets);
  uint32_t total_sec = L1I_SETS, secret;
  uint64_t monitored_sets;  
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
      secret = random() % (total_sec + 1); 
      
      /*waiting for a system tick*/
      newTimeSlice();
     
      monitored_sets = 0ull;
      for (int n = 0; n < secret; n++) 
          monitored_sets |= 1ull << n;

      /*set up the l1i buffer according to the monitored sets*/
      l1i_set_monitored_set(l1i_1, &monitored_sets); 

      /*do the probe*/
      l1i_probe(l1i_1, results);


#ifdef CONFIG_BENCH_COVERT_L1I_REWRITE
    /*rewrite the L1I probing buffer*/
    asm volatile (
                    "xorq  %%rax, %%rax\n"  /*read then writeback the 32K probing buffer*/
                    "1: \n"
                    "movq  (%%rax, %0, 1), %%rbx\n"
                    "movq  %%rbx, (%%rax, %0, 1)\n"
                    "clflush (%%rax, %0, 1)\n"
                    "addq  $8, %%rax\n"
                    "cmpq  $32768, %%rax\n"
                    "jl   1b\n"
                    "mfence\n"
                    : "+r" (l1i_1->memory)
                    : 
                    : "rax", "rbx", "memory"
                    );

#endif 

      /*update the secret read by low*/ 
      *share_vaddr = secret; 
  }
  while (1);
 
  return 0;
}

int l1i_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint64_t prepare_sets = ~0LLU; 

  uint64_t volatile pmu_start; 
  uint64_t volatile pmu_end; 

  /*spy using different virtual address to establish the probing buffer*/
  l1iinfo_t l1i_1 = l1i_prepare(&prepare_sets);


  uint16_t *results = malloc(l1i_nsets(l1i_1)*sizeof(uint16_t));


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
    /*rewrite the L1I probing buffer*/
    asm volatile (
                    "xorq  %%rax, %%rax\n"  /*read then writeback the 32K probing buffer*/
                    "1: \n"
                    "movq  (%%rax, %0, 1), %%rbx\n"
                    "movq  %%rbx, (%%rax, %0, 1)\n"
                    "clflush (%%rax, %0, 1)\n"
                    "addq  $8, %%rax\n"
                    "cmpq  $32768, %%rax\n"
                    "jl   1b\n"
                    "mfence\n"
                    : "+r" (l1i_1->memory)
                    : 
                    : "rax", "rbx", "memory"
                    );

#endif 
  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);


  return 0;
}
