#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static char *fname = NULL;
static int _nranks = 0;
static int shemm_sync_fd = -1;
static int _sync_size, _rank;
static int _ag_size, _seg_fsize;

struct record{
    int seq_num;
    char reserve[64 - sizeof(int)];
} *shmem_ptr = NULL;

double *ag_ptr = NULL;

int sync_shmem_nranks()
{
    return _nranks;
}

int sync_shmem_rank()
{
    return _rank;
}

static void _shmem_init()
{
    char *ptr;
    ptr = getenv("SYNC_SHMEM_FILE");
    if( !ptr ){
        abort();
    }
    fname = strdup(ptr);

    ptr = getenv("SYNC_SHMEM_NRANKS");
    if( !ptr ){
        abort();
    }
    _nranks = atoi(ptr);
    if( _nranks > 128 || _nranks <= 0 ) {
        abort();
    }
    _sync_size = 64 * (_nranks + 1);
    _ag_size = sizeof(double) * _nranks;
    _seg_fsize = _sync_size + _ag_size;
}

void sync_shmem_srv_init()
{
    void *seg_addr = MAP_FAILED;

    _shmem_init();

    /* enough space is available, so create the segment */
    if (-1 == (shemm_sync_fd = open(fname, O_CREAT | O_RDWR, 0600))) {
        abort();
    }

    _sync_size = (_nranks + 1) * 64;
    /* size backing file - note the use of real_size here */
    if (0 != ftruncate(shemm_sync_fd, _seg_fsize)) {
        abort();
    }

    if (MAP_FAILED == (seg_addr = mmap(NULL, _seg_fsize,
                                       PROT_READ | PROT_WRITE, MAP_SHARED,
                                       shemm_sync_fd, 0))) {
        abort();
    }

    memset(seg_addr, 0, _seg_fsize);
    shmem_ptr = (struct record*)seg_addr;
}


void sync_shmem_cli_init()
{
    mode_t mode = O_RDWR;
    int mmap_prot = PROT_READ | PROT_WRITE;
    void *seg_addr = MAP_FAILED;

    _shmem_init();

    if (-1 == (shemm_sync_fd = open(fname, mode))) {
        abort();
    }
    if (MAP_FAILED == (seg_addr = mmap(NULL,_seg_fsize, mmap_prot,
                                       MAP_SHARED, shemm_sync_fd, 0))) {
        abort();
    }
    if (0 != close(shemm_sync_fd)) {
        abort();
    }

    shmem_ptr = (struct record*)seg_addr;
    ag_ptr = (double*)( (char*)seg_addr + _sync_size);

    char *ptr = getenv("PMIX_RANK");
    if( !ptr ){
        abort();
    }
    _rank = atoi(ptr);
}

void sync_shmem_barrier()
{
    int new_val = ++shmem_ptr[_rank + 1].seq_num;
    int i;
    if( _rank == 0 ){
        for(i=0; i< _nranks; i++) {
            while(shmem_ptr[i + 1].seq_num != new_val );
        }
        shmem_ptr[0].seq_num = new_val;
    } else {
        while(shmem_ptr[0].seq_num != new_val );
    }
}

void sync_shmem_allgather(double in, double **out)
{
    ag_ptr[_rank] = in;
    sync_shmem_barrier();
    *out = ag_ptr;
}
