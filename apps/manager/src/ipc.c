#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <vka/object.h>
#include <vka/capops.h>
#if 0
#ifdef CONFIG_KERNEL_STABLE 
#include <simple-stable/simple-stable.h>
#else 
#include <simple-default/simpe-default.h>
#endif
#endif 
#include <sel4platsupport/platsupport.h>
#include "ipc.h"
#include "../../bench_common.h"
#include "manager.h"
#include "util/ansi.h"

static ccnt_t get_result(seL4_CPtr ep) {
//    seL4_Wait(ep, NULL);
 //   return seL4_GetMR(0);
    return 0;
}
/*FIXME the ep and reply ep need to be freed*/
#undef IPC_BENCH_PRINTOUT

static m_env_t *ipc_env; 

/*send msg to a process via ep*/
/*FIXME: currently support single word msg only*/
static inline 
void send_msg_to(seL4_CPtr ep, seL4_Word w) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    
    seL4_SetMR(0, w); 
    seL4_Send(ep, info);
}



vka_object_t ipc_ep; 
vka_object_t ipc_reply_ep;
vka_object_t ipc_null_ep; 
vka_t *ipc_vka; 

/*the benchmarking enviornment for two ipc threads*/
static bench_env_t thread1, thread2; 

struct bench_results results;
ipc_test_pmu_t pmu_results; 
ipc_pmu_t *pmu_v; 
/*round trip test result*/
ipc_rt_result_t *rt_v; 

static void print_all(ccnt_t *array) {
    uint32_t i;
    for (i = 0; i < IPC_RUNS; i++) {
        printf("\t"CCNT_FORMAT"\n", array[i]);
    }
}

static ccnt_t results_min(ccnt_t *array) {
    uint32_t i;
    ccnt_t min = array[0];
    for (i = 1; i < IPC_RUNS; i++) {
        if (array[i] < min)
            min = array[i];
    }
    return min;
}



static int results_stable(ccnt_t *array) {
    uint32_t i;
    for (i = 1; i < IPC_RUNS; i++) {
        if (array[i] != array[i - 1]) {
            return 0;
        }
    }
    return 1;
}


static int process_result(ccnt_t *array, const char *error) {
    if (!results_stable(array)) {
        printf("%s\n", error);
        print_all(array);
    }
    return results_min(array);
}

int process_results(struct bench_results *results) {
    results->call_cycles_intra = process_result(results->call_time_intra,
                                             "Intra-AS Call cycles are not stable");
    results->reply_wait_cycles_intra = process_result(results->reply_wait_time_intra,
                                             "Intra-AS ReplyRecv cycles are not stable");
    results->call_cycles_inter = process_result(results->call_time_inter,
                                             "Inter-AS Call cycles are not stable");
    results->reply_wait_cycles_inter = process_result(results->reply_wait_time_inter,
                                             "Inter-AS ReplyRecv cycles are not stable");
    results->call_cycles_inter_low = process_result(results->call_time_inter_low,
                                             "Inter-AS Call (Low to High) cycles are not stable");
    results->call_cycles_inter_high = process_result(results->call_time_inter_high,
                                             "Inter-AS Call (High to Call) cycles are not stable");
    results->reply_wait_cycles_inter_low = process_result(results->reply_wait_time_inter_low,
                                             "Inter-AS ReplyRecv (Low to High) cycles are not stable");
    results->reply_wait_cycles_inter_high = process_result(results->reply_wait_time_inter_high,
                                             "Inter-AS ReplyRecv (High to Call) cycles are not stable");
    results->send_cycles_inter = process_result(results->send_time_inter,
                                             "Inter-AS Send cycles are not stable");
    results->call_10_cycles_inter = process_result(results->call_10_time_inter,
                                             "Inter-AS Call(10) cycles are not stable");
    results->reply_wait_10_cycles_inter = process_result(results->reply_wait_10_time_inter,
                                             "Inter-AS ReplyRecv(10) cycles are not stable");
    return 1;
}

