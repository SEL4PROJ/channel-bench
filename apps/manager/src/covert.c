/*a covert channel benchmark 
 setting up two threads: trojan and receiver
 currently only support single core*/ 
#ifdef CONFIG_MANAGER_COVERT_SINGLE 
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <simple-stable/simple-stable.h>
#include "manager.h"

#include "../../bench_common.h"
#include "../../covert.h"


/*the benchmarking enviornment for two threads*/
static bench_env_t trojan, spy;

/*buffers for probing and trojan processes*/ 
static char p_buf[EBSIZE] __attribute__((aligned (BENCH_PAGE_SIZE)));
static char t_buf[CACHESIZE] __attribute__((aligned (BENCH_PAGE_SIZE))); 
/*ep for syn and reply*/
static vka_object_t syn_ep, t_ep, s_ep; 

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
#if CONFIG_CACHE_COLOURING
    /*configure kernel image*/
    bind_kernel_image(t->kernel,
            process->pd.cptr, process->thread.tcb.cptr);
#endif

   /*start process*/ 
    error = sel4utils_spawn_process_v(process, t->vka, 
            t->vspace, argc, argv, 1);
    assert(error == 0);
   
}

/*mapping a buffer from env to the thread t, 
vaddr in vspace of env, s size in bytes*/
static void map_init_frames(m_env_t *env, void *vaddr, uint32_t s, bench_env_t *t) {

    reservation_t v_r;
    void *t_v;  
    sel4utils_process_t *p = &t->process; 
    cspacepath_t src, dest; 
    int ret; 

    /*using simple*/
    simple_t *simple = &env->simple; 
    uint32_t offset =(uint32_t) (vaddr - __executable_start); 

    /*page aligned*/
    assert(!(s % BENCH_PAGE_SIZE)); 
    assert(!(vaddr % BENCH_PAGE_SIZE)); 
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
        ret = vka_cspace_alloc(&env->vka, &t.p_frames[i]); 
        assert(ret == 0); 
        vka_cspace_make_path(&env->vka, t.p_frames[i], &dest); 
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
        assert(ret == 0); 
    }
    /*map in*/
    ret = vspace_map_pages_at_vaddr(&p->vspace, t.p_frames, 
            NULL, t_v, s, PAGE_BITS_4K, v_r);
    assert(ret == 0); 
    /*record*/
    t->p_vaddr = t_v;
}

/*map the record buffer to thread*/
static void map_r_buf(m_env_t *env, uint32_t n_p, bench_env_t *t) {

    reservation_t v_r;
    void *t_v;  
    cspacepath_t src, cspacepath_t dest;
    uint32_t v = (uint32_t)env->record_vaddr; 
    int ret; 
    sel4utils_process_t *p = &t->process; 
    
    /*reserve an area in vspace*/ 
    v_r = vspace_reserve_range(&p->vspace, s, seL4_AllRights, 1, &t_v);
    assert(v_r.res != NULL);

    for (int i = 0; i < n_p; i++, v+= BENCH_PAGE_SIZE) {

        /*copy those frame caps*/ 
        vka_cspace_make_path(&env->vka, vspace_get_cap(&env->vspace, 
                    (void*)v), &src);
        ret = vka_cspace_alloc(&env->vka, &t.t_frames[i]); 
        assert(ret == 0); 
        vka_cspace_make_path(&env->vka, t.t_frames[i], &dest); 
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
        assert(ret == 0); 
    }
    /*map in*/
    ret = vspace_map_pages_at_vaddr(&p->vspace, t.t_frames, 
            NULL, t_v, n_p, PAGE_BITS_4K, v_r);
    assert(ret == 0); 

    t->t_vaddr = t_v;

}

/*init covert bench for single core*/
static void init_single(m_env_t *env) {
    
    int ret;

    /*ep for communicate*/
    ret = vka_alloc_endpoint(&env->vka, &syn_ep);
    assert(ret == 0);
    /*ep for spy to manager*/
    ret = vka_alloc_endpoint(&env->vka, &s_ep); 
    assert(ret == 0);
    /*ep for trojan to manager*/
    ret = vka_alloc_endpoint(&env->vka, &t_ep);
    assert(ret == 0);


    spy.ep = trojan.ep = syn_ep; 
    spy.reply_ep = s_ep;
    trojan.reply_ep = t_ep; 
    spy.test_num = BENCH_COVERT_SPY_SINGLE;
    trojan.test_num = BENCH_COVERT_TROJAN_SINGLE; 
    trojan.ipc_vka = spy.ipc_vka = &env->vka; 
    trojan.prio = spy.prio = CONFIG_BENCH_PRIORITY;

    /*one trojan, one spy thread*/ 
    create_thread(&trojan); 
    create_thread(&spy); 

   
}

static void send_env(m_env_t *env) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 
            BENCH_COVERT_MSG_LEN);

    /*trojan*/
    seL4_SetMR(0,(seL4_Word)trojan.p_vaddr); 
    seL4_SetMR(1, 0);
    seL4_Send(t_ep.cptr, info);

    /*spy*/
    seL4_SetMR(0, (seL4_Word)spy.p_vaddr); 
    seL4_SetMR(1, (seL4_Word)spy.t_vaddr);
    seL4_Send(s_ep.cptr, info);
}


/*running the single core attack*/ 
static int run_single (ts_t ts) {

    seL4_MessageInfo_t info; 
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 0); 
    int size; 


    while (1) {    
   
        /*spy can now start*/
        info = seL4_Call(s_ep.cptr, tag);
        if (seL4_MessageInfo_get_label(info) != seL4_NoFault)
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
        for (int k = 1; k < TIME_MAX; k++)
            for (int n = ts_get(ts, k); n--; ) 
                printf("%d %d\n", size, k);
    }
    
    print("done the benchmark");
    return BENCH_SUCCESS; 

}


/*entry point of covert channel benchmark*/
void lanuch_bench_covert(m_env_t *env) {
    
    int ret; 

    trojan.image = spy.image = CONFIG_BENCH_THREAD_NAME;
    trojan.vspace = spy.vspace = &env->vspace;

#ifdef CONFIG_CACHE_COLOURING
    trojan.kernel = env->kernel_colour[0].image.cptr; 
    spy.kernel = env->kernel_colour[1].image.cptr;
    trojan.vka = &env->vka_colour[0]; 
    spy.vka = &env->vka_colour[1]; 
 
#else 
    spy.vka = trojan.vka = &env->vka; 
#endif
    init_single(env); 

    /*map the buffers into spy and trojan threads*/ 
    map_init_frames(env, (void *)t_buf, CACHESIZE, &trojan);
    map_init_frames(env, (void *)p_buf, EBSIZE, &spy); 
 
    /*pages for spy record, ts structure*/
    env->record_vaddr = vspace_new_pages(&env->vspace, seL4_AllRights, 
            BENCH_COVERT_TIME_PAGES, PAGE_BITS_4K); 
    assert(env->record_vaddr != NULL);
    
    map_r_buf(env, BENCH_COVERT_TIME_PAGES, &spy);

    /*send running environment*/
    send_env(env); 

    /*run bench*/
    ret = run_single((ts_t)env->record_vaddr);
    assert(ret == BENCH_SUCCESS);
}

#endif 
