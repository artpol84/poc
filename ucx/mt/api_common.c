#include "api_common.h"

#define min(a, b) ( (a > b) ? b : a)

void api_setup_req(bm_request_t *req, char *buf, int size, void *priv)
{
    req->buf = buf;
    req->size = size;
    req->priv_data = priv;
    req->offset = 0;
}

void api_req_bcopy(bm_request_t *req, char *out_buf, int size)
{
    size_t cpy_size = min(req->size - req->offset, size);
    memcpy(req->buf, out_buf, cpy_size);
    req->offset += cpy_size;
}

void api_post_buffer() {}
