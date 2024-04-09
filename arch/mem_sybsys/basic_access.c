#include <string.h>

#include "basic_access.h"

/* ------------------------------------------------------
 * Strided access pattern 
 * ------------------------------------------------------ */

typedef struct {
    size_t stride;
    size_t buf_size;
} baccess_data_t;

typedef struct {
    baccess_data_t *data;
    void *buf;
} baccess_priv_data_t;


static int cb_baccess_init_priv(void *in_data, void **priv_data)
{
    baccess_priv_data_t *priv;

    priv = calloc(1, sizeof(*priv));
    assert(priv);
    priv->data = in_data;
    priv->buf = calloc(priv->data->buf_size, 1);
    assert(priv->buf);

    memset(priv->buf, 1, priv->data->buf_size);

    *priv_data = priv;
    return 0;
}

static int cb_baccess_fini_priv(void *priv_data)
{
     baccess_priv_data_t *priv = priv_data;

    free(priv->buf);
    free(priv);

    return 0;
}

exec_callbacks_t baccess_cbs = {
    .priv_init = cb_baccess_init_priv,
    .priv_fini = cb_baccess_fini_priv,
    .run = NULL
};

#define IDX_FORMULA(base_idx, stride, idx)  \
    ((base_idx) * (stride) + (k))

#define BUF_ACCESS_DATA_NAME(type)          \
    baccess_data_ ## type

#define BUF_ACCESS_DATA_DECL(type)  \
    typedef struct {                \
    size_t stride;                  \
    size_t buf_size;                \
    type *buf;                      \
} BUF_ACCESS_DATA_NAME(type)

#define BACCESS_CB_NAME(prefix, type, unroll_macro)     \
    cb_baccess_ ## prefix ## _ ## type ## _ ## unroll_macro

#define BACCESS_CB_DEFINE_HEADER(prefix, type, unroll_ratio)        \
    /* Define the callback */                                           \
    int                                                                \
    BACCESS_CB_NAME(prefix, type, unroll_ratio) (void *in_data)     \
    {                                                                   \
        baccess_priv_data_t *priv_data = in_data;                       \
        type *buf = (type*)priv_data->buf;                              \
        size_t esize = sizeof(type);                                    \
        size_t ecnt = priv_data->data->buf_size / esize;                \
        size_t stride = priv_data->data->stride / esize;                \
        size_t k, l;                                                    \
        for (k = 0; k < stride; k++)                                    \
        {                                                               \
            size_t inn_lim = ecnt / stride;                             \
            for (l = 0; (l + unroll_ratio - 1) < inn_lim;               \
                                            l += unroll_ratio) {    

#define BACCESS_CB_DEFINE_FOOTER                                        \
            }                                                           \
        }                                                               \
        return 0;                                                       \
    }

#define BACCESS_CB_1(prefix, type, op, val)         \
    BACCESS_CB_DEFINE_HEADER(prefix, type, 1)       \
    DO_1(op, buf, IDX_FORMULA(l, stride, k), val);  \
    BACCESS_CB_DEFINE_FOOTER

#define BACCESS_CB_2(prefix, type, op, val)         \
    BACCESS_CB_DEFINE_HEADER(prefix, type, 2)       \
    DO_2(op, buf, IDX_FORMULA(l, stride, k), val);  \
    BACCESS_CB_DEFINE_FOOTER

#define BACCESS_CB_4(prefix, type, op, val)         \
    BACCESS_CB_DEFINE_HEADER(prefix, type, 4)       \
    DO_4(op, buf, IDX_FORMULA(l, stride, k), val);  \
    BACCESS_CB_DEFINE_FOOTER

#define BACCESS_CB_8(prefix, type, op, val)         \
    BACCESS_CB_DEFINE_HEADER(prefix, type, 8)       \
    DO_8(op, buf, IDX_FORMULA(l, stride, k), val);  \
    BACCESS_CB_DEFINE_FOOTER

#define BACCESS_CB_16(prefix, type, op, val)        \
    BACCESS_CB_DEFINE_HEADER(prefix, type, 16)      \
    DO_16(op, buf, IDX_FORMULA(l, stride, k), val); \
    BACCESS_CB_DEFINE_FOOTER

#if 0
#define BACCESS_CB(prefix, type, op, val, ...)      \
    BACCESS_CB_DEFINE_HEADER(prefix, type, 1)       \
    DO_##__VA_ARGS__(op, buf, IDX_FORMULA(l, stride, k), val);  \
    BACCESS_CB_DEFINE_FOOTER


// BACCESS_CB_1(assign, uint64_t, =, 0xFFFFFFFFL)
BACCESS_CB(assign, uint64_t, =, 0xFFFFFFFFL, 1)
BACCESS_CB(assign, uint64_t, =, 0xFFFFFFFFL, 2)
BACCESS_CB(assign, uint64_t, =, 0xFFFFFFFFL, 4)
BACCESS_CB(assign, uint64_t, =, 0xFFFFFFFFL, 8)
BACCESS_CB(assign, uint64_t, =, 0xFFFFFFFFL, 16)
#endif

BACCESS_CB_1(assign, uint64_t, =, 0xFFFFFFFFL)
BACCESS_CB_2(assign, uint64_t, =, 0xFFFFFFFFL)
BACCESS_CB_4(assign, uint64_t, =, 0xFFFFFFFFL)
BACCESS_CB_8(assign, uint64_t, =, 0xFFFFFFFFL)
BACCESS_CB_16(assign, uint64_t, =, 0xFFFFFFFFL)

