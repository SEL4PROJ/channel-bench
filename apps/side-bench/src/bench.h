/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _BENCH_H_
#define _BENCH_H_


#include "../../bench_common.h"

/*data record structure*/
typedef uint64_t l1d_time[N_P_LINES][N_SETS] l1d_time_t;


/*init crypto services*/
void crypto_init(void);

/*AES encryption*/ 
void crypto_aes_en(uint8_t *in, uint8_t *out); 

/*list of aviliable benchmarks*/

/*L1 data cache prime + probe attack */
seL4_Word dcache_attack(void *record_vaddr); 


#endif 


