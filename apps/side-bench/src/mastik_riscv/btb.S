#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

/*
            ret
       |=== block 1
   spy ||   block 2   /\
       \/   ...       || trojan
            block s ===|
            ...
            ret
*/

/* number of half words per block */
#define JUMP_HW_ALIGN CONFIG_BENCH_BTB_ALIGN/2

.global btb_probe
.type btb_probe, @function
.global btb_prime
.type btb_prime, @function

.align 4
/* end of prime */
  ret
/* align. ret already takes one HW */
.rept JUMP_HW_ALIGN - 1
  nop
.endr
btb_probe:
.rept CONFIG_BENCH_BTB_ENTRIES 
  /* a1 should either be +BTB_ALIGN (probe) or -BTB_ALIGN (prime)
     a0 points to the next (probe) or previous (prime) block */
  add a0, a0, a1  // 
  jalr x0, a0     // 
  /* align. add and ret already take three HWs */
  .rept JUMP_HW_ALIGN - 3
    nop
  .endr
.endr
btb_prime:
  ret
/* end of probe */