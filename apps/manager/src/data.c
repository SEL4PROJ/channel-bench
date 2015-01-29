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
#include <stdio.h>


#include "manager.h"

/*printing out the prime+probe attack on L1 D cache*/
static void print_dcache_attack(m_env_t *env) {

    d_time_t *time_total = (d_time_t *)env->record_vaddr; 

    for (int i = 0; i < AES_MSG_SIZE; i++) {
       
        printf("P%d    \n", i); 
        
        for (int p = 0; p < N_PT_B ; p++) {
        
            for (int s = 0; s < N_L1D_SETS; s++) {
                /*data format: plaintext value, set number, time*/
                printf("%d %d %llu \n", p, s, time_total[p][s]); 

            }
        }
    
        time_total++;
    }
}




/*analysing benchmark record in shared frames*/
void bench_process_data(m_env_t *env, seL4_Word result) {


    printf("benchmark result: 0x%x \n", result); 

    printf{"analysing data in vaddr 0x%x\n", env->record_vaddr};
   
    print_dcache_attack(env); 
}
