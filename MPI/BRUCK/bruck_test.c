#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <malloc.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#define BUF_LEN 5

#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)

int intcmp(const void *param1, const void *param2)
{
    return *(int *)param1 - *(int *)param2;
}

int least_sign_bit(int number)
{
    unsigned int number_wo_lsb = number & (number - 1);
    return number - number_wo_lsb;
}

int uncutten_print(int rank, int *mass, int size, char *start_string)
{
    int index;
    char *tmp_buf;

    tmp_buf = malloc(size * BUF_LEN);
    
    memset(tmp_buf, '\0', size * BUF_LEN);
    for(index = 0; index < size; index ++) {
        sprintf(tmp_buf, "%s %d", tmp_buf, mass[index]);
    }

    debug_print("rank - %d %s = %s\n", rank, start_string, tmp_buf);
    free(tmp_buf);
    return 0;
}

int* bruck_allgather(int rank,int node_count, int *block, int block_size, int *buffer_size)
{
    int remain, lsb, steps;
    int *buffer = NULL;
    int *buffer_recv = NULL;
    int *buffer_send = NULL;
    double tmp;
    int offset = 0;
    int index, step;
    int flag;
    int recv_size = 0;
    int send_size = 0;
    int send_rank, recv_rank;
    MPI_Status status[2];
    MPI_Request req[2];

    tmp = log2(node_count);
    steps = (int) tmp;
    debug_print("rank %u - step = %u block_size = %d\n", rank, steps, block_size);
    tmp = 1 << (int)tmp;
    remain = node_count - ((int) tmp);
    debug_print("rank %u - remain = %u\n", rank, remain);
    lsb = least_sign_bit(remain);
    debug_print("rank %u - lsb = %u\n", rank, lsb);
    remain = remain - lsb;
    debug_print("rank %u - remain = %u\n", rank, remain);

    debug_print("rank - %d buffer = %p\n",rank, buffer);
    buffer = malloc(block_size * sizeof(int));
    debug_print("rank - %d buffer= %p\n",rank, buffer);
    memcpy(buffer, block, block_size * sizeof(int));

    uncutten_print(rank, buffer, block_size, "buffer");

    *buffer_size = block_size;
    for (step = 0; step < steps; step++){
        send_size = *buffer_size;
        buffer_send = malloc((send_size) * sizeof(int));
        memcpy(buffer_send, buffer, send_size * sizeof(int));

        if (remain & (1 << step)) {
            send_size = send_size + 1;
            buffer_send = realloc(buffer_send, send_size * sizeof(int));
            buffer_send[send_size - 1] = offset;
            debug_print("rank - %d append offset = %d\n", rank, buffer_send[send_size - 1]);
        }

        send_rank = (rank + node_count - (1 << step)) % node_count;
        recv_rank = (rank + (1 << step)) % node_count;
        MPI_Isend(buffer_send, send_size, MPI_INT, send_rank, 0, MPI_COMM_WORLD, &req[0]);
        flag = 0;
        while(!flag){
            MPI_Iprobe(recv_rank, 0, MPI_COMM_WORLD, &flag, &status[0]);
        }
        MPI_Get_count(&status[0], MPI_INT, &recv_size);
        buffer_recv = malloc(recv_size * sizeof(int));
        MPI_Irecv(buffer_recv, recv_size, MPI_INT, recv_rank, 0, MPI_COMM_WORLD, &req[1]);
        MPI_Waitall(2, req, status);
        uncutten_print(rank, buffer_recv, recv_size, "recv_buffer");

        if (lsb & (1 << step)) {
            offset = *buffer_size;
            debug_print("rank - %d offset = %d\n", rank, offset);
        }

        if (remain & (1 << step)) {
            offset = buffer_recv[recv_size - 1] + *buffer_size;
            debug_print("rank - %d expand offset = %d\n", rank, buffer_recv[recv_size - 1]);
            recv_size = recv_size - 1;
        }

        buffer = realloc(buffer, (*buffer_size + recv_size) * sizeof(int));
        memcpy(buffer + (*buffer_size), buffer_recv, recv_size * sizeof(int));
        *buffer_size = *buffer_size + recv_size;
        free(buffer_send);
        free(buffer_recv);
    }

    if (offset) {
        debug_print("rank - %d offset<>0 and = %d\n", rank, offset);
        send_rank = (rank + node_count - (1 << steps)) % node_count;
        recv_rank = (rank + (1 << steps)) % node_count;
        MPI_Isend(buffer, offset, MPI_INT, send_rank, 0, MPI_COMM_WORLD, &req[0]);
        flag = 0;
        while	(!flag) {
            MPI_Iprobe(recv_rank, 0, MPI_COMM_WORLD, &flag, &status[0]);
        }
        MPI_Get_count(&status[0], MPI_INT, &recv_size);
        buffer_recv = malloc(recv_size * sizeof(int));
        MPI_Irecv(buffer_recv, recv_size,MPI_INT, recv_rank, 0, MPI_COMM_WORLD, &req[1]);
        MPI_Waitall(2, req, status);
        buffer = realloc(buffer, (*buffer_size + recv_size) * sizeof(int));
        memcpy(buffer + (*buffer_size), buffer_recv, recv_size * sizeof(int));
        *buffer_size = *buffer_size + recv_size;
        free(buffer_recv);
    }

    return buffer;
}

