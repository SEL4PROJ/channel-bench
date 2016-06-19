#include <stdio.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "bench_common.h"

#include "mpi.h"


uint32_t prev_sec[NUM_KERNEL_SCHEDULE_DATA];
uint32_t cur_sec[NUM_KERNEL_SCHEDULE_DATA];
uint64_t starts[NUM_KERNEL_SCHEDULE_DATA];
uint64_t curs[NUM_KERNEL_SCHEDULE_DATA];
uint64_t prevs[NUM_KERNEL_SCHEDULE_DATA];

struct bench_kernel_schedule *r_addr = NULL;

static inline uint32_t rdtscp() {
  uint32_t rv;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}


static void measure(bench_covert_t *env) 
{
  seL4_Word badge;
  seL4_MessageInfo_t info;

  info = seL4_Recv(env->r_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);

  /*the record address*/
  r_addr = (struct bench_kernel_schedule *)seL4_GetMR(0);
  /*the shared address*/
  uint32_t *secret = (uint32_t *)seL4_GetMR(1);


  info = seL4_Recv(env->syn_ep, &badge);
  assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);


  starts[0] = 1;
  curs[0] = 1;
  prevs[0] = 1;
  int count = 0;
  uint64_t start = rdtscp_64();
  uint64_t prev = start;
  uint32_t prev_s = *secret; 
  for (int i = 0; i < NUM_KERNEL_SCHEDULE_DATA;) {
    uint64_t cur = rdtscp_64(); 
    /*at the begining of the current tick*/
    if (cur - prev > KERNEL_SCHEDULE_TICK_LENTH) {
      prevs[i] = prev;
      starts[i] = start;
      curs[i] = cur;
      prev_sec[i] = prev_s;
      /*at the beginning of this tick, update the secret*/
      prev_s = cur_sec[i] = *secret;
      start = cur;
      i++;
    }
    prev = cur;
  }
  
  //printf("Trojan: prev start cur -> on off tot\n");
  for (int i = 0; i < NUM_KERNEL_SCHEDULE_DATA; i++) {

      r_addr->prevs[i] = prevs[i]; 
      r_addr->starts[i] = starts[i]; 
      r_addr->curs[i] = curs[i]; 
      r_addr->prev_sec[i] = prev_sec[i];
      r_addr->cur_sec[i] = cur_sec[i];

  }
  //for (int i = 1; i < NUM_KERNEL_SCHEDULE_BENCH; i++) 
    //printf("Trojan: %llu %llu %llu -> %llu %llu %llu\n", prevs[i], starts[i], curs[i], prevs[i] - starts[i], curs[i] - prevs[i], curs[i] - starts[i]);

  info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);
  seL4_SetMR(0, 0);
  seL4_Send(env->r_ep, info);

  for (;;) 
  {
  }
  //exit(0);
}

int l3_kd_spy(bench_covert_t *env) {
  measure(env);
  MPI p = mpi_alloc(0);
  mpi_fromstr(p, "0xe1baf27f25bdd90774579c9df4160097fb5a6b927636d78762e45cf55d706b7443b2bb9220a0397479cd20a7e136e3bd6b3b1e41a913e70da107491cf7d6b3b0e36851246b9b93b1d902fbdc14cae6c4ca529664451138e840554ce2cb69d6a7bc552db92e86f41cc2b20ac4ce6c4f2798eb64c728e0664b6e7557e6d99d291a36e8b3889de12626ee7c18c2de07be01ceda394f96de2a2e5d22272fd6fbb8900460089a2667bd2ae9581417f3f51edd39d6bb2838be175f96ac4e347b7252d8cbbedcdbbfa0eb54dc516c90895e62241b4a5a225867a39c853aa00cefa770a59e3d12d41f09f8d3425a5c69f40ec1dff64d3f59600ff92145198b7bcf4a8e07");
  MPI b = mpi_alloc(0);
  mpi_fromstr(b, "0xc77fa6761375f390715132678666b888245e3bca5999283d28c5ee58ed9457367eeaab737dd2b848c12fc344a395bc8fb9437674ddee0d48433bebc7a8443bbab23e9318d64ba5bd147b06aa56e9d77099afda86c8bc9d4d2c02a83c3b555d5b71fd7e747cbefcd4deec785a1804c1052ebba7e210d2a08c47d208eecce1187cb21dae4d5bb5e9a61c1f153df020f3fafa6f235669a97570089f56222a6f6a7754f9d70c086bd299928f1f9856e511502cf84f2f968291eff895ca786574976d1c3c26ef754aa92932f35389b10a4e9dbf24f1d40787660be56ca8b3808a67c29022e3efe5f59a370ab32bf8e7ce0c53a0ddf702821d7a240cfc3d03f303dddf");
  MPI d = mpi_alloc(0);
  mpi_fromstr(d, "0x23c235c121bcf0a26e9217331abf094e655a5e251f90773334b60c4bd361dfc4ed14e50836e069c7daedf3f7af960a1bfe46d1c8a5d2185822d1ddb090c09774bf5d2977a42f726e41c21c3a7bf932c51635a4be3c116d08b2d6e2fc3c51427002d0c5a8e3fdae3d637004452922eff2264924bbf33eef287a8b8fd85097ab3e1da3ce84b8049be64024afd2d5bde25b3cd97e267e10de1833a2c0b0fb482f5e51e152c7db1ffc39921ffe34957195dc04f6bad347c586931722217a1a072562a8a3b091ccba6acf12b985777210e05163f4801274883c14ba2e17c5c2008c199a908ad2fe401a459d6343212d9b113cb463a36bd86ef3d0af833577f5844460");
  //mpi_fromstr(d, "0x23c235c121bcf0a26e9217331abf094e655a5e251f90773334b60c4bd361dfc4");
  MPI res = mpi_alloc(0);
  
  mpi_powm( res, b, d, p);
  
  for (;;) {
    mpi_powm( res, b, d, p);
    //printf("%s\n", mpi_tostr(res));
    uint32_t start = rdtscp();
    while (rdtscp() - start < 3000000)
      ;
  }
}
