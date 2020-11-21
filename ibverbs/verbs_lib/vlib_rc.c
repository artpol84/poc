#include "vlib_rc.h"

#define QPSN 0

typedef struct {
} vlib_rc_ep_t;

int vlib_rc_transp_set(vlib_transp_t *t)
{
    t->iface.create_ep = vlib_rc_create_ep;
    t->iface.destroy_ep = vlib_rc_ep_destroy;
    t->iface.single_addr = vlib_rc_single_addr;
    t->iface.inline_size = vlib_rc_inline_size;
    return 0;
}

int vlib_rc_single_addr(struct vlib_transp_t *t)
{
    return 0;
}

vlib_ep_t *vlib_rc_create_ep(struct vlib_transp_t *t)
{
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_qp_attr qp_attr;
    vlib_ep_rc_t *rc_ep = calloc(1, (*rc_ep));
    int ret;
    rc_ep->super.th = th;

    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.send_cq = th->rh->cq;
    qp_init_attr.recv_cq = th->rh->cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.qp_context = th->rh->vh->ctx;
    qp_init_attr.sq_sig_all = th->dflt_compl;

    qp_init_attr.cap.max_recv_sge = th->rh->vh->dev_attr.max_sge;
    qp_init_attr.cap.max_send_sge = th->rh->vh->dev_attr.max_sge;
    qp_init_attr.cap.max_send_wr = max_send;
    qp_init_attr.cap.max_recv_wr = max_recv;

    rc_ep->qp = ibv_create_qp(th->rh->vh->ctx, &qp_init_attr);

    assert(rc_ep->qp);

    /* Initialize QP */
    memset(&qp_attr, 0, sizeof(qp_attr));
    qp_attr.qp_state        = IBV_QPS_INIT;
    qp_attr.pkey_index      = 0;
    qp_attr.port_num        = th->rh->vh->port_num;
    qp_attr.qp_access_flags = 0;

    ret = ibv_modify_qp(rc_ep->qp, &qp_attr,
                        IBV_QP_STATE              |
                        IBV_QP_PKEY_INDEX         |
                        IBV_QP_PORT               |
                        IBV_QP_ACCESS_FLAGS);
    assert(0 == ret);

    rc_ep->addr->lid = th->rh->vh->port_attr.lid;
    rc_ep->addr->qpnum = rc_ep->qp->qp_num;

    return &rc_ep->super;
}

int vlib_rc_ep_inline_size(vlib_ep_t *ep)
{
    return 0;
}


int vlib_rc_ep_destroy(vlib_ep_t *ep)
{
    vlib_ep_rc_t *rc_ep = vlib_container_of(ep, vlib_ep_rc_t, super);
    ibv_destroy_qp(rc_ep->qp);
}


vlib_addr_t *vlib_rc_ep_get_addr(vlib_ep_t *ep, int *size)
{
    vlib_ep_rc_t *rc_ep = vlib_container_of(ep, vlib_ep_rc_t, super);
    *size = rc_ep->addr_size;
    return (vlib_addr_t *)&rc_ep->addr;
}

int vlib_rc_ep_connect(struct vlib_ep_s *ep, vlib_addr_t *_addr)
{
    vlib_ep_rc_t *rc_ep = vlib_container_of(ep, vlib_ep_rc_t, super);
    struct ibv_qp_attr attr;
    vlib_addr_rc_t *addr = (vlib_addr_rc_t *)_addr;
    int ret;

    memset(&attr, 0, sizeof(attr));
    attr.qp_state           = IBV_QPS_RTR;
    attr.path_mtu           = ep->th->rh->vh->port_attr.max_mtu;
    attr.dest_qp_num        = addr->qpnum;
    attr.rq_psn             = QPSN;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer      = 12;
    attr.ah_attr.is_global  = 0;
    attr.ah_attr.dlid       = addr->lid;
    attr.ah_attr.sl         = sl;
    attr.ah_attr.src_path_bits  = 0;
    attr.ah_attr.port_num   = addr->port_num;

    ret = ibv_modify_qp(rc_ep->qp, &attr,
                        IBV_QP_STATE              |
                        IBV_QP_AV                 |
                        IBV_QP_PATH_MTU           |
                        IBV_QP_DEST_QPN           |
                        IBV_QP_RQ_PSN             |
                        IBV_QP_MAX_DEST_RD_ATOMIC |
                        IBV_QP_MIN_RNR_TIMER);
    assert(0 == ret);

    attr.qp_state       = IBV_QPS_RTS;
    attr.timeout        = 14;
    attr.retry_cnt      = 7;
    attr.rnr_retry      = 7;
    attr.sq_psn         = QPSN;
    attr.max_rd_atomic  = 1;
    ret = ibv_modify_qp(rc_ep->qp, &attr,
                        IBV_QP_STATE              |
                        IBV_QP_TIMEOUT            |
                        IBV_QP_RETRY_CNT          |
                        IBV_QP_RNR_RETRY          |
                        IBV_QP_SQ_PSN             |
                        IBV_QP_MAX_QP_RD_ATOMIC);
    assert(0 == ret);

    return 0;
}

int vlib_rc_ep_disconnect(struct vlib_ep_s *ep)
{
    vlib_ep_rc_t *rc_ep = vlib_container_of(ep, vlib_ep_rc_t, super);
    struct ibv_qp_attr attr;
    vlib_addr_rc_t *addr = (vlib_addr_rc_t *)_addr;
    int ret;

    memset(&attr, 0, sizeof(attr));
    attr.qp_state           = IBV_QPS_RESET;

    ret = ibv_modify_qp(rc_ep->qp, &attr,
                        IBV_QP_STATE);
    assert(0 == ret);
}

int vlib_rc_ep_send_inline(struct vlib_ep_s *ep, char *buf, int size)
{
    assert(0);
    return 0;
}

int vlib_rc_ep_send_regular(vlib_ep_t *ep, vlib_msg_t *msg)
{
    struct ibv_sge list;
    struct ibv_send_wr wr, *bad_wr;
    vlib_ep_rc_t *rc_ep = vlib_container_of(ep, vlib_ep_rc_t, super);

    msg->complete = 0;

    memset(&list, 0, sizeof(list));
    list.addr = (uintptr_t)((char*)msg->buf->buf + msg->offs);
    list.length = msg->size;
    list.lkey = msg->buf->mr->lkey;

    memset(&wr, 0, sizeof(wr));
    wr.wr_id        = (uint64_t)msg;
    wr.sg_list    = &list;
    wr.num_sge    = 1;
    wr.opcode     = IBV_WR_SEND;
    wr.send_flags = IBV_SEND_SIGNALED;
    ret = ibv_post_send(rc_ep->qp, &wr, &bad_wr);
    assert(0 == ret);
    return 0;
}

int vlib_rc_ep_recv(vlib_ep_t *ep, vlib_msg_t *msg)
{
    struct ibv_sge list;
    struct ibv_recv_wr wr, *bad_wr;
    vlib_ep_rc_t *rc_ep = vlib_container_of(ep, vlib_ep_rc_t, super);

    memset(&list, 0, sizeof(list));
    list.addr   = (uintptr_t)((char*)msg->buf->buf + msg->offs);
    list.length = msg->size;
    list.lkey   = msg->buf->mr->lkey;

    memset(&wr, 0, sizeof(wr));
    wr.wr_id      = (uint64_t)msg;
    wr.sg_list    = &list;
    wr.num_sge    = 1;

    ret = ibv_post_recv(rc_ep->qp, &wr, &bad_wr);
    assert(0 == ret);
    return 0;
}

static int rc_single_addr(struct vlib_transp_s *th)
{
    return 0;
}

