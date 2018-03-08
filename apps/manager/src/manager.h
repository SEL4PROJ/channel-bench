/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#ifndef __MANAGER_H 
#define __MANAGER_H 


#include <autoconf.h>
#include <sel4/bootinfo.h>

#include <vka/vka.h>
#include <sel4utils/elf.h>
#include <sel4utils/process.h>

#include <simple/simple.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <sel4platsupport/timer.h>

/*common definitions*/
#include "bench_common.h"
#include "bench_types.h"

#define MANAGER_MORECORE_SIZE  (16 * 1024 * 1024)

#define MAN_KIMAGES   CC_NUM_DOMAINS

/*the kernel image used for benchmarking*/
typedef struct bench_ki {

    vka_object_t ki; 
    vka_object_t *kmems;
    seL4_Word k_size;

} bench_ki_t; 


typedef struct {
    vka_t vka;    
    vka_t *ipc_vka;    /*ep allocator*/
    vspace_t vspace; 
    vka_t vka_colour[CC_NUM_DOMAINS]; 
    bench_ki_t kimages[MAN_KIMAGES];
    seL4_CPtr kernel; 
    /*the boot info*/
    seL4_BootInfo *bootinfo; 

    /*abstracts over kernel version and boot envior*/
    simple_t simple; 

    /*virtual address for recording data, used by root*/ 
    void *record_vaddr; 
    
    /*virtual address for the huge frames, used by root thread*/ 
    void *huge_vaddr; 
    
    /*timer object sharing with the benchmark thread*/
    timer_objects_t to;

    /*FIXME: delete all of these */
    /*benchmark thread structure*/
    sel4utils_process_t bench_thread;
 
    /*endpoint that root thread is waiting on*/ 
    seL4_CPtr bench_ep;
} m_env_t;  /*environment used by the manager*/  

/*parameters for setting up a benchmark process*/ 
typedef struct bench_thread {

    seL4_CPtr kernel;              /*kernel image*/
    vka_object_t ep;               /*communication between trojan & spy*/ 
    vka_object_t reply_ep;         /*reply manager*/
    vka_object_t notification_ep;  /*using for seL4_Poll*/       
   
    vka_t *vka;              /*kernel object allocator*/
    vka_t *ipc_vka;          /*used for endpoint*/
    vka_t *root_vka;         /*vka used by root task*/ 

    vspace_t *vspace;        /*virtual address space of the root task*/
    char *image;            /*binary name of benchmarking elf file*/
    seL4_Word prio;         /*priority of benchmarking thread*/
    sel4utils_process_t process;  /*internal process context*/ 
    seL4_Word test_num;      /*test number*/
 
    /*the affinity of this thread*/ 
    seL4_Word affinity;

    /*untypes allocated to benchmarking thread*/
    vka_object_t bench_untypes[CONFIG_BENCH_UNTYPE_COUNT];
    
    /*the timer object shared with the root task, allocated with rook_vka*/
    timer_objects_t *to;

    /*the page containing benchmark args */
   /*vadd in root task, allocated with the kernel object allocator*/ 
    bench_args_t *bench_args;  
    char *name;    /*name of this thread*/ 

} bench_thread_t; 

#ifdef CONFIG_MULTI_KERNEL_IMAGES
/*creating a kernel image object*/
static int create_ki(m_env_t *env, vka_t *vka, bench_ki_t *kimage) {

    vka_object_t *ki = &kimage->ki;
    seL4_Word k_size = env->bootinfo->kernelImageSize;
    seL4_CPtr *kmem_caps; 
    seL4_MessageInfo_t tag, output_tag;
    int ret; 
    
    kimage->k_size = k_size; 

    kimage->kmems = malloc(sizeof (vka_object_t) * k_size); 
    if (!kimage->kmems) 
        return BENCH_FAILURE; 

    kmem_caps = malloc(sizeof (seL4_CPtr) * k_size);
    if (!kmem_caps) 
        return BENCH_FAILURE; 

    ret = vka_alloc_kernel_image(vka, ki); 
    if (ret) 
        return BENCH_FAILURE; 

#ifdef CONFIG_ARCH_X86 
    /*assign a ASID to the kernel image*/
    ret =  seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, ki->cptr); 
