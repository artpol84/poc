#include "lock.h"
#include <mpi.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>

static init_by_me = 0;

int shared_rwlock_create(my_lock_t *lock)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(lock, &attr);
    pthread_mutexattr_destroy(&attr);
    init_by_me = 1;
}

int shared_rwlock_init(my_lock_t *lock)
{
}

void shared_rwlock_wlock(my_lock_t *lock)
{
    pthread_mutex_lock(lock);
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    pthread_mutex_lock(lock);
}

void shared_rwlock_unlock(my_lock_t *lock)
{
    pthread_mutex_unlock(lock);
}

int shared_rwlock_fin(my_lock_t *lock)
{
    if( init_by_me ){
        pthread_mutex_destroy(lock);
    }
}   

