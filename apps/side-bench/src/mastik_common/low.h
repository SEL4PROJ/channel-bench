#ifndef __LOW_H__
#define __LOW_H__

#include <sel4bench/sel4bench.h>
#include "../ipc_test.h"

#ifndef PAGE_SIZE 
#define PAGE_SIZE 4096
#endif 

#define ALIGN_PAGE_SIZE(_addr) (((uintptr_t)(_addr) + 0xfff) & ~0xfff) 


/*the threshold for detecting a time tick*/
#define TS_THRESHOLD 100000

#define X_4(a) a a a a 
#define X_64(a) X_4(X_4(X_4(a)))

#define WARMUP_ROUNDS 0x1000 

/*the cache architecture configuration on benchmarking platforms*/
#ifdef CONFIG_ARCH_X86

#define L1_ASSOCIATIVITY   8
#define L1_SETS            64
#define L1_CACHELINE       64
#define L1_LINES           512
#define L1_STRIDE          (L1_CACHELINE * L1_SETS)
#define L1_PROBE_BUFFER    (PAGE_SIZE * (L1_ASSOCIATIVITY+1))          //BUFFER is 32KB after 4K alignment

#define L1I_ASSOCIATIVITY  8
#define L1I_SETS           64
#define L1I_CACHELINE      64

/*L2 cache feature*/
#define L2_ASSOCIATIVITY   8
#define L2_SETS            512
#define L2_CACHELINE       64
#define L2_LINES           4096
#define L2_STRIDE          (L2_CACHELINE * L2_SETS)
#define L2_PROBE_BUFFER    (PAGE_SIZE + (L2_STRIDE * L2_ASSOCIATIVITY))  


#ifdef CONFIG_BENCH_COVERT_L2 
/*the L2 probing function shares with L1*/
#undef L1_ASSOCIATIVITY   
#define L1_ASSOCIATIVITY   L2_ASSOCIATIVITY

#undef L1_SETS            
#define L1_SETS            L2_SETS

#undef L1_CACHELINE       
#define L1_CACHELINE       L2_CACHELINE 

#undef  L1_LINES           
#define L1_LINES           L2_LINES 

#undef L1_STRIDE   
#define L1_STRIDE           L2_STRIDE 

#undef L1_PROBE_BUFFER
#define L1_PROBE_BUFFER    L2_PROBE_BUFFER

#endif /*CONFIG_BENCH_COVERT_L2*/


#define L3_THRESHOLD       140
#define L3_ASSOCIATIVITY   16
#define L3_SIZE            (8*1024*1024)

#endif /* CONFIG_ARCH_X86 */

#ifdef CONFIG_PLAT_TX1
/*number of cache lines per page*/
#define PAGE_CACHELINES    64

/*32 K buffer*/
#define L1_ASSOCIATIVITY   2
#define L1_SETS            256
#define L1_LINES           512
#define L1_CACHELINE       64
#define L1_STRIDE          (L1_CACHELINE * L1_SETS)
#define L1_PROBE_BUFFER    (L1_STRIDE * L1_ASSOCIATIVITY + PAGE_SIZE)

#define L1I_ASSOCIATIVITY  3
#define L1I_SETS           256
#define L1I_LINES          768
#define L1I_CACHELINE      64
#define L1I_STRIDE         (L1I_CACHELINE * L1I_SETS)

#define L3_THRESHOLD       150
#define L3_ASSOCIATIVITY   16
#define L3_SIZE            (2*1024*1024) /* 2MB */
#define L3_CACHELINE       64
// The number of cache sets in each slice.
#define L3_SETS_PER_SLICE  2048

// The number of cache sets in each page
#define L3_SETS_PER_PAGE   64

#define BTAC_ENTRIES        256 
#define TLB_ENTRIES         1024
#define TLB_PROBE_PAGES     512

#endif /* CONFIG_PLAT_TX1 */

#ifdef CONFIG_PLAT_HIKEY
/*number of cache lines per page*/
#define PAGE_CACHELINES    64

/*32 K buffer*/
#define L1_ASSOCIATIVITY   4
#define L1_SETS            128
#define L1_LINES           512
#define L1_CACHELINE       64
#define L1_STRIDE          (L1_CACHELINE * L1_SETS)
#define L1_PROBE_BUFFER    (L1_STRIDE * L1_ASSOCIATIVITY + PAGE_SIZE)

#define L1I_ASSOCIATIVITY  2
#define L1I_SETS           256
#define L1I_LINES          512
#define L1I_CACHELINE      64
#define L1I_STRIDE         (L1I_CACHELINE * L1I_SETS)

#define L3_THRESHOLD       150
#define L3_ASSOCIATIVITY   16
#define L3_SIZE            (512*1024) /* 512KB */
#define L3_CACHELINE       64
// The number of cache sets in each slice.
#define L3_SETS_PER_SLICE  512

// The number of cache sets in each page
#define L3_SETS_PER_PAGE   64

