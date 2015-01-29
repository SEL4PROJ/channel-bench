/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


/*list of benchmarks
 * L1 Instruction cache   (prime + probe) the entire cache
 * L1 Data cache (prime + probe) the entire cache
                                (evict + time) some cache patterns 
                                * branch target buffer (prime + probe) all entires  
                                * TLB (double page fault, containing the cached kernel entry translation)   require to use invalid TLB entries for kernel address space before switching back to user space, Intel machine (INVLPG) arm (invalidate TLB entry by MVA)
                                *LLC  (flush + reload) does not working while no page duplication enabled 
                                          (prime + probe) on a cache set 

                                          covert channel with uncoloured kernel: using uncoloured kernel to evict some of the cache lines of the receiver, then receiver can monitor the progress changes with signals triggered by sender. such as: walking through root cnode, or interrupt vector table. 
 */

#include <autoconf.h>
#include <sel4/sel4.h>

#include "bench.h"

/*endpoint used for communicate with root task*/
seL4_CPtr endpoint; 

/*wait for shared frame address*/
static 
void *wait_vaddr_from(seL4_CPtr endpoint)
{
    /* wait for a message */
    seL4_Word badge;
    seL4_MessageInfo_t info;

    info = seL4_Wait(endpoint, &badge);

    /* check the label and length*/
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
    assert(seL4_MessageInfo_get_length(info) == 1);

    return (void *)seL4_GetMR(0);
}

/*send benchmark result to the root task*/
static 
void send_result_to(seL4_CPtr endpoint, seL4_Word w) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 1);

    seL4_SetMR(0, w); 
    seL4_Send(endpoint, info);
}



int main (int argc, char **argv) {

    void *record_vaddr; 
    seL4_Word result; 

    /*current format for argument: "name, endpoint slot, xxx"*/

    /*processing arguments*/
    assert(argc == CONFIG_BENCH_ARGS);
    endpoint = (seL4_Word)atoi(argv[1]); 

    /*waiting for virtual address given by root task*/
    record_vaddr = wait_vaddr_from(endpoint); 
    assert(record_vaddr); 
    /*FIXME: currently test the L1 data cache attack*/

    crypto_init(); 

    /*passing the record vaddr in */
    result = dcache_attack(record_vaddr); 

    /*return result to root task*/
    send_result_to(endpoint, result); 

    /*finished testing, halt*/
    while(1);

    return 0;

}
