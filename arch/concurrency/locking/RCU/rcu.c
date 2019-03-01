#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "rcu.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

__thread unsigned long *tls_cntr = NULL;

int rcu_init(rcu_lock_t *lock)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&lock->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
}

int rcu_free(rcu_lock_t *lock)
{
    /* We are guaranteed that no thread
     * will try to use this lock 
     */
    assert(NULL == lock->head);
    pthread_mutex_destroy(&lock->mutex);
}

void rcu_attach(rcu_lock_t *lock)
{
    rcu_list_t *elem = calloc(1, sizeof(*elem));
    tls_cntr = calloc(1, sizeof(*tls_cntr));

    /* Append ourself to the locking structure */
    pthread_mutex_lock(&lock->mutex);
    elem->counter = tls_cntr;
    elem->next = lock->head;
    lock->head = elem;
    pthread_mutex_unlock(&lock->mutex);
}

void rcu_detach(rcu_lock_t *lock)
{
    rcu_list_t *elem = NULL, *my_elem = NULL;

    /* Append ourself to the locking structure */
    pthread_mutex_lock(&lock->mutex);
    while((NULL != elem->next) && (elem->next->counter != tls_cntr)) {
        elem = elem->next;
    }
    if (NULL == elem->next) {
        /* Not found */
        goto exit;
    }
    my_elem = elem->next;
    elem->next = elem->next->next;
    free(my_elem->counter);
    free(my_elem);
exit:
    pthread_mutex_unlock(&lock->mutex);
}

void rcu_read_lock(rcu_lock_t *lock)
{
    *tls_cntr = lock->gcntr + 1;
    mb();
}

void rcu_read_unlock(rcu_lock_t *lock)
{
    mb();
    *tls_cntr = lock->gcntr;
}

void rcu_write_lock(rcu_lock_t *lock)
{
    pthread_mutex_lock(&lock->mutex);
}

void rcu_write_unlock(rcu_lock_t *lock)
{
    pthread_mutex_unlock(&lock->mutex);
}

rcu_handler_t rcu_update_initiate(rcu_lock_t *lock)
{
    rcu_handler_t hndl;
    mb();
    lock->gcntr += 2;
    hndl = lock->gcntr;
    return hndl;
}

int rcu_test_complete(rcu_lock_t *lock, rcu_handler_t hndl)
{
    rcu_list_t *elem;
    elem = lock->head;
    while (elem) {
        unsigned long cntr = *elem->counter;
        if ( !(0 == (cntr % 2)) && cntr < lock->gcntr ) {
            /* this thread might still hold a reference to the old data */
            return 0;
        }
        elem = elem->next;
    }
    /* It is safe to perform deferred cleanup */
    return 1;
}

void rcu_handle_free(rcu_handler_t hndl) {  }
