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
#include "../../bench_common.h"
#include "manager.h"

/*system resources*/
static m_env_t env; 


/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE ((1 << seL4_PageBits) * 100)

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 50)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/*static memory for virtual memory bootstrapping*/
static sel4utils_alloc_data_t vdata; 


/*copy a cap to a thread, returning the cptr in its cspace*/
static inline 
seL4_CPtr copy_cap_to(sel4utils_process_t *pro, seL4_CPtr cap) {

    seL4_CPtr copy; 
    cspacepath_t path; 

    vka_cspace_make_path(&env.vka, cap, &path); 
    copy = sel4utils_copy_cap_to_process(pro, path); 
    assert(copy != 0); 
    
    return copy;

}

/*map the N frame caps to a process*/
static inline 
void *map_frames_to(sel4utils_process_t *pro, seL4_CPtr *caps, int n) {


    void *vaddr = vspace_map_pages(&pro->vspace, 
            caps, NULL, seL4_AllRights, n, PAGE_BITS_4K, 1);

    assert(vaddr); 

    return vaddr; 
}


/*send msg to a process via ep*/
/*FIXME: currently support single word msg only*/
static inline 
void send_msg_to(seL4_CPtr ep, seL4_Word w) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    
    seL4_SetMR(0, w); 
    seL4_Send(ep, info);
}

/*wait msg from an endpoint, return msg length*/

static inline 
seL4_Word wait_msg_from(seL4_CPtr ep) {

    seL4_Word badge;
    seL4_MessageInfo_t info = seL4_Wait(ep, &badge);
    int result = seL4_GetMR(0);

    if (seL4_MessageInfo_get_label(info) != seL4_NoFault) {
        sel4utils_print_fault_message(info, CONFIG_BENCH_THREAD_NAME);
        result = BENCH_FAILURE;
    }

    return result;
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
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&env->vspace, &vdata, simple_get_pd(&env->simple), &env->vka, seL4_GetBootInfo()); 
    assert(error == BENCH_SUCCESS); 

    /*fill allocator with virtual memory*/ 
    v_reserve = vspace_reserve_range(&env->vspace, ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr); 
    assert(v_reserve.res != NULL); 
    
    bootstrap_configure_virtual_pool(allocman, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&env->simple)); 


}

#ifdef CONFIG_CACHE_COLOURING
/*init run time environment for cache colouring*/
static void init_env_colour (m_env_t *env) {

    /*allocator, vka, utils...*/
    color_allocator_t *init_allocator; 
    color_allocator_t *color_allocator[CC_NUM_DOMAINS]; 
    reservation_t v_reserve; 
    uint32_t div[CC_NUM_DOMAINS] = {CC_DIV, CC_DIV}; 
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
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&env->vspace, &vdata, simple_get_pd(&env->simple), &env->vka, seL4_GetBootInfo()); 
    assert(error == BENCH_SUCCESS); 

#if 0
    /*config malloc to use virtual memory*/
    sel4utils_reserve_range_no_alloc(&vspace, &muslc_brk_reservation_memory,
            BRK_VIRTUAL_SIZE, seL4_AllRights, 1, &muslc_brk_reservation_start); 

    muslc_this_vspace = &vspace; 
    muslc_brk_reservation = &muslc_brk_reservation_memory; 
#endif
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

#ifdef CONFIG_MANAGER_CACHE_FLUSH
static void lanuch_bench_single (void *arg) {

    /*lanuch single benchmark application*/

    cspacepath_t src, dest; 
    int ret; 
    void *r_cp, *vaddr; 
    seL4_Word bench_result;

    /*current format for argument: "name, endpoint slot, xxx"*/
    char *arg0 = CONFIG_BENCH_THREAD_NAME;
    char arg1[CONFIG_BENCH_MAX_UNIT] = {0};
    char arg2[CONFIG_BENCH_MAX_UNIT] = {0}; 
    char *argv[CONFIG_BENCH_ARGS] = {arg0, arg1, arg2};


    printf("\n"); 
    printf("Side channel benchmarks\n");
    printf("========================\n");

    /*create frames that act as record buffer, mapping 
     to benchmark processes*/
    env.record_vaddr = vspace_new_pages(&env.vspace, seL4_AllRights, 
            CONFIG_BENCH_RECORD_PAGES, PAGE_BITS_4K); 
    assert(env.record_vaddr != NULL); 

    /*copy the caps to map into the remote process*/
    for (int i = 0; i < CONFIG_BENCH_RECORD_PAGES; i++) {
	
	vaddr = env.record_vaddr + i * PAGE_SIZE_4K; 
        vka_cspace_make_path(&env.vka, vspace_get_cap(&env.vspace, 
                    vaddr), &src); 
        ret = vka_cspace_alloc(&env.vka, env.record_frames + i); 
        assert(ret == 0); 

        vka_cspace_make_path(&env.vka, env.record_frames[i], &dest); 
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
        assert(ret == 0); 

    }
    

    /*configure benchmark thread*/ 
    printf("Config benchmark thread...\n");
    ret = sel4utils_configure_process(&env.bench_thread, &env.vka, 
        &env.vspace, CONFIG_BENCH_PRIORITY, 
        CONFIG_BENCH_THREAD_NAME);
    assert(ret == 0); 


    /*copy the fault endpoint to the benchmark thread*/
    env.bench_ep = copy_cap_to(&env.bench_thread, 
            env.bench_thread.fault_endpoint.cptr); 

    /*set up args for the benchmark process
     benchmark thread is resumed afterwards.*/ 
    snprintf(arg1, CONFIG_BENCH_MAX_UNIT, "%d", env.bench_ep);

    /*entry point, system V ABI compliant*/
    ret = sel4utils_spawn_process_v(&env.bench_thread, &env.vka, 
            &env.vspace, CONFIG_BENCH_ARGS, argv, 1); 
    assert(ret == 0); 

    printf("sending init msg...\n"); 

    /*send an init msg*/ 
    send_msg_to(env.bench_thread.fault_endpoint.cptr, BENCH_INIT_MSG); 

    /*map the frames for data recording into benchmark pro*/
    r_cp = map_frames_to(&env.bench_thread, env.record_frames, 
            CONFIG_BENCH_RECORD_PAGES); 

    printf("frames map to benchmark, manager: %p, side-bench: %p\n", 
           env.record_vaddr, r_cp );

    printf("sending record vaddr... \n");
    send_msg_to(env.bench_thread.fault_endpoint.cptr, (seL4_Word)r_cp); 

    printf("waiting on results... \n");
    /*waiting on benchmark to finish*/
    bench_result = wait_msg_from(env.bench_thread.fault_endpoint.cptr); 
    

    assert(bench_result != BENCH_FAILURE); 

    /*processing record*/
    bench_process_data(&env, bench_result); 


}
#endif 

