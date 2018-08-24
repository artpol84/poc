#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static char *fname = NULL;
static int nranks = 0;
static int shemm_sync_fd = -1;
int size, rank;

struct record{
    int seq_num;
    char reserve[64 - sizeof(int)];
} *shmem_ptr = NULL;

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
    nranks = atoi(ptr);
    if( nranks > 128 || nranks <= 0 ) {
        abort();
    }
    size = 64 * (nranks + 1);
}

void sync_shmem_srv_init()
{
    void *seg_addr = MAP_FAILED;

    _shmem_init();

    /* enough space is available, so create the segment */
    if (-1 == (shemm_sync_fd = open(fname, O_CREAT | O_RDWR, 0600))) {
        abort();
    }

    size = (nranks + 1) * 64;
    /* size backing file - note the use of real_size here */
    if (0 != ftruncate(shemm_sync_fd, size)) {
        abort();
    }

    if (MAP_FAILED == (seg_addr = mmap(NULL, size,
                                       PROT_READ | PROT_WRITE, MAP_SHARED,
                                       shemm_sync_fd, 0))) {
        abort();
    }

    memset(seg_addr, 0, size);
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
    if (MAP_FAILED == (seg_addr = mmap(NULL,size, mmap_prot,
                                       MAP_SHARED, shemm_sync_fd, 0))) {
        abort();
    }
    if (0 != close(shemm_sync_fd)) {
        abort();
    }

    shmem_ptr = (struct record*)seg_addr;

    char *ptr = getenv("PMIX_RANK");
    if( !ptr ){
        abort();
    }
    rank = atoi(ptr);
}

void sync_shmem_do()
{
    int new_val = ++shmem_ptr[rank + 1].seq_num;
    int i;
    if( rank == 0 ){
        for(i=0; i< nranks; i++) {
            while(shmem_ptr[i + 1].seq_num != new_val );
        }
        shmem_ptr[0].seq_num = new_val;
    } else {
        while(shmem_ptr[0].seq_num != new_val );
    }
}
