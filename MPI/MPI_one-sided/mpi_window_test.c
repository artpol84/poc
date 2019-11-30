#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <locale.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>

uint64_t rdtsc() {
    uint64_t ts;
    asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
            "shl $32, %%rdx\n\t"  // Shift the upper bits left.
            "or %%rdx, %0"        // 'Or' in the lower bits.
            : "=a" (ts)
            : 
            : "rdx");
    return ts;
}

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

enum timers {
    T_TOTAL,
    T_INIT,
    T_EXEC,
    T_WIN_CREATE,
    T_PUT,
    T_GET,
    T_WIN_FREE,
    T_WIN_FENCE,
    T_TIMERS
};
uint64_t *timers[T_TIMERS];

#define setSrcRank(a, val) (a |= ((unsigned long)val << 48)) 
#define setTid(a, val) (a |= ((unsigned long)val << 32))
#define setDstRank(a, val) (a |= ((unsigned long)val << 16))
#define setEl(a, val) (a |= (unsigned long)val)

#define getSrcRank(a) (a >> 48)
#define getTid(a) ((a >> 32)         & (unsigned long) 0x0000ffff)
#define getDstRank(a) ((a >> 16) & (unsigned long) 0x00000000ffff)
#define getEl(a) (a & (unsigned long) 0x0000000000000fff)

#define setFlag(a) (a |= 1ul << 15)
#define unsetFlag(a) (a &= ~(1ul << 15))
#define checkFlag(a) (a & (1ul << 15))

#define MPI_LOCK_TYPE(val) (val == 0 ? MPI_LOCK_SHARED : MPI_LOCK_EXCLUSIVE)

int rank = 0,
    ranks = 0,
    omp_threads = 1,
    nthreads = 0;

enum ret_values {
    SUCCESS,
    ERROR,
    DATA_ERROR,
    PARAM_ERROR
};

enum sync_mode {
    FENCE_SYNC,
    LOCK_SYNC,
    LOCK_SYNC_SHARED,
    LOCK_SYNC_EXCLUSIVE,
    FLUSH_SYNC,
    SYNC_MODES,
};
char *sync_modes_str[] = {"fence_sync", "lock_sync", "lock_sync:shared", "lock_sync:exclusive", "flush_sync"};
enum sync_mode sync = FENCE_SYNC;
int lock = MPI_LOCK_EXCLUSIVE;

enum put_get_mode {
    PUT,
    RPUT,
    GET,
    RGET,
    PUT_GET_MODES,
};
char *put_get_mode_str[] = {"put", "rput", "get", "rget"};

enum put_get_mode putr = PUT;
enum put_get_mode getr = GET;

int el_count = 1;

enum win_modes {
    SINGLE_WIN,
    MULTI_WIN,
    WIN_MODES,
};
char *win_modes_str[] = {"single", "multi"};

enum win_modes win_mode = SINGLE_WIN;

unsigned iter = 1;
unsigned datacheck = 1;
unsigned profile = 0;
unsigned win_free = 1;

enum arg_type {
    ARG_NTHREADS,
    ARG_SYNC_MODE,
    ARG_EL_COUNT,
    ARG_DATACHECK,
    ARG_PROFILE,
    ARG_PUT,
    ARG_RPUT,
    ARG_GET,
    ARG_RGET,
    ARG_WIN_MODE,
    ARG_WIN_FREE,
    ARG_ITERATIONS,
    ARG_HELP,
    ARG_UNDEF,
};


char* get_err_str(int err) {
    switch (err) {
        case SUCCESS:
            return "Success";
        case ERROR:
            return "Error (General)";
        case DATA_ERROR:
            return "Data error";
        case PARAM_ERROR:
            return "Input parameter error";
        default:
            return "Undef error";
    };
}

enum arg_type get_arg_type (char *arg) {
    enum arg_type ret = ARG_UNDEF;

