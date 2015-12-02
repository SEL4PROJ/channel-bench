#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sel4/sel4.h>

#include "../../bench_common.h"
#include "../../covert.h"
/*striding page by page */
typedef int page_t[PAGE_SIZE / sizeof(int)];

static volatile int sum;


/*The trojan loop for single core*/
/*spy and trojan communicate on syn_ep*/
int trojan_single(char *t_buf, int line, seL4_CPtr syn_ep) {
    
    page_t *page = (page_t *)((intptr_t)t_buf + line * 64);
    register int s = 0;
    int size;
    seL4_MessageInfo_t send = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_MessageInfo_t recv;

    do {
    
        recv = seL4_Wait(syn_ep, NULL); 
        if (seL4_MessageInfo_get_label(recv) != seL4_NoFault)
            return BENCH_FAILURE; 

        /*waiting on signal from receiver*/
        if (seL4_MessageInfo_get_length(recv) != 1)
            return BENCH_FAILURE; 
        
        /*polluting the cache, page by page
          size is defined by receiver*/
        size = seL4_GetMR(0); 
        register page_t *p = page;
        for (int j = 0; j < size; j++) {
            s+= *p[0];
            p++;
        }
        seL4_SetMR(0, size);
        seL4_Send(syn_ep, send);
       // recv = seL4_ReplyWait(syn_ep, send, NULL); 
    } while (s == 0);
    // unreached
    sum = s;
    assert(1);
    return BENCH_SUCCESS;
}

void tr_callslave(seL4_CPtr syn_ep, int size) {

    /*calling torjan, the size of this run*/
    seL4_MessageInfo_t send = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
    seL4_MessageInfo_t recv; 
    seL4_SetMR(0,size); 
    seL4_Send(syn_ep, send);
    recv = seL4_Wait(syn_ep, NULL);
   // seL4_Call(syn_ep, send);

}




#if 0

int tr_startslave(int line, int core) {
  
  int up[2] = {-1, -1}, down[2] = {-1, -1};
  
  if (childpid != 0)
    return -3;

  /*FIXME: ipc ep, up[0] for read, up[1] for write
   one way*/
  if (pipe(up) < 0) 
      goto error;
  if (pipe(down) < 0) 
      goto error;

  /*FIXME: need a slave thread*/
  int pid = fork();
  switch (pid) {
  case 0:
      /*slave thread*/
    close(up[0]);
    close(down[1]); 
#if 0
    setAffinity(core);
#endif 
    slave(line, down[0], up[1]);
    exit(0);
  default:
    /*master*/
    fromslave = up[0];
    toslave = down[1];
    close(up[1]);
    close(down[0]);
    childpid = pid;
  }
  return 0;
}

static void trojan(int pagecount, int line) {
  register int s = 0;
  do {
      /*polluting the cache for page count*/
      register page_t *p = (page_t *)((intptr_t)buf + line * 64);
    for (int j = 0; j < pagecount; j++) {
      s+= *p[0];
      p++;
    }
  } while (s == 0);
  // unreached
  sum = s;
}

int tr_start(int pagecount, int line, int core) {
  if (childpid != 0)
    return -3;

  /*starting tojan on another core*/
#if 0
  int fd[2];
  if (pipe(fd) < 0) 
    return -1;
#endif
  int pid = fork();
  switch (pid) {
  case 0:
      /*child, trojan*/
#if 0
      close(fd[0]);
    setAffinity(core);
    close(fd[1]);
#endif
    trojan(pagecount, line);
    //exit(0); never reach
  case -1:
    return -2;
  default:
#if 0
    char c;
    read(fd[0], &c, 1);
#endif

    close(fd[0]);
    close(fd[1]);
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000; // 1 ms
    /*waiting for 1ms, trojan is running*/
    nanosleep(&ts, NULL);
    childpid = pid;
  }
  return 0;
}

void tr_stop() {

    /*kill the trojan*/
    if (childpid == 0)
        return;
    kill(childpid, SIGKILL);
    wait(NULL);
    if (fromslave != -1)
        close(fromslave);
    if (toslave != -1)
        close(toslave);
    fromslave = toslave = -1;
    childpid = 0;
}


#endif     

