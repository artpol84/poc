#include <stdio.h>
#include <mpi.h>
#include "utils.h"

message_desc_t scenario[] = {
    { 3, 2, {2, 4, 1}, {4, 8, 3}},
    { 1, 1, {4}, {4} },
    { 2, 4, {2, 4}, {4, 8}},
    { 1, 1, {8}, {8} }
};


int main(int argc, char **argv)
{
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    char sync[1];
    message_t *m = message_init(NULL, 0, 0, scenario, sizeof(scenario)/sizeof(scenario[0]));
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Type_indexed(m->nblocks, m->blens, m->displs, MPI_CHAR, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        MPI_Send(sync, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Isend(m->base_addr, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        char *recv_buf;
        int i;
        MPI_Recv(sync, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        ALLOC(recv_buf, m->outlen);
        memset(recv_buf, 'x', m->outlen);

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