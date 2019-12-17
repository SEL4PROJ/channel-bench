/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
/*This file contains functions for processing benchmark data.*/

#include <autoconf.h>
#include <manager/gen_config.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4bench/sel4bench.h>
#include "manager.h"

#ifdef CONFIG_MANAGER_DCACHE_ATTACK

/*the average probing time for each cache set
 currently only for the byte 0*/
static uint64_t prob_avg[N_L1D_SETS]; 

/*printing out the prime+probe attack on L1 D cache*/
static void print_dcache_attack(m_env_t *env) {

    d_time_t *time_total = (d_time_t *)env->record_vaddr; 
    
    printf("PRIME DATA START\n");

    /*note: focusing on P0 at this stage*/
    for (int p = 0; p < N_PT_B ; p++) {

        for (int s = 0; s < N_L1D_SETS; s++) {
          
            prob_avg[s] += time_total->t[p][s];
            /*data format: plaintext value, set number, time*/
            printf(" %d %d %llu \n", p, s, time_total->t[p][s]); 

        }
    }

    printf("PRIME DATA END\n");

    /*calculating the average for each cache set*/
    for (int s = 0; s < N_L1D_SETS; s++ ) 
        prob_avg[s] /= N_PT_B; 

    printf("PRIME AVG START\n"); 

    /*print out the data after substracting with the average*/
    for (int p = 0; p < N_PT_B; p++) {

        for (int s = 0; s < N_L1D_SETS; s++) 
            printf(" %d %d %lld \n", p, s, 
                    time_total->t[p][s] - prob_avg[s]);

    }
    printf("PRIME AVG END\n");

}

#endif 
/*analysing benchmark record in shared frames*/
void bench_process_data(m_env_t *env, seL4_Word result) {


    printf("benchmark result: 0x%zu \n", result); 

    printf("analysing data in vaddr %p\n", env->record_vaddr);

#ifdef CONFIG_MANAGER_DCACHE_ATTACK
    print_dcache_attack(env);
#endif 
}
