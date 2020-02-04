#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include "low.h"
#include "l1.h"


#define PTR(set, way, ptr) (void *)(((uintptr_t)l1->memory) + ((set) * L1_CACHELINE) + ((way) * L1_STRIDE) + ((ptr)*sizeof(void *)))
#define LNEXT(p) (*(void **)(p))

static int probelist(void *pp, int segments, int seglen, uint16_t *results) {
  void *p = pp;
  uint32_t s, res; 
  while (segments--) {
#ifdef CONFIG_ARCH_ARM
      SEL4BENCH_READ_CCNT(s); 
#endif 
#ifdef CONFIG_ARCH_X86
      s = rdtscp();
#endif 
#ifdef CONFIG_ARCH_RISCV
      s = rdtime();
#endif
      for (int i = seglen; i--; ) {
          // Under normal circumstances, p is never NULL. 
          // We need this test to ensure the optimiser does not kill the whole loop...
          if (p == NULL)
              break;
#ifdef CONFIG_BENCH_COVERT_L1D_WRITE
        *(uint32_t*) ((uint32_t*)p + 4) = 0xff;
#endif    
          p = LNEXT(p);
      }
#ifdef CONFIG_ARCH_ARM 
      SEL4BENCH_READ_CCNT(res); 
      res -= s; 
#endif 
#ifdef CONFIG_ARCH_X86
    res = rdtscp() - s;
#endif
#ifdef CONFIG_ARCH_RISCV
    res = rdtime() - s;
#endif
    *results = res > UINT16_MAX ? UINT16_MAX : res;
    results++;
  }
  return p == pp;
}

// This could be improved because we do not need dependent access.
static void *primelist(void *p, int segments, int seglen) {
  while (segments--) {
    for (int i = seglen; i--; ) {
      // Under normal circumstances, p is never NULL. 
      // We need this test to ensure the optimiser does not kill the whole loop...
      if (p == NULL)
	break;
      p = LNEXT(p);
    }
  }
  return p;
}

l1info_t l1_prepare(uint64_t *monitored_sets) {
  l1info_t l1 = (l1info_t)malloc(sizeof(struct l1info));
  l1->memory = mmap(0, L1_PROBE_BUFFER, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  l1->memory = (void *)((((uintptr_t)l1->memory) + 0xfff) & ~0xfff);

  assert((((uintptr_t)l1->memory) & 0xfff) == 0);
  l1->fwdlist = NULL;
  l1->bkwlist = NULL;
  for (int set = 0; set < L1_SETS; set++) {
    for (int way = 0; way < L1_ASSOCIATIVITY - 1; way++) {
      LNEXT(PTR(set, way, 0)) = PTR(set, way+1, 0);
      LNEXT(PTR(set, way+1, 1)) = PTR(set, way, 1);
    }
  }
  l1_set_monitored_set(l1, monitored_sets);
  return l1;
}

void l1_set_monitored_set(l1info_t l1, uint64_t *monitored_sets) {
  
  int nsets = 0;
 
  assert(monitored_sets != NULL);

  for (int i = 0; i < MONITOR_MASK; i++) {

        l1->monitored_sets[i] = monitored_sets[i];
  
        for (int j = 0; j < 64; j++) {
            if (monitored_sets[i] & (1ULL << j)) {
                l1->monitored[nsets++] = i * 64 + j;

            }
        }
  }

  l1->nsets = nsets;
  l1_randomise(l1);
}

void l1_randomise(l1info_t l1) {

  for (int i = 0; i < l1->nsets; i++) {
    int p = random() % (l1->nsets - i) + i;
    uint16_t t = l1->monitored[p];
    l1->monitored[p] = l1->monitored[i];
    l1->monitored[i] = t;
  }

  for (int i = 0; i < l1->nsets - 1; i++) {
    LNEXT(PTR(l1->monitored[i], L1_ASSOCIATIVITY - 1, 0)) = PTR(l1->monitored[i+1], 0, 0);
    LNEXT(PTR(l1->monitored[i], 0, 1)) = PTR(l1->monitored[i+1], L1_ASSOCIATIVITY - 1, 1);
  }
  l1->fwdlist = LNEXT(PTR(l1->monitored[l1->nsets - 1], L1_ASSOCIATIVITY - 1, 0)) = PTR(l1->monitored[0], 0, 0);
  l1->bkwlist = LNEXT(PTR(l1->monitored[l1->nsets - 1], 0, 1)) = PTR(l1->monitored[0], L1_ASSOCIATIVITY - 1, 1);

}

int l1_probe(l1info_t l1, uint16_t *results) {
  return probelist(l1->fwdlist, l1->nsets, L1_ASSOCIATIVITY, results);
}



void l1_bprobe(l1info_t l1, uint16_t *results) {
  probelist(l1->bkwlist, l1->nsets, L1_ASSOCIATIVITY, results);
}


void *l1_prime(l1info_t l1) {
  return primelist(l1->fwdlist, l1->nsets, L1_ASSOCIATIVITY);
}

