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

#ifdef CONFIG_PLAT_SABRE
#define TLB_ENTRIES  128 
#ifdef RANDOM_TLB_ENTRIES 
#define ATTACK_PAGES  (128 * 5)
#else 
#define ATTACK_PAGES 128
#endif 
#define PROBE_PAGES 64
#endif

#ifdef CONFIG_PLAT_HIKEY
#define TLB_ENTRIES  512
#define ATTACK_PAGES 512
#define PROBE_PAGES 256
#endif

#ifdef CONFIG_PLAT_TX1
#define TLB_ENTRIES  1024
#define ATTACK_PAGES 1024
#define PROBE_PAGES 512
#endif

static inline void tlb_access(char *buf, uint32_t *seq, uint32_t s) {

    /*access s tlb entries by visiting one line in each page
     using random ordered page list and a random cache line in a page*/

    for (int i = 0; i < s; i++) { 
#ifdef RANDOM_TLB_ENTRIES 
        /*having a large working set, and randomly select S entires*/
        char *addr = buf + (seq[i] * 4096); 
#else 
        char *addr = buf + i * 4096; 
#endif 
        //addr += random() % PAGE_CACHELINES * L1_CACHELINE;   
        access(addr);
    }

}

#ifdef RANDOM_TLB_ENTRIES 
static inline void init_seq (uint32_t *seq, uint32_t s) {

    /*init the buffer with a sequence of size s*/
    for (int i = 0; i < s; i++) 
        seq[i] = i; 

}

static inline void random_seq (uint32_t *seq, uint32_t s) {
    uint32_t temp, p; 

    for (int i = 0; i < s; i++) {
        p = random() % (s - i ) + i;  
        temp = seq[p]; 
        seq[p] = seq[i]; 
        seq[i] = temp;
    }

}
#endif 

int tlb_trojan(bench_covert_t *env) {

  uint32_t secret;
  seL4_Word badge;
  seL4_MessageInfo_t info;
  char *buf = malloc ((ATTACK_PAGES  * 4096) + 4096);
  uint32_t *seq = (uint32_t*)malloc (ATTACK_PAGES  * sizeof (uint32_t));

  uint32_t cur, prev; 

  assert(buf); 
  assert(seq); 

  int temp = (int)buf; 
  temp &= ~0xfff;
  temp += 0x1000; 
  buf = (char *)temp; 

#ifdef RANDOM_TLB_ENTRIES 
  init_seq(seq, ATTACK_PAGES); 
#endif 
  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  /*receive the shared address to record the secret*/
  uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
  uint32_t volatile UNUSED *syn_vaddr = share_vaddr + 1;
  *share_vaddr = 0; 
  
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);
  

  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
  secret = 0; 

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

#ifdef RANDOM_TLB_ENTRIES 
      random_seq(seq, ATTACK_PAGES); 
#endif 
      FENCE(); 
      READ_COUNTER_ARMV7(cur);
      prev = cur;


      while (cur - prev < KERNEL_SCHEDULE_TICK_LENGTH) {
          prev = cur; 

          READ_COUNTER_ARMV7(cur); 
      }
      FENCE();
#ifndef CONFIG_BENCH_DATA_SEQUENTIAL 
      secret = random() % (TLB_ENTRIES + 1); 
#endif

      tlb_access(buf, seq, secret);

      /*update the secret read by low*/ 
      *share_vaddr = secret; 
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
      if (++secret == TLB_ENTRIES + 1)
          secret = 0; 
#endif 
  }


 FENCE(); 
 
  while (1);
 
  return 0;
}

int tlb_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint32_t start, after;
  uint32_t prev, cur; 
#ifdef CONFIG_MANAGER_PMU_COUNTER 
  uint32_t pmu_start[BENCH_PMU_COUNTERS]; 
  uint32_t pmu_end[BENCH_PMU_COUNTERS]; 
#endif 

  char *buf = malloc ((ATTACK_PAGES  * 4096) + 4096);
  assert(buf);

  uint32_t *seq = (uint32_t*)malloc (ATTACK_PAGES * sizeof (uint32_t));
  assert(seq); 

  int temp = (int)buf; 
  temp &= ~0xfff;
  temp += 0x1000; 
  buf = (char *)temp; 
 
#ifdef RANDOM_TLB_ENTRIES 
  init_seq(seq, ATTACK_PAGES); 
#endif 

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
  
  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
  /*the shared address*/
  uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);
  uint32_t volatile *syn = secret + 1;
  *syn = TROJAN_SYN_FLAG;

  /*syn with trojan*/
  info = seL4_Recv(env->syn_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


 

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
#ifdef RANDOM_TLB_ENTRIES 
      random_seq(seq, ATTACK_PAGES); 
#endif 
      FENCE(); 
      READ_COUNTER_ARMV7(cur);
      prev = cur;
       while (cur - prev < KERNEL_SCHEDULE_TICK_LENGTH) {
          prev = cur; 

            READ_COUNTER_ARMV7(cur); 
      }

      FENCE(); 
#ifdef CONFIG_MANAGER_PMU_COUNTER 
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_start);  
#endif 
      READ_COUNTER_ARMV7(start);
      //start = sel4bench_get_cycle_count();

      tlb_access(buf, seq, PROBE_PAGES);
      
      READ_COUNTER_ARMV7(after);
#ifdef CONFIG_MANAGER_PMU_COUNTER 
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_end);  
#endif 

      //after = sel4bench_get_cycle_count();
      r_addr->result[i] = after - start; 
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->sec[i] = *secret;

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      /*loading the pmu counter value */
      for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
          r_addr->pmu[i][counter] = pmu_end[counter] - pmu_start[counter]; 

#endif 
      /*spy set the flag*/
  }
 

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);

  return 0;
}

