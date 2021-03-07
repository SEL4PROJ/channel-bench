#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>
#include "../mastik_common/low.h"


int mastik_victim(int ac, char **av) {
  uintptr_t p = (uintptr_t)mmap(0, 4096 * 3, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  p &= ~(0xfff); 
  p += 0x1000; 
  p += 256;
  uint32_t t = rdtime();
  for (;;) {
    do {
      low_access((void*)p);
    } while (rdtime() - t < 50000);
    t = rdtime();
    do {
    } while (rdtime() - t < 50000);
    t = rdtime();
  }
}


