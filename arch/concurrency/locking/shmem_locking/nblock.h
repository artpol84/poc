#ifndef MY_NBLOCK_H
#define MY_NBLOCK_H
#include <stdint.h>

typedef struct mcs_list_s {
    volatile uint32_t locked;
    volatile uint32_t next;
} mcs_list_t;

typedef struct mcs_lock_s {
    volatile uint64_t head;
    mcs_list_t record[2]; // [0] - for client to read, [1] - for server to write
} mcs_lock_t;

/* cache line alignment */
typedef struct  {
    mcs_lock_t e;
    char padding[64 - sizeof(mcs_lock_t)];
} align_lock_t;


typedef struct {
    align_lock_t locks[512];
} nlock_t;

#define my_lock_t nlock_t

#endif
