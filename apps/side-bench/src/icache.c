/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


/*benchmarks for the L1 i cache */
/*Zhang_JRR_12*/
/*FIXME: attack xxxx */
#include <platsupport/arch/tsc.h>

/*Core i7 2600 has 32KB L1 caches, 8 way set associative 
 64 byte cache lines*/

/*FIXME: move to  Kconfig*/
#define LINE_B   64

#define L1_B  0x8000  /*L1 cache size*/
#define L1_S  64      /*L1 cache sets*/
#define L1_P  0x8    /*pages covered by L1*/
#define P_SETS 64    /*sets in a page*/
#define PAGE  0x1000 /*page size*/ 

#if 0
typedef char page[CONFIG_P_SETS][CONFIG_LINE_B];  /*dividing a page into cache line sized block*/
typedef page buffer[CONFIG_L1_P];   /*combined size is equal to the L1 cache*/ 
#endif
/*attacking L1 I cache: 
 * allocate contiguous memory so combined size is equal to the side of the L1 I cache
 * dividing memory page into 64 cache line sized block, the ith data block in each page will map to the same cache set
 * prime a set: executing instructions within the ith block, for w times (w-ways)
 * wait for a time interval, 50,000 PCPU cycles
 * probing the cache set again, and measure the time*/


/*probing: exeucte rdtsc
 jumping to the first page's ith block, which has instructions to jump to the ith block of the next page
 final page jumps back to code that again execute rdtsc, and calculate the execution time 
 repeated for each of the m cache sets to produce a vector of cache set timings*/


/*classifying code path: square and multiply
 classify each vector as indicating a multiplication, modular reduction, or squaring operation */


typedef uint64_t t_u;  /*time unit*/
t_u costs[CONFIG_L1_S]; 


void inline  prime(unsigned int n) {

    char *b = (char*)prime_buffer; 
    /*jumping into the starting point of Nth set*/
    asm volatile(" mov %0, %%eax; 
                   call (%%eax, %1, 0x40)" 
                   : : "r"(b), "r"(n) : 
                   "%eax"); 

}


t_u inline probe(unsigned int n) {

    /*probing the Nth cache set and return cost in CPU cycles*/
   t_u start, end; 
   start = rdtsc_cpuid();
   prime(n); 
   end = rdtsc_cpuid(); 
   return end - start; 
}


void icache_attack(void) {

    for (int i = 0; i < CONFIG_L1_S; i++) {

        prime(i); 
        /*wait for 50,000 PCPU cycle*/

        /*sleep on the interrupt
FIXME: need to set up another CPU*/
        costs[i] = probe(i); 
    }

    /*do some printing*/
}

