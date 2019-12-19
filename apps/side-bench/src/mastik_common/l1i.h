#ifndef __L1I_H__
#define __L1I_H__ 1

typedef struct l1iinfo *l1iinfo_t;
l1iinfo_t l1i_prepare(uint64_t *monitored_sets);
void l1i_set_monitored_set(l1iinfo_t l1, uint64_t *monitored_sets);
void l1i_randomise(l1iinfo_t l1);
void l1i_probe(l1iinfo_t l1, uint16_t *results);
void l1i_rewrite(l1iinfo_t l1);
void l1i_prime(l1iinfo_t l1);

//---------------------------------------------
// Implementation details
//---------------------------------------------

#include <assert.h>
#include "low.h"

/*using bitmask 64 bit wide to record the monitored sets*/
#define I_MONITOR_MASK (L1I_SETS / 64)


struct l1iinfo{
  void *memory;
  uint64_t monitored_sets[I_MONITOR_MASK];
  uint8_t monitored[L1I_SETS];
  void *sets[L1I_SETS];
  int nsets;
};

inline uint64_t *l1i_get_monitored_set(l1iinfo_t l1) {
  return l1->monitored_sets;
}

static inline int l1i_nsets(l1iinfo_t l1) {
  return l1->nsets;
}

inline int l1i_monitored(l1iinfo_t l1, int ind) {
  assert(ind >= 0 && ind < l1->nsets);
  return l1->monitored[ind];
}

#endif // __L1I_H__

