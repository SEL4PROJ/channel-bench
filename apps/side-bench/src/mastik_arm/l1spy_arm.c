/*The L1 Data cache attack on sabre platform 
sabre L1 D cache, 32B cache line, 4 ways, physically indexed, physically tagged
 256 sets in total*/

#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "../../../bench_common.h"
#include "../mastik_common/low.h"
#include "../mastik_common/l1.h"
#include "../ipc_test.h"

#define L1D_TROJAN_SETS 256


/*accessing N number of L1 D cache sets*/
static void data_access(char *buf, uint32_t sets) {

    for (int s = 0; s < sets; s++) {
        for (int i = 0; i < L1_ASSOCIATIVITY; i++) {

            access(buf + s * L1_CACHELINE + i * L1_STRIDE);
        }
    }

}

int l1_trojan(bench_covert_t *env) {

  uint32_t  secret;
  seL4_Word badge;
  seL4_MessageInfo_t info;
  
  /*buffer size 32K L1 cache size
   1024 cache lines*/
  char *data = malloc(4096 * 8);

  assert(data != NULL);

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*receive the shared address to record the secret*/
  uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
  uint32_t volatile *syn_vaddr = share_vaddr + 1;
  *share_vaddr = 0; 
  
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);
  
  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
  secret = 0; 

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      FENCE(); 

      while (*syn_vaddr != TROJAN_SYN_FLAG) {
          ;
      }
      FENCE();

#ifndef CONFIG_BENCH_DATA_SEQUENTIAL 
      secret = random() % (L1D_TROJAN_SETS + 1); 
#endif

      data_access(data, secret);
      
      *share_vaddr = secret; 
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
      if (++secret == L1D_TROJAN_SETS + 1)
          secret = 0; 
#endif 
      /*wait until spy set the flag*/
      *syn_vaddr = SPY_SYN_FLAG;

  }

 
 FENCE(); 

 while (1);
 
  return 0;
}

int l1_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;

  uint64_t  monitored_mask[4] = {~0LLU, ~0LLU, ~0LLU, ~0LLU};

  l1info_t l1_1 = l1_prepare(monitored_mask);

  uint16_t *results = malloc(l1_nsets(l1_1)*sizeof(uint16_t));
 
  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
  
  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
  /*the shared address*/
  uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);
  uint32_t volatile *syn = secret + 1;
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
      /*start */
      l1_probe(l1_1, results);

      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = 0; 
      r_addr->sec[i] = *secret; 

      for (int j = 0; j < l1_nsets(l1_1); j++) 
          r_addr->result[i] += results[j];

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
