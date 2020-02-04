#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "../mastik_common/vlist.h"
#include "pp.h"
#include "../mastik_common/low.h"

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

int pp_probe(pp_t pp) {
  if (pp == NULL)
    return 0;
  int rv = 0;
  void *p = (void *)pp;
  do {
    uint32_t s = rdtime();
    p = LNEXT(p);
    s = rdtime() - s;
    if (s > L3_THRESHOLD)
      rv++;
  } while (p != (void *) pp);
  return rv;
}

