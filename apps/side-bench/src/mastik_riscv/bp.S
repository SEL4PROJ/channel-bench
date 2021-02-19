#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

#define X_4(a) a; a; a; a
#define X_16(a) X_4(X_4(a))
#define X_256(a) X_16(X_16(a))

#define CONFIGBHT_ENTRIES 64
#define JMP_HW_ALIGN CONFIG_BENCH_BHT_ALIGN/2

.global bp_probe
.type bp_probe, @function
.align 4
bp_probe:  
  rdcycle t0
.rept CONFIG_BENCH_BHT_ENTRIES
  .word 0x50A63 //beqz a0, 20. bloody compiler will rip this apart otherwise. 20 = CONFIG_BENCH_BHT_ALIGN
  addi a0, a0, -1 
  .rept JMP_HW_ALIGN - 3
    nop
  .endr
.endr
  rdcycle a0
  sub a0, a0, t0
  ret