BACCESS_CB_1(inc, uint64_t, +=, 0x16)
BACCESS_CB_2(inc, uint64_t, +=, 0x16)
BACCESS_CB_4(inc, uint64_t, +=, 0x16)
BACCESS_CB_8(inc, uint64_t, +=, 0x16)
BACCESS_CB_16(inc, uint64_t, +=, 0x16)

BACCESS_CB_1(mul, uint64_t, *=, 0x16)
BACCESS_CB_2(mul, uint64_t, *=, 0x16)
BACCESS_CB_4(mul, uint64_t, *=, 0x16)
BACCESS_CB_8(mul, uint64_t, *=, 0x16)
BACCESS_CB_16(mul, uint64_t, *=, 0x16)

typedef enum {
    OP_ASSIGN, OP_INC, OP_MUL
} baccess_t;


static int
exec_one(cache_struct_t *cache, exec_infra_desc_t *desc, size_t bsize, 
        size_t stride, size_t esize, exec_loop_cb_t *cb)
{
    uint64_t *ticks;
    uint64_t niter;
    baccess_data_t data;
    int ret;

    /* Initialize private test data */
    ticks = calloc(exec_get_ctx_cnt(desc), sizeof(ticks[0]));
    data.buf_size = bsize;
    data.stride = ROUND_UP(stride, esize);
    baccess_cbs.run = cb;

    ret = exec_loop(desc, &baccess_cbs, (void *)&data, &niter, ticks);
    if (ret) {
        return ret;
    }
    
    exec_log_data(cache, desc, bsize, bsize, niter, ticks);
    
    return 0;
}

int run_buf_strided_access(cache_struct_t *cache, exec_infra_desc_t *desc)
{
    int i;
    size_t esize = sizeof(uint64_t);
    size_t stride = 1;
    int unroll = 1;
    baccess_t op_type = OP_ASSIGN;
    exec_loop_cb_t *cb = NULL;
    char *tok, *tok_h = NULL;
    char op_name[256];

    tok = strtok_r(desc->test_arg, ":", &tok_h);
    if (tok) {
        if (!strcmp(tok, "a")) {
            op_type = OP_ASSIGN;
        } else if (!strcmp(tok, "i")) {
            op_type = OP_INC;
        } else if (!strcmp(tok, "m")) {
            op_type = OP_MUL;
        } else if (strcmp(tok, "")) {
            printf("%s: unknown operation type modifier '%s'\n",
                    TEST_CACHE, tok);
            return -1;
        }
        strcpy(op_name, tok);
    }

    tok = strtok_r(NULL, ":", &tok_h);
    if (tok) {
        if (atoi(tok)) {
            stride = atoi(tok);
        }
    }

    tok = strtok_r(NULL, ":", &tok_h);
    if (tok) {
        if (atoi(tok)) {
            unroll = atoi(tok);
        }
    }

    if (!desc->quiet) {
        printf("\t[modifiers: op=%s(%d), stride=%zd, unroll=%d]\n",
                op_name, op_type, stride, unroll);
    }

    switch(op_type) {
    case OP_ASSIGN:
        switch(unroll) {
        case 1:
            cb = BACCESS_CB_NAME(assign, uint64_t, 1);
            break;    
        case 2:
            cb = BACCESS_CB_NAME(assign, uint64_t, 2);
            break;    
        case 4:
            cb = BACCESS_CB_NAME(assign, uint64_t, 4);
            break;    
        case 8:
            cb = BACCESS_CB_NAME(assign, uint64_t, 8);
            break;    
        case 16:
            cb = BACCESS_CB_NAME(assign, uint64_t, 16);
            break;    
        }
        break;
    case OP_INC:
        switch(unroll) {
        case 1:
            cb = BACCESS_CB_NAME(inc, uint64_t, 1);
            break;    
        case 2:
            cb = BACCESS_CB_NAME(inc, uint64_t, 2);
            break;    
        case 4:
            cb = BACCESS_CB_NAME(inc, uint64_t, 4);
            break;    
        case 8:
            cb = BACCESS_CB_NAME(inc, uint64_t, 8);
            break;    
        case 16:
            cb = BACCESS_CB_NAME(inc, uint64_t, 16);
            break;    
        }
        break;
    case OP_MUL:
        switch(unroll) {
        case 1:
            cb = BACCESS_CB_NAME(mul, uint64_t, 1);
            break;    
        case 2:
            cb = BACCESS_CB_NAME(mul, uint64_t, 2);
            break;    
        case 4:
            cb = BACCESS_CB_NAME(mul, uint64_t, 4);
            break;    
        case 8:
            cb = BACCESS_CB_NAME(mul, uint64_t, 8);
            break;    
        case 16:
            cb = BACCESS_CB_NAME(mul, uint64_t, 16);
            break;    
        }
        break;
    }

    exec_log_hdr(desc);

    if (desc->focus_size > 0) {
        return exec_one(cache, desc,desc->focus_size, stride, esize, cb);
    }

    for (i = 0; i < cache->nlevels; i++)
    {
        /* get 90% of the cache size */
        size_t wset = ROUND_UP((cache->cache_sizes[i] - cache->cache_sizes[i] / 10), cache->cl_size);
        int j;
        int ret;

        for (j = 0; j < 2; j++) {
            ret = exec_one(cache, desc, wset / (1 + !!j), stride, esize, cb);
            if (ret) {
                return ret;
            }
        }
    }
    return 0;
}