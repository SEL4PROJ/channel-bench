
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


#ifdef __APPLE__
#include <mach/vm_statistics.h>
#undef PAGE_SIZE
#endif

#include "pageset.h"
#include "timestats.h"
#include "sysinfo.h"

#ifdef VM_FLAGS_SUPERPAGE_SIZE_ANY
#define MAP_LARGEPAGES	VM_FLAGS_SUPERPAGE_SIZE_ANY
#define MAP_ROUNDSIZE	(2*1024*1024)
#define SETINDEX_BITS	17
#endif

#ifdef MAP_HUGETLB
#define MAP_LARGEPAGES	MAP_HUGETLB
#define MAP_ROUNDSIZE	(2*1024*1024)
#define SETINDEX_BITS	17
#endif

#define PAGE_BITS	12
#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(PAGE_SIZE - 1)
#define PAGE_LBITS	(PAGE_BITS - CLBITS)
#define PAGE_LINES	(PAGE_SIZE >> CLBITS)

#ifndef SETINDEX_BITS
#error Expected large pages
#define MAP_LARGEPAGES	0
#define MAP_ROUNDSIZE	4096
#define SETINDEX_BITS	12
#endif



#define CLBITS	6
#define CLSIZE (1 << CLBITS)


#define SETINDEX_SIZE	(1 << SETINDEX_BITS)
#define SETINDEX_MASK	(SETINDEX_SIZE - 1)
#define SETINDEX_LBITS	(SETINDEX_BITS - CLBITS)
#define SETINDEX_LINES	(SETINDEX_SIZE >> CLBITS)


// Number of probes to find the set index of a virtual address
#define SETINDEX_NPROBE	64

#define MAX_SLICES 32

#define L3THRESHOLD 120
#define L3REPEATS 32
#define SPLIT_MEASURE_EVICT_COUNT 32


static int debug = 0;

#define SETINDEXLINK 0

union cacheline {
  union cacheline *next;
  union cacheline *cl_links[CLSIZE/8];
  char bytes[CLSIZE];
#define cl_next cl_links[0]
#define cl_ebnext cl_links[1]
#define cl_sinext cl_links[SETINDEXLINK]
};

union setindex {
  union setindex *next;
  union cacheline cachelines[SETINDEX_LINES];
  char bytes[SETINDEX_SIZE];
};

typedef union cacheline *cacheline_t;
typedef union setindex *setindex_t;

typedef char setindexmap[SETINDEX_LINES];

/*receving msg, probing buf*/
static struct probeinfo {
  uint64_t ebsetindices;
  setindex_t eb;
  pageset_t *map;
  cacheline_t indexlist[SETINDEX_LINES];
} probeinfo;

void probe_clflush(volatile void *p) {
  asm __volatile__ ("clflush 0(%0)" : : "r" (p):);
}

void probe_access(volatile void *p) {
  asm __volatile__ ("mov (%0), %%ebx" : : "r" (p) : "%ebx");
}

static void walk(cacheline_t cl, int ind) {
  while (cl != NULL) 
    cl = cl->cl_links[ind];
}


void probe_evict(int si) {
  walk (&probeinfo.eb[0].cachelines[si], 0);
}

int probe_npages() {
  return probeinfo.ebsetindices;
}

int probe_noffsets() {
  return SETINDEX_LINES;
}

int probe_nways() {
  return NWAYS;
}

int probe_ncores() {
  return NCORES;
}

int probe_pagesize() {
  return SETINDEX_SIZE;
}



int probe_time(volatile void *p) {
  volatile int rv;
  asm __volatile__ (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      "mov %%eax, %%esi\n"
      "mov (%%rdi), %%rdi\n"
      "rdtscp\n"
      "sub %%eax, %%esi\n"
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "xorl %%eax, %%eax\n"
      "subl %%esi, %%eax\n"
      : "=a" (rv) : "D" (p) : "%rbx", "%rcx", "%rdx", "%rsi");
  return rv;
}

/*probling a list then return time*/
int __attribute__ ((noinline)) probe_silist(volatile void *p) {
  volatile int rv;
  asm __volatile__ (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      "mov %%eax, %%esi\n"
      "1:\n"
      "mov (%%rdi), %%rdi\n"
      "cmpq $0, %%rdi\n"
      "jne 1b\n"
      "rdtscp\n"
      "sub %%eax, %%esi\n"
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "xorl %%eax, %%eax\n"
      "subl %%esi, %%eax\n"
      : "=a" (rv) : "D" (p) : "%rbx", "%rcx", "%rdx", "%rsi");
  return rv;
}

uint32_t rdtsc() {
  volatile uint32_t rv;
  asm __volatile__ ("rdtsc": "=a" (rv) : : "%rcx");
  return rv;
}

void probe_wait(int time) {
  uint32_t start = rdtsc();
  while (rdtsc() - start < time)
    ;
}

int probe_sitime(ts_t ts, int si, int count) {
 
    /*probing and record time, the first 10 rounds is warming up*/
  for (int i = count <= 1 ? 0 : -10; i < count; i++)  {
    int t = probe_silist(probeinfo.indexlist[si]);
    if (ts != NULL && i >= 0)  {
      ts_add(ts, t);
    }
  }
  return 0;
}

