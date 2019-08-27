#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <string.h>

#define setSrcRank(a, val) (a |= ((unsigned long)val << 48)) 
#define setTid(a, val) (a |= ((unsigned long)val << 32))
#define setDstRank(a, val) (a |= ((unsigned long)val << 16))
#define setEl(a, val) (a |= (unsigned long)val)

#define getSrcRank(a) (a >> 48)
#define getTid(a) ((a >> 32)         & (unsigned long) 0x0000ffff)
#define getDstRank(a) ((a >> 16) & (unsigned long) 0x00000000ffff)
#define getEl(a) (a          & (unsigned long) 0x000000000000ffff)

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
    FLUSH_SYNC,
    SYNC_MODES,
};
char *sync_modes_str[] = {"fence_sync", "lock_sync", "flush_sync"};
enum sync_mode sync = FENCE_SYNC;

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
unsigned win_free = 1;

enum arg_type {
    ARG_NTHREADS,
    ARG_SYNC_MODE,
    ARG_EL_COUNT,
    ARG_DATACHECK,
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

    /* Ignore threads for now, we'll specify it via env variable */
//     if (strcmp(arg, "-nthreads") == 0) {
//         ret = ARG_NTHREADS;
//     }
    if (strcmp(arg, "-sync_mode") == 0) {
        ret = ARG_SYNC_MODE;
    }
    else if (strcmp(arg, "-el_count") == 0) {
        ret = ARG_EL_COUNT;
    }
    else if (strcmp(arg, "-datacheck") == 0) {
        ret = ARG_DATACHECK;
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
    if (rank) return;

    if (strcmp(arg1, "help")) {
        printf ("Wrong argument for \"%s\" <%s>\n", arg1, arg2);
    }
    printf ("Accepted args:\n"
           "-sync_mode <\"fence_sync\", \"lock_sync\", \"flush_sync\">\n"
           "-el_count <int>\n"
           "-datacheck <int> (0,1)\n"
           "-put, -rput, -get, -rget\n"
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
            case ARG_NTHREADS: // XXX this case is ignored
                i++;
                if (argv[i] == NULL || atoi(argv[i]) < 1 || atoi(argv[i]) > 256) {
                    print_accepted_args(arg, argv[i]);
                    return PARAM_ERROR;
                }
                nthreads = atoi(argv[i]);
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
                i++;
                if (argv[i] == NULL || atoi(argv[i]) < 0 || atoi(argv[i]) > 1) {
                    print_accepted_args(arg, argv[i]);
                    return PARAM_ERROR; 
                }
                datacheck = atoi(argv[i]);
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
                "datacheck = %d, win_free = %d\n",
                sync_modes_str[sync], omp_threads, el_count,
                put_get_mode_str[putr], put_get_mode_str[getr],
                iter, win_modes_str[win_mode],
                datacheck, win_free);
    }

    return;
}

