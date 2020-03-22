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

message_t *message_init(char *base_ptr, int rangeidx, int bufidx, int blockidx, message_desc_t *desc, int desc_cnt);
int test_mpi_index(char *base_ptr, int rangeidx, int bufidx, int blockidx, message_desc_t *scenario, int desc_cnt);
void create_mpi_index(char *base_ptr, int rangeidx, int bufidx, int blockidx,
                      message_desc_t *scenario, int desc_cnt,
                      MPI_Datatype *type, message_t **m_out);
