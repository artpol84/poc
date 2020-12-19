#ifndef BASIC_TCP_H
#define BASIC_TCP_H

#include "basic_tcp.h"

int exch_tcp_send(ib_context_t *ctx);
int init_tcp_recv(ib_context_t *ctx);
int exch_tcp_recv(ib_context_t *ctx);

#endif // BASIC_TCP_H
