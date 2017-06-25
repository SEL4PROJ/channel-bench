/*a covert channel benchmark 
 setting up two threads: trojan and receiver
 currently only support single core*/ 

/*Kconfig variables*/
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <sel4utils/mapping.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <simple/simple.h>

#include "manager.h"
#include "ipc.h"
#include "../../covert.h"

#ifdef  CONFIG_MANAGER_COVERT_BENCH

/*start of the elf file*/
extern char __executable_start;

/*the benchmarking enviornment for two threads*/
static bench_env_t trojan, spy;

#ifdef CONFIG_BENCH_COVERT_SINGLE
/*using the cache colouring allocator for the probe/trojan buffers*/
#ifndef CONFIG_LIB_SEL4_CACHECOLOURING 
/*buffers for probing and trojan processes*/ 
static char p_buf[EBSIZE] __attribute__((aligned (BENCH_PAGE_SIZE)));
static char t_buf[CACHESIZE] __attribute__((aligned (BENCH_PAGE_SIZE))); 
#endif 
#endif 
/*ep for syn and reply*/
static vka_object_t syn_ep, t_ep, s_ep; 

/*mapping a buffer from env to the thread t, 
vaddr in vspace of env, s size in bytes*/
void map_init_frames(m_env_t *env, void *vaddr, uint32_t s, bench_env_t *t) {

    reservation_t v_r;
    void *t_v;  
    sel4utils_process_t *p = &t->process; 
    cspacepath_t src, dest; 
    int ret; 
    seL4_CPtr frame;

    /*using simple*/
    simple_t *simple = &env->simple; 
    seL4_Word offset =(seL4_Word) vaddr - (seL4_Word)&__executable_start; 

    if (!s)
        return; 

    /*page aligned*/
    assert(!(s % BENCH_PAGE_SIZE)); 
    assert(!((seL4_Word)vaddr % BENCH_PAGE_SIZE)); 
    assert(simple);

    /*reserve an area in vspace*/ 
    v_r = vspace_reserve_range(&p->vspace, s, seL4_AllRights, 1, &t_v);
    assert(v_r.res != NULL);
   
    /*num of pages*/
    s /= BENCH_PAGE_SIZE; 
    offset /= BENCH_PAGE_SIZE; 
    assert(s <= BENCH_COVERT_BUF_PAGES); 

    for (int i = 0; i < s; i++, offset++) {
    
        /*find the frame caps for the p_buf and t_buf*/ 
        frame = simple->nth_userimage(simple->data, offset); 
        /*copy those frame caps*/ 
        vka_cspace_make_path(&env->vka, frame, &src); 
        ret = vka_cspace_alloc(&env->vka, &t->p_frames[i]); 
        assert(ret == 0); 
        vka_cspace_make_path(&env->vka, t->p_frames[i], &dest); 
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
        assert(ret == 0); 
    }
    /*map in*/
    ret = vspace_map_pages_at_vaddr(&p->vspace, t->p_frames, 
            NULL, t_v, s, PAGE_BITS_4K, v_r);
    assert(ret == 0); 
    /*record*/
    t->p_vaddr = t_v;

}
/*map a probing buffer to thread*/
void map_p_buf(bench_env_t *t, uint32_t size) {

    if (!size)
        return; 

    sel4utils_process_t *p = &t->process; 
    uint32_t n_p = size / PAGE_SIZE;
 
    /*pages for probing buffer*/
    t->p_vaddr = vspace_new_pages(&p->vspace, seL4_AllRights, 
            n_p, PAGE_BITS_4K);
    assert(t->p_vaddr != NULL); 

}

