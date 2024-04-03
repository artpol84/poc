#include "args.h"

void args_init_common(common_args_t *args)
{
    common_args_t ref_args = {
        .run_time = 1,  /* 1 sec */
        .test = TEST_UCX,
        .modifier = "",
        .buf_size_focus = -1,
        .get_cache_info = 0,
        .min_iter = 10,
        .nthreads = 1
    };

    *args = ref_args;
}

void args_common_usage(char *cmd)
{
    common_args_t args;
    args_init_common(&args);

    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "\t-h        Display this help\n");
    fprintf(stderr, "\t-i        Print cache information only\n");
    fprintf(stderr, "Test description:\n");
    fprintf(stderr, "\t-b        Do not treat fail to bind as fatal error\n");
    fprintf(stderr, "\t-p [arg]  Execute test on all (if no arg is given) or 'arg' available cores and report cumulative data\n");
    fprintf(stderr, "\t-r <arg>  Minimum run time of the test in seconds (the benchmark will adjust # of iterations, default: %.1lf)\n", args.run_time);
    fprintf(stderr, "\t-a <arg>  Minimum number of iteraions (default: %d)\n", args.min_iter);
    fprintf(stderr, "\t-t <arg>  Test name (see the list below, default: %s)\n", args.test);
    fprintf(stderr, "\t-m <arg>  Test modifier if applicable (see the list below)\n");
    fprintf(stderr, "\t-s <arg>  Focus on specific buffer size\n");
    
    fprintf(stderr, "\tAvailable tests & modifiers:\n");
    tests_print();
}

void args_common_process(int argc, char **argv, common_args_t *args)
{
    char *tmp;
    int c;

    args_init_common(args);
    
    while((c = getopt(argc, argv, "hibp:r:a:t:m:s:")) != -1) {
        switch (c) {
        case 'h':
            args_common_usage(argv[0]);
            exit(0);
            break;
        case 'i':
            args->get_cache_info = 1;
            break;
        case 'b':
            args->bind_not_a_fail = 1;
            break;
        case 'p':
            args->nthreads = atoi(optarg);
            break;
        case 'r':
            /* from the getopt perspective thrds is an optarg. Check that 
             * user haven't specified anything else */
            args->run_time = strtod(optarg, &tmp);
            break;
        case 'a':
            /* from the getopt perspective thrds is an optarg. Check that 
             * user haven't specified anything else */
            args->min_iter = atoi(optarg);
            break;
        case 't':
            args->test = strdup(optarg);
            break;
        case 'm':
            args->modifier = strdup(optarg);
            break;
        case 's': {
            int64_t tmp = atoll(optarg);
            if (tmp > 0) {
                args->buf_size_focus = tmp;
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
    args_common_usage(argv[0]);
    exit(1);
}
