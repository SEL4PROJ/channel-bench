#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>

#include "../../../covert.h"
#include "timestats.h"

ts_t ts_alloc(void *vaddr) {
  ts_t rv = (ts_t)vaddr;
  ts_clear(rv);
  return rv;
}

void ts_clear(ts_t ts) {
  bzero(ts, sizeof(struct ts));
}

/*data is indexed by the time tick value*/
void ts_add(ts_t ts, int tm) {
  assert(tm > 0);
  if (tm < TIME_MAX)
    ts->data[tm]++;
  else
    ts->data[0]++;
}

uint32_t ts_outliers(ts_t ts) {
  return ts->data[0];
}


int ts_median(ts_t ts) {
  int c = 0;
  for (int i = 0; i < TIME_MAX; i++)
    c += ts->data[i];
  c = (c + 1) / 2;
  for (int i = 1; i < TIME_MAX; i++)
    if ((c -= ts->data[i]) < 0)
      return i;
  return 0;
}

int ts_mean(ts_t ts, int scale) {
  uint64_t sum = 0;
  int count = 0;
  for (int i = 0; i < TIME_MAX; i++) {
    count += ts->data[i];
    sum += i* (uint64_t)ts->data[i];
  }
  return (int)((sum * scale)/count);
}
