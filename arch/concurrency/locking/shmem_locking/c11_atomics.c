#include <mpi.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

#include "lock.h"


static int init_by_me = 0;
static int i_am_writer = 0;

#define WRITER_FLAG 0x80000000

int shared_rwlock_create(my_lock_t *lock)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&lock->wlock,  &mattr);
    pthread_mutex_init(&lock->wclock, &mattr);
    pthread_mutex_init(&lock->rclock, &mattr);
    pthread_mutexattr_destroy(&mattr);

    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&lock->wcond,  &cattr);
    pthread_cond_init(&lock->rcond,  &cattr);
    pthread_condattr_destroy(&cattr);

    lock->status = 0;

    init_by_me = 1;
}

int shared_rwlock_init(my_lock_t *lock)
{
}

static inline void
set_writer_flag(atomic_int *status)
{
    bool flag = false;
    int ret = 0;

    while( !flag ){
        ret = *status;
        flag = atomic_compare_exchange_weak(status, &ret, ret | WRITER_FLAG);
    }
}

static inline void
clear_writer_flag(atomic_int *status)
{
    bool flag = false;
    int ret = 0;

    while( !flag ){
        ret = *status;
        flag = atomic_compare_exchange_weak(status, &ret, ret & (~WRITER_FLAG) );
    }
}

static int writer_waits = 0;
static inline void
wait_for_readers(my_lock_t *lock)
{
    /* fastpath - no readers already */
    if( !(lock->status & (~WRITER_FLAG)) ){
        return;
    }

    pthread_mutex_lock(&lock->wclock);
    do {
        /* check that we still have active readers */
        if( !(lock->status & (~WRITER_FLAG)) ){
            /* all readers are done now */
            pthread_mutex_unlock(&lock->wclock);
            return;
        }
        writer_waits++;
        /* wait until all readers will finish */
        pthread_cond_wait(&lock->wcond, &lock->wclock);
    } while(1);
}

static inline void 
release_writer_lock(my_lock_t *lock)
{
    pthread_mutex_lock(&lock->rclock);
    clear_writer_flag(&lock->status);
    pthread_cond_broadcast(&lock->rcond);
    pthread_mutex_unlock(&lock->rclock);

    /* release lock for writers */
    pthread_mutex_unlock(&lock->wlock);
}


void shared_rwlock_wlock(my_lock_t *lock)
{
    pthread_mutex_lock(&lock->wlock);
    set_writer_flag(&lock->status);
    wait_for_readers(lock);
    i_am_writer = 1;
}

static inline bool
inc_reader_counter(atomic_int *status)
{
    bool flag = false;
    int ret = 0;

    while( !flag ){
        ret = *status;
        if( ret & WRITER_FLAG ){
            return false;
        }
        flag = atomic_compare_exchange_weak(status, &ret, ret + 1);
    }
    return true;
}

static inline int
dec_reader_counter(atomic_int *status)
{
    bool flag = false;
    int ret = 0;

    while( !flag ){
        ret = *status;
        flag = atomic_compare_exchange_weak(status, &ret, ret - 1);
    }
    return ret - 1;
}

static inline void
wait_for_writer(my_lock_t *lock)
{
    pthread_mutex_lock(&lock->rclock);
    do {
        /* check that we still have active readers */
        if( !(lock->status & WRITER_FLAG) ){
            /* no writers there */
            pthread_mutex_unlock(&lock->rclock);
            return;
        }
        /* wait until all readers will finish */
        pthread_cond_wait(&lock->rcond, &lock->rclock);
    } while(1);
}

static inline void
release_reader_lock(my_lock_t *lock)
{
    int ret = dec_reader_counter(&lock->status);

    if( (ret & WRITER_FLAG) && !(ret & (~WRITER_FLAG)) ){
        /* there is a writer waiting and I am the last 
         * reader - wake up writer */
        pthread_mutex_lock(&lock->wclock);
        pthread_cond_broadcast(&lock->wcond);
        pthread_mutex_unlock(&lock->wclock);
    }
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    i_am_writer = 0;
    while (!inc_reader_counter(&lock->status) ){
        wait_for_writer(lock);
    }
}

void shared_rwlock_unlock(my_lock_t *lock)
{
    if( i_am_writer ){
        release_writer_lock(lock);
    } else {
        release_reader_lock(lock);
    }
}

int shared_rwlock_fin(my_lock_t *lock)
{
    if( init_by_me ){
        pthread_mutex_destroy(&lock->wlock);
        pthread_mutex_destroy(&lock->wclock);
        pthread_mutex_destroy(&lock->rclock);
        pthread_cond_destroy(&lock->rcond);
        pthread_cond_destroy(&lock->wcond);
    }
}

