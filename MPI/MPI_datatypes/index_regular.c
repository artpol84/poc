#include <stdio.h>
#include <mpi.h>

#ifndef START_IDX
#define START_IDX 16
#endif

int main(int argc, char **argv)
{
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    char buf[32], sync[1];
    int count = sizeof(buf)/4;
    int blocklens[count];
    int displs[count];
    int i;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    char *base_addr = &buf[START_IDX];

    for(i=0; i < count; i++) {
        blocklens[i] = 2;
        displs[i] = &buf[i * 4] - base_addr;
    }

    MPI_Type_indexed(count, blocklens, displs, MPI_CHAR, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        MPI_Send(sync, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        for(i=0; i < sizeof(buf); i++){
            buf[i] = i % ('Z' - 'A' + 1) + 'A';
        }
        MPI_Isend(base_addr, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        int i;
        MPI_Recv(sync, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for(i=0; i < sizeof(buf); i++){
            buf[i] = 'x';
        }

        MPI_Recv(buf, 2 * count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received: ");
        for(i=0; i < count * 2; i++){
            printf("%c ", buf[i]);
        }
        printf("\n");
    }
    
    MPI_Finalize();
}