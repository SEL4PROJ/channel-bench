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
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdlib.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/attribute.h>
#include <sel4platsupport/platsupport.h>
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include "bench.h"
#include <channel-bench/bench_helper.h>
#include "bench_support.h"
#include "mastik_common/low.h"
/*the benchmark env created based on 
  the arguments passed by the root thread*/
bench_env_t setup_env; 

static int (*covert_bench_fun[BENCH_COVERT_FUNS])(bench_env_t *) = {NULL, 
    l1_trojan, l1_spy,  
    NULL, NULL, 
    NULL, NULL, 
    l1_trojan, l1_spy, 
    l1i_trojan, l1i_spy,
    l3_kd_trojan, l3_kd_spy,
    tlb_trojan, tlb_spy,
    btb_trojan, btb_spy,
    l3_trojan_single, l3_spy_single,
    bp_trojan, bp_spy,
    l3_trojan, l3_spy,
    timer_high, timer_low,
};

static int (*flush_bench_fun[BENCH_CACHE_FLUSH_FUNS])(bench_env_t *) = 
{l1_cache_nothing, l1_cache_flush, l1_cache_flush,llc_cache_flush, llc_attack_flush, l1_cache_flush, l1_cache_flush_only, llc_cache_flush_only, bench_idle};  


#ifdef CONFIG_BENCH_SPLASH
static unsigned long (*splash_bench_fun[BENCH_SPLASH_FUNS])(int argc, char*argv[]) = 
{fft_main, cholesky_main, lu_main,
radix_main, barnes_main, fmm_main, 
ocean_main, radiosity_main, raytrace_main,
water_nsquared_main, water_spatial_main, bench_idle};

#ifdef CONFIG_PLAT_IMX6
char *splash_fft_argv[] = {"./FFT", "-m22", "-p1", "-n32768", "-l5"};
#else 
char *splash_fft_argv[] = {"./FFT", "-m22", "-p1", "-n131072", "-l6"};
#endif /*haswell*/

#ifdef CONFIG_PLAT_IMX6
/*ARM cannot run the base tk29, using the tk14*/
char *splash_cholesky_argv[] = {"./CHOLESKY", "-p1", "-B32", "-C1048576", 
    "cholesky_tk29_data" }; 
#else 
char *splash_cholesky_argv[] = {"./CHOLESKY", "-p1", "-B32", "-C8388608", 
    "cholesky_tk29_data"}; 
#endif 

char *splash_lu_argv[] = {"./LU", "-n1024", "-p1", "-b16" };
    

char *splash_radix_argv[] = { "./RADIX", "-p1", "-n13107200", "-r1024", "-m26214400"};

char *splash_barnes_argv[] = {"./BARNES"};

char *splash_fmm_argv[] = {"./FMM", "two_cluster", "plummer", "32768", "1e-6", "1", "5", "0.025"," 0.0", "cost_zones"};

char *splash_ocean_argv[] = {"./OCEAN", "-n514"}; 

char *splash_radiosity_argv[] = {"./RADIOSITY", "-batch", "-largerroom"}; 

char *splash_raytrace_argv[] = {"./RAYTRACE","-m8", "balls4.env"/*"teapot.env"*/};

char *splash_water_nsquared_argv[] = {"./WATER-NSQUARED"}; 

char *splash_water_spatial_argv[] = {"./WATER-SPATIAL"}; 


static splash_para_t splash_bench_parameter[BENCH_SPLASH_FUNS] = {

    {5, splash_fft_argv}, 
    {5, splash_cholesky_argv}, 
    {4, splash_lu_argv}, 
    {5, splash_radix_argv}, 
    {1, splash_barnes_argv}, 
    {10, splash_fmm_argv}, 
    {2,  splash_ocean_argv}, 
    {2, splash_radiosity_argv},
    {3, splash_raytrace_argv}, 
    {1, splash_water_nsquared_argv}, 
    {1, splash_water_spatial_argv}
}; 

#endif /*CONFIG_BENCH_SPLASH*/

/* dummy global for libsel4muslcsys */
// char _cpio_archive[1];



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
    result_ep = bench_env->args->r_ep;  
    
    record_vaddr = (void *)bench_env->args->record_vaddr;  

    ipc_bench(result_ep, ep, test_num, record_vaddr);

    wait_init_msg_from(ep);
    

}
#endif 
 
