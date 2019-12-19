/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <sel4/sel4.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <sel4utils/process.h>
#include <sel4platsupport/io.h>
#include <channel-bench/bench_types.h>
#include "bench_support.h"

#ifdef CONFIG_BENCH_SPLASH_MORECORE 
/*use to create huge mappings and given to cache flush benchmark*/
extern uintptr_t morecore_base;
extern size_t morecore_size;
extern uintptr_t morecore_top; 
#endif


/* allocator */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 20)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

#define ALLOCMAN_VIRTUAL_SIZE BIT(20)

static allocman_t*
init_allocator(simple_t *simple, vka_t *vka) { 
    
    /* set up allocator */
    allocman_t *allocator = bootstrap_new_1level_simple(simple, 
            CONFIG_SEL4UTILS_CSPACE_SIZE_BITS, ALLOCATOR_STATIC_POOL_SIZE,
            allocator_mem_pool);
    if (!allocator) 
        return NULL; 
    
    /* create vka backed by allocator */
    allocman_make_vka(vka, allocator);
    return allocator;
}


static void
init_allocator_vspace(allocman_t *allocator, vspace_t *vspace)
{
    int error;
    /* Create virtual pool */
    reservation_t pool_reservation;
    void *vaddr;

    pool_reservation.res = allocman_mspace_alloc(allocator, 
            sizeof(sel4utils_res_t), &error);
    assert(error == 0); 

    error = sel4utils_reserve_range_no_alloc(vspace, 
            pool_reservation.res,
            ALLOCMAN_VIRTUAL_SIZE, 
            seL4_AllRights, 1, &vaddr);
    assert(error == 0); 

    bootstrap_configure_virtual_pool(allocator, vaddr, 
            ALLOCMAN_VIRTUAL_SIZE, SEL4UTILS_PD_SLOT);
}

static size_t
add_frames(void *frames[], size_t start, uintptr_t addr, size_t num_frames)
{
    size_t i;

    if (!addr)
        return start; 

    for (i = start; i < (num_frames + start); i++) {
        frames[i] = (void *) addr;
        addr += SIZE_BITS_TO_BYTES(seL4_PageBits);
    }

    return i;
}

static void
init_vspace(bench_env_t *env) {

    bench_args_t *args = env->args; 
    void *existing_frames;
    size_t pages = 1;  /*one page containing the arguments*/ 
    int index;
    int error; 
    
    /*FIXME: currently does not support adding huge page into the vspace*/
    assert(args->hugepage_vaddr == 0); 
   
    pages += BYTES_TO_SIZE_BITS_PAGES(sizeof(seL4_IPCBuffer), seL4_PageBits);

    if (args->shared_vaddr) 
        pages += args->shared_pages; 

    if (args->record_vaddr)
        pages += args->record_pages; 
   
    pages += args->stack_pages;  

    existing_frames = malloc(sizeof (void*) * pages); 
    assert(existing_frames); 
    memset(existing_frames, 0, sizeof (void *) * pages); 

    index = add_frames(existing_frames, 0, 
            args->shared_vaddr, args->shared_pages);
    
    index = add_frames(existing_frames, index, 
            args->record_vaddr, args->record_pages);

    index = add_frames(existing_frames, index, 
            args->stack_vaddr, args->stack_pages);

    index = add_frames(existing_frames, index, 
            (uintptr_t)seL4_GetIPCBuffer(), 
            BYTES_TO_SIZE_BITS_PAGES(sizeof(seL4_IPCBuffer), seL4_PageBits));

    index = add_frames(existing_frames, index, (uintptr_t)args, 1);


    error = sel4utils_bootstrap_vspace(&env->vspace, &env->data, 
                SEL4UTILS_PD_SLOT, &env->vka, NULL, NULL, existing_frames); 
    assert(error == 0); 

    free(existing_frames); 
}


static int get_untyped_count(void *data)
{
    return CONFIG_BENCH_UNTYPE_COUNT;
}

static uint8_t get_cnode_size(void *data)
{
    return CONFIG_SEL4UTILS_CSPACE_SIZE_BITS;
}

static seL4_CPtr get_nth_untyped(void *data, 
        int n, size_t *size_bits, 
        uintptr_t *paddr, bool *device)
{
    
    bench_env_t *env = data;

    if (n >= CONFIG_BENCH_UNTYPE_COUNT) {
        return seL4_CapNull;
    }

    if (size_bits) {
        *size_bits = CONFIG_BENCH_UNTYPE_SIZEBITS; 
    }

    if (device) {
        *device = false;
    }

    if (paddr) {
        *paddr = 0;
    }
    return env->args->untyped_cptr[n];
}


static int get_cap_count(void *data) {
    /* Our parent only tells us our first free slot, 
       so we have to work out from that how many
       capabilities we actually have. 
       We assume we have cspace layout from sel4utils_cspace_layout
       and so we have 1 empty null slot, 
       and if we're not on the RT kernel two more unused slots */

    int last = ((bench_env_t *) data)->args->first_free;
    /* skip the null slot */
    last--;

    if (config_set(CONFIG_ARCH_X86_64)) {
        /* skip the ASID pool slot */
        last--;
    }

    if (!config_set(CONFIG_KERNEL_RT)) {
        /* skip the 2 RT only slots */
        last -= 2;
    }

    return last;
}

