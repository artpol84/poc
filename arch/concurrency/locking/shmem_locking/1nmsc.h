#ifndef MY_MSC_H
#define MY_MSC_H
#include <stdint.h>
<<<<<<< HEAD
=======
//#include <pthread.h>
>>>>>>> 2c694d3eb2c830b7af0957800af6825c4b184704

typedef struct msc_list_s {
    volatile uint32_t locked;
    volatile uint32_t next;
} msc_list_t;

typedef struct msc_lock_s {
    volatile uint64_t head;
    msc_list_t record[2]; // [0] - for client to read, [1] - for server to write
} msc_lock_t;

/* cache line alignment */
typedef struct  {
    msc_lock_t e;
    char padding[64 - sizeof(msc_lock_t)];
} align_lock_t;


typedef struct {
    align_lock_t locks[512];
} nlock_t;

#define my_lock_t nlock_t

#endif
