/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */




/*The common header file shared by benchmark application 
 and root task (manager process). Mainly containing: 
 data format and hardware configurations for testing.*/


#ifndef _BENCH_COMMON_H 
#define _BENCH_COMMON_H 

/*elf file name of benchmark thread*/ 
#define CONFIG_BENCH_THREAD_NAME    "side-bench"

/*num of arguments passed to benchmark tests*/
#define CONFIG_BENCH_ARGS    3

/*num of digits used for maximum unsigned int*/
#define CONFIG_BENCH_MAX_UINT   10 

/*number of shared frames for benchmark recording*/

/*TODO: currently only suits the L1 data benchmark
 which is 16(msg length) * 256 (maximum 8 bits integer) 
 * 64 (number of cache sets) * 8 (timing measurement) = 0x200000 */
#define CONFIG_BENCH_RECORD_PAGES 0x200 


/*cache line size*/
#define CL_SIZE    64 
/**************************************************/
/*prime + probe attack on L1 D cache, in dcache.c*/
/*AES attack, synchrous*/

/*L1 d cache size 32K*/
#define L1D_SIZE  0x8000

/*number of l1 d cache sets: 64*/
#define N_L1D_SETS   64
/*number of l1 d cache ways*/
#define N_L1D_WAYS   8 
/*size of an entire L1 D set: 4096 */
#define L1D_SET_SIZE  0x1000
/*AES msg size, 128 bits 
 key size 128 bits*/
#define AES_MSG_SIZE  16
#define AES_KEY_SIZE  16
/*aes service size 128 bits block */
#define AES_BITS      128

/*number of possible plaintext values in each byte*/
#define N_PT_B 256   


/*data maxtix for probing result (x,y) = (Plain text 
  value, set number)*/
typedef uint64_t p_time[N_PT_B][N_SETS] d_time_t;


/****************************************************/

#endif 



