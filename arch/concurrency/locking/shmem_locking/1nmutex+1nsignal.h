#ifndef MY_PTHREAD_NLOCKS_H
#define MY_PTHREAD_NLOCKS_H
#include <pthread.h>

typedef struct  {
    pthread_mutex_t lock;
    volatile int signal;
} nlock_elem_int_t;

typedef struct  {
    nlock_elem_int_t e;
    char padding[64 - sizeof(nlock_elem_int_t)];
} nlock_int_t;

typedef struct {
    nlock_int_t locks[512];
} nlock_t;
    

#define my_lock_t nlock_t

#endif
