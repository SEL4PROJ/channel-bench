#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <mach/vm_statistics.h>
#endif

//#define DEBUG
#include "../mastik_common/low.h"
#include "../mastik_common/vlist.h"
#include "l3_arm.h"
#include "../mastik_common/timestats.h"

#define CHECKTIMES 256

#define LNEXT(t) (*(void **)(t))
#define OFFSET(p, o) ((void *)((uintptr_t)(p) + (o)))
#define NEXTPTR(p) (OFFSET((p), sizeof(void *)))

#define IS_MONITORED(monitored, setno) ((monitored)[(setno)>>5] & (1 << ((setno)&0x1f)))
#define SET_MONITORED(monitored, setno) ((monitored)[(setno)>>5] |= (1 << ((setno)&0x1f)))
#define UNSET_MONITORED(monitored, setno) ((monitored)[(setno)>>5] &= ~(1 << ((setno)&0x1f)))

#ifdef MAP_HUGETLB
#define HUGEPAGES MAP_HUGETLB
#endif
#ifdef VM_FLAGS_SUPERPAGE_SIZE_2MB
#define HUGEPAGES VM_FLAGS_SUPERPAGE_SIZE_2MB
#endif

#ifdef HUGEPAGES
#define HUGEPAGEBITS 21
#define HUGEPAGESIZE (1<<HUGEPAGEBITS)
#define HUGEPAGEMASK (HUGEPAGESIZE - 1)
#endif


#define SLICE_MASK_0 0x1b5f575440UL
#define SLICE_MASK_1 0x2eb5faa880UL
#define SLICE_MASK_2 0x3cccc93100UL


static void fillL3Info(l3pp_t l3) {
    l3->l3info.associativity = L3_ASSOCIATIVITY;
    l3->l3info.bufsize = L3_SIZE; 

}

#if 0
void printL3Info() {
  struct l3info l3Info;
  loadL3cpuidInfo(&l3Info);
  print("type            : %u\n", l3Info.cpuidInfo.cacheInfo.type);
  print("level           : %u\n", l3Info.cpuidInfo.cacheInfo.level);
  print("selfInitializing: %u\n", l3Info.cpuidInfo.cacheInfo.selfInitializing);
  print("fullyAssociative: %u\n", l3Info.cpuidInfo.cacheInfo.fullyAssociative);
  print("logIds          : %u\n", l3Info.cpuidInfo.cacheInfo.logIds + 1);
  print("phyIds          : %u\n", l3Info.cpuidInfo.cacheInfo.phyIds + 1);
  print("lineSize        : %u\n", l3Info.cpuidInfo.cacheInfo.lineSize + 1);
  print("partitions      : %u\n", l3Info.cpuidInfo.cacheInfo.partitions + 1);
  print("associativity   : %u\n", l3Info.cpuidInfo.cacheInfo.associativity + 1);
  print("sets            : %u\n", l3Info.cpuidInfo.cacheInfo.sets + 1);
  print("wbinvd          : %u\n", l3Info.cpuidInfo.cacheInfo.wbinvd);
  print("inclusive       : %u\n", l3Info.cpuidInfo.cacheInfo.inclusive);
  print("complexIndex    : %u\n", l3Info.cpuidInfo.cacheInfo.complexIndex);
  exit(0);
}
#endif


static void *allocate(size_t size, int align) {                                              
    char *p = malloc(size + align);                                                     
    return (void *)((((uintptr_t)p) + align - 1) & ~(align - 1));                         
} 

static void *sethead_single(l3pp_t l3, int set) { 
    /*the group list for that set*/
    vlist_t list = l3->groups[set / l3->groupsize];

    /*number of lines in a set*/
    int count = l3->l3info.associativity;
    if (count == 0 || vl_len(list) < count)
        count = vl_len(list);

    /*offset within a group (page)*/
    int offset = (set % l3->groupsize) * L3_CACHELINE;

    /*link all the lines in a set, forward,  circular link list*/
    /*It does not matter how many pages linked in the group,
      the probe would only select N sets(cache associativity) from a page*/
    for (int i = 0; i < count; i++) {
        LNEXT(OFFSET(vl_get(list, i), offset)) = OFFSET(vl_get(list, (i + 1) % count), offset);
    }

    return OFFSET(vl_get(list, 0), offset);
}

