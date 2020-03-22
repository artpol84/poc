#include <stdio.h>
#include <mpi.h>
#include "utils.h"

#include <time.h>
#define GET_TS() ({ \
    struct timespec ts;                     \
    double ret;                             \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9 * ts.tv_nsec;    \
    ret;                                    \
})

void measure_cycle(int wsize, MPI_Datatype types[], message_t *msgs[],
                   char *recv_bufs[], int icnt)
{
    int rank;
    char sync[1];
    MPI_Request reqs[wsize];
    int i, j;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* warmup */
    if(0 == rank) {
        for(i = 0; i < icnt; i++) {
            for(j = 0; j < wsize; j++) {
                MPI_Isend(msgs[j]->base_addr, 1, types[j], 1, 0, MPI_COMM_WORLD, &reqs[j]);
            }
            MPI_Waitall(wsize, reqs, MPI_STATUSES_IGNORE);
            MPI_Recv(sync, 1, MPI_CHAR, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        for(i = 0; i < icnt; i++) {
            for(j = 0; j < wsize; j++) {
                MPI_Irecv(recv_bufs[j], msgs[j]->outlen, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &reqs[j]);
            }
            MPI_Waitall(wsize, reqs, MPI_STATUSES_IGNORE);
            MPI_Send(sync, 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char **argv)
{
    int wsize, rank;
    int warmups = 10;
    int iterations = 100;

    message_desc_t scenario[] = {
        { 1, 1, { 30*8 }, {256*8} },
        { 1, 23, {30*8}, {60*8} },

    };

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(argc < 2) {
        if(rank == 0){
            printf("Need window size as input\n");
        }
        MPI_Finalize();
        return 0;
    }
    wsize = atoi(argv[1]);
    message_t *msgs[wsize];
    MPI_Datatype types[wsize];
    char *recv_bufs[wsize];
    int i;

    for(i = 0; i < wsize; i++){
        create_mpi_index(0, NULL, 0, 0, 0, scenario, sizeof(scenario)/sizeof(scenario[0]),
                &types[i], &msgs[i]);
        ALLOC(recv_bufs[i], msgs[i]->outlen);
    }

    /* warmup */
    measure_cycle(wsize, types, msgs, recv_bufs, warmups);

    double start = GET_TS();
    measure_cycle(wsize, types, msgs, recv_bufs, iterations);
    double end = GET_TS();

    if(0 == rank) {
        printf("latency = %lf\n", (end - start)/iterations * 1E6);
    }

    MPI_Finalize();
}
