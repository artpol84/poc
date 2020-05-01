/*
Copyright 2020 Artem Y. Polyakov <artpol84@gmail.com>
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without specific
prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

#include <time.h>
#define GET_TS() ({ \
    struct timespec ts;                     \
    double ret;                             \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9 * ts.tv_nsec;    \
    ret;                                    \
})

#define MAX_DESC_CNT    16
#define MAX_DESC_LEN    1024
#define WINDOW_SIZE     1
#define MAX_WIN_SIZE    1000
#define WARMUPS         100
#define ITERATIONS      1000

void measure_cycle(int wsize,
                   MPI_Datatype types[], message_t *msgs[], char *recv_bufs[],
                   MPI_Datatype types_split[], message_t *msgs_split[], char *recv_bufs_split[],
                   int split_mode, int icnt)
{
    int size, rank, peer;
    MPI_Request reqs[4*wsize];
    int i, j;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    peer = (rank + size / 2) % size;

    for(i = 0; i < icnt; i++) {
        for(j = 0; j < wsize; j++) {
            MPI_Irecv(recv_bufs[j], msgs[j]->outlen, MPI_CHAR, peer, i*MAX_WIN_SIZE+j, MPI_COMM_WORLD, &reqs[j]);
        }
        if (split_mode) {
            for(j = 0; j < wsize; j++) {
                MPI_Irecv(recv_bufs_split[j], msgs_split[j]->outlen, MPI_CHAR, peer, 2*i*MAX_WIN_SIZE+j, MPI_COMM_WORLD, &reqs[2*wsize+j]);
            }
        }
        for(j = 0; j < wsize; j++) {
            MPI_Isend(msgs[j]->base_addr, 1, types[j], peer, i*MAX_WIN_SIZE+j, MPI_COMM_WORLD, &reqs[wsize+j]);
        }
        if (split_mode) {
            for(j = 0; j < wsize; j++) {
                MPI_Isend(msgs_split[j]->base_addr, 1, types_split[j], peer, 2*i*MAX_WIN_SIZE+j, MPI_COMM_WORLD, &reqs[3*wsize+j]);
            }
        }
        if (split_mode) {
            MPI_Waitall(4*wsize, reqs, MPI_STATUSES_IGNORE);
        } else {
            MPI_Waitall(2*wsize, reqs, MPI_STATUSES_IGNORE);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

void usage(int rank, int argc, char **argv) {
    if (rank == 0) {
        fprintf(stderr, "Usage: %s [-h|-d <datatype description>|-i <iterations>|-s <umr repcount>|-w <window size>|-x <warmups>]\n", argv[0]);
        fprintf(stderr, "    -h                           this help page\n");
        fprintf(stderr, "    -d <datatype description>    format has to be <interleave>:<repcount>:<block>:<stride>\n");
        fprintf(stderr, "                                 multiple of them can be cancatenated\n");
        fprintf(stderr, "    -i <iterations>              number of iterations\n");
        fprintf(stderr, "    -s <umr repcount>            split mode, <umr repcount> is the <repcount> for UMR portion\n");
        fprintf(stderr, "    -w <window size>             window size\n");
        fprintf(stderr, "    -x <warmups>                 number of warmups\n");
    }
}

static int message_desc_init(char dt_desc[MAX_DESC_CNT][MAX_DESC_LEN], 
                             message_desc_t *scenario, 
                             message_desc_t *scenario_split,
                             int umr_split, int *msg_size, int desc_cnt) {
    int i;
    char *token;

    for (i = 0; i < desc_cnt; i++) {
        token = strtok(dt_desc[i], ":");
        if (token != NULL) {
            scenario[i].ilv = atoi(token);
            scenario_split[i].ilv = scenario[i].ilv;
        }
        else return -1;

        token = strtok(NULL, ":");
        if (token != NULL) {
            if (umr_split > 1) {
                scenario[i].rcnt = umr_split;
                scenario_split[i].rcnt = atoi(token) - umr_split;
            } else {
                scenario[i].rcnt = atoi(token);
                scenario_split[i].rcnt = 0;
            }
        }
        else return -1;

        /* FIXME: need to be able to handle multiple payloads and multiple strides */
        token = strtok(NULL, ":");
        if (token != NULL) {
            scenario[i].payloads[0] = atoi(token);
            scenario_split[i].payloads[0] = scenario[i].payloads[0];
        }
        else return -1;

        token = strtok(NULL, ":");
        if (token != NULL) {
            scenario[i].strides[0] = atoi(token);
            scenario_split[i].strides[0] = scenario[i].strides[0];
        }
        else return -1;

        if (scenario[i].ilv <= 0 || scenario[i].rcnt <= 0 || scenario_split[i].rcnt < 0 ||
            scenario[i].payloads[0] <= 0 || scenario[i].strides[0] <= 0) {
            return -1;
        }

        *msg_size += scenario[i].rcnt * scenario[i].payloads[0] +
                     scenario_split[i].rcnt * scenario_split[i].payloads[0];
    }

    return 0;
}

