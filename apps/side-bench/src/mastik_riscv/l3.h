#ifndef __L3_H__
#define __L3_H__ 1

#include "../mastik_common/vlist.h"
typedef void (*l3progressNotification_t)(int count, int est, void *data);
struct l3info {
  int associativity;
  int bufsize;
  int flags;
  l3progressNotification_t progressNotification;
  void *progressNotificationData;
};


struct l3pp {
  struct l3info l3info;

  // To reduce probe time we group sets in cases that we know that a group of consecutive cache lines will
  // always map to equivalent sets. In the absence of user input (yet to be implemented) the decision is:
  // Non linear mappings - 1 set per group (to be implemeneted)
  // Huge pages - L3_SETS_PER_SLICE sets per group (to be impolemented)
  // Otherwise - L3_SETS_PER_PAGE sets per group.
  int ngroups;
  int groupsize;
  vlist_t *groups;
  void *buffer;
  uint32_t *monitoredbitmap;
  int *monitoredset;
  int nmonitored;
  void **monitoredhead;
};



typedef struct l3pp *l3pp_t;
typedef struct l3info *l3info_t;


#define L3FLAG_NOHUGEPAGES	0x01
#define L3FLAG_NOPTE		0x02


l3pp_t l3_prepare(void);

// Returns the number of probed sets in the LLC
int l3_getSets(l3pp_t l3);

// Returns the number slices
int l3_getSlices(l3pp_t l3);

// Returns the LLC associativity
int l3_getAssociativity(l3pp_t l3);

int l3_monitor(l3pp_t l3, int line);
int l3_unmonitor(l3pp_t l3, int line);
void l3_unmonitorall(l3pp_t l3);
int l3_getmonitoredset(l3pp_t l3, int *lines, int nlines);

void l3_randomise(l3pp_t l3);

void l3_probe(l3pp_t l3, uint16_t *results);
void l3_bprobe(l3pp_t l3, uint16_t *results);
void l3_probecount(l3pp_t l3, uint16_t *results);
void l3_bprobecount(l3pp_t l3, uint16_t *results);
void l3_probecount_simple(l3pp_t l3, uint16_t *results);
int probeloop(l3pp_t l3, uint16_t *results, int count, int slot);
int probecount(void *pp); 
void *l3_set_probe_head(l3pp_t l3, int line); 
int probetime(void *pp); 
void probetime_lines(void *pp, uint16_t *res);
#endif // __L3_H__

