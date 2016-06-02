#ifndef __LOW_H__
#define __LOW_H__


#define L1_ASSOCIATIVITY 8
#define L1_SETS 64
#define L1_CACHELINE 64
#define L1_STRIDE (L1_CACHELINE * L1_SETS)

#define L1I_ASSOCIATIVITY 8
#define L1I_SETS 64
#define L1I_CACHELINE 64

#define L3_THRESHOLD 140
#define L3_ASSOCIATIVITY 16
#define L3_SIZE (6*1024*1024)
#ifndef PAGE_SIZE 
#define PAGE_SIZE 4096
#endif 

static inline int access(void *v) {
  int rv;
  asm volatile("mov (%1), %0": "=r" (rv): "r" (v):);
  return rv;
}

static inline int accesstime(void *v) {
  int rv = 0;
  asm volatile(
      "mfence\n"
      "lfence\n"
      "rdtscp\n"
      "mov %%eax, %%esi\n"
      "mov (%1), %%eax\n"
      "rdtscp\n"
      "sub %%esi, %%eax\n"
      : "+a" (rv): "r" (v): "ecx", "edx", "esi");
  return rv;
}

static inline void clflush(void *v) {
  asm volatile ("clflush 0(%0)": : "r" (v):);
}

static inline uint32_t rdtscp() {
  uint32_t rv;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}

static inline void mfence() {
  asm volatile("mfence");
}


#ifdef X86_64_UNDEF
static inline void walk(void *p, int count) {
  if (p == NULL)
    return;
  asm volatile(
    "movq %0, %%rsi\n"
    "1:\n"
    "movq (%0), %0\n"
    "cmpq %0, %%rsi\n"
    "jnz 1b\n"
    "decl %1\n"
    "jnz 1b\n"
    : "+r" (p), "+r" (count)::"rsi");
}
#else
static inline void walk(void *p, int count) {
  if (p == NULL)
    return;
  asm volatile(
    "movl %0, %%esi\n"
    "1:\n"
    "movl (%0), %0\n"
    "cmpl %0, %%esi\n"
    "jnz 1b\n"
    "decl %1\n"
    "jnz 1b\n"
    : "+r" (p), "+r" (count)::"esi");
}
#endif


struct cpuidRegs {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

#define CPUID_CACHEINFO		4


#define CACHETYPE_NULL          0
#define CACHETYPE_DATA          1
#define CACHETYPE_INSTRUCTION   2
#define CACHETYPE_UNIFIED       3

struct cacheInfo {
  uint32_t      type:5;
  uint32_t      level:3;
  uint32_t      selfInitializing:1;
  uint32_t      fullyAssociative:1;
  uint32_t      reserved1:4;
  uint32_t      logIds:12;
  uint32_t      phyIds:6;

  uint32_t      lineSize:12;
  uint32_t      partitions:10;
  uint32_t      associativity:10;

  uint32_t      sets:32;

  uint32_t      wbinvd:1;
  uint32_t      inclusive:1;
  uint32_t      complexIndex:1;
  uint32_t      reserved2:29;
};

inline void cpuid(struct cpuidRegs *regs) {
  asm volatile ("cpuid": "+a" (regs->eax), "+b" (regs->ebx), "+c" (regs->ecx), "+d" (regs->edx));
}


#endif //__LOW_H__
