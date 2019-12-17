#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

#define X_4(a) a; a; a; a
#define X_16(a) X_4(X_4(a))
#define X_256(a) X_16(X_16(a))

#define JMP	beq 1f; .align 4; 1:

.global bp_probe
.align 6
bp_probe:
  push {r4-r6}
  eor  r4, r4, r4
  cmp  r4, #0x0 
  X_256(JMP)
  mrc p15, 0, r5, c9, c13, 0   //reading counter
  cmp  r0, #0x0
  beq  2f 
  X_256(nop) 
 2:
  mrc p15, 0, r6, c9, c13, 0   //reading counter
  sub   r6, r6, r5 
  mov   r0, r6 
  pop   {r4-r6}
  bx lr 

