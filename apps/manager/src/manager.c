/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*Kconfig variables*/
#include <autoconf.h>
#include <manager/gen_config.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h> 

#include <allocman/bootstrap.h>
#include <allocman/vka.h> 

#include <sel4utils/vspace.h>
#include <sel4utils/stack.h>
#include <sel4utils/process.h>
#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/timer.h>
#include <simple/simple.h> 
#include <sel4bench/sel4bench.h>

#ifdef CONFIG_KERNEL_STABLE 
#include <simple-stable/simple-stable.h>
#else 
#include <simple-default/simple-default.h>
#endif 

#include <utils/util.h>

#include <vka/object.h>
#include <vka/capops.h>

#include <vspace/vspace.h>
#include <channel-bench/bench_common.h>
#include "manager.h"

#ifdef CONFIG_LIB_SEL4_CACHECOLOURING 
#include <cachecoloring/color_allocator.h>
#endif 

/*system resources*/
static m_env_t env; 

#ifdef CONFIG_MANAGER_HUGE_PAGES
/*use to create huge mappings and given to cache flush benchmark*/
extern char *morecore_area;
extern size_t morecore_size;
char manager_morecore_area[MANAGER_MORECORE_SIZE]; 
#endif

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (1024*1024*10)

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (1024 * 1024 * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/*static memory for virtual memory bootstrapping*/
static sel4utils_alloc_data_t vdata; 

/*copy a cap to a thread, returning the cptr in its cspace*/
static inline 
seL4_CPtr copy_cap_to(sel4utils_process_t *pro, seL4_CPtr cap) {

    seL4_CPtr copy; 

    copy = sel4utils_copy_cap_to_process(pro, &env.vka, cap); 
    assert(copy != 0); 
    
    return copy;
}



/*init run time environment*/
void init_env (m_env_t *env) {

    allocman_t *allocman = NULL; 
    int error = 0; 
    void *vaddr; 
    reservation_t v_reserve; 

    /*create an allocator*/
    allocman = bootstrap_use_current_simple(&env->simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool); 
    assert(allocman != NULL);

    /*connect virtual interface with allocator*/
    allocman_make_vka(&env->vka, allocman); 

    /*create a vspace*/
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&env->vspace, &vdata, simple_get_pd(&env->simple), &env->vka, platsupport_get_bootinfo()); 
    assert(error == BENCH_SUCCESS); 

    /*fill allocator with virtual memory*/ 
    v_reserve = vspace_reserve_range(&env->vspace, ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr); 
    assert(v_reserve.res != NULL); 

    bootstrap_configure_virtual_pool(allocman, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&env->simple)); 
}

#ifdef CONFIG_LIB_SEL4_CACHECOLOURING
/*init run time environment for cache colouring*/
static void init_env_colour(m_env_t *env) {

    /*allocator, vka, utils...*/
    color_allocator_t *init_allocator; 
    color_allocator_t *color_allocator[CC_NUM_DOMAINS]; 
    reservation_t v_reserve; 
#ifdef CONFIG_MANAGER_CACHE_DIV_UNEVEN
    size_t div[CC_NUM_DOMAINS] = {CC_BIG, CC_LITTLE}; 
#else  
    size_t div[CC_NUM_DOMAINS] = {CC_DIV, CC_DIV}; 
#endif 
    int error = 0; 
    void *vaddr; 

    /*create the init allocator*/
    init_allocator = color_create_init_allocator_use_simple(
            &env->simple, 
            ALLOCATOR_STATIC_POOL_SIZE, 
            allocator_mem_pool, CC_NUM_DOMAINS, div); 
    assert(init_allocator); 

    /*abstrating allocator*/
    color_make_vka(&env->vka, init_allocator); 

    /*create a vspace*/
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&env->vspace, &vdata, simple_get_pd(&env->simple), &env->vka, platsupport_get_bootinfo()); 
    assert(error == BENCH_SUCCESS); 

    /*fill allocator with virtual memory*/ 
    v_reserve = vspace_reserve_range(&env->vspace, ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr); 
    assert(v_reserve.res != NULL); 

    color_config_init_allocator(init_allocator, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&env->simple)); 

    /*setup the colored allocators*/
    for (int i = 0; i < CC_NUM_DOMAINS; i++) {

        color_allocator[i] = color_create_domain_allocator(init_allocator, i); 
        assert(color_allocator[i]);
        color_make_vka(&env->vka_colour[i], color_allocator[i]); 
    }

}
#endif 

