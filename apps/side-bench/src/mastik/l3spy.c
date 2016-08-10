#include <autoconf.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include "vlist.h"
#include "cachemap.h"
#include "pp.h"
#include "../mastik_common/low.h"
#include "../../../bench_common.h"


#define SIZE (16*1024*1024)

#define RECORDLEN 100000
#define SLOT 5000

#define SEARCHLEN 1000

#define TS_THRESHOLD 10000

static void  newTimeSlice() {
  asm("");
  uint32_t prev = rdtscp();
  for (;;) {
    uint32_t cur = rdtscp();
    if (cur - prev > TS_THRESHOLD)
      return;
    prev = cur;
  }
}


static int  newTimeSlice_tl(uint64_t end) {
  asm("");
  uint64_t prev = rdtscp_64();
  for (;;) {
    uint64_t cur = rdtscp_64();
    if (cur - prev > TS_THRESHOLD)
      return 1;
    if (cur > end)
      return 0;
    prev = cur;
  }
}




static volatile int a=0;
static cachemap_t map() {
  for (int i = 0; i < 1024*1024; i++)
    for (int j = 0; j < 1024; j++)
      a *=i+j;
  printf("%d\n", a);
  char *buf = (char *)mmap(NULL, SIZE + 4096 * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  if (buf == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  /*making the buffer page aligned*/
  uintptr_t buf_switch = (uintptr_t) buf; 
  buf_switch &= ~(0xfffULL); 
  buf_switch += 0x1000; 
  buf = (char *) buf_switch; 
  printf("buffer %p\n", buf);


  cachemap_t cm;
  vlist_t candidates;
  candidates = vl_new();
  for (int i = 0; i < SIZE; i += 4096){
    vl_push(candidates, buf + i);
  }
  cm = cm_linelist(candidates);
  printf("Cachemap done: %d sets, %d unloved\n", cm->nsets, vl_len(candidates));
  return cm;

}



#define TEST_COUNT 10
seL4_CPtr async_ep;
seL4_Word sender;

int buffer[100];

int __attribute__((noinline)) test(pp_t pp, int recv) {
  int count = 0;
  pp_probe(pp);
  pp_probe(pp);
  pp_probe(pp);
  for (int i = 0; i < TEST_COUNT; i++) {
    if (recv)
      seL4_Poll(async_ep, &sender);
      //for (int j = 0; j < 100; j++)
	//buffer[j]++;
    count <<= 1;
    if (pp_probe(pp) != 0)
      count++;
  }
  // If at most one is set - this is not a collision
  int c = count & (count - 1);
  c = c & (c - 1);
  if ((c & (c - 1))== 0)
    return 0;
  // If not all bits are set we have a problem
  //if ((count & (count + 1)) != 0) 
    //printf("Problem at  %p: probed %x out of %d times\n", pp, count, TEST_COUNT);
  return 1;
}


int scan(pp_t pp, int offset) {
  if (test(pp, 0))
    return 0;
  /*if do not have conflict with the test code, 
   try to detect the conflict with seL4 Poll*/
  return test(pp, 1);
}

vlist_t search(cachemap_t cm) {
  vlist_t probed = vl_new();
  for (int line = 0; line < 4096; line += 64) {
    //printf("Searching in line 0x%02x\n", line / 64);
    // For every page...
      /*preparing the probing set 
       find one that do not have conflict with the test code
       but have conflict with the seL4 Poll service*/
      for (int p  = 0; p < cm->nsets; p++) {
      pp_t pp = pp_prepare(cm->sets[p] , L3_ASSOCIATIVITY, line);
      int t = scan(pp, line);
      if (t) {
          /*record target probing set*/
          vl_push(probed, pp);
	printf("Probed at %d.%03x (%p)\n", p, line, pp);
      }
    }
  }
  return probed;
}

int results[100][100];

static void realspy(vlist_t probed) {
  int l = vl_len(probed);
  if (l > 100)
    l = 100;
  for (int i = 0; i < 100; i++) {
      /*try 100 time slices, each wait to probe until the new 
       time slice*/
    newTimeSlice();
    for (int j = 0; j < l; j++)
      results[i][j] = pp_probe((pp_t)vl_get(probed, j));
    /*
    i++;
    for (int j = 0; j < l; j++)
      results[i][j] = pp_bprobe((pp_t)vl_get(probed, j));
      */
  }

  printf("%d probing sets\n", l);
  int count = 1;
  for (int i = 0; i < 100; i++) {
      /*the slot number*/
      for (int j = 0; j < l; j++) 
          /*number of cache misses during the probe*/
          printf("%2d ", results[i][j]);
      putchar('\n');
     
  }
}


void realtrojan() {
  uint64_t end = rdtscp_64() + 5000000000ULL;

  for (int i = 0; i < 1000; i++) {
      /*wait for a new time slice*/
      if (!newTimeSlice_tl(end))
      return;
      /*poll kernel service for 5 time slices in 13 time slices*/
    if ((i % 13) < 5)
      seL4_Poll(async_ep, &sender);
  }
}


int l3_spy(bench_covert_t *env) {
  
  async_ep = env->notification_ep;
  seL4_CPtr ep = env->r_ep; 
  seL4_Word badge;
  seL4_MessageInfo_t info;

  info = seL4_Recv(ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  cachemap_t cm = map();
  printf("Test at %p, buffer at %p\n", test, buffer);
  vlist_t probed = search(cm);

  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(ep, info);

  info = seL4_Recv(ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  realspy(probed);

  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(ep, info);

  info = seL4_Recv(ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  realtrojan();

  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(ep, info);
  return 0;
}

int l3_trojan(bench_covert_t *env) {

    async_ep = env->notification_ep;
    seL4_CPtr ep = env->r_ep; 
    seL4_Word badge;
    seL4_MessageInfo_t info;

    info = seL4_Recv(ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    cachemap_t cm = map();
    printf("Test at %p, buffer at %p\n", test, buffer);
    vlist_t probed = search(cm);

    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(ep, info);

    info = seL4_Recv(ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    realtrojan();

    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(ep, info);

    info = seL4_Recv(ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    realspy(probed);

    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(ep, info);
    return 0;
}

