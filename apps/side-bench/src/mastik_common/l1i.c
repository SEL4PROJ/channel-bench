#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include "../mastik_common/low.h"
#include "l1i.h"
#include "../ipc_test.h"
#ifdef CONFIG_ARCH_X86
#define JMP_OFFSET (PAGE_SIZE - 5)
#define JMP_OPCODE 0xE9
#define RET_OPCODE 0xC3
#define SET(page, set) (((uint8_t *)l1->memory) + PAGE_SIZE * (page) + L1I_CACHELINE * (set))
extern void nop_spy(void);
uint32_t l1i_probe_nop_time;
#endif 

#ifdef CONFIG_ARCH_ARM 
#define SET(way, set) (((uint8_t *)l1->memory) + L1I_STRIDE * (way) + L1I_CACHELINE * (set))
extern void arm_branch_lines(void); 
#endif 


#ifdef CONFIG_ARCH_X86
l1iinfo_t l1i_prepare(uint64_t *monitored_sets) {
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
#endif 


#ifdef CONFIG_ARCH_ARM  
l1iinfo_t l1i_prepare(uint64_t *monitored_sets) {
  
  /*prepare the probing buffer according to the bitmask in monitored_sets*/
  l1iinfo_t l1 = (l1iinfo_t)malloc(sizeof(struct l1iinfo));

  /*using existing code for probing*/
  l1->memory = arm_branch_lines; 

  l1i_set_monitored_set(l1, monitored_sets);
  return l1;
}

#endif

void l1i_set_monitored_set(l1iinfo_t l1, uint64_t *monitored_sets) {
  int nsets = 0;
  
  assert(monitored_sets != NULL);

  for (int i = 0; i < I_MONITOR_MASK; i++) {
      l1->monitored_sets[i] = monitored_sets[i];
        for (int j = 0; j < 64; j++) {
            if (monitored_sets[i] & (1ULL << j)) {
                l1->monitored[nsets++] = i * 64 + j;
            }
        }
  }
  l1->nsets = nsets;
  l1i_randomise(l1);
}

void l1i_randomise(l1iinfo_t l1) {

    /*randomise the monitoring order stroed in monitored*/
    for (int i = 0; i < l1->nsets; i++) {
        int p = random() % (l1->nsets - i) + i;
        uint8_t t = l1->monitored[p];
        l1->monitored[p] = l1->monitored[i];
        l1->monitored[i] = t;
    }
}

typedef void (*fptr)(void);
void l1i_probe_1(l1iinfo_t l1, uint16_t *results) {
    uint32_t start, res; 

    for (int i = 0; i < l1->nsets; i++) {
#ifdef CONFIG_ARCH_X86
        start = rdtscp();
        // Using assembly because I am not sure I can trust the compiler
        //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

        /*for the total number of monitored cache sets 
          do a probe, monitored contains the cache set number*/
        (*((fptr)SET(0, l1->monitored[i])))();
        res = rdtscp() - start;
#endif
#ifdef CONFIG_ARCH_ARM 
        READ_COUNTER_ARMV7(start); 
        fptr head = (fptr)SET(0, l1->monitored[i]);
        /*jump to the start of this set*/
        asm volatile ("blx %0" : : "r" (head) :"lr");

        READ_COUNTER_ARMV7(res);
        res -= start;
#endif 
        results[i] = res > UINT16_MAX ? UINT16_MAX : res;
    }
}
void l1i_probe(l1iinfo_t l1, uint16_t *results) {

    uint32_t start, res; 

    for (int i = 0; i < l1->nsets; i++) {
#ifdef CONFIG_ARCH_X86
        start = rdtscp();
        // Using assembly because I am not sure I can trust the compiler
        //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

        /*for the total number of monitored cache sets 
          do a probe, monitored contains the cache set number*/
        (*((fptr)SET(0, l1->monitored[i])))();
        res = rdtscp() - start;
#endif 
#ifdef CONFIG_ARCH_ARM 
        READ_COUNTER_ARMV7(start); 
        fptr head = (fptr)SET(0, l1->monitored[i]);
        /*jump to the start of this set*/
        asm volatile ("blx %0" : : "r" (head) :"lr");

        READ_COUNTER_ARMV7(res);
        res -= start;

#endif
        results[i] = res > UINT16_MAX ? UINT16_MAX : res;
    }
}

#ifdef CONFIG_ARCH_X86
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
#endif 