void create_huge_pages(bench_env_t *owner, uint32_t size) {


   /*creating the large page that uses as the probe buffer 
    for testing, mapping to the benchmark process, and passing in 
    the pointer and the size to the benchmark thread*/ 
    uint32_t huge_page_size;
    sel4utils_process_t *p_o = &owner->process;

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

    owner->huge_vaddr = vspace_new_pages(&p_o->vspace, seL4_AllRights, size, 
            huge_page_size);
    

    /*find the physical address of the page*/
    for (int i = 0; i < size; i++) {
        cookies = vspace_get_cookie(&p_o->vspace, owner->huge_vaddr + i * (1 << huge_page_size)); 
        phy = vka_utspace_paddr(owner->vka, cookies, huge_page_object, huge_page_size);

        printf("huge page vaddr %p phy %p\n", owner->huge_vaddr + i * (1 << huge_page_size), (void *)phy);
    }

}
/*init covert bench for single core*/
void init_single(m_env_t *env) {
    

    /*single core*/
    trojan.affinity = spy.affinity = 0; 

    /*one trojan, one spy thread*/ 
    printf("creating trojan\n"); 
    create_thread(&trojan); 
    printf("creating spy\n"); 
    create_thread(&spy); 

}



int run_single_l2(m_env_t *env) {

#if 0  
    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 0); 
    int size;

    assert(ts); 
    printf("start covert benchmark, single core, l2 cache\n"); 
    while (1) {    
   
        /*spy can now start*/
        seL4_Send(s_ep.cptr, tag);
        info = seL4_Recv(s_ep.cptr, NULL); 
        if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
            return BENCH_FAILURE; 

        /*end of test*/
        if (seL4_MessageInfo_get_length(info) == 0)
            break; 
        else if (seL4_MessageInfo_get_length(info) == 1) 
            size = seL4_GetMR(0); 
        else 
            return BENCH_FAILURE; 

        /*collecting data for size N */
        /*data format: secret time*/
        for (int k = 1; k < TIME_MAX; k++) {
            int n = ts_get(ts, k);
            if (n)
                printf("%d %d %d\n", n, size, k);
        }
    }

#endif 
    assert(0);
    printf("done covert benchmark\n");
    return BENCH_SUCCESS; 
}



int run_single_l1(m_env_t *env) {
    
    seL4_MessageInfo_t info;
    seL4_MessageInfo_t tag;
    struct bench_l1 *r_d;
    uint32_t n_p = (sizeof (struct bench_l1) / BENCH_PAGE_SIZE) + 1;
    uint32_t share_phy; 


    printf("starting covert channel benchmark\n");

#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
    printf("data points %d with sequential sequence\n", CONFIG_BENCH_DATA_POINTS);
#else 
    printf("data points %d with random sequence\n", CONFIG_BENCH_DATA_POINTS);
#endif 

    map_shared_buf(&spy, &trojan, NUM_L1D_SHARED_PAGE, &share_phy);
    map_r_buf(env, n_p, &spy);
    /*create huge pages*/ 
#ifdef CONFIG_MANAGER_HUGE_PAGES
    create_huge_pages(&trojan, BENCH_MORECORE_HUGE_SIZE); 
    create_huge_pages(&spy, BENCH_MORECORE_HUGE_SIZE); 
#endif 

    tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 4);

    /*spy*/
    seL4_SetMR(0,(seL4_Word)spy.t_vaddr);
    seL4_SetMR(1, (seL4_Word)spy.s_vaddr);
    seL4_SetMR(2, share_phy);
    seL4_SetMR(3, (seL4_Word)spy.huge_vaddr);

    seL4_Send(s_ep.cptr, tag);
    printf("spy is ready\n");   
 
    /*trojan*/
    tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 3);
    seL4_SetMR(0, (seL4_Word)trojan.s_vaddr);
    seL4_SetMR(1, share_phy);
    seL4_SetMR(2, (seL4_Word)trojan.huge_vaddr);
    seL4_Send(t_ep.cptr, tag);
    
    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
       return BENCH_FAILURE;

    printf("trojan is ready\n");
    
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    
    printf("benchmark result ready\n");
    
    r_d =  (struct bench_l1 *)env->record_vaddr;
    printf("probing time start\n");
    
    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
        printf("%d %u\n", r_d->sec[i], r_d->result[i]);

    }
    printf("probing time end\n");


