#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "vlist.h"
#include "pp.h"
#include "low.h"

#define LNEXT(p) (*(void **)(p))
#define OFFSET(p, o) ((void *)((uintptr_t)(p) + (o)))
#define NEXTPTR(p) (OFFSET((p), sizeof(void *)))


pp_t pp_prepare(vlist_t list, int count, int offset) {
  assert(list != NULL);
  //assert(count == 0 || vl_len(list) >= count);
  assert((offset & 0x3f) == 0);

  if (count == 0 || vl_len(list)<count)
    count = vl_len(list);
  for (int i = 0; i < count; i++)
    LNEXT(OFFSET(vl_get(list, i), offset)) = OFFSET(vl_get(list, (i + 1) % count), offset);

  return (pp_t)OFFSET(vl_get(list, 0), offset);
}

void pp_prime(pp_t pp, int reps) {
  walk((void *)pp, reps);
}

#define str(x) #x
#define xstr(x) str(x)
/*probing on the probe list, return the total cost in time
 used by cache flush benchmark*/
uint32_t pp_probe_flush(pp_t pp) {

    if (pp == NULL)
        return 0;
    void *p = (void *)pp;
    uint32_t s, e; 
    
    s = rdtscp();
    do {
  
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
        *(uint32_t*) ((uint32_t*)p + 4 ) = 0xff;
#endif 
        p = LNEXT(p);
        asm volatile("" ::"r"(p):"memory");
   } while (p != (void *) pp);
    
    e = rdtscp(); 

    return e - s; 
}


int pp_probe(pp_t pp) {
  if (pp == NULL)
    return 0;
  int rv = 0;
  void *p = (void *)pp;
  do {
    uint32_t s = rdtscp();
    p = LNEXT(p);
    s = rdtscp() - s;
    if (s > L3_THRESHOLD)
      rv++;
  } while (p != (void *) pp);
  return rv;
  /*
  asm volatile(
    "xorl %0, %0\n"
    "movq %1, %%rsi\n"
    "rdtscp\n"
    "movl %%eax, %%ebx\n"
    "L2:\n"
    "movq (%1), %1\n"
    "rdtscp\n"
    "movl %%eax, %%edi\n"
    "subl %%eax, %%ebx\n"
    "cmpl $" xstr(L3_THRESHOLD) ", %%eax\n"
    "jg L3\n"
    "incl %0\n"
    "L3:\n"
    "movl %%edi, %%ebx\n"
    "cmpq %1, %%rsi\n"
    "jne  L2\n"
    : "=r" (rv), "+r" (pp) : : "rsi", "eax", "ebx", "ecx", "edx", "edi");
    */
}