static void *sethead(l3pp_t l3, int set) { 
    /*the group list for that set*/
    vlist_t list = l3->groups[set / l3->groupsize];

    /*number of lines in a set*/
    int count = l3->l3info.associativity;
    if (count == 0 || vl_len(list) < count)
        count = vl_len(list);
    /*offset within a group (page)*/
    int offset = (set % l3->groupsize) * L3_CACHELINE;

    /*link all the lines in a set, forward, and backward, circular link list*/
    /*It does not matter how many pages linked in the group,
     the probe would only select N sets(cache associativity) from a page*/
    for (int i = 0; i < count; i++) {
        LNEXT(OFFSET(vl_get(list, i), offset)) = OFFSET(vl_get(list, (i + 1) % count), offset);
        LNEXT(OFFSET(vl_get(list, i), offset+sizeof(void*))) = OFFSET(vl_get(list, (i + count - 1) % count), offset+sizeof(void *));
    }

    return OFFSET(vl_get(list, 0), offset);
}

void prime(vlist_t vl, int offset, int count) {
    /*prime on the offset within a page, all pages in the page group*/
    for (int i = 0; i < count; i++) 
        (*(int *)OFFSET(vl_get(vl, i), offset))++;
}

#define str(x) #x
#define xstr(x) str(x)

int probetime(void *pp) {
  if (pp == NULL)
    return 0;
  void *p = (void *)pp;
  uint32_t start, end;
  
  SEL4BENCH_READ_CCNT(start);  
  
  do {
      (*(int *)OFFSET(p, 2*sizeof(void *)))++;
      p = LNEXT(p);

  } while (p != (void *) pp);
  SEL4BENCH_READ_CCNT(end);  

  return end - start; 
}

void probetime_lines(void *pp, uint16_t *res) {
  if (pp == NULL)
    return;
  void *p = (void *)pp;
  uint32_t start, end;
  
  do {

      SEL4BENCH_READ_CCNT(start);  
      (*(int *)OFFSET(p, 2*sizeof(void *)))++;
      p = LNEXT(p);

      SEL4BENCH_READ_CCNT(end);  
      *res = end - start; 
      res++;

  } while (p != (void *) pp);
}


int bprobetime(void *pp) {
  if (pp == NULL)
    return 0;
  return probetime(NEXTPTR(pp));
}


int probecount(void *pp) {
  if (pp == NULL)
    return 0;
  int rv = 0;
  void *p = (void *)pp;
  uint32_t start, end;

  /*probe and count how many cache misses*/ 
  do {
      SEL4BENCH_READ_CCNT(start);  
 
      (*(int *)OFFSET(p, 2*sizeof(void *)))++;
      p = LNEXT(p);
      SEL4BENCH_READ_CCNT(end);  
      
      end -= start;
    if (end > L3_THRESHOLD)
      rv++;

  } while (p != (void *) pp);

  return rv;
}

int bprobecount(void *pp) {
  if (pp == NULL)
    return 0;
  return probecount(NEXTPTR(pp));
}


static int timedwalk(void *list, register void *candidate) {

#if defined(DEBUG) && DEBUG-0 > 2
    static int debug = 100;
    static int debugl = 1000;
#else
#define debug 0
#endif //DEBUG

    if (list == NULL)
        return 0;
    if (LNEXT(list) == NULL)
        return 0;

    //void *start = list;
    ts_t ts = ts_alloc();

    void *c2 = (void *)((uintptr_t)candidate ^ 0x200);

    LNEXT(c2) = candidate;
    
    memaccess(candidate);

    for (int i = 0; i < CHECKTIMES; i++) {
        walk(list, 200);
        void *p = LNEXT(c2);
        uint32_t time = memaccesstime(p);
        ts_add(ts, time);
    }

    //int rv = ts_median(ts);
    int rv = ts_percentile(ts, 90);
#if defined(DEBUG) && DEBUG-0 > 2
    if (!--debugl) {
        debugl=1000;
        debug++;
    }
    if (debug) {
        printf("--------------------\n");
        for (int i = 0; i < TIME_MAX; i++) 
            if (ts_get(ts, i) != 0)
                printf("++ %4d: %4d\n", i, ts_get(ts, i));
        debug--;
    }
#endif //DEBUG
    ts_free(ts);
    return rv;
}

