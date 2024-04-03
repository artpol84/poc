#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include "args.h"
#include "arch.h"
#include "platform.h"
#include "utils.h"
#include "ucx_mem_bw.h"
#include "basic_access.h"
#include "exec_infra.h"

int main(int argc, char **argv)
{
    char *buf = NULL;
    int i, j;
    volatile uint64_t sum, val = 4;
    cache_struct_t cache;
    exec_infra_desc_t desc;
    common_args_t args;
 
    
    tests_reg(TEST_UCX, TEST_UCX_DESCR, run_ucx_mem_bw); 
    tests_reg(TEST_CACHE, TEST_CACHE_DESCR, run_buf_strided_access);

    args_common_process(argc, argv, &args);

    printf("Freuency: %lf\n", clck_per_sec());
    caches_discover(&cache);

    if (args.get_cache_info) {
        /* Exit without running the benchmark */
        caches_output(&cache, 1);
        return 0;
    } else {
        caches_output(&cache, 0);
    }

    if (args.nthreads == USE_ALL_CORES) {
        args.nthreads = cache.topo.core_subs.count;
    } else if (args.nthreads > cache.topo.core_subs.count) {
        printf("Too many parallel threads requested (%u), only %d available. reset ...\n",
                args.nthreads, cache.topo.core_subs.count);
        args.nthreads = cache.topo.core_subs.count;
    }

    printf("Executing '%s' benchmark\n", args.test);
    desc.run_time = args.run_time;
    desc.test_arg = args.modifier;
    desc.focus_size = args.buf_size_focus;
    desc.min_iter = args.min_iter;
    desc.bind_not_a_fail = args.bind_not_a_fail;
    printf("nthreades = %d\n", args.nthreads);
    exec_assign_res(&cache, &desc, args.nthreads);

    if (exec_test(&cache, args.test, &desc)) {
        printf("Failed to execute test '%s'\n", args.test);
        
    }

    caches_finalize(&cache);

    return 0;
}
