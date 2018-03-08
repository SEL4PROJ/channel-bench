/*
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#
*/

/*This file contains the benchmarks for measuring the indirect cost
 of flushing caches. The working set size of this thread is configured 
 at Kconfig.*/

#include <autoconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sel4/sel4.h>
#include <sel4bench/sel4bench.h>

#ifdef CONFIG_ARCH_X86 
#include "low.h"
#include "l1.h"
#include "vlist.h"
#include "./mastik/cachemap.h"
#include "./mastik/pp.h"
#endif
#include "bench_common.h"
#include "bench.h"

#ifdef CONFIG_BENCH_CACHE_FLUSH 
/*creating the mmap buffer for creating the probe buffer on LLC*/
#define SIZE (16*1024*1024)


static uint32_t probe_size = CONFIG_BENCH_FLUSH_START;
static sel4bench_counter_t measure_overhead[OVERHEAD_RUNS];
static volatile uint32_t readc, readt; 
static uint32_t overhead_llc; 

static inline int overhead_stable(sel4bench_counter_t *array) {
    for (int i = 1; i < OVERHEAD_RUNS; i++) {
        if (array[i] != array[i - 1]) {
            return 0;
        }
    }
    /*the overhead is stable, using any record in the array as 
     the set value, using in the probe list function*/
    overhead_llc = array[0]; 
    printf("overhead_llc %d will be used in the probe_llc\n", overhead_llc);
    return 1;
}


static inline void overhead(void) {

    /*measure the overhead of benchmarking method*/
    sel4bench_counter_t start, end; 

    for (int i = 0; i < OVERHEAD_RETRIES; i++) {
        for (int j = 0; j < OVERHEAD_RUNS; j++) { 
            FENCE(); 
            for (int k = 0; k < WARMUPS; k++) { 

#ifdef CONFIG_ARCH_ARM
                start = sel4bench_get_cycle_count(); 
                end = sel4bench_get_cycle_count(); 
#endif
#ifdef CONFIG_ARCH_X86
                start = rdtscp(); 
                end = rdtscp();
#endif 
            }
            FENCE(); 
            measure_overhead[j] = end - start;
        }

        if (overhead_stable(measure_overhead)) break; 
    }

}

#ifdef CONFIG_ARCH_X86

static uint16_t *l1_probe_result; 
/*l1 data flush buffer*/
static l1info_t l1_1, l1_2, l1_3;
/*llc flush buffer*/ 
static cachemap_t cm_1, cm_2, cm_3;
/*llc flush probe link list*/
static pp_t *pps_1; 
static pp_t *pps_2; 
static pp_t *pps_3; 
static int a; /*variabel used in map(), do not understand the purpose*/
/*the probe buffer used by llc test, trying to test a user-level cache 
 flush*/
uint32_t probe_buffer_1, probe_buffer_2, probe_buffer_3; 

static inline void flush_buffer_llc (void) { 


     /*trying to flush the probe buffer used by probe_llc test, 
     by using the linear address of the probing buffer*/
    
    for (int i = 0; i < SIZE + 4096 * 2 ; i += 64) {
        clflush((void *)(probe_buffer_1 + i)); 
        clflush((void *)(probe_buffer_2 + i)); 
        clflush((void *)(probe_buffer_3 + i)); 
    }

           
}



