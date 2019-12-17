/*the branch attack that probes N number of branch instructions*/
#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>

/*putting the probing code in data section therefore 
 giving the L1I_REWRITE an option to rewrite the buffer
 otherwise there will be an permission VM fault*/
.data

#ifdef CONFIG_PLAT_SABRE

/*simplify the probing, arranging them into cache sets
 L1 has 256 sets, 4 ways, one way == 8 KB*/
/*jumping to the next way */
#define BPL() \
    b .+8192;\
.align 5; 

#define BLR() \
      bx lr;\
.align 5;

#endif  /*CONFIG_PLAT_SABRE*/

#ifdef CONFIG_PLAT_HIKEY
/*L1 has 256 sets, 2 ways, 64 b/line, one way = 16384*/
/*jumping to the next way*/
#define BPL() \
    b .+16384;\
.align 6; 

/*return to caller*/
#define BLR() \
      bx lr;\
.align 6;

#endif   /*CONFIG_PLAT_HIKEY*/

#ifdef CONFIG_PLAT_TX1
/*L1 has 256 sets, 3 ways, 64 b/line, one way = 16384*/
/*jumping to the next way*/
#define BPL() \
    b .+16384;\
.align 6; 

/*return to caller (aarch64)*/
#define BLR() \
      ret;\
.align 6;
#endif

#define BPL_4() \
        BPL() \
        BPL() \
        BPL() \
        BPL()

#define BPL_16() \
        BPL_4() \
        BPL_4() \
        BPL_4() \
        BPL_4() 

#define BPL_64() \
        BPL_16() \
        BPL_16() \
        BPL_16() \
        BPL_16()



#define BLR_4() \
        BLR() \
        BLR() \
        BLR() \
        BLR() 
 
       
#define BLR_16() \
        BLR_4() \
        BLR_4() \
        BLR_4() \
        BLR_4() 


#define BLR_64() \
        BLR_16() \
        BLR_16() \
        BLR_16() \
        BLR_16() 

#ifdef CONFIG_PLAT_SABRE
#define BPL_PAGE() \
        BPL_64() \
        BPL_64() 
#define BLR_PAGE() \
        BLR_64() \
        BLR_64() 
#endif /*CONFIG_PLAT_SABRE*/


#ifdef CONFIG_PLAT_HIKEY
#define BPL_PAGE() \
        BPL_64() 

#define BLR_PAGE() \
        BLR_64() 
#endif  /*CONFIG_PLAT_HIKEY*/

#ifdef CONFIG_PLAT_TX1
#define BPL_PAGE() \
        BPL_64() 

#define BLR_PAGE() \
        BLR_64() 
#endif /*CONFIG_PLAT_TX1*/

/*sabre and hikey 32K L1I 
 tx1 48K L1I*/

.align 12 
.global arm_branch_sets
arm_branch_sets:
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
#ifdef CONFIG_PLAT_TX1
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
#endif  /*CONFIG_PLAT_TX1*/
BLR_PAGE()
BLR_PAGE()
BLR_PAGE()
BLR_PAGE()



#define B_4BYTES() \
      b .+4; 

#define B_4BYTES_4() \
        B_4BYTES() \
        B_4BYTES() \
        B_4BYTES() \
        B_4BYTES() 

#define B_4BYTES_16() \
        B_4BYTES_4() \
        B_4BYTES_4() \
        B_4BYTES_4() \
        B_4BYTES_4() 

#define B_4BYTES_64() \
        B_4BYTES_16() \
        B_4BYTES_16() \
        B_4BYTES_16() \
        B_4BYTES_16() 

#define B_4BYTES_256() \
        B_4BYTES_64() \
        B_4BYTES_64() \
        B_4BYTES_64() \
        B_4BYTES_64() 

/*using 4 bytes, 256 sets to probe the btb 
 sabre: 512 branch instruction
 hikey and tx1: 256 branch instruction
 */ 
.global arm_branch_lines
arm_branch_lines:
#ifdef CONFIG_PLAT_SABRE
B_4BYTES_256()
#endif 
B_4BYTES_256()
#ifdef CONFIG_ARCH_AARCH64
        ret
#else
        bx lr   /*bx lr uses the return stack, 8 entries 32 bits
                 a seperate cache */
#endif
