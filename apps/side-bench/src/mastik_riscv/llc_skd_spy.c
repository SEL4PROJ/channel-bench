#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#include "../mastik_common/low.h"
#include "../mastik_common/l1.h"
#include <channel-bench/bench_common.h>
#include <channel-bench/bench_types.h>
#include <channel-bench/bench_helper.h>

/* LLC Shared Kernel Data Channel */

int llc_skd_trojan(bench_env_t *env) {


    seL4_MessageInfo_t info;

    int secret = 0;

    bench_args_t *args = env->args;

    /*buffer size 512K LLC size */
    char *data = malloc(L3_SIZE);
    assert(data);
    data = (char*)ALIGN_PAGE_SIZE(data);

    uint32_t *share_vaddr = (uint32_t*)args->shared_vaddr;

    /*manager: trojan is ready*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    /*syn with spy*/
    seL4_Send(args->ep, info);


    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
        if (i % 1000 == 0 || (i - 1) % 1000 == 0 || i == (CONFIG_BENCH_DATA_POINTS - 1)) printf("TROJAN: Data point %d\n", i);

        /*waiting for a system tick*/
        newTimeSlice();

        /*evict llc*/
        for (unsigned int s = 0; s < L3_SETS; s++) {
            for (unsigned int i = 0; i < 8; i++) {
                void *v = data + s * 64 + i * 64 * 1024;
                volatile unsigned int rv;
                asm volatile("lw %0, 0(%1)": "=r" (rv): "r" (v):);
            }
        }

        secret = random() % (L3_SETS + 1);

        /*encode secret by accessing secret number of LLC lines in shared kernel gadget*/
        riscv_sys_access_shared_gadget(secret);

        /*update the secret read by low*/
        *share_vaddr = secret;

    }
    while (1);

    return 0;
}

int llc_skd_spy(bench_env_t *env) {
    seL4_Word badge;
    seL4_MessageInfo_t info;

    uint64_t monitored_mask[MONITOR_MASK];

    for (int m = 0; m < MONITOR_MASK; m++)
        monitored_mask[m] = ~0LLU;


    ccnt_t UNUSED pmu_start[BENCH_PMU_COUNTERS];
    ccnt_t UNUSED pmu_end[BENCH_PMU_COUNTERS];

    bench_args_t *args = env->args;

    char *data = malloc(L3_SIZE);

    l1info_t l1_1 = l1_prepare(monitored_mask);
    uint16_t *results = malloc(l1_nsets(l1_1)*sizeof(uint16_t));

    printf("SPY: Probe buffer base: 0x%lx\n", l1_1->memory);
    printf("SPY: l1_nsets: %d\n", l1_1->nsets);

    /*the record address*/
    struct bench_l1 *r_addr = (struct bench_l1 *)args->record_vaddr;
    /*the shared address*/
    uint32_t volatile *secret = (uint32_t *)args->shared_vaddr;

    /*syn with trojan*/
    info = seL4_Recv(args->ep, &badge);
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);


    for (int i = 0; i < CONFIG_BENCH_DATA_POINTS; i++) {
        if (i % 1000 == 0 || (i - 1) % 1000 == 0 || i == (CONFIG_BENCH_DATA_POINTS - 1)) printf("SPY: Data point %d\n", i);

        /*wait for new time slice*/
        newTimeSlice();

#ifdef CONFIG_MANAGER_PMU_COUNTER
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_start);
#endif
        /*probe by accessing entire shared kernel gadget*/
        uint32_t before = rdtime();
        riscv_sys_access_shared_gadget(L3_SETS);
        r_addr->result[i] = rdtime() - before;

#ifdef CONFIG_MANAGER_PMU_COUNTER
        sel4bench_get_counters(BENCH_PMU_BITS, pmu_end);
        /*loading the pmu counter value */
        for (int counter = 0; counter < BENCH_PMU_COUNTERS; counter++ )
            r_addr->pmu[i][counter] = pmu_end[counter] - pmu_start[counter];

#endif
        /*secret is updated by trojan in the previous system tick*/
        r_addr->sec[i] = *secret;

        /*evict LLC*/
        for (unsigned int s = 0; s < L3_SETS; s++) {
            for (unsigned int i = 0; i < 8; i++) {
                void *v = data + s * 64 + i * 64 * 1024;
                volatile unsigned int rv;
                asm volatile("lw %0, 0(%1)": "=r" (rv): "r" (v):);
            }
        }
    }

    /*send result to manager, spy is done*/
    info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(args->r_ep, info);

    while (1);

    return 0;
}


