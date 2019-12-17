#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

#define X_4(a) a; a; a; a
#define X_16(a) X_4(X_4(a))
#define X_256(a) X_16(X_16(a))

#define JMP	jnc 1f; .align 16; 1:

#ifdef CONFIG_ARCH_X86_64

.global bp_probe
.type bp_probe, @function
.align 64
bp_probe:
  xorl %eax, %eax
  X_256(JMP)
  
  rdtscp
  movl %eax, %esi
  andl $1, %edi
  jz  2f
  X_256(nop)
2:
  rdtscp
  subl %esi, %eax
  retq

#else /* CONFIG_ARCH_X86_64 */

.global bp_probe
.type bp_probe, @function
.align 64
bp_probe:
  push %ebp
  mov %esp, %ebp
  pushl %esi
  pushl %edi
  mov 8(%ebp), %edi
  xorl %eax, %eax
  X_256(JMP)
  
  rdtscp
  movl %eax, %esi
  and $1, %edi
  jz  2f
  X_256(nop)
2:
  rdtscp

  subl %esi, %eax
  popl %edi
  popl %esi
  pop %ebp
  ret

#endif /* CONFIG_ARCH_X86_64 */
