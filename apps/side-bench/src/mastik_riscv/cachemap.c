#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "../mastik_common/vlist.h"
#include "cachemap.h"
#include "../mastik_common/timestats.h"
#include "../mastik_common/low.h"

#define CHECKTIMES 16


cachemap_t cm_pagelist(vlist_t pages);


#define PTR(t, i) (((void **)(t))[i])
#define LNEXT(t) (*(void **)(t))


static int timedwalk(void *list, register void *candidate) {
  if (list == NULL)
    return 0;
  if (LNEXT(list) == NULL)
    return 0;
  ts_t ts = ts_alloc(NULL);
  void *c2 = (void *)((uintptr_t)candidate ^ 0x200);
  LNEXT(c2) = candidate;
  clflush(c2);
  low_access(candidate);
  for (int i = 0; i < CHECKTIMES; i++) {
    walk(list, 20);
    void *p = LNEXT(c2);
    uint32_t time = accesstime(p);
    ts_add(ts, time);
  }
  int rv = ts_median(ts);
  ts_free(ts);
  return rv;
}

static int checkevict(vlist_t es, void *candidate) {
  if (vl_len(es) == 0)
    return 0;
  for (int i = 0; i < vl_len(es); i++) 
    LNEXT(vl_get(es, i)) = vl_get(es, (i + 1) % vl_len(es));
  int timecur = timedwalk(vl_get(es, 0), candidate);
  return timecur > L3_THRESHOLD;
}


static void *expand(vlist_t es, vlist_t candidates) {
  //assert(vl_len(candidates) + vl_len(es) > SKIP);
  //while (vl_len(es) <= SKIP)
    //vl_push(es, vl_poprand(candidates));
  while (vl_len(candidates) > 0) {
    void *current = vl_poprand(candidates);
    if (checkevict(es, current))
      return current;
    vl_push(es, current);
  }
  return NULL;
}

static void contract(vlist_t es, vlist_t candidates, void *current) {
  for (int i = 0; i < vl_len(es);) {
    void *cand = vl_get(es, i);
    vl_del(es, i);
    clflush(current);
    if (checkevict(es, current))
      vl_push(candidates, cand);
    else {
      vl_insert(es, i, cand);
      i++;
    }
  }
}

static void collect(vlist_t es, vlist_t candidates, vlist_t set) {
  for (int i = vl_len(candidates); i--; ) {
    void *p = vl_del(candidates, i);
    if (checkevict(es, p))
      vl_push(set, p);
    else
      vl_push(candidates, p);
  }
}


cachemap_t cm_linelist(vlist_t lines) {
#ifdef CONFIG_DEBUG_BUILD
  printf("%d lines\n", vl_len(lines));
#endif 
  cachemap_t cm = (cachemap_t)malloc(sizeof(struct cachemap));
  assert(cm != NULL); 
  vlist_t sets = vl_new();
  vlist_t es = vl_new();
  int fail = 0;
  while (vl_len(lines)) {
      assert(vl_len(es) == 0);
      if (fail > 5) {
          break;
      }
      void *c = expand(es, lines);
      if (c == NULL) {
          while (vl_len(es))
              vl_push(lines, vl_del(es, 0));
          fail++;
          continue;
      }
      contract(es, lines, c);
      contract(es, lines, c);
      contract(es, lines, c);
      if (vl_len(es) > L3_ASSOCIATIVITY || vl_len(es) < L3_ASSOCIATIVITY - 3) {
          while (vl_len(es))
              vl_push(lines, vl_del(es, 0));
          fail++;
          continue;
      } 
      fail = 0;
      vlist_t set = vl_new();
      vl_push(set, c);
      collect(es, lines, set);
      while (vl_len(es))
          vl_push(set, vl_del(es, 0));
#ifdef CONFIG_DEBUG_BUILD
     printf("Set %d - %d\n", vl_len(sets), vl_len(set));
#endif
      vl_push(sets, set);
  }

  cm->nsets = vl_len(sets);
  cm->sets = (vlist_t *)calloc(cm->nsets, sizeof(vlist_t));
  for (int i = 0; i < vl_len(sets); i++)
      cm->sets[i] = vl_get(sets, i);
  vl_free(sets);
  vl_free(es);
  return cm;
}