int run_bench_covert(bench_env_t *bench_env) {
   
    seL4_Word test_num = bench_env->args->test_num;
    unsigned int seed; 

#ifdef CONFIG_ARCH_ARM 
    SEL4BENCH_READ_CCNT(seed);  
#elif defined CONFIG_ARCH_RISCV
    seed = rdtime();
#else 
    seed = rdtscp();
#endif 
    /*run bench*/
    assert(covert_bench_fun[test_num] != NULL); 

    srandom(seed);

    return covert_bench_fun[test_num](bench_env); 
}


int run_bench_cache_flush(bench_env_t *bench_env) {

    int test_num = bench_env->args->test_num; 

    assert(flush_bench_fun[test_num - BENCH_CACHE_FLUSH_FUN_START]); 

    return flush_bench_fun[test_num - BENCH_CACHE_FLUSH_FUN_START](bench_env);
}

#ifdef CONFIG_BENCH_SPLASH

static int run_bench_splash(bench_env_t *bench_env) {

    int test_num = bench_env->args->test_num; 
    ccnt_t overhead; 

    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0); 

    splash_bench_result_t *record_vaddr = 
        (splash_bench_result_t*)bench_env->args->record_vaddr;  

    assert(test_num < BENCH_SPLASH_FUNS); 

    if (test_num == BENCH_SPLASH_IDLE_NUM) 
        return bench_idle(bench_env);  

    /*measuring the overhead: reading the timestamp counter*/
    measure_splash_overhead(&overhead);

#ifdef CONFIG_MANAGER_SPLASH_BENCH_SWITCH
    /*syn with the idle thread */
    {
        seL4_MessageInfo_t info;
        bench_args_t *args = bench_env->args; 

        info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
        seL4_SetMR(0, 0); 
        seL4_Send(args->ep, info);
#ifdef CONFIG_KERNEL_SWITCH_COST_BENCH 
 
        /*just waiting to be interrupted by kernel*/
        newTimeTick();
#else 
        /*waiting for a system tick before starting the test*/
        newTimeSlice();

#endif 
        
    }

#else 
    newTimeTick();

#endif 

#ifdef CONFIG_KERNEL_SWITCH_COST_BENCH 
 
    /* flag of measuring the domain switching cost*/
    seL4_SetMR(100, 0x12345678); 
#endif 
 
#ifdef CONFIG_PLAT_IMX6
    uint64_t start =  seL4_GlobalTimer(); 
    splash_bench_fun[test_num](
            splash_bench_parameter[test_num].argc,
            splash_bench_parameter[test_num].argv
            );
    uint64_t end = seL4_GlobalTimer(); 
    record_vaddr->overall  =  end - start; 
#else 


    uint64_t start = sel4bench_get_cycle_count(); 

    splash_bench_fun[test_num](
            splash_bench_parameter[test_num].argc,
            splash_bench_parameter[test_num].argv
            );
    uint64_t end = sel4bench_get_cycle_count(); 
    record_vaddr->overall = end - start; 

#endif 

    record_vaddr->overhead = overhead; 

    seL4_Send(bench_env->args->r_ep, tag); 

    wait_init_msg_from(bench_env->args->ep);

    return 0;

}
#endif  /*CONFIG_BENCH_SPLASH*/

#ifdef CONFIG_MASTIK_ATTACK
void run_bench_mastik(bench_env_t *bench_env) {

    seL4_CPtr ep, reply_ep; 
    seL4_Word test_num = bench_env->args->test_num;
   
    ep = bench_env->args->ep;
    reply_ep = bench_env->args->r_ep;  


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
int run_bench_func_tests(bench_env_t *bench_env) {
    
    seL4_Word test_num; 
    
    test_num = bench_env->args->test_num;; 
    
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

    bench_init_env(argc, argv, &setup_env); 

#ifdef CONFIG_BENCH_IPC
    run_bench_ipc(&setup_env); 
#endif 
#ifdef CONFIG_BENCH_COVERT
    run_bench_covert(&setup_env); 
#endif

#ifdef CONFIG_BENCH_CACHE_FLUSH
    run_bench_cache_flush(&setup_env);
#endif

#ifdef CONFIG_MASTIK_ATTACK
    run_bench_mastik(&setup_env);
#endif 
#ifdef CONFIG_MANAGER_FUNC_TESTS 
    run_bench_func_tests(&setup_env);
#endif 

#ifdef CONFIG_BENCH_SPLASH 
    run_bench_splash(&setup_env); 
#endif 
    
    /*finished testing, halt*/
    while(1);

    return 0;
}
