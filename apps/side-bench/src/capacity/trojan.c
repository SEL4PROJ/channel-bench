#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "affinity.h"
#include "trojan.h"

static int childpid = 0;

/*striding page by page */
typedef int page_t[4096/sizeof(int)];

static volatile int sum;

static void *buf;  /*polluting the cache, sending msg */
int toslave = -1;
int fromslave = -1;

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

static void slave(int line, int readfd, int writefd) {
  page_t *page = (page_t *)((intptr_t)buf + line * 64);
  register int s = 0;

  do {
    int size;
    /*waiting on signal from receiver*/
    switch (read(readfd, &size, sizeof(int))) {
    case 0:
    case -1:
    default:
      exit(1);
    case sizeof(int): 
      {
          /*polluting the cache, page by page
           size is defined by receiver*/
	register page_t *p = page;
	for (int j = 0; j < size; j++) {
	  s+= *p[0];
	  p++;
	}
        /*done*/
	write(writefd, &size, sizeof(int));
      }
    }

  } while (s == 0);
  // unreached
  sum = s;
}

void tr_callslave(int size) {
    /*sending signal to slave, id for this run*/
  write(toslave, &size, sizeof(int));
  read(fromslave, &size, sizeof(int));
}



void tr_init(uint64_t size) {
    /*FIXME: mmap for a buffer shared between processes*/
    buf = mmap(NULL, size, 
            PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if (buf == MAP_FAILED) {
        perror("allocate");
        exit(1);
    }
}


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


    

