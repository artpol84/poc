#ifndef MY_MSC_H
#define MY_MSC_H
//#include <pthread.h>

typedef struct msc_list_s {
    volatile uint32_t locked;
    volatile struct msc_list_s *next;
} msc_list_t;

typedef struct msc_lock_s {
    msc_list_t *head;
    msc_list_t cli_record;
    msc_list_t srv_record;
} msc_lock_t;

/*
typedef struct  {
    pthread_mutex_t lock;
    volatile int signal;
} nlock_elem_int_t;
*/
typedef struct  {
    msc_lock_t e;
    char padding[64 - sizeof(msc_lock_t)];
} align_lock_t;


typedef struct {
    align_lock_t locks[512];
    /* cache line alignment */
    /* msc_list_t srv_records[512]; */
} nlock_t;

#define my_lock_t nlock_t

#endif