    if (strcmp(arg, "-sync_mode") == 0) {
        ret = ARG_SYNC_MODE;
    }
    else if (strcmp(arg, "-el_count") == 0) {
        ret = ARG_EL_COUNT;
    }
    else if (strcmp(arg, "-datacheck") == 0) {
        ret = ARG_DATACHECK;
    }
    else if (strcmp(arg, "-profile") == 0) {
        ret = ARG_PROFILE;
    }
    else if (strcmp(arg, "-put") == 0) {
        ret = ARG_PUT;
    }
    else if (strcmp(arg, "-get") == 0) {
        ret = ARG_GET;
    }
    else if (strcmp(arg, "-rput") == 0) {
        ret = ARG_RPUT;
    }
    else if (strcmp(arg, "-rget") == 0) {
        ret = ARG_RGET;
    }
    else if (strcmp(arg, "-win_mode") == 0) {
        ret = ARG_WIN_MODE;
    }
    else if (strcmp(arg, "-win_free") == 0) {
        ret = ARG_WIN_FREE;
    }
    else if (strcmp(arg, "-iter") == 0) {
        ret = ARG_ITERATIONS;
    }
    else if(strcmp(arg, "-help") == 0) {
        ret = ARG_HELP;
    }

    return ret;
}

int check_and_set_sync_mode(char* arg)
{
    int i = 0;
    int ret = 0;
    for (i = 0; i < SYNC_MODES; i++) {
        if (strcmp(arg, sync_modes_str[i]) == 0) {
            sync = i;
            if (!strcmp(arg, "lock_sync:shared")) {
                lock = MPI_LOCK_SHARED;
                sync = LOCK_SYNC;
            }
            else if (!strcmp(arg, "lock_sync:exclusive")) {
                lock = MPI_LOCK_EXCLUSIVE;
                sync = LOCK_SYNC;
            }
           
            ret++;
            break;
        }
    }

    return ret;
}

int check_and_set_win_mode(char* arg)
{
    int i = 0;
    int ret = 0;

    for (i = 0; i < WIN_MODES; i++) {
        if (strcmp(arg, win_modes_str[i]) == 0) {
            win_mode = i;
            ret++;
            break;
        }
    }

    return ret;
}

void print_accepted_args(char* arg1, char* arg2)
{
    if (strcmp(arg1, "help")) {
        printf ("Wrong argument for \"%s\" <%s>\n", arg1, arg2);
    }
    printf ("Accepted args:\n"
           "-sync_mode <\"fence_sync\", \"lock_sync\", \"flush_sync\">\n"
           "-el_count <int>\n"
           "-datacheck, -profile, put, -rput, -get, -rget\n"
           "-win_mode <\"single\", \"multi\">\n"
           "-win_free <int> (0,1)\n"
           "-iter <int>\n");
}

int process_args(int argc, char *argv[])
{
    int i=1;
    for (i = 1; i < argc; i++) {
        char *arg = argv[i];
        switch ( get_arg_type (arg) ) {
            case ARG_PROFILE:
                profile = 1;
                int j = 0;
                for (j = 0; j < T_TIMERS; j++) {
                    timers[j] = malloc(omp_threads * sizeof(uint64_t));
                    memset(timers[j], 0, omp_threads * sizeof(uint64_t));
                }
                break;
            case ARG_SYNC_MODE:
                i++;
                arg = argv[i];
                if (check_and_set_sync_mode(arg)) {
                    break;
                }
                else {
                    return PARAM_ERROR;
                }
                break;
            case ARG_EL_COUNT:
                i++;
                if (argv[i] == NULL || atoi(argv[i]) < 1 || atoi(argv[i]) > 1000000) {
                    print_accepted_args(arg, argv[i]);
                    return PARAM_ERROR;
                }
                el_count = atoi(argv[i]);
                break;
            case ARG_DATACHECK:
                datacheck = 1;
                break;
            case ARG_PUT:
                putr = PUT;
                break;
            case ARG_GET:
                getr = GET;
                break;
            case ARG_RPUT:
                putr = RPUT;
                break;
            case ARG_RGET:
                getr = RGET;
                break;
            case ARG_WIN_MODE:
                i++;
                if (argv[i] != NULL && check_and_set_win_mode(argv[i])) {
                    break;
                }
                else {
                    print_accepted_args(arg, argv[i]);
                    return PARAM_ERROR;
                }
                break;
            case ARG_WIN_FREE:
                i++;
                if (argv[i] != NULL) {
                    win_free = atoi(argv[i]);
                    break;
                }
                else {
                    print_accepted_args(arg, argv[i]);
                    return PARAM_ERROR;
                }
                break;
            case ARG_ITERATIONS:
                i++;
                if (argv[i] == NULL || atoi(argv[i]) < 1) {
                    print_accepted_args(arg, argv[i]);
                    return PARAM_ERROR;
                }
                iter = atoi(argv[i]);
                break;
            case ARG_HELP:
                print_accepted_args("help", NULL);
                exit(0);
            default:
                return PARAM_ERROR;
        }
    }

    return SUCCESS;
}

