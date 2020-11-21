#ifndef VLIB_H
#define VLIB_H

#include <stdio.h>
#include <infiniband/verbs.h>

#define vlib_container_of(ptr, type, member) ({                 \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef enum {
    VLIB_RC,
    VLIB_UD,
    VLIB_DC
} vlib_transp_type_t;

typedef enum {
    VLIB_SEND,
    VLIB_RECV,
} vlib_msg_type_t;


typedef struct {
    int port_num;
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_device_attr dev_attr;
    struct ibv_port_attr port_attr;
    struct ibv_gid gid;
} vlib_hndl_t;

typedef struct {
    /* Main handler */
    vlib_hndl_t *vh;
    /* CQ */
    struct ibv_cq *cq;
    int cq_size;
    /* SRQ */
    struct ibv_srq *srq;
    int srq_size;
} vlib_shres_t;

typedef struct {
    struct ibv_mr *mr;
    char *buf;
    size_t size;
} vlib_buf_t;

typedef char vlib_addr_t;

typedef struct {
    int (*single_addr)(struct vlib_transp_s *t);
    void *(*create_ep)(struct vlib_transp_s *t);
    int (*destroy_ep)(struct vlib_transp_s *t);
    int (*inline_size)(struct vlib_transp_s *t);
} vlib_transp_cbs_t;

typedef struct vlib_transp_s {
    /* Callbacks */
    vlib_transp_cbs_t iface;
    /* Shared resource handler used during creation */
    vlib_shres_t *rh;
    /* Transport ID */
    vlib_transp_type_t type;
    /* Transport-specific data */
    void *data;
    /*  */
    int dflt_compl;
} vlib_transp_t;

typedef struct {
    vlib_addr_t *(*get_addr)(struct vlib_ep_s *ep, int *size);
    int (*connect)(struct vlib_ep_s *ep, vlib_addr_t *addr);
    int (*disconnect)(struct vlib_ep_s *ep);
    int (*send_inline)(struct vlib_ep_s *ep, char *buf, int size);
    int (*send_regular)(vlib_ep_t *ep, vlib_msg_t *msg);
    int (*recv)(struct vlib_ep_s *ep, vlib_buf_t *bh,
                   char *buf, int size);
} vlib_ep_cbs_t;

typedef struct vlib_ep_s {
    vlib_transp_t *th;
    vlib_ep_cbs_t iface;
    size_t sends_posted, recvs_posted;
    size_t sends_compl, recvs_compl;
} vlib_ep_t;

typedef struct vlib_msg_s {
    vlib_msg_type_t type;
    vlib_buf_t *buf;
    vlib_ep_t *eph;
    int offs, size;
    int complete;
    size_t seq_num;
} vlib_msg_t;

/* Context init */
vlib_hndl_t *vlib_ctx_create(char *dev_name, int port);
int vlib_ctx_destroy(vlib_hndl_t *vh);

vlib_shres_t *vlib_res_create(vlib_hndl_t *vh, int cq_size, int srq_size);
int vlib_res_destroy(vlib_shres_t *rh);

vlib_buf_t *vlib_buf_reg(vlib_hndl_t *vh, char *buf, size_t size);
int vlib_buf_dereg(vlib_buf_t *bh);


vlib_transp_t *vlib_transp_create(vlib_hndl_t *h, vlib_transp_type_t t,
                                  vlib_transp_t *rh, int dflt_compl);
int vlib_transp_destroy(vlib_transp_t *t);

vlib_ep_t *vlib_ep_create(vlib_transp_t *th);
vlib_addr_t *vlib_ep_addr(vlib_ep_t *eph);
vlib_ep_t *vlib_ep_connect(vlib_ep_t *eph, vlib_addr_t *addr);
int vlib_ep_disconnect(vlib_ep_t *ep);
vlib_msg_t *vlib_ep_send(vlib_ep_t *ep, vlib_buf_t *buf,
                         ssize_t offset, size_t size);
vlib_msg_t *vlib_ep_recv(vlib_ep_t *ep, vlib_buf_t *buf,
                         ssize_t offset, size_t size);
vlib_wait(vlib_msg_t **msgs, int count);

#endif // VLIB_H
