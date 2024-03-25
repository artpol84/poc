#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arch.h"
#include "platform.h"
#include "utils.h"
#include "ucx_mem_bw.h"

/* ------------------------------------------------------
 * UCX BW test replication 
 * ------------------------------------------------------ */
typedef struct {
    size_t buf_size;
} memcpy_data_t;

typedef struct {
    size_t size;
    void *src, *dst;
} memcpy_priv_data_t;


int cb_ucx_mem_bw_init_priv(void *in_data, void **priv_data)
{
    memcpy_priv_data_t *priv;

    priv = calloc(1, sizeof(*priv));
    assert(priv);
    priv->size = ((memcpy_data_t*)in_data)->buf_size;
    priv->src = calloc(priv->size, 1);
    assert(priv->src);
    priv->dst = calloc(priv->size, 1);
    assert(priv->dst);

    memset(priv->src, 1, priv->size);
    memset(priv->dst, 1, priv->size);
    memcpy(priv->src, priv->dst, priv->size);

    *priv_data = priv;
    return 0;
}

int cb_ucx_mem_bw_fini_priv(void *priv_data)
{
    memcpy_priv_data_t *priv = priv_data;

    free(priv->src);
    free(priv->dst);
    free(priv);

    return 0;
}

static int cb_ucx_mem_bw_run(void *priv_data)
{
    memcpy_priv_data_t *priv = priv_data;
    memcpy(priv->src, priv->dst, priv->size);
    return 0;
}

exec_callbacks_t ucx_mem_bw_cbs = {
    .priv_init = cb_ucx_mem_bw_init_priv,
    .priv_fini = cb_ucx_mem_bw_fini_priv,
    .run = cb_ucx_mem_bw_run
};

static int
exec_one(cache_struct_t *cache, exec_infra_desc_t *desc, size_t bsize)
{
    memcpy_data_t data;
    uint64_t *ticks;
    uint64_t niter;
    int ret;

    /* Initialize test data */
    ticks = calloc(exec_get_ctx_cnt(desc), sizeof(ticks[0]));
    data.buf_size = bsize;

    ret = exec_loop(desc, &ucx_mem_bw_cbs, &data, &niter, ticks);
    if (ret) {
        return ret;
    }
    
    exec_log_data(cache, desc, bsize, bsize * 2, niter, ticks);

    free(ticks);

    return 0;
}

int run_ucx_mem_bw(cache_struct_t *cache, exec_infra_desc_t *desc)
{
    int i, ret;

    exec_log_hdr();


    if (desc->focus_size > 0) {
        return exec_one(cache, desc, desc->focus_size);
    }

    for (i = 0; i < cache->nlevels; i++)
    {
        /* get 90% of the cache size */
        size_t wset = cache->cache_sizes[i] - cache->cache_sizes[i] / 10;
        ret += exec_one(cache, desc, wset/2);
        if (ret) {
            return ret;
        }
    }

    return 0;
}
