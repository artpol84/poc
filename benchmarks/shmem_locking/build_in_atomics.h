#ifndef MY_C11_ATOMICS_H
#define MY_C11_ATOMICS_H

#include <pthread.h>

typedef struct
{
    volatile int status;
} my_lock_t;

#endif