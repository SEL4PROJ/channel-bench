
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

#pragma once

#include <sel4utils/slab.h>
#include <simple/arch/simple.h>
#include <channel-bench/bench_types.h>

void benchmark_arch_get_simple(arch_simple_t *simple);
void benchmark_arch_get_timers(bench_env_t *env, ps_io_ops_t ops);

/*init timer for the benchmark thread*/
int timing_benchmark_init_timer(bench_env_t *env);
/*init benchmarking enviroment accroding to the arguments created by root thread*/
void bench_init_env(int argc, char **argv, bench_env_t *env); 
 
