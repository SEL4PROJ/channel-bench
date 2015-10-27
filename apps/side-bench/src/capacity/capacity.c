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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>


#include "probe.h"
#include "timestats.h"
#include "sysinfo.h"
#include "trojan.h"

#define LINE 4  /*targeting the 4th cache line in pages*/
#define REPS 100

/*running enviorment for bench capability*/
typedef struct bench_cap {
    void *probe_buf; /*private, contiguous buffer, detecting hash*/ /*probe_init_simple*/
    void *tr_buf;    /*shared, P+P, trojan and receiver*/ /*tr_init*/
    void *ts_buf;    /*time statistic, ts_alloc*/
    seL4_CPtr up_ep; /*comm trojan and receiver, tr_start_slave*/ 
    seL4_CPtr down_ep; 
}bench_cap_t; 


int debug = 0;



static void init() {
    /*FIXME: making sure init is run on core 0*/
#if 0
    setAffinity(0);
#endif 
    probe_init_simple(EBSIZE, WAY_SIZE);
    tr_init(CACHESIZE);
}


/*entry for capacity benchmark 
opt: single core, multicore 
env: running enviornment 
record: data collection*/
int bench_capacity(int opt, bench_cap_t *env, void *record) {
    int slave = 0;
    char *ofile = NULL;
    FILE *f = stdout;

    if (opt == BENCH_CAP_SINGLE) 
        slave = 1; 

    init();

    //probe_printmap();
    /*single core bench*/
    if (slave)
        tr_startslave(LINE, 0);
    /*ts buffer in env*/
    ts_t ts = ts_alloc();

    for (int mask =0; mask < 32; mask++) 
        for (int shift = 0; shift < 10; shift++) {

            //printf("%d/%d\n", mask, shift);

            for (int i = 0; i < 1024; i++) {

                /*working set size, the secret msg*/
                int size = i ^ mask <<5 ^ mask;
                size = (size >> shift) ^ (size << (10-shift));
                size &=0x3ff;
                ts_clear(ts);

                if (slave) {  
                    /*single core: prime, trojan, probe*/
                    /*prime*/
                    probe_sitime(NULL, LINE, 10);
                    /*do probe and record time*/
                    for (int k = 0; k < REPS; k++) {
                        /*singal*/
                        tr_callslave(size);
                        /*probe*/
                        probe_sitime(ts, LINE, 1);
                    }
                } else {
                    /*multicore: trojan run, wait for 1ms, 
                      then trojan and receiver running concurrently*/
                    /*trojan running, polluting cache*/
                    tr_start(size, LINE, 1);
                    /*currently detecting, probing*/
                    probe_sitime(ts, LINE, REPS);
                    /*kill trojan*/
                    tr_stop();
                }
                /*data format: secret time*/
                for (int k = 1; k < TIME_MAX; k++)
                    for (int n = ts_get(ts, k); n--; ) 
                        fprintf(f, "%d %d\n", size, k);

                // Force LRU eviction
                for (int j = 0; j < 2048; j+=64) {
                    probe_sitime(ts, (LINE^j)&0x3f, 100);
                    probe_sitime(ts, (LINE^j)&0x3f, 100);
                    probe_sitime(ts, (LINE^j)&0x3f, 100);
                }
            }
        }

}
