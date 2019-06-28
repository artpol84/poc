#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>

int main(int argc, char *argv[])
{
    int ret, errs = 0;
    int provided, main_thread, claimed,
        ntids, tid, omp_threads, rank, ranks, i, j, k, itter;

    MPI_Win *wins;
    MPI_Comm *comms;

    char *env = getenv("OMP_NUM_THREADS");
    omp_threads = atoi(env == NULL ? "1" : env);

    if (argc > 1) {
        itter = atoi(argv[1]);
    }

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

    /* Get runtime threadid and treads count */
#pragma omp parallel private (ntids, tid)
    {
        ntids = omp_get_num_threads();
        tid =omp_get_thread_num();

        printf ("Hello from rank %d/%d and thread %d/%d\n", rank, ranks, tid, ntids);
        fflush(stdout);
#pragma omp barrier
    }

    MPI_Barrier(MPI_COMM_WORLD);
    sleep(1);
    if (rank == 0 && main_thread == true) {
        printf ("\n\n!!! Starting multi-threaded win_create test !!!\n\n");
        printf ("Testing %d win_create operations using %d threads.\n", itter, omp_threads);
    }

    wins = malloc(omp_threads * sizeof(MPI_Win));
    comms = malloc(omp_threads * sizeof(MPI_Comm));

    for (i=0 ; i < omp_threads; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

    /* Fill data buffer with put and get data for each thread */
//     for (i=0; i<omp_threads; i++) {
//         put_data[i] = (rank * 100) + i;
//         get_data[i] = ((rank * 100) + i) * -1;;
//     }

#pragma omp parallel private (ntids, tid, i, j, k)
    {
        ntids = omp_get_num_threads();
        tid = omp_get_thread_num();

        int* put_data = malloc(sizeof(int) * ranks);
        int* get_data = malloc(sizeof(int) * ranks);
        int* res      = malloc(sizeof(int) * ranks);
        for ( i=0; i<ranks; i++) {
            put_data[i] = -11111111;
            get_data[i] = -11111111;
        }

        for (i = 0; i < itter; i++) {
            MPI_Win_create(put_data, ranks * sizeof(int), sizeof(int), MPI_INFO_NULL, comms[tid], &wins[tid]);

//             for (k = 0; k < ntids; k++) {
//                 if (tid == k) {
                    MPI_Win_fence(0, wins[tid]);
//                     sleep(1);
//                 }
//                 #pragma omp barrier
//             }
//             #pragma omp barrier

            for (j = 0; j < ranks; j++) {
                /* Run Put/Get */
                res[j] = (rank * 1E6) + (j * 1E3) + tid; /* encode rank */
                printf ("/%d/ res = %09d to: %d at %d\n", i, res[j], j , rank);
                fflush(stdout);
                MPI_Put(&res[j], 1, MPI_INT, /*target rank*/ j, /*target offset*/ rank, 1, MPI_INT, wins[tid]);
            }
            
//             MPI_Get(&data[tid], 1, MPI_INT, src, );

            /* Check results */
//             for (k = 0; k < ntids; k++) {
//                 if (tid == k) {
                    MPI_Win_fence(0, wins[tid]);
//                     sleep(1);
//                 }
//                 #pragma omp barrier
//             }
//             MPI_Barrier(MPI_COMM_WORLD);
//             #pragma omp barrier

            for (k = 0; k < ntids; k++) {
                if (k == tid) {
                    j = 0;
                    for (j = 0; j < ranks; j++) {
                        /* check results */
                        int expected = (j * 1E6) + (rank*1E3) + tid;
                        if (put_data[j] != expected) {
                            printf ("/%d/ put_data[%d]: recvd:%09d, expected:%09d; ", i, j, put_data[j], expected );
                        }
                        else {
                            printf ("/%d/ put_data[%d]: %09d, ", i, j, put_data[j]);
                        }
                    }
                    printf ("\n");
                    fflush(stdout);
                }
                #pragma omp barrier
            }
            MPI_Win_free(&wins[tid]);
        }
        free(put_data);
        free(get_data);
        free(res);
    }

    free (wins); wins = NULL;

    for (i=0; i<omp_threads; i++) {
        MPI_Comm_free(&comms[i]);
    }
    
    free(comms);

    MPI_Finalize();

EXIT:

    if (errs > 0) {
        printf ("Errs = %d\n", errs); fflush(stdout); 
    }

    return errs;
}
