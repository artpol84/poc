#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#include "lock.h"

#define VERBOSE_OUT(verbose, format, args...) { \
    if( verbose ){                              \
        printf(format, ##args);                 \
        fflush(stdout);                         \
    }                                           \
}

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})


int seg_fd;
int rank, size;
int verbose = 0;
int nordwr = 0;
int print_header = 1;

void print_usage(char *pname)
{
    if( 0 == rank ){
        printf("Usage %s:\n");
        printf("\t-h        Display this message\n");
        printf("\t-v        Verbose output: show the progress\n");
        printf("\t--no-rdwr Disable read/write test (for some locks it just hangs)\n");
        printf("\t--no-hdr  Don't print the header (useful for subsequent iterations in the scripts)\n");
    }
}

void parse_cmdline(int argc, char **argv)
{
    int i;
    for(i=1; i < argc; i++){
        if( !strcmp(argv[i], "-v") ){
            verbose = 1;
            continue;
        }
        if( !strcmp(argv[i], "-h") ){
            print_usage(argv[0]);
            MPI_Finalize();
            exit(0);
        }
        
        if( !strcmp(argv[i], "--no-rdwr") ){
            nordwr = 1;
            continue;
        }

        if( !strcmp(argv[i], "--no-hdr") ){
            print_header = 0;
            continue;
        }
        /* Unknown option: error exit */
        printf("ERROR: unknown option %s\n", argv[i]);
        print_usage(argv[0]);
        MPI_Finalize();
        exit(1);
    }
}

