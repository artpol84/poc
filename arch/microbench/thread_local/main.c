#include <stdio.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

#define ITERATIONS 1000000

int main()
{
	int i;
	double time = GET_TS();
	for(i=0; i<ITERATIONS; i++) {
		func();
	}
	time = GET_TS() - time;
	printf("lat = %lf\n", 1000000 * time / ITERATIONS);
}