int probe_setindex(void *p) {
  int page = ((uint32_t)(intptr_t)p & PAGE_MASK) / PAGE_LINES;
  if (PAGE_SIZE == SETINDEX_SIZE)
    return page;
  ts_t ts = ts_alloc();

  int max = 0;
  int maxsi = -1;
  for (int si = page; si < SETINDEX_LINES; si+= PAGE_LINES) {
    for (int i = 0; i < SETINDEX_NPROBE; i++) {
      probe_access(p);
      probe_evict(si);
      ts_add(ts, probe_time(p));
    }
    int median = ts_median(ts);
    if (median > max) {
      max = median;
      maxsi = si;
    }
    ts_clear(ts);
  }
  ts_free(ts);
  return maxsi;
}

static pageset_t ebpageset() {
  pageset_t rv = ps_new();
  for (int i = 0; i < probeinfo.ebsetindices; i++) 
    ps_push(rv, i);
  ps_randomise(rv);
  return rv;
}

static void build_probelines() {
  pageset_t tmp = ps_new();
  for (int i = 0; i < MAX_SLICES; i++) {
    if (probeinfo.map[i] == NULL)
      break;
    if (ps_size(probeinfo.map[i]) < NWAYS)
      continue;
    for (int j = 0; j < NWAYS; j++) {
      ps_push(tmp, ps_get(probeinfo.map[i], j));
    }
  }
  ps_randomise(tmp);

  for (int i = 0; i < SETINDEX_LINES; i++) {
    cacheline_t cl = NULL;
    for (int j = ps_size(tmp); j--; ) {
      int n = ps_get(tmp, j);
      probeinfo.eb[n].cachelines[i].cl_links[SETINDEXLINK] = cl;
      cl = &probeinfo.eb[n].cachelines[i];
    }
    probeinfo.indexlist[i] = cl;
  }
  ps_delete(tmp);
}
 



void evictmeasureloop(pageset_t ps, int candidate, int si, int link, ts_t ts, int count) {
  ts_clear(ts);
  void *cc = &probeinfo.eb[candidate].cachelines[si];
  cacheline_t cl = NULL;
  for (int i = ps_size(ps); i--; ) {
    int n = ps_get(ps, i);
    probeinfo.eb[n].cachelines[si].cl_links[link] = cl;
    cl = &probeinfo.eb[n].cachelines[si];
  }
  for (int i = 0; i < count; i++) {
    probe_access(cc);
    for (int j = 0; j < SPLIT_MEASURE_EVICT_COUNT; j++)
      walk(cl, link);
    ts_add(ts, probe_time(cc));
  }
}

int probe_evictMeasure(pageset_t evict, int measure, int offset, ts_t ts, int count) {
  evictmeasureloop(evict, measure, offset, 1, ts, count);
  return ts_median(ts);
}

static void findmap(pageset_t eb, int candidate, char *map, pageset_t *pss, int si, ts_t ts) {
  pageset_t t = ps_dup(eb);
  pageset_t quick[MAX_SLICES];
  for (int i = 0; i < MAX_SLICES; i++)
    quick[i] = NULL;
  int psid = -1;
  int new = 0;
  for (int i = 0; i < ps_size(eb); i++)  {
    int r = ps_get(eb, i);
    ps_remove(t, r);
    if (probe_evictMeasure(t, candidate, si, ts, L3REPEATS) < L3THRESHOLD) {
      if (map[r] == -1) {
	if (psid == -1) {
	  for (int j = 0; j < MAX_SLICES; j++) 
	    if (pss[j] == NULL) {
	      pss[j] = ps_new();
	      psid = j;
	      break;
	    }
	  new = 1;
	}
	if (debug) {
	  if (!new)
	    fprintf(stderr, "Unmapped eb %d added to set %d\n", r, psid);
	  else
	    fprintf(stderr, "(eb) %d ==> %d\n", r, psid);
	}
	map[r] = psid;
	ps_push(pss[psid], r);
      }
      if (psid == -1) 
	psid = map[r];
      if (psid != map[r])
	fprintf(stderr, "Double conflict %d, %d (on eb %d)\n", psid, map[r], r);
      if (map[candidate] == -1)  {
	//fprintf(stderr, "(ca) %d ==> %d\n", candidate, psid);
	map[candidate] = psid;
	ps_push(pss[psid], candidate);
      }
    }
    ps_push(t, r);
  }
}
  

static int findquick(pageset_t *quick, int candidate, pageset_t *pss,  char *map, int si, ts_t ts, int count) {
  for (int i = 0; i < MAX_SLICES; i++) {
    if (quick[i] != NULL && probe_evictMeasure(quick[i], candidate, si, ts, count) >= L3THRESHOLD) {
      int index = ps_get(quick[i], 0);
      map[candidate] = map[index];
      ps_push(pss[map[index]], candidate);
      return 1;
    }
  }
  return 0;
}


