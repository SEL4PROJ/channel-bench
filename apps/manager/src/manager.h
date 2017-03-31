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
#ifdef CONFIG_CACHE_COLOURING 
#include <cachecoloring/color_allocator.h>
#endif

/*common definitions*/
#include "../../bench_common.h"

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
    vka_t *ipc_vka;    /*pointer for ep allocator*/
    vspace_t vspace; 
    vka_t vka_colour[CC_NUM_DOMAINS]; 
    bench_ki_t kimages[MAN_KIMAGES];
    seL4_CPtr kernel; 
    /*the boot info*/
    seL4_BootInfo *bootinfo; 

    /*abstracts over kernel version and boot envior*/
    simple_t simple; 

    /*path for the default timer irq handler*/
    cspacepath_t irq_path; 
#ifdef CONFIG_ARCH_ARM 
    /*frame for the default timer*/ 
    cspacepath_t frame_path; 
#endif 

#ifdef CONFIG_ARCH_IA32 
    /*io port for the default timer*/ 
    seL4_CPtr io_port_cap; 
#endif 
    /*extra caps to the record data frames for mapping into 
     the benchmark vspace*/ 
    seL4_CPtr record_frames[CONFIG_BENCH_RECORD_PAGES]; 

    /*virtual address for recording data, used by root*/ 
    void *record_vaddr; 

    /*mapping the huge pages into the process address space for benchmarking
     40 * 4M = 160M*/
    seL4_CPtr huge_frames[40]; 

    /*virtual address for the huge frames, used by root thread*/ 
    void *huge_vaddr; 
    
    /*benchmark thread structure*/
    sel4utils_process_t bench_thread;
   // [MAX_BENCH_THREADS]; 

    /*endpoint that root thread is waiting on*/ 
    seL4_CPtr bench_ep;
    
} m_env_t;  /*environnt used by the manager*/  


/*parameters for setting up a benchmark process*/ 
typedef struct bench_env {

    seL4_CPtr kernel;       /*kernel image*/
    vka_object_t ep;               /*communication between trojan & spy*/ 
    vka_object_t reply_ep;         /*reply manager*/
    vka_object_t notification_ep;  /*using for seL4_Poll*/       
    vka_object_t null_ep;    /*never returned ep*/
    vka_t *vka;              /*kernel object allocator*/
    vka_t *ipc_vka;          /*used for endpoint*/
    vspace_t *vspace;        /*virtual address space of the root task*/
    char *image;            /*binary name of benchmarking elf file*/
    seL4_Word prio;         /*priority of benchmarking thread*/
    sel4utils_process_t process;  /*internal process context*/ 
    uint32_t test_num;      /*test number*/
    /*extra caps to the record data frames for mapping into 
     the benchmark vspace*/ 
    seL4_CPtr record_frames[BENCH_PMU_PAGES]; 
    seL4_CPtr t_frames[BENCH_COVERT_BUF_PAGES];
    seL4_CPtr p_frames[BENCH_COVERT_BUF_PAGES]; /*shared frame caps for prime/trojan buffers*/
    seL4_CPtr s_frames[BENCH_COVERT_BUF_PAGES]; /*shared buffer*/
    void *p_vaddr;
    void *t_vaddr;  /*vaddr for above buffers in bench thread vspace*/ 
    void *s_vaddr;  /*shared address between spy and trojan*/
    void *huge_vaddr;  /*the starting point of huge pages*/
    /*the affinity of this thread*/ 
    uint32_t affinity; 
    char *name;    /*name of this thread*/ 

} bench_env_t; 

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

    /*assign a ASID to the kernel image*/
    ret =  seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, ki->cptr); 
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
    tag = seL4_MessageInfo_new(X86KernelImageClone, 0, 1, k_size + 1);
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
    return BENCH_SUCCESS;
}



static void create_thread(bench_env_t *t) {

    sel4utils_process_t *process = &t->process; 
    cspacepath_t src;
    seL4_CPtr ep_arg, reply_ep_arg;
    int argc = 3;
    char arg_str0[15] = {0}; 
    char arg_str1[15] = {0}; 
    char arg_str2[15] = {0}; 

    char *argv[3] = {arg_str0, arg_str1, arg_str2}; 
    
    int error __attribute__((unused)); 

    printf("creating benchmarking thread\n");
    /*configure process*/ 
    error = sel4utils_configure_process(process, 
            t->vka, t->vspace, t->prio, 
            t->image); 
            
    assert(error == 0); 


    vka_cspace_make_path(t->ipc_vka, t->ep.cptr, &src);  
    ep_arg = sel4utils_copy_cap_to_process(process, src);
    assert(ep_arg); 

    vka_cspace_make_path(t->ipc_vka, t->reply_ep.cptr, &src);  
    reply_ep_arg = sel4utils_copy_cap_to_process(process, src);
    assert(reply_ep_arg);

    sprintf(arg_str0, "%d", t->test_num); 
    sprintf(arg_str1, "%d", ep_arg); 
    sprintf(arg_str2, "%d", reply_ep_arg); 

#ifdef CONFIG_MULTI_KERNEL_IMAGES 
    /*configure the kernel image to the process*/
    printf("set kernel image\n");
    error = seL4_TCB_SetKernel(process->thread.tcb.cptr, t->kernel);
    assert(error == 0);
#endif 
    printf("spawn process\n");
   /*start process*/ 
    error = sel4utils_spawn_process_v(process, t->vka, 
            t->vspace, argc, argv, 0);
    assert(error == 0);
    
    /*assign the affinity*/ 
#if (CONFIG_MAX_NUM_NODES > 1)
    error = seL4_TCB_SetAffinity(process->thread.tcb.cptr, t->affinity);
    assert(error == 0);
#endif 
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(process->thread.tcb.cptr, t->name);
#endif

    printf("process is ready\n"); 
    error = seL4_TCB_Resume(process->thread.tcb.cptr);
    assert(error == 0);
   
}

