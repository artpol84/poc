#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int num_unexp = 0, num_iter = 0, freeze = 0;
    int rank, j, i, buf, flag = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank > 2){
        if(rank == 0){
            printf("This test only supports 2 processes\n");
            MPI_Finalize();
            exit(0);
        }
    }

    if(argc < 3){
        num_unexp = 1;
        num_iter = 1;
    } else {
        num_unexp = atoi(argv[1]);
        num_iter = atoi(argv[2]);
    }
    if(argc >= 4) {
        freeze = atoi(argv[3]);
    }

    if( rank == 0 ){
        printf("Start unexpected test: iters=%d, sends=%d\n", num_iter, num_unexp);
    }

    for(j = 0; j < num_iter; j++){
        MPI_Request *reqs = calloc(num_unexp, sizeof(MPI_Request));
        if(rank == 0){
            for(i=0; i < num_unexp; i++) {
                MPI_Isend(&buf, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &reqs[i]);
            }
            struct timeval tv;
            gettimeofday(&tv, NULL);
            double start = tv.tv_sec + 1E-6 * tv.tv_usec, end;
            do{
                MPI_Testall(num_unexp, reqs, &flag, MPI_STATUSES_IGNORE);
                gettimeofday(&tv, NULL);
                end = tv.tv_sec + 1E-6 * tv.tv_usec;
            } while(!flag && ((end - start) < 5.0));
            MPI_Barrier(MPI_COMM_WORLD);
        } else{
            if ( 0 ) {
                int delay = 1;
                while(delay) {
                    sleep(1);
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
            for(i=0; i < num_unexp; i++) {
                MPI_Irecv(&buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &reqs[i]);
            }
        }
        MPI_Waitall(num_unexp, reqs, MPI_STATUSES_IGNORE);
    }

    while(freeze) {
        sleep(1);
    }

    MPI_Finalize();
}