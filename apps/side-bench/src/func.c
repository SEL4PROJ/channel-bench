/*the function testing code*/
#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>

int funcs_sender(bench_env_t *env) {

  seL4_Word badge;
  seL4_MessageInfo_t info;
  uint32_t UNUSED ipc_runs = 100; 
  bench_args_t *args = env->args; 
 

  /*doing IPC ping-pong test to receiver*/ 
#if (CONFIG_MAX_NUM_NODES > 1)
      while (1)
#else  
      while (ipc_runs--) 
#endif 
      {

          info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
          seL4_SetMR(0, 0); 
          seL4_Call(args->ep, info); 
      }
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 

  seL4_Send(args->ep, info); 

  /*test is done*/
  info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
  seL4_SetMR(0, 0); 
  seL4_Send(args->r_ep, info);


  /*never returned by root task*/
  info = seL4_Recv(args->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

  return 0;

} 


int funcs_receiver(bench_env_t *env) {

    seL4_Word badge;
    seL4_MessageInfo_t info;
    bench_args_t *args = env->args; 


    uint32_t UNUSED ipc_runs = 100; 

    /*receive the shared address to record the secret*/
    ccnt_t volatile *record_vaddr = (ccnt_t *)args->record_vaddr;
    ccnt_t tick = 0;

    seL4_Recv(args->ep, NULL);  
#if (CONFIG_MAX_NUM_NODES > 1)
    while (1)
#else  
        while (ipc_runs--) 
#endif 
        {

            /*updating the shared memory
              indicating both threads are alive*/
            *record_vaddr = tick++;  
            info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
            seL4_SetMR(0, 0); 
            seL4_ReplyRecv(args->ep, info, &badge); 
        }
    /*test is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0); 
    seL4_Send(args->r_ep, info);

    /*never return by the root*/ 
    info = seL4_Recv(args->r_ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    return 0;
}

