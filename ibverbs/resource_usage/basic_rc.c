#include "basic_ib.h"

int create_qp(ib_qp_t *qp, ib_cq_t *cq)
{
    struct ibv_qp_init_attr attr = { 0 };
    struct ibv_qp_attr qp_modify_attr = { 0 };

    attr.send_cq = cq->ib_cq;
    attr.recv_cq = cq->ib_cq;
    attr.cap.max_send_wr  = cq->size / 2;
    attr.cap.max_recv_wr  = cq->size / 2;
    attr.cap.max_send_sge = 1;
    attr.cap.max_recv_sge = 1;
    attr.qp_type = IBV_QPT_RC;

    qp->ib_qp = ibv_create_qp(cq->ctx->ib_pd, &attr);
    if (!qp->ib_qp) {
        fprintf(stderr, "Couldn't create QP\n");
        return 1;
    }
    printf("QP #%d\n", qp->ib_qp->qp_num);

    qp_modify_attr.qp_state        = IBV_QPS_INIT;
    qp_modify_attr.pkey_index      = 0;
    qp_modify_attr.port_num        = cq->ctx->s->dev_port;
    qp_modify_attr.qp_access_flags = 0;
    if (ibv_modify_qp(qp->ib_qp, &qp_modify_attr,
                      IBV_QP_STATE              |
                      IBV_QP_PKEY_INDEX         |
                      IBV_QP_PORT               |
                      IBV_QP_ACCESS_FLAGS)) {
        fprintf(stderr, "Failed to modify QP to INIT\n");
        return 1;
    }
    return 0;
}

void destroy_qp(ib_qp_t *qp)
{
    ibv_destroy_qp(qp->ib_qp);
}

int connect_qp(ib_qp_t *from_qp, ib_qp_t *to_qp)
{
    struct ibv_qp_attr qp_modify_attr = { 0 };

    memset(&qp_modify_attr, 0, sizeof(qp_modify_attr));
    qp_modify_attr.qp_state           = IBV_QPS_RTR;
    qp_modify_attr.path_mtu           = from_qp->cq->ctx->max_mtu;
    qp_modify_attr.dest_qp_num        = to_qp->ib_qp->qp_num;
    qp_modify_attr.rq_psn             = 0;
    qp_modify_attr.max_dest_rd_atomic = 1;
    qp_modify_attr.min_rnr_timer      = 12;
    qp_modify_attr.ah_attr.is_global  = 0;
    qp_modify_attr.ah_attr.dlid       = from_qp->cq->ctx->lid;
    qp_modify_attr.ah_attr.sl         = 0;
    qp_modify_attr.ah_attr.src_path_bits	= 0;
    qp_modify_attr.ah_attr.port_num	= from_qp->cq->ctx->s->dev_port;
    if (ibv_modify_qp(to_qp->ib_qp, &qp_modify_attr,
                      IBV_QP_STATE              |
                      IBV_QP_AV                 |
                      IBV_QP_PATH_MTU           |
                      IBV_QP_DEST_QPN           |
                      IBV_QP_RQ_PSN             |
                      IBV_QP_MAX_DEST_RD_ATOMIC |
                      IBV_QP_MIN_RNR_TIMER)) {
        fprintf(stderr, "Failed to modify QP to RTR\n");
        return 1;
    }

    memset(&qp_modify_attr, 0, sizeof(qp_modify_attr));
    qp_modify_attr.qp_state       = IBV_QPS_RTS;
    qp_modify_attr.rnr_retry      = 7;
    qp_modify_attr.sq_psn         = 0;
    qp_modify_attr.max_rd_atomic  = 1;

    /* Setup the send retry parameters:
     *  - timeout (see [1] for details on how to calculate)
     *    $ Value of 23 corresponds to 34 sec
     *  - retry_cnt (max is 7 times)
     * Max timeout is (retry_cnt * timeout)
     *  - 7 * 34s = 231s = 3 m
     * [1] https://www.rdmamojo.com/2013/01/12/ibv_modify_qp/
     */
    qp_modify_attr.timeout            = 16;
    qp_modify_attr.retry_cnt          = 7;

    if (ibv_modify_qp(to_qp->ib_qp, &qp_modify_attr,
                      IBV_QP_STATE              |
                      IBV_QP_TIMEOUT            |
                      IBV_QP_RETRY_CNT          |
                      IBV_QP_RNR_RETRY          |
                      IBV_QP_SQ_PSN             |
                      IBV_QP_MAX_QP_RD_ATOMIC)) {
        fprintf(stderr, "Failed to modify QP to RTS\n");
        return 1;
    }
    return 0;
}

#if 0
int send_rc(ib_context_t *ctx, int size)
{
    struct ibv_sge list;
    struct ibv_send_wr wr;

    memset(&list, 0, sizeof(list));
    list.addr = (uintptr_t)ctx->mr_buffer;
    list.length = size;
    list.lkey = ctx->mr->lkey;

    memset(&wr, 0, sizeof(wr));
    wr.wr_id        = (uint64_t)0;
    wr.sg_list    = &list;
    wr.num_sge    = 1;
    wr.opcode     = IBV_WR_SEND;
    wr.send_flags = IBV_SEND_SIGNALED;

    return send_wr(ctx, &wr);
}
#endif
