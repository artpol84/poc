#ifndef VLIB_RC_H
#define VLIB_RC_H

#include "vlib.h"

typedef struct {
    int port_num;
    int qpnum;
    int lid;
} vlib_addr_rc_t;

typedef struct {
    vlib_ep_t super;
    struct ibv_qp *qp;
    vlib_addr_rc_t *addr;
    int addr_size;
} vlib_ep_rc_t;

int vlib_rc_transp_set(struct vlib_transp_t *t);

int vlib_rc_single_addr(struct vlib_transp_t *t);
vlib_ep_t *vlib_rc_create_ep(struct vlib_transp_t *t);
int vlib_rc_ep_destroy(vlib_ep_t *ep);
int vlib_rc_inline_size(vlib_ep_t *ep);


vlib_addr_t *vlib_rc_ep_get_addr(struct vlib_ep_t *ep, int *size);
int vlib_rc_ep_connect(struct vlib_ep_s *ep, vlib_addr_t *addr);
int vlib_rc_ep_disconnect(struct vlib_ep_s *ep);
int vlib_rc_ep_send_inline(struct vlib_ep_s *ep, char *buf, int size);
int vlib_rc_ep_send_regular(struct vlib_ep_s *ep, vlib_buf_t *bh,
                   char *buf, int size);
int vlib_rc_ep_recv(struct vlib_ep_s *ep, vlib_buf_t *bh,
               char *buf, int size);

#endif // VLIB_RC_H
