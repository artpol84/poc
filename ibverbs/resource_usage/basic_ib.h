#ifndef BASIC_IB_COMMON_H
#define BASIC_IB_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <malloc.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <time.h>
#include <infiniband/verbs.h>

typedef struct {
    char devname[64];
    int  dev_port;
    int nqps;
    int nmrs;
} settings_t;

typedef struct {
    settings_t *s;
    struct ibv_context *ctx;
    struct ibv_device *ib_dev;
    struct ibv_pd *ib_pd;
    int  lid;
    enum ibv_mtu max_mtu;
} ib_context_t;

typedef struct {
    ib_context_t *ctx;
    void *base;
    int size;
    struct ibv_mr *ib_mr;
} ib_mr_t;

typedef struct {
    ib_context_t *ctx;
    struct ibv_cq *ib_cq;
    int size;
} ib_cq_t;

typedef struct {
    ib_cq_t *cq;
    struct ibv_qp *ib_qp;
} ib_qp_t;

int init_ctx(ib_context_t *ctx, settings_t *s);
void free_ctx(ib_context_t *ctx);
int create_mr(ib_mr_t *mr, ib_context_t *ctx, void *base, int size);
void destroy_mr(ib_mr_t *mr);
int create_cq(ib_cq_t *cq, ib_context_t *ctx, int count);
void destroy_cq(ib_cq_t *cq);

#if 0
int recv_loop(ib_context_t *ctx, int is_ud, int cnt);
int send_wr(ib_context_t *ctx, struct ibv_send_wr *wr);
struct ibv_ah *get_ah(ib_context_t *ctx);
#endif

#endif // BASIC_IB_COMMON_H
