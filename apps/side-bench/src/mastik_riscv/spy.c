#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include "../mastik_common/vlist.h"
#include "cachemap.h"
#include "pp.h"
#include "../mastik_common/low.h"
#include "search.h"


int mastik_spy(seL4_CPtr ep, char **av) {
    
    seL4_Word badge;
    seL4_MessageInfo_t info;

    info = seL4_Recv(ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


    cachemap_t cm = map();

    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(ep, info);

 
  for (;;) {
    /*waiting on msg to start*/
      info = seL4_Recv(ep, &badge);
      assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

      printf("test start\n");
      attack(cm);

      info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
      seL4_SetMR(0, 0); 
      seL4_Send(ep, info);

    //mastik_sleep(10);
  }
}



