#ifndef __LOW_H__
#define __LOW_H__

#include <autoconf.h>
#include <sel4bench/sel4bench.h>
#include <channel-bench/bench_helper.h>
#include <channel-bench/bench_types.h>
#include "vlist.h"

#ifdef CONFIG_ARCH_ARM 
#include "../mastik_arm/l3_arm.h"
#endif 

#if defined CONFIG_ARCH_X86 || defined CONFIG_ARCH_RISCV
#include "../mastik/cachemap.h"
#endif 

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
#define L1I_LINES          512

/*L2 cache feature*/
#define L2_ASSOCIATIVITY   8
#define L2_SETS            512
#define L2_CACHELINE       64
#define L2_LINES           4096
#define L2_STRIDE          (L2_CACHELINE * L2_SETS)
#define L2_PROBE_BUFFER    (PAGE_SIZE + (L2_STRIDE * L2_ASSOCIATIVITY))  


#if defined (CONFIG_BENCH_COVERT_L2) || defined (CONFIG_BENCH_CACHE_FLUSH_L2_CACHES)
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

#ifdef CONFIG_BENCH_COVERT_L2_KERNEL_SCHEDULE 

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

#endif /*CONFIG_BENCH_COVERT_L2_KERNEL_SCHEDULE*/



#define L3_THRESHOLD       140
#define L3_ASSOCIATIVITY   16
#define L3_SETS            8192
#define L3_SIZE            (8*1024*1024)

#define L3_SETS_PER_PAGE   64


#endif /* CONFIG_ARCH_X86 */

#ifdef CONFIG_PLAT_TX1

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

#ifdef CONFIG_MANAGER_MITIGATION 
#define L3_PROBE_GROUPS    8
#else 
#define L3_PROBE_GROUPS    16 
#endif 


#endif /* CONFIG_PLAT_HIKEY */

#ifdef CONFIG_PLAT_SABRE 

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

#define L3_THRESHOLD       110
#define L3_ASSOCIATIVITY   16
#define L3_SIZE            (1*1024*1024)
#define L3_CACHELINE       32
// The number of cache sets in each slice.
#define L3_SETS_PER_SLICE  2048

// The number of cache sets in each page
#define L3_SETS_PER_PAGE   128

/*the total probing group = groups * sets per page*/
/*there are total 16 colours on L3 cache*/

#if defined (CONFIG_MANAGER_MITIGATION) || defined(CONFIG_BENCH_COVERT_LLC_KERNEL) 
#define L3_PROBE_GROUPS    8
#else 
#define L3_PROBE_GROUPS    16 
#endif 
#define BTAC_ENTRIES      512 

/*the tlb attack probs on half of the TLB entries */
#define TLB_ENTRIES       128 
#define TLB_PROBE_PAGES    64

#endif /* CONFIG_PLAT_SABRE  */

#ifdef CONFIG_PLAT_ARIANE

#define L1_ASSOCIATIVITY   8
#define L1_SETS            256
#define L1_LINES           2048
#define L1_CACHELINE       16
#define L1_STRIDE          (L1_CACHELINE * L1_SETS)
#define L1_PROBE_BUFFER    (L1_STRIDE * L1_ASSOCIATIVITY + PAGE_SIZE)

#define L1I_ASSOCIATIVITY  4
#define L1I_SETS           256
#define L1I_LINES          1024
#define L1I_CACHELINE      16
#define L1I_STRIDE         (L1I_CACHELINE * L1I_SETS)

#define L3_THRESHOLD       100
#define L3_ASSOCIATIVITY   8
#define L3_SIZE            (512 * 1024) /* 512KiB */
#define L3_CACHELINE       64
// The number of cache sets in each slice.
#define L3_SETS_PER_SLICE  64

// The number of cache sets in each page
#define L3_SETS_PER_PAGE   1

#define BTAC_ENTRIES        32
#define BHT_ENTRIES         64
#define TLB_ENTRIES         16
#define TLB_PROBE_PAGES     8

