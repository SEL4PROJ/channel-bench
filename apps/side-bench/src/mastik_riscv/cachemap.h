#ifndef __CACHEMAP_H__
#define __CACHEMAP_H__


struct cachemap {
  int nsets;
  vlist_t *sets;
};

typedef struct cachemap *cachemap_t;

struct cachemap *cm_pagelist(vlist_t pages);
struct cachemap *cm_linelist(vlist_t lines);


#endif // __CACHEMAP_H__
