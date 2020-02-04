#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include "low.h"
#include "l1i.h"

#ifdef CONFIG_ARCH_X86
#define JMP_OFFSET (PAGE_SIZE - 5)
#define JMP_OPCODE 0xE9
#define RET_OPCODE 0xC3
#define SET(page, set) (((uint8_t *)l1->memory) + PAGE_SIZE * (page) + L1I_CACHELINE * (set))
#endif 

#ifdef CONFIG_ARCH_ARM 
#define SET(way, set) (((uint8_t *)l1->memory) + L1I_STRIDE * (way) + L1I_CACHELINE * (set))
extern void arm_branch_sets(void); 
#endif 

#ifdef CONFIG_ARCH_RISCV
#define JMP_OFFSET PAGE_SIZE
#define JMP_OPCODE(offset) (((offset & 0x100000) << 11) | ((offset & 0x7FE) << 20) | ((offset & 0x800) << 9) | (offset & 0xFF000) | (0x6F))
#define RET_OPCODE 0x8067
#define SET(way, set) (((void *)l1->memory) + L1I_STRIDE * (way) + L1I_CACHELINE * (set))
#endif

#ifdef CONFIG_ARCH_X86
l1iinfo_t l1i_prepare(uint64_t *monitored_sets) {
  
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

void l1i_rewrite(l1iinfo_t l1i) {

    void *line_ptr; 
    unsigned long volatile content; 
    assert(l1i->memory); 

   /*rewrite the l1i probing buffer content, aligned by the
    the cache line size*/ 

    for (int i = 0; i < L1I_LINES; i++) {

        line_ptr = l1i->memory + i * L1I_CACHELINE; 
#ifdef CONFIG_ARCH_X86_64
        asm volatile("movq (%1), %0\n" 
                "movq %0, (%1) " : "=r" (content), "+r" (line_ptr): :);

#else 
        asm volatile("mov (%1), %0\n" 
                      "mov %0, (%1) " : "=r" (content), "+r" (line_ptr): :);
#endif 
    }
    mfence();

}

#endif 


#ifdef CONFIG_ARCH_ARM  
l1iinfo_t l1i_prepare(uint64_t *monitored_sets) {
  
  /*prepare the probing buffer according to the bitmask in monitored_sets*/
  l1iinfo_t l1 = (l1iinfo_t)malloc(sizeof(struct l1iinfo));

  /*using existing code for probing*/
  l1->memory = arm_branch_sets; 

  l1i_set_monitored_set(l1, monitored_sets);
  return l1;
}

void l1i_rewrite(l1iinfo_t l1i) {

    void *line_ptr; 
    unsigned long volatile content; 
    assert(l1i->memory); 

   /*rewrite the l1i probing buffer content, aligned by the
    the cache line size*/ 

    for (int i = 0; i < L1I_LINES; i++) {

        line_ptr = l1i->memory + i * L1I_CACHELINE;

        asm volatile("ldr %0, [%1]\n" 
                "str %0, [%1] " : "=r" (content), "+r" (line_ptr): :);
    }
    dmb();

}
#endif

#ifdef CONFIG_ARCH_RISCV
l1iinfo_t l1i_prepare(uint64_t *monitored_sets) {

  /*prepare the probing buffer according to the bitmask in monitored_sets*/
  l1iinfo_t l1 = (l1iinfo_t)malloc(sizeof(struct l1iinfo));
  l1->memory = mmap(0, PAGE_SIZE * (L1_ASSOCIATIVITY+1), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);
  l1->memory = (void *)((((uintptr_t)l1->memory) + 0xfff) & ~0xfff);
  assert((((uintptr_t)l1->memory) & 0xfff) == 0);

  for (int i = 0; i < L1I_SETS; i++) {
    for (int j = 0; j < L1I_ASSOCIATIVITY - 1; j++) {
      uint32_t *p = (uint32_t*)SET(j, i);
      *p = JMP_OPCODE(JMP_OFFSET);
    }
    *(uint32_t*)SET(L1I_ASSOCIATIVITY - 1, i) = RET_OPCODE;
  }
  l1i_set_monitored_set(l1, monitored_sets);
  return l1;
}

void l1i_rewrite(l1iinfo_t l1i) {

    void *line_ptr;
    unsigned long volatile content;
    assert(l1i->memory);

   /*rewrite the l1i probing buffer content, aligned by the
    the cache line size*/

    for (int i = 0; i < L1I_LINES; i++) {

        line_ptr = l1i->memory + i * L1I_CACHELINE;
        asm volatile("lw %0, (%1) \n"
                      "sw %0, (%1) " : "=r" (content), "+r" (line_ptr): :);
    }
    fence();

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

#ifdef CONFIG_ARCH_X86
void l1i_probe(l1iinfo_t l1, uint16_t *results) {

    uint32_t start, res; 

    for (int i = 0; i < l1->nsets; i++) {

        start = rdtscp();
        // Using assembly because I am not sure I can trust the compiler
        //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

        /*for the total number of monitored cache sets 
          do a probe, monitored contains the cache set number*/
        (*((fptr)SET(0, l1->monitored[i])))();
        res = rdtscp() - start;
        results[i] = res > UINT16_MAX ? UINT16_MAX : res;
    }
}


void l1i_prime(l1iinfo_t l1) {

    for (int i = 0; i < l1->nsets; i++) {

        // Using assembly because I am not sure I can trust the compiler
        //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

        /*for the total number of monitored cache sets 
          do a probe, monitored contains the cache set number*/
        (*((fptr)SET(0, l1->monitored[i])))();
    }
}

#endif /*CONFIG_ARCH_X86*/ 

#ifdef CONFIG_ARCH_ARM 
/*taking the time measurements at the caller side*/
void l1i_probe(l1iinfo_t l1, uint16_t *results) {

    for (int i = 0; i < l1->nsets; i++) {
        fptr head = (fptr)SET(0, l1->monitored[i]);
        /*jump to the start of this set*/
#ifdef CONFIG_ARCH_AARCH64
        asm volatile ("blr %0" : : "r" (head) :"x30");
#else
        asm volatile ("blx %0" : : "r" (head) :"lr");
#endif
    }
}

void l1i_prime(l1iinfo_t l1) {

    for (int i = 0; i < l1->nsets; i++) {
        fptr head = (fptr)SET(0, l1->monitored[i]);
        /*jump to the start of this set*/
#ifdef CONFIG_ARCH_AARCH64
        asm volatile ("blr %0" : : "r" (head) :"x30");
#else
        asm volatile ("blx %0" : : "r" (head) :"lr");
#endif
    }
}

#endif /*CONFIG_ARCH_ARM*/ 

#ifdef CONFIG_ARCH_RISCV
void l1i_probe(l1iinfo_t l1, uint16_t *results) {

    uint32_t start, res;

    for (int i = 0; i < l1->nsets; i++) {
        //printf("l1i_probe: i = %d\n", i);
        start = rdtime();
        // Using assembly because I am not sure I can trust the compiler
        //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

        /*for the total number of monitored cache sets 
          do a probe, monitored contains the cache set number*/
        //printf("BREAK NOW\n");
        //for (int i = 0; i < 1000000000; i++);
        asm volatile ("jalr ra, %0" : : "r" (SET(0, l1->monitored[i])));
        res = rdtime() - start;
        results[i] = res > UINT16_MAX ? UINT16_MAX : res;
    }
    //printf("l1i_probe: done\n");

}


void l1i_prime(l1iinfo_t l1) {

    for (int i = 0; i < l1->nsets; i++) {
        //printf("l1i_prime: i = %d\n", i);

        // Using assembly because I am not sure I can trust the compiler
        //asm volatile ("callq %0": : "r" (SET(0, l1->monitored[i])):);

        /*for the total number of monitored cache sets 
          do a prime, monitored contains the cache set number*/
        asm volatile ("jalr ra, %0" : : "r" (SET(0, l1->monitored[i])));
    }
    //printf("l1i_prime: done\n");
}

#endif /*CONFIG_ARCH_RISCV*/ 
