#include <stdio.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>

#include "lib.h"

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
    })

int indexes[1024];
double results[1024] = { 0 };
int niter, nthr, start = 0;

void do_bind(int idx)
{
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(idx, &set);
    if( pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
        printf("FAIL to bind the thread!\n");
        exit(1);
    }
}

void *eval_tloc(void *data)
{
    int myidx = *(int*)data;
    int i;

    do_bind(myidx);

    double time = GET_TS();
    for(i=0; i<niter; i++) {
        func_tloc();
    }
    results[myidx] = GET_TS() - time;
}

void *eval_glob(void *data)
{
    int myidx = *(int*)data;
    int i;

    do_bind(myidx);

    double time = GET_TS();
    for(i=0; i<niter; i++) {
        func_glob();
    }
    results[myidx] = GET_TS() - time;
}


void *eval_floc(void *data)
{
    int myidx = *(int*)data;
    int i;

    do_bind(myidx);

    double time = GET_TS();
    for(i=0; i<niter; i++) {
        func_floc();
    }
    results[myidx] = GET_TS() - time;
}

void *eval_fstatic(void *data)
{
    int myidx = *(int*)data;
    int i;

    do_bind(myidx);

    double time = GET_TS();
    for(i=0; i<niter; i++) {
        func_fstatic();
    }
    results[myidx] = GET_TS() - time;
}

void *eval_pth_key(void *data)
{
    int myidx = *(int*)data;
    int i;

    do_bind(myidx);

    double time = GET_TS();
    for(i=0; i<niter; i++) {
        func_pth_key();
    }
    results[myidx] = GET_TS() - time;
}

int execute(char *prefix, void *(*func)(void*))
{
    int i;
    struct timespec ts = { 0, 10000000 };
    double sum = 0;

    if( nthr == 1 ){
        int idx = 0;
        func(&idx);
    } else {
        pthread_t thr[nthr];

        for( i = 0; i < nthr; ++i){
            indexes[i] = i;
            pthread_create(&thr[i], NULL, func, &indexes[i]);
        }
        nanosleep(&ts, NULL);

        start = 1;
        for( i = 0; i < nthr; ++i)
            pthread_join(thr[i], NULL);
    }

    sum = 0;
    for(i = 0; i < nthr; i++) {
        sum += results[i];
    }

    printf("%s: avg lat = %lf us\n", prefix, 1E6 * sum / niter / nthr);

}

int main(int argc, char **argv)
{
    if( argc < 3 ) {
        printf("Want <nthr> and <niter>\n");
        return 0;
    }
    nthr = atoi(argv[1]);
    niter = atoi(argv[2]);

    if( nthr < 0 ){
        printf("Bad number of threads: %d\n", nthr);
        return 0;
    }

    lib_init();

    execute("stack:", eval_floc);
    execute("fstatic:", eval_fstatic);
    execute("global:", eval_glob);
    execute("tlocal:", eval_tloc);
    execute("pth_key:", eval_pth_key);

    return 0;
}
