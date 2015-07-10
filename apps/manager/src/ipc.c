#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <vka/object.h>
#include <simple-stable/simple-stable.h>
#include <sel4platsupport/platsupport.h>
#include "ipc.h"
#include "../../bench_common.h"
#include "manager.h"
//#include "util/ansi.h"

#if defined(CCNT32BIT)
static ccnt_t get_result(seL4_CPtr ep) {
    seL4_Wait(ep, NULL);
    return seL4_GetMR(0);
}
#elif defined(CCNT64BIT)
static ccnt_t get_result(seL4_CPtr ep) {
    seL4_Wait(ep, NULL);
    return ( ((ccnt_t)seL4_GetMR(0)) << 32ull) | ((ccnt_t)seL4_GetMR(1));
}
#else
#error Unknown ccnt size
#endif
/*FIXME the ep and reply ep need to be freed*/

vka_object_t ipc_ep; 
vka_object_t ipc_reply_ep;
/*two kernel objects used by ipc benchmarking*/
seL4_CPtr kernel1, kernel2;
/*the benchmarking enviornment for two ipc threads*/
bench_env_t thread1, thread2; 

struct bench_results results;

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

static int process_results(struct bench_results *results) {
    results->call_cycles_intra = process_result(results->call_time_intra,
                                             "Intra-AS Call cycles are not stable");
    results->reply_wait_cycles_intra = process_result(results->reply_wait_time_intra,
                                             "Intra-AS ReplyWait cycles are not stable");
    results->call_cycles_inter = process_result(results->call_time_inter,
                                             "Inter-AS Call cycles are not stable");
    results->reply_wait_cycles_inter = process_result(results->reply_wait_time_inter,
                                             "Inter-AS ReplyWait cycles are not stable");
    results->call_cycles_inter_low = process_result(results->call_time_inter_low,
                                             "Inter-AS Call (Low to High) cycles are not stable");
    results->call_cycles_inter_high = process_result(results->call_time_inter_high,
                                             "Inter-AS Call (High to Call) cycles are not stable");
    results->reply_wait_cycles_inter_low = process_result(results->reply_wait_time_inter_low,
                                             "Inter-AS ReplyWait (Low to High) cycles are not stable");
    results->reply_wait_cycles_inter_high = process_result(results->reply_wait_time_inter_high,
                                             "Inter-AS ReplyWait (High to Call) cycles are not stable");
    results->send_cycles_inter = process_result(results->send_time_inter,
                                             "Inter-AS Send cycles are not stable");
    results->call_10_cycles_inter = process_result(results->call_10_time_inter,
                                             "Inter-AS Call(10) cycles are not stable");
    results->reply_wait_10_cycles_inter = process_result(results->reply_wait_10_time_inter,
                                             "Inter-AS ReplyWait(10) cycles are not stable");
    return 1;
}

