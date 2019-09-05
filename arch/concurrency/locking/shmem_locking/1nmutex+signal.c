#include "1nmutex+signal.h"
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
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    for(i = 0; i < lock_num; i++) {
        pthread_mutex_init(&lock->locks[i].e.lock, &attr);
    }
    lock->signal = 0;
    pthread_mutexattr_destroy(&attr);
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
    
    lock->signal = 1;
    /* Lock the second lock first */
    for(i=0; i<lock_num; i++) {
        pthread_mutex_lock(&lock->locks[i].e.lock);
    }
}

inline void shared_rwlock_rlock_1(my_lock_t *lock)
{
    pthread_mutex_t *mutex = &lock->locks[lock_idx].e.lock;

    if( lock->signal ) {
        while(1) {
            if( !pthread_mutex_trylock(mutex) ){
                if(lock->signal) {
                    pthread_mutex_unlock(mutex);
                    continue;
                } else {
                    goto exit;
                }
            }else {
                goto lock;
            }
        }
    }

lock:
    pthread_mutex_lock(&lock->locks[lock_idx].e.lock);
exit:
    return;
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    shared_rwlock_rlock_1(lock);
}
void shared_rwlock_unlock(my_lock_t *lock)
{
    int i;
    if( init_by_me ){
        lock->signal = 0;
        for(i=0; i<lock_num; i++) {
            pthread_mutex_unlock(&lock->locks[i].e.lock);
        }
    } else {
        pthread_mutex_unlock(&lock->locks[lock_idx].e.lock);
    }
}

int shared_rwlock_fin(my_lock_t *lock)
{
    int i;
    if( init_by_me ){
        for(i = 0; i < lock_num; i++) {
            pthread_mutex_destroy(&lock->locks[i].e.lock);
        }
    }
}   

