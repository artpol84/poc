#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

int myrank = -1;

int get_pmix_rank()
{
    if( myrank < 0 ){
        char *ptr = getenv("PMIX_RANK");
        myrank = atoi(ptr);
    }
    return myrank;
}

void do_freeze_detail(int want)
{
    if(0 != get_pmix_rank()){
        return;
    }
    int break_flg = 0;
    while(want && !break_flg) {
        sleep(1);
    }
}


int main(int argc, char **argv)
{
    int num_unexp = 0, num_iter = 0, freeze = 0, freeze_detail = 0;
    int rank, size;
    int j, i, buf, flag = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if( size%2 != 0){
        if(rank == 0){
            printf("This test only supports even number of processes\n");
            MPI_Finalize();
            exit(0);
        }
        MPI_Finalize();
        exit(0);
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

    if(argc >= 5) {
        freeze_detail = atoi(argv[4]);
    }

    if( rank == 0 ){
        printf("Start unexpected test: iters=%d, sends=%d; freeze=%d, freeze_det=%d\n",
               num_iter, num_unexp, freeze, freeze_detail);
    }

    do_freeze_detail(freeze_detail);

    int my_peer = (rank + size/2) % size;
    for(j = 0; j < num_iter; j++){
        MPI_Request *reqs = calloc(num_unexp, sizeof(MPI_Request));
        if(rank < (size/2)){
            for(i=0; i < num_unexp; i++) {
                MPI_Isend(&buf, 1, MPI_INT, my_peer, 0, MPI_COMM_WORLD, &reqs[i]);
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
        } else {
            MPI_Barrier(MPI_COMM_WORLD);
            for(i=0; i < num_unexp; i++) {
                MPI_Irecv(&buf, 1, MPI_INT, my_peer, 0, MPI_COMM_WORLD, &reqs[i]);
            }
        }
        MPI_Waitall(num_unexp, reqs, MPI_STATUSES_IGNORE);
        do_freeze_detail(freeze_detail);
    }

    while(freeze) {
        sleep(1);
    }

    MPI_Finalize();
}