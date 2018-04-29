/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#include <autoconf.h>
#include <stdio.h>

#include <sel4/sel4.h>
#include "bench_types.h"
#include "bench_common.h"


int bench_idle(bench_env_t *env) {
    seL4_MessageInfo_t info;
    seL4_Word badge;

    bench_args_t *args = env->args; 

   
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);

    /*starting to spin, consuming CPU cycles only*/
    while (1); 

    return BENCH_SUCCESS; 
}
