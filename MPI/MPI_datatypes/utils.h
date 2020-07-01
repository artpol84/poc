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
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

typedef struct range_s {
    int bufcnt;
    int repcnt;
    int *payloads, *strides, *insizes;
    char **inbufs;
    char *outbuf;
    int outsize;
    int nblocks;
} range_t;

typedef struct message_desc_s {
    int ilv, rcnt;
    int payloads[32], strides[32];
} message_desc_t;

typedef struct message_s {
    int ranges_cnt;
    range_t **ranges;
    int nblocks, outlen;
    int *displs, *blens;
    char *outbuf;
    char *base_addr;
} message_t;

#define ALLOC(ptr, count) { (ptr) = calloc(count, sizeof(*(ptr))); }

range_t *range_init(int bufcnt, int repcnt, int *payloads, int *strides);
void range_fill(range_t *r, char *base_addr, int displs[], int blocklens[], char outbuf[]);

message_t *message_init(int verbose,
                        char *base_ptr,
                        int rangeidx, int bufidx, int blockidx,
                        message_desc_t *desc, int desc_cnt);
void create_mpi_index(int verbose,
                      char *base_ptr, int rangeidx, int bufidx, int blockidx,
                      message_desc_t *scenario, int desc_cnt,
                      MPI_Datatype *type, message_t **m_out);

#ifndef BASE_RANGE
#define BASE_RANGE 0
#endif

#ifndef BASE_BUF
#define BASE_BUF 0
#endif

#ifndef BASE_IDX
#define BASE_IDX 0
#endif

#ifndef RECV_TYPE
#define RECV_TYPE 0
#endif

#ifndef FORCE_UNEXP
#define FORCE_UNEXP 0
#endif

#define WANT_VERIFICATION 1
#define NO_VERIFICATION 0



int test_mpi_index(char *base_ptr, int rangeidx, int bufidx, int blockidx,
                   message_desc_t *scenario, int desc_cnt,
                   int ndts, int recv_use_dt, int force_unexp,
                   int want_verification);


