/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


/*list of benchmarks
 * L1 Instruction cache   (prime + probe) the entire cache
 * L1 Data cache (prime + probe) the entire cache
                                (evict + time) some cache patterns 
                                * branch target buffer (prime + probe) all entires  
                                * TLB (double page fault, containing the cached kernel entry translation)   require to use invalid TLB entries for kernel address space before switching back to user space, Intel machine (INVLPG) arm (invalidate TLB entry by MVA)
                                *LLC  (flush + reload) does not working while no page duplication enabled 
                                          (prime + probe) on a cache set 

                                          covert channel with uncoloured kernel: using uncoloured kernel to evict some of the cache lines of the receiver, then receiver can monitor the progress changes with signals triggered by sender. such as: walking through root cnode, or interrupt vector table. 
 */

#include <autoconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/attribute.h>
#include "../../bench_common.h"
#include "bench.h"


/* dummy global for libsel4muslcsys */
char _cpio_archive[1];

/*endpoint used for communicate with root task*/
seL4_CPtr endpoint; 

/*wait for init msg from manager*/ 
static 
int wait_init_msg_from(seL4_CPtr endpoint) {

    seL4_Word badge; 
    seL4_MessageInfo_t info; 
    
    info = seL4_Wait(endpoint, &badge); 

    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault); 
    assert(seL4_MessageInfo_get_length(info) == 1); 

    if (seL4_GetMR(0) == BENCH_INIT_MSG) 
        return BENCH_SUCCESS; 
    else 
        return BENCH_FAILURE; 
}


/*wait for shared frame address*/
static inline void *wait_vaddr_from(seL4_CPtr endpoint)
{
    /* wait for a message */
    seL4_Word badge;
    seL4_MessageInfo_t info;

    info = seL4_Wait(endpoint, &badge);

    /* check the label and length*/
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
    assert(seL4_MessageInfo_get_length(info) == 1);

    return (void *)seL4_GetMR(0);
}


/*send benchmark result to the root task*/
static inline void send_result_to(seL4_CPtr endpoint, seL4_Word w) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);

    seL4_SetMR(0, w); 
    seL4_Send(endpoint, info);
}


 void run_bench_single (char **argv) {
    
    void *record_vaddr; 
    seL4_Word result UNUSED; 
    int ret; 
    seL4_CPtr endpoint; 

    /*current format for argument: "name, endpoint slot, xxx"*/

    //printf("side-bench running \n");
    endpoint = (seL4_CPtr)atol(argv[1]);

    /*waiting for init msg*/
    ret = wait_init_msg_from(endpoint); 
    assert(ret == BENCH_SUCCESS); 

    /*waiting for virtual address given by root task*/
    record_vaddr = wait_vaddr_from(endpoint); 
    assert(record_vaddr != NULL); 
    
    //printf("side-bench record vaddr: %p\n", record_vaddr);

#ifdef CONFIG_BENCH_DCACHE_ATTACK 
    crypto_init(); 

    /*passing the record vaddr in */
    result = dcache_attack(record_vaddr); 
    /*return result to root task*/
    send_result_to(endpoint, result); 

#endif 
   
#ifdef CONFIG_BENCH_CACHE_FLUSH 
    /*measuring the cost of flushing caches*/
    result = bench_flush(record_vaddr); 
    /*return result to root task*/
    send_result_to(endpoint, result); 

#endif


}

#ifdef CONFIG_BENCH_IPC
void run_bench_ipc(char **argv) {
    unsigned int test_num; 
    seL4_CPtr ep, result_ep; 
    void *record_vaddr = NULL;

    /*get the test number*/
    test_num = atol(argv[0]); 
    ep = (seL4_CPtr)atol(argv[1]);
    result_ep = (seL4_CPtr)atol(argv[2]);
  
#ifdef CONFIG_BENCH_PMU_COUNTER 
    /*waiting for virtual address given by root task*/
    record_vaddr = wait_vaddr_from(result_ep); 
    assert(record_vaddr != NULL); 

#endif 
    ipc_bench(result_ep, ep, test_num, record_vaddr);

    /*waiting on a endpoit which will never return*/
    int  ret = wait_init_msg_from(ep); 
    //printf("ERROR: IPC benchmark receive unexpected message\n"); 
    assert(ret == BENCH_SUCCESS); 

}
#endif 
int main (int argc, char **argv) {


    /*processing arguments*/
    assert(argc == CONFIG_BENCH_ARGS);


#ifdef CONFIG_BENCH_IPC
    run_bench_ipc(argv); 
#else 
    run_bench_single(argv);
#endif 
        
    /*finished testing, halt*/
    while(1);

    return 0;

}
