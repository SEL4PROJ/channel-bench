/*the branch attack that probes N number of branch instructions*/

#if 0
/*code for flushing the return stack on arm*/
bl call0; 
call0 : 
 push {lr};
 bl call1; 

call1: 
 
call 7: 
    push {lr}; 
    bl call8;
    pop {lr}; 
    ldr pc, sp!
    bx lr 
call8: 

 bx lr; 
    b address 
#endif 

.text

/*32 byte aglined, 8 instructions*/
/*r0 contains the address of this line  r1 contains the loop number
 lr contains the return address of the calling function*/

/*one iteration contains two branch target buffer store 
 and one global history table update
 do not handle if the r1 equal to 0*/
#if 0
/*rewrite the code with direct jumps 
 because did not observe any contention through probing latency*/
#define BPL() \
    mov r0, pc;\
    add r0, #32;\
    sub r1, #1;\
    cmp r1, #0;\
    beq .+8;\
    bx  r0;\
    bx  lr;\
.align 5 ;
#endif 

/*simplify the probing, arranging them into cache sets
 L1 has 256 sets, 4 ways, one way == 8 KB*/
/*jumping to the next way */
#define BPL() \
    b .+8192;\
.align 5; 

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


#define BPL_PAGE() \
        BPL_64() \
        BPL_64() 

#define BLR() \
      bx lr;\
.align 5;


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


#define BLR_PAGE() \
        BLR_64() \
        BLR_64() 


/*altogether 1024 lines
 8 pages: 6 pages jump, 2 pages return*/
.align 12 
.global arm_branch_lines
arm_branch_lines:
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BLR_PAGE()
BLR_PAGE()