#ifdef CONFIG_MANAGER_PMU_COUNTER 
    printf("enabled %d pmu counters\n", BENCH_PMU_COUNTERS);
    for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++) {

        /*print out the pmu counter one by one */
        printf("pmu counter %d start\n",  counter); 
        for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
            printf("%d %u\n", r_d->sec[i], r_d->pmu[i][counter]);
        }

        printf("pmu counter %d end\n", counter);

    }
#endif 

    printf("done covert benchmark\n");
    return BENCH_SUCCESS; 
}


int run_single_llc_kernel(m_env_t *env) {

    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 0); 
    printf("start covert benchmark, single core, LLC through a shared kernel with seL4_Poll\n"); 


    printf("Starting L3 spy setup\n");
    seL4_Send(s_ep.cptr, tag);
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("Starting L3 Trojan setup\n");
    seL4_Send(t_ep.cptr, tag);

    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("Starting test 1\n");
    seL4_Send(s_ep.cptr, tag);
    seL4_Send(t_ep.cptr, tag);
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("Starting test 2\n");
    seL4_Send(s_ep.cptr, tag);
    seL4_Send(t_ep.cptr, tag);
    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;

    printf("done covert benchmark\n");
    return BENCH_SUCCESS; 
}


int run_single_llc_kernel_schedule(m_env_t *env) {

    seL4_MessageInfo_t info;
    seL4_MessageInfo_t tag;
    struct bench_kernel_schedule *r_d;
    uint32_t n_p = (sizeof (struct bench_kernel_schedule) / BENCH_PAGE_SIZE) + 1;
    uint32_t share_phy; 

    printf("starting covert channel benchmark, LLC, kernel deterministic scheduling\n");
#ifdef CONFIG_BENCH_DATA_SEQUENTIAL 
    printf("data points %d with sequential sequence\n", CONFIG_BENCH_DATA_POINTS);
#else 
    printf("data points %d with random sequence\n", CONFIG_BENCH_DATA_POINTS);
#endif 
    /*first the shared buffer, then the record buffer*/
    map_shared_buf(&spy, &trojan, NUM_KERNEL_SCHEDULE_SHARED_PAGE, &share_phy);
    map_r_buf(env, n_p, &spy);
    
    tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 2);

    /*spy*/
    seL4_SetMR(0,(seL4_Word)spy.t_vaddr);
    seL4_SetMR(1, (seL4_Word)spy.s_vaddr);

    seL4_Send(s_ep.cptr, tag);
    printf("spy is ready\n");   
    
    tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, (seL4_Word)trojan.s_vaddr);
    seL4_Send(t_ep.cptr, tag);

    info = seL4_Recv(t_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
       return BENCH_FAILURE;

    printf("trojan is ready\n");

    info = seL4_Recv(s_ep.cptr, NULL);
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE;
    
    printf("benchmark result ready\n");

    r_d =  (struct bench_kernel_schedule *)env->record_vaddr;
    printf("online time start\n");

    for (int i = 3; i < NUM_KERNEL_SCHEDULE_DATA; i++) {

        printf("%d "CCNT_FORMAT"\n", r_d->prev_sec[i], r_d->prevs[i] - r_d->starts[i]);

    }

    printf("online time end\n");

    printf("offline time start\n");
    for (int i = 3; i < NUM_KERNEL_SCHEDULE_DATA; i++) {
        printf("%d "CCNT_FORMAT"\n", r_d->cur_sec[i], r_d->curs[i] - r_d->prevs[i]);
    }
    printf("offline time end\n");

    /*printf("Trojan: %llu %llu %llu -> %llu %llu %llu\n", r_d->prevs[i], r_d->starts[i], r_d->curs[i], r_d->prevs[i] - r_d->starts[i], r_d->curs[i] - r_d->prevs[i], r_d->curs[i] - r_d->starts[i]);
     */

    printf("done covert benchmark\n");
    return BENCH_SUCCESS;


}