static cachemap_t map(uint32_t *probe_buf) {
  for (int i = 0; i < 1024*1024; i++)
    for (int j = 0; j < 1024; j++)
      a *=i+j;
  
  char *buf = (char *)mmap(NULL, SIZE + 4096 * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  if (buf == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
   
  /*making the buffer page aligned*/
  int buf_switch = (int) buf; 
  *probe_buf = buf_switch;

  buf_switch &= ~(0xfff); 
  buf_switch += 0x1000; 
  buf = (char *) buf_switch; 

  printf("buf_switch in map() 0x%x\n", buf_switch); 

  cachemap_t cm;
  vlist_t candidates;
  candidates = vl_new();
  for (int i = 0; i < SIZE; i += 4096) 
    vl_push(candidates, buf + i);

  cm = cm_linelist(candidates);
  printf("Cachemap done: %d sets, %d unloved\n", cm->nsets, vl_len(candidates));
  //if (cm->nsets != 128 || vl_len(candidates) != 0) 
    //exit (1);
  return cm;

}
static inline void prepare_probe_buffer_llc(void) {

    /*3 buffers, one for half of the LLC, one for LLC, two for twice as big as LLC*/
 
    pp_t *pp; 
    cm_1 = map(&probe_buffer_1);
    cm_2 = map(&probe_buffer_2);
    cm_3 = map(&probe_buffer_3); 

    /*creating the probing buffer*/
    pps_1 =(pp_t *) malloc (32 * 128 * sizeof (pp_t)); 
    assert(pps_1 != NULL);
    pps_2 = (pp_t *)malloc(64 * 128 * sizeof (pp_t));
    assert(pps_2 != NULL); 
    pps_3 = (pp_t *)malloc(64 * 128 * sizeof (pp_t));
    assert(pps_3 != NULL); 

    pp = pps_1; 
    for (int i = 0; i < 2048; i += 64) {

        for (int j = 0; j <  cm_1->nsets; j++) {
            *pp = pp_prepare(cm_1->sets[j], L3_ASSOCIATIVITY, i); 
            clflush((void *)pp); 
            pp++;
        }
       
    }
    pp = pps_2; 
    for (int i = 0; i < 4096; i += 64) {

        for (int j = 0; j < cm_2->nsets; j++) {
            *pp = pp_prepare(cm_2->sets[j], L3_ASSOCIATIVITY, i); 
            clflush((void *)pp);
            pp++;
        
        }
    }
    pp = pps_3; 
    for (int i = 0; i < 4096; i += 64) {

        for (int j = 0; j < cm_3->nsets; j++) {
            *pp = pp_prepare(cm_3->sets[j], L3_ASSOCIATIVITY, i); 
            clflush((void *)pp);
            pp++; 
        }
    }
}

static inline sel4bench_counter_t walk_probe_buffer_llc (pp_t *probe_list, 
        cachemap_t cm, bool half) { 

    int offset = half ? 2048: 4096; 

    sel4bench_counter_t result = 0ull;
    
    for (int i = 0; i < offset; i += 64) {

        /*probe the buffer for different offset within a page*/

        for (int j = 0; j < cm->nsets; j++) {
            result += pp_probe_flush(*probe_list);
            result -= overhead_llc; 
            probe_list++;
        }
    }

    return result;

}




/*benchmark the cost of flushing LLC currently only supports x86*/
seL4_Word bench_flush_llc(void *record) {

    /*recording the result at buffer, only return the result*/
    sel4bench_counter_t *r_buf = (sel4bench_counter_t*)record; 
    uint32_t n_flush = 0, n_runs = CONFIG_BENCH_FLUSH_RUNS + WARMUPS;

    /*measure the overhead of read timestamp*/
    overhead();

    prepare_probe_buffer_llc(); 
    /*flushing the buffer from llc, then loading them into 
     the llc with correct state, shared state for RO test, or modified 
     for RW test*/
    flush_buffer_llc();

    while (probe_size <= CONFIG_BENCH_CACHE_BUFFER) {

        FENCE();

        while (n_flush++ < n_runs) {

#ifndef CONFIG_BENCH_CACHE_FLUSH_NONE 
            /*using seL4 yield to jump into kernel
              seL4 will do the cache flush. according to the configured type
             because the L1 cache is small, only jumping into the kernel may be
             enough to pollute the cache*/
            seL4_Yield();
#endif

       

            /*walk the probe buffer*/
            if (probe_size == CONFIG_BENCH_FLUSH_START) {
                *r_buf = walk_probe_buffer_llc(pps_1, cm_1, true);
            }
            else if (probe_size == CONFIG_BENCH_FLUSH_START * 2) {
                *r_buf = walk_probe_buffer_llc(pps_2, cm_2, false);
            } else {
                *r_buf = walk_probe_buffer_llc(pps_2, cm_2, false);
                *r_buf += walk_probe_buffer_llc(pps_3, cm_3, false);
            }
            r_buf++;

        }

        FENCE();
        n_flush = 0;
        probe_size *= 2;
    }

    /*report the overhead of measure timestamp*/
    for (int i = 0; i < OVERHEAD_RUNS; i++) {
        *r_buf = measure_overhead[i];
        r_buf++;
    }
    return BENCH_SUCCESS;
}

static inline sel4bench_counter_t walk_probe_buffer(l1info_t p_buf) {


    l1_probe(p_buf, l1_probe_result); 

    return l1_probe_result[0] - overhead_llc;
}

static inline void prepare_probe_buffer(void) {

    l1_1 = l1_prepare(~0LLU);
    l1_2 = l1_prepare(~0LLU);
    l1_3 = l1_prepare(~0LLU);

    l1_probe_result = malloc(l1_nsets(l1_2)*sizeof(uint16_t));
    assert(l1_probe_result != NULL);  
    /*using only half of the L1 for the L1_1*/
    l1_set_monitored_set(l1_1, 0xffffffffLLU);


    /*flush the buffer from caches, resetting the cache line states */
    l1_probe_clflush(l1_1, l1_probe_result); 
    l1_probe_clflush(l1_2, l1_probe_result); 
    l1_probe_clflush(l1_3, l1_probe_result); 

}

seL4_Word bench_flush(seL4_CPtr reply_ep, void *record) {

    /*recording the result at buffer, only return the result*/
    sel4bench_counter_t *r_buf = (sel4bench_counter_t*)record; 
    uint32_t n_flush = 0, n_runs = CONFIG_BENCH_FLUSH_RUNS + WARMUPS;

    /*using a sperate function for testing the LLC buffer*/
   if (CONFIG_BENCH_CACHE_BUFFER * 4  > 64 * 1024) 
       return bench_flush_llc(record); 

    /*measure the overhead of read timestamp*/
    overhead();

    prepare_probe_buffer();


    while (probe_size <= CONFIG_BENCH_CACHE_BUFFER) {

        FENCE();

        while (n_flush++ < n_runs) {

#ifndef CONFIG_BENCH_CACHE_FLUSH_NONE 
            /*using seL4 yield to jump into kernel
              seL4 will do the cache flush. according to the configured type
             because the L1 cache is small, only jumping into the kernel may be
             enough to pollute the cache*/
            seL4_Yield();
#endif
            /*walk the probe buffer*/
            if (probe_size == CONFIG_BENCH_FLUSH_START) {
                *r_buf = walk_probe_buffer(l1_1);
            }
            else if (probe_size == CONFIG_BENCH_FLUSH_START * 2) {
                *r_buf = walk_probe_buffer(l1_2); 
            } else {

                *r_buf = walk_probe_buffer(l1_2); 
                *r_buf += walk_probe_buffer(l1_3);
            }

            r_buf++;

        }

        FENCE();
        n_flush = 0;
        probe_size *= 2;
    }

    /*report the overhead of measure timestamp*/
    for (int i = 0; i < OVERHEAD_RUNS; i++) {
        *r_buf = measure_overhead[i];
        r_buf++;
    }
    return BENCH_SUCCESS;

}

#endif 
#ifdef CONFIG_ARCH_ARM 

uint32_t *probe_buffer_1, *probe_buffer_2, *probe_buffer_3;

static uint32_t *map_buffer(uint32_t size) {
    printf("map_buf size  0x%x\n", size);
    /*allocating buffer from huge page, mapped by manager*/
    char *buf = (char *)mmap(NULL, 4 * size  + 4096 * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
   
  /*making the buffer page aligned*/
  int buf_switch = (int) buf; 

  buf_switch &= ~(0xfff); 
  buf_switch += 0x1000; 
  return  (uint32_t *) buf_switch; 

}

static inline void create_probe_list(uint32_t *probe_buffer, uint32_t size) {

    /*creating a probing list that is generated in random order*/

    uint32_t lines = 0, *current = probe_buffer, *next = NULL; 
    uint32_t nlines = size / CONFIG_BENCH_CACHE_LINE_SIZE;
    uint32_t buffer_lines = nlines; 
    /*if the size is more than 32K, select from a large section*/
   // if (size > 1 * 1024 * 1024) 
     //   buffer_lines =  2 * nlines; 

    int select;

    printf("creating probe list size 0x%x lines %d  \n", size, nlines);

    while (lines < nlines - 1) {

        /*select a random line*/
        select = random() % buffer_lines; 
        next = (uint32_t *)((char*)probe_buffer + select * CONFIG_BENCH_CACHE_LINE_SIZE); 
        /*no deadloop*/
        if (next == current) 
            continue; 
        /*not connected yet*/
        if (*next)
            continue;

        /*link the line, and continue*/
        *current = (uint32_t)next; 
        current = next;
        lines++; 
    }
    /*the last line connect to the first line*/
    *current = (uint32_t)probe_buffer;    

}


static void prepare_probe_buffer_arm(void) {

   probe_buffer_1 = map_buffer(probe_size * 4);
   probe_buffer_2 = map_buffer(probe_size * 4 * 2 ); 
   probe_buffer_3 = map_buffer(probe_size * 4 * 4);

   printf("probe_buffers %p, %p, %p\n", probe_buffer_1, probe_buffer_2, probe_buffer_3); 

  /*generating the link list on the probing buffers*/
  create_probe_list(probe_buffer_1, probe_size * 4); 
  create_probe_list(probe_buffer_2, probe_size * 4 * 2); 
  create_probe_list(probe_buffer_3, probe_size * 4 * 4);

  
}

static sel4bench_counter_t walk_random_buffer_arm(uint32_t **list) {


    /*starting from the buffer */
    uint32_t volatile __attribute__((unused)) **p = list; 
    sel4bench_counter_t s, e; 
    
    s = sel4bench_get_cycle_count(); 

    do {

        /*unwrap the loop to mitigate the branch prediction cost*/
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
      p = (uint32_t **)*p;
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;           
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;      


#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;           
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;      




#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;           
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;      




#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;           
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;    
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p; 
#ifndef CONFIG_BENCH_CACHE_FLUSH_READ 
      *(uint32_t*)((uint32_t)p + 8) =  0xff;
#endif 
        p = (uint32_t **)*p;      

        asm volatile("" ::"r"(p):"memory");
    } while (p != list); 

    e = sel4bench_get_cycle_count(); 
    return e - s - overhead_llc; 
}


seL4_Word bench_flush(seL4_CPtr result_ep, void *record) {

    /*recording the result at buffer, only return the result*/
    sel4bench_counter_t *r_buf = (sel4bench_counter_t*)record; 
    uint32_t n_flush = 0, n_runs = CONFIG_BENCH_FLUSH_RUNS + WARMUPS;

    /*measure the overhead of read timestamp*/
    overhead();

    prepare_probe_buffer_arm(); 
    /*sending msg manager, doing a cache flush on probe buffers*/
    send_msg_to(result_ep, 0); 
    wait_msg_from(result_ep);
 
   
    while (probe_size <= CONFIG_BENCH_CACHE_BUFFER) {

        FENCE();
            /*using seL4 yield to jump into kernel
              seL4 will do the cache flush. according to the configured type
              because the L1 cache is small, only jumping into the kernel may be
              enough to pollute the cache*/
        
            while (n_flush++ < n_runs) {

#ifndef CONFIG_BENCH_MANAGER_FLUSH_NONE 
            /*using seL4 yield to jump into kernel
              seL4 will do the cache flush. according to the configured type
              because the L1 cache is small, only jumping into the kernel may be
              enough to pollute the cache*/
            seL4_Yield();
#endif
            /*walk the random buffer created for sabre platform*/

            if (probe_size == CONFIG_BENCH_FLUSH_START) {
                *r_buf = walk_random_buffer_arm(probe_buffer_1);
            }
            else if (probe_size == CONFIG_BENCH_FLUSH_START * 2) {
                *r_buf = walk_random_buffer_arm(probe_buffer_2);
            } else {
                *r_buf = walk_random_buffer_arm(probe_buffer_2);
                *r_buf += walk_random_buffer_arm(probe_buffer_3);
            }

            r_buf++;

        }

        FENCE();
        n_flush = 0;
        probe_size *= 2;
    }

    /*report the overhead of measure timestamp*/
    for (int i = 0; i < OVERHEAD_RUNS; i++) {
        *r_buf = measure_overhead[i];
        r_buf++;
    }
    return BENCH_SUCCESS;




}
#endif 

#endif 
