#ifndef RCU_H
#define RCU_H

#include <pthread.h>

#include "x86_64.h"

typedef struct rcu_list_s {
    unsigned long *counter;
    struct rcu_list_s *next;
} rcu_list_t;

typedef struct rcu_lock_s {
    /* lock that ensures writer exclusivity */
    pthread_mutex_t mutex;
    unsigned long gcntr;
    /* List of pointers to the TLS counters */
    rcu_list_t *head;
} rcu_lock_t;

typedef unsigned long rcu_handler_t;

int rcu_init(rcu_lock_t *lock);
int rcu_free(rcu_lock_t *lock);
void rcu_read_lock(rcu_lock_t *lock);
void rcu_read_unlock(rcu_lock_t *lock);
void rcu_write_lock(rcu_lock_t *lock);
void rcu_write_unlock(rcu_lock_t *lock);
rcu_handler_t rcu_update_initiate(rcu_lock_t *lock);
int rcu_test_complete(rcu_lock_t *lock, rcu_handler_t hndl);
void rcu_handle_free(rcu_handler_t hndl);

#endif
