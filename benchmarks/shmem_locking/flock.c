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


#define LOCK_FNAME "lock_file"

int lockfd = -1;

static int open_lock_file(int create)
{
    mode_t mode = O_RDWR ;
    if( create ){
        mode |= O_CREAT | O_EXCL;
    }
    int lockfd = open(LOCK_FNAME, mode, 0600);

    /* if previous launch was crashed, the lockfile might not be deleted and unlocked,
     * so we delete it and create a new one. */
    if (lockfd < 0) {
        unlink(LOCK_FNAME);
        lockfd = open(LOCK_FNAME, O_CREAT | O_RDWR, 0600);
        if (lockfd < 0) {
            fprintf(stderr,"Fail to open lock file\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    return lockfd;
}

int shared_rwlock_create(my_lock_t *lock)
{
    lockfd = open_lock_file(1);
}

int shared_rwlock_init(my_lock_t *lock)
{
    lockfd = open_lock_file(0);
}

int _LOCK(lockfd, operation)
{
    int ret = 0;
    int i;      
    struct flock fl = {0};
    fl.l_type = operation;
    fl.l_whence = SEEK_SET;
    for(i = 0; i < 10; i++) {
        if( 0 > fcntl(lockfd, F_SETLKW, &fl) ) {
            switch( errno ){
                case EINTR:
                    continue;
                case ENOENT:
                case EINVAL:
                    ret = -1;
                    break;
                case EBADF:
                    ret = -1;
                    break;
                case EDEADLK:
                case EFAULT:
                case ENOLCK:
                    ret = -1;
                    break;
                default:
                    ret = -1;
                    break;
            }
        }
        break;
    }
    return ret;
}

#define _WRLOCK(lockfd) _LOCK(lockfd, F_WRLCK)
#define _RDLOCK(lockfd) _LOCK(lockfd, F_RDLCK)
#define _UNLOCK(lockfd) _LOCK(lockfd, F_UNLCK)

void shared_rwlock_wlock(my_lock_t *lock)
{
    int rc = _WRLOCK(lockfd);
    assert( 0 == rc);
}

void shared_rwlock_rlock(my_lock_t *lock)
{
    int rc = _RDLOCK(lockfd);
    assert( 0 == rc);
}

void shared_rwlock_unlock(my_lock_t *lock)
{
    int rc = _UNLOCK(lockfd);
    assert( 0 == rc);
}

int shared_rwlock_fin(my_lock_t *lock)
{
    close(lockfd);
}   