#ifdef CONFIG_MANAGER_PMU_COUNTER
static void init_pmu_counters(void) {

    uint32_t bit_counter = BENCH_PMU_BITS;

#ifdef CONFIG_ARCH_X86 
    {
        uint32_t eax, ebx, ecx, edx; 

        sel4bench_private_cpuid(IA32_CPUID_LEAF_PMC, 0, &eax, &ebx, &ecx,
                &edx);
        printf("the cpuid for pmu leaf 0xa vaule eax  0x%x  ebx 0x%x edx 0x%x\n",
                eax, ebx, edx);

        sel4bench_private_cpuid(IA32_CPUID_LEAF_MODEL, 0, &eax, &ebx, 
                &ecx, &edx); 
        printf("cpu family and model value 0x%x\n", eax); 

    }
#endif 

    /*set up the pmu counter events*/

    /*hikey platform*/
    /*Level 1 instruction cache refill*/
    //sel4bench_set_count_event(0, 0x1);
    /*sabre platform */
    /*instruction cache dependent stall cycles*/

    //sel4bench_set_count_event(0, 0x60);

#if 0
    /*tried on sabre but no corelation */
    /*L1 I cache refill*/ 
    sel4bench_set_count_event(0, 0x1);
    /*L1I_CACHE acess*/
    sel4bench_set_count_event(0, 0x14);
    /*L1 I tlb refill*/
    sel4bench_set_count_event(0, 0x2);
#endif 

#if 0
    /*sandy bridge*/
    //sel4bench_set_count_event(0, 0x3024);  /*L2_RQSTS.ALL_CODE_RD*/

    /*event that is currently used for the paper*/
    sel4bench_set_count_event(0, 0x0280); /*ICACHE.MISSES*/

    /*tried events for sandybridge*/
    sel4bench_set_count_event(0, 0x8024); /*L2_RQSTS.PF_MISS*/
    sel4bench_set_count_event(1, 0x0149); /*DTLB_STORE_MISSES.MISS_CA
                                            USES_A_WALK*/

    sel4bench_set_count_event(2, 0x0149); /*DTLB_STORE_MISSES.MISS_CA
                                            USES_A_WALK*/

    sel4bench_set_count_event(3, 0x02c1); /*OTHER_ASSISTS.ITLB_MISS_R
                                            ETIRED*/


    sel4bench_set_count_event(1, 0x0108); /*DTLB_LOAD_MISSES.MISS_CAUS
                                            ES_A_WALK*/

    sel4bench_set_count_event(0, SEL4BENCH_EVENT_EXECUTE_INSTRUCTION);
    sel4bench_set_count_event(0, 0x412e); /*LONGEST_LAT_CACHE.MISS*/

    sel4bench_set_count_event(0, 0x2024); /*L2_RQSTS.CODE_RD_MISS*/


    sel4bench_set_count_event(0, 0x0185); /*ITLB_MISSES.MISS_CAUSES_A_
                                            WALK*/

#endif 

    /*haswell*/
#if 0
     sel4bench_set_count_event(0, 0x0280); /*ICACHE.MISSES*/
     sel4bench_set_count_event(1, 0x0185); /*ITLB_MISSES.MISS_CAUSES_A_
                                            WALK*/

    sel4bench_set_count_event(2, 0x0149); /*DTLB_STORE_MISSES.MISS_CAU
                                           SES_A_WALK*/
    sel4bench_set_count_event(3, 0x01ae); /*ITLB.ITLB_FLUSH*/

    sel4bench_set_count_event(2, 0x00c5); /*BR_MISP_RETIRED.ALL_BRANC
                                            HES*/

    sel4bench_set_count_event(0, 0x6085); /*ITLB_MISSES.STLB_HIT*/
#endif 

#if 1
    sel4bench_set_count_event(0, 0x0149); /*DTLB_STORE_MISSES.MISS_CAU
                                           SES_A_WALK*/
    sel4bench_set_count_event(1, 0x6008);  /*DTLB_LOAD_MISSES.STLB_HIT*/
    sel4bench_set_count_event(2, 0x0185); /*ITLB_MISSES.MISS_CAUSES_A_
                                            WALK*/

    sel4bench_set_count_event(3, 0x8008); /*DTLB_LOAD_MISSES.PDE_CACH
                                            E_MISS*/
    sel4bench_set_count_event(4, 0x1008); /*DTLB_LOAD_MISSES.WALK_DUR
                                           ATION*/
    sel4bench_set_count_event(5, 0x1085);  /*ITLB_MISSES.WALK_DURATION*/
 
 
#endif 

    /*skylake*/
    // sel4bench_set_count_event(0, 0x0283); /*ICACHE_64B.IFTAG_MISS*/

    //     sel4bench_set_count_event(0, 0xe424); /*L2_RQSTS.ALL_CODE_RD*/


#if 0
    /*the following event have numbers */
    sel4bench_set_count_event(0, 0x4424); /*L2_RQSTS.CODE_RD_HIT*/
    sel4bench_set_count_event(2, 0x412e); /*LONGEST_LAT_CACHE.MISS*/
#endif 

#if 0
    /****the followings events are not related, numbers do not change**/
    sel4bench_set_count_event(1, 0x0e08);  /*DTLB_LOAD_MISSES.WALK_COM
                                             PLETED*/ 
    sel4bench_set_count_event(0, 0x0185); /*ITLB_MISSES.MISS_CAUSES_A_
                                            WALK*/

    sel4bench_set_count_event(2, 0x2008);  /*DTLB_LOAD_MISSES.STLB_HIT*/
    /*L2 cache requests*/
    sel4bench_set_count_event(3, 0x2424);  /*L2_RQSTS.CODE_RD_MISS*/
    sel4bench_set_count_event(4, 0x3824);  /*L2_RQSTS.PF_MISS*/ 

    sel4bench_set_count_event(1, 0x3f24); /*L2_RQSTS.MISS*/

    sel4bench_set_count_event(3, 0xd824); /*L2_RQSTS.PF_HIT*/
    sel4bench_set_count_event(0, 0xf824); /*L2_RQSTS.ALL_PF*/
    sel4bench_set_count_event(1, 0x4f2e); /*LONGEST_LAT_CACHE.REFEREN
                                            CE*/


    sel4bench_set_count_event(0, 0x2085); /*ITLB_MISSES.STLB_HIT*/
    sel4bench_set_count_event(1, 0x00c5); /*BR_MISP_RETIRED.ALL_BRANC
                                            HES*/
    sel4bench_set_count_event(0, 0x0108); /*DTLB_LOAD_MISSES.MISS_CAUS*/
    sel4bench_set_count_event(0, 0x20c5); /*BR_MISP_RETIRED.ALL_BRANCHES*/


    /**********************/

    /*haswell number does not change, not related*/
    //    sel4bench_set_count_event(0, 0xf824); /*L2_RQSTS.ALL_PF*/
    //   sel4bench_set_count_event(0, 0x3024); /*L2_RQSTS.L2_PF_MISS*/


    sel4bench_set_count_event(0, 0x5024); /*L2_RQSTS.L2_PF_HIT*/
    sel4bench_set_count_event(0, 0x08f0); /*L2_TRANS.ALL_PF*/
#endif 

    /*start the pmu counter*/ 

    sel4bench_start_counters(bit_counter); 
    sel4bench_reset_counters();
}
#endif


