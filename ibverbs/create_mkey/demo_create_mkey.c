#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <malloc.h>
#include <assert.h>
#include <infiniband/verbs.h>
#include <infiniband/mlx5dv.h>

typedef struct {
    uint32_t mkey;
    struct mlx5dv_devx_umem *umem;
    struct mlx5dv_devx_obj *obj;
    struct ibv_mr mr;
} ib_mr_t;


typedef struct {
    int port;
    struct ibv_context *ctx;
    struct ibv_device *dev;
    struct ibv_pd *pd;
    struct ibv_cq *cqs[2][2];
    struct ibv_qp *qp[2];
    int *buf;
    int size;
    ib_mr_t mem_desc;
    struct ibv_mr *mr;
} ib_context_t;


#define MAX_GATHER_ENTRIES 2
#define MAX_SCATTER_ENTRIES 2
#define MAX_WR 4

#define SEND_CQ 0
#define RECV_CQ 1

#ifndef USE_DEVX
#define USE_DEVX 0
#endif

#ifndef USE_IOVA
#define USE_IOVA 0
#endif

void create_qp(ib_context_t *ctx)
{
   struct ibv_port_attr pattrs;
   int i;

   if (ibv_query_port(ctx->ctx, ctx->port, &pattrs)) {
       fprintf(stderr, "Couldn't Query port %d for %s\n",
               ctx->port, ibv_get_device_name(ctx->dev));
       abort();
   }

   /* Create QPs */

   for(i = 0; i < 2; i++) {
       struct ibv_qp_init_attr qp_init_attr = { 0 };

       qp_init_attr.sq_sig_all = 1;
       qp_init_attr.send_cq = ctx->cqs[i][SEND_CQ];
       qp_init_attr.recv_cq = ctx->cqs[i][RECV_CQ];
       qp_init_attr.cap.max_send_wr  = MAX_WR;
       qp_init_attr.cap.max_recv_wr  = MAX_WR;
       qp_init_attr.cap.max_send_sge = MAX_GATHER_ENTRIES;
       qp_init_attr.cap.max_recv_sge = MAX_SCATTER_ENTRIES;
       qp_init_attr.qp_type = IBV_QPT_RC;

       ctx->qp[i] = ibv_create_qp(ctx->pd, &qp_init_attr);
       if (!ctx->qp) {
           fprintf(stderr, "Couldn't create QP\n");
           abort();
       }
       printf("QP[%d] #%d\n", i, ctx->qp[i]->qp_num);

       struct ibv_qp_attr qp_modify_attr = { 0 };

       qp_modify_attr.qp_state        = IBV_QPS_INIT;
       qp_modify_attr.pkey_index      = 0;
       qp_modify_attr.port_num        = ctx->port;
       qp_modify_attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE |
                                        IBV_ACCESS_REMOTE_READ |
                                        IBV_ACCESS_REMOTE_WRITE;
       if (ibv_modify_qp(ctx->qp[i], &qp_modify_attr,
                         IBV_QP_STATE              |
                         IBV_QP_PKEY_INDEX         |
                         IBV_QP_PORT               |
                         IBV_QP_ACCESS_FLAGS)) {
           fprintf(stderr, "Failed to modify QP to INIT\n");
           abort();
       }
   }

    for (i = 0; i < 2; i++) {
        struct ibv_qp_attr qp_modify_attr = { 0 };

        /* Move to RTR */
        memset(&qp_modify_attr, 0, sizeof(qp_modify_attr));
        qp_modify_attr.qp_state           = IBV_QPS_RTR;
        qp_modify_attr.path_mtu           = pattrs.max_mtu;
        qp_modify_attr.dest_qp_num        = ctx->qp[(i+1)%2]->qp_num;
        qp_modify_attr.rq_psn             = 0;
        qp_modify_attr.max_dest_rd_atomic = 1;
        qp_modify_attr.min_rnr_timer      = 0x12;
        qp_modify_attr.ah_attr.is_global  = 0;
        qp_modify_attr.ah_attr.dlid       = pattrs.lid;
        qp_modify_attr.ah_attr.sl         = 0;
        qp_modify_attr.ah_attr.src_path_bits = 0;
        qp_modify_attr.ah_attr.port_num	= ctx->port;
        if (ibv_modify_qp(ctx->qp[i], &qp_modify_attr,
                          IBV_QP_STATE              |
                          IBV_QP_PATH_MTU           |
                          IBV_QP_DEST_QPN           |
                          IBV_QP_RQ_PSN             |
                          IBV_QP_MAX_DEST_RD_ATOMIC |
                          IBV_QP_MIN_RNR_TIMER      |
                          IBV_QP_AV)) {
            fprintf(stderr, "Failed to modify QP[%d] to RTR\n", i);
            abort();
        }

        /* Move to RTS */
        memset(&qp_modify_attr, 0, sizeof(qp_modify_attr));
        qp_modify_attr.qp_state       = IBV_QPS_RTS;
        qp_modify_attr.timeout        = 0x12;
        qp_modify_attr.retry_cnt      = 6;
        qp_modify_attr.rnr_retry      = 7;
        qp_modify_attr.sq_psn         = 0;
        qp_modify_attr.max_rd_atomic  = 1;

        if (ibv_modify_qp(ctx->qp[i], &qp_modify_attr,
                          IBV_QP_STATE              |
                          IBV_QP_TIMEOUT            |
                          IBV_QP_RETRY_CNT          |
                          IBV_QP_RNR_RETRY          |
                          IBV_QP_SQ_PSN             |
                          IBV_QP_MAX_QP_RD_ATOMIC)) {
            fprintf(stderr, "Failed to modify QP[%d] to RTS\n", i);
            abort();
        }
    }
}