struct my {
    my_lock_t lock;
    volatile int counter1, counter2;
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
#define PERCENT(x, p) (x/(100/p))
#define WRONLY_REPS 10000
#define RDONLY_REPS 10000
#define WR_REPS 100

void reset_data(struct my *data)
{
    MPI_Barrier(MPI_COMM_WORLD);
    if( 0 == rank ){
        data->counter1 = 0;
        data->counter2 = 0;
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

void verification(struct my *data)
{
    int i;
    
    /* sync before starting verification */
    reset_data(data);
   
    /* Verify that we really getting truly rwlock
     * behavior
     */
    if( 0 == rank ){
        VERBOSE_OUT(verbose, "Data correctness verification\n");
        for(i=0; i<100; i++){
            if( (i % 10) == 9 ){
                 VERBOSE_OUT(verbose, "Step #%d%%\n", (i+1));
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
}

void wronly_test(struct my* data, double *perf)
{
    double time = 0, start;
    int i;
    
    reset_data(data);

    if (0 == rank) {
        VERBOSE_OUT(verbose, "Performance evaluation: write only\n");
        for(i=0; i<WRONLY_REPS; i++){
            if( ((i + 1) % PERCENT(WRONLY_REPS,10)) == 0 ){
                 VERBOSE_OUT(verbose, "Step #%d%%\n", (i+1)/PERCENT(WRONLY_REPS,1) );
            }
            start = GET_TS();
            shared_rwlock_wlock(&data->lock);
            time += GET_TS() - start;

            data->counter1++;
            data->counter2++;

            start = GET_TS();
            shared_rwlock_unlock(&data->lock);
            time += GET_TS() - start;
        }
    }
    *perf = time;
}

void rdonly_test(struct my* data, double *perf)
{
    double time = 0, start, time_cum;
    int i;
    
    reset_data(data);

    if( 0 == rank ){
        VERBOSE_OUT(verbose, "Performance evaluation: read only\n");
        int counter = 0, prev_step = 0;
        do {
            counter = data->counter1;
            if( ((counter + 1) / PERCENT(RDONLY_REPS,10)) > prev_step ){
                 VERBOSE_OUT(verbose, "Step #%d%%\n", (counter+1)/PERCENT(RDONLY_REPS,1) );
                 prev_step = (counter + 1) / PERCENT(RDONLY_REPS,10);
            }
        } while( data->counter1 < (RDONLY_REPS - 1) );
    } else {
        for(i=0; i<RDONLY_REPS; i++){
            start = GET_TS();
            shared_rwlock_rlock(&data->lock);
            time += GET_TS() - start;
            
            if( 1 == rank ){
                data->counter1++;
            }
            nsleep(500);
            start = GET_TS();
            shared_rwlock_unlock(&data->lock);
            time += GET_TS() - start;
        }
    }
    MPI_Reduce(&time, &time_cum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    *perf = (double)time_cum / (size - 1);
}

void rdwr_test(struct my *data, double *rlock_avg_cnt, 
                double *wr_time_out, double *wr_lock_time_out)
{
    int cur_count = 0;
    int prev_barrier = 0;
    unsigned long rlock_cnt = 0, rlock_cnt_cum = 0;
    int i;

    reset_data(data);

    *wr_time_out = 0;
    *wr_lock_time_out = 0;
    
    if (0 == rank) {
        double lock_time = 0;
        double wr_start, lock_start;

        VERBOSE_OUT(verbose,"Performance evaluation: read/write\n");
        VERBOSE_OUT(verbose,"\twriter: %d write cycles with %d usec sleep between them\n",
                            WR_REPS, size);
        VERBOSE_OUT(verbose,"\treaders: acquire as many as possible read locks\n");
        
        wr_start = GET_TS();
        for(i=0; i<WR_REPS; i++){
            if( ((i + 1) % PERCENT(WR_REPS,10)) == 0 ){
                 VERBOSE_OUT(verbose, "Step #%d%%\n", (i+1)/PERCENT(WR_REPS, 1));
            }
            lock_start = GET_TS();
            shared_rwlock_wlock(&data->lock);
            lock_time += GET_TS() - lock_start;
            
            data->counter1++;
            data->counter2++;
            cur_count++;
            
            lock_start = GET_TS();
            shared_rwlock_unlock(&data->lock);
            lock_time += GET_TS() - lock_start;

            /* Sleep proportionally to the number of procs */
            usleep(size);
        }
        *wr_lock_time_out = lock_time;
        *wr_time_out = GET_TS() - wr_start;
    } else {
        int cur_count = 0;
        int prev_barrier = 0;
        int i = 0;
        while( cur_count < WR_REPS ){
            /* almost busy-looping :) */
            shared_rwlock_rlock(&data->lock);

            cur_count = data->counter1;
            nsleep(100);

            shared_rwlock_unlock(&data->lock);
            rlock_cnt++;
        }
    }
    MPI_Reduce(&rlock_cnt, &rlock_cnt_cum, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    *rlock_avg_cnt = (double)rlock_cnt_cum / (size - 1);

}

int main(int argc, char **argv)
{
    char *membase = NULL;
    int i;
    double init_time = 0, start = 0;
    double wronly_lock_ovh = 0, rdonly_lock_ovh = 0;
    double rlock_avg_cnt = 0, rdwr_wr_time = 0, rdwr_wr_lock_ovh = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    parse_cmdline(argc, argv);


    {
        int delay = 0;
        while( delay ){
            sleep(1);
        }
    }

    init_time = GET_TS();
    if( 0 == rank ) {
        membase = create_seg(segname, 4096);
        data = (struct my*)membase;
        data->counter1 = 0;
        data->counter2 = 0;
        shared_rwlock_create(&data->lock);
    }
    init_time = GET_TS() - init_time;

    MPI_Barrier(MPI_COMM_WORLD);

    start = GET_TS();
    if( 0 != rank ){
        membase = attach_seg(segname, 4096);
        data = (struct my*)membase;
        shared_rwlock_init(&data->lock);
    }
    init_time += GET_TS() - start;

    // Correctness verification
#if (!defined (MY_PTHREAD_MUTEX) || ( MY_PTHREAD_MUTEX == 0 ))
    /* Mutexes will not pass this test as only one reader at the
     * time can get into the region - so the won't be able to
     * call barrier together
     */
     verification(data);
#endif
    

    wronly_test(data, &wronly_lock_ovh);
    rdonly_test(data, &rdonly_lock_ovh);
    if( !nordwr ){
        rdwr_test(data, &rlock_avg_cnt, &rdwr_wr_time, &rdwr_wr_lock_ovh);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    if( rank == 0){
        if( print_header ){
            printf("readers\twronly ovh\twronly (lck/s)\trdonly ovh\trdonly (lck/s)");
            if( !nordwr ) {
                printf("\trdwr (wr ovh)\trdwr (wr time)\trdwr (rlck/s)");
            }
            printf("\n");
        }
        printf("%d\t%lf\t%lf\t%lf\t%lf",
                size - 1, wronly_lock_ovh, WRONLY_REPS/wronly_lock_ovh,
                rdonly_lock_ovh,  RDONLY_REPS/rdonly_lock_ovh);
        if( !nordwr ){
           printf("\t%lf\t%lf\t%lf",rdwr_wr_lock_ovh, rdwr_wr_time, rlock_avg_cnt / rdwr_wr_time);
        }
        printf("\n");
    }

    shared_rwlock_fin(&data->lock);
    MPI_Finalize();
    return 0;
}