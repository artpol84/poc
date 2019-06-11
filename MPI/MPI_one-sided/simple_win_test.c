#include <mpi.h>
#include <stdio.h>

int rank, size;

void win_interact()
{
    MPI_Win win;
    int *buf;
    int local[2] = { 0, 20 + rank + 1 };

    if(rank == 0){
        printf("Performing window interactoin ...\n");
    }

    MPI_Win_allocate(2 * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &buf, &win);
    buf[0] = 10 + 1 + rank;
    buf[1] = -1;
    MPI_Barrier(MPI_COMM_WORLD);

    if( rank < 2){
        int target = !rank;
        MPI_Put(&local[1], 1, MPI_INT, target, 1, 1, MPI_INT, win);
        MPI_Get(&local[0], 1, MPI_INT, target, 0, 1, MPI_INT, win);
    }
    MPI_Win_flush_all(win);

    if( rank < 2){
        int target = !rank;
        if( local[0] != (target + 10 + 1) ){
            printf("Error vrifying Get\n");
        }
        if( buf[1] != (target + 20 + 1) ){
            printf("Error vrifying Put\n");
        }
//        printf("%d: loc[0]=%d, loc[1] = %d, buf[0] = %d, buf[1] = %d\n",
//                rank, local[0], local[1], buf[0], buf[1]);
    }

    if(rank == 0){
        printf("\tDONE\n");
    }

    
    MPI_Win_free(&win);
    MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("Hello, im %d of %d\n", rank, size);

    win_interact();
    win_interact();
    
    MPI_Finalize();

    return 0;
}