#endif /* CONFIG_PLAT_ARIANE */

typedef void *pp_t;

static void inline do_timing_api(enum timing_api api_no, 
        kernel_timing_caps_t *caps) {
    
    seL4_Word sender;                          
    switch (api_no) {
        case timing_signal: 
            seL4_Signal(caps->async_ep);
            break; 
        case timing_tcb: 
            seL4_TCB_SetPriority(caps->fake_tcb, caps->self_tcb, 0);
            break; 
        case timing_poll: 

            seL4_Poll(caps->async_ep, &sender);
            break;

        case timing_api_num: 
        default: 
            break;
    }
}


#ifdef CONFIG_ARCH_ARM 

#define INSTRUCTION_LENGTH  4

#define LLC_KERNEL_TEST_COUNT 16
static inline int low_access(void *v) {
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
    asm volatile("dmb sy" : : :);
}

static volatile int a;

static inline int memaccess(void *v) {
    a +=  *(int *)v;
    return *(int *)v;
}   

static inline int memaccesstime(void *v) {
    uint32_t start, end ; 

    SEL4BENCH_READ_CCNT(start);  
 
    int rv; 
    asm volatile("");
    rv = *(int *)v;
    asm volatile("");
    a+= rv;
    SEL4BENCH_READ_CCNT(end);  
 
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

static inline void  newTimeTick(void){
  
  uint32_t volatile  prev, cur; 
  SEL4BENCH_READ_CCNT(prev);  
  for (;;) {
      SEL4BENCH_READ_CCNT(cur);  
    if (cur - prev > 300)
      return;
    prev = cur;
  }
}


static int __attribute__((noinline)) test(void *pp, 
        int recv, enum timing_api api_no, 
        kernel_timing_caps_t *caps) {
  
    int count = 0;
   
    probecount(pp);
    probecount(pp);
    probecount(pp);

    for (int i = 0; i < LLC_KERNEL_TEST_COUNT; i++) {
        
        if (recv){
            do_timing_api(api_no, caps);
            do_timing_api(api_no, caps);
            do_timing_api(api_no, caps);

        }
        
        count <<= 1;
        if (probecount(pp) != 0)
            count++;
    }
    // If at most one is set - this is not a collision
    int c = count & (count - 1);
    c = c & (c - 1);
    if ((c & (c - 1))== 0)
        return 0;
    
    return 1;
}


static int scan(pp_t pp,  enum timing_api api_no, 
                kernel_timing_caps_t *caps) {
    if (test(pp, 0, api_no, caps))
        return 0;
    /*if do not have conflict with the test code, 
      try to detect the conflict with seL4 Poll*/
    return test(pp, 1, api_no, caps);
}



static inline vlist_t search(l3pp_t l3,  enum timing_api api_no, 
            kernel_timing_caps_t *caps) {

    vlist_t probed = vl_new();

    int nsets = l3_getSets(l3);

    for (int set = 0; set < nsets; set++) {
        //printf("Searching in line 0x%02x\n", set);
        // For every page...
        /*preparing the probing set 
          find one that do not have conflict with the test code
          but have conflict with the seL4 Poll service*/

        void *pp = l3_set_probe_head(l3, set);
        assert(pp); 

        int t = scan(pp, api_no, caps);
        if (t) {
            /*record target probing set*/
            vl_push(probed, pp);

#ifdef CONFIG_DEBUG_BUILD
            printf("search at %x (%08p)\n", set, pp);
#endif
     
        }


  }
  return probed;
}


static inline uint32_t probing_sets(vlist_t set_list) {

    uint32_t result = 0; 

    for (int j = 0; j < vl_len(set_list); j++){
       //result += probecount((pp_t)vl_get(set_list, j));
        result += probetime(vl_get(set_list, j));
    }

    return result; 
}


#endif /* CONFIG_ARCH_ARM  */


#ifdef CONFIG_ARCH_X86

#define LLC_KERNEL_TEST_COUNT 10
static inline int low_access(void *v) {
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

/*return when a big jump of the time stamp counter is detected*/
static inline void  newTimeTick(void){
  asm("");
  uint32_t volatile  prev = rdtscp();
  for (;;) {
    uint32_t volatile cur = rdtscp();
    if (cur - prev > 300)
      return;
    prev = cur;
  }
}

extern int pp_probe(pp_t pp);
extern pp_t pp_prepare(vlist_t list, int count, int offset);

static int __attribute__((noinline)) test(pp_t pp, int recv, enum timing_api api_no, 
        kernel_timing_caps_t *caps) {
  
    int count = 0;

    pp_probe(pp);
    pp_probe(pp);
    pp_probe(pp);


    for (int i = 0; i < LLC_KERNEL_TEST_COUNT; i++) {
        
        if (recv){
            do_timing_api(api_no, caps);
        }
        
        count <<= 1;
        if (pp_probe(pp) != 0)
            count++;
    }
    // If at most one is set - this is not a collision
    int c = count & (count - 1);
    c = c & (c - 1);
    if ((c & (c - 1))== 0)
        return 0;
    
    return 1;
}

static int scan(pp_t pp,  enum timing_api api_no, 
                kernel_timing_caps_t *caps) {
    if (test(pp, 0, api_no, caps))
        return 0;
    /*if do not have conflict with the test code, 
      try to detect the conflict with seL4 Poll*/
    return test(pp, 1, api_no, caps);
}




static inline vlist_t search(cachemap_t cm, enum timing_api api_no, 
            kernel_timing_caps_t *caps) {


    vlist_t probed = vl_new();
    for (int line = 0; line < 4096; line += 64) {
        //printf("Searching in line 0x%02x\n", line / 64);
        // For every page...
        /*preparing the probing set 
          find one that do not have conflict with the test code
          but have conflict with the seL4 Poll service*/
        for (int p  = 0; p < cm->nsets; p++) {
            pp_t pp = pp_prepare(cm->sets[p] , L3_ASSOCIATIVITY, line);
            int t = scan(pp, api_no, caps);
            if (t) {
                /*record target probing set*/
                vl_push(probed, pp);

#ifdef CONFIG_DEBUG_BUILD
                printf("Probed at %3d.%03x (%08p)\n", p, line, pp);
#endif

            }
        }
  }
  return probed;
}



static inline uint32_t probing_sets(vlist_t set_list) {

    uint32_t result = 0; 

    for (int j = 0; j < vl_len(set_list); j++){
       result += pp_probe((pp_t)vl_get(set_list, j));
    }

    return result; 

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

#ifdef CONFIG_ARCH_RISCV

#define LLC_KERNEL_TEST_COUNT 10
static inline int low_access(void *v) {
    int rv = 0xff; 
#ifdef CONFIG_BENCH_L1D_WRITE
    asm volatile("sw %1, 0(%0)": "+r" (v): "r" (rv):);
#else
    asm volatile("lw %0, 0(%1)": "=r" (rv): "r" (v):);
#endif 
    return rv;
}


static inline int accesstime(void *v) {
  int rv = 0;
  asm volatile(
      "fence\n"
      "rdcycle t0\n"
      "lw t0, 0(%1)\n"
      "rdcycle a0\n"
      "sub a0, a0, t0\n"
      : "=r" (rv): "r" (v): );
  return rv;
}

static inline void clflush(void *v) {
//  asm volatile ("clflush 0(%0)": : "r" (v):);
//  asm volatile("clflush %[v]" : [v] "+m"(*(volatile char *)v));
}

static inline void fence() {
  asm volatile("fence": : :);
}

/*return when a big jump of the time stamp counter is detected*/
static inline void  newTimeSlice(void){
  asm("");
  uint32_t best = 0;
  uint32_t volatile  prev = rdtime();
  for (;;) {
    uint32_t volatile cur = rdtime();
    /*if (cur - prev > best) {
      best = cur - prev;
      //printf("best = %d\n", best);
    }*/
    if (cur - prev > TS_THRESHOLD)
      return;
    prev = cur;
  }
}

/*return when a big jump of the time stamp counter is detected*/
static inline void  newTimeTick(void){
  asm("");
  uint32_t volatile  prev = rdtime();
  for (;;) {
    uint32_t volatile cur = rdtime();
    if (cur - prev > 100)
      return;
    prev = cur;
  }
}

extern int pp_probe(pp_t pp);
extern pp_t pp_prepare(vlist_t list, int count, int offset);

static int __attribute__((noinline)) test(pp_t pp, int recv, enum timing_api api_no, 
        kernel_timing_caps_t *caps) {

    int count = 0;

    pp_probe(pp);
    pp_probe(pp);
    pp_probe(pp);


    for (int i = 0; i < LLC_KERNEL_TEST_COUNT; i++) {
        
        if (recv){
            do_timing_api(api_no, caps);
        }
        
        count <<= 1;
        if (pp_probe(pp) != 0)
            count++;
    }
    // If at most one is set - this is not a collision
    int c = count & (count - 1);
    c = c & (c - 1);
    if ((c & (c - 1))== 0)
        return 0;
    
    return 1;
}

static int scan(pp_t pp,  enum timing_api api_no, 
                kernel_timing_caps_t *caps) {
    if (test(pp, 0, api_no, caps))
        return 0;
    /*if do not have conflict with the test code, 
      try to detect the conflict with seL4 Poll*/
    return test(pp, 1, api_no, caps);
}




static inline vlist_t search(cachemap_t cm, enum timing_api api_no, 
            kernel_timing_caps_t *caps) {


    vlist_t probed = vl_new();
    for (int line = 0; line < 4096; line += 64) {
        //printf("Searching in line 0x%02x\n", line / 64);
        // For every page...
        /*preparing the probing set 
          find one that do not have conflict with the test code
          but have conflict with the seL4 Poll service*/
        for (int p  = 0; p < cm->nsets; p++) {
            pp_t pp = pp_prepare(cm->sets[p] , L3_ASSOCIATIVITY, line);
            int t = scan(pp, api_no, caps);
            if (t) {
                /*record target probing set*/
                vl_push(probed, pp);

#ifdef CONFIG_DEBUG_BUILD
                printf("Probed at %3d.%03x (%08p)\n", p, line, pp);
#endif

            }
        }
  }
  return probed;
}



static inline uint32_t probing_sets(vlist_t set_list) {

    uint32_t result = 0; 

    for (int j = 0; j < vl_len(set_list); j++){
       result += pp_probe((pp_t)vl_get(set_list, j));
    }

    return result; 

}

static volatile int b;

static inline void walk(void *p, int count) {
    if (p == NULL)
        return;
    void *pp;
    for (int i = 0; i < count; i++) {
        pp = p;
        do {
            pp = (void *)(((void **)pp)[0]);
        } while (pp != p);
        b += *(int *)pp;
    }

}

#endif /* CONFIG_ARCH_RISCV */

/*accessing N number of L1 D cache sets*/
static inline void l1d_data_access(char *buf, uint32_t sets) {

    /*sets == 0 return*/

    for (int s = 0; s < sets; s++) {
        for (int i = 0; i < L1_ASSOCIATIVITY; i++) {

            low_access(buf + s * L1_CACHELINE + i * L1_STRIDE);
        }
    }
}



/*remove any data from p_b if p_a has the same*/
static inline void remove_same_probesets(vlist_t p_a, vlist_t p_b) {

    for (int a = 0; a < vl_len(p_a); a++) {

        for (int b = 0; b < vl_len(p_b); b++) {

            if (vl_get(p_a, a) == vl_get(p_b, b)) 
                vl_del(p_b, b);
        }
    }
}

#endif /*_LOW_H_*/
