#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "low.h"
#include "l1i.h"
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



int l1i_trojan(bench_covert_t *env) {
  uint16_t results[64];

  l1iinfo_t l1i_1 = l1i_prepare(~0LLU);

  int i = 0;
  for (;;) {
    newTimeSlice();
    if (i & 4) 
      l1i_probe(l1i_1, results);
    i++;
  }
  return 0;
}

int l1i_spy(bench_covert_t *env) {
  seL4_Word badge;
  seL4_MessageInfo_t info;
  /*using reply ep talking to manger*/
  seL4_CPtr ep = env->r_ep;
  l1iinfo_t l1i_1 = l1i_prepare(~0LLU);

  uint16_t *results = malloc(100*l1i_nsets(l1i_1)*sizeof(uint16_t));
  uint32_t times[100];

  info = seL4_Recv(ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
  for (int i = 0; i < 100; i++) {
    newTimeSlice();
    l1i_probe(l1i_1, results+(i*l1i_nsets(l1i_1)));
    times[i] = rdtscp();
    //l1i_prime(l1i_1);
    //l1i_prime(l1i_1);
    //l1i_prime(l1i_1);
  }
  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(ep, info);


  info = seL4_Recv(ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
  printf("%d probe sets\n", l1i_nsets(l1i_1));
  for (int i = 1; i < 100; i++) {
   // printf("%3d: %10u %10u", i, times[i], times[i] - times[i-1]);
    for (int j = 0; j < l1i_nsets(l1i_1); j++)
      printf("%4d ", results[i*l1i_nsets(l1i_1) + j]);
    printf("\n");
  }

  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(ep, info);
  return 0;
}