void print_results(struct bench_results *results) {
    printf("\t<result name = \"Intra-AS Call\">"CCNT_FORMAT"</result>\n",results->call_cycles_intra);
    printf("\t<result name = \"Intra-AS ReplyRecv\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_intra);
    printf("\t<result name = \"Inter-AS Call\">"CCNT_FORMAT"</result>\n",results->call_cycles_inter);
    printf("\t<result name = \"Inter-AS ReplyRecv\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_inter);
    printf("\t<result name = \"Inter-AS Call (Low to High)\">"CCNT_FORMAT"</result>\n",results->call_cycles_inter_low);
    printf("\t<result name = \"Inter-AS ReplyRecv (High to Low)\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_inter_high);
    printf("\t<result name = \"Inter-AS Call (High to Low)\">"CCNT_FORMAT"</result>\n",results->call_cycles_inter_high);
    printf("\t<result name = \"Inter-AS ReplyRecv (Low to High)\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_inter_low);
    printf("\t<result name = \"Inter-AS Send\">"CCNT_FORMAT"</result>\n",results->send_cycles_inter);
    printf("\t<result name = \"Inter-AS Call(10)\">"CCNT_FORMAT"</result>\n",results->call_10_cycles_inter);
    printf("\t<result name = \"Inter-AS ReplyRecv(10)\">"CCNT_FORMAT"</result>\n",results->reply_wait_10_cycles_inter);
}

void print_pmu_results(ccnt_t r[][BENCH_PMU_COUNTERS]) {

    for (int i = 0; i < IPC_RUNS; i++) { 

        for (int j = 0; j < BENCH_PMU_COUNTERS; j++) 
            printf("\t"CCNT_FORMAT, r[i][j]);

        printf("\n"); 
    }
}

void process_pmu_results (ipc_test_pmu_t *results) {

    printf("inter-as call: \n"); 
    print_pmu_results(results->call_time_inter); 

    printf("inter-as reply wait: \n"); 
    print_pmu_results(results->reply_wait_time_inter); 

}

void create_benchmark_process(bench_env_t *t) {
        
    sel4utils_process_t *process = &t->process; 
    cspacepath_t src;
    seL4_CPtr ep_arg;
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

#if 0
    vka_cspace_make_path(t->ipc_vka, t->reply_ep.cptr, &src);  
    reply_ep_arg = sel4utils_copy_cap_to_process(process, src);
    assert(reply_ep_arg);
#endif
    void *vaddr = vspace_map_pages(&process->vspace, t->record_frames, 
            NULL, seL4_AllRights, BENCH_PMU_PAGES, PAGE_BITS_4K, 1);
    assert(vaddr); 
 
    sprintf(arg_str0, "%d", t->test_num); 
    sprintf(arg_str1, "%d", ep_arg); 
    sprintf(arg_str2, "%d", (unsigned int)vaddr); 

#if CONFIG_LIB_SEL4_CACHECOLOURING
    /*configure kernel image*/
    //bind_kernel_image(t->kernel,
      //      process->pd.cptr, process->thread.tcb.cptr);
#endif

   /*create a  process*/ 
    error = sel4utils_spawn_process_v(process, t->vka, 
            t->vspace, argc, argv, 0);
    assert(error == 0);

#if (CONFIG_MAX_NUM_NODES > 1)
    /*assign the affinity*/ 
    error = seL4_TCB_SetAffinity(process->thread.tcb.cptr, t->affinity);
    assert(error == 0); 
#endif 

#if 0 
   /*a never returned ep*/
    vka_cspace_make_path(t->ipc_vka, t->null_ep.cptr, &src);  
    null_ep = sel4utils_copy_cap_to_process(process, src);
    assert(null_ep);

    send_msg_to(t->reply_ep.cptr, (seL4_Word)null_ep); 
   send_msg_to(t->reply_ep.cptr, (seL4_Word)vaddr); 
#endif 
}


void ipc_destroy_process(bench_env_t *t1, bench_env_t *t2) {

    /*destory the two processes used by ipc benchmarks*/
    sel4utils_destroy_process(&t1->process, t1->vka); 
    sel4utils_destroy_process(&t2->process, t2->vka);

}


void ipc_alloc_eps(vka_t *vka) {
    
    int error __attribute__((unused)); 
    /*create 2 end points: ep and reply ep*/
    error = vka_alloc_endpoint(vka, &ipc_ep);
    assert(error == 0); 

    printf("ipc alloc eps: ep paddr 0x%x\n", vka_object_paddr(vka, &ipc_ep)); 

    error = vka_alloc_endpoint(vka, &ipc_reply_ep);
    assert(error == 0); 

    error = vka_alloc_endpoint(vka, &ipc_null_ep);
    assert(error == 0); 


}
void ipc_delete_eps(vka_t *vka) {

    vka_free_object(vka, &ipc_ep); 
    vka_free_object(vka, &ipc_reply_ep); 
    vka_free_object(vka, &ipc_null_ep); 
}

void print_overhead(void) {

     printf("call_reply_wait_overhead"CCNT_FORMAT" send_wait_overhead "CCNT_FORMAT" call_reply_wait_10_overhead "CCNT_FORMAT"  \n",
             results.call_reply_wait_overhead, 
             results.send_wait_overhead, 
             results.call_reply_wait_10_overhead); 
    

}
void ipc_overhead (bench_env_t *thread) {
    sel4utils_process_t *process = &thread->process; 
    int error; 
    cspacepath_t src, dest; 
    int ret; 
    thread->prio = IPC_PROCESS_PRIO; 
    thread->test_num = IPC_OVERHEAD; 
    /*copy the caps to map into the remote process*/
    vka_cspace_make_path(&ipc_env->vka, vspace_get_cap(&ipc_env->vspace, 
                ipc_env->record_vaddr), &src);


    ret = vka_cspace_alloc(&ipc_env->vka, &thread->record_frames[0]); 
    assert(ret == 0); 

    vka_cspace_make_path(&ipc_env->vka, thread->record_frames[0], &dest); 
    ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
    assert(ret == 0); 

 
    create_benchmark_process(thread); 
    /*resume the thread*/ 
    error = seL4_TCB_Resume(process->thread.tcb.cptr); 
    assert(error == 0); 
    
    COMPILER_BARRIER;       
    while (rt_v->call_reply_wait_overhead == 0); 
    COMPILER_BARRIER; 
    
    printf("call reply wait overhead "CCNT_FORMAT"\n", rt_v->call_reply_wait_overhead); 

    error = seL4_TCB_Suspend(process->thread.tcb.cptr); 
    assert(error == 0); 
     
    /*destory the two processes used by ipc benchmarks*/
    //sel4utils_destroy_process(&thread->process, thread->vka);
}


void ipc_reply_wait_time_inter(bench_env_t *t1, bench_env_t *t2,
        ccnt_t *result, ccnt_t *pmu_counters) {

    ccnt_t end, start; 

//    ipc_alloc_eps(vka0); 

    t1->test_num = IPC_CALL; 
    t2->test_num = IPC_REPLY_WAIT; 
    create_benchmark_process(t1); 
    create_benchmark_process(t2); 

    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start;
#ifdef IPC_BENCH_PRINTOUT
    printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
#endif 
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    /*get pmu counter value*/
    for (int i = 0; i < BENCH_PMU_COUNTERS; i++) {

        assert(pmu_v->pmuc[IPC_CALL][i] >=
                pmu_v->pmuc[IPC_REPLY_WAIT][i]);

        pmu_counters[i] = pmu_v->pmuc[IPC_CALL][i] - 
            pmu_v->pmuc[IPC_REPLY_WAIT][i];  

    }
#endif 

    ipc_destroy_process(t1, t2); 
    //ipc_delete_eps(vka0);
}

void ipc_reply_wait_10_time_inter(bench_env_t *t1, bench_env_t *t2, ccnt_t *result) {

    ccnt_t end, start; 

    //ipc_alloc_eps(vka0); 
    t1->test_num = IPC_CALL_10; 
    t2->test_num = IPC_REPLY_WAIT_10; 
    create_benchmark_process(t1); 
    create_benchmark_process(t2); 
    
    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 
#ifdef IPC_BENCH_PRINTOUT
    printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
#endif 
    ipc_destroy_process(t1, t2);

    //ipc_delete_eps(vka0);
}
void ipc_call_time_inter(bench_env_t *t1, bench_env_t *t2, 
        ccnt_t *result, 
        ccnt_t *pmu_counters) {

    ccnt_t end, start; 
    
    //ipc_alloc_eps(vka0); 
    t1->test_num = IPC_CALL2; 
    t2->test_num = IPC_REPLY_WAIT2; 
    create_benchmark_process(t1);
    create_benchmark_process(t2);

    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);

    assert(end > start); 
    *result = end - start; 
    printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
#ifdef IPC_BENCH_PRINTOUT
    printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
#endif 
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    /*get pmu counter value*/
    for (int i = 0; i < BENCH_PMU_COUNTERS; i++) {

        assert(pmu_v->pmuc[IPC_REPLY_WAIT2][i] >=
                pmu_v->pmuc[IPC_CALL2][i]);

        pmu_counters[i] = pmu_v->pmuc[IPC_REPLY_WAIT2][i] - 
            pmu_v->pmuc[IPC_CALL2][i];  

    }
#endif 
    
    ipc_destroy_process(t1, t2);
    //ipc_delete_eps(vka0); 
}

/*latency of kernel switching*/
void ipc_kernel_latency_inter(bench_env_t *t1, bench_env_t *t2) {

    sel4utils_process_t *process = NULL; 
    int error;
    cspacepath_t src, dest; 
    int ret; 

    for (int i = 0; i < IPC_RUNS; i++ ) {
        rt_v->call_time[i] = rt_v->reply_wait_time[i] = 0; 
    }

    /*copy the caps to map into the remote process*/
    vka_cspace_make_path(&ipc_env->vka, vspace_get_cap(&ipc_env->vspace, 
                ipc_env->record_vaddr), &src);


    ret = vka_cspace_alloc(&ipc_env->vka, &t1->record_frames[0]); 
    assert(ret == 0); 

    vka_cspace_make_path(&ipc_env->vka, t1->record_frames[0], &dest); 
    ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
    assert(ret == 0); 

 
    ret = vka_cspace_alloc(&ipc_env->vka, &t2->record_frames[0]); 
    assert(ret == 0); 

    vka_cspace_make_path(&ipc_env->vka, t2->record_frames[0], &dest); 
    ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
    assert(ret == 0);    


    //ipc_alloc_eps(vka0); 
    t1->test_num = IPC_LATENCY_CALL; 
    t2->test_num = IPC_LATENCY_REPLY_WAIT; 
    /*t1 has the sensitive attribute, with holdTime configured*/
    create_benchmark_process(t1);
    create_benchmark_process(t2);

    printf("kernel switching latency observed at user-level:\n");
    /*resume the thread*/ 
    process = &t2->process;
    error = seL4_TCB_Resume(process->thread.tcb.cptr); 
    assert(error == 0); 
    process = &t1->process;
    error = seL4_TCB_Resume(process->thread.tcb.cptr); 
    assert(error == 0); 


    for (int i = 0; i < IPC_RUNS; i++ ) {
        COMPILER_BARRIER; 
        while (rt_v->reply_wait_time[i] == 0); 
        COMPILER_BARRIER; 
        printf(" "CCNT_FORMAT"  ", rt_v->reply_wait_time[i] - rt_v->call_time[i]); 
    }
    printf("\n"); 
    process = &t2->process;
    error = seL4_TCB_Suspend(process->thread.tcb.cptr); 
    assert(error == 0); 
    process = &t1->process;
    error = seL4_TCB_Suspend(process->thread.tcb.cptr);    
 

}


/*round trip call performance*/
void ipc_rt_call_time_inter(bench_env_t *t1, bench_env_t *t2, 
        ccnt_t *result, 
        ccnt_t *pmu_counters) {
    sel4utils_process_t *process = NULL; 
    int error;
    cspacepath_t src, dest; 
    int ret; 

    for (int i = 0; i < IPC_RUNS; i++ ) {
        rt_v->call_rt_time[i] = 0; 
    }
    /*copy the caps to map into the remote process*/
    vka_cspace_make_path(&ipc_env->vka, vspace_get_cap(&ipc_env->vspace, 
                ipc_env->record_vaddr), &src);


    ret = vka_cspace_alloc(&ipc_env->vka, &t1->record_frames[0]); 
    assert(ret == 0); 

    vka_cspace_make_path(&ipc_env->vka, t1->record_frames[0], &dest); 
    ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
    assert(ret == 0); 

 
    ret = vka_cspace_alloc(&ipc_env->vka, &t2->record_frames[0]); 
    assert(ret == 0); 

    vka_cspace_make_path(&ipc_env->vka, t2->record_frames[0], &dest); 
    ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
    assert(ret == 0);    


    //ipc_alloc_eps(vka0); 
    t1->test_num = IPC_RT_CALL; 
    t2->test_num = IPC_RT_REPLY_WAIT; 
    create_benchmark_process(t1);
    create_benchmark_process(t2);

    /*resume the thread*/ 
    process = &t2->process;
    error = seL4_TCB_Resume(process->thread.tcb.cptr); 
    assert(error == 0); 
    process = &t1->process;
    error = seL4_TCB_Resume(process->thread.tcb.cptr); 
    assert(error == 0); 
 

    for (int i = 0; i < IPC_RUNS; i++ ) {
        COMPILER_BARRIER; 
        while (rt_v->call_rt_time[i] == 0); 
        COMPILER_BARRIER; 
        printf("result "CCNT_FORMAT"  \n", rt_v->call_rt_time[i]); 
    }
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    /*get pmu counter value*/
    for (int i = 0; i < BENCH_PMU_COUNTERS; i++) {

        assert(pmu_v->pmuc[IPC_REPLY_WAIT2][i] >=
                pmu_v->pmuc[IPC_CALL2][i]);

        pmu_counters[i] = pmu_v->pmuc[IPC_REPLY_WAIT2][i] - 
            pmu_v->pmuc[IPC_CALL2][i];  

    }
#endif 
    process = &t2->process;
    error = seL4_TCB_Suspend(process->thread.tcb.cptr); 
    assert(error == 0); 
    process = &t1->process;
    error = seL4_TCB_Suspend(process->thread.tcb.cptr);    
    //ipc_destroy_process(t1, t2);
    //ipc_delete_eps(vka0); 
}


void ipc_call_10_time_inter(bench_env_t *t1, bench_env_t *t2,
        ccnt_t *result) {

    ccnt_t end, start; 
    
   // ipc_alloc_eps(vka0); 
    t1->test_num = IPC_CALL2_10; 
    t2->test_num = IPC_REPLY_WAIT2_10; 
    create_benchmark_process(t1); 
    create_benchmark_process(t2); 

    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 
#ifdef IPC_BENCH_PRINTOUT
    printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
#endif 
    ipc_destroy_process(t1, t2); 

    //ipc_delete_eps(vka0); 
}
void ipc_send_time_inter(bench_env_t *t1, bench_env_t *t2, 
        ccnt_t *result) {

    ccnt_t end, start; 
    //ipc_alloc_eps(vka0); 

    t1->test_num = IPC_SEND; 
    t2->test_num = IPC_WAIT; 
    create_benchmark_process(t1);
    create_benchmark_process(t2);

    start = get_result(ipc_reply_ep.cptr);
    end = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 
#ifdef IPC_BENCH_PRINTOUT
    printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
#endif 
    ipc_destroy_process(t1, t2); 

    //ipc_delete_eps(vka0); 
}


static void ipc_benchmark (bench_env_t *thread1, bench_env_t *thread2) {


    /*set up one thread to measure the overhead*/
    ccnt_t result;
 
#ifdef IPC_BENCH_PRINTOUT
    printf("Doing benchmarks...\n\n");
#endif
    
    ipc_overhead(thread1); 

    //print_overhead(); 
    
    /*round trip ipc performance*/
    ipc_rt_call_time_inter(thread1, thread2, &result,
            pmu_results.reply_wait_time_inter[0]);


#if 0
    for (i = 0; i < IPC_RUNS; i++) {
#ifdef IPC_BENCH_PRINTOUT
        printf("\tDoing iteration %d\n",i);
#endif  

#ifdef CONFIG_MANAGER_PMU_COUNTER 
        memset(pmu_v, 0, BENCH_PMU_PAGES * BIT(PAGE_BITS_4K)); 
        sel4bench_reset_counters(BENCH_PMU_BITS); 
#endif

#ifdef IPC_BENCH_PRINTOUT
        printf("Running Call+ReplyRecv Inter-AS test 1\n");
#endif 
        thread1->prio = thread2->prio = IPC_PROCESS_PRIO; 
        ipc_reply_wait_time_inter(thread1, thread2, &result, 
                pmu_results.reply_wait_time_inter[i]);
        results.reply_wait_time_inter[i] = result - results.call_reply_wait_overhead;

#ifdef IPC_BENCH_PRINTOUT
        printf("Running Call+ReplyRecv Inter-AS test 2\n");
#endif
        ipc_call_time_inter(thread1, thread2, &result, 
                pmu_results.call_time_inter[i]); 
        results.call_time_inter[i] = result - results.call_reply_wait_overhead;

#ifdef IPC_BENCH_PRINTOUT 
        printf("Running Send test\n");
#endif 
        ipc_send_time_inter(thread1, thread2, &result); 
        results.send_time_inter[i] = result - results.send_wait_overhead;

#ifdef IPC_BENCH_PRINTOUT
        printf("Running Call+ReplyRecv long message test 1\n");
#endif 
        ipc_reply_wait_10_time_inter(thread1, thread2, &result);
        results.reply_wait_10_time_inter[i] = result - results.call_reply_wait_10_overhead;

#ifdef IPC_BENCH_PRINTOUT
        printf("Running Call+ReplyRecv long message test 2\n");
#endif 
        ipc_call_10_time_inter(thread1, thread2, &result); 
        results.call_10_time_inter[i] = result - results.call_reply_wait_10_overhead;
#ifdef IPC_BENCH_PRINTOUT
        printf("Running Call+ReplyRecv Different prio test 1\n");
#endif 
        thread1->prio = IPC_PROCESS_PRIO_LOW; 
        thread2->prio = IPC_PROCESS_PRIO_HIGH; 
        ipc_reply_wait_time_inter(thread1, thread2, &result,
                pmu_results.reply_wait_time_inter_high[i]); 
        results.reply_wait_time_inter_high[i] = result - results.call_reply_wait_overhead;
        
#ifdef IPC_BENCH_PRINTOUT 
        printf("Running Call+ReplyRecv Different prio test 2\n");
#endif 
        ipc_call_time_inter(thread1, thread2, &result,
                pmu_results.call_time_inter_low[i]); 
        results.call_time_inter_low[i] = result -  results.call_reply_wait_overhead;
#ifdef IPC_BENCH_PRINTOUT
        printf("Running Call+ReplyRecv Different prio test 3\n");
#endif 
        thread1->prio = IPC_PROCESS_PRIO_HIGH; 
        thread2->prio = IPC_PROCESS_PRIO_LOW; 
        ipc_reply_wait_time_inter(thread1, thread2, &result,
                pmu_results.reply_wait_time_inter_low[i]); 
        results.reply_wait_time_inter_low[i] = result - results.call_reply_wait_overhead;

#ifdef IPC_BENCH_PRINTOUT
        printf("Running Call+ReplyRecv Different prio test 4\n");
#endif  
        ipc_call_time_inter(thread1, thread2, &result,
                pmu_results.call_time_inter_high[i]); 
        results.call_time_inter_high[i] = result - results.call_reply_wait_overhead;
    }

#endif 
   // process_results(&results); 
    //print_results(&results);
#ifdef CONFIG_MANAGER_PMU_COUNTER 
    process_pmu_results(&pmu_results); 
#endif 

}

/*lanuching the ipc bench on various cores*/
static void multi_bench_ipc(bench_env_t *t1, bench_env_t *t2) {


    /*t1 and t2 on the same core*/
    for (int i = 1; i < CONFIG_MAX_NUM_NODES; i++) {

        printf("assign T1 to core %d T2 to core %d\n", i, i); 
        printf("=========================================\n"); 
        t1->affinity = t2->affinity = i; 
#if 0
        seL4_KernelImage_Sensitive(t1->kernel);
#endif 
        ipc_benchmark(t1, t2); 
    }
#if 0

    /*t1 and t2 on different core, except only one core aviliable*/
    if (CONFIG_MAX_NUM_CORES == 1) 
        return; 
    
    for (int i = 0; i < CONFIG_MAX_NUM_CORES - 1; i++) {

        printf("assign T1 to core %d T2 to core %d\n", i, i + 1); 
        printf("=========================================\n"); 
        t1->affinity = i; 
        t2->affinity = i + 1; 
        ipc_benchmark(t1, t2); 
    }
#endif
}


void multi_bench_kernel_latency(bench_env_t *t1, bench_env_t *t2) {

    uint32_t low = 0 , high = 0; 

    printf("Test the kernel switching latency\n");
    printf("assign T1 (high) to core 1 T2 (low) to core 1 \n"); 
    printf("=========================================\n"); 
    t1->affinity = t2->affinity = 1;

#if 0
    seL4_KernelImage_Sensitive(t1->kernel);
#endif
    for (int i = 0; i < NLATENCY; i++) {
        /*set the latency*/ 
        printf("set the caller kernel latency to %d %d \n", low, high);

#if 0 
        seL4_KernelImage_HoldTime(t1->kernel, low, high);
#endif 
        ipc_kernel_latency_inter(t1, t2); 

        low += 10000; 
    }
    
    

}

/*entry point for ipc benchmarks */
void launch_bench_ipc(m_env_t *env) {

    thread2.image = thread1.image = CONFIG_BENCH_THREAD_NAME;
    thread2.vspace = thread1.vspace = &env->vspace;
    ipc_env = env; 


    /*create frames that act as record buffer, mapping 
     to benchmark processes*/
    env->record_vaddr = vspace_new_pages(&env->vspace, seL4_AllRights, 
            BENCH_PMU_PAGES, PAGE_BITS_4K); 
    assert(env->record_vaddr != NULL); 
   // pmu_v = env->record_vaddr; 
    rt_v = env->record_vaddr; 


#ifdef CONFIG_LIB_SEL4_CACHECOLOURING
    thread1.kernel = env->kimages[0].ki.cptr; 
    thread2.kernel = env->kimages[1].ki.cptr; 
    thread1.vka = &env->vka_colour[0]; 
    thread2.vka = &env->vka_colour[1]; 
    /*FIXME: regarding the thread 1 with low classification level*/
    
    ipc_alloc_eps(&env->vka_colour[0]);
    thread2.ipc_vka = thread1.ipc_vka = &env->vka_colour[0]; 
    thread2.ep = thread1.ep = ipc_ep;
    thread2.reply_ep = thread1.reply_ep = ipc_reply_ep;
    thread2.null_ep = thread1.null_ep = ipc_null_ep; 

    printf("\n"); 
    printf("ipc inter colour benchmarks\n");
    printf("========================\n");
    
    multi_bench_ipc(&thread1, &thread2); 

    multi_bench_kernel_latency(&thread1, &thread2);

    printf("\n"); 
    printf("ipc intra colour benchmarks\n");
    printf("========================\n");
    thread2.kernel = thread1.kernel = env->kimages[0].ki.cptr; 
    thread2.vka = thread1.vka = &env->vka_colour[0]; 
    multi_bench_ipc(&thread1, &thread2); 

    printf("\n"); 
    printf("ipc no colour benchmarks\n");
    printf("========================\n");
    ipc_alloc_eps(&env->vka);
    thread2.ipc_vka = thread1.ipc_vka = &env->vka; 
    
    thread2.ep = thread1.ep = ipc_ep;
    thread2.reply_ep = thread1.reply_ep = ipc_reply_ep;
    thread2.null_ep = thread1.null_ep = ipc_null_ep; 

    thread2.kernel = thread1.kernel = env->kernel;
    thread2.vka = thread1.vka = &env->vka; 
    multi_bench_ipc(&thread1, &thread2); 

#else
    thread1.vka = &env->vka; 
    thread2.vka = &env->vka; 
    ipc_alloc_eps (&env->vka); 
    thread2.ipc_vka = thread1.ipc_vka = &env->vka; 

    thread2.ep = thread1.ep = ipc_ep;
    thread2.reply_ep = thread1.reply_ep = ipc_reply_ep;
    thread2.null_ep = thread1.null_ep = ipc_null_ep; 

   
    printf("\n"); 
    printf("ipc original kernel benchmarks\n");
    printf("========================\n");
    multi_bench_ipc(&thread1, &thread2); 



#endif
}