void print_args(int f)
{
    if (f) {
        printf ("sync_mode = %s; threads = %d; "
                "el_count = %d, put_op = %s, get_op = %s, "
                "iter = %d, win_mode = %s, "
                "datacheck = %d, profile = %d, win_free = %d\n",
                sync_modes_str[sync], omp_threads, el_count,
                put_get_mode_str[putr], put_get_mode_str[getr],
                iter, win_modes_str[win_mode],
                datacheck, profile, win_free);
    }

    return;
}

int run_single_window_test(int run)
{
    MPI_Win *wins;
    MPI_Comm comm;
    uint64_t _t;
    unsigned long *put_data, *put_buffer, *get_buffer;
    int tid, ntids, dst, err = 0,
        mem_size, get_mem_size, i, ii, jj, kk;

    if (rank == 0 && run) {
        printf("Running single window test using %d ranks and %d threads/rank\n", ranks, omp_threads);
    }

    wins = malloc(iter * sizeof(*wins));
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    mem_size = omp_threads * ranks * el_count * sizeof(unsigned long int);
    get_mem_size = mem_size * ranks;
    put_data = malloc(mem_size);
    put_buffer = malloc(mem_size);
    get_buffer = malloc(get_mem_size);

    // main thread creates win
    // everyone writes put/get
    // main thread destroys win
    for (i = 0; i < iter; i++) {
        MPI_Barrier(comm);
        if(profile && run)
            _t = rdtsc();
        MPI_Win_create (put_data, mem_size,
                sizeof(unsigned long int),
                MPI_INFO_NULL, comm, &wins[i]);

        if (sync == FENCE_SYNC)
            MPI_Win_fence(0, wins[i]);

        if (profile && run) {
            _t = rdtsc() - _t;
            timers[T_WIN_CREATE][0] += _t;
        }

        /* Clear data */
        if (datacheck) {
                memset(put_data, 0x0, mem_size);
                memset(put_buffer, 0x0, mem_size);
                memset(get_buffer, 0x0, get_mem_size);
                MPI_Win_fence(0, wins[i]);

#pragma omp parallel private (ntids, tid, dst, ii, jj, kk, _t)
            {
                ntids = omp_get_num_threads();
                tid = omp_get_thread_num();

                /* Put data */
                int offset;
                int l_offset;
               
                if (profile && run)
                    _t = rdtsc();

                for (dst = 0; dst < ranks; dst++)
                {
                    offset = (tid * ranks * el_count) + (rank * el_count);
                    l_offset = (tid * ranks * el_count) + (dst * el_count);
                    
                    /* Prepare buffer */
                    for (ii = 0; ii < el_count; ii++) {
                        int _i = l_offset + ii;
                        put_buffer[_i] = 0;
                        setSrcRank(put_buffer[_i], rank);
                        setTid(put_buffer[_i], tid);
                        setDstRank(put_buffer[_i], dst);
                        setEl(put_buffer[_i], ii);
                        if (sync == LOCK_SYNC)
                            setFlag(put_buffer[_i]);
#if 0
                        printf ("PUT: %d[%d] -> %d[%d]: %lu %lu %lu %lu [%lx]\n",
                                rank, tid, dst, tid,
                                    getSrcRank(put_buffer[_i]),
                                    getTid(put_buffer[_i]),
                                    getDstRank(put_buffer[_i]),
                                    getEl(put_buffer[_i]),
                                    put_buffer[_i]);
#endif
                    }

                    if (sync == LOCK_SYNC) {
                        /* linear ring */
                        for (ii = 0; ii < ntids; ii++) {
                            if (ii == tid) {
                                MPI_Win_lock(lock, dst, 0, wins[i]);
                            
                                MPI_Put(&put_buffer[l_offset], el_count, MPI_UNSIGNED_LONG,
                                        dst, offset, el_count, MPI_UNSIGNED_LONG,
                                        wins[i]);
                            
                                MPI_Win_unlock(dst, wins[i]);
                                if (lock == MPI_LOCK_EXCLUSIVE)
                                    MPI_Barrier(comm);
                            }
                            #pragma omp barrier
                        }
                    }
                    else { /* Fence sync */
                        MPI_Put(&put_buffer[l_offset], el_count, MPI_UNSIGNED_LONG,
                                dst, offset, el_count, MPI_UNSIGNED_LONG,
                                wins[i]);
                    }
                }

#pragma omp barrier
                /* Synchronize puts */
                if (sync == FENCE_SYNC) {
                    if (tid == 0)
                        MPI_Win_fence(0, wins[i]);
                }
                else if (sync == LOCK_SYNC && lock == MPI_LOCK_SHARED) {
                    if (tid == 0)
                        MPI_Barrier(comm);
                }

                if (profile && run) {
                    _t = rdtsc() - _t;
                    timers[T_PUT][tid] += _t;
                }
#pragma omp barrier

                /* Check Put data */
#if 0
                char str[2048]; memset(str, 0, 2048);
                sprintf (str, "PUT CHECK: %d[%d]: ", rank, tid);
                for (ii = 0; ii < omp_threads * ranks * el_count; ii++) {
                    sprintf(str, "%s %lu %lu %lu %lu, ", str,
                            getSrcRank(put_data[ii]), getTid(put_data[ii]),
                            getDstRank(put_data[ii]), getEl(put_data[ii]));
                }
                printf ("%s\n", str);
#endif
                offset = (tid * ranks * el_count);
                for (ii = 0; ii < ranks; ii++) {
                    for (jj = 0; jj < el_count; jj++) {
                        unsigned long expected = 0;
                        setSrcRank(expected, ii);
                        setTid(expected, tid);
                        setDstRank(expected, rank);
                        setEl(expected, jj);
                        if (sync == LOCK_SYNC) {
                            setFlag(expected);
                            while (1) {
                                if (checkFlag(put_data[offset])) {
                                    break;
                                }
                            }
                        }

                        if (expected != put_data[offset] && run) {
                            printf ("[Thread %d]: MPI_Put data error: expected: %lx, recv: %lx, at put_data offset = %d\n",
                                    tid, expected, put_data[offset], offset);
                            err++;
                        }
                        offset++;
                    }
                }

                /* Check get op */
                if (profile && run)
                    _t = rdtsc();

                for (dst = 0; dst < ranks; dst++) {
                    l_offset = (tid * ranks * ranks * el_count) + (dst * ranks * el_count);
                    offset = tid * ranks * el_count;
   
                    if (sync == LOCK_SYNC) {
                        for (ii = 0; ii < ntids; ii++) {
                            if (ii == tid) {
                                MPI_Win_lock(lock, dst, 0, wins[i]);

                                MPI_Get(&get_buffer[l_offset], ranks * el_count, MPI_UNSIGNED_LONG,
                                        dst, offset, ranks * el_count, MPI_UNSIGNED_LONG,
                                        wins[i]);
                            
                                MPI_Win_unlock(dst, wins[i]);

                                if (lock == MPI_LOCK_EXCLUSIVE)
                                    MPI_Barrier(comm);
                            }
                            #pragma omp barrier
                        }
                    }
                    else {
                        /* Fence sync */
                        MPI_Get(&get_buffer[l_offset], ranks * el_count, MPI_UNSIGNED_LONG,
                                dst, offset, ranks * el_count, MPI_UNSIGNED_LONG,
                                wins[i]);
                    }
                }

                /* Synchronize gets */
#pragma omp barrier
                if (sync == FENCE_SYNC) {
                    if (tid == 0)
                        MPI_Win_fence(0, wins[i]);
                }
                else if (sync == LOCK_SYNC && lock == MPI_LOCK_SHARED) {
                    if (tid == 0) 
                        MPI_Barrier(comm);
                }
           
                if (profile && run) {
                    _t = rdtsc() - _t;
                    timers[T_GET][tid] += _t;
                }
#pragma omp barrier

                /* Check Get data */
#if 0
                memset(str, 0, 2048);
                sprintf (str, "GET CHECK: %d[%d]: ", rank, tid);
                for (ii = 0; ii < omp_threads * ranks * ranks * el_count; ii++) {
                    sprintf(str, "%s %lu %lu %lu %lu, ", str,
                            getSrcRank(get_buffer[ii]), getTid(get_buffer[ii]),
                            getDstRank(get_buffer[ii]), getEl(get_buffer[ii]));
                }
                printf ("%s\n", str);
#endif

                l_offset = tid * ranks * ranks * el_count;
                int ll_offset = l_offset;
                for (ii = 0; ii < ranks; ii++) {
                    for (jj = 0; jj < ranks; jj++) {
                        for (kk = 0; kk < el_count; kk++) {
                            unsigned long expected = 0;
                            setSrcRank(expected, jj);
                            setTid(expected, tid);
                            setDstRank(expected, ii);
                            setEl(expected, kk);
                            if (sync == LOCK_SYNC) {
                                setFlag(expected);
                                while (1) {
                                    if (checkFlag(get_buffer[ll_offset])) {
                                        break;
                                    }
                                }
                            }
                            if (expected != get_buffer[ll_offset] && run) {
                                printf ("[Thread %d]: MPI_Get data error: expected: %lx, recv: %lx, at get_buffer offset = [%d], l_offset = %d, ii = %d, jj = %d, kk = %d\n",
                                        tid, expected, get_buffer[ll_offset], ll_offset, l_offset, ii, jj, kk);
                                err++;
                            }
                            ll_offset++;
                        }
                    }
                }
            }
        }

        if (err) {
            free(wins);
            free(put_data);
            free(put_buffer);
            free(get_buffer);
            return err;
        }

        if (win_free) {
            if (profile && run)
                _t = rdtsc();
            MPI_Win_free(&wins[i]);
            if (profile && run) {
                _t = rdtsc() - _t;
                timers[T_WIN_FREE][0] += _t;
            }
        }
    }


    if (!win_free) {
        if (profile && run)
            _t = rdtsc();
        for (i = 0; i < iter; i++) {
            MPI_Win_free(&wins[i]);
        }
        if (profile && run) {
            _t = rdtsc() - _t;
            timers[T_WIN_FREE][0] += _t;
        }
    }
    
    MPI_Barrier(comm);
    
    if (profile && run) {
        for (i = 1; i < omp_threads; i++) {
            timers[T_PUT][0] += timers[T_PUT][i];
            timers[T_GET][0] += timers[T_GET][i];
            timers[T_WIN_FENCE][0] += timers[T_WIN_FENCE][i];
        }

        timers[T_PUT][0] = timers[T_PUT][0] / omp_threads;
        timers[T_GET][0] = timers[T_GET][0] / omp_threads;
        timers[T_WIN_FENCE][0] = timers[T_WIN_FENCE][0] / omp_threads;
    }

    free(wins);
    free(put_data);
    free(put_buffer);
    free(get_buffer);

    return err;
}