/*running the single core attack*/ 
int run_single (m_env_t *env) {

#ifdef CONFIG_BENCH_COVERT_L2
    return run_single_l1(env); 
#endif     
#if defined (CONFIG_BENCH_COVERT_L1D) || defined (CONFIG_BENCH_COVERT_L1I)  
    return run_single_l1(env);
#endif 
#ifdef CONFIG_BENCH_COVERT_TLB 
    return run_single_l1(env);
#endif 
#ifdef CONFIG_BENCH_COVERT_BTB 
    return run_single_l1(env); 
#endif
#ifdef CONFIG_BENCH_COVERT_BP
    return run_single_l1(env); 
#endif
    /*the LLC single core covert channel*/
#ifdef CONFIG_BENCH_COVERT_LLC
    return run_single_l1(env); 
#endif 
#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL 
    return run_single_llc_kernel(env); 
#endif 
#ifdef CONFIG_BENCH_COVERT_LLC_KERNEL_SCHEDULE
    return run_single_llc_kernel_schedule(env); 
#endif 
    return BENCH_FAILURE; 
}

static inline void mfence() {
  asm volatile("mfence");
}

int run_multi(m_env_t *env) {

    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 0); 
    
    printf("start multicore side channel benchmark\n"); 
    
    seL4_Send(s_ep.cptr, tag);
    
    info = seL4_Recv(s_ep.cptr, NULL); 
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE; 
    
    printf("spy ready\n");


    seL4_Send(s_ep.cptr, tag);
        
    info = seL4_Recv(s_ep.cptr, NULL); 
    if (seL4_MessageInfo_get_label(info) != seL4_Fault_NullFault)
        return BENCH_FAILURE; 

    printf("done covert benchmark\n"); 
    /*do not return*/
    for (;;)
    return BENCH_SUCCESS; 

}

/*prepare the running enviroment for benchmarking on single core*/
void prepare_single(m_env_t *env) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 
            BENCH_COVERT_MSG_LEN);
    seL4_CPtr n_ep; 

    printf("Preparing the running environment for benchmarking threads....\n"); 
#ifdef CONFIG_LIB_SEL4_CACHECOLOURING 
    map_p_buf(&trojan, CACHESIZE);
    map_p_buf(&spy, EBSIZE);

#else
    /*map the buffers into spy and trojan threads*/ 
    map_init_frames(env, (void *)t_buf, CACHESIZE, &trojan);
    map_init_frames(env, (void *)p_buf, EBSIZE, &spy); 
#endif 

    /*recording buffer, outcome from spy*/
    map_r_buf(env, BENCH_COVERT_TIME_PAGES, &spy);

   /*copy the notification cap*/ 
    n_ep = sel4utils_copy_cap_to_process(&trojan.process, trojan.vka, trojan.notification_ep.cptr);
    assert(n_ep); 
 
    printf("Sending Trojan env message...."); 
    /*trojan, probming buffer, 0, notification ep*/
    seL4_SetMR(0,(seL4_Word)trojan.p_vaddr); 
    seL4_SetMR(1, 0);
    seL4_SetMR(2, n_ep);


    seL4_Send(t_ep.cptr, info);
    printf("done\n"); 

    n_ep = sel4utils_copy_cap_to_process(&spy.process, spy.vka, spy.notification_ep.cptr);
    assert(n_ep); 
    printf("Sending Spy env message...."); 
    /*spy, probming buffer, recording buffer, notification ep*/
    seL4_SetMR(0, (seL4_Word)spy.p_vaddr); 
    seL4_SetMR(1, (seL4_Word)spy.t_vaddr);
    seL4_SetMR(2, n_ep);
    seL4_Send(s_ep.cptr, info);
    printf("done\n"); 

    printf("\t\t\t\t environment is set up\n\n");

}

/*init the covert benchmark for multicore attack*/
void init_multi(m_env_t *env) {


    spy.affinity  = 0;
    trojan.affinity = 1; 

    /*one trojan, one spy thread*/ 
    create_thread(&trojan); 
    create_thread(&spy); 

    if (run_multi(env) != BENCH_SUCCESS) 
        printf("running multi benchmark failed\n");

    /*never return*/
    seL4_Recv(syn_ep.cptr, NULL); 
}
#ifdef CONFIG_ARCH_X86
static inline uint32_t rdtscp() {
  uint32_t rv;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}
