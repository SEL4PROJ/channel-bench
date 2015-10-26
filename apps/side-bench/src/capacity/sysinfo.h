#ifndef __SYSINFO_H__
#define __SYSINFO_H__ 1

#define KB	1024
#define MB 	(1024 * KB)
#define GB	(1024 * MB)

#define NCORES 4
#define NWAYS 12
#define HUGEPAGESIZE (2 * MB)

#if ((HUGEPAGESIZE & (HUGEPAGESIZE - 1))  != 0)
#error "HUGEPAGESIZE must be a power of two"
#endif

#define CACHESIZE (2048ULL * NCORES * NWAYS * 64 )


#define EBSIZE ((CACHESIZE * 2 + HUGEPAGESIZE - 1) & ~(HUGEPAGESIZE -1))



#endif // __SYSINFO_H__
