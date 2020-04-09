#include "1nmsc.h"
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
#include "x86.h"

#define compiler_fence() { asm volatile ("" : : : "memory"); }

//static int init_by_me = 0;
static int lock_num = 0;
static int lock_idx = 0;

int shared_rwlock_create(my_lock_t *lock) /* rank == 0 */
{
    int i;
    MPI_Comm_size(MPI_COMM_WORLD, &lock_num);
    lock_num--;
    if( sizeof(lock->locks) / sizeof(lock->locks[0]) * 2 < lock_num ) {
        printf("Too many processes, cannot allocate enough locks\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }
    for(i = 0; i < lock_num; i++) {
        lock->head = NULL;
    }

    init_by_me = 1; /* ? rank 0 will be write ? */
    static __thread msc_list_t my_record; /* ? when threads should create my_record ? */
}

int shared_rwlock_init(my_lock_t *lock) /* rank != 0 */
{
    MPI_Comm_rank(MPI_COMM_WORLD, &lock_idx);
    lock_idx--;
    static __thread msc_list_t my_record;
}

inline void shared_rwlock_lock(msc_lock_t *lock)
{
    msc_list_t *prev;
    my_record.next = NULL;
    prev = (msc_list_t *)atomic_swap((int64_t*)&lock->head, (int64_t)&my_record);

    if( NULL == prev ) {
        // lock is taken, no one else is waiting
        return;
    }
    my_record.locked = 1;
    compiler_fence();
    prev->next = &my_record;
    compiler_fence();
    /* TODO: Find out
     * For some reasons "volatile" specifier of the structure field
     * was unable to prevent compiler from optimizing this part of the
     * code resulting in a deadlock
     */
    volatile uint32_t *flag = &my_record.locked;
    while(*flag) {
        asm volatile (
            "label%=:\n"
            "   pause\n"
            "   cmp $0, (%[flag])\n"
            "   jne label%=\n"
            :
            : [flag] "r" (flag)
            : "memory");
    }
    // lock is taken
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    shared_rwlock_lock(lock->locks[lock_idx]);
}

void shared_rwlock_wlock(my_lock_t *lock)
{
    int i;

    for(i=0; i<lock_num; i++) {
        shared_rwlock_lock(lock->locks[i]);
    }
}

void shared_rwlock_ulock(msc_lock_t *lock)
{
    if( CAS((int64_t*)&lock->head, (int64_t)&my_record, (int64_t)NULL) ) {
        // No one elase approached the lock after this thread
        return;
    }

    // Wait for the next record initialization
    volatile uint64_t *flag = (uint64_t*)&my_record.next;
    while(!(*flag)){
        asm volatile ("pause" : : : "memory");
    }

    // Release the next one in a queue
    compiler_fence();
    my_record.next->locked =0;
    //asm volatile ("sfence" : : : "memory");
    // lock is released
    my_record.next = NULL;
    my_record.locked = 0;
}

void shared_rwlock_unlock(my_lock_t *lock)
{
    int i;
    if( init_by_me ){
        for(i=0; i<lock_num; i++) {
            shared_rwlock_ulock(lock->locks[i]);
        }
    } else {
        shared_rwlock_ulock(lock->locks[lock_idx]);
    }
}

int shared_rwlock_fin(my_lock_t *lock)
{
    /* nothing to do */
/*    int i;
    if( init_by_me ){
        for(i = 0; i < lock_num; i++) {
            pthread_mutex_destroy(&lock->locks[i].e.lock);
        }
    }
*/
}   

