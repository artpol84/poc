#include <stdio.h>
#include <mpi.h>
#include "utils.h"

#ifndef START_IDX
#define START_IDX 2
#endif

message_desc_t scenario[] = {
    { 1, 4, {2}, {4} }
};

int main(int argc, char **argv)
{
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    message_t *m = NULL;
    char *recv_buf, sync[1];
    int i;

    m = message_init(NULL, 0, 0, START_IDX, scenario, sizeof(scenario)/sizeof(scenario[0]));
    ALLOC(recv_buf, m->outlen);
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Type_indexed(m->nblocks, m->blens, m->displs, MPI_CHAR, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        MPI_Send(sync, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Isend(m->base_addr, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        int i;
        MPI_Recv(sync, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(recv_buf, m->outlen, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received: ");
        for(i=0; i < m->outlen; i++){
            if( recv_buf[i] != m->outbuf[i] ){
                printf("Message mismatch in offset=%d, expect '%c', got '%c'\n",
                        i, m->outbuf[i], recv_buf[i]);
            }
            printf("%c ", recv_buf[i]);
        }
        printf("\n");
    }
    
    MPI_Finalize();
}