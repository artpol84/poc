#ifndef MSC_LOCK_H
#define MSC_LOCK_H

#include "x86.h"

#define compiler_fence() { asm volatile ("" : : : "memory"); }

typedef struct msc_list_s {
    volatile uint32_t locked;
    volatile struct msc_list_s *next;
} msc_list_t;

typedef struct msc_lock_s {
    msc_list_t *head;
} msc_lock_t;

static void msc_init(msc_lock_t *lock)
{
    lock->head = NULL;
}

static void msc_destroy(msc_lock_t *lock)
{
    /* nothing to do */
}

static __thread msc_list_t my_record;

static inline void msc_lock(msc_lock_t *lock, int id)
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
    while(my_record.locked);
    // lock is taken
}

static inline void msc_unlock(msc_lock_t *lock, int id)
{
    if( CAS((int64_t*)&lock->head, (int64_t)&my_record, (int64_t)NULL) ) {
        // No one elase approached the lock after this thread
        return;
    }

    // Wait for the next record initialization
    while( NULL == my_record.next);

    // Release the next one in a queue
    compiler_fence();
    my_record.next->locked =0;
    // lock is released
    my_record.next = NULL;
    my_record.locked = 0;
}


#endif