static int checkevict(vlist_t es, void *candidate) {
    if (vl_len(es) == 0)
        return 0;

    /*prepare the probing buffer for es*/
    for (int i = 0; i < vl_len(es); i++) 
        LNEXT(vl_get(es, i)) = vl_get(es, (i + 1) % vl_len(es));

    /*the time for accessing the candidate after probing the es */
    int timecur = timedwalk(vl_get(es, 0), candidate);

    /*if the candidate is evicted by probing the es*/
    return timecur > L3_THRESHOLD;
}

#ifdef DEBUG
uintptr_t base;
#endif

static void *expand(vlist_t es, vlist_t candidates) {

    while (vl_len(candidates) > 0) {

        /*select a page from candidates
         put that into the current list*/
        void *current = vl_poprand(candidates);
        //printf("Expand: testing %d\n", ((uintptr_t)current-base)/4096);

        /*the es list is conflict with the current*/
        if (checkevict(es, current))
            return current;

        /*push the current into the es*/
        vl_push(es, current);
    }
    return NULL;
}

static void contract(vlist_t es, vlist_t candidates, void *current) {
    for (int i = 0; i < vl_len(es);) {
        void *cand = vl_get(es, i);

        vl_del(es, i);

        //printf("Contract: testing %d (%d)\n", ((uintptr_t)current-base)/4096, i);
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

static vlist_t map(l3pp_t l3, vlist_t lines) {

#ifdef DEBUG

    /*number of pages*/
    printf("%d pages\n", vl_len(lines));
    base=(uintptr_t)l3->buffer;
#endif // DEBUG
  vlist_t groups = vl_new();
  
  vlist_t es = vl_new();

  /*for every page*/
  int nlines = vl_len(lines);
  int fail = 0;
  while (vl_len(lines)) {

      assert(vl_len(es) == 0);
#ifdef DEBUG
      int d_l1 = vl_len(lines);
#endif // DEBUG
 
      if (fail > 8) 
          break;
     
      void *c = expand(es, lines);

#ifdef DEBUG
      int d_l2 = vl_len(es);
#endif //DEBUG

      /*es contains the probing set*/
      if (c == NULL) {
          while (vl_len(es))
              vl_push(lines, vl_del(es, 0));
#ifdef DEBUG
          printf("set %3d: lines: %4d expanded: %4d c=NULL\n", vl_len(groups), d_l1, d_l2);
#endif // DEBUG
          fail++;
          continue;
      }

      contract(es, lines, c);
      contract(es, lines, c);
      contract(es, lines, c);
      contract(es, lines, c);
      contract(es, lines, c);
#ifdef DEBUG
      int d_l3 = vl_len(es);
#endif //DEBUG
      if (vl_len(es) > l3->l3info.associativity || vl_len(es) < l3->l3info.associativity - 3) {
          vl_push(lines, c);
          while (vl_len(es))
              vl_push(lines, vl_del(es, 0));
#ifdef DEBUG
          printf("set %3d: lines: %4d expanded: %4d contracted: %2d failed\n", vl_len(groups), d_l1, d_l2, d_l3);
#endif // DEBUG
          fail++;
          continue;
      } 
      fail = 0;
      vlist_t set = vl_new();
      vl_push(set, c);
      collect(es, lines, set);
      while (vl_len(es) < l3->l3info.associativity && vl_len(set) > 0)
          vl_push(es, vl_pop(set));
      collect(es, lines, set);
      collect(es, lines, set);
      while (vl_len(es))
          vl_push(set, vl_del(es, 0));
#ifdef DEBUG
      printf("set %3d: lines: %4d expanded: %4d contracted: %2d collected: %d\n", vl_len(groups), d_l1, d_l2, d_l3, vl_len(set));
#endif // DEBUG
      vl_push(groups, set);
      if (l3->l3info.progressNotification) 
          (*l3->l3info.progressNotification)(nlines - vl_len(lines), nlines, l3->l3info.progressNotificationData);
  }

  vl_free(es);
  return groups;
}
static int probemap(l3pp_t l3) {

    vlist_t pages = vl_new();

    /*pushing every page into the pages list*/
    for (int i = 0; i < l3->l3info.bufsize; i+= l3->groupsize * L3_CACHELINE) 
        vl_push(pages, l3->buffer + i);

    vlist_t groups = map(l3, pages);

    //Store map results
    l3->ngroups = vl_len(groups);
    l3->groups = (vlist_t *)calloc(l3->ngroups, sizeof(vlist_t));
    for (int i = 0; i < vl_len(groups); i++)
        l3->groups[i] = vl_get(groups, i);


    vl_free(groups);
    vl_free(pages);
    return 1;
}


static int ptemap(l3pp_t l3) {

  l3->ngroups = L3_PROBE_GROUPS; 

  l3->groups = (vlist_t *)calloc(l3->ngroups, sizeof(vlist_t));
  for (int i = 0; i < l3->ngroups; i++) {
    l3->groups[i] = vl_new();

    /*starting of a page in the group 32 pages for the uncoloured 
     64 pages for the coloured system*/
    for (int p = i * 4096; p < l3->l3info.bufsize; p += 4096*l3->ngroups) {
      vl_push(l3->groups[i], l3->buffer + p);
    }
  }
  return 1;
}
    



l3pp_t l3_prepare(void) {

    // Setup
    l3pp_t l3 = (l3pp_t)malloc(sizeof(struct l3pp));
    if (!l3) 
        return NULL; 

    memset(l3, 0, sizeof(struct l3pp));
    fillL3Info(l3);

    // Cache buffer
    l3->groupsize = L3_SETS_PER_PAGE;		
    // Should change for huge pages and for non-linear maps
    //buffer = mmap(NULL, bufsize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);

    l3->buffer = allocate(l3->l3info.bufsize, 4096);

    if (l3->buffer == NULL) {
        free(l3);
        return NULL;
    }
#ifdef CONFIG_DEBUG_BUILD 
    printf("probe buffer vaddr %p size 0x%x\n", l3->buffer, l3->l3info.bufsize);

#endif 

    /*always return 1, probe map not triggered*/
    if (!ptemap(l3))
        probemap(l3);

#ifdef CONFIG_DEBUG_BUILD 
    for (int i = 0; i < l3->ngroups; i++) {
        printf("%2d:", i);

        /*for the number of pages in this set*/
        for (int j = 0; j < vl_len(l3->groups[i]); j++)
            /*the page number in this probing buffer*/
            printf(" %03x", ((uintptr_t)vl_get(l3->groups[i], j) - (uintptr_t)l3->buffer)/4096);
        printf("\n");
    }
#endif

    // Allocate monitored set info
    /*8 or 16 groups, 128 sets per page = total sets monitored*/
    /*colouring: 1024 sets, normal 2048 sets*/
    l3->monitoredbitmap = (uint32_t *)calloc(l3->ngroups * l3->groupsize / sizeof(uint32_t), sizeof(uint32_t));
    if (!l3->monitoredbitmap) 
        return NULL; 

    l3->monitoredset = (int *)malloc(l3->ngroups * l3->groupsize * sizeof(int));
    if (!l3->monitoredset) 
        return NULL; 

    l3->monitoredhead = (void **)malloc(l3->ngroups * l3->groupsize * sizeof(void *));
    if (!l3->monitoredhead) 
        return NULL;
    l3->nmonitored = 0;

    return l3;

}


void *l3_set_probe_head(l3pp_t l3, int line) {

    if (line < 0 || line >= l3->ngroups * l3->groupsize) 
        return NULL;

    return sethead_single(l3, line);
}


int l3_monitor(l3pp_t l3, int line) {

    /*monitor the Nth set*/
    if (line < 0 || line >= l3->ngroups * l3->groupsize) 
        return 0;

    if (IS_MONITORED(l3->monitoredbitmap, line))
        return 0;
    SET_MONITORED(l3->monitoredbitmap, line);
    l3->monitoredset[l3->nmonitored] = line;
    l3->monitoredhead[l3->nmonitored] = sethead(l3, line);
    l3->nmonitored++;
    return 1;
}

int l3_unmonitor(l3pp_t l3, int line) {
  if (line < 0 || line >= l3->ngroups * l3->groupsize) 
    return 0;
  if (!IS_MONITORED(l3->monitoredbitmap, line))
    return 0;
  UNSET_MONITORED(l3->monitoredbitmap, line);
  for (int i = 0; i < l3->nmonitored; i++) {
      if (l3->monitoredset[i] == line) {

          --l3->nmonitored;
          l3->monitoredset[i] = l3->monitoredset[l3->nmonitored];
          l3->monitoredhead[i] = l3->monitoredhead[l3->nmonitored];
          break;
      }
  }
  return 1;
}

void l3_unmonitorall(l3pp_t l3) {
  l3->nmonitored = 0;
  for (int i = 0; i < l3->ngroups*l3->groupsize / sizeof (uint32_t); i++)
    l3->monitoredbitmap[i] = 0;
}

int l3_getmonitoredset(l3pp_t l3, int *lines, int nlines) {
  if (lines == NULL || nlines == 0)
    return l3->ngroups;
  if (nlines > l3->ngroups)
    nlines = l3->ngroups;
  memcpy(lines, l3->monitoredset, nlines * sizeof(int));
  return l3->ngroups;
}

void l3_randomise(l3pp_t l3) {
  for (int i = 0; i < l3->nmonitored; i++) {
    int p = random() % (l3->nmonitored - i) + i;
    int t = l3->monitoredset[p];
    l3->monitoredset[p] = l3->monitoredset[i];
    l3->monitoredset[i] = t;
    void *vt = l3->monitoredhead[p];
    l3->monitoredhead[p] = l3->monitoredhead[i];
    l3->monitoredhead[i] = vt;
  }
}

void l3_probe(l3pp_t l3, uint16_t *results) {
  for (int i = 0; i < l3->nmonitored; i++) {
    int t = probetime(l3->monitoredhead[i]);
    results[i] = t > UINT16_MAX ? UINT16_MAX : t;
  }
}

void l3_bprobe(l3pp_t l3, uint16_t *results) {
  for (int i = 0; i < l3->nmonitored; i++) {
    int t = bprobetime(l3->monitoredhead[i]);
    results[i] = t > UINT16_MAX ? UINT16_MAX : t;
  }
}

void l3_probecount(l3pp_t l3, uint16_t *results) {
 
    /*probe and count L3 cache misses in each set*/
    for (int i = 0; i < l3->nmonitored; i++) {
        results[i] = probecount(l3->monitoredhead[i]);

        int g = l3->monitoredset[i];
        /*prime the set using the buffer stroed in the group, identified 
         by the offset within a page*/
        prime(l3->groups[g/l3->groupsize], (g%l3->groupsize)*L3_CACHELINE + 2*sizeof(void*), l3->l3info.associativity);
        prime(l3->groups[g/l3->groupsize], (g%l3->groupsize)*L3_CACHELINE + 2*sizeof(void*), l3->l3info.associativity);
        prime(l3->groups[g/l3->groupsize], (g%l3->groupsize)*L3_CACHELINE + 2*sizeof(void*), l3->l3info.associativity);
    }

}
void l3_probecount_simple(l3pp_t l3, uint16_t *results) {
 
    /*probe and count L3 cache misses in each set*/
    for (int i = 0; i < l3->nmonitored; i++) {
        results[i] = probecount(l3->monitoredhead[i]);
    }

}
void l3_bprobecount(l3pp_t l3, uint16_t *results) {
  for (int i = 0; i < l3->nmonitored; i++) {
    results[i] = bprobecount(l3->monitoredhead[i]);
    int g = l3->monitoredset[i];
    prime(l3->groups[g/l3->groupsize], (g%l3->groupsize)*L3_CACHELINE + 2*sizeof(void*), l3->l3info.associativity);
    prime(l3->groups[g/l3->groupsize], (g%l3->groupsize)*L3_CACHELINE + 2*sizeof(void*), l3->l3info.associativity);
    prime(l3->groups[g/l3->groupsize], (g%l3->groupsize)*L3_CACHELINE + 2*sizeof(void*), l3->l3info.associativity);
  }
}

int probeloop(l3pp_t l3, uint16_t *results, int count, int slot) {
  uint16_t *start = results;
  /*probing count times */
  uint32_t time_s, time_e; 
  
  for (int i = 0; i < count; i++) {
    l3_probecount(l3, results);
    results += l3->nmonitored;
    SEL4BENCH_READ_CCNT(time_s);  
  
    do {
        SEL4BENCH_READ_CCNT(time_e);  
    } while ((time_e - time_s) < slot);
  }
#ifdef CONFIG_DEBUG_BUILD 
  if (results - start == 0)
    printf("Groups: %d\n", l3->ngroups);
#endif
  return results-start;

}


// Returns the number of probed sets in the LLC
/*normal 2048, colouring 1024*/
int l3_getSets(l3pp_t l3) {
  return l3->ngroups * l3->groupsize;
}


// Returns the LLC associativity
int l3_getAssociativity(l3pp_t l3) {
  return l3->l3info.associativity;
}


