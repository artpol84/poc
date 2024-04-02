#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "arch.h"
#include "platform.h"
#include "utils.h"
#include "ucx_mem_bw.h"
#include "basic_access.h"
#include "exec_infra.h"

double run_time;
int min_iter;
char *test, *modifier;
ssize_t buf_size_focus;
int get_cache_info;
uint32_t nthreads;
int bind_not_a_fail = 0;

void set_default_args()
{
    run_time = 1;  /* 1 sec */
    test = TEST_UCX;
    modifier = "";
    buf_size_focus = -1;
    get_cache_info = 0;
    min_iter = 10;
    nthreads = 1;
}

void usage(char *cmd)
{

    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "\t-h        Display this help\n");
    fprintf(stderr, "\t-i        Print cache information only\n");
    fprintf(stderr, "Test description:\n");
    fprintf(stderr, "\t-b        Do not treat fail to bind as fatal error\n");
    fprintf(stderr, "\t-p [arg]  Execute test on all (if no arg is given) or 'arg' available cores and report cumulative data\n");
    fprintf(stderr, "\t-r <arg>  Minimum run time of the test in seconds (the benchmark will adjust # of iterations, default: %.1lf)\n", run_time);
    fprintf(stderr, "\t-a <arg>  Minimum number of iteraions (default: %d)\n", min_iter);
    fprintf(stderr, "\t-t <arg>  Test name (see the list below, default: %s)\n", test);
    fprintf(stderr, "\t-m <arg>  Test modifier if applicable (see the list below)\n");
    fprintf(stderr, "\t-s <arg>  Focus on specific buffer size\n");
    
    fprintf(stderr, "\tAvailable tests & modifiers:\n");
    tests_print();
}

void process_args(int argc, char **argv)
{
    char *tmp;
    int c;

    set_default_args();
    
    while((c = getopt(argc, argv, "hibp:r:a:t:m:s:")) != -1) {
        switch (c) {
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        case 'i':
            get_cache_info = 1;
            break;
        case 'b':
            bind_not_a_fail = 1;
            break;
        case 'p':
            nthreads = atoi(optarg);
            break;
        case 'r':
            /* from the getopt perspective thrds is an optarg. Check that 
             * user haven't specified anything else */
            run_time = strtod(optarg, &tmp);
            break;
        case 'a':
            /* from the getopt perspective thrds is an optarg. Check that 
             * user haven't specified anything else */
            min_iter = atoi(optarg);
            break;
        case 't':
            test = strdup(optarg);
            break;
        case 'm':
            modifier = strdup(optarg);
            break;
        case 's': {
            int64_t tmp = atoll(optarg);
            if (tmp > 0) {
                buf_size_focus = tmp;
            }
            break;
        }
        default:
            c = -1;
            goto error;
        }
    }
    return;
error:
    
    fprintf(stderr, "Bad argument of '-%c' option\n", (char)c);
    usage(argv[0]);
    exit(1);
}

int main(int argc, char **argv)
{
    char *buf = NULL;
    int i, j;
    volatile uint64_t sum, val = 4;
    cache_struct_t cache;
    exec_infra_desc_t desc;
 
    
    tests_reg(TEST_UCX, TEST_UCX_DESCR, run_ucx_mem_bw); 
    tests_reg(TEST_CACHE, TEST_CACHE_DESCR, run_buf_strided_access);

    process_args(argc, argv);

    printf("Freuency: %lf\n", clck_per_sec());
    caches_discover(&cache);

    if (get_cache_info) {
        /* Exit without running the benchmark */
        caches_output(&cache, 1);
        return 0;
    } else {
        caches_output(&cache, 0);
    }

    if (nthreads == USE_ALL_CORES) {
        nthreads = cache.topo.core_subs.count;
    } else if (nthreads > cache.topo.core_subs.count) {
        printf("Too many parallel threads requested (%u), only %d available. reset ...\n",
                nthreads, cache.topo.core_subs.count);
        nthreads = cache.topo.core_subs.count;
    }

    printf("Executing '%s' benchmark\n", test);
    desc.run_time = run_time;
    desc.test_arg = modifier;
    desc.focus_size = buf_size_focus;
    desc.min_iter = min_iter;
    desc.bind_not_a_fail = bind_not_a_fail;
    printf("nthreades = %d\n", nthreads);
    exec_assign_res(&cache, &desc, nthreads);

    if (exec_test(&cache, test, &desc)) {
        printf("Failed to execute test '%s'\n", test);
        
    }

    caches_finalize(&cache);

    return 0;
}
