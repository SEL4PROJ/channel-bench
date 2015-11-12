/* 
   the original usage of capacity should be:
   /capacity [-1] [-o <outputfile>]
   Where  '-1' runs the same core measurements.
   both single core and multicore is supported
 */
/*the ported version on seL4 should be:
  one sender thread, one receiver thread, communicating 
  through ipc. The output of this test is recorded in 
  a data buffer, collected by root task. */
/*The size changes over all the values from 0 to 1023 (pages).  mask and shift control the order of scanning the 1024 values.  The aim is to avoid repeated patterns within the sizes.  For each combination of mask and shift we get REPS (100) reads for each size.  There are 32 masks and 10 shifts, so one run of the program gives you 32,000 data points for each working set size.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>

#include "../../../bench_common.h"
#include "probe.h"
#include "timestats.h"
#include "sysinfo.h"
#include "trojan.h"

#define LINE 4  /*targeting the 4th cache set in a slice */
#define REPS 100


bench_covert_t covert_env; 

/*reply error message to manager thread*/
static void reply_error(seL4_CPtr r_ep) {

    /*len == 2 is an error msg*/
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 2); 

    seL4_SetMR(0,0); 
    seL4_SetMR(1,0);
    seL4_Send(r_ep, tag);

}

/*entry for capacity benchmark 
opt: single core, multicore 
env: running enviornment 
record: data collection*/
static int capacity_single(bench_covert_t *env) {
    ts_t ts; 
    seL4_MessageInfo_t send = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_MessageInfo_t recv;
    
    /*run torjan never return*/
    if (env->opt == BENCH_COVERT_TROJAN_SINGLE) 
        return trojan_single(env->p_buf, LINE, env->syn_ep); 

    /*perpare the probe buffer*/
    if (opt == BENCH_COVERT_SPY_SINGLE) {
        probe_init_simple(env->p_buf, EBSIZE); 
        /*ts buffer in env*/
        ts = evn->ts_buf;
    }

    /*syn with manager*/ 
    recv = seL4_Wait(env->r_ep, NULL); 
    if (seL4_MessageInfo_get_label(recv) != seL4_NoFault) {
        reply_error(env->r_ep);
        return BENCH_FAILURE; 
    }
    /*span over 0-1023 pages, using mask and shift to aviod 
      repeating patterns, 100 repeats for each size.*/
    for (int mask =0; mask < 32; mask++) 
        for (int shift = 0; shift < 10; shift++) {

            for (int i = 0; i < 1024; i++) {

                /*working set size, the secret msg*/
                int size = i ^ mask <<5 ^ mask;
                size = (size >> shift) ^ (size << (10-shift));
                size &=0x3ff; /*1024 sizes*/
                ts_clear(ts);

                /*single core: prime, trojan, probe*/
                /*prime*/
                probe_sitime(NULL, LINE, 10);
                /*do probe and record time*/
                for (int k = 0; k < REPS; k++) {
                    /*singal*/
                    tr_callslave(env->syn_ep, size);
                    /*probe*/
                    probe_sitime(ts, LINE, 1);
                }
                /*tell the manager that we have 
                  data ready*/
                seL4_SetMR(0, size); 
                recv = seL4_ReplyWait(env->r_ep, send, NULL);

                // Force LRU eviction
                for (int j = 0; j < 2048; j+=64) {
                    probe_sitime(ts, (LINE^j)&0x3f, 100);
                    probe_sitime(ts, (LINE^j)&0x3f, 100);
                    probe_sitime(ts, (LINE^j)&0x3f, 100);
                }
            }
        }
    
    /*never return, manager does not reply, 0 msg len, the end of test*/
    send = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 0);
    recv = seL4_ReplyWait (env->r_ep, send, NULL); 

    return BENCH_SUCCESS; 
}

static int wait_env_from(bench_covert_t *env, seL4_CPtr ep) {

    seL4_Word badge; 
    seL4_MessageInfo_t info; 
    
    info = seL4_Wait(ep, &badge); 

    if (seL4_MessageInfo_get_label(info) != seL4_NoFault)
        return BENCH_FAILURE; 

    if (seL4_MessageInfo_get_length(info) != BENCH_COVERT_MSG_LEN)
        return BENCH_FAILURE; 
    
    env->p_buf = (void *)seL4_GetMR(0);
    env->ts_buf = (void *)seL4_GetMR(1); 
    return BENCH_SUCCESS; 

}

int run_bench_covert(char **argv) {

    covert_env.opt = atol(argv[0]); 
    covert_env.syn_ep = (seL4_CPtr)atol(argv[1]);
    covert_env.r_ep = (seL4_CPtr)atol(argv[2]);

    if (wait_env_from(&covert_env, covert_env.r_ep) != BENCH_SUCCESS) 
        return BENCH_FAILURE; 
    /*run bench*/
    return capacity_single(&covert_env); 

}