static void *main_continued (void* arg) {
    
    int error = 0; 

    printf("Done\n"); 

    /*understanding the timer, creating a IRQ cap*/
    error = sel4platsupport_init_default_timer_caps(&env.vka, &env.vspace, &env.simple, &env.to);
    assert(error == 0); 
    
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    init_pmu_counters(); 
#endif 

#if defined( CONFIG_MANAGER_CACHE_FLUSH) || defined (CONFIG_MANAGER_IPC) 
    launch_bench_flush(&env);
#endif

#ifdef CONFIG_MANAGER_COVERT_BENCH
    launch_bench_covert(&env); 
#endif 

#ifdef CONFIG_MANAGER_FUNC_TESTS 
    launch_bench_func_test(&env); 
#endif 

#ifdef CONFIG_MANAGER_SPLASH_BENCH 
    launch_bench_splash(&env);
#endif 

    /*halt cpu*/
    printf("Finished benchmark, now halting...\n"); 
    /*NOTE: using while loop, as debug feature is disabled.*/
    while (1);
    
    return NULL;
}

#ifdef CONFIG_MULTI_KERNEL_IMAGES 


static int destroy_ki(m_env_t *env, bench_ki_t *kimage) {
    cspacepath_t src;
    int ret; 
    ccnt_t overhead, start, end;
    measure_overhead(&overhead);



    vka_cspace_make_path(&env->vka, kimage->ki.cptr, &src);  
  
    /*revoke first*/ 
    ret = vka_cnode_revoke(&src); 
    if (ret) 
        return ret; 

    start = sel4bench_get_cycle_count(); 
    ret =   vka_cnode_delete(&src); 
    end = sel4bench_get_cycle_count(); 

    printf("cost of the system call on kernel deletion:\n");
    printf(" "CCNT_FORMAT" \n", (end - start) - overhead); 

    return ret; 

 
}