static void print_results(struct bench_results *results) {
    printf("\t<result name = \"Intra-AS Call\">"CCNT_FORMAT"</result>\n",results->call_cycles_intra);
    printf("\t<result name = \"Intra-AS ReplyWait\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_intra);
    printf("\t<result name = \"Inter-AS Call\">"CCNT_FORMAT"</result>\n",results->call_cycles_inter);
    printf("\t<result name = \"Inter-AS ReplyWait\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_inter);
    printf("\t<result name = \"Inter-AS Call (Low to High)\">"CCNT_FORMAT"</result>\n",results->call_cycles_inter_low);
    printf("\t<result name = \"Inter-AS ReplyWait (High to Low)\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_inter_high);
    printf("\t<result name = \"Inter-AS Call (High to Low)\">"CCNT_FORMAT"</result>\n",results->call_cycles_inter_high);
    printf("\t<result name = \"Inter-AS ReplyWait (Low to High)\">"CCNT_FORMAT"</result>\n",results->reply_wait_cycles_inter_low);
    printf("\t<result name = \"Inter-AS Send\">"CCNT_FORMAT"</result>\n",results->send_cycles_inter);
    printf("\t<result name = \"Inter-AS Call(10)\">"CCNT_FORMAT"</result>\n",results->call_10_cycles_inter);
    printf("\t<result name = \"Inter-AS ReplyWait(10)\">"CCNT_FORMAT"</result>\n",results->reply_wait_10_cycles_inter);
}



void create_benchmark_process(sel4utils_process_t *process, 
        vka_t *vka, vspace_t *vspace, 
        int prio, int no, 
        char *image, seL4_CPtr kernel) {

    cspacepath_t src;
    seL4_CPtr ep_arg, reply_ep_arg; 
    int argc = 3;
    char arg_str0[15] = {0}; 
    char arg_str1[15] = {0}; 
    char arg_str2[15] = {0}; 

    char *argv[3] = {arg_str0, arg_str1, arg_str2}; 
    
    int error; 
    /*configure process*/ 
    error = sel4utils_configure_process(process, vka, vspace, prio, image); 
    assert(error == 0); 

    vka_cspace_make_path(vka, ipc_ep.cptr, &src);
    ep_arg = sel4utils_copy_cap_to_process(process, src);
    assert(ep_arg); 

    vka_cspace_make_path(vka, ipc_reply_ep.cptr, &src);
    reply_ep_arg = sel4utils_copy_cap_to_process(process, src);
    assert(reply_ep_arg);

    sprintf(arg_str0, "%d", no); 
    sprintf(arg_str1, "%d", ep_arg); 
    sprintf(arg_str2, "%d", reply_ep_arg); 
#if CONFIG_CACHE_COLOURING
    /*configure kernel image*/
    bind_kernel_image(kernel, process->pd.cptr, process->thread.tcb.cptr)
#endif
   /*start process*/ 
    error = sel4utils_spawn_process_v(process, vka, vspace, argc, argv, 1);
    assert(error == 0); 
    
}


void ipc_destory_process(vka_t *vka0, vka_t *vka1) {

    /*destory the two processes used by ipc benchmarks*/
    sel4utils_destroy_process(&ipc_process[0], vka0); 
    sel4utils_destroy_process(&ipc_process[1], vka1);

}


void ipc_alloc_eps(vka_t *vka) {
    
    int error; 
    /*create 2 end points: ep and reply ep*/
    error = vka_alloc_endpoint(vka, &ipc_ep);
    assert(error == 0); 
    
    error = vka_alloc_endpoint(vka, &ipc_reply_ep);
    assert(error == 0); 


}
void ipc_delete_eps(vka_t *vka) {

    vka_free_object(vka, &ipc_ep); 
    vka_free_object(vka, &ipc_reply_ep); 

}

void ipc_overhead (bench_env_t *thread) {


    create_benchmark_process(&ipc_process[0], vka0, vspace, IPC_PROCESS_PRIO, IPC_OVERHEAD, image, kernel); 

    results.call_reply_wait_overhead = get_result(ipc_reply_ep.cptr);
    results.send_wait_overhead = get_result(ipc_reply_ep.cptr);
    results.call_reply_wait_10_overhead = get_result(ipc_reply_ep.cptr);
    
    /*destory the two processes used by ipc benchmarks*/
    sel4utils_destroy_process(&ipc_process[0], vka0); 

}


void ipc_reply_wait_time_inter(vspace_t *vspace, vka_t *vka0, vka_t *vka1, 
        char *image, unsigned int prio0, unsigned int prio1, ccnt_t *result) {

    ccnt_t end, start; 

    ipc_alloc_eps(vka0); 

    create_benchmark_process(&ipc_process[0], vka0, vspace, prio0, IPC_CALL, image); 
    create_benchmark_process(&ipc_process[1], vka1, vspace, prio1, IPC_REPLY_WAIT, image); 

    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 
    //printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
   

    ipc_destory_process(vka0, vka1); 

    //ipc_delete_eps(vka0);
}

void ipc_reply_wait_10_time_inter(vspace_t *vspace, vka_t *vka0, vka_t *vka1, char *image, unsigned int prio0, unsigned int prio1, ccnt_t *result) {

    ccnt_t end, start; 

    ipc_alloc_eps(vka0); 

    create_benchmark_process(&ipc_process[0], vka0, vspace, prio0, IPC_CALL_10, image); 
    create_benchmark_process(&ipc_process[1], vka1, vspace, prio1, IPC_REPLY_WAIT_10, image); 

    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 
//    printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 
    
    ipc_destory_process(vka0, vka1); 

    //ipc_delete_eps(vka0);
}
void ipc_call_time_inter(vspace_t *vspace, vka_t *vka0, vka_t *vka1, char *image,unsigned int prio0, unsigned int prio1, ccnt_t *result) {

    ccnt_t end, start; 
    
    ipc_alloc_eps(vka0); 


    create_benchmark_process(&ipc_process[0], vka0, vspace, prio0, IPC_CALL2, image); 
    create_benchmark_process(&ipc_process[1], vka1, vspace, prio1, IPC_REPLY_WAIT2, image); 

    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 

    //printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 

    
    ipc_destory_process(vka0, vka1);
    //ipc_delete_eps(vka0); 


}


void ipc_call_10_time_inter(vspace_t *vspace, vka_t *vka0, vka_t *vka1, char *image,unsigned int prio0, unsigned int prio1, ccnt_t *result) {

    ccnt_t end, start; 
    
    ipc_alloc_eps(vka0); 


    create_benchmark_process(&ipc_process[0], vka0, vspace, prio0, IPC_CALL2_10, image); 
    create_benchmark_process(&ipc_process[1], vka1, vspace, prio1, IPC_REPLY_WAIT2_10, image); 

    end = get_result(ipc_reply_ep.cptr);
    start = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 

   // printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 

    
    ipc_destory_process(vka0, vka1);
    //ipc_delete_eps(vka0); 


}
void ipc_send_time_inter(vspace_t *vspace, vka_t *vka0, vka_t *vka1, char *image,unsigned int prio0, unsigned int prio1, ccnt_t *result) {

    ccnt_t end, start; 
    
    ipc_alloc_eps(vka0); 


    create_benchmark_process(&ipc_process[0], vka0, vspace, prio0, IPC_SEND, image); 
    create_benchmark_process(&ipc_process[1], vka1, vspace, prio1, IPC_WAIT, image); 

    start = get_result(ipc_reply_ep.cptr);
    end = get_result(ipc_reply_ep.cptr);
    assert(end > start); 

    *result = end - start; 

    //printf("start"CCNT_FORMAT" end "CCNT_FORMAT" result "CCNT_FORMAT"  \n", start, end, *result); 

    
    ipc_destory_process(vka0, vka1);
    //ipc_delete_eps(vka0); 

}


void ipc_benchmark (bench_env_t *thread1, bench_env_t *thread2) {


    /*set up one thread to measure the overhead*/
    uint32_t i;
    ccnt_t result;
    
    printf("Doing benchmarks...\n\n");

    ipc_overhead(thread1); 

    for (i = 0; i < IPC_RUNS; i++) {
        printf("\tDoing iteration %d\n",i);

        /*one way IPC, reply -> call */
        printf("Running Call+ReplyWait Inter-AS test 1\n");

        ipc_reply_wait_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO, IPC_PROCESS_PRIO, &result); 
        
        results.reply_wait_time_inter[i] = result - results.call_reply_wait_overhead;
        
        printf("Running Call+ReplyWait Inter-AS test 2\n");
       
        ipc_call_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO, IPC_PROCESS_PRIO, &result); 
        results.call_time_inter[i] = result - results.call_reply_wait_overhead;
        printf("Running Call+ReplyWait Different prio test 1\n");
        ipc_reply_wait_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO_LOW, IPC_PROCESS_PRIO_HIGH, &result); 

        results.reply_wait_time_inter_high[i] = result - results.call_reply_wait_overhead;
        printf("Running Call+ReplyWait Different prio test 2\n");
       
        ipc_call_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO_LOW, IPC_PROCESS_PRIO_HIGH, &result); 
        results.call_time_inter_low[i] = result -  results.call_reply_wait_overhead;

        printf("Running Call+ReplyWait Different prio test 3\n");
        ipc_reply_wait_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO_HIGH, IPC_PROCESS_PRIO_LOW, &result); 

        results.reply_wait_time_inter_low[i] = result - results.call_reply_wait_overhead;
        printf("Running Call+ReplyWait Different prio test 4\n");
        ipc_call_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO_HIGH, IPC_PROCESS_PRIO_LOW, &result); 

        results.call_time_inter_high[i] = result - results.call_reply_wait_overhead;
        printf("Running Send test\n");
        ipc_send_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO, IPC_PROCESS_PRIO, &result); 

        results.send_time_inter[i] = result - results.send_wait_overhead;

        printf("Running Call+ReplyWait long message test 1\n");
        ipc_reply_wait_10_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO, IPC_PROCESS_PRIO, &result); 

        results.reply_wait_10_time_inter[i] = result - results.call_reply_wait_10_overhead;
        printf("Running Call+ReplyWait long message test 2\n");

        ipc_call_10_time_inter(vspace, vka0, vka1, image, IPC_PROCESS_PRIO, IPC_PROCESS_PRIO, &result); 

        results.call_10_time_inter[i] = result - results.call_reply_wait_10_overhead;
    }

    process_results(&results); 
    print_results(&results);

}

/*entry point for ipc benchmarks */
void lanuch_bench_ipc(m_env_t *env) {

    printf("\n"); 
    printf("ipc benchmarks\n");
    printf("========================\n");

    thread2.image = thread1.image = CONFIG_BENCH_THREAD_NAME;
    thread2.vspace = thread1.vspace = &env->vspace;

#ifdef CONFIG_CACHE_COLOURING
    thread1.kernel = env->kernel_colour[0].image.cptr; 
    thread2.kernel = env->kernel_colour[1].image.cptr; 
    thread1.vka = &env->vka_colour[0]; 
    thread2.vka = &env->vka_colour[1]; 
    /*regarding the thread 1 with low classification level*/
    ipc_alloc_eps(&env->vka_colour[0]);
#else
    thread1.vka = &env->vka; 
    thread2.vka = &env->vka; 
    ipc_alloc_eps (&env->vka); 
#endif 

    thread2.ep = thread1.ep = ipc_ep.cptr; 
    thread2.ep = thread1.ep = ipc_reply_ep.cptr; 


    ipc_benchmark(&thread1, &thread2); 

}
