/*the l1i channel for arm platforms*/
#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include "../../../bench_common.h"
#include "../../../bench_types.h"

#include "../bench.h"

int l1i_trojan(bench_env_t *env) {
    return btb_trojan(env); 
}


int l1i_spy(bench_env_t *env) {
    return btb_spy(env);
}
