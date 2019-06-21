#include <omp.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int errs = 0;
    int provided, flag, claimed,
        ntids, tid, rank, size;

    MPI_Win *win;
    MPI_Comm *comms;

    MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);
//     MPI_Init_thread(0, 0, MPI_THREAD_SINGLE, &provided);

    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &size);

    MPI_Is_thread_main ( &flag );
    if (!flag) {
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

    char *env = getenv("OMP_NUM_THREADS");
    int threads = atoi(env == NULL ? "1" : env);

    win = malloc(threads * sizeof(MPI_Win));
    comms = malloc(threads * sizeof(MPI_Comm));

    int i = 0;
    for (i=0 ; i < threads; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }


    int win_data = 0;
#pragma omp parallel private (ntids, tid)
    {
        ntids = omp_get_num_threads();
        tid = omp_get_thread_num();

        int i = 0;
        for (i=0; i<1000; i++) {
            MPI_Win_create(&win_data, sizeof(int), 0, MPI_INFO_NULL, comms[tid], &win[tid]);
//             MPI_Win_fence(0, win[tid]);
            MPI_Win_free(&win[tid]);
        }
    }

    free (win); win = NULL;

    for (i=0; i<threads; i++) {
        MPI_Comm_free(&comms[i]);
    }

    free(comms);

    MPI_Finalize();

    return errs;
}
