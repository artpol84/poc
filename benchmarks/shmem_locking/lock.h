#ifndef MY_LOCK_H
#define MY_LOCK_H

#ifdef MY_FLOCK
#ifdef MY_PTHREAD
#error "Can't define both MY_FLOCK and MY_PTHREAD"
#endif
#include "flock.h"
#endif

#ifdef MY_PTHREAD
#include "pthread.h"
#endif

int shared_rwlock_create(my_lock_t *lock);
int shared_rwlock_init(my_lock_t *lock);
void shared_rwlock_wlock(my_lock_t *lock);
void shared_rwlock_rlock(my_lock_t *lock);
void shared_rwlock_unlock(my_lock_t *lock);
int shared_rwlock_fin(my_lock_t *lock);


#endif