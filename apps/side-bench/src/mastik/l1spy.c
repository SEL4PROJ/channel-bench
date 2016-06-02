#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "low.h"
#include "l1.h"
#include "../../../bench_common.h"


#define TS_THRESHOLD 10000

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



int l1_trojan(bench_covert_t *env) {
  /*buffer size 32K L1 cache size */
  char *data = malloc(4096*8);
  int i = 0;
  for (;;) {
      /*waiting for a system tick*/
    newTimeSlice();
    /*sending signal in every 4 ticks: accessing the buffer*/
    if (i & 4) 
      for (int i = 0; i < 4096*8; i+=64) 
	access(data+i);
    i++;
  }
  return 0;
}

int l1_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  /*using the reply ep, talking to manager*/
  seL4_CPtr ep = env->r_ep;

  l1info_t l1_1 = l1_prepare(~0LLU);

  uint16_t *results = malloc(100*l1_nsets(l1_1)*sizeof(uint16_t));
  uint32_t times[100];

  info = seL4_Recv(ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
  /*probing for 100 ticks*/
  for (int i = 0; i < 100; i++) {
    newTimeSlice();
    l1_probe(l1_1, results+(i*l1_nsets(l1_1)));
    times[i] = rdtscp();
    //l1_prime(l1_1);
    //l1_prime(l1_1);
    //l1_prime(l1_1);
  }
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(ep, info);


  info = seL4_Recv(ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  printf("%d probing sets\n", l1_nsets(l1_1));
  /*print out the result*/
  for (int i = 1; i < 100; i++) {
      /*the variance of the probing time*/
    //printf("%3d: %10u %10u", i, times[i], times[i] - times[i-1]);
    for (int j = 0; j < l1_nsets(l1_1); j++)
        /*variance of that cache set*/
      printf("%4d ", results[i*l1_nsets(l1_1) + j]);
    printf("\n");
  }

  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(ep, info);
  return 0;
}
