#ifndef MY_PTHREAD_NLOCKS_H
#define MY_PTHREAD_NLOCKS_H
#include <pthread.h>

typedef struct {
    pthread_rwlock_t locks[512];
} nlock_t;
    

#define my_lock_t nlock_t

#endif
