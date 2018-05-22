/*this file contains the covert channel attack using the LLC on sabre*/
/*cache line size 32 bytes, direct mapped to 16 way associativity, way size 64 KB, replacement policy round-robin, total 16 colours , 1MB. 2048 sets.*/

/*the attack currently only works for single core: 
 on single core: sender probes 0-2048 sets, receiver probes 2048 sets*/

#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "bench_common.h"
#include "bench_types.h"
#include "bench_helper.h"
#include "low.h"
#include "l3_arm.h"



int l3_trojan_single(bench_env_t *env) {

    uint32_t secret;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 

    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);

#ifdef CONFIG_DEBUG_BUILD
    printf("trojan: Got %d sets\n", nsets);
#endif

    /*receive the shared address to record the secret*/
    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 

    uint16_t *results = malloc(sizeof (uint16_t) * nsets); 
    assert(results);

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*ready to do the test*/
    seL4_Send(args->ep, info);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();

        secret = random() % (nsets + 1); 

        l3_unmonitorall(l3);

        /*l3_monitor already touch the cache lines*/
        for (int s = 0; s < secret; s++) {
            l3_monitor(l3, s);
        }
        /*update the secret read by low*/ 
        *share_vaddr = secret; 

    }

    while (1);
    
    return 0;
}


int l3_spy_single(bench_env_t *env) {
    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args;  
      
    struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr; 
    
    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);

#ifdef CONFIG_DEBUG_BUILD
    printf("spy: Got %d sets\n", nsets);
#endif

    uint16_t *results = malloc(sizeof (uint16_t) * nsets); 
    assert(results);

    l3_unmonitorall(l3);

    /*monitor all sets*/
    for (int s = 0; s < nsets; s++) {
        l3_monitor(l3, s);
    }

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 
        newTimeSlice();
        
        l3_probe(l3, results);
        r_addr->result[i] = 0; 
        for (int s = 0; s < nsets; s++) {
            r_addr->result[i] += results[s];
        }

        /*result is the total probing cost
          secret is updated by trojan in the previous system tick*/
        r_addr->sec[i] = *secret; 
    }
    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
} 

