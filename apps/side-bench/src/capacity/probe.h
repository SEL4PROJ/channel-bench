#ifndef __PROBE_H__
#define __PROBE_H__ 1

#include "timestats.h"

void probe_clflush(void *p);
void probe_access(void *p);
int probe_setindex(void *p);
void probe_evict(int si);
int probe_time(void *p);
void probe_init(uint64_t ebsize);
void probe_init_simple(uint64_t ebsize, int stride);

int probe_sitime(ts_t ts, int si, int count);

// Hardware and config info
int probe_npages();
int probe_noffsets();
int probe_nways();
int probe_ncores();
int probe_pagesize();

// debug
void probe_printmap();
#endif // __PROBE_H__
