#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "lock.h"

int seg_fd;
int rank;

struct my {
    my_lock_t lock;
    int counter1, counter2;
};

struct my *data = NULL;

void *create_seg(char *fname, int size)
{
    void * seg_addr = NULL;

    if (-1 == (seg_fd = open(fname, O_CREAT | O_RDWR, 0600))) {
        fprintf(stderr, "%d: can't open %s\n", rank, fname);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* size backing file - note the use of real_size here */
    if (0 != ftruncate(seg_fd, size)) {
        fprintf(stderr, "%d: can't ftruncate %s\n", rank, fname);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (MAP_FAILED == (seg_addr = mmap(NULL, size,
                                       PROT_READ | PROT_WRITE, MAP_SHARED,
                                       seg_fd, 0))) {
        fprintf(stderr, "%d: can't mmap %s\n", rank, fname);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    return seg_addr;
}

void *attach_seg(char *fname, int size)
{
    void *seg_addr;
    mode_t mode = O_RDWR;
    int mmap_prot = PROT_READ | PROT_WRITE;
//    mode_t mode = O_RDONLY;
//    int mmap_prot = PROT_READ;

    if (-1 == (seg_fd = open(fname, mode))) {
        fprintf(stderr, "%d: can't open %s\n", rank, fname);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    seg_addr = mmap(NULL, size, mmap_prot, MAP_SHARED, seg_fd, 0);
    if (MAP_FAILED == seg_addr ){
        fprintf(stderr, "%d: can't mmap %s\n", rank, fname);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (0 != close(seg_fd)) {
        fprintf(stderr, "%d: can't mmap %s\n", rank, fname);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    return seg_addr;

}

#define segname "seg"

int main(int argc, char **argv)
{
    char *membase = NULL;
    int i;
    double perf1;
    double perf2;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    {
        int delay = 0;
        while( delay ){
            sleep(1);
        }
    }

    if( 0 == rank ) {
        membase = create_seg(segname, 4096);
        data = (struct my*)membase;
        data->counter1 = 0;
        data->counter2 = 0;
        shared_rwlock_create(&data->lock);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if( 0 != rank ){
        membase = attach_seg(segname, 4096);
        data = (struct my*)membase;
        shared_rwlock_init(&data->lock);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Correctness verification
    if( 0 == rank ){
        printf("Data correctness verification\n");
        for(i=0; i<100; i++){
            if( (i % 10) == 9 ){
                 printf("Step #%d%%\n", (i+1)/10);
                 fflush(stdout);
            }
            shared_rwlock_wlock(&data->lock);
            data->counter1++;
            /* To verify that locking works - release readers
             * now and sleep to let them a chance to hit
             * inconsistent data
             */
            MPI_Barrier(MPI_COMM_WORLD);
            usleep(100);
            data->counter2++;
            shared_rwlock_unlock(&data->lock);
            MPI_Barrier(MPI_COMM_WORLD);
        }

    } else {
        for(i=0; i<100; i++){
            MPI_Barrier(MPI_COMM_WORLD);
            shared_rwlock_rlock(&data->lock);
            if( data->counter1 != data->counter2 ){
                fprintf(stderr, "%d: data corruption, i=%d\n", rank, i);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            MPI_Barrier(MPI_COMM_WORLD);
            shared_rwlock_unlock(&data->lock);
        }
    }    


    MPI_Barrier(MPI_COMM_WORLD);
    if( 0 == rank ){
        data->counter1 = 0;
        data->counter2 = 0;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    
    struct timeval tv;
    double time = 0, start;
    if (0 == rank) {
        printf("Performance evaluation: writer only\n");
        for(i=0; i<10000; i++){
            if( (i % 1000) == 999 ){
                 printf("Step #%d%%\n", (i+1)/100);
                 fflush(stdout);
            }
            gettimeofday(&tv, NULL);
            start = tv.tv_sec + 1E-6*tv.tv_usec;
            shared_rwlock_wlock(&data->lock);
            data->counter1++;
            /* To verify that locking works - release readers
             * now and sleep to let them a chance to hit
             * inconsistent data
             */
            data->counter2++;
            shared_rwlock_unlock(&data->lock);
            gettimeofday(&tv, NULL);
            time += (tv.tv_sec + 1E-6*tv.tv_usec) - start;
        }
        perf1 = time;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if( 0 == rank ){
        data->counter1 = 0;
        data->counter2 = 0;
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (0 == rank) {
        printf("Performance evaluation: writer/reader\n");
        for(i=0; i<10000; i++){
            if( (i % 1000) == 999 ){
                 printf("Step #%d%%\n", (i+1)/100);
                 fflush(stdout);
            }
            gettimeofday(&tv, NULL);
            start = tv.tv_sec + 1E-6*tv.tv_usec;
            shared_rwlock_wlock(&data->lock);
            data->counter1++;
            /* To verify that locking works - release readers
             * now and sleep to let them a chance to hit
             * inconsistent data
             */
            data->counter2++;
            shared_rwlock_unlock(&data->lock);
            gettimeofday(&tv, NULL);
            time += (tv.tv_sec + 1E-6*tv.tv_usec) - start;
            usleep(1);
        }
        perf2 = time;
    } else {

        int cur_count = 0;
        while( cur_count < 8000 ){
            gettimeofday(&tv, NULL);
            start = tv.tv_sec + 1E-6*tv.tv_usec;
            shared_rwlock_rlock(&data->lock);
            cur_count = data->counter1;
            shared_rwlock_unlock(&data->lock);
            gettimeofday(&tv, NULL);
            time += (tv.tv_sec + 1E-6*tv.tv_usec) - start;
            usleep(1);
        }
        perf2 = time;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    shared_rwlock_fin(&data->lock);

    MPI_Barrier(MPI_COMM_WORLD);
    if( rank == 0){
        printf("%d: Writer-only: Time to do 10000 lock/unlocks = %lf\n",
                rank, perf1);
        printf("%d: Writer/reader: Time to do 10000 lock/unlocks = %lf\n",
                rank, perf2);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if( rank == 1){
        printf("%d: Writer/reader: Time to do 10000 lock/unlocks = %lf\n",
                rank, perf2);
    }
    
    
    MPI_Finalize();
    return 0;
}