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
int rank, size;

struct my {
    my_lock_t lock;
    int counter1, counter2;
};

struct my *data = NULL;

inline void nsleep(size_t nsec)
{
    struct timespec ts, ts1;
    ts.tv_sec = nsec / 1000000000;
    ts.tv_nsec = nsec % 1000000000;
    nanosleep(&ts, &ts1);
}

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
#define PERCENT(x, p) (x/p)
#define WRONLY_REPS 10000
#define RD_REPS 10000
#define WR_REPS 100

int main(int argc, char **argv)
{
    char *membase = NULL;
    int i;
    double perf1;
    double perf2;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    struct timeval tv;
    double time = 0, start, init_time = 0;

    {
        int delay = 0;
        while( delay ){
            sleep(1);
        }
    }

    gettimeofday(&tv, NULL);
    start = tv.tv_sec + 1E-6*tv.tv_usec;
    if( 0 == rank ) {
        membase = create_seg(segname, 4096);
        data = (struct my*)membase;
        data->counter1 = 0;
        data->counter2 = 0;
        shared_rwlock_create(&data->lock);
    }
    gettimeofday(&tv, NULL);
    init_time += (tv.tv_sec + 1E-6*tv.tv_usec) - start;

    MPI_Barrier(MPI_COMM_WORLD);

    gettimeofday(&tv, NULL);
    start = tv.tv_sec + 1E-6*tv.tv_usec;
    if( 0 != rank ){
        membase = attach_seg(segname, 4096);
        data = (struct my*)membase;
        shared_rwlock_init(&data->lock);
    }
    gettimeofday(&tv, NULL);
    init_time += (tv.tv_sec + 1E-6*tv.tv_usec) - start;

    MPI_Barrier(MPI_COMM_WORLD);

    // Correctness verification
    if( 0 == rank ){
        printf("Data correctness verification\n");
        for(i=0; i<100; i++){
            if( (i % 10) == 9 ){
                 printf("Step #%d%%\n", (i+1));
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
    
    
    if (0 == rank) {
        printf("Performance evaluation: writer only\n");
        for(i=0; i<WRONLY_REPS; i++){
            if( ((i + 1) % PERCENT(WRONLY_REPS,10)) == 0 ){
                 printf("Step #%d%%\n", (i+1)/PERCENT(WRONLY_REPS,10) );
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

    int cur_count = 0;
    int prev_barrier = 0;
    unsigned long rlock_cnt = 0, rlock_cnt_cum;
    
    if (0 == rank) {
        printf("Performance evaluation: writer/reader\n");
        for(i=0; i<WR_REPS; i++){
            if( ((i + 1) % PERCENT(WR_REPS,10)) == 0 ){
                 printf("Step #%d%%\n", (i+1)/PERCENT(WR_REPS, 10));
                 fflush(stdout);
            }
            gettimeofday(&tv, NULL);
            start = tv.tv_sec + 1E-6*tv.tv_usec;
            shared_rwlock_wlock(&data->lock);
            data->counter1++;
            data->counter2++;
            cur_count++;
            shared_rwlock_unlock(&data->lock);
            gettimeofday(&tv, NULL);
            time += (tv.tv_sec + 1E-6*tv.tv_usec) - start;
            usleep(size);
        }
        perf2 = time;
    } else {
        int cur_count = 0;
        int prev_barrier = 0;
        int i = 0;
        while( cur_count < WR_REPS ){
            /* almost busy-looping :) */
            nsleep(100);

            gettimeofday(&tv, NULL);
            start = tv.tv_sec + 1E-6*tv.tv_usec;
            shared_rwlock_rlock(&data->lock);
            cur_count = data->counter1;
            usleep(1);
            shared_rwlock_unlock(&data->lock);
            gettimeofday(&tv, NULL);
            time += (tv.tv_sec + 1E-6*tv.tv_usec) - start;
            rlock_cnt++;
        }
        perf2 = time;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    shared_rwlock_fin(&data->lock);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Reduce(&rlock_cnt, &rlock_cnt_cum, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if( rank == 0){
        printf("%d: init time: %lf\n", rank, init_time);
        printf("%d: Writer-only: %d lock/unlocks in %lf sec\n",
                rank, WRONLY_REPS, perf1);
        printf( "%d: Writer/reader:"
                "\n\twriter: %d locks/unlocks in %lf sec"
                "\n\treader: avg %lf locks/unlocs\n",
                rank,
                WR_REPS, perf2,
                (double)rlock_cnt_cum / (size-1));
    }
    MPI_Finalize();
    return 0;
}