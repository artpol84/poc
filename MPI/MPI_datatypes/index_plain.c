#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    char buf1[] = { 'A' }, buf2[] = { 'B', 'B' }, buf3[] = { 'C', 'C', 'C', 'C' },
                    buf4[] = { 'D', 'd', 'D', 'd','D', 'd' };
    int blocklens[] = { sizeof(buf1), sizeof(buf2), sizeof(buf3), sizeof(buf4) };
    int rcv_size = sizeof(buf1) + sizeof(buf2) + sizeof(buf3) + sizeof(buf4);
    char rcv_buf[rcv_size];
    int displs[] = {0, buf2 - buf1, buf3 - buf1, buf4 - buf1 };
    
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Type_indexed(4, blocklens, displs, MPI_CHAR, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        int i;
        MPI_Send(buf1, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Isend(buf1, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        int i;
        MPI_Recv(rcv_buf, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(rcv_buf, rcv_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received: ");
        for(i=0; i < rcv_size; i++){
            printf("%c ", rcv_buf[i]);
        }
        printf("\n");
    }
    
    MPI_Finalize();
}