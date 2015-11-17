
/*covert channel attack related macros*/
#ifndef _COVERT_COMMON_H_ 
#define _COVERT_COMMON_H_ 

/*porting from sysinfo.h*/
/*NOTE: current attack only works on x86 machine */

#define KB	1024
#define MB 	(1024 * KB)
#define GB	(1024 * MB)

/*FIXME: configurable for different machine models*/
#define NCORES 4
#define NWAYS 16
#define NSETS 2048ULL 
#define LINE_SIZE 64
#define WAY_SIZE ( 2048 * 64)
#define HUGEPAGESIZE (2 * MB)

#if ((HUGEPAGESIZE & (HUGEPAGESIZE - 1))  != 0)
#error "HUGEPAGESIZE must be a power of two"
#endif

/*assuming number of cores equals number of cache slices */
#define CACHESIZE (NSETS * NCORES * NWAYS * LINE_SIZE )


#define EBSIZE ((CACHESIZE * 2 + HUGEPAGESIZE - 1) & ~(HUGEPAGESIZE -1))




/*porting from capacity/probe.c*/
/*backing up contingous mem by manager thread*/
#define MAP_ROUNDSIZE	(2*1024*1024)
#define SETINDEX_BITS	17

#define PAGE_BITS	12
#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(PAGE_SIZE - 1)
#define PAGE_LBITS	(PAGE_BITS - CLBITS)
#define PAGE_LINES	(PAGE_SIZE >> CLBITS)



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


#define SETINDEXLINK 0


/*time statistics timestat.h*/
#define TIME_MAX	20480

struct ts {
  uint32_t data[TIME_MAX];
};

typedef struct ts *ts_t;

inline uint32_t ts_get(ts_t ts, int tm) {
  assert(tm > 0);
  return tm < TIME_MAX ? ts->data[tm] : 0;
}




#endif 


