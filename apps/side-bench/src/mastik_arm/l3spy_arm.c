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
#include "../../../bench_common.h"
#include "../mastik_common/low.h"
#include "l3_arm.h"

#define SAMPLE_PREFIX 50
#define SAMPLE_LEN 1000

 

int l3_trojan_single(bench_covert_t *env) {

    uint32_t secret;
    seL4_Word badge;
    seL4_MessageInfo_t info;
    
    /*creat the probing buffer*/
    l3pp_t l3 = l3_prepare();
    assert(l3); 

    int nsets = l3_getSets(l3);

#ifdef CONFIG_DEBUG_BUILD
    printf("trojan: Got %d sets\n", nsets);
#endif

    uint16_t *results = malloc(sizeof (uint16_t) * nsets); 
    assert(results);

    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    /*receive the shared address to record the secret*/
    uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(0);
    uint32_t volatile *syn_vaddr = share_vaddr + 4;
    *share_vaddr = 0; 

    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(env->r_ep, info);


    /*ready to do the test*/
    seL4_Send(env->syn_ep, info);


    secret = 0; 
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 

        while (*syn_vaddr != TROJAN_SYN_FLAG) {
            ;
        }
        FENCE();

#ifndef CONFIG_BENCH_DATA_SEQUENTIAL 
        secret = random() % (nsets + 1); 
#endif
        l3_unmonitorall(l3);
 
        for (int s = 0; s < secret; s++) {
            l3_monitor(l3, s);
        }
        /*do simple probe*/
        l3_probe(l3, results); 

        //branch_probe(secret);
        /*update the secret read by low*/ 
        *share_vaddr = secret; 
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
        if (++secret == nsets + 1)
            secret = 0; 
#endif 
        /*wait until spy set the flag*/
        *syn_vaddr = SPY_SYN_FLAG;

    }

    FENCE(); 

    while (1);
    
    return 0;
}


int l3_spy_single(bench_covert_t *env) {
    seL4_Word badge;
    seL4_MessageInfo_t info;
    
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

    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    /*the record address*/
    struct bench_l1 *r_addr = (struct bench_l1 *)seL4_GetMR(0);
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)seL4_GetMR(1);
    uint32_t volatile *syn = secret + 4;
    *syn = TROJAN_SYN_FLAG;

    /*syn with trojan*/
    info = seL4_Recv(env->syn_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {

        FENCE(); 

        while (*syn != SPY_SYN_FLAG) 
        {
            ;
        }

        FENCE(); 
        /*reset the counter to zero*/
        sel4bench_reset_cycle_count();
        /*do simple probe*/
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
    info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(env->r_ep, info);

    while (1);

    return 0;
} 

void l3_spy_multicore(seL4_CPtr ep, char *p) {


    l3pp_t l3 = l3_prepare();

    int nsets = l3_getSets(l3);
#ifdef CONFIG_DEBUG_BUILD
    printf("Got %d sets\n", nsets);
#endif 
    uint16_t res[SAMPLE_LEN + SAMPLE_PREFIX];
    char str[SAMPLE_LEN + 1];
    const char mark[]=".@@@@@@@@@@@@@@@@";
    /*2048 sets in total*/
    int data[2048];

    str[SAMPLE_LEN] = '\0';


    for (int l = 0; l < 4; l++) {
        for (int s = 0; s < nsets; s++) {
            l3_unmonitorall(l3);
            l3_monitor(l3, s);
            probeloop(l3, res, SAMPLE_LEN+SAMPLE_PREFIX, 10000);
            data[s] = 0;

            /*counting the probs that detect cache misses on this cache set*/
            for (int i = 0; i < SAMPLE_LEN; i++)
                if (res[SAMPLE_PREFIX + i] != 0)
                    data[s]++;

#ifdef CONFIG_DEBUG_BUILD
            if (s %128 == 0)
                printf("# at %d\n", s);
#endif 
        }
#ifdef CONFIG_DEBUG_BUILD
        for (int i = 0; i < nsets; i++) 
            print("%d\n", data[i]);
#endif 
        printf("\n");
    }





#if 0
#define SET 20
    for (int i = 0; i < nsets; i+= 128)
        l3_monitor(l3, i + SET);

    char *dummy = allocate(4096, 4096);
    uint16_t *res = calloc(nsets/128 * 100, sizeof(uint16_t));
    uint16_t *tmp = calloc(nsets/128, sizeof(uint16_t));

    l3_probe(l3, res);
    for (int i = 0; i < 100; ) {
        if (i % 25 < 5)
            dummy[SET*32]++;
        l3_probecount(l3, res + i * nsets/128);
        i++;
        /*
           if (i % 25 < 5)
           dummy[35*32]++;
        //for (int j = 0; j < 4096; j++)
        //	dummy[j]++;
        l3_bprobecount(l3, res + i * nsets/128);
        i++;
         */
    }
    for (int i = 0; i < 100; i++) {
        print("%3d:", i);
        for (int j = 0; j < nsets/128; j++) 
            print(" %3d", res[i * nsets/128 + j]);
        print("\n");
    }
#endif


#if 0

    char *buf = al_alloc(16*4096*16*2, 4096);
    //char *buf = malloc(16*4096*16*2);
    //char *buf = mmap(0, 4096*128,PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    print("Mapped at %p\n",  buf);

    uint32_t start=0, end=1, last = 0, sum = 0;

    for (int i = 1; i < 512; i++) {
        int s = 0;
        for (int k = 0; k < 128; k++)  {
            start = sel4bench_get_cycle_count();
            for (int j = 0; j < i; j++)
                s += buf[4096*j];
            end = sel4bench_get_cycle_count();
            a+=s;
            sum += end - start;
        }

        print("%d: %u cycles, %d\n", i, sum/128, sum/128 - last);
        last = sum/128;
        sum = 0;
    }
#endif
}
