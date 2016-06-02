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

#ifdef CONFIG_CACHE_COLOURING 
#include <cachecoloring/color_allocator.h>
#endif

/*common definitions*/
#include "../../bench_common.h"


typedef struct {

    vka_t vka;    
    vka_t *ipc_vka;    /*pointer for ep allocator*/
    vspace_t vspace; 
#ifdef CONFIG_CACHE_COLOURING  
    vka_t vka_colour[CC_NUM_DOMAINS]; 
    colored_kernel_t kernel_colour[CC_NUM_DOMAINS];
    seL4_CPtr kernel; 
#endif 
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
    seL4_CPtr t_frames[BENCH_COVERT_TIME_PAGES];
    seL4_CPtr p_frames[BENCH_COVERT_BUF_PAGES]; /*shared frame caps for prime/trojan buffers*/
    void *p_vaddr;
    void *t_vaddr;  /*vaddr for above buffers in bench thread vspace*/ 
 /*the affinity of this thread*/ 
    uint32_t affinity; 

} bench_env_t; 



/*interfaces in data.c*/

/*analysing benchmark results*/
void bench_process_data(m_env_t *env, seL4_Word result); 

/*interface in ipc.c*/
/*lanuch ipc benchmarking threads*/ 
void launch_bench_ipc(m_env_t *);
/*interface in covert.c*/
/*entry point of covert channel benchmark*/
void launch_bench_covert(m_env_t *env);

#endif

