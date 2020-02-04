#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

#define X_4(a) a; a; a; a
#define X_16(a) X_4(X_4(a))
#define X_256(a) X_16(X_16(a))

#define JMP	beqz t0, 1f
#define JMP_4 JMP; JMP; JMP; JMP
#define JMP_16 JMP_4; JMP_4; JMP_4; JMP_4
#define JMP_64 JMP_16; JMP_16; JMP_16; JMP_16
#define JMP_256 JMP_64; JMP_64; JMP_64; JMP_64; .align 4; 1:

.global bp_probe
.type bp_probe, @function
.align 8
bp_probe:
  mv t0, zero
  JMP_256
  
  rdtime t1
  beqz a0, 2f
  X_256(nop)
2:
  rdtime a0
  sub a0, a0, t1
  ret
