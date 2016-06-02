
/*covert channel attack related macros*/
#ifndef _COVERT_COMMON_H_ 
#define _COVERT_COMMON_H_ 

#include <autoconf.h>
/*porting from sysinfo.h*/
/*NOTE: current attack only works on x86 machine */

#define KB	1024
#define MB 	(1024 * KB)
#define GB	(1024 * MB)


#define HUGEPAGESIZE (2 * MB)

#define LINE_SIZE 64

#define CACHE_LINES_PER_PAGE  64

/*cache details for L1 cache*/
#define L1_NCORES    1
#define L1_NWAYS 8 
#define L1_NSETS 64ULL 
#define L1_WAY_SIZE  (64 * 64)
/*32K*/
#define L1_CACHESIZE (L1_NSETS * L1_NWAYS * LINE_SIZE) 

#define L1_EBSIZE  L1_CACHESIZE

/*cache details for L2 cache*/
#define L2_NCORES    1
#define L2_NWAYS 8 
#define L2_NSETS 512ULL 
#define L2_WAY_SIZE  (512 * 64)

#define L2_CACHESIZE (L2_NSETS * L2_NWAYS * LINE_SIZE) 

/*single core covert channel uses a L2 cache sized buffer*/
#define L2_EBSIZE      L2_CACHESIZE
#define L2_WORKING_SET_PAGES  64 
#define L2_NSETS_MASK      0x1ff 
#define L2_NSETS_LRU       512

#define LLC_NCORES 4
#define LLC_NWAYS 16
#define LLC_NSETS 2048ULL 
#define LLC_WAY_SIZE ( 2048 * 64)

/*the number of cores equals number of cache slices */
#define LLC_CACHESIZE (LLC_NSETS * LLC_NCORES * LLC_NWAYS * LINE_SIZE )

/*a simple attack uses a LLC sized buffer*/
#define LLC_EBSIZE LLC_CACHESIZE
//#define EBSIZE ((CACHESIZE * 2 + HUGEPAGESIZE - 1) & ~(HUGEPAGESIZE -1))
/*max working set size for trojan in pages*/
#define LLC_WORKING_SET_PAGES   2048 
#define LLC_NSETS_MASK       0x7ff
#define LLC_NSETS_LRU        2048

#define NUM_COVERT_RUNS   30 /*number of attemps in capacity bench*/


/*porting from capacity/probe.c*/
/*backing up contingous mem by manager thread*/
#define MAP_ROUNDSIZE	(2*1024*1024)
#define LLC_SETINDEX_BITS	17
#define L2_SETINDEX_BITS     15

#define CLBITS	6
#define CLSIZE (1 << CLBITS)

#define PAGE_BITS       12 
#ifndef PAGE_SIZE 
#define PAGE_SIZE       (1 << PAGE_BITS)
#endif 
#define PAGE_MASK	(PAGE_SIZE - 1)
#define PAGE_LBITS	(PAGE_BITS - CLBITS)
#define PAGE_LINES	(PAGE_SIZE >> CLBITS)


#define LLC_SETINDEX_SIZE	(1 << LLC_SETINDEX_BITS)
#define LLC_SETINDEX_MASK	(LLC_SETINDEX_SIZE - 1)
#define LLC_SETINDEX_LBITS	(LLC_SETINDEX_BITS - CLBITS)
#define LLC_SETINDEX_LINES	(LLC_SETINDEX_SIZE >> CLBITS)

#define L2_SETINDEX_SIZE	(1 << L2_SETINDEX_BITS)
#define L2_SETINDEX_MASK	(L2_SETINDEX_SIZE - 1)
#define L2_SETINDEX_LBITS	(L2_SETINDEX_BITS - CLBITS)
#define L2_SETINDEX_LINES	(L2_SETINDEX_SIZE >> CLBITS)



#define SETINDEXLINK 0




#ifdef CONFIG_BENCH_COVERT_L2 
#define WORKING_SET_PAGES   L2_WORKING_SET_PAGES 
#define EBSIZE              L2_EBSIZE 
#define NSETS_LRU           L2_NSETS_LRU 
#define NSETS_MASK          L2_NSETS_MASK
#define SETINDEX_LINES      L2_SETINDEX_LINES 
#define SETINDEX_SIZE       L2_SETINDEX_SIZE
#define NWAYS               L2_NWAYS 
#define NCORES              L2_NCORES 
#define CACHESIZE           L2_CACHESIZE 
#endif 

#ifdef CONFIG_BENCH_COVERT_LLC 
#define WORKING_SET_PAGES  LLC_WORKING_SET_PAGES 
#define EBSIZE             LLC_EBSIZE
#define NSETS_LRU          LLC_NSETS_LRU 
#define NSETS_MASK         LLC_NSETS_MASK 
#define SETINDEX_LINES     LLC_SETINDEX_LINES
#define SETINDEX_SIZE      LLC_SETINDEX_SIZE 
#define NWAYS              LLC_NWAYS 
#define NCORES             LLC_NCORES
#define CACHESIZE          LLC_CACHESIZE 
#endif 


#ifndef EBSIZE   
#define EBSIZE              0
#endif 
#ifndef CACHESIZE
#define CACHESIZE           0
#endif 

#if 0
// Number of probes to find the set index of a virtual address
#define SETINDEX_NPROBE	64

#define MAX_SLICES 32

#define L3THRESHOLD 120
#define L3REPEATS 32
#define SPLIT_MEASURE_EVICT_COUNT 32
#endif 


#define LINE 2  /*targeting this cache set in a slice */
#define REPS 200  /*the number of repeats for each run*/


/*time statistics timestat.h*/
#define LLC_TIME_MAX	20480
#define L2_TIME_MAX     1000   /*8 cache lines in a L2 set, more than 100 cycle each*/

#ifdef CONFIG_BENCH_COVERT_L2
#define TIME_MAX     L2_TIME_MAX 
#endif 

#ifdef CONFIG_BENCH_COVERT_LLC
#define TIME_MAX     LLC_TIME_MAX 
#endif 

#ifndef TIME_MAX  
#define TIME_MAX     100 
#endif 




#endif 


