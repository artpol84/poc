#ifndef API_COMMON_H
#define API_COMMON_H

#include "api.h"

typedef struct {
    api_request_t *buffer;
    int size;
    int tail, head;
} req_fifo_t;

void api_setup_req(api_request_t *req, char *buf, int size, void *priv);
void api_req_bcopy(api_request_t *req, char *out_buf, int size);
static inline int api_req_complete(api_request_t *req)
{
    return req->offset == req->size;
}
void api_post_buffer();
#endif // API_COMMON_H
