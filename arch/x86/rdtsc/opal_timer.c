/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2016      Los Alamos National Security, LLC. ALl rights
 *                         reserved.
 * Copyright (c) 2017      Artem Y. Polyakov <artpol84@gmail.com>
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <math.h>

struct ts_t {
    unsigned int l, h;
};

/* TODO: add AMD mfence version and dispatch at init */
static inline void
get_cycles(struct ts_t *ts)
{
     unsigned int l, h;

     asm volatile ("lfence\n\t"
                   "rdtsc\n\t"
                   : "=a" (ts->l), "=d" (ts->h));
}

unsigned long build_ts(struct ts_t *ts) {
     return ((unsigned long)ts->l) | (((unsigned long)ts->h) << 32);
}

#define REPS 2048

int main()
{
    struct ts_t array[REPS] = { 0 };
    unsigned int avg_get = 0, min_get = 0;
    unsigned int avg_add = 0, min_add;
    int i, j;
    int count;

    /* warmup CPU (borrowed from A. Fog measurement programs) */
    volatile double v = 1;
    double x = v;
    for (i = 0; i < 10000000; i++) {
        x = cos(x);
    }
    v = x;

    /* warmup caches */
    for(j=0; j < 10; j++){
        for(i=0; i<REPS; i++){
            get_cycles(array + i);
        }
    }

    /* measure overhead */
    for(i=0; i<REPS; i++){
        get_cycles(array + i);
    }

    min_get = build_ts(array);
    count = 0;
    for(i=1; i<REPS; i++){
        unsigned long cur = build_ts(array + i);
        unsigned long prev = build_ts(array + i - 1);
        unsigned long diff = cur - prev;
        if( cur < prev ){
            /* skip counter overflow */
            continue;
        }
        count++;
        avg_get += (unsigned int)diff;
        min_get = (min_get > diff) ? diff : min_get;
    }
    avg_get /= count;
    printf("avg_get = %u, min_get = %u\n", avg_get, min_get);

    for(i=0; i<REPS; i++){
        get_cycles(array + i);
/*
        asm volatile (
            "add $1, %%eax\n\t"
            : : : "eax"
        );
*/
        asm volatile (
            "add $1, %%eax\n\t"
            "add $1, %%ebx\n\t"
            "add $1, %%ecx\n\t"
            "add $1, %%edx\n\t"
            : : : "%eax", "ebx", 
                  "ecx", "edx"
        );

    }

    min_add = build_ts(array);
    count = 0;
    for(i=1; i<REPS; i++){
        unsigned long cur = build_ts(array + i);
        unsigned long prev = build_ts(array + i - 1);
        unsigned long diff = cur - prev;
        if( cur < prev ) {
            /* skip counter overflow */
            continue;
        }
        count++;
        avg_add += (unsigned int)diff;
        min_add = (min_add > diff) ? diff : min_add;
    }
    avg_add /= count;
    if( avg_add < avg_get ){
        avg_get = min_get;
    }
    printf("avg_add = %u, (avg_add - ovh) = %u\n", avg_add, avg_add - avg_get);
    printf("min_add = %u, (min_ovh) = %u\n", min_add, min_add - min_get);

    return 0;
}
