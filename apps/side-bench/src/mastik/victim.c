#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>

static inline uint32_t rdtsc() {
  uint32_t rv = 0;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}

static inline int access(void *v) {
  int rv = 0;
  asm volatile("mov (%1), %0": "+r" (rv): "r" (v):);
  return rv;
}

int mastik_victim(int ac, char **av) {
  uintptr_t p = (uintptr_t)mmap(0, 4096 * 3, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  p &= ~(0xfff); 
  p += 0x1000; 
  p += 256;
  uint32_t t = rdtsc();
  for (;;) {
    do {
      access((void*)p);
    } while (rdtsc() - t < 50000);
    t = rdtsc();
    do {
    } while (rdtsc() - t < 50000);
    t = rdtsc();
  }
}


