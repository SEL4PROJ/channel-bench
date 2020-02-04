#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

#define X_4(a) a; a; a; a
#define X_16(a) X_4(X_4(a))
#define X_256(a) X_16(X_16(a))
#define X_4096(a) X_256(X_16(a))

#define JMP	j 1f; .align 4; 1:

.global btb_probe
.type btb_probe, @function
.align 4
btb_probe:
  X_16(JMP)
  X_16(JMP)
  ret
  