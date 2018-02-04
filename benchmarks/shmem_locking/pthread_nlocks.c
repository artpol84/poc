#include "pthread_nlocks.h"
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

static int init_by_me = 0;
static int lock_num = 0;
static int lock_idx = 0;

int shared_rwlock_create(my_lock_t *lock)
{
    int i;
    MPI_Comm_size(MPI_COMM_WORLD, &lock_num);
    lock_num--;
    if( sizeof(lock->locks) / sizeof(lock->locks[0]) * 2 < lock_num ) {
        printf("Too many processes, cannot allocate enough locks\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
#if (MY_PTHREAD_PRIO == 1)
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
#endif
    for(i = 0; i < 2 * lock_num; i++) {
        pthread_rwlock_init(&lock->locks[i], &attr);
    }
    pthread_rwlockattr_destroy(&attr);
    init_by_me = 1;
}

int shared_rwlock_init(my_lock_t *lock)
{
    MPI_Comm_rank(MPI_COMM_WORLD, &lock_idx);
    lock_idx--;
}

void shared_rwlock_wlock(my_lock_t *lock)
{
    int i;
    /* Lock the second lock first */
    for(i=0; i<lock_num; i++) {
        pthread_rwlock_wrlock(&lock->locks[2*i + 1]);
    }
    /* Now lock the first one */
    for(i=0; i<lock_num; i++) {
        pthread_rwlock_wrlock(&lock->locks[2*i]);
    }
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    while( 1 ) {
        /* Lock the first lock */
        pthread_rwlock_rdlock(&lock->locks[2* lock_idx]);
        if( pthread_rwlock_tryrdlock(&lock->locks[2* lock_idx + 1]) ){
            /* Server already signalled us that it wants access
             * wait until it releases the lock */
            pthread_rwlock_unlock(&lock->locks[2* lock_idx]);
            continue;
        }
        /* Unlock the second lock */
        pthread_rwlock_unlock(&lock->locks[2* lock_idx + 1]);
        break;
    }
}

void shared_rwlock_unlock(my_lock_t *lock)
{
    int i;
    if( init_by_me ){
        for(i=0; i<lock_num; i++) {
            pthread_rwlock_unlock(&lock->locks[2*i]);
            pthread_rwlock_unlock(&lock->locks[2*i + 1]);
        }
    } else {
        pthread_rwlock_unlock(&lock->locks[2 * lock_idx]);
    }
}

int shared_rwlock_fin(my_lock_t *lock)
{
    int i;
    if( init_by_me ){
        for(i = 0; i < 2 * lock_num; i++) {
            pthread_rwlock_destroy(&lock->locks[i]);
        }
    }
}   

