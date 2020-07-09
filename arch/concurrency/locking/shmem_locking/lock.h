#ifndef MY_LOCK_H
#define MY_LOCK_H

#if ((MY_FLOCK + MY_PTHREAD + MY_PTHREAD_MUTEX + MY_DUMMY + MY_C11_ATOMICS + MY_GCC_BUILDIN + MY_PTHREAD_N + MY_PTHREAD_N2 + MY_PTHREAD_N1 + MY_PTHREAD_N1S + MY_PTHREAD_NS1 + MY_MSC) != 1 )
#error "Use one and only one locking type at the time"
#endif

#if (MY_FLOCK == 1)
#include "flock.h"
#endif

#if (MY_PTHREAD == 1)
#include "pthread.h"
#endif

#if (MY_PTHREAD_N == 1)
#include "pthread_nlocks.h"
#endif

#if (MY_PTHREAD_N2 == 1)
#include "2nmutex.h"
#endif

#if (MY_PTHREAD_N1 == 1)
#include "1nmutex.h"
#endif

#if (MY_PTHREAD_N1S == 1)
#include "1nmutex+signal.h"
#endif

#if (MY_PTHREAD_NS1 == 1)
#include "1nmutex+1nsignal.h"
#endif

#if (MY_PTHREAD_MUTEX == 1)
#include "pthread_mutex.h"
#endif

#if (MY_DUMMY == 1)
#include "dummy.h"
#endif

#if (MY_C11_ATOMICS == 1)
#include "c11_atomics.h"
#endif

#if (MY_MSC == 1)
#include "1nmsc.h"
#endif

int shared_rwlock_create(my_lock_t *lock);
int shared_rwlock_init(my_lock_t *lock);
void shared_rwlock_wlock(my_lock_t *lock);
void shared_rwlock_rlock(my_lock_t *lock);
void shared_rwlock_unlock(my_lock_t *lock);
int shared_rwlock_fin(my_lock_t *lock);

#endif
