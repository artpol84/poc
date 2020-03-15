#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int buf[16];
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Type_vector(4, 2, 4, MPI_INT, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        int i;

        MPI_Send(buf, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

        for(i=0; i < 16; i++){
            buf[i] = i+1;
        }
        MPI_Isend(buf, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        int i;
        MPI_Recv(buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(buf,8, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received: ");
        for(i=0; i<8; i++){
            printf("%d ", buf[i]);
        }
        printf("\n");
    }
    
    MPI_Finalize();
}