#ifdef CONFIG_MANAGER_PMU_COUNTER
static void init_pmu_counters(void) {

    uint32_t num_counter; 
    uint32_t bit_counter = BENCH_PMU_BITS;


    num_counter =  sel4bench_get_num_counters(); 
    printf("\n num of pmu counter is %d \n", num_counter); 
   
    /*set up the pmu counter events*/
    sel4bench_set_count_event(0, SEL4BENCH_EVENT_TLB_L1I_MISS); 
   // sel4bench_set_count_event(0, SEL4BENCH_EVENT_EXECUTE_INSTRUCTION); 
    sel4bench_set_count_event(1, SEL4BENCH_EVENT_TLB_L1D_MISS); 
// sel4bench_set_count_event(2, SEL4BENCH_EVENT_BRANCH_MISPREDICT); 
   // sel4bench_set_count_event(3, SEL4BENCH_EVENT_CACHE_L1I_MISS); 
  //  sel4bench_set_count_event(4, SEL4BENCH_EVENT_CACHE_L1D_MISS); 
    //sel4bench_set_count_event(5, SEL4BENCH_EVENT_EXECUTE_INSTRUCTION); 
    
    /*stall due to instruction micro tlb miss*/ 
    sel4bench_set_count_event(2, 0x84); 
    
    /*stall due to data micro tlb miss*/ 
    sel4bench_set_count_event(3, 0x85); 
    /*stall due to main tlb miss*/ 
    sel4bench_set_count_event(4, 0x62);
    sel4bench_start_counters(bit_counter); 
    /*start the pmu counter*/ 
}


#endif 
static void *main_continued (void* arg) {

       /*init the benchmakring functions*/
    sel4bench_init(); 
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    init_pmu_counters(); 
#endif 
    
#ifdef CONFIG_MANAGER_IPC
    lanuch_bench_ipc(&env); 
#endif 
#ifdef CONFIG_MANAGER_CACHE_FLUSH 
    lanuch_bench_single(arg);
#endif 
    /*halt cpu*/
    printf("Finished benchmark, now halting...\n"); 

    /*NOTE: using while loop, as debug feature is disabled.*/
    while (1);
   
    return NULL;
}

#ifdef CONFIG_CACHE_COLOURING
/*creak kernel images*/
static void create_kernel_pd(seL4_BootInfo *info, m_env_t *env) {

    for (int i = 0; i < CC_NUM_DOMAINS; i++) {

        create_colored_kernel_image(info, &(env->vka_colour[i]), 
                &(env->kernel_colour[i])); 

    }

    /*the default kernel image created at bootup*/ 
    env->kernel = seL4_CapKernel; 
}
#endif

int main (void) {

    seL4_BootInfo *info = seL4_GetBootInfo(); 
    
#ifdef CONFIG_KERNEL_STABLE 
    simple_stable_init_bootinfo(&env.simple, info); 
#else 
    simple_default_init_bootinfo(&env.simple, info); 
#endif

#ifdef CONFIG_CACHE_COLOURING
    /*allocator, vka, and vspace*/
    init_env_colour(&env);
    create_kernel_pd(info, &env); 
#else 
    init_env(&env); 
#endif

    /*enable serial driver*/
    platsupport_serial_setup_simple(NULL, &env.simple, &env.vka); 

    /*switch to a bigger, safer stack with guard page*/ 

    printf("Manager, switching to a safer, bigger stack... "); 
    fflush(stdout); 


    /*starting test, never return from this function*/
    sel4utils_run_on_stack(&env.vspace, main_continued, NULL);

    return 0;

}

