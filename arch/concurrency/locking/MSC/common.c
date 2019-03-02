#include <getopt.h>
#include "common.h"

int nthreads, niter, verify_mode;


void set_default_args()
{
    nthreads = DEFAULT_NTHREADS;
    niter = 100;
    verify_mode = 1;
}

void usage(char *cmd)
{
    set_default_args();
    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "\t-h\tDisplay this help\n");
    fprintf(stderr, "\t-n\tNumber of threads (default: %d)\n", nthreads);
    fprintf(stderr, "\t-i\tNumber of iterations per thread (default: %d)\n", niter);
    fprintf(stderr, "\t-v\tVerification mode (default: %d)\n", verify_mode);
}

int check_unsigned(char *str)
{
    return (strlen(str) == strspn(str,"0123456789") );
}

void process_args(int argc, char **argv)
{
    int c, rank;

    set_default_args();

    while((c = getopt(argc, argv, "hn:i:v:")) != -1) {
        printf("Process option '%c'\n", c);
        switch (c) {
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        case 'n':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            nthreads = atoi(optarg);
            printf("nthreads = %d\n", nthreads);
            break;
        case 'i':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            niter = atoi(optarg);
            printf("niters = %d\n", niter);
            break;
        case 'v':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            verify_mode = !!atoi(optarg);
            printf("verify_mode = %d\n", verify_mode);
            break;
        default:
            c = -1;
            goto error;
        }
    }
    return;
error:
    if( c != -1 ){
        fprintf(stderr, "Bad argument of '-%c' option\n", (char)c);
    }
    usage(argv[0]);
    exit(1);
}

void bind_to_core(int thr_idx)
{
    long configured_cpus = 0;
    cpu_set_t set;
    int error_loc = 0, error;
    int ncpus;

    /* How many cpus we have on the node */
    configured_cpus = sysconf(_SC_NPROCESSORS_CONF);

    if( sizeof(set) * 8 < configured_cpus ){
        /* Shouldn't happen soon, currentlu # of cpus = 1024 */
        if( 0 == thr_idx ){
            fprintf(stderr, "The size of CPUSET is larger that we can currently handle\n");
        }
        exit(1);
    }

    if( configured_cpus < thr_idx ){
        fprintf(stderr, "ERROR: (ncpus < thr_idx)\n");
        exit(1);
    }

    CPU_ZERO(&set);
    CPU_SET(thr_idx, &set);
    if( pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
        abort();
    }
}
