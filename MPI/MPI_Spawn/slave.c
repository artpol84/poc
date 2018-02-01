#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{ 
    int rank, size;
    int lsize, rsize;
    int grank, gsize;
    MPI_Comm parent, global;
    MPI_Init(&argc, &argv);
    // Locat info
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Global info
    MPI_Comm_get_parent(&parent);
    if (parent == MPI_COMM_NULL)
        error("No parent!");
    MPI_Comm_remote_size(parent, &size);

    MPI_Comm_size(parent, &lsize);
    MPI_Comm_remote_size(parent, &rsize);

    MPI_Intercomm_merge(parent, 1, &global);
    MPI_Comm_rank(global, &grank);
    MPI_Comm_size(global, &gsize);
    printf("child %d: lsize=%d, rsize=%d, grank=%d, gsize=%d\n",
           rank, lsize, rsize, grank, gsize);

    MPI_Barrier(global);

    printf("%d: after Barrier\n", grank);

    MPI_Comm_free(&global);

    MPI_Finalize();
    return 0;
} 