#else 
    ret = seL4_ARM_ASIDPool_Assign(seL4_CapInitThreadASIDPool, ki->cptr); 
#endif

    if (ret) 
        return BENCH_FAILURE; 

    /*creating multiple kernel memory objects*/
    for (int mems = 0; mems < k_size; mems++) {

        ret = vka_alloc_kernel_mem(vka, &kimage->kmems[mems]); 
        if (ret) 
            return BENCH_FAILURE; 
        kmem_caps[mems] = kimage->kmems[mems].cptr; 
    }

    /*calling kernel clone with the kernel image and master kernel*/
#ifdef CONFIG_ARCH_X86
    tag = seL4_MessageInfo_new(X86KernelImageClone, 0, 1, k_size + 1);
#else 
    tag = seL4_MessageInfo_new(ARMKernelImageClone, 0, 1, k_size + 1); 
#endif 
    seL4_SetCap(0, simple_get_ik_image(&env->simple)); 

    seL4_SetMR(0, k_size);

    for (int mems = 0; mems < k_size; mems++) {
        seL4_SetMR(mems + 1, kmem_caps[mems]);
    }

    output_tag = seL4_Call(ki->cptr, tag); 
    ret = seL4_MessageInfo_get_label(output_tag);
    if (ret) 
        return BENCH_FAILURE; 

    printf("the kernel image clone is done \n");
    free(kmem_caps); 

    return BENCH_SUCCESS;
}

#endif

static int allocate_untyped(vka_t *vka, vka_object_t *untyped)
{
    int error = 0;

    /*allocating untyped according to the config*/    
    for (int i = 0; i < CONFIG_BENCH_UNTYPE_COUNT; i++) {
        error = vka_alloc_untyped(vka, 
                CONFIG_BENCH_UNTYPE_SIZEBITS, untyped + i);
        if (error) {
            break; 
        }
    }
    return error;
}

static int copy_untyped(sel4utils_process_t *process, vka_t *vka, 
        vka_object_t *untyped, seL4_CPtr *untyped_cptr) {

    for (int i = 0; i < CONFIG_BENCH_UNTYPE_COUNT; i++) {
  
        untyped_cptr[i] = sel4utils_copy_cap_to_process(process, 
                vka, untyped[i].cptr);
        if (!untyped_cptr[i])
            return BENCH_FAILURE; 

    }
    return BENCH_SUCCESS; 
}




static void create_thread(bench_thread_t *t) {

    sel4utils_process_t *process = &t->process; 
    bench_args_t *bench_args = NULL; 
    
    int error = 0; 

    /*allocating a page for arguments*/
    assert(sizeof(bench_args_t) < PAGE_SIZE_4K);
    bench_args = (bench_args_t*)vspace_new_pages(t->vspace, seL4_AllRights, 1, seL4_PageBits);
    assert(bench_args); 
    memset(bench_args, 0 , sizeof (bench_args_t));
    t->bench_args = bench_args; 
    
    /*test number*/
    bench_args->test_num = t->test_num; 

    /*configure process, requiring a seperate vspace and cspace*/ 
    error = sel4utils_configure_process(process, 
            t->vka, t->vspace,  
            t->image); 
    assert(error == 0); 

    bench_args->stack_pages = CONFIG_SEL4UTILS_STACK_SIZE / SIZE_BITS_TO_BYTES(seL4_PageBits);
    bench_args->stack_vaddr = ((uintptr_t) process->thread.stack_top) - CONFIG_SEL4UTILS_STACK_SIZE;
 
    NAME_THREAD(process->thread.tcb.cptr, t->name);

#ifdef CONFIG_MULTI_KERNEL_IMAGES 
    /*configure the kernel image to the process*/
    printf("set kernel image\n");
    error = seL4_TCB_SetKernel(process->thread.tcb.cptr, t->kernel);
    assert(error == 0);
#endif

#if (CONFIG_MAX_NUM_NODES > 1)
    /*assign the affinity*/ 
    error = seL4_TCB_SetAffinity(process->thread.tcb.cptr, t->affinity);
    assert(error == 0);
#endif 
    
    /*copy caps used for communication*/
    bench_args->ep = sel4utils_copy_cap_to_process(process, t->ipc_vka, t->ep.cptr);
    assert(bench_args->ep); 

    bench_args->r_ep = sel4utils_copy_cap_to_process(process, t->ipc_vka, t->reply_ep.cptr);
    assert(bench_args->r_ep);

}

