#include <stdio.h>
#include <mpi.h>
#include <string.h>

#define NITER (1024)
#define WINDOW 16
#define NBLOCKS (16 * 1024)
#define BSIZE 64
#define STRIDE 128

char inbuf[WINDOW][NBLOCKS * STRIDE];
char outbuf[WINDOW][NBLOCKS * STRIDE];

int main(int argc, char **argv)
{
    MPI_Datatype types[WINDOW];
    int rank, size, peer;
    MPI_Request req[2 * WINDOW];
    int i, cntr = 0;;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    peer = (rank + (size/2)) % size;
    printf("rank = %d: send/recv to/from %d\n", rank, peer);

    /* Wireup the connection */
    MPI_Irecv(inbuf, 1, MPI_INT, peer, 0, MPI_COMM_WORLD, &req[0]);
    MPI_Send(outbuf, 1, MPI_INT, peer, 0, MPI_COMM_WORLD);
    MPI_Wait(&req[0], MPI_STATUS_IGNORE);

    for(i=0; i < WINDOW; i++) {
        int j;
        for(j = 0; j < NBLOCKS * STRIDE; j++) {
            outbuf[i][j] = cntr++;
        }
    }

    for(i=0; i < NITER; i++) {
        int j, k;

        memset(inbuf, 0, sizeof(inbuf));
        
#if 0
if(rank == 0) {
    static int delay = 1;
    while(delay) {
        sleep(1);
    }
}
#endif

        MPI_Barrier(MPI_COMM_WORLD);

        for(j = 0; j < WINDOW; j++) {
            MPI_Type_vector(NBLOCKS, BSIZE, STRIDE, MPI_CHAR, &types[j]);
            MPI_Type_commit(&types[j]);
            MPI_Isend(outbuf[j], 1, types[j], peer, 1, MPI_COMM_WORLD, &req[2*j]);
            MPI_Irecv(inbuf[j], 1, types[j], peer, 1, MPI_COMM_WORLD, &req[2*j + 1]);
        }

        MPI_Waitall(2 * WINDOW, req, MPI_STATUSES_IGNORE);

        /* Verify */
        for(j = 0; j < WINDOW; j++) {
            int k, l;
            MPI_Type_free(&types[j]);
            for(k = 0; k < NBLOCKS; k++){
                int offs = k * STRIDE;
                for(l = 0; l < BSIZE; l++) {
                    if( inbuf[j][offs + l] != outbuf[j][offs + l] ) {
                        printf("Consistency failure: iter=%d, win_idx=%d, block #%d, boffs=%d, offs=%d: expect %hhx, got %hhx\n",
                            i, j, k, offs, l, outbuf[j][offs + l], inbuf[j][offs + l]);
                    }
                }
            }
        }
    }

    MPI_Finalize();
}