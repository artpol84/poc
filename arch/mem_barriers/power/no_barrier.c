#include <stdio.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

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

inline void dummy_sync() { }

#define NUM_EXP 1000
int consumer_observed[NUM_EXP][2];

#define CORE_CONSUMER_REVERSE(sync_primitive)   \
void consumer_rev_ ## sync_primitive () {           \
    int n;                                      \
    for(n=0; n<NUM_EXP; n++){                   \
        int f2 = flag2;                         \
        sync_primitive ();                   \
        int f1 = flag1;                         \
        consumer_observed[n][0] = f1;           \
        consumer_observed[n][1] = f2;           \
    }                                           \
}

#define CORE_CONSUMER_REVERSE_NAME(sync_primitive) \
    consumer_rev_ ## sync_primitive

#define CORE_CONSUMER_STRAIGHT(sync_primitive)   \
void consumer_strght_ ## sync_primitive  () {           \
    int n;                                      \
    for(n=0; n<NUM_EXP; n++){                   \
        int f1 = flag1;                         \
        sync_primitive ();                   \
        int f2 = flag2;                         \
        consumer_observed[n][0] = f1;           \
        consumer_observed[n][1] = f2;           \
    }                                           \
}

#define CORE_CONSUMER_STRAIGHT_NAME(sync_primitive) \
    consumer_strght_ ## sync_primitive


CORE_CONSUMER_REVERSE(isync)
CORE_CONSUMER_REVERSE(lwsync)
CORE_CONSUMER_REVERSE(sync)
CORE_CONSUMER_REVERSE(dummy_sync)

CORE_CONSUMER_STRAIGHT(isync)
CORE_CONSUMER_STRAIGHT(lwsync)
CORE_CONSUMER_STRAIGHT(sync)
CORE_CONSUMER_STRAIGHT(dummy_sync)

typedef struct {
    int idx;
    void (*core)();
} thread_priv_t;

void *consumer(void* thr_data)
{
    thread_priv_t *priv = thr_data;
    int my_idx = priv->idx;
    int n;
    double time;

    bind_to_cpu(my_idx);
    
    hwsync();
    
    while( !start );

    isync();

    time = GET_TS();
    
    priv->core();
    
    time = GET_TS() - time;
    
    stop = 1;
    hwsync();
    
    int violations = 0;
    for(n=0; n<NUM_EXP; n++){
        if( consumer_observed[n][0] < consumer_observed[n][1] ) {
            violations++;
        }
    }
    
    printf("TIME = %lf, violations: %d\n", time, violations);
    return 0;
}

#define CORE_PODUCER(sync_primitive)            \
void producer_ ## sync_primitive () {          \
    while(!stop) {                              \
        flag1++;                                \
        sync_primitive ();                   \
        flag2++;                                \
    }                                           \
}

#define CORE_PRODUCER_NAME(sync_primitive) \
    producer_ ## sync_primitive

CORE_PODUCER(isync)
CORE_PODUCER(lwsync)
CORE_PODUCER(sync)
CORE_PODUCER(dummy_sync)

void *producer(void* thr_data)
{
    thread_priv_t *priv = thr_data;
    int my_idx = priv->idx;

    volatile int tmp = 13;
    bind_to_cpu(my_idx);

    hwsync();

    while( !start );
     
    priv->core();
    
    return 0;
}

void run_test(void (*producer_p)(), void (*consumer_p)())
{
    int i, n;
    struct timespec ts;
    int consumer_id = 0;
    int producer_id[5] = {
        1,  /* same core */
        8,  /* same numa */
        40, /* Group #0, NUMA #1 */
        80, /* Group #1, NUMA #0 */
        120/* Group #1, NUMA #1 */
    };

    char *producer_descr[5] = {
        "same core (Group #0, NUMA #0, CPU #0, HWTHR #1",
        "close core (Group #0, NUMA #0, CPU #1, HWTHR #0",
        "close NUMA (Group #0, NUMA #1, CPU #0, HWTHR #0",
        "Group #1, NUMA #0, CPU #0, HWTHR #0",
        "Group #1, NUMA #1, CPU #0, HWTHR #0",
    };

    int producer_cnt = sizeof(producer_id) / sizeof(producer_id[0]);
    ts.tv_sec = 0;
    ts.tv_nsec = 100000;

    pthread_t thr[2];

    for(i=0; i < producer_cnt; i++) {
        memset(consumer_observed, 0, sizeof(consumer_observed));
        start = 0;
        stop = 0;
        flag1 = 0;
        flag2 = 0;

        printf("%s: ", producer_descr[i]);
        hwsync();
        thread_priv_t privs[2];

        privs[0].idx = consumer_id;
        privs[0].core = consumer_p;
        pthread_create(&thr[0], NULL, consumer, (void*)&privs[0]);

        privs[1].idx = producer_id[i];
        privs[1].core = producer_p;
        pthread_create(&thr[1], NULL, producer, (void*)&privs[1]);

        nanosleep(&ts, NULL);

        start = 1;
        hwsync();

        for( n = 0; n < 2; ++n){
            pthread_join(thr[n], NULL);
        }
    }
}

