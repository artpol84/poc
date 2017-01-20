#ifndef MY_LOCK_H
#define MY_LOCK_H

#if ((MY_FLOCK + MY_PTHREAD + MY_DUMMY) > 1 )
#error "Use one and only one locking type at the time"
#endif

#if ((MY_FLOCK + MY_PTHREAD + MY_DUMMY) == 0 )
#error "Use one and only one locking type at the time"
#endif
    
#if (MY_FLOCK == 1)
#include "flock.h"
#endif

#if (MY_PTHREAD == 1)
#include "pthread.h"
#endif

#if (MY_DUMMY == 1)
#include "dummy.h"
#endif

int shared_rwlock_create(my_lock_t *lock);
int shared_rwlock_init(my_lock_t *lock);
void shared_rwlock_wlock(my_lock_t *lock);
void shared_rwlock_rlock(my_lock_t *lock);
void shared_rwlock_unlock(my_lock_t *lock);
int shared_rwlock_fin(my_lock_t *lock);


#endif