int run_single_window_test(void)
{
    MPI_Win *wins;
    MPI_Comm comm;
    unsigned long *put_data, *put_buffer, *get_buffer;
    int tid, ntids, dst, err = 0,
        mem_size, get_mem_size, i, ii, jj, kk;

    if (rank == 0) {
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
        MPI_Win_create (put_data, mem_size,
                sizeof(unsigned long int),
                MPI_INFO_NULL, comm, &wins[i]);

        MPI_Win_fence(0, wins[i]);

        /* Clear data */
        if (datacheck) {
                memset(put_data, 0xff, mem_size);
                memset(put_buffer, 0xff, mem_size);
                memset(get_buffer, 0xff, get_mem_size);
                MPI_Win_fence(0, wins[i]);

#pragma omp parallel private (ntids, tid, dst, ii, jj, kk)
            {
                ntids = omp_get_num_threads();
                tid = omp_get_thread_num();

                /* Put data */
                int offset;
                int l_offset;
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
                        /*
                        printf ("PUT: %d[%d] -> %d[%d]: %lu %lu %lu %lu, offset = %d, l_offset = %d\n", rank, tid, dst, tid,
                                    getSrcRank(put_buffer[_i]),
                                    getTid(put_buffer[_i]),
                                    getDstRank(put_buffer[_i]),
                                    getEl(put_buffer[_i]),
                                    offset + ii, _i);
                        */
                    }
                   
                    /* Put to rank dst */
                    MPI_Put(&put_buffer[l_offset], el_count, MPI_UNSIGNED_LONG,
                            dst, offset, el_count, MPI_UNSIGNED_LONG,
                            wins[i]);
                }
                    
                /* Synchronize puts */
#pragma omp barrier
                if (tid == 0)
                MPI_Win_fence(0, wins[i]);
#pragma omp barrier

                /* Check Put data */
                /*
                char str[2048]; memset(str, 0, 2048);
                sprintf (str, "PUT CHECK: %d[%d]: ", rank, tid);
                for (ii = 0; ii < omp_threads * ranks * el_count; ii++) {
                    sprintf(str, "%s %lu %lu %lu %lu, ", str,
                            getSrcRank(put_data[ii]), getTid(put_data[ii]),
                            getDstRank(put_data[ii]), getEl(put_data[ii]));
                }
                printf ("%s\n", str);
                */

                offset = (tid * ranks * el_count);
                for (ii = 0; ii < ranks; ii++) {
                    for (jj = 0; jj < el_count; jj++) {
                        unsigned long expected = 0;
                        setSrcRank(expected, ii);
                        setTid(expected, tid);
                        setDstRank(expected, rank);
                        setEl(expected, jj);
                        if (expected != put_data[offset]) {
                            printf ("[Thread %d]: MPI_Put data error: expected: %lx, recv: %lx, at put_data offset = %d\n",
                                    tid, expected, put_data[offset], offset);
                            err++;
                        }
                        offset++;
                    }
                }

                for (dst = 0; dst < ranks; dst++) {
                    l_offset = (tid * ranks * ranks * el_count) + (dst * ranks * el_count);
                    offset = tid * ranks * el_count;
                    MPI_Get(&get_buffer[l_offset], ranks * el_count, MPI_UNSIGNED_LONG,
                            dst, offset, ranks * el_count, MPI_UNSIGNED_LONG,
                            wins[i]);
                }
                
               /* Synchronize gets */
#pragma omp barrier
                if (tid == 0)
                    MPI_Win_fence(0, wins[i]);
#pragma omp barrier

                /* Check Get data */
                /*
                memset(str, 0, 2048);
                sprintf (str, "GET CHECK: %d[%d]: ", rank, tid);
                for (ii = 0; ii < omp_threads * ranks * ranks * el_count; ii++) {
                    sprintf(str, "%s %lu %lu %lu %lu, ", str,
                            getSrcRank(get_buffer[ii]), getTid(get_buffer[ii]),
                            getDstRank(get_buffer[ii]), getEl(get_buffer[ii]));
                }
                printf ("%s\n", str);
                */

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
                            if (expected != get_buffer[ll_offset]) {
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
            MPI_Win_free(&wins[i]);
        }
    }

    if (!win_free) {
        for (i = 0; i < iter; i++) {
            MPI_Win_free(&wins[i]);
        }
    }

    MPI_Barrier(comm);
    
    free(wins);
    free(put_data);
    free(put_buffer);
    free(get_buffer);

    return SUCCESS;
}