void alloc_mem(ib_context_t *ctx, int size)
{
    if (posix_memalign((void**)&ctx->buf, 4096, size)) {
        fprintf(stderr, "failed to allocate buffer of %u bytes: %m\n", size);
        abort();
    }
    ctx->size = size;
}

void free_mem(ib_context_t *ctx)
{
    free(ctx->buf);
}

#if USE_DEVX

void alloc_umem(ib_context_t *ctx)
{
    ctx->mem_desc.umem = mlx5dv_devx_umem_reg(ctx->ctx, ctx->buf, ctx->size, 0);
    if (ctx->mem_desc.umem == NULL) {
        fprintf(stderr, "mlx5dv_devx_umem_reg() failed: %m\n");
        abort();
    }
}

void free_umem(ib_context_t *ctx)
{
    mlx5dv_devx_umem_dereg(ctx->mem_desc.umem);
}

int create_devx_mr(ib_context_t *ctx)
{
    struct mlx5dv_pd dvpd  = {};
    struct mlx5dv_obj dv   = {};
    uint32_t out[DEVX_ST_SZ_DW(create_mkey_out)] = {};
    uint32_t in[DEVX_ST_SZ_DW(create_mkey_in)] = {};
    struct mlx5dv_devx_obj *obj;
    uint8_t access_mode = 0x1; // MTT, is it the right one?
    void *mkc;

    dv.pd.in  = ctx->pd;
    dv.pd.out = &dvpd;
    mlx5dv_init_obj(&dv, MLX5DV_OBJ_PD);
    alloc_umem(ctx);

    DEVX_SET(create_mkey_in, in, opcode, MLX5_CMD_OP_CREATE_MKEY);
    DEVX_SET(create_mkey_in, in, mkey_umem_id, ctx->mem_desc.umem->umem_id);
    DEVX_SET64(create_mkey_in, in, mkey_umem_offset, 0);

    mkc = DEVX_ADDR_OF(create_mkey_in, in, memory_key_mkey_entry);
    DEVX_SET(mkc, mkc, rw, 1);
    DEVX_SET(mkc, mkc, rr, 1);
    DEVX_SET(mkc, mkc, lw, 1);
    DEVX_SET(mkc, mkc, lr, 1);
    DEVX_SET(mkc, mkc, access_mode_1_0, access_mode & 0x3);
    DEVX_SET(mkc, mkc, access_mode_4_2, (access_mode >> 2) & 0x7);
    DEVX_SET(mkc, mkc, qpn, 0xffffff);
    DEVX_SET(mkc, mkc, pd, dvpd.pdn);
    DEVX_SET64(mkc, mkc, start_addr, 0);
    DEVX_SET(mkc, mkc, mkey_7_0, 0);
    DEVX_SET64(mkc, mkc, len, size);

    ctx->mem_desc.obj = mlx5dv_devx_obj_create(ibv_ctx, in, sizeof(in), out, sizeof(out));
    if (obj == NULL) {
        fprintf(stderr, "Failed to create MKey object\n");
        abort();
    }

    ctx->mem_desc.mkey = (UCT_IB_MLX5DV_GET(create_mkey_out, out, mkey_index) << 8);

    /* Initialize IBV MR */
    ctx->mem_desc.mr.addr = 0;
    ctx->mem_desc.mr.context = ctx->ctx;
    ctx->mem_desc.mr.pd = ctx->pd;
    ctx->mem_desc.mr.length = size;
    ctx->mem_desc.mr.lkey = ctx->mem_desc.mkey;
    ctx->mem_desc.mr.rkey = ctx->mem_desc.mkey;
    return &ctx->mem_desc.mr;
}

void destroy_devx_mr(ib_context_t *ctx)
{
    if (mlx5dv_devx_obj_destroy(ctx->mem_desc.obj)) {
        fprintf(stderr, "Failed to destroy MKey object\n");
        abort();
    }
    free_umem(ctx);
}
#endif