/*reassign a thread to a different core
 assume the affinity is already updated by the caller*/
static void reassign_core( bench_env_t *t) {
    
    sel4utils_process_t *process = &t->process; 
    
    int error __attribute__((unused)); 


    error = seL4_TCB_Suspend(process->thread.tcb.cptr);
    assert(error == 0);
 
#if (CONFIG_MAX_NUM_NODES > 1)
    error = seL4_TCB_SetAffinity(process->thread.tcb.cptr, t->affinity);
    assert(error == 0);
#endif 

    error = seL4_TCB_Resume(process->thread.tcb.cptr);
    assert(error == 0);
}


static int alive(m_env_t *env) {

    volatile uint32_t *s_vaddr = env->record_vaddr; 
    uint32_t local = *s_vaddr; 
    /*try to probe on the shared buffer N times 
     if the content changes, the thread that uses the share buffer 
     is alive*/
    for(int i = 0; i < 10000; i++) {
        if (local != *s_vaddr)
            return BENCH_SUCCESS; 

    }
    return BENCH_FAILURE; 
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

/*map the record buffer to thread*/
static void map_r_buf(m_env_t *env, uint32_t n_p, bench_env_t *t) {

    reservation_t v_r;
    void  *e_v;  
    cspacepath_t src, dest;
    int ret; 
    sel4utils_process_t *p = &t->process; 
    
    if (!n_p)
        return; 

    t->t_vaddr = vspace_new_pages(&p->vspace, seL4_AllRights, 
            n_p, PAGE_BITS_4K);
    assert(t->t_vaddr != NULL); 

    uint32_t v = (uint32_t)t->t_vaddr; 

    /*reserve an area in vspace of the manager*/ 
    v_r = vspace_reserve_range(&env->vspace, n_p * BENCH_PAGE_SIZE, 
            seL4_AllRights, 1, &e_v);
    assert(v_r.res != NULL);

    env->record_vaddr = e_v; 

    for (int i = 0; i < n_p; i++, v+= BENCH_PAGE_SIZE) {

        /*copy those frame caps*/ 
        vka_cspace_make_path(t->vka, vspace_get_cap(&p->vspace, 
                    (void*)v), &src);
        ret = vka_cspace_alloc(&env->vka, &t->t_frames[i]); 
        assert(ret == 0); 
        vka_cspace_make_path(&env->vka, t->t_frames[i], &dest); 
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
        assert(ret == 0); 
    }
    /*map in*/
    ret = vspace_map_pages_at_vaddr(&env->vspace, t->t_frames, 
            NULL, e_v, n_p, PAGE_BITS_4K, v_r);
    assert(ret == 0); 

}

static void map_shared_buf(bench_env_t *owner, bench_env_t *share, 
        uint32_t n_p, uint32_t *phy) {

    /*allocate buffer from owner that shared between two threads*/
    reservation_t v_r;
    void  *e_v;
    cspacepath_t src, dest;
    int ret;
    sel4utils_process_t *p_o = &owner->process, *p_s = &share->process;
    uint32_t cookies; 

    /*pages for spy record, ts structure*/
    owner->s_vaddr = vspace_new_pages(&p_o->vspace, seL4_AllRights,
            n_p, PAGE_BITS_4K);
    assert(owner->s_vaddr != NULL);

    /*find the physical address of the page*/
    cookies = vspace_get_cookie(&p_o->vspace, owner->s_vaddr); 

    *phy = vka_utspace_paddr(owner->vka, cookies, seL4_ARCH_4KPage, seL4_PageBits);

    uint32_t v = (uint32_t)owner->s_vaddr;

    /*reserve the area in shared process*/
    v_r = vspace_reserve_range(&p_s->vspace, n_p * BENCH_PAGE_SIZE,
            seL4_AllRights, 1, &e_v);
    assert(v_r.res != NULL);

    share->s_vaddr = e_v;

    for (int i = 0; i < n_p; i++, v+= BENCH_PAGE_SIZE) {

        /*copy those frame caps*/
        vka_cspace_make_path(owner->vka, vspace_get_cap(&p_o->vspace,
                    (void*)v), &src);
        ret = vka_cspace_alloc(share->vka, &share->s_frames[i]);
        assert(ret == 0);
        vka_cspace_make_path(share->vka, share->s_frames[i], &dest);
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights);
        assert(ret == 0);
    }
    /*map in*/
    ret = vspace_map_pages_at_vaddr(&p_s->vspace, share->s_frames,
            NULL, e_v, n_p, PAGE_BITS_4K, v_r);
    assert(ret == 0);

}



static void send_run_env(bench_env_t *p, seL4_CPtr ep) {

    //seL4_MessageInfo_t info;
    seL4_MessageInfo_t tag;

    /*sending the running enviornment to process*/
    tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 2);

    seL4_SetMR(0,(seL4_Word)p->t_vaddr);
    seL4_SetMR(1, (seL4_Word)p->s_vaddr);

    seL4_Send(ep, tag);
}

static int wait_msg(seL4_CPtr ep) {
    seL4_MessageInfo_t info;

    info = seL4_Recv(ep, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
       return BENCH_FAILURE;

    return BENCH_SUCCESS;

}


/*interfaces in data.c*/

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

#endif

