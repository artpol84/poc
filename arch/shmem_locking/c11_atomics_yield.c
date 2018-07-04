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
#include <sched.h>

#include "lock.h"


static int init_by_me = 0;
static int i_am_writer = 0;

#define WRITER_FLAG 0x80000000

int shared_rwlock_create(my_lock_t *lock)
{
    lock->status = 0;
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

    do {
        /* check that we still have active readers */
        if( !(lock->status & (~WRITER_FLAG)) ){
            /* all readers are done now */
            return;
        }
        sched_yield();
    } while(1);
}

static inline void 
release_writer_lock(my_lock_t *lock)
{
    clear_writer_flag(&lock->status);
}


void shared_rwlock_wlock(my_lock_t *lock)
{
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
    do {
        /* check that we still have active readers */
        if( !(lock->status & WRITER_FLAG) ){
            /* no writers there */
            return;
        }
        sched_yield();
    } while(1);
}

static inline void
release_reader_lock(my_lock_t *lock)
{
    dec_reader_counter(&lock->status);
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    i_am_writer = 0;
    bool flag = false;
    while (!flag) {
        flag = inc_reader_counter(&lock->status);
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
}