static seL4_CPtr get_nth_cap(void *data, int n)
{
    if (n < get_cap_count(data)) {
        /* need to turn the contiguous index n 
           into a potentially non contiguous set of
           cptrs, in accordance with the holes discussed in get_cap_count. */
        /* first convert our index that starts at 0, 
           to one starting at 1, this removes the
           the hole of the null slot */
        n++;

        /* introduce a 1 cptr hole if we on x86_64, as it does not have an ASID pool */
        if (config_set(CONFIG_ARCH_X86_64) && n >= SEL4UTILS_ASID_POOL_SLOT) {
            n++;
        }
        /* now introduce a 2 cptr hole if we're not on the RT kernel */
        if (!config_set(CONFIG_KERNEL_RT) && n >= SEL4UTILS_SCHED_CONTEXT_SLOT) {
            n += 2;
        }
        return n;
    }
    return seL4_CapNull;
}

    
static seL4_Error
get_irq(void *data, int irq, seL4_CNode cnode, seL4_Word index, uint8_t depth)
{
    bench_env_t *env = data;
    seL4_CPtr cap = sel4platsupport_timer_objs_get_irq_cap(&env->args->to, irq, PS_INTERRUPT);
    cspacepath_t path;
    vka_cspace_make_path(&env->vka, cap, &path);
    seL4_Error error = seL4_CNode_Move(cnode, index, depth,
            path.root, path.capPtr, path.capDepth);
    assert(error == seL4_NoError); 

    return seL4_NoError;
}


static void init_simple(bench_env_t *env)
{
    env->simple.data = env;

    //env->simple.frame_cap =
    //env->simple.frame_mapping =
    //env->simple.frame_info =
    //env->simple.ASID_assign =

    env->simple.cap_count = get_cap_count;
    env->simple.nth_cap = get_nth_cap;
    env->simple.init_cap = sel4utils_process_init_cap;
    env->simple.cnode_size = get_cnode_size;
    env->simple.untyped_count = get_untyped_count;
    env->simple.nth_untyped = get_nth_untyped;
    //env->simple.core_count = get_core_count;
    //env->simple.sched_ctrl = sched_ctrl;
    //env->simple.userimage_count =
    //env->simple.nth_userimage =
    //env->simple.print =
    //env->simple.arch_info =
    //env->simple.extended_bootinfo =

    env->simple.arch_simple.data = env;
    env->simple.arch_simple.irq = get_irq;
    benchmark_arch_get_simple(&env->simple.arch_simple);
}

int timing_benchmark_init_timer(bench_env_t *env)
{   
    
    ps_io_ops_t ops;

    if (env->timer_initialised)
        return BENCH_SUCCESS;

    /* set up irq caps */ 
    int error = sel4platsupport_init_timer_irqs(&env->vka, 
            &env->simple, env->ntfn.cptr,
            &env->timer, &env->args->to);
    if (error) 
        return error; 

    error = sel4platsupport_new_io_mapper(&env->vspace, 
            &env->vka, &ops.io_mapper);
    if (error) 
        return error; 

    error = sel4platsupport_new_malloc_ops(&ops.malloc_ops);
    if (error) 
        return error; 

    benchmark_arch_get_timers(env, ops);
    env->timer_initialised = true;
 
    return BENCH_SUCCESS;
}   



/*parse the benchmarking arguements, and init the benchmarking enviornment*/
void bench_init_env(int argc, char **argv, 
        bench_env_t *env) {
    
    int error; 
    bench_args_t *args; 

    /*the virtual address contained the argument, passed by the root task*/
    env->args = (void *)atol(argv[0]);
    args = env->args; 


#ifdef CONFIG_BENCH_SPLASH_MORECORE
    if (args->morecore_size) {
        /*only the splash bench has more core area, not the idle thread*/
        morecore_base = args->morecore_vaddr;
        morecore_size = args->morecore_size; 
        morecore_top = args->morecore_vaddr + args->morecore_size; 
    }
#endif    
    init_simple(env);

    /*no untypes, do not create the vspace*/
    if (args->untype_none) 
        return; 

    env->allocman = init_allocator(&env->simple, &env->vka); 
    assert(env->allocman); 

    if (args->timer_enabled) {
        
        error = allocman_add_untypeds_from_timer_objects(env->allocman, &args->to);
        assert(error == 0);
    }

    init_vspace(env); 
    
    init_allocator_vspace(env->allocman, &env->vspace);

    /* In case we used any FPU during our setup we will attempt to put the system
     * back into a steady state before returning */
#ifdef CONFIG_X86_FPU_MAX_RESTORES_SINCE_SWITCH
    for (int i = 0; i < CONFIG_X86_FPU_MAX_RESTORES_SINCE_SWITCH; i++) {
        seL4_Yield();
    }
#endif
    /* allocate a notification for timers */                                       
    if (args->timer_enabled) {
        error = vka_alloc_notification(&env->vka, &env->ntfn);  
        assert(error == 0);
    }
}
