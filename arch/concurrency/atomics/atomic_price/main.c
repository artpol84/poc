#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <math.h>

#include "x86.h"

inline uint64_t rdtsc() {
    uint64_t ts;
    asm volatile ( 
        "rdtsc\n\t"    // Returns the time in EDX:EAX.
        "shl $32, %%rdx\n\t"  // Shift the upper bits left.
        "or %%rdx, %0"        // 'Or' in the lower bits.
        : "=a" (ts)
        : 
        : "rdx");
    return ts;
}

inline void sfence() {
    uint64_t ts;
    asm volatile ( "sfence\n" : : : "memory");
}

uint64_t shared_val;
int operations_cnt = 0;
int stop = 0;
#define NUM_PROBES (1 * 1024 * 1024)
uint64_t probes[NUM_PROBES + 1] = { 0 };
uint64_t intervals[NUM_PROBES] = { 0 };

pthread_barrier_t tbarrier;

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



void *reader(void *_unused)
{
    int i;

    bind_to_core(0);

    pthread_barrier_wait(&tbarrier);

    for(i=0; i < NUM_PROBES; i++) {
	int j;
	probes[i] = rdtsc();
	for(j=0; j<1024; j++){
	    asm volatile ( 
		"mov (%[ptr]), %%rax\n\t"
	        : 
	        : [ptr] "r" (&shared_val)
	        : "rax");
	}   
    }
    probes[i] = rdtsc();
    stop = 1;
}

typedef void *(*thread_func_t)(void *_unused);

void *writer_mov(void *cbdata)
{
    int use_sfence = *(int*)cbdata;
    int i;
    int tmp = 0;
    uint64_t prev = 0, new = 1;
    shared_val = 0;

    bind_to_core(1);

    pthread_barrier_wait(&tbarrier);

    while( !stop ) {
	int j;
	for(j=0; j<10*1024; j++){
	    asm volatile ( "inc %[ptr]\n\t" 
		: 
		: [ptr] "r" (tmp)  
		: "memory", "cc" );
	}
	if( use_sfence ){
	    sfence();
	}

	asm volatile ( 
    	    "movl $1, (%[ptr])\n\t"
	    : 
	    : [ptr] "r" (&shared_val)
	    : "memory" );
	operations_cnt++;
    }
}


void *writer_cas(void *cbdata)
{
    int use_sfence = *(int*)cbdata;
    int i;
    int tmp = 0;
    uint64_t prev = 0, new = 1;
    shared_val = 0;

    bind_to_core(1);

    pthread_barrier_wait(&tbarrier);

    while( !stop ) {
	int j;
	for(j=0; j<10*1024; j++){
	    asm volatile ( "inc %[ptr]\n\t" 
		: 
		: [ptr] "r" (tmp)  
		: "memory", "cc" );
	}
	if( use_sfence ){
	    sfence();
	}
	CAS(&shared_val, prev, new);
	prev++;
	new++;
	operations_cnt++;
    }
}

void *writer_inc(void *cbdata)
{
    int i;
    int tmp = 0;
    int use_sfence = *(int*)cbdata;

    bind_to_core(1);

    shared_val = 0;
    pthread_barrier_wait(&tbarrier);

    while( !stop ) {
	int j;
	for(j=0; j<10*1024; j++){
	    asm volatile ( "inc %[ptr]\n\t" 
		: 
		: [ptr] "r" (tmp)  
		: "memory", "cc" );
	}
	if( use_sfence ){
	    sfence();
	}
	atomic_fadd64(&shared_val,1 );
	operations_cnt++;
    }
}

void *writer_swap(void *cbdata)
{
    int use_sfence = *(int*)cbdata;
    int i;
    int tmp = 0;
    uint64_t prev = 1;

    bind_to_core(1);

    shared_val = 0;

    pthread_barrier_wait(&tbarrier);

    while( !stop ) {
	int j;
	for(j=0; j<10*1024; j++){
	    asm volatile ( "inc %[ptr]\n\t" 
		: 
		: [ptr] "r" (tmp)  
		: "memory", "cc" );
	}
	if( use_sfence ){
	    sfence();
	}
	atomic_swap(&shared_val, prev);
	operations_cnt++;
    }
}

/* Sort backwards */
int compare(const void *l, const void *r)
{
    uint64_t lv = *(uint64_t*)l;
    uint64_t rv = *(uint64_t*)r;
    if( lv == rv ){
	return 0;
    } else if( lv > rv ) {
	return -1;
    } else {
        return 1;
    }
}

int filter1()
{
    int i;
    // Discard and count number of close to min values
    uint64_t min = intervals[0];
    for(i = 0; i < NUM_PROBES; i++){
	if( min > intervals[i]  ) {
	    min = intervals[i];
	}
    }
    
    int fast_loop_cnt = 0;
    for(i = 0; i < NUM_PROBES; i++){
	if( (double)min*1.1 >= intervals[i] ){
	    fast_loop_cnt++;
	    intervals[i] = 0;
	}
    }

    // Discard too large values
    int slow_loop_cnt = 0;
    for(i = 0; i < NUM_PROBES; i++){
	if( (double)min*4 < intervals[i] ){
	    slow_loop_cnt++;
	    intervals[i] = 0;
	}
    }
    
    // Push those minimums to the end
    qsort(intervals, NUM_PROBES, sizeof(intervals[0]), compare);
        
    // find the second-level minimum
    int new_len = NUM_PROBES - fast_loop_cnt - slow_loop_cnt;
    printf("\tnormal loop: %lu (%d, %lf%% of total)\n", min, fast_loop_cnt, (double)fast_loop_cnt / NUM_PROBES );
    printf("\tatomic_overhead: %lu (%d)\n", intervals[new_len / 2] - min, new_len);
}

int main(int argc, char **argv)
{
    int i, t;

    pthread_barrier_init(&tbarrier, NULL, 3);
    pthread_t id[2];

    shared_val = 0;
    
    char *test_names[] = { "regilar Write", "atomic CAS", "atomic CAS (sfence)", "atomic Inc", "atomic Inc (sfence)", "atomic Swap", "atomic Swap (sfence)" };
    thread_func_t fptr[] = { writer_mov, writer_cas, writer_cas, writer_inc, writer_inc, writer_swap, writer_swap };
    int thread_data[] = { 0 /*no sfence*/, 0, 1 /*use sfence*/, 0, 1, 0, 1 };

    for(t = 0; t < sizeof(test_names)/sizeof(test_names[0]); t++) {
	operations_cnt = 0;
	stop = 0;
	/* setup and create threads */
        pthread_create(&id[0], NULL, reader, (void*)NULL);
        pthread_create(&id[1], NULL, fptr[t], (void*)&thread_data[t]);

	/* Start the test */
        pthread_barrier_wait(&tbarrier);

        for (i=0; i<2; i++) {
	    pthread_join(id[i], NULL);
        }
    
	for(i = 0; i < NUM_PROBES; i++){
    	    intervals[i] = probes[i+1] - probes[i];
    	    if( 0 &&  t == 1 ){
    		printf("%lu\n", intervals[i]);
    	    }
        }

	printf("Test: %s\n", test_names[t]);
        printf("\ttotal_count = %d\n", NUM_PROBES);
	printf("\toperations_cnt = %d\n", operations_cnt);
	filter1();
	memset(probes, 0, sizeof(probes));
	memset(intervals, 0, sizeof(intervals));
    }
}
