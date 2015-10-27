#ifndef __SYSINFO_H__
#define __SYSINFO_H__ 1

/*NOTE: current attack only works on x86 machine */

#define KB	1024
#define MB 	(1024 * KB)
#define GB	(1024 * MB)

/*FIXME: configurable for different machine models*/
#define NCORES 4
#define NWAYS 12
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



#endif // __SYSINFO_H__
