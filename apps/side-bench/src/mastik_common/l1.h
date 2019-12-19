#ifndef __L1_H__
#define __L1_H__ 1

typedef struct l1info *l1info_t;

l1info_t l1_prepare(uint64_t *monitored_sets);
void l1_set_monitored_set(l1info_t l1, uint64_t *monitored_sets);
void l1_randomise(l1info_t l1);
int l1_probe(l1info_t l1, uint16_t *results);
int l1_probe_clflush(l1info_t l1, uint16_t *results);
void l1_bprobe(l1info_t l1, uint16_t *results);

void *l1_prime(l1info_t l1);


//---------------------------------------------
// Implementation details
//---------------------------------------------

#include <assert.h>
#include "low.h"
/*using bitmask 64 bit wide to record the monitored sets*/
#define MONITOR_MASK (L1_SETS / 64)

struct l1info{
  void *memory;
  void *fwdlist;
  void *bkwlist;
  uint64_t monitored_sets[MONITOR_MASK];
  uint16_t monitored[L1_SETS];
  int nsets;
};

inline uint64_t *l1_get_monitored_set(l1info_t l1) {
  return l1->monitored_sets;
}

static inline int l1_nsets(l1info_t l1) {
  return l1->nsets;
}

inline int l1_monitored(l1info_t l1, int ind) {
  assert(ind >= 0 && ind < l1->nsets);
  return l1->monitored[ind];
}

#endif // __L1_H__

