#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>


#define JMP_ALIGN   16
#define BTB_ENTRIES  (4096*2)


extern void btb_probe(void);



static void btb_jmp(uint32_t s) {


    /*calculate the jump entry based on the secret*/ 
    unsigned long entry = (unsigned long)btb_probe + (BTB_ENTRIES - s) * JMP_ALIGN; 

#ifdef CONFIG_ARCH_X86_64
    asm volatile ("callq %0" : : "r" (entry): );
#else 
    asm volatile ("call %0" : : "r" (entry): );

#endif 

}

int btb_trojan(bench_env_t *env) {

  uint32_t total_sec = 128 + 1, secret;
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

      /*waiting for a system tick*/
      newTimeSlice();

      /*trojan: 3584 - 3712*/
      secret = (random() % total_sec ) + 3584; 
      /*do the probe*/
      btb_jmp(secret);

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

      newTimeSlice();
      start = rdtscp(); 
      btb_jmp(4096);
       
      r_addr->result[i] = rdtscp() - start; 
      r_addr->sec[i] = *secret; 

  }

  /*send result to manager, spy is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(args->r_ep, info);

  while (1);


  return 0;
}
