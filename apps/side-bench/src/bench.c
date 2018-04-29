/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include <autoconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/attribute.h>
#include <sel4platsupport/platsupport.h>
#include "../../bench_common.h"
#include "../../bench_types.h"
#include "bench.h"
#include "bench_support.h"

/*the benchmark env created based on 
  the arguments passed by the root thread*/
bench_env_t bench_env; 

static int (*covert_bench_fun[BENCH_COVERT_FUNS])(bench_env_t *) = {NULL, 
    l1_trojan, l1_spy,  
    NULL, NULL, 
    NULL, NULL, 
    l1_trojan, l1_spy, 
    l1i_trojan, l1i_spy,
    l3_trojan, l3_spy, 
    l3_kd_trojan, l3_kd_spy,
    tlb_trojan, tlb_spy,
    btb_trojan, btb_spy,
    l3_trojan_single, l3_spy_single,
    bp_trojan, bp_spy,
    timer_high, timer_low,
};

static int (*flush_bench_fun[BENCH_CACHE_FLUSH_FUNS])(bench_env_t *) = 
{l1_cache_flush, NULL,llc_cache_flush, bench_idle};  

/* dummy global for libsel4muslcsys */
char _cpio_archive[1];


/*wait for init msg from manager*/ 
static int wait_init_msg_from(seL4_CPtr endpoint) {

    seL4_Word badge; 
    seL4_MessageInfo_t info; 
    
    info = seL4_Recv(endpoint, &badge); 

    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault); 
    assert(seL4_MessageInfo_get_length(info) == 1); 

    if (seL4_GetMR(0) == BENCH_INIT_MSG) 
        return BENCH_SUCCESS; 
    else 
        return BENCH_FAILURE; 
}


/*send benchmark result to the root task*/
static inline void send_result_to(seL4_CPtr endpoint, seL4_Word w) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);

    seL4_SetMR(0, w); 
    seL4_Send(endpoint, info);
}


#ifdef CONFIG_BENCH_DCACHE_ATTACK 
/*benchmarking cache flush costs*/
void run_bench_single (bench_env_t *bench_env) {
    
    bench_args_t *args = bench_env->args; 
    seL4_Word result UNUSED; 
    int ret; 
    seL4_CPtr endpoint = args->r_ep; 
    void *record_vaddr = args->record_vaddr; 
   
    crypto_init(); 

    /*passing the record vaddr in */
    result = dcache_attack(record_vaddr); 
    /*return result to root task*/
    send_result_to(endpoint, result); 


}
#endif 

#ifdef CONFIG_BENCH_IPC
void run_bench_ipc(bench_env_t *bench_env) {
    seL4_Word test_num; 
    seL4_CPtr ep, result_ep; 
    void *record_vaddr = NULL;
    
    /*get the test number*/
    test_num = bench_env->args->test_num;; 
    ep = bench_env->args->ep;
    result_ep = env->args->r_ep;  
    
    record_vaddr = env->args->record_vaddr;  

    ipc_bench(reply_ep, ep, test_num, record_vaddr);

    /*waiting on a endpoit which will never return*/
    wait_init_msg_from(ep);
    

}
#endif 
 
int run_bench_covert(bench_env_t *bench_env) {
   
    seL4_Word test_num = bench_env->args->test_num;

    /*run bench*/
    assert(covert_bench_fun[test_num] != NULL); 
    return covert_bench_fun[test_num](bench_env); 
}


int run_bench_cache_flush(bench_env_t *bench_env) {

    int test_num = bench_env->args->test_num; 

    assert(flush_bench_fun[test_num - BENCH_CACHE_FLUSH_FUN_START]); 

    return flush_bench_fun[test_num - BENCH_CACHE_FLUSH_FUN_START](bench_env);

}


#ifdef CONFIG_MASTIK_ATTACK
void run_bench_mastik(bench_args_t *bench_evn) {

    seL4_Word test_num; 
    seL4_CPtr ep, reply_ep; 
    
    test_num = bench_env->args->test_num;; 
    ep = bench_env->args->ep;
    reply_ep = env->args->r_ep;  


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

#ifdef CONFIG_MANAGER_FUNC_TESTS
int run_bench_func_tests(benchmark_args_t *bench_env) {
    
    seL4_Word test_num; 
    seL4_CPtr ep, reply_ep; 
    
    test_num = bench_env->args->test_num;; 
    ep = bench_env->args->ep;
    reply_ep = env->args->r_ep;  

    
    /*run bench*/
    if (test_num == BENCH_FUNC_RECEIVER)
        funcs_receiver(bench_env); 
        
    if (test_num == BENCH_FUNC_SENDER)
        funcs_sender(bench_env);
   
    return 0;
}
#endif

int main (int argc, char **argv) {

#ifdef CONFIG_DEBUG_BUILD
    platsupport_serial_setup_simple(NULL, NULL, NULL); 
#endif 
    assert(argc == CONFIG_BENCH_ARGS);

    bench_init_env(argc, argv, &bench_env); 

#ifdef CONFIG_BENCH_IPC
    run_bench_ipc(&bench_env); 
#endif 
#ifdef CONFIG_BENCH_COVERT_SINGLE
    run_bench_covert(&bench_env); 
#endif

#ifdef CONFIG_BENCH_CACHE_FLUSH
    run_bench_cache_flush(&bench_env);
#endif

#ifdef CONFIG_MASTIK_ATTACK
    run_bench_mastik(&bench_env);
#endif 
#ifdef CONFIG_MANAGER_FUNC_TESTS 
    run_bench_func_tests(&bench_env);
#endif 

    /*finished testing, halt*/
    while(1);

    return 0;
}
