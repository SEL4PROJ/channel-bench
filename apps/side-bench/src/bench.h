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


#include <sel4/sel4.h>


/*init crypto services*/
void crypto_init(void);

/*AES encryption*/ 
void crypto_aes_en(uint8_t *in, uint8_t *out); 

/*list of aviliable benchmarks*/

/*L1 data cache prime + probe attack */
seL4_Word dcache_attack(void *record_vaddr); 
/*cache flushing benchmark*/
seL4_Word bench_flush(void *record_vaddr);

/*ipc benchmark*/ 
seL4_Word ipc_bench(seL4_CPtr result_ep, seL4_CPtr test_ep, int test_n);

#endif 