int run_multi_window_test(int run)
{
    MPI_Win **wins;
    MPI_Comm *comms;

    int dst, err = 0, i, ii, jj, kk, j, k, ntids, tid, mem_size, get_mem_size;
    uint64_t _t;

    if (rank == 0 && run) {
        printf("Running multi window test using %d ranks and %d threads/rank\n", ranks, omp_threads);
    }

    wins = malloc(omp_threads * sizeof(*wins));
    for (i = 0; i < omp_threads; i++) {
        wins[i] = malloc(iter * sizeof(**wins));
    }

    comms = malloc(omp_threads * sizeof(MPI_Comm));
    for (i=0 ; i < omp_threads; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

#pragma omp parallel private (ntids, tid, i, j, k, dst, ii, jj, kk, _t)
    {
        ntids = omp_get_num_threads();
        tid = omp_get_thread_num();

        mem_size = ranks * el_count * sizeof(unsigned long int);
        get_mem_size = mem_size * ranks;
        unsigned long *put_data = malloc(mem_size);
        unsigned long *put_buffer = malloc(mem_size);
        unsigned long *get_buffer = malloc(get_mem_size);

        for (i = 0; i < iter; i++) {

            MPI_Barrier(comms[tid]);

            if(profile && run)
                _t = rdtsc();
            MPI_Win_create(put_data, ranks * el_count * sizeof(unsigned long), sizeof(unsigned long), MPI_INFO_NULL, comms[tid], &wins[tid][i]);
        
            if (sync == FENCE_SYNC)
                MPI_Win_fence(0, wins[tid][i]);

            if (profile && run) {
                _t = rdtsc() - _t;
                timers[T_WIN_CREATE][tid] += _t;
            }

            if (datacheck) {
                memset(put_data, 0x0, mem_size);
                memset(put_buffer, 0x0, mem_size);
                memset(get_buffer, 0x0, get_mem_size);
                MPI_Win_fence(0, wins[tid][i]);

                if (profile && run)
                    _t = rdtsc();

                /* Put data */
                int offset;
                int l_offset;

                for (dst = 0; dst < ranks; dst++)
                {
                    offset = rank * el_count;
                    l_offset = dst * el_count;

                    /* Prepare buffer */
                    for (ii = 0; ii < el_count; ii++) {
                        int _i = l_offset + ii;
                        put_buffer[_i] = 0;
                        setSrcRank(put_buffer[_i], rank);
                        setTid(put_buffer[_i], tid);
                        setDstRank(put_buffer[_i], dst);
                        setEl(put_buffer[_i], ii);
                        if (sync == LOCK_SYNC) {
                            setFlag(put_buffer[_i]);
                        }
                        /* printf ("PUT: %d[%d] -> %d[%d]: %lu %lu %lu %lu, offset = %d, l_offset = %d\n", rank, tid, dst, tid,
                                    getSrcRank(put_buffer[_i]),
                                    getTid(put_buffer[_i]),
                                    getDstRank(put_buffer[_i]),
                                    getEl(put_buffer[_i]),
                                    offset + ii, _i); */
                          
                    }

                    /* Put to rank dst */
                    if (sync == LOCK_SYNC) {

                        MPI_Win_lock(lock, dst, 0, wins[tid][i]);

                        MPI_Put(&put_buffer[l_offset], el_count, MPI_UNSIGNED_LONG,
                                dst, offset, el_count, MPI_UNSIGNED_LONG,
                                wins[tid][i]);


                        MPI_Win_unlock(dst, wins[tid][i]);
                        if (lock == MPI_LOCK_EXCLUSIVE) {
                            MPI_Barrier(comms[tid]);
                        }
                    }
                    else {
                       MPI_Put(&put_buffer[l_offset], el_count, MPI_UNSIGNED_LONG,
                                dst, offset, el_count, MPI_UNSIGNED_LONG,
                                wins[tid][i]);
                    }
                }
                    
                /* Synchronize puts */
                if (sync == FENCE_SYNC) {
                    MPI_Win_fence(0, wins[tid][i]);
                }
                else if (sync == LOCK_SYNC && lock == MPI_LOCK_SHARED) {
                    MPI_Barrier(comms[tid]);
                }

                if (profile && run) {
                    _t = rdtsc() - _t;
                    timers[T_PUT][tid] += _t;
                }

                /* Check Put data */
#if 0
                char str[2048]; memset(str, 0, 2048);
                sprintf (str, "PUT CHECK: %d[%d]: ", rank, tid);
                for (ii = 0; ii < ranks * el_count; ii++) {
                    sprintf(str, "%s %lu %lu %lu %lu, ", str,
                            getSrcRank(put_data[ii]), getTid(put_data[ii]),
                            getDstRank(put_data[ii]), getEl(put_data[ii]));
                }
                printf ("%s\n", str);
#endif
                offset = 0;
                for (ii = 0; ii < ranks; ii++) {
                    for (jj = 0; jj < el_count; jj++) {
                        unsigned long expected = 0;
                        setSrcRank(expected, ii);
                        setTid(expected, tid);
                        setDstRank(expected, rank);
                        setEl(expected, jj);
                        if (sync == LOCK_SYNC) {
                            setFlag(expected);
                            while (1) {
                                if (checkFlag(put_data[offset])) {
                                    break;
                                }
                            }
                        }

                        if (expected != put_data[offset] && run) {
                            printf ("[Thread %d]: MPI_Put data error: expected: %lx, recv: %lx, at put_data offset = %d\n",
                                    tid, expected, put_data[offset], offset);
                            err++;
                        }
                        offset++;
                    }
                }

                if (profile && run)
                    _t = rdtsc();

                /* Do MPI_Get() */
                for (dst = 0; dst < ranks; dst++) {
                    l_offset = dst * ranks * el_count;

                    if (sync == LOCK_SYNC) {
                        MPI_Win_lock(lock, dst, 0, wins[tid][i]);

                        MPI_Get(&get_buffer[l_offset], ranks * el_count, MPI_UNSIGNED_LONG,
                                dst, 0, ranks * el_count, MPI_UNSIGNED_LONG,
                                wins[tid][i]);

                        MPI_Win_unlock(dst, wins[tid][i]); 
                        if (lock == MPI_LOCK_EXCLUSIVE) {
                            MPI_Barrier(comms[tid]);
                        }
                    }
                    else{
                        MPI_Get(&get_buffer[l_offset], ranks * el_count, MPI_UNSIGNED_LONG,
                                dst, 0, ranks * el_count, MPI_UNSIGNED_LONG,
                                wins[tid][i]);
                    }
                }
                
                /* Synchronize gets */
                if (sync == FENCE_SYNC) {
                    MPI_Win_fence(0, wins[tid][i]);
                }
                else if (sync == LOCK_SYNC && lock == MPI_LOCK_SHARED) {
                    MPI_Barrier(comms[tid]);
                }
                
                if (profile && run) {
                    _t = rdtsc() - _t;
                    timers[T_GET][tid] += _t;
                }

                /* Check Get data */
#if 0
                memset(str, 0, 2048);
                sprintf (str, "GET CHECK: %d[%d]: ", rank, tid);
                for (ii = 0; ii < ranks * ranks * el_count; ii++) {
                    sprintf(str, "%s %lu %lu %lu %lu, ", str,
                            getSrcRank(get_buffer[ii]), getTid(get_buffer[ii]),
                            getDstRank(get_buffer[ii]), getEl(get_buffer[ii]));
                }
                printf ("%s\n", str);
#endif

                l_offset = 0;
                for (ii = 0; ii < ranks; ii++) {
                    for (jj = 0; jj < ranks; jj++) {
                        for (kk = 0; kk < el_count; kk++) {
                            unsigned long expected = 0;
                            setSrcRank(expected, jj);
                            setTid(expected, tid);
                            setDstRank(expected, ii);
                            setEl(expected, kk);
                            if (sync == LOCK_SYNC) {
                                setFlag(expected);
                                while (1) {
                                    if (checkFlag(get_buffer[l_offset])) {
                                        break;
                                    }
                                }
                            }
                            if (expected != get_buffer[l_offset] && run) {
                                printf ("[Thread %d]: MPI_Get data error: expected: %lx, recv: %lx, at get_buffer offset = [%d], l_offset = %d, ii = %d, jj = %d, kk = %d\n",
                                        tid, expected, get_buffer[l_offset], l_offset, l_offset, ii, jj, kk);
                                err++;
                            }
                            l_offset++;
                        }
                    }
                }
            }
            if (win_free) {
                if (profile && run)
                    _t = rdtsc();

                MPI_Win_free(&wins[tid][i]);

                if (profile && run) {
                    _t = rdtsc() - _t;
                    timers[T_WIN_FREE][tid] += _t;
                }
            }
        }
            
        free(put_data);
        free(put_buffer);
        free(get_buffer);

        if (!win_free) {
            if (profile && run)
                _t = rdtsc();
            for (j = 0; j < iter; j++) {
                MPI_Win_free(&wins[tid][j]);
            }
            if (profile && run) {
                _t = rdtsc() - _t;
                timers[T_WIN_FREE][tid] += _t;
            }
        }
    }

    free (wins);
    wins = NULL;

    for (i=0; i<omp_threads; i++) {
        MPI_Comm_free(&comms[i]);
    }

    free(comms);

    if (profile && run && 0) {
        for (i = 1; i < omp_threads; i++) {
            timers[T_WIN_CREATE][0] += timers[T_WIN_CREATE][i];
            timers[T_WIN_FREE][0] += timers[T_WIN_FREE][i];
            timers[T_WIN_FENCE][0] += timers[T_WIN_FENCE][i];
            timers[T_PUT][0] += timers[T_PUT][i];
            timers[T_GET][0] += timers[T_GET][i];
        }

        timers[T_WIN_CREATE][0] = timers[T_WIN_CREATE][0] / omp_threads;
        timers[T_WIN_FREE][0] = timers[T_WIN_FREE][0] / omp_threads;
        timers[T_GET][0] = timers[T_GET][0] / omp_threads;
        timers[T_GET][0] = timers[T_GET][0] / omp_threads;
        timers[T_WIN_FENCE][0] = timers[T_WIN_FENCE][0] / omp_threads;
    }

    return err;
}

int main(int argc, char *argv[])
{
    char *env = NULL;
    int ret, errs = 0;
    int provided, main_thread, claimed,
        ntids, tid, i, j, k;

    env = getenv("OMP_NUM_THREADS");
    omp_threads = atoi(env == NULL ? "1" : env);

    ret = process_args(argc, argv);
    if (ret != SUCCESS) {
        printf ("Failed to process arguments: %s\n", get_err_str(ret));
        return 0;
    }

    /* print current values */
    print_args(1);

    if (profile) 
        timers[T_INIT][0] = rdtsc();

    ret = MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);
    if (MPI_SUCCESS != ret) {
        char error_str[MPI_MAX_ERROR_STRING];
        int error_len;
        MPI_Error_string(ret, error_str, &error_len);
        printf("%s\n", error_str);
        return ret;
    }

    /* Check threading support */
    MPI_Is_thread_main ( &main_thread );
    if (!main_thread) {
        errs++;
        printf ("This thread called init_thread but Is_thread_main gave false\n");
        fflush(stdout);
    }
    MPI_Query_thread (&claimed);
    if (claimed != provided) {
        errs++;
        printf ("Query thread gave thread level %d but Init_thread gave %d\n", claimed, provided);
        fflush(stdout);
    }
    if (errs > 0) {
        goto EXIT;
    }

    if (profile)
        timers[T_INIT][0] = rdtsc() - timers[T_INIT][0];

    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &ranks);


    /* Get runtime threadid and treads count */
