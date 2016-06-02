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
#include <sel4/sel4.h>

#include "../../../bench_common.h"
#include "../../../covert.h"
#include "probe.h"
#include "timestats.h"
#include "trojan.h"
#include "pageset.h"



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
int capacity_single(bench_covert_t *env) {
    ts_t ts = NULL; 
    seL4_MessageInfo_t send = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_MessageInfo_t recv;
    pageset_t ps = NULL;
    int size = 0;

    /*run torjan never return*/
    if (env->opt == BENCH_COVERT_L2_TROJAN) {
        return trojan_single(env->p_buf, LINE, env->syn_ep); 
    }

    /*perpare the probe buffer*/
    if (env->opt == BENCH_COVERT_L2_SPY) {
        probe_init_simple(env->p_buf, EBSIZE); 
        /*ts buffer in env*/
        ts = env->ts_buf;
    }

    /*span over 0-1023 pages, using mask and shift to aviod 
      repeating patterns, N repeats for each size.
     currently covering maximum 4M of the trojan buffer*/
    ps = ps_new(); 
    ps_clear(ps);
    for (int s = 0; s < WORKING_SET_PAGES; s++) 
        ps_push(ps, s); 

    for (int l = 0; l < NUM_COVERT_RUNS; l++) { 

        /*generating a randomized list from 0 to max number 
         of pages*/
        ps_randomise(ps); 

        for (int s = 0; s < WORKING_SET_PAGES; s++) {
            
            size = ps_get(ps, s);
            /*syn with manager*/ 
            recv = seL4_Recv(env->r_ep, NULL); 

            if (seL4_MessageInfo_get_label(recv) != seL4_NoFault) {
                reply_error(env->r_ep);
                return BENCH_FAILURE; 
            } 

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
            seL4_Send(env->r_ep, send); 

            // Force LRU eviction
            /*probing on any possible cache set that 
              trojan may touched, exclude the target set.*/
            /*advance j by 64, which is already covered 
              by pervious page 64 * 64 per cache line. 
              trojan stepping on pages, 
              whereas probe stepping on cache sets.*/
            for (int j = 0; j < NSETS_LRU; j += CACHE_LINES_PER_PAGE) {
                probe_sitime(ts, (LINE^j) & NSETS_MASK, 100);
                probe_sitime(ts, (LINE^j) & NSETS_MASK, 100);
                probe_sitime(ts, (LINE^j) & NSETS_MASK, 100);
            }
        }
    }
    ps_delete(ps); 

#if 0
    for (int mask =0; mask < 32; mask++) 
        for (int shift = 0; shift < 10; shift++) {

            for (int i = 0; i < 1024; i++) {
                /*syn with manager*/ 
                recv = seL4_Recv(env->r_ep, NULL); 

                if (seL4_MessageInfo_get_label(recv) != seL4_NoFault) {
                    reply_error(env->r_ep);
                    return BENCH_FAILURE; 
                } 

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
                seL4_Send(env->r_ep, send); 

                // Force LRU eviction
                /*probing on any possible cache set that 
                 trojan may touched, exclude the target set.*/
                /*advance j by 64, which is already covered 
                 by pervious page 64 * 64 per cache line. 
                 trojan stepping on pages, 
                 whereas probe stepping on cache sets.*/
                for (int j = 0; j < NSETS_LRU; j += CACHE_LINES_PER_PAGE) {
                    probe_sitime(ts, (LINE^j) & NSETS_MASK, 100);
                    probe_sitime(ts, (LINE^j) & NSETS_MASK, 100);
                    probe_sitime(ts, (LINE^j) & NSETS_MASK, 100);
                }
            }
        }
#endif 
    /*never return, manager does not reply, 0 msg len, the end of test*/

    recv = seL4_Recv(env->r_ep, NULL);
    send = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 0);
    seL4_Send(env->r_ep, send); 
    recv = seL4_Recv(env->r_ep, NULL);
    assert(1);
    return BENCH_SUCCESS; 
}



