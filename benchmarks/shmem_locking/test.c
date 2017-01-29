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

/*
#define GET_TS() ({                         \
    struct timeval tv;                      \
    double ret = 0;                         \
    gettimeofday(&tv, NULL);                \
    ret = tv.tv_sec + 1E-6*tv.tv_usec;      \
    ret;                                    \
})
*/

#define WMB() \
{ asm volatile ("" : : : "memory"); }


int seg_fd;
int rank, size;
int verbose = 0;
int nordwr = 0;
int print_header = 1;
unsigned long us1;

#define usleep_my(x) {                  \
    unsigned long i;                    \
    int j;                              \
    for(j=0; j < x; j++){               \
        for(i = 0; i < us1; i++){       \
            asm("");                    \
        }                               \
    }                                   \
}

#define nsleep_my(x) {                  \
    unsigned long i;                    \
    double iters = (double)us1*x/1000;  \
    for(i = 0; i < iters; i++){    \
        asm("");                    \
    }                               \
}

#define SQUARE(x) (x * x)


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

struct stats_dbl{
    double avg, min, max;
};

struct my *data = NULL;

void *create_seg(char *fname, int size)
{
    void * seg_addr = NULL;

    unlink(fname);
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

void calibrate_sleep(struct my *data)
{
    double time;
    
    reset_data(data);
 
    if( 0 == rank ){
        unsigned long best_estim, up_border, step;
        double best_time;
        for(us1 = 0; us1 < 10000; us1++){
            asm("");
        }

        /* step1: rude estimation going down from 1E6 iters */
        us1 = 1000000;
        time = GET_TS();
        usleep_my(1);
        time = GET_TS() - time;

        best_estim = us1;
        best_time = time;
        
        for(us1/=10; us1 > 0; us1/=10){
            time = GET_TS();
            usleep_my(1);
            time = GET_TS() - time;
            if( SQUARE(best_time - 1E-06) > SQUARE(time - 1E-06) ){
                best_estim = us1;
                best_time = time;
            }
            if( time < 1E-06 ){
                break;
            }
        }

        /* step1: finer estimation going up with smaller steps */
        up_border = us1*10;
        step = us1;
        for(; us1 < up_border; us1 += step)
        {
            time = GET_TS();
            usleep_my(1);
            time = GET_TS() - time;
            if( SQUARE(best_time - 1E-06) > SQUARE(time - 1E-06) ){
                best_estim = us1;
                best_time = time;
            }
        }
            
        us1 = best_estim;
        VERBOSE_OUT(verbose,"Calibration: 1us = %zd iters\n", us1);
        
        /* Make sure that we finish with calculation first */
        WMB();
        data->counter1 = 1;
        
    } else {
        /* MPI_Bcast may cause heavy progress and keep
         * CPU busy, instead we want a quiet environment.
         * Use shared memory segment to sync.
         */
        while( data->counter1 == 0 ){
            sleep(1);
        }
    }
    MPI_Bcast(&us1, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
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
            usleep_my(100);
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

void rdonly_test(struct my* data, struct stats_dbl *perf)
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
            usleep_my(10);
            start = GET_TS();
            shared_rwlock_unlock(&data->lock);
            time += GET_TS() - start;
        }
    }
    MPI_Reduce(&time, &time_cum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    perf->avg = (double)time_cum / (size - 1);
    if( rank == 0){
        /* root doesn't contribute - 
         *use avg value so we'll get reasonable min and max 
         */
        time = perf->avg;
    }
    MPI_Reduce(&time, &perf->min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time, &perf->max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
}

void rdwr_test(struct my *data, struct stats_dbl *rlocks, 
                double *wr_time_out, double *wr_lock_time_out)
{
    int cur_count = 0;
    int prev_barrier = 0;
    unsigned long rlock_cnt = 0, rlock_cnt_cum = 0;
    int i;
    double rd_delay = 0, rd_delay_cum, wr_delay = 0;
    double rd_delay_tot = 0, rd_delay_tot_cum;

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
            double tmp = GET_TS();
            usleep_my(size * 10);
            wr_delay += GET_TS() - tmp;
        }
        *wr_lock_time_out = lock_time;
        *wr_time_out = GET_TS() - wr_start;
    } else {
        int cur_count = 0;
        int prev_barrier = 0;
        int i = 0;
        double time = 0, start;
        while( cur_count < WR_REPS ){
            /* almost busy-looping :) */
            shared_rwlock_rlock(&data->lock);

            cur_count = data->counter1;
            
            start = GET_TS();
            usleep_my(5);
            time += GET_TS() - start;

            shared_rwlock_unlock(&data->lock);
            rlock_cnt++;
        }
        rd_delay = time / rlock_cnt;
        rd_delay_tot = time;
    }


    if( rank == 0 ){
        rlock_cnt = 0;
    }
    MPI_Reduce(&rlock_cnt, &rlock_cnt_cum, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    rlocks->avg = (double)rlock_cnt_cum / (size - 1);

    if( rank == 0){
        /* root doesn't contribute - 
         *use avg value so we'll get reasonable min and max 
         */
        rlock_cnt = rlocks->avg;
    }
    MPI_Reduce(&rlock_cnt, &rlock_cnt_cum, 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
    rlocks->min = rlock_cnt_cum;
    MPI_Reduce(&rlock_cnt, &rlock_cnt_cum, 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
    rlocks->max = rlock_cnt_cum;

    MPI_Reduce(&rd_delay, &rd_delay_cum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&rd_delay_tot, &rd_delay_tot_cum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if( rank == 0 ){
        VERBOSE_OUT(verbose, "Avg rd_delay_tot = %le, rd_delay=%le, wr_delay_tot = %le, wr_delay = %le\n", 
            rd_delay_tot_cum / (size-1), rd_delay_cum / (size-1), wr_delay, wr_delay / 100);
    }
}

int main(int argc, char **argv)
{
    char *membase = NULL;
    int i;
    double init_time = 0, start = 0;
    double wronly_lock_ovh = 0;
    double rdwr_wr_time = 0, rdwr_wr_lock_ovh = 0;
    struct stats_dbl rdonly_lock_ovh, rlock_cnt;

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

    calibrate_sleep(data);

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
        rdwr_test(data, &rlock_cnt, &rdwr_wr_time, &rdwr_wr_lock_ovh);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    if( rank == 0){
        if( print_header ){
            printf("readers\tWO:ovh\tWO:lk/s\tRO:ovh[avg/min/max]\tRO:lk/s");
            if( !nordwr ) {
                printf("\tRW:wovh\tRW:wtm\tRW:rlk/s[avg/min/max]");
            }
            printf("\n");
        }
        printf("%d\t%.0le\t%u\t%.0le/%.0le/%0.le\t%u",
                size - 1, wronly_lock_ovh, (unsigned)(WRONLY_REPS/wronly_lock_ovh),
                rdonly_lock_ovh.avg, rdonly_lock_ovh.min, rdonly_lock_ovh.max, (unsigned)(RDONLY_REPS/rdonly_lock_ovh.avg));
        if( !nordwr ){
           printf("\t%.0le\t%.0le\t%u/%u/%u",rdwr_wr_lock_ovh, rdwr_wr_time, 
                        (unsigned)(rlock_cnt.avg / rdwr_wr_time), (unsigned)(rlock_cnt.min / rdwr_wr_time),
                        (unsigned)(rlock_cnt.max / rdwr_wr_time));
        }
        printf("\n");
    }

    shared_rwlock_fin(&data->lock);
    MPI_Finalize();
    return 0;
}