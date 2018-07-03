#include <stdio.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <numa.h>

#include "ppc.h"

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

inline void bind_to_cpu(int idx)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(idx, &set);
    if( pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
        abort();
    }
}

volatile int flag1 = 0;
volatile int flag2 = 1;

volatile int start = 0;
volatile int stop = 0;

#define NUM_EXP 1000
int consumer_observed[NUM_EXP][2];

void *consumer(void* thr_data)
{
    int my_idx = *(int*)thr_data;
    int n;
    double time;

    bind_to_cpu(my_idx);
    
    hwsync();
    
    while( !start );

    isync();

    time = GET_TS();
    
    for(n=0; n<NUM_EXP; n++){
        int f2 = flag2;     
        isync();
//        lwsync();
        int f1 = flag1;
        consumer_observed[n][0] = f1;
        consumer_observed[n][1] = f2;
    }
    
    time = GET_TS() - time;
    
    stop = 1;
    hwsync();
    
    for(n=0; n<NUM_EXP; n++){
        if( consumer_observed[n][0] < consumer_observed[n][1] ) {
            printf("n=%d: f1=%d, f2 = %d\n", n, consumer_observed[n][0], consumer_observed[n][1]);
        }
    }
    
    printf("TIME = %lf\n", time);
    return 0;
}

void *producer(void* thr_data)
{
    int my_idx = *(int*)thr_data;
    int n;

    volatile int tmp = 13;
    bind_to_cpu(my_idx);

    hwsync();

    while( !start );
     
    while(!stop) {
        flag1++; // = tmp / 13;
        isync();
//        lwsync();
        flag2++;
//        if( !(flag2 % (10*NUM_EXP)) ) {
//            hwsync();
//        }
        tmp += 13;
    }
    
    return 0;
}


int main(int argc, char **argv)
{
    int n, i;
    struct timespec ts;
    int consumer_id = 0;
    int producer_id[5] = {
        1,  /* same core */
        8,  /* same numa */
        40, /* Group #0, NUMA #1 */
        80, /* Group #1, NUMA #0 */
        120/* Group #1, NUMA #1 */
    };
    int producer_cnt = sizeof(producer_id) / sizeof(producer_id[0]);
    ts.tv_sec = 0;
    ts.tv_nsec = 100000;

    /* How many cpus we have on the node */
    long configured_cpus = sysconf(_SC_NPROCESSORS_CONF);
    
    /* Bind the driving thread to the last core */
    bind_to_cpu(159);

    printf("ncpus = %ld\n", configured_cpus);

    pthread_t thr[2];

    for(i=0; i < producer_cnt; i++) {
        memset(consumer_observed, 0, sizeof(consumer_observed));
        start = 0;
        stop = 0;
        flag1 = 0;
        flag2 = 0;
        
        printf("--------------- Producer index = %d --------------\n", producer_id[i]);
        hwsync();
        pthread_create(&thr[0], NULL, consumer, &consumer_id);
        pthread_create(&thr[1], NULL, producer, &producer_id[i]);

        nanosleep(&ts, NULL);

        start = 1;
        hwsync();
    
        for( n = 0; n < 2; ++n){
            pthread_join(thr[n], NULL);
        }
    }
    return 0;
}