/*creak kernel images*/
static void create_kernel_pd(m_env_t *env) {

    seL4_CPtr ik_image; 
    seL4_Word k_size = env->bootinfo->kernelImageSize;
    int ret;

    ik_image = simple_get_ik_image(&env->simple); 
    printf("ik image cap is %zu size %zu \n", ik_image, k_size);

    for (int i = 0; i < MAN_KIMAGES; i++ )  {
        printf("creating ki %d\n", i);
#ifdef CONFIG_LIB_SEL4_CACHECOLOURING
        ret = create_ki(env, env->vka_colour + i, env->kimages + i); 
#else 
        ret = create_ki(env, &env->vka, env->kimages + i);
#endif
        assert(ret == BENCH_SUCCESS); 

    }
    printf("done creating ki\n");

    /*the default kernel image created at bootup*/ 
    env->kernel = ik_image; 
}
#endif

int main (void) {

    seL4_BootInfo *info = platsupport_get_bootinfo(); 
    int err;

#ifdef CONFIG_MANAGER_HUGE_PAGES
    /*init the more core area for the libc to use*/
    morecore_area = manager_morecore_area; 
    morecore_size = MANAGER_MORECORE_SIZE; 
#endif 

#ifdef CONFIG_KERNEL_STABLE 
    simple_stable_init_bootinfo(&env.simple, info); 
#else 
    simple_default_init_bootinfo(&env.simple, info); 
#endif
    env.bootinfo = info; 
    
    /*init the benchmarking functions*/
    sel4bench_init();


#ifdef CONFIG_LIB_SEL4_CACHECOLOURING
    /*allocator, vka, and vspace*/
    printf("Init env colour\n");
    init_env_colour(&env);
#else 
    printf("Init env\n");
    init_env(&env); 
#endif

    /*enable serial driver*/
    err = platsupport_serial_setup_simple(&env.vspace, &env.simple, &env.vka); 
    assert(err == 0);

#ifdef CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER
    
    seL4_CPtr kernel_log_cap; 
    /*init the kernel log buffer, only read at user side*/
    env.kernel_log_vaddr = vspace_new_pages(&env.vspace, 
            seL4_CanRead,
            1, seL4_LargePageBits);
    assert(env.kernel_log_vaddr); 

    kernel_log_cap = vspace_get_cap(&env.vspace, env.kernel_log_vaddr);
    assert(kernel_log_cap != seL4_CapNull); 

    error = seL4_BenchmarkSetLogBuffer(kernel_log_cap);
    assert(error == seL4_NoError); 

#endif 

#ifdef CONFIG_MULTI_KERNEL_IMAGES
    create_kernel_pd(&env); 
#endif

    /*switch to a bigger, safer stack with guard page*/ 
    printf("Manager, switching to a safer, bigger stack... "); 
    fflush(stdout); 

    /*starting test, never return from this function*/
    sel4utils_run_on_stack(&env.vspace, main_continued, NULL, NULL);

    return 0;
}

