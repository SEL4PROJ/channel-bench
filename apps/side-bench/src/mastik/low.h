#ifndef __LOW_H__
#define __LOW_H__


#define L3_THRESHOLD 140
#define L3_ASSOCIATIVITY 16

static inline int access(void *v) {
  int rv = 0;
  asm volatile("mov (%1), %0": "+r" (rv): "r" (v):);
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

static inline uint64_t rdtscp_64(void) {
    uint32_t low, high;

    asm volatile ( 
            "rdtscp          \n"
            "movl %%edx, %0 \n"
            "movl %%eax, %1 \n"
            : "=r" (high), "=r" (low)
            :
            : "eax", "ecx", "edx");

    return ((uint64_t) high) << 32llu | (uint64_t) low;
}

static inline void mfence() {
  asm volatile("mfence");
}


static inline void walk(void *p, int count) {
  if (p == NULL)
    return;
  asm volatile(
    "mov %0, %%esi\n"
    ".L1:\n"
    "mov (%0), %0\n"
    "cmp %0, %%esi\n"
    "jnz .L1\n"
    "decl %1\n"
    "jnz .L1\n"
    : "+r" (p), "+r" (count)::"esi");
}

#endif //__LOW_H__
