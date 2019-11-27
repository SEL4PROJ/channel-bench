#ifndef __VLIST_H__
#define __VLIST_H__

typedef struct vlist *vlist_t;

vlist_t vl_new();
void vl_free(vlist_t vl);

inline void *vl_get(vlist_t vl, int ind);
void vl_set(vlist_t vl, int ind, void *dat);
void vl_push(vlist_t vl, void *dat);
void *vl_pop(vlist_t vl);
void *vl_poprand(vlist_t vl);
void *vl_del(vlist_t vl, int ind);
inline int vl_len(vlist_t vl);
void vl_insert(vlist_t vl, int ind, void *dat);

//---------------------------------------------
// Implementation details
//---------------------------------------------

struct vlist {
  int size;
  int len;
  void **data;
};

inline void *vl_get(vlist_t vl, int ind) {
  assert(vl != NULL);
  assert(ind < vl->len);
  return vl->data[ind];
}

inline int vl_len(vlist_t vl) {
  assert(vl != NULL);
  return vl->len;
}



#endif // __VLIST_H__
