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
#include <channel-bench/bench_types.h>

/*init crypto services*/
void crypto_init(void);

/*AES encryption*/ 
void crypto_aes_en(uint8_t *in, uint8_t *out); 

/*list of aviliable benchmarks*/

/*L1 data cache prime + probe attack */
seL4_Word dcache_attack(void *record_vaddr); 
/*cache flushing benchmark*/
seL4_Word bench_flush(seL4_CPtr result_ep, void *record_vaddr);
/*ipc benchmark*/ 
seL4_Word ipc_bench(seL4_CPtr result_ep, seL4_CPtr test_ep, int test_n,
        void *record_vaddr);

/*the covert benchmarking interfaces*/
int mastik_test(int ac, char **av); 
 
int mastik_victim(int ac, char **av);
void mpi_victim(void);
int mastik_spy(seL4_CPtr ep, char** av); 

int l3_kd_trojan(bench_env_t *);
int l3_kd_spy(bench_env_t *);
int l1_trojan(bench_env_t *env); 
int l1_spy(bench_env_t *env); 
int l1i_trojan(bench_env_t *env); 
int l1i_spy(bench_env_t *env);
int l3_trojan(bench_env_t *env); 
int l3_spy(bench_env_t *env); 
int tlb_trojan(bench_env_t *env); 
int tlb_spy(bench_env_t *env); 
int btb_trojan(bench_env_t *env); 
int btb_spy(bench_env_t *env); 
int l3_trojan_single(bench_env_t *env); 
int l3_spy_single(bench_env_t *env); 
int bp_trojan(bench_env_t *env); 
int bp_spy(bench_env_t *env); 
int timer_high(bench_env_t *env); 
int timer_low(bench_env_t *env); 

int l1_cache_nothing(bench_env_t *env);
int l1_cache_flush(bench_env_t *env);
int bench_idle(bench_env_t *env); 
int llc_cache_flush(bench_env_t *env);
int llc_attack_flush(bench_env_t *env);
int l1_cache_flush_only(bench_env_t *env);
int llc_cache_flush_only(bench_env_t *env);

/*the function correctness test */
int funcs_receiver(bench_env_t *env); 
int funcs_sender(bench_env_t *env);


/*the function used for the spalsh bench*/
unsigned long fft_main(int argc, char *argv[]); 
unsigned long  cholesky_main(int argc, char *argv[]); 
unsigned long lu_main(int argc, char *argv[]); 
unsigned long radix_main(int argc, char *argv[]); 
unsigned long barnes_main(int argc, char *argv[]); 
unsigned long fmm_main(int argc, char *argv[]); 
unsigned long ocean_main(int argc, char *argv[]); 
unsigned long radiosity_main(int argc, char *argv[]); 
unsigned long raytrace_main(int argc, char *argv[]); 
unsigned long water_nsquared_main(int argc, char *argv[]); 
unsigned long water_spatial_main(int argc, char *argv[]); 

#endif 