static void launch_thread(bench_thread_t *t) {

    /*sperating this with the thread creation because something 
     can be done in between, such as, sharing frames between 
     threads*/
    sel4utils_process_t *process = &t->process; 
    bench_args_t *args = t->bench_args;
    char string_args[1][WORD_STRING_SIZE];
    char *argv[1];
    int error;

    /*map the page containing the arguments into the thread vspace*/
    void *remote_args_vaddr = vspace_share_mem(t->vspace, &process->vspace, 
            args, 1, seL4_PageBits, seL4_AllRights, true);
    assert(remote_args_vaddr); 

    sel4utils_create_word_args(string_args, argv, 1, remote_args_vaddr);
    
    /*create untypes with the vka belong to this process then copy*/
    error = allocate_untyped(t->vka, t->bench_untypes);
    assert(error == 0); 
    error = copy_untyped(process, t->vka, t->bench_untypes, args->untyped_cptr); 
    assert(error == 0);
    
    /* this is the last cap we copy - initialise the first free cap */           
    args->first_free = args->untyped_cptr[CONFIG_BENCH_UNTYPE_COUNT - 1] + 1;                                     
    printf("spawn process\n");
    /*start process*/ 
    error = sel4utils_spawn_process_v(process, t->vka, 
            t->vspace, 1, argv, 1);
    assert(error == 0);
}


/*software polling for number of CPU ticks*/
static void sw_sleep(unsigned int microsec) {

    unsigned long long s_tick = (unsigned long long)microsec * CPU_FEQ_MICROSEC;  
    volatile unsigned long long cur, tar; 

    /*a self implmeneted sleep function*/
#ifdef CONFIG_ARCH_X86
    cur = rdtscp_64();
#else 
    cur = sel4bench_get_cycle_count(); 
#endif 
    tar = cur + s_tick; 
    while (cur < tar) {
#ifdef CONFIG_ARCH_X86
        cur = rdtscp_64();
#else 
        cur = sel4bench_get_cycle_count(); 
#endif 
    }

}

static int alive(m_env_t *env) {

    volatile ccnt_t *s_vaddr = env->record_vaddr; 
    ccnt_t local = *s_vaddr; 
    /*try to probe on the shared buffer N times 
     if the content changes, the thread that uses the share buffer 
     is alive*/
    for(int i = 0; i < 10000; i++) {
        sw_sleep(1);
        if (local != *s_vaddr)
            return BENCH_SUCCESS; 

    }
    return BENCH_FAILURE; 
}


static void map_r_buf(m_env_t *env, uint32_t n_p, bench_thread_t *t) {

    sel4utils_process_t *p = &t->process; 
    bench_args_t *args = t->bench_args;
    
    if (!n_p)
        return; 

    /*allocate record buffer from thread*/
    args->record_pages = n_p; 
    args->record_vaddr = (uintptr_t)vspace_new_pages(&p->vspace, seL4_AllRights, 
            n_p, PAGE_BITS_4K);
    assert(args->record_vaddr); 
    
    /*map to the root thread*/
    env->record_vaddr = vspace_share_mem(&p->vspace, &env->vspace, 
            (void *)args->record_vaddr, n_p, 
            PAGE_BITS_4K, seL4_AllRights, true); 
    assert(env->record_vaddr); 
}

