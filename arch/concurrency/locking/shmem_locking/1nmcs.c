#include "1nmcs.h"
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
#include <unistd.h>
#include <string.h>

#if defined(__x86_64__)
#include "atomic_x86.h"
#define compiler_fence() { __asm__ __volatile__ ("sfence" : : : "memory"); }
#define proc_sleep() { __asm__ __volatile__ ("pause" : : : "memory"); }
#elif defined(__PPC64__)
#include "atomic_ppc.h"
#define compiler_fence() { __asm__ __volatile__ ("lwsync": : :"memory");}
#define proc_sleep() { __asm__ __volatile__ ("yield" : : : "memory"); }
#else
    printf("Unknown processor architecture!\n");
#endif

static int init_by_me = 0;
static int lock_num = 0; 
static int lock_idx = 0; // one lock for every reader
static uint32_t record_idx = 0; // each lock include two records for client (reader) and server (writer)

int shared_rwlock_create(my_lock_t *lock) /* rank == 0 */
{
    int i;
    MPI_Comm_size(MPI_COMM_WORLD, &lock_num);
    lock_num--;

    if( sizeof(lock->locks) / sizeof(lock->locks[0]) * 2 < lock_num ) {
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    memset(lock->locks, 0, sizeof(lock->locks));
    
    init_by_me = 1; /* rank 0 will be write */
    record_idx = 2; // 1 - client for read, 0 - for nobody as next
}

int shared_rwlock_init(my_lock_t *lock) /* rank != 0 */
{
    MPI_Comm_rank(MPI_COMM_WORLD, &lock_idx);
    lock_idx--;
    record_idx = 1; // 2 - server for write, 0 - for nobody as next
}

static inline void _mcs_lock(mcs_lock_t *mcs_lock)
{
    uint32_t prev_idx;
    (&mcs_lock->record[record_idx - 1])->next = 0;
    
    prev_idx = (uint32_t)atomic_swap((int64_t*)&mcs_lock->head, (int64_t)record_idx);
    if( 0 == prev_idx ) {
        return;
    }

    (&mcs_lock->record[record_idx-1])->locked = 1;

    compiler_fence();
    (&mcs_lock->record[prev_idx-1])->next = record_idx;
    compiler_fence();

    /* TODO: Find out
     * For some reasons "volatile" specifier of the structure field
     * was unable to prevent compiler from optimizing this part of the
     * code resulting in a deadlock
     */
    volatile uint32_t *flag = &((&mcs_lock->record[record_idx - 1])->locked);
    while(*flag){
        proc_sleep();
    }
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    _mcs_lock(&lock->locks[lock_idx].e);
}

void shared_rwlock_wlock(my_lock_t *lock)
{
    int i;
    for(i = 0; i < lock_num; i++) {
        _mcs_lock(&lock->locks[i].e);
    }
}

static inline void _mcs_unlock(mcs_lock_t *mcs_lock)
{
    if( CAS((int64_t*)(&mcs_lock->head), (int64_t)record_idx, (int64_t)NULL) ) {
        // No one elase approached the lock after this thread
        return;
    }

    // Wait for the next record initialization
    volatile uint32_t *flag = &((&mcs_lock->record[record_idx-1])->next);
    while(!(*flag)){
        proc_sleep();
    }

    // Release the next one in a queue
    uint32_t next_idx = (&mcs_lock->record[record_idx-1])->next;
    compiler_fence();
    (&mcs_lock->record[next_idx-1])->locked = 0;
    compiler_fence();

    // lock is released
    (&mcs_lock->record[record_idx-1])->next = 0;
    (&mcs_lock->record[record_idx-1])->locked = 0; //?
}

void shared_rwlock_unlock(my_lock_t *lock)
{
    int i;
    if( init_by_me ){
        for(i = 0; i < lock_num; i++) {
            _mcs_unlock(&lock->locks[i].e);
        }
    } else {
        _mcs_unlock(&lock->locks[lock_idx].e);
    }
}

int shared_rwlock_fin(my_lock_t *lock)
{
    /* nothing to do */
}   
