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


#define SAMPLE_PREFIX 50
#define SAMPLE_LEN 1000

#define L3_SINGLE_TROJAN_RANGE 2048
#define TROJAN_BUFFER_SIZE  (2048 * 32)

#ifdef CONFIG_MANAGER_HUGE_PAGES
extern char *morecore_area;
extern size_t morecore_size;
#endif

/*assuming 2048 sets, one line each*/
char *trojan_buf;


static void trojan_access( uint32_t secret) { 

    for (int i = 0; i < secret; i++) 
        access(trojan_buf + i * L1_CACHELINE);

}

#ifdef CONFIG_MANAGER_HUGE_PAGES
static void update_morecore_area(void *vaddr, size_t size) {

    assert(morecore_area == NULL); 
    assert(vaddr);
    morecore_area = vaddr;
    morecore_size = size;

}
#endif



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
    uint32_t volatile *share_vaddr = args->shared_vaddr; 
    uint32_t volatile *syn_vaddr = share_vaddr + 1;
    *share_vaddr = 0; 

#ifdef CONFIG_MANAGER_HUGE_PAGES  
    void *huge_vaddr = (void*)seL4_GetMR(2);
    update_morecore_area(huge_vaddr);
#endif

    uint16_t *results = malloc(sizeof (uint16_t) * nsets); 
    assert(results);

    trojan_buf = (char *)malloc(TROJAN_BUFFER_SIZE); 
    assert(trojan_buf);

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);


    /*ready to do the test*/
    seL4_Send(args->ep, info);


    secret = 0; 
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 

        while (*syn_vaddr != TROJAN_SYN_FLAG) {
            ;
        }
        FENCE();

       //secret = random() % (nsets + 1); 
       secret = random() % (L3_SINGLE_TROJAN_RANGE + 1); 

#if 0 
        l3_unmonitorall(l3);
        for (int s = 0; s < secret; s++) {
            l3_monitor(l3, s);
        }
#endif
        /*do simple probe*/
        //l3_probe(l3, results); 
        trojan_access(secret);
        /*update the secret read by low*/ 
        *share_vaddr = secret; 

        *syn_vaddr = SPY_SYN_FLAG;

    }

    FENCE(); 

    while (1);
    
    return 0;
}


int l3_spy_single(bench_env_t *env) {
    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args;  
      
    struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr; 
    uint32_t volatile *secret = args->shared_vaddr; 
    uint32_t volatile *syn = secret + 1;
#ifdef CONFIG_MANAGER_HUGE_PAGES 
    update_morecore_area(args->hugepage_vaddr, args->hugepage_size);
#endif
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

    /*unmonitor any possible set that can be used for syn, 
     16 sets for the orginial seL4, 8 sets for the uncoloured*/
    for (int i = 0; i < l3->ngroups; i++ ) 
        l3_unmonitor(l3, i * l3->groupsize); 

    l3_probe(l3, results);
    l3_probe(l3, results);

    *syn = TROJAN_SYN_FLAG;

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 

        while (*syn != SPY_SYN_FLAG) 
        {
            ;
        }

        FENCE(); 
        /*reset the counter to zero*/
        //sel4bench_reset_cycle_count();
        /*do simple probe*/
        //l3_probecount_simple(l3, results); 
        l3_probe(l3, results);
        r_addr->result[i] = 0; 
        for (int s = 0; s < nsets; s++) {
            r_addr->result[i] += results[s];
        }

        /*result is the total probing cost
          secret is updated by trojan in the previous system tick*/
        r_addr->sec[i] = *secret; 
        /*spy set the flag*/
        *syn = TROJAN_SYN_FLAG; 
    }
    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
} 

