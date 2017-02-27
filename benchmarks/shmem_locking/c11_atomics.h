#ifndef MY_C11_ATOMICS_H
#define MY_C11_ATOMICS_H

#include <stdatomic.h>
#include <pthread.h>

typedef struct
{
    atomic_int status;
    pthread_mutex_t wlock, wclock, rclock;
    pthread_cond_t wcond, rcond;
} my_lock_t;

#endif