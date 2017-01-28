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
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
#if (MY_PTHREAD_PRIO == 1)
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
#endif
    pthread_rwlock_init(lock, &attr);
    pthread_rwlockattr_destroy(&attr);
    init_by_me = 1;
}

int shared_rwlock_init(my_lock_t *lock)
{
}

void shared_rwlock_wlock(my_lock_t *lock)
{
    pthread_rwlock_wrlock(lock);
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    pthread_rwlock_rdlock(lock);
}

void shared_rwlock_unlock(my_lock_t *lock)
{
    pthread_rwlock_unlock(lock);
}

int shared_rwlock_fin(my_lock_t *lock)
{
    if( init_by_me ){
        pthread_rwlock_destroy(lock);
    }
}   

