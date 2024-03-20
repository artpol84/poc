#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arch.h"
#include "mem_sys.h"
#include "utils.h"
#include "ucx_mem_bw.h"

/* ------------------------------------------------------
 * UCX BW test replication 
 * ------------------------------------------------------ */
typedef struct {
    size_t buf_size;
    void *src, *dst;
} memcpy_data_t;

int cb_ucx_mem_bw(void *in_data)
{
    memcpy_data_t *data = (memcpy_data_t *)in_data;
    memcpy(data->src, data->dst, data->buf_size);
    return 0;
}

static int
exec_one(cache_struct_t *cache, exec_infra_desc_t *desc, size_t bsize)
{
    memcpy_data_t data;
    int ret;
    uint64_t ticks;
    uint64_t niter;
    /* As we are copying from one and saving to another, the footprint is 2x */
    int level = caches_detect_level(cache, bsize * 2);

    data.buf_size = bsize;
    data.src = calloc(data.buf_size, 1);
    data.dst = calloc(data.buf_size, 1);

    memset(data.src, 1, data.buf_size);
    memset(data.dst, 1, data.buf_size);
    memcpy(data.src, data.dst, data.buf_size);

    ret = exec_loop(desc, cb_ucx_mem_bw, (void*)&data, &niter, &ticks);
    if (ret) {
        return ret;
    }
    
    log_output(level, data.buf_size, bsize * 2, niter, ticks);
    
    free(data.src);
    free(data.dst);
    return 0;
}

int run_ucx_mem_bw(cache_struct_t *cache, exec_infra_desc_t *desc)
{
    int i, ret;

    log_header();


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