int run_multi_window_test(void)
{
    MPI_Win **wins;
    MPI_Comm *comms;

    int dst, err = 0, i, ii, jj, kk, j, k, ntids, tid, mem_size, get_mem_size;

    if (rank == 0 ) {
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

#pragma omp parallel private (ntids, tid, i, j, k, dst, ii, jj, kk)
    {
        ntids = omp_get_num_threads();
        tid = omp_get_thread_num();

        mem_size = ranks * el_count * sizeof(unsigned long int);
        get_mem_size = mem_size * ranks;
        unsigned long *put_data = malloc(mem_size);
        unsigned long *put_buffer = malloc(mem_size);
        unsigned long *get_buffer = malloc(get_mem_size);

        for (i = 0; i < iter; i++) {

            MPI_Win_create(put_data, ranks * el_count * sizeof(unsigned long), sizeof(unsigned long), MPI_INFO_NULL, comms[tid], &wins[tid][i]);
        
            MPI_Win_fence(0, wins[tid][i]);
            
            if (datacheck) {
                memset(put_data, 0xff, mem_size);
                memset(put_buffer, 0xff, mem_size);
                memset(get_buffer, 0xff, get_mem_size);

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
                          
                        /* printf ("PUT: %d[%d] -> %d[%d]: %lu %lu %lu %lu, offset = %d, l_offset = %d\n", rank, tid, dst, tid,
                                    getSrcRank(put_buffer[_i]),
                                    getTid(put_buffer[_i]),
                                    getDstRank(put_buffer[_i]),
                                    getEl(put_buffer[_i]),
                                    offset + ii, _i); */
                          
                    }
                   
                    /* Put to rank dst */
                    MPI_Put(&put_buffer[l_offset], el_count, MPI_UNSIGNED_LONG,
                            dst, offset, el_count, MPI_UNSIGNED_LONG,
                            wins[tid][i]);
                }
                    
                /* Synchronize puts */
                MPI_Win_fence(0, wins[tid][i]);
#pragma omp barrier

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
                        if (expected != put_data[offset]) {
                            printf ("[Thread %d]: MPI_Put data error: expected: %lx, recv: %lx, at put_data offset = %d\n",
                                    tid, expected, put_data[offset], offset);
                            err++;
                        }
                        offset++;
                    }
                }

                /* Do MPI_Get() */
                for (dst = 0; dst < ranks; dst++) {
                    l_offset = dst * ranks * el_count;
                    MPI_Get(&get_buffer[l_offset], ranks * el_count, MPI_UNSIGNED_LONG,
                            dst, 0, ranks * el_count, MPI_UNSIGNED_LONG,
                            wins[tid][i]);
                }
                
                /* Synchronize gets */
                MPI_Win_fence(0, wins[tid][i]);
#pragma omp barrier

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
                            if (expected != get_buffer[l_offset]) {
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
                MPI_Win_free(&wins[tid][i]);
            }

        }

        free(put_data);
        free(put_buffer);
        free(get_buffer);

        if (!win_free) {
            for (j = 0; j < iter; j++) {
                MPI_Win_free(&wins[tid][j]);
            }
        }
    }

    free (wins);
    wins = NULL;

    for (i=0; i<omp_threads; i++) {
        MPI_Comm_free(&comms[i]);
    }

    free(comms);

    return SUCCESS;
}

int main(int argc, char *argv[])
{
    int ret, errs = 0;
    int provided, main_thread, claimed,
        ntids, tid, i, j, k;
    char *env = NULL;

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

    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &ranks);

    env = getenv("OMP_NUM_THREADS");
    omp_threads = atoi(env == NULL ? "1" : env);

    ret = process_args(argc, argv);
    if (ret != SUCCESS) {
        printf ("Failed to process arguments: %s\n", get_err_str(ret));
        return 0;
    }

    /* print current values */
    if (rank == 0) {
        printf ("\n");
        print_args(1);
    }


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

    if ( win_mode == SINGLE_WIN) {
        ret = run_single_window_test();
        if (ret != SUCCESS) {
           errs++;
           goto EXIT;
        }
    }
    else if (win_mode == MULTI_WIN) {
        ret = run_multi_window_test();
        if (ret != SUCCESS) {
            errs++;
            goto EXIT;
        }
    }
       
    MPI_Finalize();

EXIT:

    if (errs > 0) {
        printf ("Failed! Errs = %d\n", errs); fflush(stdout); 
    }
    else if (rank == 0) {
        printf ("Success!\n");
    }

    return errs;
}
