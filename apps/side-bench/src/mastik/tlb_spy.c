#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "../../../bench_common.h"

#define WARMUP_ROUNDS 0x1000 

#ifdef CONFIG_ARCH_X86_SKYLAKE
#define SPY_TLB_PAGES 32
#define TROJAN_TLB_PAGES 64

#else
#define SPY_TLB_PAGES 32
#define TROJAN_TLB_PAGES 128
#endif


#define TS_THRESHOLD 100000



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


static int32_t *buf;

#define offset(i, spy) (((i) * 793) % 512 + ((spy) & (i)) ? 512 : 0)
static void allocbuf(int spy) {
  int pages = spy ? SPY_TLB_PAGES : TROJAN_TLB_PAGES;
  char *b = malloc(pages * 4096 + 4096);
  buf =  (int32_t *)(((uintptr_t)b + 4096) & ~0xfff);
  int *r = malloc(pages * sizeof(int));
  for (int i = 0; i < pages; i++)
    r[i] = i;
  for (int i = 1; i < pages; i++) {
    int t = r[i];
    int j = i + random() %(pages - i);
    r[i] = r[j];
    r[j] = t;
  }
  for (int i = 0; i < pages - 1; i++) {
    int from = 1024 * r[i] + offset(i, spy);
    buf[from] = 1024 * r[i+1] + offset(i+1, spy);
  }
  buf[1024 * r[pages-1] + offset(pages-1, spy)] = -1;
}

static volatile int a;
uint32_t tlb_probe(int secret) {
  uint32_t start = rdtscp();
  int i = 0;

  if (!secret)
      return rdtscp() - start; 
  do {
    i = buf[i];
  } while ((i!=0) && (--secret > 0));
  a = i;
  return rdtscp() - start;
}


int tlb_trojan(bench_covert_t *env) {


  uint32_t secret;
  seL4_Word badge;
  seL4_MessageInfo_t info;

  srandom(rdtscp());
  allocbuf(0);

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*receive the shared address to record the secret*/
  volatile uint32_t *share_vaddr = (uint32_t *)seL4_GetMR(0);
  *share_vaddr = SYSTEM_TICK_SYN_FLAG; 
  
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);

  /*warm up the platform*/
  for (int i = 0; i < WARMUP_ROUNDS; i++) {
      secret = random() % (TROJAN_TLB_PAGES+1);
      
      tlb_probe(secret);
  }
  /*ready to do the test*/
  seL4_Send(env->syn_ep, info);
  
  /*giving some number of system ticks to syn trojan and spy */

  for (int i = 0; i < 100; i++) {
      secret = random() % (TROJAN_TLB_PAGES+1);
      newTimeSlice();
      tlb_probe(secret);
      
  }

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      secret = (random() / 2) % (TROJAN_TLB_PAGES+1);
      
      /*update the secret read by low*/ 
      *share_vaddr = secret; 
      tlb_probe(secret);
      newTimeSlice();

  }
  while (1);
 
  return 0;
}

int tlb_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;

#ifdef CONFIG_x86_64
  uint64_t volatile pmu_start[BENCH_PMU_COUNTERS]; 
  uint64_t volatile pmu_end[BENCH_PMU_COUNTERS]; 
#else
  uint32_t volatile pmu_start[BENCH_PMU_COUNTERS]; 
  uint32_t volatile pmu_end[BENCH_PMU_COUNTERS]; 
#endif

  srandom(rdtscp());
  allocbuf(1);
  

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
  /*the shared address*/
  volatile uint32_t *secret = (uint32_t *)seL4_GetMR(1);

  /*syn with trojan*/
  info = seL4_Recv(env->syn_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*waiting for a start*/
  while (*secret == SYSTEM_TICK_SYN_FLAG) ;


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      newTimeSlice();
#ifdef CONFIG_MANAGER_PMU_COUNTER
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_start);  
#endif 
      //yval
      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = tlb_probe(SPY_TLB_PAGES);
      r_addr->sec[i] = *secret; 

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      sel4bench_get_counters(BENCH_PMU_BITS, pmu_end);
#endif 

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      /*loading the pmu counter value */
      for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
          r_addr->pmu[i][counter] = pmu_end[counter] - pmu_start[counter]; 

#endif

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  while (1);


  return 0;
}
