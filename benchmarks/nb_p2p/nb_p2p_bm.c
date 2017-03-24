/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2017      Mellanox Technologies. All rights reserved.
 *
 * Non-blocking p2p benchmark
 */

#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

#define NITERS 1000
#define MSIZE (32*1024)

int main(int argc, char *argv[])
{
    int rank, size, send_size, recv_size, *next, *prev;
    char **smsg[NITERS], **rmsg[NITERS];
    MPI_Request *sreqs, *rreqs;
    int i;
    double time, time_max, time_min, time_avg;

    int param_reuse= 0, param_barrier = 0;
    int param_even = 0;

    /* Start up MPI */

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for( i = 1; i < argc; i++){
        if( !strcmp("--reuse-buf", argv[i]) ){
            param_reuse = 1;
        } else if( !strcmp("--barrier", argv[i]) ){
            param_barrier = 1;
        } else if( !strcmp("--even-exch", argv[i]) ){
            param_barrier = 1;
        }
    }

    sreqs = calloc(size, sizeof(*sreqs));
    rreqs = calloc(size, sizeof(*rreqs));
    for(i = 0; i < size; i++){
        sreqs[i] = MPI_REQUEST_NULL;
        rreqs[i] = MPI_REQUEST_NULL;
    }
    
    /* Calculate neighbors for exchange */
    if( param_even ){
        recv_size = send_size = (size / 10) ? (size / 10) : 1;
        next = calloc(send_size, sizeof(*next));
        prev = calloc(recv_size, sizeof(*prev));
        for(i=0; i < send_size; i++){
            next[i] = (rank + i + 1) % size;
            prev[i] = (rank + size - (i + 1)) % size;
        }
    } else {
        int half_set = size/2;
        int to_process = (half_set / 10) ? (half_set / 10) : 1;
        if( rank < half_set ){
            send_size = to_process;
            recv_size = 0;
            next = calloc(send_size, sizeof(*next));
            for(i=0; i < send_size; i++){
                next[i] = (rank + i) % half_set + half_set;
            }
        } else {
            recv_size = to_process;
            send_size = 0;
            prev = calloc(recv_size, sizeof(*prev));
            for(i=0; i < recv_size; i++){
                int lrank = rank - half_set;
                prev[i] = (lrank + half_set - i) % half_set;
            }
        }
    }
/*
    // DEBUG
    char output[1024] = "";
    sprintf(output, "%02d: send: ", rank);
    for(i=0; i<send_size;i++){
        sprintf(output,"%s %d", output, next[i]);
    }
    sprintf(output,"%s; rcv: ", output);
    for(i=0; i<recv_size;i++){
        sprintf(output,"%s %d", output, prev[i]);
    }
    printf("%s;\n", output);
    
    MPI_Finalize();
    exit(0);
*/


{
    int delay = 1;
    while(delay){
        sleep(1);
    }
}
    for(i=0; i< NITERS; i++){
        int j;
        
        /* Prepare send buffers */
        smsg[i] = calloc(send_size, sizeof(*smsg[i]));
        for(j=0; j < send_size; j++){
            if( !param_reuse ){
                /* New buffer for each send */
                smsg[i][j] = calloc(MSIZE, sizeof(**smsg[i]));
            } else {
                if( i == 0 ) {
                    smsg[0][j] = calloc(MSIZE, sizeof(**smsg[i]));
                 } else {
                    smsg[i][j] = smsg[0][j];
                }
            }
        }

        /* Prepare recv buffers */
        rmsg[i] = calloc(recv_size, sizeof(*rmsg[i]));
        for(j=0; j < recv_size; j++){
            if( !param_reuse ){
                /* New buffer for each send */
                rmsg[i][j] = calloc(MSIZE, sizeof(**rmsg[i]));
            } else {
                if( i == 0 ) {
                    rmsg[0][j] = calloc(MSIZE, sizeof(**rmsg[i]));
                 } else {
                    rmsg[i][j] = rmsg[0][j];
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    time = GET_TS();
    for(i = 0; i < NITERS; i++){
        int j;
        char result[MSIZE] = { 0 };
        
        if( param_barrier ){
            MPI_Barrier(MPI_COMM_WORLD);
        }
        /* schedule all receives */
        for(j = 0; j < recv_size; j++){
            MPI_Irecv(rmsg[i][j], MSIZE, MPI_CHAR, prev[j], 0, MPI_COMM_WORLD, &rreqs[prev[j]]);
        }
        
        /* schedule all sends */
        for(j=0; j<send_size; j++){
            memset(smsg[i][j], i, MSIZE);
            MPI_Isend(smsg[i][j], MSIZE, MPI_CHAR, next[j], 0, MPI_COMM_WORLD, &sreqs[next[j]]);
        }
        
        for(j=0; j<recv_size; j++){
            int idx, k, ridx = -1;
            MPI_Status st;
            MPI_Waitany(size, rreqs, &idx, &st);
            assert( idx != MPI_UNDEFINED);
            /* find index in receive buffer */
            for(k=0; k<recv_size; k++){
                if( prev[k] == idx ){
                    ridx = k;
                    break;
                }
            }
            assert(0 <= ridx);
            for(k = 0; k < MSIZE; k++){
                result[k] += rmsg[i][ridx][k];
            }
        }
        MPI_Waitall(size, sreqs, MPI_STATUSES_IGNORE);
    }
    time = GET_TS() - time;
    MPI_Barrier(MPI_COMM_WORLD);

    if( rank == 0 ){
        printf("Time is %lf s\n", time);
    }

    /* All done */
    MPI_Finalize();
    return 0;
}