int main(int argc, char **argv)
{
    int opt, rc, size, rank;
    int wsize = WINDOW_SIZE;
    int warmups = WARMUPS;
    int iterations = ITERATIONS;
    int umr_split = 0;
    int split_mode = 0;
    int desc_cnt = 0;
    int msg_size = 0;
    char dt_desc[MAX_DESC_CNT][MAX_DESC_LEN];
    message_desc_t *scenario, *scenario_split;

    /* example
    message_desc_t scenario[] = {
        { 1, 1,  {30*8}, {256*8} },
        { 1, 23, {30*8}, {60*8} },
    };
    */

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size%2 != 0) {
        if (rank == 0) {
            fprintf(stderr, "need to have even number of ranks\n");
        }
        usage(rank, argc, argv);
        MPI_Finalize();
        return -1;
    }

    memset(dt_desc, '\0', sizeof(dt_desc));

    while ((opt = getopt(argc, argv, ":hd:i:s:w:x:")) != -1) {
        switch (opt) {
            case 'd':
                desc_cnt++;
                if (desc_cnt > MAX_DESC_CNT) {
                    if (rank == 0) {
                        fprintf(stderr, "too many datatypes provided (>%d)\n", MAX_DESC_CNT);
                    }
                    usage(rank, argc, argv);
                    MPI_Finalize();
                    return -1;
                }
                if (strlen(optarg) >= MAX_DESC_LEN) {
                    if (rank == 0) {
                        fprintf(stderr, "datatype description too long (>%d)\n", MAX_DESC_LEN);
                    }
                    usage(rank, argc, argv);
                    MPI_Finalize();
                    return -1;
                }
                strcpy(dt_desc[desc_cnt-1], optarg);
                break;
            case 'i':
                iterations = atoi(optarg);
                break;
            case 's':
                umr_split = atoi(optarg);
                break;
            case 'w':
                wsize = atoi(optarg);
                break;
            case 'x':
                warmups = atoi(optarg);
                break;
            case 'h':
            case '?':
                usage(rank, argc, argv);
                MPI_Finalize();
                return 0;
        }
    }

    /* sanity check */
    if (desc_cnt < 1) {
        if (rank == 0) {
            fprintf(stderr, "need to provide at least one data description\n");
        }
        usage(rank, argc, argv);
        MPI_Finalize();
        return -1;
    }

    if (umr_split > 1 && desc_cnt > 1) {
        if (rank == 0) {
            fprintf(stderr, "UMR split mode does not support multiple datatype descriptors\n");
        }
        usage(rank, argc, argv);
        MPI_Finalize();
        return -1;
    }

    /* initialize message descriptors */
    ALLOC(scenario, desc_cnt);
    ALLOC(scenario_split, desc_cnt);
    rc = message_desc_init(dt_desc, scenario, scenario_split, umr_split, &msg_size, desc_cnt);
    if (rc != 0) {
        if (rank == 0) {
            fprintf(stderr, "incorrect datatype description\n");
        }
        usage(rank, argc, argv);
        MPI_Finalize();
        return -1;
    }

    /* set UMR split mode */
    if (scenario[0].rcnt > 1 && scenario_split[0].rcnt > 1) split_mode = 1;

#if 0
    fprintf(stderr, "%d %d %d %d\n", scenario[0].ilv, scenario[0].rcnt, scenario[0].payloads[0], scenario[0].strides[0]);
    fprintf(stderr, "%d %d %d %d\n", scenario_split[0].ilv, scenario_split[0].rcnt, scenario_split[0].payloads[0], scenario_split[0].strides[0]);
    MPI_Finalize();
    return 0;
#endif

    message_t *msgs[wsize], *msgs_split[wsize];
    MPI_Datatype types[wsize], types_split[wsize];
    char *recv_bufs[wsize], *recv_bufs_split[wsize];
    int i;

    for(i = 0; i < wsize; i++){
        if (split_mode) {
            setenv("OMPI_WANT_UMR", "1", 1);
            create_mpi_index(0, NULL, 0, 0, 0, scenario, desc_cnt, &types[i], &msgs[i]);
            ALLOC(recv_bufs[i], msgs[i]->outlen);
            unsetenv("OMPI_WANT_UMR");
            create_mpi_index(0, NULL, 0, 0, 0, scenario_split, desc_cnt, &types_split[i], &msgs_split[i]);
            ALLOC(recv_bufs_split[i], msgs_split[i]->outlen);
        } else {
            create_mpi_index(0, NULL, 0, 0, 0, scenario, desc_cnt, &types[i], &msgs[i]);
            ALLOC(recv_bufs[i], msgs[i]->outlen);
        }
    }

    /* warmup */
    measure_cycle(wsize, types, msgs, recv_bufs, types_split, msgs_split, recv_bufs_split, split_mode, warmups);

    double start = GET_TS();
    measure_cycle(wsize, types, msgs, recv_bufs, types_split, msgs_split, recv_bufs_split, split_mode, iterations);
    double end = GET_TS();

    if(0 == rank) {
        fprintf(stdout, "window size = %d, iterations = %d, msg size = %d\n", wsize, iterations, msg_size);
        fprintf(stdout, "total = %lf  latency = %lf\n", (end - start)/iterations * 1E6, (end - start)/iterations/wsize * 1E6);
    }

    MPI_Finalize();
    return 0;
}
