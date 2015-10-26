#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __linux__
#define _GNU_SOURCE
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sched.h>
#endif

#include "affinity.h"


void setAffinity(int processor) {
#ifdef __linux__
  cpu_set_t cs;

  if (processor < 0 || processor > 31)
    return;
  CPU_ZERO(&cs);
  CPU_SET(processor, &cs);
  sched_setaffinity(0, sizeof(cs), &cs);
//  printf("%d: set affinity to %d\n", getpid(), processor);
#endif
}