int main(int argc, char **argv)
{
    int index;
    int rank, size;
    int seed = 0;
    int block_size = 0;
    int *block  = NULL;
    int *buffer = NULL;
    int *buffer_mpi = NULL;
    int *recvcounts_mpi = NULL;
    int *disp_mpi = NULL;
    int buffer_size = 0;
    int buffer_mpi_size = 0;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf("I`m  node - %d of %d\n",rank, size);

    seed = (time(NULL)) + rank;
    block_size = rand_r(&seed);
    block_size = block_size % 10 + 2;
    debug_print("rank %d - block size = %d\n", rank, block_size);

    block = malloc(block_size * sizeof(int));
    for(index = 0; index < block_size; index ++)
        block[index] = rank;

    uncutten_print(rank, block, block_size, "block");

    debug_print("rank - %d buffer = %p\n", rank, buffer);
    buffer = bruck_allgather(rank, size, block, block_size, &buffer_size);

    uncutten_print(rank, buffer, buffer_size, "rezult buffer");

    MPI_Allreduce(&block_size, &buffer_mpi_size, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    debug_print("rank - %d buffer_size = %d buffer_mpi_size = %d \n", rank, 
                buffer_size, buffer_mpi_size);
    assert(buffer_size == buffer_mpi_size);
    buffer_mpi = malloc(buffer_mpi_size * sizeof(int));
    recvcounts_mpi = calloc(size, sizeof(int));
    disp_mpi = calloc(size, sizeof(int));

    MPI_Allgather(&block_size, 1, MPI_INT, recvcounts_mpi, 1, MPI_INT, MPI_COMM_WORLD);
    uncutten_print(rank, recvcounts_mpi, size, "recvcounts_mpi");

    disp_mpi[0] = 0;
    for(index = 1; index < size; index ++) {
        disp_mpi[index] = disp_mpi[index - 1] + recvcounts_mpi[index - 1];
    }
    uncutten_print(rank, disp_mpi, size, "disp_mpi");

    MPI_Allgatherv(block, block_size, MPI_INT, buffer_mpi, recvcounts_mpi,
                    disp_mpi, MPI_INT, MPI_COMM_WORLD);
    uncutten_print(rank, buffer_mpi, buffer_mpi_size, "result mpi buffer");

    qsort(buffer, buffer_size, sizeof(int), intcmp);
    qsort(buffer_mpi, buffer_mpi_size, sizeof(int), intcmp);
    uncutten_print(rank, buffer, buffer_size, "sorted buffer");
    uncutten_print(rank, buffer_mpi, buffer_mpi_size, "sorted buffer_mpi");
    assert(!memcmp(buffer, buffer_mpi, buffer_size));

    free(block);
    free(buffer);
    free(buffer_mpi);
    free(recvcounts_mpi);
    free(disp_mpi);
    MPI_Finalize();
}
