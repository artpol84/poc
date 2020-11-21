#include "vlib.h"

/* Context init */
vlib_hndl_t *vlib_ctx_create(char *dev_name, int port)
{
    vlib_hndl_t *vh = NULL;
    struct ibv_device **dev_list = NULL;
    int num_devices, i;
    int ret;

    /* Open IB device */
    dev_list = ibv_get_device_list(&num_devices);
    assert(dev_list);
    for (i = 0; i < num_devices; ++i)
        if (!strcmp(ibv_get_device_name(dev_list[i]), dev_name))
            break;
    assert(i < num_devices);

    vh = calloc(1, sizeof(*vh));
    vh->ctx = ibv_open_device(dev_list[i]);
    assert(vh->ctx);

    /* Cleanup device list object */
    ibv_free_device_list(dev_list);
    dev_list = NULL;

    ret = ibv_query_device(vh->ctx, &vh->dev_attr);
    assert(0 == ret);

    ret = ibv_query_port(vh->ctx, vh->port_num, &vh->port_attr);
    assert(0 == ret);

    ret = ibv_query_gid(vh->ctx, vh->port_num, 0, &vh->gid);
    assert(0 == ret);

    vh->pd = ibv_alloc_pd(vh->ctx);
    assert(vh->pd);

    return vh;
}
int vlib_ctx_destroy(vlib_hndl_t *vh)
{
    ibv_close_device(vh->ctx);
    ibv_dealloc_pd(vh->pd);
}

vlib_shres_t *vlib_res_create(vlib_hndl_t *vh, int cq_size, int srq_size)
{
    vlib_shres_t *rh = calloc(1, sizeof(*r));
    rh->vh = vh;

    if(0 < cq_size) {
        rh->cq = ibv_create_cq(vh->ctx, cq_size, NULL, NULL, 0);
        assert(rh->cq);
        rh->cq_size = cq_size;
    }

    if(0 < srq_size) {
        /* No yet supported */
        assert(rh->srq);
        rh->srq_size = srq_size;
    }
    return rh;
}

int vlib_res_destroy(vlib_shres_t *rh)
{

    if(0 < rh->cq_size) {
        ibv_destroy_cq(rh->cq);
    }
    if(0 < rh->srq_size) {
        /* No yet supported */
        assert(rh->srq);
    }
}

vlib_buf_t *vlib_buf_reg(vlib_hndl_t *vh, char *buf, size_t size)
{
    vlib_buf_t *bh = calloc(1, sizeof(*bh));
    bh->mr = ibv_reg_mr(vh->pd, buf, size,
                        IBV_ACCESS_LOCAL_WRITE  |
                        IBV_ACCESS_REMOTE_WRITE |
                        IBV_ACCESS_REMOTE_READ  |
                        IBV_ACCESS_REMOTE_ATOMIC);
    bh->buf = buf;
    bh->size = size;
}
int vlib_buf_dereg(vlib_buf_t *bh)
{
    ibv_dereg_mr(bh->mr);
    free(bh);
}

vlib_transp_t *
vlib_transp_create(vlib_transp_type_t transp, vlib_shres_t *rh)
{
    vlib_transp_t *th = calloc(1, sizeof(*th));
    th->rh = rh;
    th->type = transp;
    switch(transp) {
    case VLIB_RC:
        vlib_rc_transp_set(th);
        break;
    case VLIB_UD:
        //vlib_ud_transp_set(th);
        assert(0);
        break;
    case VLIB_DC:
        perror("DC transport is not yet supported");
        //vlib_dc_transp_set(th);
        assert(0);
    default:
        /* Shouldn't get here */
        assert(0);
    }
    return th;
}

int vlib_transp_destroy(vlib_transp_t *th)
{
    switch(th->type) {
    case VLIB_RC:
        vlib_transp_rc_cleanup(th->data);
        break;
    case VLIB_UD:
        vlib_transp_ud_cleanup(th->data);
        break;
    case VLIB_DC:
        perror("DC transport is not yet supported");
    default:
        /* Shouldn't get here */
        assert(0);
    }
}

int vlib_transp_single_addr(vlib_transp_t *th)
{
    return th->iface.single_addr(th);
}

vlib_ep_t *vlib_ep_create(vlib_transp_t *th)
{
    return th->iface.create_ep(eph->th);
}

int vlib_ep_destroy(vlib_ep_t *eph)
{
    eph->th->iface.destroy_ep(eph);
    free(eph);
    return 0;
}

vlib_msg_t *vlib_ep_send(vlib_ep_t *ep, int req_compl, vlib_buf_t *buf,
                         int offset, int size)
{
    assert((0 <= offset) && (0 < size) && ((offset + size) < buf->size));
    if(ep->iface.inline_size(ep) > size ) {
        ep->iface.send_inline(ep, req_compl, buf->buf + offset, size);
        return NULL;
    }
    vlib_msg_t *msg = calloc(1, sizeof(*msg));
    msg->type = VLIB_SEND;
    msg->buf = buf;
    msg->eph = eph;
    msg->offs = offset;
    msg->size = size;
    ep->iface.send_regular(msg);
    return msg;
}
vlib_msg_t *vlib_ep_recv(vlib_ep_t *eph, vlib_buf_t *buf,
                         ssize_t offset, size_t size)
{
    assert((0 <= offset) && (0 < size) && ((offset + size) < buf->size));
    vlib_msg_t *msg = calloc(1, sizeof(*msg));
    msg->type = VLIB_RECV;
    msg->seq_num = eph->recvs_posted++;
    msg->buf = buf;
    msg->eph = eph;
    msg->offs = offset;
    msg->size = size;
    ep->iface.recv(msg);
    return msg;
}

static int _test_msg(vlib_msg_t *msg, struct ibv_exp_wc *wc)
{
    int ret = 0;
    int ne = ibv_poll_cq(msg->eph->th->rh->cq, 1, wc);
    if (ne <= 0) {
        *err = ne;
        ret = 1;
    } else {
        vlib_msg_t *in_msg = (vlib_msg_t *)wc->wr_id;
        in_msg->complete = 1;
        switch(in_msg->type) {
        case VLIB_SEND:
            in_msg->eph->sends_compl = in_msg->seq_num;
            break;
        }
        case VLIB_RECV:
            in_msg->eph->recvs_compl = in_msg->seq_num;
            break;
    }
    return (ne < 0) ? ne : 0;
}

static int _wait_msg(vlib_msg_t *msg, struct ibv_exp_wc *wc)
{
    int err;
    do {
        err = _test_msg(msg, wc);
    } while(!err && !msg->complete);
    return err;
}


int vlib_wait_all(vlib_msg_t **msgs, int count)
{
    int i, err;
    struct ibv_exp_wc wc;
    for(i = 0; i < count; i++){
        err = _wait_msg(msg, &wc);
    }
}
