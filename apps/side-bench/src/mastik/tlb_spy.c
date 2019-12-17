#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>


#define SPY_TLB_PAGES 32
#define TROJAN_TLB_PAGES 64


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
  /*size of int = 4, a page contains 1024 int */
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


int tlb_trojan(bench_env_t *env) {


  uint32_t secret;
  seL4_MessageInfo_t info;
  bench_args_t *args = env->args; 

  allocbuf(0);

  volatile uint32_t *share_vaddr = (uint32_t *)args->shared_vaddr;
  
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(args->r_ep, info);

 /*ready to do the test*/
  seL4_Send(args->ep, info);

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      newTimeSlice();
      secret = random() % (TROJAN_TLB_PAGES+1);

      tlb_probe(secret);
      /*update the secret read by low*/ 
      *share_vaddr = secret; 

  }
  while (1);
 
  return 0;
}

int tlb_spy(bench_env_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  bench_args_t *args = env->args; 

  uint64_t volatile UNUSED pmu_start; 
  uint64_t volatile UNUSED pmu_end; 


  allocbuf(1);
  
  struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr;
  /*the shared address*/
  volatile uint32_t *secret = (uint32_t*)args->shared_vaddr;

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

      newTimeSlice();
#ifdef CONFIG_MANAGER_PMU_COUNTER
      pmu_start = sel4bench_get_counter(0);  
#endif 

      /*result is the total probing cost
        secret is updated by trojan in the previous system tick*/
      r_addr->result[i] = tlb_probe(SPY_TLB_PAGES);
      r_addr->sec[i] = *secret; 

#ifdef CONFIG_MANAGER_PMU_COUNTER 
      /*loading the pmu counter value */
      pmu_end = sel4bench_get_counter(0);  
      r_addr->pmu[i][0] = pmu_end - pmu_start; 

#endif
  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);


  return 0;
}
