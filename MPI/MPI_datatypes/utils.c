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

#include <stdio.h>
#include <mpi.h>
#include "utils.h"

char init_symb(int bufidx, int idx)
{
    char start, end;
    int len;
    switch(bufidx % 4){
    case 0:
        start = 'A';
        end = 'Z';
        break;
    case 1:
        start = 'a';
        end = 'z';
        break;
    case 2:
        start = '0';
        end = '9';
        break;
    case 3:
        start = '!';
        end = '/';
        break;
    }
    return start + idx % (end - start + 1);
}

range_t *init_range(int start_bufidx, int bufcnt, int repcnt, int *payloads, int *strides)
{
    range_t *r = calloc(1, sizeof(*r));
    int i, j, k, idx;
    
    r->bufcnt = bufcnt;
    r->repcnt = repcnt;
    r->nblocks = repcnt * bufcnt;

    ALLOC(r->payloads, bufcnt);
    memcpy(r->payloads, payloads, bufcnt * sizeof(*r->payloads));
    ALLOC(r->strides, bufcnt);
    memcpy(r->strides, strides, bufcnt * sizeof(*r->strides));
    
    ALLOC(r->insizes, bufcnt);
    ALLOC(r->inbufs, bufcnt);
    for(i = 0; i < bufcnt; i++){
        r->insizes[i] = r->repcnt * r->strides[i];
        r->outsize += r->repcnt * r->payloads[i];
        ALLOC(r->inbufs[i], r->insizes[i]);
        for(j=0; j < r->insizes[i]; j++){
            r->inbufs[i][j] = init_symb(start_bufidx + i,j);
        }
    }
    ALLOC(r->outbuf, r->outsize);

    for(j = 0, idx = 0; j < repcnt; j++){
        for(i = 0; i < bufcnt; i++){
            for(k = 0; k < r->payloads[i]; k++){
                r->outbuf[idx++] = r->inbufs[i][ j * r->strides[i] + k];
            }
        }
    }
    return r;
}

void range_fill(range_t *r, char *base_addr, int displs[], int blocklens[], char outbuf[])
{
    int idx, i, j;
    memcpy(outbuf, r->outbuf, r->outsize);
    for(j = 0, idx = 0; j < r->repcnt; j++){
        for(i = 0; i < r->bufcnt; i++){
            blocklens[idx] = r->payloads[i];
            displs[idx] = &r->inbufs[i][ j * r->strides[i]] - base_addr;
            idx++;
        }
    }
}


message_t *message_init(int verbose,
                        char *base_ptr,
                        int rangeidx, int bufidx, int blockidx,
                        message_desc_t *desc, int desc_cnt)
{
    int i;
    int blk_offs;
    int out_offs;
    message_t *m;
    int buf_offs = 0;
    int rank;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    ALLOC(m, 1);
    m->ranges_cnt = desc_cnt;
    ALLOC(m->ranges, desc_cnt);
    
    for(i=0 ; i < desc_cnt; i++){
        m->ranges[i] = init_range(buf_offs, desc[i].ilv, desc[i].rcnt, desc[i].payloads, desc[i].strides);
        m->nblocks += m->ranges[i]->nblocks;
        m->outlen += m->ranges[i]->outsize;
        buf_offs += desc[i].ilv;
    }
    ALLOC(m->displs, m->nblocks);
    ALLOC(m->blens, m->nblocks);
    ALLOC(m->outbuf, m->outlen);
    
    if(base_ptr == NULL){
        /* point to the very first buffer */
        base_ptr = m->ranges[rangeidx]->inbufs[bufidx] + m->ranges[rangeidx]->strides[bufidx] * blockidx;
        if( rank == 0 && verbose ){
            printf("BASE addr: (%d, %d, %d), start_offs = %zd\n", rangeidx, bufidx, blockidx, 
                    base_ptr - m->ranges[0]->inbufs[0]);
        }
    }
    m->base_addr = base_ptr;

    blk_offs = out_offs = 0;
    for(i=0 ; i < desc_cnt; i++){
        range_fill(m->ranges[i], base_ptr, m->displs + blk_offs, m->blens + blk_offs, m->outbuf + out_offs);
        blk_offs += m->ranges[i]->nblocks;
        out_offs += m->ranges[i]->outsize;
    }
    return m;
}

void create_mpi_index(int verbose,
                      char *base_ptr, int rangeidx, int bufidx, int blockidx,
                      message_desc_t *scenario, int desc_cnt,
                      MPI_Datatype *type, message_t **m_out)
{
    message_t *m = message_init(verbose, base_ptr, rangeidx, bufidx, blockidx, scenario, desc_cnt);
    MPI_Type_indexed(m->nblocks, m->blens, m->displs, MPI_CHAR, type);
    MPI_Type_commit(type);
    *m_out = m;
}

int test_mpi_index(char *base_ptr, int rangeidx, int bufidx, int blockidx,
                   message_desc_t *scenario, int desc_cnt, int ndts, int recv_use_dt)
{
    MPI_Datatype type[ndts];
    int rank;
    MPI_Request reqs[ndts];
    message_t *m[ndts];
    char *recv_buf[ndts], sync[1];
    int k, j;
    int rc = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for(j = 0; j < ndts; j++){
        create_mpi_index(1, base_ptr, rangeidx, bufidx, blockidx,
                         scenario, desc_cnt,
                         &type[j], &m[j]);
        ALLOC(recv_buf[j], m[j]->outlen);
    }
    int verbose = 1;
    for(k = 0; k < 5; k++) {
        if( rank == 0 ){
            MPI_Send(sync, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
            for(j=0; j<ndts; j++){
                MPI_Isend(m[j]->base_addr, 1, type[j], 1, 0, MPI_COMM_WORLD,
                          &reqs[j]);
            }
            MPI_Waitall(ndts, reqs, MPI_STATUSES_IGNORE);
        } else {
            int i;
            MPI_Recv(sync, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for(j=0; j<ndts; j++){
                if(!recv_use_dt) {
                    MPI_Irecv(recv_buf[j], m[j]->outlen, MPI_CHAR, 0, 0,
                              MPI_COMM_WORLD, &reqs[j]);
                } else {
                    MPI_Irecv(m[j]->base_addr, 1, type[j], 0, 0,
                              MPI_COMM_WORLD, &reqs[j]);
                }
            }
            MPI_Waitall(ndts, reqs, MPI_STATUSES_IGNORE);

            for(j=0; j < ndts; j++){
                if(verbose){
                    printf("[%d] Received: ", j);
                }
                for(i=0; i < m[j]->outlen; i++){
                    if( recv_buf[j][i] != m[j]->outbuf[i] ){
                        printf("Message mismatch in offset=%d, expect '%c', got '%c'\n",
                               i, m[j]->outbuf[i], recv_buf[j][i]);
                        rc = 1;
                    }
                    if(verbose) {
                        printf("%c", recv_buf[j][i]);
                    }
                }
                if(verbose){
                    printf("\n");
                }
            }
        }
        verbose = 0;
    }
    
    return rc;
}
