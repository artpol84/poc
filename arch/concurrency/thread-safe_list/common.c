#include <getopt.h>
#include "common.h"
#include "x86.h"

int nthreads, nadds, nbatch, nskip, nrems;


void set_default_args()
{
    nthreads = DEFAULT_NTHREADS;
    nadds = 100;
    nbatch = 1;
    nskip = 1;
}

void usage(char *cmd)
{
    set_default_args();
    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "\t-h\tDisplay this help\n");
    fprintf(stderr, "\t-n\tNumber of threads (default: %d)\n", nthreads);
    fprintf(stderr, "\t-a\tNumber of elements to add (default: %d)\n", nadds);
    fprintf(stderr, "\t-b\tBatching appends by x elements (default: %d)\n", nbatch);
    fprintf(stderr, "\t-s\tSkip n tail mismatches before adjusting list tail"
                    " (default: %d)\n", nskip);
}

int check_unsigned(char *str)
{
    return (strlen(str) == strspn(str,"0123456789") );
}

void process_args(int argc, char **argv)
{
    int c, rank;

    set_default_args();

    while((c = getopt(argc, argv, "hn:a:b:s:")) != -1) {
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
            if (0 >= nthreads) {
                goto error;
            }
            break;
        case 'a':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            nadds = atoi(optarg);
            printf("niters = %d\n", nadds);
            if (0 >= nadds) {
                goto error;
            }
            break;
        case 'b':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            nbatch = atoi(optarg);
            break;
        case 's':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            nskip = atoi(optarg);
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

int32_t global_thread_id = -1;
__thread int32_t local_thread_id = -1;

uint32_t get_thread_id()
{
    if( local_thread_id < 0 ){
        local_thread_id = atomic_inc(&global_thread_id, 1);
    }
    return local_thread_id;
}
