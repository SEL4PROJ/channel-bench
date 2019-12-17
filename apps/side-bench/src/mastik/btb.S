#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

#define X_4(a) a; a; a; a
#define X_16(a) X_4(X_4(a))
#define X_256(a) X_16(X_16(a))
#define X_4096(a) X_256(X_16(a))

#define JMP	jmp 1f; .align 16; 1:

#ifdef CONFIG_ARCH_X86_64
.global btb_probe
.type btb_probe, @function
.align 16
btb_probe:
  X_4096(JMP)
  X_4096(JMP)
  retq

#else /* CONFIG_ARCH_X86_64 */
.global btb_probe
.type btb_probe, @function
.align 16
btb_probe:
  X_4096(JMP)
  X_4096(JMP)
  ret
#endif /* CONFIG_ARCH_X86_64 */
