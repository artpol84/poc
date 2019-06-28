#include <getopt.h>
#include "common.h"
#include "arch.h"

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
    if( error = pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
        printf("bind_to_core: pthread_setaffinity_np returned \"%d\"\n", error);
        exit(0);
    }
}

int32_t global_thread_id = -1;
__thread int32_t local_thread_id = -1;

uint32_t get_thread_id()
{
    if( unlikely(local_thread_id < 0) ){
        local_thread_id = atomic_inc(&global_thread_id, 1);
    }
    return local_thread_id;
}

/*
Copyright 2019 Artem Y. Polyakov <artpol84@gmail.com>
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without specific
prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/
