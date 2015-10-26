#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>


#include "probe.h"
#include "timestats.h"
#include "sysinfo.h"
#include "affinity.h"
#include "trojan.h"

#define LINE 4
#define REPS 100


int debug = 0;



void init() {
  setAffinity(0);
  probe_init_simple(EBSIZE, 2048*64);
  tr_init(CACHESIZE);
}


int main(int c, char **v) {
  int slave = 0;
  char *ofile = NULL;
  FILE *f = stdout;
  int opt;

  while ((opt = getopt(c, v, "1o:")) != -1) {
    switch (opt) {
    case '1': slave = 1; break;
    case 'o': ofile = optarg; break;
    default:
      fprintf(stderr, "Usage: %s [-1] [-o <filename>]\n", v[0]);
      exit(1);
    }
  }
  if (ofile != NULL) {
    f = fopen(ofile, "w");
    if (f == NULL) {
      perror(ofile);
      exit(1);
    }
  }
  setbuf(stdout, NULL);
  init();

  //probe_printmap();
  if (slave)
    tr_startslave(LINE, 0);
  ts_t ts = ts_alloc();
  for (int mask =0; mask < 32; mask++) 
    for (int shift = 0; shift < 10; shift++) {
      printf("%d/%d\n", mask, shift);
      for (int i = 0; i < 1024; i++) {
	int size = i ^ mask <<5 ^ mask;
	size = (size >> shift) ^ (size << (10-shift));
	size &=0x3ff;
	ts_clear(ts);
	if (slave) {
	  probe_sitime(NULL, LINE, 10);
	  for (int k = 0; k < REPS; k++) {
	    tr_callslave(size);
	    probe_sitime(ts, LINE, 1);
	  }
	} else {
	  tr_start(size, LINE, 1);
	  probe_sitime(ts, LINE, REPS);
	  tr_stop();
	}
	for (int k = 1; k < TIME_MAX; k++)
	  for (int n = ts_get(ts, k); n--; ) 
	    fprintf(f, "%d, %d\n", size, k);
	// FOrce LRU eviction
	for (int j = 0; j < 2048; j+=64) {
	  probe_sitime(ts, (LINE^j)&0x3f, 100);
	  probe_sitime(ts, (LINE^j)&0x3f, 100);
	  probe_sitime(ts, (LINE^j)&0x3f, 100);
	}
      }
    }
  
  /*
  for (int i = 1; i < TIME_MAX; i++) {
    int t = ts_get(ts, i);
    if (t == 0) 
      continue;
    printf("%5d: %d\n", i, t);
  }
  */
}
