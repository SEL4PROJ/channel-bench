#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vlist.h"

extern void *vl_get(vlist_t vl, int ind);
extern int vl_len(vlist_t vl);

#define VLIST_DEF_SIZE 16

vlist_t vl_new() {
  vlist_t vl = (vlist_t)malloc(sizeof(struct vlist));
  assert(vl != NULL);
 
  vl->size = VLIST_DEF_SIZE;
  vl->data = (void **)malloc(VLIST_DEF_SIZE * sizeof (void *));
 
  assert(vl->data != NULL);
  memset(vl->data, 0, VLIST_DEF_SIZE * sizeof (void *));
 
  vl->len = 0;
  return vl;
}

void vl_free(vlist_t vl) {
  assert(vl != NULL);
  free(vl->data);
  free(vl);
}

void vl_set(vlist_t vl, int ind, void *dat) {
  assert(vl != NULL);
  assert(dat != NULL);
  assert(ind < vl->len);
  vl->data[ind] = dat;
}

static void vl_setsize(vlist_t vl, int size) {
  assert(vl != NULL);
  assert(size >= vl->len);
  void **old = vl->data;
  vl->data = (void **)malloc(size * sizeof (void *)); 
  assert(vl->data != NULL);
  memcpy(vl->data, old, vl->size * sizeof (void *));
  free(old); 
  vl->size = size;
}


void vl_push(vlist_t vl, void *dat) {
  assert(vl != NULL);
  assert(dat != NULL);
  if (vl->len == vl->size)
    vl_setsize(vl, vl->size * 2);
  assert(vl->len < vl->size);
  vl->data[vl->len++] = dat;
}

void *vl_pop(vlist_t vl) {
  assert(vl != NULL);
  if (vl->len == 0)
    return NULL;
  return vl->data[--vl->len];
}

void *vl_del(vlist_t vl, int ind) {
  assert(vl != NULL);
  assert(ind < vl->len);
  void * rv = vl->data[ind];
  vl->data[ind] = vl->data[--vl->len];
  return rv;
}

void *vl_poprand(vlist_t vl) {
    assert(vl != NULL);
    if (vl->len == 0)
        return NULL;
    int ind = random() % vl->len;
    void *rv = vl->data[ind];
    vl_del(vl, ind);
    return rv;
}

void vl_insert(vlist_t vl, int ind, void *dat) {
  assert(vl != NULL);
  assert(dat != NULL);
  assert(ind <= vl->len);
  if (ind == vl->len) {
    vl_push(vl, dat);
  } else {
    vl_push(vl, vl->data[ind]);
    vl->data[ind] = dat;
  }
}

