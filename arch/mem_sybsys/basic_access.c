#include <string.h>

#include "basic_access.h"

/* ------------------------------------------------------
 * Strided access pattern 
 * ------------------------------------------------------ */

typedef struct {
    size_t stride;
    size_t buf_size;
    void *buf;
} baccess_cbdata_t;


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
        baccess_cbdata_t *data = (baccess_cbdata_t*)in_data;            \
        type *buf = (type*)data->buf;                                   \
        size_t esize = sizeof(type);                                    \
        size_t ecnt = data->buf_size / esize;                           \
        size_t stride = data->stride / esize;                           \
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
    uint64_t ticks;
    uint64_t niter;
    baccess_cbdata_t data;
    int ret;
    int level = caches_detect_level(cache, bsize);
    
    data.buf_size = bsize;
    data.stride = ROUND_UP(stride, esize);
    data.buf = calloc(data.buf_size, 1);

    memset(data.buf, 1, data.buf_size);

    ret = exec_loop(desc, cb, (void *)&data, &niter, &ticks);
    if (ret) {
        return ret;
    }
    log_output(level, data.buf_size, bsize, niter, ticks);

    free(data.buf);
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

    printf("\t[modifiers: op=%s(%d), stride=%zd, unroll=%d]\n",
            op_name, op_type, stride, unroll);

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

    log_header();

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