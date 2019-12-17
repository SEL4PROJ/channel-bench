/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */



/*L1 data cache attacks with 2 method*/
/* L1 Data cache (prime + probe) the entire cache
                                (evict + time) some cache patterns */

/*Tromer_OS_10, extracting AES using 65ms measurements*/

/*assuming random plaintext, no collisions in accessing table indexes*/


/*synchronous known-data(plaintext and cypertest) attacks*/
/*online stage:  know plaintest and memory-access side0channel information 
  during the encryption of that plaintext.*/

/*prime + probe, using the pointer chasing*/ 
/*
 -memory initialized into a linked list, randomly permuted 
 -travsersing this list for pirme and probe 
 -using a doubled linked list, traversing forward for priming
backward for probing 
 -stores each obtained sample into the same cache set it has just finished measuring */

#ifdef CONFIG_BENCH_DCACHE_ATTACK 
/*For the first round, each of the table access is independent 
 distinguished up to the size of a cache line*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <platsupport/arch/tsc.h>
#include <channel-bench/bench_common.h>
#include "bench.h"

/*a buffer line layout, less than a cache line*/
typedef struct bl{
    uint64_t time; 
    struct bl *prev; 
    struct bl *next; 
    struct bl *next_set; 
} bl_s;                  

/*prime buffer, aligned to cache set size
 in order to simplify buffer init*/
uint8_t p_buf[L1D_SIZE] __attribute__ ((aligned (L1D_SET_SIZE)));

/*encryption msg*/
uint8_t p_text[AES_MSG_SIZE]; 

/*record prime time after an encryption
 we have AES_MSG_SIZE of the d_time_t in shared frame*/
d_time_t *msg_time; 

/*starting point for prime, traversing forward*/
bl_s *set0_head = NULL; 
/*starting point for probe, traversing backward*/
bl_s *set0_tail = NULL; 

void buffer_init(void) {

    bl_s *p_l = NULL, *l = NULL; 
    bl_s *set_h = NULL, *set_t = NULL, *p_set_h = NULL, *p_set_t = NULL; 
    uint32_t w = 0, bit_mask = 0;

    memset(p_buf, 0, L1D_SIZE); 

    for (int s = 0; s < N_L1D_SETS; s++) {

        p_l = set_h = set_t = NULL; 
        bit_mask = 0; 
        
        while (bit_mask != N_L1D_WAY_BITMASK) {

            /*memory initalized into a double linked list, 
              randomly permuted*/
            w = rand() % N_L1D_WAYS; 
            l = (bl_s *)(p_buf + s * CL_SIZE + w * L1D_SET_SIZE); 

           // printf("buffer_init: s %d w %d l %p \n", s, w, l);

            /*is connected?*/
            if (bit_mask & (1 << w))
                continue;

            l->prev = p_l; 
            if (p_l) 
                p_l->next = l; 

            /*the first line as set head*/
            if (!bit_mask)
                set_h = l; 
            
            bit_mask |= 1 << w;
            if (bit_mask == N_L1D_WAY_BITMASK) 
                set_t = l;

            p_l = l;
    }

        if (!s) {
            /*the starting point for prime + probe*/
            set0_head = set_h; 
            set0_tail = set_t; 
        } else {
           /*reversing connection, see fig 7*/ 
            p_set_h->next_set = set_t; 
            p_set_t->next_set = set_h; 
        }
    
        p_set_h = set_h; 
        p_set_t = set_t;
    }
}

/*read a value from every memory block in buffer*/
void prime (void) {

    bl_s *s, *l; 

    s = set0_head; 

    /*starting from set 0, until the last set*/
    while (s) {

        l = s; 
    
        /*traversing forward for priming*/
        while (l->next) 
            l = l->next; 

        s = l->next_set; 
    }

}

/*probing set by set, record time in the same set*/
void probe (void) {

    bl_s *s, *l; 
    uint64_t start, end; 

    s = set0_tail; 

    while (s) {

        l = s; 
        start = rdtsc_cpuid(); 
        
        /*traversing backward for probing*/

        while (l->prev) 
            l = l->prev; 
        
        end = rdtsc_cpuid(); 
        /*record the time in the set, not polluting cache*/
        l->time = end - start;
        s = l->next_set; 
    }
}


static void encry(void) {

    uint8_t out[AES_MSG_SIZE] = {0}; 

    /*constracting a plain text and trigger an AES encryption*/
    for (int i = 0; i < AES_MSG_SIZE; i++) 
        p_text[i] = rand() % N_PT_B;

    crypto_aes_en(p_text, out); 
}

/*record the time that collected in the cache lines after 
 each probe attack*/
void record(void) {

    uint8_t p; 
    bl_s *l;
    d_time_t *g; 

    for (int i = 0; i < AES_MSG_SIZE; i++) {

        g = msg_time + i; 
       
        /*data format: (plaintext value, set number) */
        /*later on using guessing test for the key values
         not included in the benchmark*/
        p = p_text[i]; 

        l = set0_tail; 

        for (int s = 0; s < N_L1D_SETS; s++) {

            while (l->prev) 
                l = l->prev;

            g->t[p][s] += l->time; 
            l = l->next_set;
        }
    }
}

void record_clear(void) {

    d_time_t *g = msg_time; 

    for (int i = 0; i < AES_MSG_SIZE; i++) {

        memset((void *)g, 0, sizeof (d_time_t));
        g++;
    }
}

seL4_Word dcache_attack(void *vaddr) {

    msg_time = (d_time_t *)vaddr;
    
    /*clearing the record buffer */
    record_clear();

    /*initialize prime + probe buffer*/
    buffer_init(); 

    /*collecting data for N runs*/
    for (int n = 0; n < N_D_TESTS; n++) {

        prime(); 
        encry(); 
        
        probe(); 
        record(); 
    }


    return BENCH_SUCCESS;
}
#endif 
