#ifndef __XMALLOC_H__
#define __XMALLOC_H__


static inline void *xmalloc(size_t size) {
  return malloc(size);
}

static inline void *xmalloc_secure(size_t size) {
  return malloc(size);
}

static inline void xfree(void *m) {
  free(m);
}

static inline void *xmalloc_clear(size_t size) {
  void *rv = malloc(size);
  memset(rv, 0, size);
  return rv;
}

static inline void *xrealloc(void *m, size_t size) {
  return realloc(m, size);
}

static inline int m_is_secure(void *p) {
  return 0;
}

static inline void g10_log_bug( const char *fmt, ... ) {
  exit(1);
}

#define m_check(n) 

#endif // __XMALLOC_H__
