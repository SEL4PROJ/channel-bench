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


