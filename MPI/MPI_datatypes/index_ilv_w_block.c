#include <stdio.h>
#include <mpi.h>

#ifndef BUF_SIZE
#define BUF_SIZE 32
#endif

int main(int argc, char **argv)
{
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    char buf1[BUF_SIZE], buf2[BUF_SIZE], buf3[BUF_SIZE], buf4[2] = { '@', '#' }, sync[1];
    int count = 3 * (BUF_SIZE/4) + 1;
    int payload_size = 2;
    int stride = 4;
    int recv_size = count * payload_size;
    int blocklens[count];
    int displs[count];
    char recv_buf[recv_size];
    int i;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    char *base_addr = buf1;

    for(i=0; i < count - 1; i += 3) {
        blocklens[i + 0] = payload_size;
        displs[i + 0]    = &buf1[(i/3) * stride] - base_addr;
        blocklens[i + 1] = 2;
        displs[i + 1]    = &buf2[(i/3) * stride] - base_addr;
        blocklens[i + 2] = 2;
        displs[i + 2]    = &buf3[(i/3) * stride] - base_addr;
    }
    blocklens[count - 1] = 2;
    displs[count - 1]    = buf4 - base_addr;

    MPI_Type_indexed(count, blocklens, displs, MPI_CHAR, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        MPI_Send(sync, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        for(i=0; i < BUF_SIZE; i++){
            buf1[i] = i % ('Z' - 'A' + 1) + 'A';
            buf2[i] = i % ('z' - 'a' + 1) + 'a';
            buf3[i] = i % ('9' - '0' + 1) + '0';
        }
        MPI_Isend(base_addr, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        int i;
        MPI_Recv(sync, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for(i=0; i < recv_size; i++){
            recv_buf[i] = 'x';
        }

        MPI_Recv(recv_buf, recv_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received: ");
        for(i=0; i < recv_size; i++){
            printf("%c ", recv_buf[i]);
        }
        printf("\n");
    }
    
    MPI_Finalize();
}