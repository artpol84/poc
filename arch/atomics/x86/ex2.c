#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "x86.h"

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})


volatile int acnt;
volatile int start = 0;

int cnt;
int nthr, niter;
int indexes[200];
double timings[200][2];
 
void *f(void* thr_data)
{
    int my_idx = *(int*)thr_data;
    int n;

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(my_idx, &set);
    if( pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
        abort();
    }

    
    while( !start );

    timings[my_idx][0] = GET_TS();
    
    for(n = 0; n < niter; ++n) {
        int flag = 0;
        while( !flag ){
            int tmp = acnt;
            flag = CAS(&acnt, tmp, tmp + 1);
        }
        ++cnt; // undefined behavior, in practice some updates missed
    }
    timings[my_idx][1] = GET_TS();
    return 0;
}
 
int main(int argc, char **argv)
{
    int n;

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000;


    if( argc < 3 ){
        printf("Want <nthr> and <niter>\n");
        return 0;
    }
    nthr = atoi(argv[1]);
    niter = atoi(argv[2]);

    pthread_t thr[nthr];

    for(n = 0; n < nthr; ++n){
        indexes[n] = n;
        pthread_create(&thr[n], NULL, f, &indexes[n]);
    }

    nanosleep(&ts, NULL);

    start = 1;
    for(n = 0; n < nthr; ++n)
        pthread_join(thr[n], NULL);
 
    double min_start = GET_TS(), max_start = 0, avg_start = 0;
    double min_work = GET_TS(), max_work = 0, avg_work = 0;
    int min_w_idx = -1, max_w_idx = -1;
    for(n=0; n < nthr; n++){
        if( min_start > timings[n][0] ){
            min_start = timings[n][0];
        }
        avg_start += timings[n][0];
        if( max_start < timings[n][0] ){
            max_start = timings[n][0];
        }
        double interval = timings[n][1] - timings[n][0];
        if( interval < min_work ){
            min_work = interval;
            min_w_idx = n;
        }
        if( interval > max_work ){
            max_w_idx = n;
            max_work = interval;
        }
        avg_work += interval;
    }
    printf("The atomic counter is %u\n", acnt);
    printf("The non-atomic counter is %u\n", cnt);
    printf("Start: %lf/%lf/%lf\n", avg_start / nthr, min_start, max_start);
    printf("Work : %lf / %lf / %lf %d/%d\n", avg_work / nthr, min_work, max_work, min_w_idx, max_w_idx);
    return 0;
}
