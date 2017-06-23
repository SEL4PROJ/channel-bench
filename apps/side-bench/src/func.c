/*the function testing code*/
#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../../bench_common.h"




int funcs_sender(bench_covert_t *env) {

  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint32_t ipc_runs = 100; 

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

  /*receive the shared address to record the secret*/
  uint32_t volatile *record_vaddr = (uint32_t *)seL4_GetMR(0);
  uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(1);
  
  /*doing IPC ping-pong test to receiver*/ 
#if (CONFIG_MAX_NUM_NODES > 1)
  while (1)
#else  
  while (ipc_runs--) 
#endif 
  {
      info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
      seL4_SetMR(0, 0); 
      seL4_Call(env->syn_ep, info); 

  }
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 

  seL4_Send(env->syn_ep, info); 

  /*test is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(env->r_ep, info);


  /*never returned by root task*/
  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

  return 0;
  
} 


int funcs_receiver(bench_covert_t *env) {

    
    seL4_Word badge;
    seL4_MessageInfo_t info;
    uint32_t ipc_runs = 100; 
    
    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    /*receive the shared address to record the secret*/
    uint32_t volatile *record_vaddr = (uint32_t *)seL4_GetMR(0);
    uint32_t volatile *share_vaddr = (uint32_t *)seL4_GetMR(1);


    seL4_Recv(env->syn_ep, NULL);  
#if (CONFIG_MAX_NUM_NODES > 1)
  while (1)
#else  
  while (ipc_runs--) 
#endif 
  {

        info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
        seL4_SetMR(0, 0); 
        seL4_ReplyRecv(env->syn_ep, info, &badge); 

        printf("r\n");
        /*updating the shared memory
          indicating both threads are alive*/
        *record_vaddr  = rdtscp_64();

    }


    /*test is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(env->r_ep, info);

    /*never return by the root*/ 
    info = seL4_Recv(env->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    return 0;

}
  