#define BTAC_ENTRIES       256 
#define TLB_ENTRIES        512
#define TLB_PROBE_PAGES    256

#endif /* CONFIG_PLAT_HIKEY */

#ifdef CONFIG_PLAT_SABRE 
/*number of cache lines per page*/
#define PAGE_CACHELINES    128

#define L1_ASSOCIATIVITY   4
#define L1_SETS            256
#define L1_LINES           1024
#define L1_CACHELINE       32
#define L1_STRIDE          (L1_CACHELINE * L1_SETS)
#define L1_PROBE_BUFFER    (L1_STRIDE * L1_ASSOCIATIVITY + PAGE_SIZE)

#define L1I_ASSOCIATIVITY  4
#define L1I_SETS           256
#define L1I_LINES          1024
#define L1I_CACHELINE      32
#define L1I_STRIDE         (L1I_CACHELINE * L1I_SETS)

#define L3_THRESHOLD       150
#define L3_ASSOCIATIVITY   16
#define L3_SIZE            (1*1024*1024)
#define L3_CACHELINE       32
// The number of cache sets in each slice.
#define L3_SETS_PER_SLICE  2048

// The number of cache sets in each page
#define L3_SETS_PER_PAGE   128

/*the total probing group = groups * sets per page*/
/*there are total 16 colours on L3 cache*/

#ifdef CONFIG_MANAGER_MITIGATION 
#define L3_PROBE_GROUPS    8
#else 
#define L3_PROBE_GROUPS    16 
#endif 

#define BTAC_ENTRIES      512 

/*the tlb attack probs on half of the TLB entries */
#define TLB_ENTRIES       128 
#define TLB_PROBE_PAGES    64

#endif /* CONFIG_PLAT_SABRE  */


#ifdef CONFIG_ARCH_ARM 

#define INSTRUCTION_LENGTH  4

static inline int access(void *v) {
    int rv = 0xff; 
#ifdef CONFIG_BENCH_L1D_WRITE
    asm volatile("str %1, [%0]": "+r" (v): "r" (rv):);
#else
    asm volatile("ldr %0, [%1]": "=r" (rv): "r" (v):);
#endif 
    return rv;
}
static inline void clflush(void *v) {

}


static inline void dmb(void) {
    /*memory barrier*/
    asm volatile("dmb" : : :);
}

static volatile int a;

static inline int memaccess(void *v) {
    a +=  *(int *)v;
    return *(int *)v;
}   

static inline int memaccesstime(void *v) {
    uint32_t start, end ; 
    READ_COUNTER_ARMV7(start);
 
    int rv; 
    asm volatile("");
    rv = *(int *)v;
    asm volatile("");
    a+= rv;
    READ_COUNTER_ARMV7(end);
 
    return end - start; 
} 

static inline void walk(void *p, int count) {
    if (p == NULL)
        return;
    void *pp;
    for (int i = 0; i < count; i++) {
        pp = p;
        do {
            pp = (void *)(((void **)pp)[0]);
        } while (pp != p);
        a += *(int *)pp;
    }

}

static inline void  newTimeSlice(void){
  
  uint32_t volatile  prev, cur; 
  SEL4BENCH_READ_CCNT(prev);  
  for (;;) {
      SEL4BENCH_READ_CCNT(cur);  
    if (cur - prev > TS_THRESHOLD)
      return;
    prev = cur;
  }
}

#endif /* CONFIG_ARCH_ARM  */


#ifdef CONFIG_ARCH_X86

static inline int access(void *v) {
  int rv = 0xff;
#ifdef CONFIG_BENCH_L1D_WRITE
  asm volatile("mov %1, (%0)": "+r" (v): "r" (rv): "memory");
#else 
  asm volatile("mov (%1), %0": "+r" (rv): "r" (v): "memory");
#endif
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
//  asm volatile ("clflush 0(%0)": : "r" (v):);
    asm volatile("clflush %[v]" : [v] "+m"(*(volatile char *)v));
}

static inline uint32_t rdtscp() {
  uint32_t rv;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}
static inline void mfence() {
  asm volatile("mfence": : :);
}

/*return when a big jump of the time stamp counter is detected*/
static inline void  newTimeSlice(void){
  asm("");
  uint32_t volatile  prev = rdtscp();
  for (;;) {
    uint32_t volatile cur = rdtscp();
    if (cur - prev > TS_THRESHOLD)
      return;
    prev = cur;
  }
}
#ifdef CONFIG_ARCH_X86_64
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
#else /* CONFIG_ARCH_X86_64 */
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
#endif /* CONFIG_ARCH_X86_64 */


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
#endif /* CONFIG_ARCH_X86 */

/*accessing N number of L1 D cache sets*/
static inline void l1d_data_access(char *buf, uint32_t sets) {

    /*sets == 0 return*/

    for (int s = 0; s < sets; s++) {
        for (int i = 0; i < L1_ASSOCIATIVITY; i++) {

            access(buf + s * L1_CACHELINE + i * L1_STRIDE);
        }
    }
}


#endif /*_LOW_H_*/
