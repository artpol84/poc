#ifndef BASIC_RC_H
#define BASIC_RC_H

#include "basic_ib.h"

int create_qp(ib_qp_t *qp, ib_cq_t *cq);
void destroy_qp(ib_qp_t *qp);
int connect_qp(ib_qp_t *from_qp, ib_qp_t *to_qp);

#if 0
int send_rc(ib_context_t *ctx, int size);
#endif

#endif // BASIC_RC_H
