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


/*TODO: fixing this benchmark setup*/

#ifdef CONFIG_MANAGER_CACHE_FLUSH
static void launch_bench_single (void *arg) {

    cspacepath_t src, dest; 
    int ret; 
    void *r_cp, *vaddr; 
    seL4_Word bench_result;
    uint32_t total, huge_page_size;

    /*current format for argument: "name, endpoint slot, xxx"*/
    char *arg0 = CONFIG_BENCH_THREAD_NAME;
    char arg1[CONFIG_BENCH_MAX_UNIT] = {0};
    char arg2[CONFIG_BENCH_MAX_UNIT] = {0}; 
    char *argv[CONFIG_BENCH_ARGS] = {arg0, arg1, arg2};

    printf("\n"); 
    printf("Flushing cache benchmark, preparing...\n");

    /*create frames that act as record buffer, mapping 
     to benchmark processes*/
    env.record_vaddr = vspace_new_pages(&env.vspace, seL4_AllRights, 
            CONFIG_BENCH_RECORD_PAGES, PAGE_BITS_4K); 
    assert(env.record_vaddr != NULL); 

    /*copy the caps to map into the remote process*/
    for (int i = 0; i < CONFIG_BENCH_RECORD_PAGES; i++) {
	
	vaddr = env.record_vaddr + i * PAGE_SIZE_4K; 
        vka_cspace_make_path(&env.vka, vspace_get_cap(&env.vspace, 
                    vaddr), &src); 
        ret = vka_cspace_alloc(&env.vka, env.record_frames + i); 
        assert(ret == 0); 

        vka_cspace_make_path(&env.vka, env.record_frames[i], &dest); 
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
        assert(ret == 0); 

    }

   /*creating the large page that uses as the probe buffer 
    for testing, mapping to the benchmark process, and passing in 
    the pointer and the size to the benchmark thread*/ 

    total = 16 * 6 * 1024 * 1024; 

#ifdef CONFIG_ARCH_X86
    huge_page_size = seL4_LargePageBits; 
#endif 

#ifdef CONFIG_ARCH_ARM 
     huge_page_size = vka_get_object_size(seL4_ARM_SuperSectionObject, 0); 
#endif 

    if (total % (1 << huge_page_size)) { 
        total += 1 << huge_page_size; 
    }
    total /= 1 << huge_page_size; 
    env.huge_vaddr = vspace_new_pages(&env.vspace, seL4_AllRights, total, 
            huge_page_size);

    for (uint32_t i = 0; i < total; i++) {

        vaddr = env.huge_vaddr + i * (1 << huge_page_size); 
        vka_cspace_make_path(&env.vka, vspace_get_cap(&env.vspace, 
                    vaddr), &src); 
        ret = vka_cspace_alloc(&env.vka, env.huge_frames + i); 
        assert(ret == 0); 

        vka_cspace_make_path(&env.vka, env.huge_frames[i], &dest); 
        ret = vka_cnode_copy(&dest, &src, seL4_AllRights); 
        assert(ret == 0); 
    }
    
    /*configure benchmark thread*/ 
    printf("Config benchmark thread...\n");
    ret = sel4utils_configure_process(&env.bench_thread, &env.vka, 
        &env.vspace, CONFIG_BENCH_PRIORITY, 
        CONFIG_BENCH_THREAD_NAME);
    assert(ret == 0); 


    /*copy the fault endpoint to benchmaring thread for communicate*/
    env.bench_ep = copy_cap_to(&env.bench_thread, 
            env.bench_thread.fault_endpoint.cptr); 

    /*set up args for the benchmark process
     benchmark thread is resumed afterwards.*/ 
    snprintf(arg1, CONFIG_BENCH_MAX_UNIT, "%d", env.bench_ep);

    /*entry point, system V ABI compliant*/
    ret = sel4utils_spawn_process_v(&env.bench_thread, &env.vka, 
            &env.vspace, CONFIG_BENCH_ARGS, argv, 1); 
    assert(ret == 0); 

    printf("sending init msg...\n"); 

    /*send an init msg*/ 
    send_msg_to(env.bench_thread.fault_endpoint.cptr, BENCH_INIT_MSG); 

    /*map the frames for data recording into benchmark pro*/
    r_cp = map_frames_to(&env.bench_thread, env.record_frames, 
            CONFIG_BENCH_RECORD_PAGES, PAGE_BITS_4K); 
    
    printf("frames map to benchmark, manager: %p, side-bench: %p\n", 
           env.record_vaddr, r_cp );

    printf("sending record vaddr... \n");
    send_msg_to(env.bench_thread.fault_endpoint.cptr, (seL4_Word)r_cp); 

    r_cp = map_frames_to(&env.bench_thread, env.huge_frames, 
            total, huge_page_size); 
    
    printf("huge frames map to benchmark, manager: %p, side-bench: %p, %d pages\n", 
            env.huge_vaddr, r_cp, total );

    printf("sending huge page vaddr... \n");
    send_msg_to(env.bench_thread.fault_endpoint.cptr, (seL4_Word)r_cp); 

#ifdef CONFIG_ARCH_ARM 

        wait_msg_from(env.bench_thread.fault_endpoint.cptr); 

        printf("clean and invalidate the huge pages from cache....\n"); 
        for (uint32_t i = 0; i < total; i++) {

            seL4_ARM_Page_CleanInvalidate_Data(env.huge_frames[i], 0, (1 << huge_page_size)); 
            seL4_ARM_Page_CleanInvalidate_Data(env.huge_frames[i], 0, (1 << huge_page_size)); 
        }
        printf("starting benchmark in side-bench... \n");
        send_msg_to(env.bench_thread.fault_endpoint.cptr, 0); 
#endif
    
    printf("waiting on results... \n");
    /*waiting on benchmark to finish*/
    bench_result = wait_msg_from(env.bench_thread.fault_endpoint.cptr); 
    

    assert(bench_result != BENCH_FAILURE); 

    /*processing record*/
    bench_process_data(&env, bench_result); 

}
#endif 


