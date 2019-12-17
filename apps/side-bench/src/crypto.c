/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifdef CONFIG_BENCH_DCACHE_ATTACK 
#include <stdlib.h>
#include <channel-bench/bench_common.h>
#include "bench.h"
#include "aes/aes.h"

/*This file contains the crypto services for AES, etc.*/

/*AES key*/
#ifdef AES_KEY_ALL_ZERO 
static uint8_t aes_key[AES_KEY_SIZE] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
#else 
static uint8_t aes_key[AES_KEY_SIZE] = {0x43, 0x74, 0x84, 0xa9, 0x89, 0xb1, 0x62, 0xa7, 0xbb, 0x7f, 0xd0, 0x3d, 0x9e, 0xec, 0x3d, 0x35};
#endif

/*expended aes key after initialization*/
static AES_KEY aes_e_key;



/*providing aes encryption*/
void crypto_aes_en (uint8_t *in, uint8_t *out) {

    AES_encrypt(in, out, &aes_e_key); 

}

/*initialize crypto services that are required in 
 the following attacks*/
void crypto_init(void) {


    /*TODO: currently include the AES service in the same process
     current version is OPEN SSL 0.9.8 */

     /*set AES key */
    AES_set_encrypt_key(aes_key, AES_BITS, &aes_e_key);


}
#endif 
