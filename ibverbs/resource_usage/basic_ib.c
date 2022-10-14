#include "basic_ib.h"
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>

int init_ctx(ib_context_t *ctx, settings_t *s)
{
    int num_devices, ret = -1;

    memset(ctx, 0, sizeof(*ctx));
    ctx->s = s;

    struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list) {
        perror("Failed to get RDMA devices list");
        return ret;
    }

    int i;
    for (i = 0; i < num_devices; ++i)
        if (!strcmp(ibv_get_device_name(dev_list[i]), s->devname))
            break;

    if (i == num_devices) {
        fprintf(stderr, "RDMA device %s not found\n", s->devname);
        goto  free_dev_list;
    }

    ctx->ib_dev = dev_list[i];

    ctx->ctx = ibv_open_device(ctx->ib_dev);
    if (!ctx->ctx) {
        fprintf(stderr, "Couldn't get context for %s\n",
                ibv_get_device_name(ctx->ib_dev));
        goto free_dev_list;
    }

    struct ibv_port_attr pattrs;
    if (ibv_query_port(ctx->ctx, ctx->s->dev_port, &pattrs)) {
        fprintf(stderr, "Couldn't Query port %d for %s\n",
                ctx->s->dev_port, ibv_get_device_name(ctx->ib_dev));
        goto free_dev_list;
    }

    ctx->lid = pattrs.lid;
    ctx->max_mtu = pattrs.max_mtu;

    ctx->ib_pd = ibv_alloc_pd(ctx->ctx);
    if (!ctx->ib_pd) {
        fprintf(stderr, "Couldn't allocate PD\n");
        goto close_device;
    }

    ret = 0;
    goto free_dev_list;

close_pd:
    ibv_dealloc_pd(ctx->ib_pd);

close_device:
    ibv_close_device(ctx->ctx);

free_dev_list:
    ibv_free_device_list(dev_list);
    return ret;
}

void free_ctx(ib_context_t *ctx)
{
    ibv_dealloc_pd(ctx->ib_pd);
    ibv_close_device(ctx->ctx);
}

int create_mr(ib_mr_t *mr, ib_context_t *ctx, void *base, int size)
{
    mr->ib_mr = ibv_reg_mr(ctx->ib_pd, base, size,
                           IBV_ACCESS_LOCAL_WRITE);
    if (!mr->ib_mr) {
        fprintf(stderr, "Couldn't create MR\n");
        return -1;
    }
    mr->ctx = ctx;
    mr->base = base;
    mr->size = size;
    printf("Creating MR: lkey #%d\n", mr->ib_mr->lkey);

    return 0;
}

void destroy_mr(ib_mr_t *mr)
{
    ibv_dereg_mr(mr->ib_mr);
}

int create_cq(ib_cq_t *cq, ib_context_t *ctx, int count)
{
    cq->ib_cq = ibv_create_cq(ctx->ctx, count, NULL,
                              NULL, 0);
    if (!cq->ib_cq) {
        return -1;
    }
    cq->ctx = ctx;
    cq->size = count;
    return 0;
}

void destroy_cq(ib_cq_t *cq)
{
    ibv_destroy_cq(cq->ib_cq);
}


#if 0

int recv_loop(ib_context_t *ctx, int is_ud, int cnt)
{
    struct ibv_recv_wr wr;
    struct ibv_recv_wr *bad_wr;
    struct ibv_sge list;
    struct ibv_wc wc;
    int ne, i;

#define MAX_MSG_SIZE 0x100
    memset(ctx->mr_buffer, 0, MAX_MSG_SIZE * cnt);

    for (i = 0; i < cnt; i++) {
        list.addr   = (uint64_t)(ctx->mr_buffer + MAX_MSG_SIZE*i);
        list.length = MAX_MSG_SIZE;
        list.lkey   = ctx->mr->lkey;


        wr.wr_id      = i;
        wr.sg_list    = &list;
        wr.num_sge    = 1;
        wr.next       = NULL;

        if (ibv_post_recv(ctx->qp, &wr, &bad_wr)) {
            fprintf(stderr, "Function ibv_post_recv failed\n");
            return 1;
        }
    }

    i = 0;
    while (i < cnt) {
        do { ne = ibv_poll_cq(ctx->cq, 1,&wc);}  while (ne == 0);
        if (ne < 0) {
            fprintf(stderr, "CQ is in error state");
            return 1;
        }

        if (wc.status) {
            fprintf(stderr, "Bad completion (status %d)\n",(int)wc.status);
            return 1;
        } else {
            char *ptr = ctx->mr_buffer + MAX_MSG_SIZE*i;
            if (is_ud) {
                ptr += 40;
            }
            printf("received (#%d == %d): %s\n", i, (int)wc.wr_id, ptr);
        }

        i++;
    }
}

int send_wr(ib_context_t *ctx, struct ibv_send_wr *wr)
{
    struct ibv_send_wr *bad_wr;
    struct ibv_wc wc;
    int ne;

    printf("Ready for post send\n");
    scanf("%*d");
    if (ibv_post_send(ctx->qp, wr,&bad_wr)) {
        fprintf(stderr, "Function ibv_post_send failed\n");
        return -1;
    }

    do {
        ne = ibv_poll_cq(ctx->cq, 1, &wc);
    } while (ne == 0);

    if (ne < 0) {
        fprintf(stderr, "CQ is in error state");
        return -1;
    }

    if (wc.status) {
        fprintf(stderr, "Bad completion (status %d)\n",(int)wc.status);
        return -1;
    }
    return 0;
}
#endif
