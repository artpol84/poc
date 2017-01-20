#include "lock.h"
#include <mpi.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>


int shared_rwlock_create(my_lock_t *lock)
{
}

int shared_rwlock_init(my_lock_t *lock)
{
}

void shared_rwlock_wlock(my_lock_t *lock)
{
}

void shared_rwlock_rlock(my_lock_t *lock)
{
}

void shared_rwlock_unlock(my_lock_t *lock)
{
}

int shared_rwlock_fin(my_lock_t *lock)
{
}   

