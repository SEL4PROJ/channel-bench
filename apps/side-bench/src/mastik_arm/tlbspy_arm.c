/*using tlb entries for covert channels on sabre*/
/*there are two levels of tlb on cortexA9, the micro tlbs (I and D) 
 have 32 entries, fully associative, and flushed during context switch as 
 a result of update the contextID register
 the unified tlb has 2 * 64 entreis, 2 way associative*/
#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "../../../bench_common.h"
#include "../mastik_common/low.h"
#include "../ipc_test.h"

#define TLB_ENTRIES  128


static inline void tlb_access(char *buf, uint32_t s) {

    /*access s tlb entries by visiting one line in each page*/
    for (int i = 0; i < s; i ++) {
        char *addr = buf + (i * 4096);
        access(addr);
        //*addr = 0xff;
       // asm volatile (""::"r"(addr): "memory");
    }

}

int tlb_trojan(bench_covert_t *env) {

  uint32_t secret;
  seL4_Word badge;
  seL4_MessageInfo_t info;
  char *buf = malloc ((TLB_ENTRIES * 4096) + 4096);

  assert(buf); 
  int temp = (int)buf; 
  temp &= ~0xfff;
  temp += 0x1000; 
  buf = (char *)temp; 
 
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
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS / (TLB_ENTRIES + 1); i++) {

      for (secret = 0; secret <= TLB_ENTRIES; secret++) {

          FENCE(); 

          while (*syn_vaddr != TROJAN_SYN_FLAG) {
              ;
          }
          FENCE();

          tlb_access(buf, secret);
          
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
     secret = random() % (TLB_ENTRIES + 1); 

     tlb_access(buf,secret);
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

int tlb_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint32_t start, after;
  
  char *buf = malloc ((TLB_ENTRIES * 4096) + 4096);
  assert(buf);
 
  int temp = (int)buf; 
  temp &= ~0xfff;
  temp += 0x1000; 
  buf = (char *)temp; 
 

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
      READ_COUNTER_ARMV7(start);
      //start = sel4bench_get_cycle_count();

      tlb_access(buf, 64);
      
      READ_COUNTER_ARMV7(after);
      //after = sel4bench_get_cycle_count();
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