int main(int argc, char **argv)
{

    /* How many cpus we have on the node */
    long configured_cpus = sysconf(_SC_NPROCESSORS_CONF);
    
    /* Bind the driving thread to the last core */
    bind_to_cpu(159);

    printf("ncpus = %ld\n", configured_cpus);

    printf("---------------------- Straight ------------------\n");
    printf("prod:dummy - cons:dummy\n");
    run_test(CORE_PRODUCER_NAME(dummy_sync), CORE_CONSUMER_STRAIGHT_NAME(dummy_sync));
    printf("prod:dummy - cons:isync\n");
    run_test(CORE_PRODUCER_NAME(dummy_sync), CORE_CONSUMER_STRAIGHT_NAME(isync));
    printf("prod:dummy - cons:lwsync\n");
    run_test(CORE_PRODUCER_NAME(dummy_sync), CORE_CONSUMER_STRAIGHT_NAME(lwsync));

    printf("\n");
    printf("prod:isync - cons:dummy\n");
    run_test(CORE_PRODUCER_NAME(isync), CORE_CONSUMER_STRAIGHT_NAME(dummy_sync));
    printf("prod:isync - cons:isync\n");
    run_test(CORE_PRODUCER_NAME(isync), CORE_CONSUMER_STRAIGHT_NAME(isync));
    printf("prod:isync - cons:lwsync\n");
    run_test(CORE_PRODUCER_NAME(isync), CORE_CONSUMER_STRAIGHT_NAME(lwsync));

    printf("\n");
    printf("prod:lwsync - cons:dummy\n");
    run_test(CORE_PRODUCER_NAME(lwsync), CORE_CONSUMER_STRAIGHT_NAME(dummy_sync));
    printf("prod:lwsync - cons:isync\n");
    run_test(CORE_PRODUCER_NAME(lwsync), CORE_CONSUMER_STRAIGHT_NAME(isync));
    printf("prod:lwsync - cons:lwsync\n");
    run_test(CORE_PRODUCER_NAME(lwsync), CORE_CONSUMER_STRAIGHT_NAME(lwsync));


    printf("---------------------- Reversed ------------------\n");
    printf("prod:dummy - cons:dummy\n");
    run_test(CORE_PRODUCER_NAME(dummy_sync), CORE_CONSUMER_REVERSE_NAME(dummy_sync));
    printf("prod:dummy - cons:isync\n");
    run_test(CORE_PRODUCER_NAME(dummy_sync), CORE_CONSUMER_REVERSE_NAME(isync));
    printf("prod:dummy - cons:lwsync\n");
    run_test(CORE_PRODUCER_NAME(dummy_sync), CORE_CONSUMER_REVERSE_NAME(lwsync));

    printf("\n");
    printf("prod:isync - cons:dummy\n");
    run_test(CORE_PRODUCER_NAME(isync), CORE_CONSUMER_REVERSE_NAME(dummy_sync));
    printf("prod:isync - cons:isync\n");
    run_test(CORE_PRODUCER_NAME(isync), CORE_CONSUMER_REVERSE_NAME(isync));
    printf("prod:isync - cons:lwsync\n");
    run_test(CORE_PRODUCER_NAME(isync), CORE_CONSUMER_REVERSE_NAME(lwsync));

    printf("\n");
    printf("prod:lwsync - cons:dummy\n");
    run_test(CORE_PRODUCER_NAME(lwsync), CORE_CONSUMER_REVERSE_NAME(dummy_sync));
    printf("prod:lwsync - cons:isync\n");
    run_test(CORE_PRODUCER_NAME(lwsync), CORE_CONSUMER_REVERSE_NAME(isync));
    printf("prod:lwsync - cons:lwsync\n");
    run_test(CORE_PRODUCER_NAME(lwsync), CORE_CONSUMER_REVERSE_NAME(lwsync));


    return 0;
}
