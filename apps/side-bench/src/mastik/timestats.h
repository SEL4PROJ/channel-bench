/*
 * Copyright 2015 The University of Adelaide
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TIMESTATS_H__
#define __TIMESTATS_H__ 1

#define TIME_MAX	1024

typedef struct ts *ts_t;

ts_t ts_alloc();
void ts_free(ts_t ts);

void ts_clear(ts_t ts);

// tm > 0
void ts_add(ts_t ts, int tm);

// tm > 0
uint32_t ts_get(ts_t ts, int tm);

uint32_t ts_outliers(ts_t ts);

int ts_median(ts_t ts);

int ts_mean(ts_t ts, int scale);

#endif // __TIMESTATS_H__