void create_mr(ib_context_t *ctx, int size)
{
#if !USE_DEVX
    int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
#endif

    alloc_mem(ctx, size);

#if USE_DEVX
    ctx->mr = create_devx_mr(ctx);
#elif USE_IOVA

    ctx->mr = ibv_reg_mr_iova(ctx->pd, ctx->buf, size, 0, mr_flags);
#else
    ctx->mr = ibv_reg_mr(ctx->pd, ctx->buf, size, mr_flags);
#endif
}

void destroy_mr(ib_context_t *ctx)
{
#if USE_DEVX
    destroy_devx_mr(ctx, size);
#else
    ibv_dereg_mr(ctx->mr);
#endif
    free_mem(ctx);
}

void ib_init(ib_context_t *ctx, char *devname, int port)
{

    int num_devices, ret = -1;
    int i;

    memset(ctx, 0, sizeof(*ctx));
    ctx->port = port;

    struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list) {
        perror("Failed to get RDMA devices list");
        abort();
    }

    for (i = 0; i < num_devices; ++i)
        if (!strcmp(ibv_get_device_name(dev_list[i]), devname))
            break;

    if (i == num_devices) {
        fprintf(stderr, "RDMA device %s not found\n", devname);
        abort();
    }

    ctx->dev = dev_list[i];

    ctx->ctx = ibv_open_device(ctx->dev);
    if (!ctx->ctx) {
        fprintf(stderr, "Couldn't get context for %s\n",
                ibv_get_device_name(ctx->dev));
        abort();
    }

    ctx->pd = ibv_alloc_pd(ctx->ctx);
    if (!ctx->pd) {
        fprintf(stderr, "Couldn't allocate PD\n");
        abort();
    }

    for (i = 0; i < 2; i++) {
        int j;
        for (j = 0; j < 2; j++) {
            ctx->cqs[i][j] = ibv_create_cq(ctx->ctx, MAX_WR, NULL,
                                     NULL, 0);
            if (!ctx->cqs[i][j]) {
                fprintf(stderr, "Couldn't create CQ[%d][%d]\n", i, j);
                abort();
            }
        }
    }

    create_qp(ctx);
    create_mr(ctx, 4096);
}

void ib_finalize(ib_context_t *ctx)
{
    int i;

    destroy_mr(ctx);

    for (i = 0; i < 2; i++) {
        ibv_destroy_qp(ctx->qp[i]);
    }

    for (i = 0; i < 2; i++) {
        int j;
        for (j = 0; j < 2; j ++) {
            ibv_destroy_cq(ctx->cqs[i][j]);
        }
    }
    ibv_dealloc_pd(ctx->pd);
    ibv_close_device(ctx->ctx);
}

void test_write(ib_context_t *ctx)
{
    struct ibv_send_wr sr;
    struct ibv_sge sge;
    struct ibv_send_wr *bad_wr = NULL;
    struct ibv_wc wc;
    int ne;

    memset(ctx->buf, 0, ctx->mr->length);
    ctx->buf[0] = 0xCAFECAFE;
    assert(ctx->buf[0] != ctx->buf[1]);

    /* prepare the scatter/gather entry */
     memset(&sge, 0, sizeof(sge));
     sge.addr = (uint64_t)ctx->mr->addr;
     sge.length = sizeof(int);
     sge.lkey = ctx->mr->lkey;

     sr.next = NULL;
     sr.wr_id = 0;
     sr.sg_list = &sge;
     sr.num_sge = 1;
     sr.opcode = IBV_WR_RDMA_WRITE;
     sr.send_flags = IBV_SEND_SIGNALED;
     sr.wr.rdma.remote_addr = (uint64_t)ctx->mr->addr + sizeof(int);
     sr.wr.rdma.rkey = ctx->mr->rkey;

     if (ibv_post_send(ctx->qp[0], &sr, &bad_wr)) {
         fprintf(stderr, "Failed to ibv_post_send()\n");
         abort();
     }

     do {
         ne = ibv_poll_cq(ctx->cqs[0][SEND_CQ], 1, &wc);
     } while (ne == 0);

     if (ne < 0) {
         fprintf(stderr, "CQ is in error state\n");
         abort();
     }

     if (wc.status) {
         fprintf(stderr, "Bad completion (status %d)\n",(int)wc.status);
         abort();
     }

     assert(ctx->buf[0] == ctx->buf[1]);
}

int main(int argc, char *argv[]) {
    ib_context_t ib_ctx, *ctx = &ib_ctx;

    ib_init(ctx, "mlx5_0", 1);
    test_write(ctx);
    ib_finalize(ctx);
    return 0;
}