pageset_t *split(int si) {
  pageset_t candidates = ebpageset();
  pageset_t eb = ps_new();
  cacheline_t ebcl = NULL;
  pageset_t *rv = calloc(MAX_SLICES, sizeof(pageset_t));
  pageset_t quick[MAX_SLICES];
  for (int i = 0; i < MAX_SLICES; i++)
    quick[i] = NULL;
  char *map = malloc(probeinfo.ebsetindices);
  for (int i = 0; i < probeinfo.ebsetindices; i++)
    map[i] = -1;

  ts_t ts = ts_alloc();
  int i = 0;
  int fq = 0;
  while (ps_size(candidates)) {
    int candidate = ps_pop(candidates);
    if (findquick(quick, candidate, rv, map, si, ts, L3REPEATS)) {
      fq++;
      continue;
    }
    int time = probe_evictMeasure(eb, candidate, si, ts, L3REPEATS);
    //printf("%3d: %3d %d\n", i++, candidate, time);
    if (time < L3THRESHOLD) 
      ps_push(eb, candidate);
    else {
      findmap(eb, candidate, map, rv, si, ts);
      if (map[candidate] != -1 && ps_size(rv[map[candidate]]) ==25) {
	for (int i = 0; i < MAX_SLICES; i++)
	  if (quick[i] == NULL) {
	    quick[i] = ps_dup(rv[map[candidate]]);
	    break;
	  }
      }
    }
  }
  for (int i = 0; i < MAX_SLICES; i++) {
    if (quick[i] != NULL)
      ps_delete(quick[i]);
  }
  free(map);
  return rv;
}

/*
int acctime(ts_t ts, pageset_t ps, int si, int link, int count) {
  ts_clear(ts);
  pageset_t tps = ps_new();
  for (int i = 0; i < 10; i++)
    ps_push(tps, ps_get(ps, i));
  int cand = ps_get(ps, 10);
  evictmeasureloop(tps, cand, si, link, ts, count);
  int rv = ts_median(ts);
  ps_delete(tps);
  return rv;
}
*/


void probe_printmap() {
  for (int i = 0; i < MAX_SLICES; i++) {
    if (probeinfo.map[i] == NULL)
      break;
    pageset_t map = ps_dup(probeinfo.map[i]);
    ps_sort(map);
    printf("Map %2d: ", i);
    int l = ps_size(map);
    for (int j=0; j < l; j++)
      printf("%3d ", ps_get(map, j));
    printf("\n");
    ps_delete(map);
  }
}


void probe_init(uint64_t ebsetindices) {

  probeinfo.ebsetindices = ebsetindices  / SETINDEX_SIZE;
  probeinfo.eb = (setindex_t) mmap64(NULL, probeinfo.ebsetindices * SETINDEX_SIZE, PROT_READ|PROT_WRITE, 
      						MAP_LARGEPAGES|MAP_ANON|MAP_PRIVATE, -1, 0);
  if (probeinfo.eb == MAP_FAILED) {
    perror("probe_init: eb: mmap");
    exit(1);
  }

    
  probeinfo.map = split(17);
  for (int i = 0; i < MAX_SLICES; i++) {
    if (probeinfo.map[i] == NULL)
      break;
    ps_randomise(probeinfo.map[i]);
  }

  build_probelines();

}

/*
 * bufsize - size of the buffer
 * stride - cache stride within the buffer
 *
 * Need bufsize >= (NWAYS*NCORES)stride
 * Assumes that positions x+i*stride for 0<=i<NWAYS*NCORES completely cover some cache sets.
 *
 * For Intel processors with 2, 4 or 8 cores, when using large pages, a stride of 128K should work.
 */
/*building the probe buffer, the stride parameter is not used.*/
void probe_init_simple(uint64_t bufsize, int stride) {
  pageset_t ps = ps_new();
  for (int i = 0; i < NWAYS*NCORES; i++)
    ps_push(ps, i);
  ps_randomise(ps);

  /*FIXME: backup a large contiguous memory for probing*/
  probeinfo.ebsetindices = bufsize/SETINDEX_SIZE;
  probeinfo.eb = (setindex_t) mmap64(NULL, 
          probeinfo.ebsetindices * SETINDEX_SIZE,
          PROT_READ|PROT_WRITE, 
          MAP_LARGEPAGES|MAP_ANON|MAP_PRIVATE, -1, 0);

  if (probeinfo.eb == MAP_FAILED) {
    perror("probe_init_simple: eb: mmap");
    exit(1);
  }

   /*linking the cache lines for a cache set, currently only known the 
    offset within a page*/ 
  for (int i = 0; i < PAGE_SIZE/CLSIZE; i++) {
    cacheline_t cl = NULL;
    for (int j = ps_size(ps); j--; ) {
      int n = ps_get(ps, j);
      probeinfo.eb[n].cachelines[i].cl_links[SETINDEXLINK] = cl;
      cl = &probeinfo.eb[n].cachelines[i];
    }
    probeinfo.indexlist[i] = cl;
  }
  ps_delete(ps);
}