#if 0
#pragma omp parallel private (ntids, tid)
    {
        ntids = omp_get_num_threads();
        tid = omp_get_thread_num();

        printf ("Hello from rank %d/%d and thread %d/%d\n", rank, ranks, tid, ntids);
        fflush(stdout);
#pragma omp barrier
    }
#endif

    MPI_Barrier(MPI_COMM_WORLD);

//     if (win_mode == SINGLE_WIN) {
//         run_single_window_test(0);
//     }
//     else if (win_mode == MULTI_WIN) {
//         run_multi_window_test(0);
//     }

    if (profile)
        timers[T_EXEC][0] = rdtsc();

    if ( win_mode == SINGLE_WIN) {
        ret = run_single_window_test(1);
        if (ret != SUCCESS) {
           errs++;
           goto EXIT;
        }
    }
    else if (win_mode == MULTI_WIN) {
        ret = run_multi_window_test(1);
        if (ret != SUCCESS) {
            errs++;
            goto EXIT;
        }
    }

    if (profile)
        timers[T_EXEC][0] = rdtsc() - timers[T_EXEC][0];

    unsigned long res[21];
    if (profile) {

        MPI_Reduce(&timers[T_INIT][0], &res[0], 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_INIT][0], &res[1], 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_INIT][0], &res[2], 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Reduce(&timers[T_EXEC][0], &res[3], 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_EXEC][0], &res[4], 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_EXEC][0], &res[5], 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Reduce(&timers[T_WIN_CREATE][0], &res[6], 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_WIN_CREATE][0], &res[7], 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_WIN_CREATE][0], &res[8], 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Reduce(&timers[T_WIN_FREE][0], &res[9], 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_WIN_FREE][0], &res[10], 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_WIN_FREE][0], &res[11], 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Reduce(&timers[T_PUT][0], &res[12], 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_PUT][0], &res[13], 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_PUT][0], &res[14], 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Reduce(&timers[T_GET][0], &res[15], 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_GET][0], &res[16], 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_GET][0], &res[17], 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Reduce(&timers[T_WIN_FENCE][0], &res[18], 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_WIN_FENCE][0], &res[19], 1, MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&timers[T_WIN_FENCE][0], &res[20], 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

EXIT:
    if (errs > 0) {
        printf ("Failed! Errs = %d\n", errs); fflush(stdout); 
    }
    else if (rank == 0) {
        printf("Success!\n");
        if (profile) {
            //iter = (win_free) ? iter : 1;
            printf("Init_time: %lu %lu %lu\n", res[0], res[1], res[2] / ranks);
            printf("Execution_time: %lu %lu %lu\n", res[3], res[4], res[5] / ranks);
            printf("win_create(): %lu %lu %lu\n", res[6], res[7], res[8] / ranks); 
            printf("win_free(): %lu %lu %lu\n", res[9], res[10], res[11] / ranks);
            //printf("win_fence(): %lu %lu %lu\n", res[18] / iter, res[19] / iter, (res[20] / (double)ranks) / iter);
            printf("put(): %lu %lu %lu\n", res[12], res[13], res[14] / ranks);
            printf("get(): %lu %lu %lu\n", res[15], res[16], res[17] / ranks);
        }
    }

    if (profile) {
        for (i = 0; i < T_TIMERS; i++) {
            free(timers[i]);
        }
    }

    return errs;
}
