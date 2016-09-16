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
#define BPL() \
    mov r0, pc;\
    add r0, #32;\
    sub r1, #1;\
    cmp r1, #0;\
    beq .+8;\
    bx  r0;\
    bx  lr;\
.align 5 ;

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

/*altogether 1024 lines*/
.align 12 
.global arm_branch_lines
arm_branch_lines:
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()
BPL_PAGE()


