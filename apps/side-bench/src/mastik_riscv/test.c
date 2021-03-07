#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#include "../mastik_common/vlist.h"
#include "cachemap.h"
#include "pp.h"
#include "../mastik_common/low.h"

#define SIZE (16*1024*1024)


struct cachemap *cm_linelist(vlist_t lines);

int a;

void probe100(pp_t pp) {
  pp_prime(pp, 50);
  char out[1001];
  uint32_t p = rdtime();
  for (int i = 0; i < 1000; i++) {
    out[i] = '0' + pp_probe(pp);
    do {
    } while (rdtime() - p < 5000);
    p = rdtime();
  }
  out[1000] = '\0';
  printf("%s\n", out);
}
/*requirement, a 16 M buffer for morecore area used as mmap*/

int mastik_test(int ac, char **av) {
  for (int i = 0; i < 1024*1024; i++)
    for (int j = 0; j < 1024; j++)
      a *=i+j;
  
  char *buf = (char *)mmap(NULL, (SIZE + 4096 * 2), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  if (buf == MAP_FAILED) {
    printf("mmap failed");
    assert(0);
  }
  /*making the buffer page aligned*/
  int buf_switch = (int) buf; 
  buf_switch &= ~(0xfff); 
  buf_switch += 0x1000; 
  buf = (char *) buf_switch; 
  printf("buffer %p\n", buf);

  cachemap_t cm;
  vlist_t candidates;
  candidates = vl_new();
  for (int i = 0; i < SIZE; i += 4096) 
    vl_push(candidates, buf + i);
  cm = cm_linelist(candidates);
  printf("%d sets, %d unloved\n", cm->nsets, vl_len(candidates));
  if (vl_len(candidates) != 0) 
    exit (1);

  pp_t *pps[256];
  for (int i = 0; i < cm->nsets; i++)
    pps[i] = pp_prepare(cm->sets[i], L3_ASSOCIATIVITY, 256);

  printf("Done preparing\n");

  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < cm->nsets; i++) {
      printf("%3d: ", i);
      probe100(pps[i]);
    }
  }

  return 0;
}



