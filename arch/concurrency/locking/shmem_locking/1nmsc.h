#ifndef MY_MSC_H
#define MY_MSC_H
//#include <pthread.h>

typedef struct msc_list_s {
    volatile uint32_t locked;
    volatile struct msc_list_s *next;
} msc_list_t;

typedef struct msc_lock_s {
    msc_list_t *head;
} msc_lock_t;

/*
typedef struct  {
    pthread_mutex_t lock;
    volatile int signal;
} nlock_elem_int_t;

typedef struct  {
    nlock_elem_int_t e;
    char padding[64 - sizeof(nlock_elem_int_t)];
} nlock_int_t;
*/

typedef struct {
    msc_lock_t locks[512];
} nlock_t;

#define my_lock_t nlock_t

#endif
