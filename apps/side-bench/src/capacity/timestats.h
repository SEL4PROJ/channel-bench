#ifndef __TIMESTATS_H__
#define __TIMESTATS_H__ 1


ts_t ts_alloc(void *);

void ts_clear(ts_t ts);

// tm > 0
void ts_add(ts_t ts, int tm);

// tm > 0

uint32_t ts_outliers(ts_t ts);

int ts_median(ts_t ts);

int ts_mean(ts_t ts, int scale);

#endif // __TIMESTATS_H__
