#include <mpi.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>

#include <pthread.h>

#include "lock.h"


#ifdef MY_GCC_BUILDIN_YIELD
#include <sched.h>
#endif

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
set_writer_flag(volatile int *status)
{
    int flag = 0;
    int val = 0, ret;

    while( !flag ){
        val = *status;
        ret = __sync_val_compare_and_swap(status, val, val | WRITER_FLAG);
        if( val == ret ){
            flag = 1;
        }
    }
}

static inline void
clear_writer_flag(volatile int *status)
{
    int flag = 0;
    int val = 0, ret;

    while( !flag ){
        val = *status;
        ret = __sync_val_compare_and_swap(status, val, val & (~WRITER_FLAG) );
        if( val == ret ){
            flag = 1;
        }
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
#ifdef MY_GCC_BUILDIN_YIELD
        sched_yield();
#endif

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

static inline int
inc_reader_counter(volatile int *status)
{
    int flag = 0;
    int val, ret = 0;

    while( !flag ){
        val = *status;
        if( val & WRITER_FLAG ){
            return 0;
        }
        ret = __sync_val_compare_and_swap (status, val, val + 1);
        if( ret == val ){
            flag = 1;
        }
    }
    return 1;
}

static inline int
dec_reader_counter(volatile int *status)
{
    int flag = 0;
    int val, ret = 0;

    while( !flag ){
        val = *status;
        ret = __sync_val_compare_and_swap (status, val, val - 1);
        if( ret == val ){
            flag = 1;
        }
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
#ifdef MY_GCC_BUILDIN_YIELD
        sched_yield();
#endif
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
    int flag = 0;
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