#endif 
/*entry point of covert channel benchmark*/
void launch_bench_covert (m_env_t *env) {

    int ret; 
    trojan.image = spy.image = CONFIG_BENCH_THREAD_NAME;
    trojan.vspace = spy.vspace = &env->vspace;
    trojan.name = "trojan"; 
    spy.name = "spy";
    
#ifdef CONFIG_MANAGER_COVERT_MITIGATION 
    /*using sperate kernels*/
    trojan.kernel = env->kimages[0].ki.cptr; 
    spy.kernel = env->kimages[1].ki.cptr;
#else 
    /*by default the kernel is shared*/
    trojan.kernel = spy.kernel = env->kernel;
#endif 

#ifdef CONFIG_MANAGER_COVERT_MITIGATION 
    trojan.vka = &env->vka_colour[0]; 
    spy.vka = &env->vka_colour[1]; 
    env->ipc_vka = &env->vka_colour[0];
#else 
    spy.vka = trojan.vka = &env->vka; 
    env->ipc_vka = &env->vka;
#endif

#if 0
    /*setting the kernel sensitivity*/
#ifdef CONFIG_COVERT_TROJAN_SENSITIVE 
    seL4_KernelImage_Sensitive(trojan.kernel); 
    seL4_KernelImage_HoldTime(trojan.kernel, CONFIG_COVERT_TROJAN_HOLD_TIME, 0);
#ifdef CONFIG_ARCH_X86 
    srandom(rdtscp());
#else 
    srandom(sel4bench_get_cycle_count());
#endif 
   
#ifdef CONFIG_ARCH_X86
    for (int i = 0; i < 512; i ++)
        seL4_KernelImage_Random(trojan.kernel, i, random());
#endif  /*CONFIG_ARCH_X86*/
#endif  /*CONFIG COVERT TROJAN SENSITIVE*/

#ifdef CONFIG_COVERT_SPY_SENSITIVE 

    seL4_KernelImage_Sensitive(spy.kernel);                                    
    seL4_KernelImage_HoldTime(spy.kernel, CONFIG_COVERT_SPY_HOLD_TIME, 0); 
#ifdef CONFIG_ARCH_X86 
    srandom(rdtscp());
#else 
    srandom(sel4bench_get_cycle_count());
#endif
#ifdef CONFIG_ARCH_X86
     for (int i = 0; i < 512; i ++)
        seL4_KernelImage_Random(spy.kernel, i, random());
#endif   /*CONFIG_ARCH_X86*/
#endif  /*CONFIG_COVERT_SPY_SENSITIVE*/
#endif 


    /*ep for communicate*/
    ret = vka_alloc_endpoint(env->ipc_vka, &syn_ep);
    assert(ret == 0);
    /*ep for spy to manager*/
    ret = vka_alloc_endpoint(env->ipc_vka, &s_ep); 
    assert(ret == 0);
    /*ep for trojan to manager*/
    ret = vka_alloc_endpoint(env->ipc_vka, &t_ep);
    assert(ret == 0);

    trojan.ipc_vka = spy.ipc_vka = env->ipc_vka; 
    spy.ep  = trojan.ep = syn_ep; 
    spy.reply_ep = s_ep;
    trojan.reply_ep = t_ep; 

    /*create a notification endpoint, used only within a domain*/
    ret = vka_alloc_notification(trojan.vka, &trojan.notification_ep); 
    assert(ret == 0); 
    
    ret = vka_alloc_notification(spy.vka, &spy.notification_ep); 
    assert(ret == 0); 
 
    spy.prio  = 100;
    trojan.prio = 100;
    
    /*set the actual testing num in bench_common.h*/
    spy.test_num = BENCH_COVERT_SPY;
    trojan.test_num = BENCH_COVERT_TROJAN; 


#ifdef CONFIG_BENCH_COVERT_SINGLE

    init_single(env);
    prepare_single(env); 
    /*run bench*/
    ret = run_single(env);
    assert(ret == BENCH_SUCCESS);
#endif /*covert single*/ 

#ifdef CONFIG_MANAGER_COVERT_MULTI 
    init_multi(env); 
#endif 

}
#endif 
