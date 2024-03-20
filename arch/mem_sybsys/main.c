#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "arch.h"
#include "mem_sys.h"
#include "utils.h"
#include "ucx_mem_bw.h"
#include "basic_access.h"
#include "exec_infra.h"

double run_time;
int min_iter;
char *test, *modifier;
ssize_t buf_size_focus;
int get_cache_info;

void set_default_args()
{
    run_time = 1;  /* 1 sec */
    test = TEST_UCX;
    modifier = "";
    buf_size_focus = -1;
    get_cache_info = 0;
    min_iter = 10;
}

void usage(char *cmd)
{

    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "\t-h        Display this help\n");
    fprintf(stderr, "\t-i        Print cache information only\n");
    fprintf(stderr, "Test description:\n");
    fprintf(stderr, "\t-r [arg]  Minimum run time of the test in seconds (the benchmark will adjust # of iterations, default: %.1lf)\n", run_time);
    fprintf(stderr, "\t-a [arg]  Minimum number of iteraions (default: %d)\n", min_iter);
    fprintf(stderr, "\t-t [arg]  Test name (see the list below, default: %s)\n", test);
    fprintf(stderr, "\t-m [arg]  Test modifier if applicable (see the list below)\n");
    fprintf(stderr, "\t-s [arg]  Focus on specific buffer size\n");
    
    fprintf(stderr, "\tAvailable tests & modifiers:\n");
    tests_print();
}

void process_args(int argc, char **argv)
{
    char *tmp;
    int c;

    set_default_args();
    
    while((c = getopt(argc, argv, "hir:a:t:m:s:")) != -1) {
        switch (c) {
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        case 'i':
            get_cache_info = 1;
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
        return 0;
    }

    /* In case discovery failed - use some default values */
    if (!cache.nlevels) {
        printf("Cache subsystem discovery failed - use defaults\n");
        caches_set_default(&cache);
    }

    printf("Executing '%s' benchmark\n", test);
    desc.run_time = run_time;
    desc.test_arg = modifier;
    desc.focus_size = buf_size_focus;
    desc.min_iter = min_iter;
    if (tests_exec(&cache, test, &desc)) {
        printf("Failed to execute test '%s'\n", test);
        
    }

    // printf("\n");
    // printf("===================================\n");
    // printf("Recreate UCX memory BW performance:\n\n");
    // run_ucx_mem_bw(&cache, run_time);


    // printf("\n");
    // printf("===================================\n");
    // printf("Performance of '=' operation\n\n");

    // printf("Loop unroll = 1\n");

    // printf("Sequential access to a buffer:\n");
    // run_buf_strided_access(&cache, run_time, 1, BACCESS_CB_NAME(assign, uint64_t, 1));

    // printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    // run_buf_strided_access(&cache, run_time, cache.cl_size, BACCESS_CB_NAME(assign, uint64_t, 1));

    // printf("Loop unroll = 8\n");

    // printf("Sequential access to a buffer:\n");
    // run_buf_strided_access(&cache, run_time, 1, BACCESS_CB_NAME(assign, uint64_t, 8));

    // printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    // run_buf_strided_access(&cache, run_time, cache.cl_size, BACCESS_CB_NAME(assign, uint64_t, 8));


    // printf("\n");
    // printf("===================================\n");
    // printf("Performance of '+=' operation\n\n");

    // printf("Loop unroll = 1\n");

    // printf("Sequential access to a buffer:\n");
    // run_buf_strided_access(&cache, run_time, 1, BACCESS_CB_NAME(inc, uint64_t, 1));

    // printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    // run_buf_strided_access(&cache, run_time, cache.cl_size, BACCESS_CB_NAME(inc, uint64_t, 1));

    // printf("Loop unroll = 8\n");

    // printf("Sequential access to a buffer:\n");
    // run_buf_strided_access(&cache, run_time, 1, BACCESS_CB_NAME(inc, uint64_t, 8));

    // printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    // run_buf_strided_access(&cache, run_time, cache.cl_size, BACCESS_CB_NAME(inc, uint64_t, 8));

    // printf("\n");
    // printf("===================================\n");
    // printf("Performance of '*=' operation\n\n");

    // printf("Loop unroll = 1\n");

    // printf("Sequential access to a buffer:\n");
    // run_buf_strided_access(&cache, run_time, 1, BACCESS_CB_NAME(mul, uint64_t, 1));

    // printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    // run_buf_strided_access(&cache, run_time, cache.cl_size, BACCESS_CB_NAME(mul, uint64_t, 1));

    // printf("Loop unroll = 8\n");

    // printf("Sequential access to a buffer:\n");
    // run_buf_strided_access(&cache, run_time, 1, BACCESS_CB_NAME(mul, uint64_t, 8));

    // printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    // run_buf_strided_access(&cache, run_time, cache.cl_size, BACCESS_CB_NAME(mul, uint64_t, 8));

    return 0;
}
