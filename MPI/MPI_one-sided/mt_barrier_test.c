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
        printf ("\n\n!!! Starting multi-threaded MPI_Barrier test !!!\n\n");
    }

    comms = malloc(omp_threads * sizeof(MPI_Comm));

    for (i=0 ; i < omp_threads; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

#pragma omp parallel private (ntids, tid, i)
    {
        ntids = omp_get_num_threads();
        tid = omp_get_thread_num();

        for (i = 0; i < itter; i++) {
            MPI_Barrier(comms[tid]);
        }
    }

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
