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

typedef int page_t[4096/sizeof(int)];

static volatile int sum;

static void *buf;
int toslave = -1;
int fromslave = -1;

void trojan(int pagecount, int line) {
  register int s = 0;
  do {
    register page_t *p = (page_t *)((intptr_t)buf + line * 64);
    for (int j = 0; j < pagecount; j++) {
      s+= *p[0];
      p++;
    }
  } while (s == 0);
  // unreached
  sum = s;
}

void slave(int line, int readfd, int writefd) {
  page_t *page = (page_t *)((intptr_t)buf + line * 64);
  register int s = 0;
  do {
    int size;
    switch (read(readfd, &size, sizeof(int))) {
    case 0:
    case -1:
    default:
      exit(1);
    case sizeof(int): 
      {
	register page_t *p = page;
	for (int j = 0; j < size; j++) {
	  s+= *p[0];
	  p++;
	}
	write(writefd, &size, sizeof(int));
      }
    }

  } while (s == 0);
  // unreached
  sum = s;
}

void tr_callslave(int size) {
  write(toslave, &size, sizeof(int));
  read(fromslave, &size, sizeof(int));
}



void tr_init(uint64_t size) {
  buf = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (buf == MAP_FAILED) {
    perror("allocate");
    exit(1);
  }
}


int tr_startslave(int line, int core) {
  if (childpid != 0)
    return -3;

  int up[2] = {-1, -1}, down[2] = {-1, -1};
  if (pipe(up) < 0) 
    goto error;
  if (pipe(down) < 0) 
    goto error;

  int pid = fork();
  switch (pid) {
  case 0:
    close(up[0]);
    close(down[1]);
    setAffinity(core);
    slave(line, down[0], up[1]);
    exit(0);
  case -1:
    goto error;
  default:
    fromslave = up[0];
    toslave = down[1];
    close(up[1]);
    close(down[0]);
    childpid = pid;
  }
  return 0;

error:
  if (up[0] >= 0)
    close(up[0]);
  if (up[1] >= 0)
    close(up[1]);
  if (down[0] >= 0)
    close(down[0]);
  if (down[1] >= 0)
    close(down[1]);
  return -1;
}


int tr_start(int pagecount, int line, int core) {
  if (childpid != 0)
    return -3;

  int fd[2];
  if (pipe(fd) < 0) 
    return -1;

  int pid = fork();
  switch (pid) {
  case 0:
    close(fd[0]);
    setAffinity(core);
    close(fd[1]);
    trojan(pagecount, line);
    exit(0);
  case -1:
    return -2;
  default:
    close(fd[1]);

    char c;
    read(fd[0], &c, 1);
    close(fd[0]);

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000; // 1 ms
    nanosleep(&ts, NULL);
    childpid = pid;
  }
  return 0;
}

void tr_stop() {
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


    

