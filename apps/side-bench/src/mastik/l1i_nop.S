

.text


#define NOP_LINE() \
1:    nop; \
.balign 64, 0x90; 



#define NOP_LINE4() \
NOP_LINE() \
NOP_LINE() \
NOP_LINE() \
NOP_LINE()

#define NOP_LINE16() \
NOP_LINE4() \
NOP_LINE4() \
NOP_LINE4() \
NOP_LINE4()

#define NOP_PAGE() \
NOP_LINE16() \
NOP_LINE16() \
NOP_LINE16() \
NOP_LINE16()


.balign 4096
.global nop_spy
nop_spy:
    movl $0, %ecx
    movl $0, %eax
    cpuid 
    movl $0, %ecx
    movl $0, %eax
    rdtscp 
    movl %eax, l1i_probe_nop_time 
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
    movl $0, %ecx
    movl $0, %eax
    rdtscp 
    subl l1i_probe_nop_time, %eax
    movl %eax, l1i_probe_nop_time
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/

.balign 4096
.global nop_trojan_1
nop_trojan_1:
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/

.balign 4096
.global nop_trojan_2
nop_trojan_2:
NOP_PAGE()
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/

.balign 4096
.global nop_trojan_3
nop_trojan_3:
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/

.balign 4096
.global nop_trojan_4
nop_trojan_4:
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/


.balign 4096
.global nop_trojan_5
nop_trojan_5:
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/


.balign 4096
.global nop_trojan_6
nop_trojan_6:
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/

.balign 4096
.global nop_trojan_7
nop_trojan_7:
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/




.balign 4096
.global nop_trojan_8
nop_trojan_8:
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
NOP_PAGE()
2:
1:
.fill 4096, 1, 0xc3 /*ret instruction*/

