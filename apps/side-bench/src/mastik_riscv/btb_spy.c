#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>


// same function, once forwards and once backwards
extern void btb_probe(void);
extern void btb_prime(void);

int btb_jmp(bool spy, uint32_t s) {

    unsigned long entry;
    unsigned long increment;
    unsigned long result;
    if (spy)
    {
      /* start from the first indirect jump and jump forwards */
      entry = (unsigned long)btb_probe;
      increment = (signed long)CONFIG_BENCH_BTB_ALIGN;
    }
    else
    {
      /* calculate the jump entry based on the secret and jump backwards */
      entry = (unsigned long)btb_prime - (CONFIG_BENCH_BTB_ENTRIES - s + 1) * CONFIG_BENCH_BTB_ALIGN;
      increment = -(signed long)CONFIG_BENCH_BTB_ALIGN;
    }

    volatile long ra;
    asm volatile (
      "sd ra, %0\n"
      "mv a0, %2\n"
      "mv a1, %3\n"
      "rdcycle t0\n"
      "jalr ra, a0\n"
      "rdcycle %1\n"
      "sub %1, %1, t0\n"
      "lw ra, %0\n"
      : "+m"(ra), "=r"(result) : "r"(entry), "r"(increment));

    return((int)result);
}

int btb_trojan(bench_env_t *env) {

  uint32_t secret;
  seL4_MessageInfo_t info;
  bench_args_t *args = env->args;

  /*receive the shared address to record the secret*/
  volatile uint32_t *share_vaddr = (uint32_t *)args->shared_vaddr; 

  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(args->r_ep, info);

  /*ready to do the test*/
  seL4_Send(args->ep, info);

  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
      if (i % 1000 == 0 || (i - 1) % 1000 == 0 || i == (CONFIG_BENCH_DATA_POINTS - 1)) printf("TROJAN: Data point %d\n", i);

      /*waiting for a system tick*/
      newTimeSlice();

      /*trojan: 0 - BTB_ENTRIES*/
      secret = random() % (CONFIG_BENCH_BTB_ENTRIES + 1); 

      /*do the probe*/
      btb_jmp(0, secret);

      /*update the secret read by low*/ 
      *share_vaddr = secret; 
  }
  while (1);

  return 0;
}


int btb_spy(bench_env_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint32_t start; 
  bench_args_t *args = env->args; 

  /*the record address*/
  struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
  /*the shared address*/
  volatile uint32_t *secret = (uint32_t*)args->shared_vaddr; 

  /*syn with trojan*/
  info = seL4_Recv(args->ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


  for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
    if (i % 1000 == 0 || (i - 1) % 1000 == 0 || i == (CONFIG_BENCH_DATA_POINTS - 1)) printf("SPY: Data point %d\n", i);

      newTimeSlice();

      r_addr->result[i] = btb_jmp(1, CONFIG_BENCH_BTB_ENTRIES);
      r_addr->sec[i] = *secret; 

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);


  return 0;
}
