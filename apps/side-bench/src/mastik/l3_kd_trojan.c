#include <autoconf.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include "vlist.h"
#include "cachemap.h"
#include "pp.h"
#include "low.h"
#include "search.h"

#include "bench_common.h"
#include "bench_types.h"


uint32_t on[1000];
uint32_t off[1000];
uint32_t xx[1000];

void measure() {
  on[0] = 1;
	off[0] = 1;
	xx[0] = 1;
  uint32_t start = rdtscp();
  uint32_t prev = start;
  for (int i = 0; i < 100;) {
    uint32_t cur = rdtscp();
    if (cur - start > 10000) {
      xx[i] = start;
      on[i] = prev;
      off[i] = cur;
      prev = cur;
      i++;
    }
    start = cur;
  }
 
  printf("start prev cur -> on off tot\n");
  for (int i = 0; i < 100; i++) 
    printf("%u %u %u -> %u %u %u\n", xx[i], on[i], off[i], xx[i] - on[i], off[i] - xx[i], off[i] - on[i]);
  exit(0);
}
      
static void warmup(cachemap_t cm) {

    /*warming up the testing platform with randomized 
     cache accessing sequence*/
    int total_sec = cm->nsets * 64;
    int secret; 
    for (int i = 0; i < NUM_KD_WARMUP_ROUNDS; i++) {
        secret = random() % total_sec; 

        for (int i = 0; i < secret; i++) {
                /*16 is the associativity*/
                vlist_t vl = cm->sets[i >> 6]; 
                int l = 16; 
                if (l > vl_len(vl)) 
                    l = vl_len(vl); 

                /*touch the page*/
                for (int j = 0; j < l; j++) {

                    char *p = (char *)vl_get(vl, j); 
                    p[(i & 0x3f) << 6] = 0; 
                }

            }

    }

}


int l3_kd_trojan(bench_env_t *env) {
    seL4_Word badge;
    seL4_MessageInfo_t info;
    int total_sec, secret;
    bench_args_t *args = env->args;

    uint32_t volatile *share_vaddr = (uint32_t *)args->shared_vaddr; 

    *share_vaddr = SYSTEM_TICK_SYN_FLAG; 
    
    cachemap_t cm = map();

    /*to root task*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    warmup(cm);

    /*to SPY*/
    seL4_Send(args->ep, info);
    uint64_t cur, prev; 

    /*nprobing sets is 64 (16 colour * 4 cores) on a coloured platform
     and 128 (32 colour * 4 cores) on a non-coloured platform
     secret represents the number of cache sets in total, 64 sets in a 
     page, all together nsets * 64.*/

    total_sec = cm->nsets * 64;

    /*using 100 time ticks to syn scheduling sequence*/
    for (int n = 0; n < 100; n++) {
        secret = random() % total_sec;  
        /*wait for a new time slice*/ 
        cur = prev = rdtscp_64(); 
        while (cur - prev < KERNEL_SCHEDULE_TICK_LENTH) {
            prev = cur;
            /*waiting for the timer tick to break the schedule*/
            cur = rdtscp_64();
        }
        /*update the secret read by low*/ 
        for (int i = 0; i < secret; i++) {
            /*16 is the associativity*/
            vlist_t vl = cm->sets[i >> 6]; 
            int l = 16; 
            if (l > vl_len(vl)) 
                l = vl_len(vl); 

            /*touch the page*/
            for (int j = 0; j < l; j++) {

                char *p = (char *)vl_get(vl, j); 
                p[(i & 0x3f) << 6] = 0; 
            }

        }
    }

    for (int n = 0; n < CONFIG_BENCH_DATA_POINTS; n++) {
        secret = random() % total_sec;  
        /*wait for a new time slice*/ 
        cur = prev = rdtscp_64(); 
        while (cur - prev < KERNEL_SCHEDULE_TICK_LENTH) {
            prev = cur;
            /*waiting for the timer tick to break the schedule*/
            cur = rdtscp_64();
        }
        /*update the secret read by low*/ 
        *share_vaddr = secret; 
        for (int i = 0; i < secret; i++) {
            /*16 is the associativity*/
            vlist_t vl = cm->sets[i >> 6]; 
            int l = 16; 
            if (l > vl_len(vl)) 
                l = vl_len(vl); 

            /*touch the page*/
            for (int j = 0; j < l; j++) {

                char *p = (char *)vl_get(vl, j); 
                p[(i & 0x3f) << 6] = 0; 
            }

        }
    }

    for(;;) {
     /*never return*/

/*     vlist_t vl = cm->sets[i];
      int l = 16;
      if (l > vl_len(vl))
	l = vl_len(vl);
      for (int c = 0; c < 4096; c += 128) {
	for (int j = 0; j < l; j++) {
	  char *p = (char *)vl_get(vl, j);
	  p[c] = 0;
	}
      }
      i++;
      if (i == cm->nsets)
	i = 0;
  */      
    }


 
  for (;;) {
    /*waiting on msg to start*/
      info = seL4_Recv(args->ep, &badge);
      assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
      measure();

//      attack(cm);

      info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
      seL4_SetMR(0, 0); 
      seL4_Send(args->ep, info);

    //mastik_sleep(10);
  }
}



