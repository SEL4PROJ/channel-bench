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
#include <sel4platsupport/platsupport.h>
#include "../../bench_common.h"
#include "bench.h"

bench_covert_t covert_env; 
#ifdef CONFIG_BENCH_CACHE_FLUSH
extern char *morecore_area;
extern size_t morecore_size;
#endif 
static int (*covert_bench_fun[BENCH_COVERT_FUNS])(bench_covert_t *) = {NULL, 
    NULL, NULL,  
    NULL, NULL, 
    NULL, NULL, 
    l1_trojan, l1_spy, 
    l1i_trojan, l1i_spy,
    l3_trojan, l3_spy, 
    l3_kd_trojan, l3_kd_spy,
    tlb_trojan, tlb_spy,
};

/* dummy global for libsel4muslcsys */
char _cpio_archive[1];

/*endpoint used for communicate with root task*/
seL4_CPtr endpoint; 

/*wait for init msg from manager*/ 
static 
int wait_init_msg_from(seL4_CPtr endpoint) {

    seL4_Word badge; 
    seL4_MessageInfo_t info; 
    
    info = seL4_Recv(endpoint, &badge); 

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

    info = seL4_Recv(endpoint, &badge);

    /* check the label and length*/
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
    assert(seL4_MessageInfo_get_length(info) == 1);

    return (void *)seL4_GetMR(0);
}

/*wait for shared frame address*/
static inline seL4_CPtr wait_ep_from(seL4_CPtr endpoint)
{
    /* wait for a message */
    seL4_Word badge;
    seL4_MessageInfo_t info;

    info = seL4_Recv(endpoint, &badge);

    /* check the label and length*/
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
    assert(seL4_MessageInfo_get_length(info) == 1);

    return (seL4_CPtr)seL4_GetMR(0);
}

/*send benchmark result to the root task*/
static inline void send_result_to(seL4_CPtr endpoint, seL4_Word w) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);

    seL4_SetMR(0, w); 
    seL4_Send(endpoint, info);
}


/*benchmarking cache flush costs*/
void run_bench_single (char **argv) {
    
    void *record_vaddr; 
    seL4_Word result UNUSED; 
    int ret; 
    seL4_CPtr endpoint; 

#ifdef CONFIG_DEBUG_BUILD
    platsupport_serial_setup_simple(NULL, NULL, NULL); 
#endif 

    /*current format for argument: "name, endpoint slot, xxx"*/

    //printf("side-bench running \n");
    endpoint = (seL4_CPtr)atol(argv[1]);

    /*waiting for init msg*/
    ret = wait_init_msg_from(endpoint); 
    assert(ret == BENCH_SUCCESS); 

    /*waiting for virtual address given by root task*/
    record_vaddr = wait_vaddr_from(endpoint); 
    assert(record_vaddr != NULL); 
    
#ifdef CONFIG_DEBUG_BUILD
    printf("side-bench starting, record vaddr: %p\n", record_vaddr);
#endif 

#ifdef CONFIG_BENCH_DCACHE_ATTACK 
    crypto_init(); 

    /*passing the record vaddr in */
    result = dcache_attack(record_vaddr); 
    /*return result to root task*/
    send_result_to(endpoint, result); 

#endif 
   
#ifdef CONFIG_BENCH_CACHE_FLUSH 

    /*init the more core space 64M to be used as the probe buffer*/
    morecore_area = wait_vaddr_from(endpoint); 
    assert(morecore_area != NULL); 
    morecore_size = 16 * 6 * 1024 * 1024; 

#ifdef CONFIG_DEBUG_BUILD
    printf("more core area in side bench %p \n", morecore_area);
#endif

    /*measuring the cost of flushing caches*/
    result = bench_flush(endpoint, record_vaddr); 
    /*return result to root task*/
    send_result_to(endpoint, result); 

#endif


}

#ifdef CONFIG_BENCH_IPC
void run_bench_ipc(char **argv) {
    unsigned int test_num; 
    seL4_CPtr ep, result_ep, null_ep; 
    void *record_vaddr = NULL;
    /*get the test number*/
    test_num = atol(argv[0]); 
    ep = (seL4_CPtr)atol(argv[1]);
    record_vaddr = (void *)atol(argv[2]);

    assert(record_vaddr != NULL); 
//    null_ep = wait_ep_from(result_ep); 

    ipc_bench(0, ep, test_num, record_vaddr);

    /*waiting on a endpoit which will never return*/
    wait_init_msg_from(ep);
    

}
#endif 
#ifdef CONFIG_BENCH_COVERT_SINGLE
 
int run_bench_covert(char **argv) {
    seL4_Word badge; 
    seL4_MessageInfo_t info; 
    

    covert_env.opt = atol(argv[0]); 
    covert_env.syn_ep = (seL4_CPtr)atol(argv[1]);
    covert_env.r_ep = (seL4_CPtr)atol(argv[2]);
    
#ifdef CONFIG_DEBUG_BUILD
    platsupport_serial_setup_simple(NULL, NULL, NULL); 
#endif 
    info = seL4_Recv(covert_env.r_ep, &badge); 

    if (seL4_MessageInfo_get_label(info) != seL4_NoFault)
        return BENCH_FAILURE; 

    if (seL4_MessageInfo_get_length(info) != BENCH_COVERT_MSG_LEN)
        return BENCH_FAILURE; 
   

    covert_env.p_buf = (void *)seL4_GetMR(0);
    covert_env.ts_buf = (void *)seL4_GetMR(1); 
    covert_env.notification_ep = (seL4_CPtr)seL4_GetMR(2); 
    
    /*run bench*/
    assert(covert_bench_fun[covert_env.opt] != NULL); 
    return covert_bench_fun[covert_env.opt](&covert_env); 

}
#endif
#ifdef CONFIG_MASTIK_ATTACK
 
void run_bench_mastik(char **argv) {

    unsigned int test_num; 
    seL4_CPtr ep, reply_ep; 
    /*get the test number*/
    test_num = atol(argv[0]); 
    ep = (seL4_CPtr)atol(argv[1]);
    reply_ep = (seL4_CPtr)atol(argv[2]);

    /*enable serial driver*/
    platsupport_serial_setup_simple(NULL, NULL, NULL); 

   /*the covert channel multicore*/
    if (test_num == BENCH_MASTIK_TEST) 
        mastik_test(0, NULL); 
   
    if (test_num == BENCH_MASTIK_VICTIM) 
        mastik_victim(0, NULL); 

    /*the side channel multicore*/ 
    if(test_num == BENCH_MPI_VICTIM) 
        mpi_victim();

    if (test_num == BENCH_MASTIK_SPY) 
        mastik_spy(reply_ep, NULL); 

    /*waiting on a endpoit which will never return*/
    wait_init_msg_from(ep);

}

#endif
int main (int argc, char **argv) {


    /*processing arguments*/
    assert(argc == CONFIG_BENCH_ARGS);


#ifdef CONFIG_BENCH_IPC
    run_bench_ipc(argv); 
#endif 
#ifdef CONFIG_BENCH_COVERT_SINGLE
    run_bench_covert(argv); 
#endif

#ifdef CONFIG_BENCH_CACHE_FLUSH 
    run_bench_single(argv);
#endif 
#ifdef CONFIG_MASTIK_ATTACK
    run_bench_mastik(argv);
#endif 

    /*finished testing, halt*/
    while(1);

    return 0;

}
