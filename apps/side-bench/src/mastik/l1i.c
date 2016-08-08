#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>

#include "low.h"
#include "l1i.h"

#define JMP_OFFSET (PAGE_SIZE - 5)
#define JMP_OPCODE 0xE9
#define RET_OPCODE 0xC3

#define SET(page, set) (((uint8_t *)l1->memory) + PAGE_SIZE * (page) + L1I_CACHELINE * (set))
extern void nop_spy(void);

uint32_t l1i_probe_nop_time;
l1iinfo_t l1i_prepare(uint64_t monitored_sets) {
  static uint8_t jmp[] = { JMP_OPCODE, 
    				JMP_OFFSET & 0xff, 
				(JMP_OFFSET >>8) & 0xff, 
    				(JMP_OFFSET >> 16) & 0xff, 
				(JMP_OFFSET >> 24) & 0xff};
  
  /*prepare the probing buffer according to the bitmask in monitored_sets*/
  l1iinfo_t l1 = (l1iinfo_t)malloc(sizeof(struct l1iinfo));
  l1->memory = mmap(0, PAGE_SIZE * (L1_ASSOCIATIVITY+1), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);
  l1->memory = (void *)((((uintptr_t)l1->memory) + 0xfff) & ~0xfff);
  assert((((uintptr_t)l1->memory) & 0xfff) == 0);

  for (int i = 0; i < L1I_SETS; i++) {
    for (int j = 0; j < L1I_ASSOCIATIVITY - 1; j++) {
      uint8_t *p = SET(j, i);
      *p++ = JMP_OPCODE;
      *p++ = JMP_OFFSET & 0xff;
      *p++ = (JMP_OFFSET >>8) & 0xff;
      *p++ = (JMP_OFFSET >> 16) & 0xff;
      *p++ = (JMP_OFFSET >> 24) & 0xff;
    }
    *SET(L1I_ASSOCIATIVITY - 1, i) = RET_OPCODE;
  }
  l1i_set_monitored_set(l1, monitored_sets);
  return l1;
}

void l1i_set_monitored_set(l1iinfo_t l1, uint64_t monitored_sets) {
  l1->monitored_sets = monitored_sets;
  int nsets = 0;
  for (int i = 0; i < 64; i++) {
    if (monitored_sets & (1ULL << i))
      l1->monitored[nsets++] = i;
  }
  l1->nsets = nsets;
  l1i_randomise(l1);
}

void l1i_randomise(l1iinfo_t l1) {

    /*randomise the monitoring order stroed in monitored*/
    char *mem = (char *)l1->memory;
  for (int i = 0; i < l1->nsets; i++) {
    int p = random() % (l1->nsets - i) + i;
    uint8_t t = l1->monitored[p];
    l1->monitored[p] = l1->monitored[i];
    l1->monitored[i] = t;
  }
}

typedef void (*fptr)(void);
void l1i_probe_1(l1iinfo_t l1, uint16_t *results) {
  for (int i = 0; i < l1->nsets; i++) {
    uint32_t start = rdtscp();
    // Using assembly because I am not sure I can trust the compiler
    //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

    /*for the total number of monitored cache sets 
     do a probe, monitored contains the cache set number*/
    (*((fptr)SET(0, l1->monitored[i])))();
    uint32_t res = rdtscp() - start;
    results[i] = res > UINT16_MAX ? UINT16_MAX : res;
  }
}
void l1i_probe(l1iinfo_t l1, uint16_t *results) {
  for (int i = 0; i < l1->nsets; i++) {
    uint32_t start = rdtscp();
    // Using assembly because I am not sure I can trust the compiler
    //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

    /*for the total number of monitored cache sets 
     do a probe, monitored contains the cache set number*/
    (*((fptr)SET(0, l1->monitored[i])))();
    uint32_t res = rdtscp() - start;
    results[i] = res > UINT16_MAX ? UINT16_MAX : res;
  }
}

uint32_t l1i_probe_nop(void) {
//    uint32_t start = rdtscp();
    // Using assembly because I am not sure I can trust the compiler
    //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

    /*for the total number of monitored cache sets 
     do a probe, monitored contains the cache set number*/
    asm volatile ("call nop_spy;"::: "eax", "ebx", "ecx", "edx");
    return l1i_probe_nop_time;
   //return rdtscp() - start;
}
