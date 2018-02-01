/* manager */ 
#include <mpi.h> 
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) 
{ 
    int rank, size;
    int universe_size, *universe_sizep, flag;
    int lsize, rsize;
    int grank, gsize;
    MPI_Comm everyone, global;           /* intercommunicator */
    char worker_program[100];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE,  
                &universe_sizep, &flag);  
    if (!flag) { 
        universe_size = 8;
    } else 
        universe_size = *universe_sizep; 

    if( rank == 0 ) {    
        printf("univ size = %d\n", universe_size);
    }

    sprintf(worker_program, "./slave");
    MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, 6,
                   MPI_INFO_NULL, 0, MPI_COMM_WORLD, &everyone,
                   MPI_ERRCODES_IGNORE);

    MPI_Comm_size(everyone, &lsize);
    MPI_Comm_remote_size(everyone, &rsize);

    MPI_Intercomm_merge(everyone, 1, &global);
    MPI_Comm_rank(global, &grank);
    MPI_Comm_size(global, &gsize);
    printf("parent %d: lsize=%d, rsize=%d, grank=%d, gsize=%d\n",
           rank, lsize, rsize, grank, gsize);

    MPI_Barrier(global);

    printf("%d: after Barrier\n", grank);

    MPI_Comm_free(&global);


    MPI_Finalize();
    return 0;
}