static void map_shared_buf(bench_thread_t *owner, bench_thread_t *share, 
        uint32_t n_p, uint32_t *phy) {

    /*allocate buffer from owner that shared between two threads*/
    sel4utils_process_t *p_o = &owner->process, *p_s = &share->process;
    uint32_t cookies; 
    bench_args_t *args_o = owner->bench_args, *args_s = share->bench_args; 
    
    /*the number of pages shared*/
    args_o->shared_pages = args_s->shared_pages = n_p;  

    /*allocating from the owner*/
    args_o->shared_vaddr = (uintptr_t)vspace_new_pages(&p_o->vspace, seL4_AllRights,
            n_p, PAGE_BITS_4K);
    assert(args_o->shared_vaddr); 

    /*find the physical address of the page*/
    cookies = vspace_get_cookie(&p_o->vspace, (void *)args_o->shared_vaddr); 

    *phy = vka_utspace_paddr(owner->vka, cookies, seL4_ARCH_4KPage, seL4_PageBits);

    /*sharing with the other thread*/
    args_s->shared_vaddr = (uintptr_t)vspace_share_mem(&p_o->vspace, 
            &p_s->vspace, (void *)args_o->shared_vaddr,
            n_p, PAGE_BITS_4K, seL4_AllRights, true);
    assert(args_s->shared_vaddr); 
}



static void create_huge_pages(bench_thread_t *owner, uint32_t size) {

   /*creating the large page that uses as the probe buffer 
    for testing, mapping to the benchmark process, and passing in 
    the pointer and the size to the benchmark thread*/ 
    uint32_t huge_page_size;
    sel4utils_process_t *p_o = &owner->process;
    bench_args_t *args = owner->bench_args;

    uint32_t cookies, huge_page_object;
    seL4_Word phy;

#ifdef CONFIG_ARCH_X86
    /*4M*/
    huge_page_size = seL4_LargePageBits;
    huge_page_object = seL4_X86_LargePageObject;
#else  /*ARCH_X86*/

#ifdef CONFIG_ARCH_AARCH64
    /*2M, Can be changed to be 1G with seL4_HugePageBits*/ 
    huge_page_size = vka_get_object_size(seL4_ARM_LargePageObject, 0); 
    huge_page_object = seL4_ARM_LargePageObject;
#else
    /*16M*/ 
    huge_page_size = vka_get_object_size(seL4_ARM_SuperSectionObject, 0); 
    huge_page_object = seL4_ARM_SuperSectionObject;
#endif 
#endif  /*ARCH_X86*/

    if (size % (1 << huge_page_size)) { 
        size += 1 << huge_page_size; 
    }
    size /= 1 << huge_page_size;

    args->hugepage_size = size * huge_page_size; 
    args->hugepage_vaddr = (uintptr_t)vspace_new_pages(&p_o->vspace, seL4_AllRights, size, 
            huge_page_size);

    /*find the physical address of the page*/
    for (int i = 0; i < size; i++) {
        cookies = vspace_get_cookie(&p_o->vspace, (void *)args->hugepage_vaddr + i * (1 << huge_page_size)); 
        phy = vka_utspace_paddr(owner->vka, cookies, huge_page_object, huge_page_size);
        printf("huge page vaddr %p phy %p\n", (void *)args->hugepage_vaddr + i * (1 << huge_page_size), (void *)phy);
    }

}

static inline void send_msg_to(seL4_CPtr endpoint, seL4_Word w) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new
        (seL4_Fault_NullFault, 0, 0, 1);

    seL4_SetMR(0, w); 
    seL4_Send(endpoint, info);
}


/*analysing benchmark results*/
void bench_process_data(m_env_t *env, seL4_Word result); 

/*interface in ipc.c*/
/*lanuch ipc benchmarking threads*/ 
void launch_bench_ipc(m_env_t *);
/*interface in covert.c*/
/*entry point of covert channel benchmark*/
void launch_bench_covert(m_env_t *env);
/*entry point of the functional correctness test
 in func_test.c*/
void launch_bench_func_test(m_env_t *env);
#endif   /*__MANAGER_H*/

