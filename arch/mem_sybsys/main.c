#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arch.h"
#include "mem_sys.h"
#include "utils.h"

#define NACCESSES 16

int nlevels;
size_t cl_size;
size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX];
double min_run_time = 1; /* 1 sec */


/* ------------------------------------------------------
 * UCX BW test replication 
 * ------------------------------------------------------ */
typedef struct {
    size_t buf_size;
    void *src, *dst;
} memcpy_data_t;

void cb_ucx_mem_bw(void *in_data)
{
    memcpy_data_t *data = (memcpy_data_t *)in_data;
    memcpy(data->src, data->dst, data->buf_size);
}

void run_ucx_mem_bw()
{
    memcpy_data_t data;
    int i;

    for (i = 0; i < nlevels; i++)
    {
        /* get 90% of the cache size */
        size_t wset = cache_sizes[i] - cache_sizes[i] / 10;
        uint64_t ticks;
        uint64_t niter;

        data.buf_size = wset/2;
        data.src = calloc(data.buf_size, 1);
        data.dst = calloc(data.buf_size, 1);

        memset(data.src, 1, data.buf_size);
        memset(data.dst, 1, data.buf_size);
        memcpy(data.src, data.dst, data.buf_size);

        exec_loop(min_run_time, cb_ucx_mem_bw, (void*)&data, &niter, &ticks);
        
        printf("[%d]:\twset=%zd, bsize=%zd, niter=%llu, ticks=%llu, %lf MB/sec\n",
                i, wset, data.buf_size, niter, ticks,
                (data.buf_size * niter)/(ticks/clck_per_sec())/1e6);
        
        free(data.src);
        free(data.dst);

    }
}


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
    void                                                                \
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

void run_buf_strided_access(size_t stride, exec_loop_cb_t *cb)
{
    baccess_cbdata_t data;
    int i;

    for (i = 0; i < nlevels; i++)
    {
        /* get 90% of the cache size */
        size_t wset = ROUND_UP((cache_sizes[i] - cache_sizes[i] / 10), cl_size);
        uint64_t ticks;
        uint64_t niter;
        size_t esize = sizeof(uint64_t);
        int j;

        for (j = 0; j < 2; j++) {
            data.buf_size = wset / (1 + !!j);
            data.stride = ROUND_UP(stride, esize);
            data.buf = calloc(data.buf_size, 1);

            memset(data.buf, 1, data.buf_size);

            exec_loop(min_run_time, cb, (void *)&data, &niter, &ticks);
            printf("[%d]:\twset=%zd, bsize=%zd, niter=%llu, ticks=%llu, %lf MB/sec\n",
                   i, wset, data.buf_size, niter, ticks,
                   (data.buf_size * esize * niter) / (ticks / clck_per_sec()) / 1e6);
            free(data.buf);
        }
    }
}

int main()
{
    char *buf = NULL;
    int i, j;
    volatile uint64_t sum, val = 4;
    cl_size = cache_line_size();

    printf("Freuency: %lf\n", clck_per_sec());
    discover_caches(&nlevels, cache_sizes);

    /* In case discovery failed - use some default values */
    if (!nlevels) {
        printf("Cache subsystem discovery failed - use defaults\n");
        nlevels = 4;
        cache_sizes[0] = 32*1024;
        cache_sizes[1] = 1024*1024;
        cache_sizes[2] = 32*1024*1024;
        cache_sizes[3] = cache_sizes[2] * 8;
        cl_size = 64;
    }

    printf("\n");
    printf("===================================\n");
    printf("Recreate UCX memory BW performance:\n\n");
    run_ucx_mem_bw();


    printf("\n");
    printf("===================================\n");
    printf("Performance of '=' operation\n\n");

    printf("Loop unroll = 1\n");

    printf("Sequential access to a buffer:\n");
    run_buf_strided_access(1, BACCESS_CB_NAME(assign, uint64_t, 1));

    printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    run_buf_strided_access(cl_size, BACCESS_CB_NAME(assign, uint64_t, 1));

    printf("Loop unroll = 8\n");

    printf("Sequential access to a buffer:\n");
    run_buf_strided_access(1, BACCESS_CB_NAME(assign, uint64_t, 8));

    printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    run_buf_strided_access(cl_size, BACCESS_CB_NAME(assign, uint64_t, 8));


    printf("\n");
    printf("===================================\n");
    printf("Performance of '+=' operation\n\n");

    printf("Loop unroll = 1\n");

    printf("Sequential access to a buffer:\n");
    run_buf_strided_access(1, BACCESS_CB_NAME(inc, uint64_t, 1));

    printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    run_buf_strided_access(cl_size, BACCESS_CB_NAME(inc, uint64_t, 1));

    printf("Loop unroll = 8\n");

    printf("Sequential access to a buffer:\n");
    run_buf_strided_access(1, BACCESS_CB_NAME(inc, uint64_t, 8));

    printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    run_buf_strided_access(cl_size, BACCESS_CB_NAME(inc, uint64_t, 8));

    printf("\n");
    printf("===================================\n");
    printf("Performance of '*=' operation\n\n");

    printf("Loop unroll = 1\n");

    printf("Sequential access to a buffer:\n");
    run_buf_strided_access(1, BACCESS_CB_NAME(mul, uint64_t, 1));

    printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    run_buf_strided_access(cl_size, BACCESS_CB_NAME(mul, uint64_t, 1));

    printf("Loop unroll = 8\n");

    printf("Sequential access to a buffer:\n");
    run_buf_strided_access(1, BACCESS_CB_NAME(mul, uint64_t, 8));

    printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    run_buf_strided_access(cl_size, BACCESS_CB_NAME(mul, uint64_t, 8));


#if 0
    /* Allocate data buffer */
    buf = calloc(cache_sizes[nlevels - 1], 1);

    printf("Cache performance (strided access):\n");
    for (i = 0; i < nlevels; i++)
    {
        /* get 90% of the cache size */
        size_t wset = cache_sizes[i] - cache_sizes[i] / 10;
        uint64_t ts = rdtsc();
        for (j = 0; j < NACCESSES; j++)
        {
//            memset(buf, j, wset);
            size_t k,l;
            
            for(k = 0; k < cl_size; k++){
        	for(l = 0; l < wset / cl_size; l++){
                    buf[l * cl_size + k] += 1;
                }
            }
        }
        ts = rdtsc() - ts;
        
        for(j = 0; j < wset; j++){
            sum += buf[j];
        }
        
        printf("[%d]:\twset=%zd, ts=%llu, %llu cyc/word, %lf cyc/byte, %lf MB/sec\n",
                i, wset, ts, ts / (wset/8) / NACCESSES, (double)ts / wset / NACCESSES,
                (wset * NACCESSES)/(ts/clck_per_sec())/1e6);
    }
    

    printf("Cache performance (sequential access):\n");
    for (i = 0; i < nlevels; i++)
    {
        /* get 90% of the cache size */
        size_t wset = cache_sizes[i] - cache_sizes[i] / 10;
        uint64_t ts = rdtsc();
        for (j = 0; j < NACCESSES; j++)
        {
            memset(buf, j, wset);
        }
        ts = rdtsc() - ts;
        
        for(j = 0; j < wset; j++){
            sum += buf[j];
        }
        
        printf("[%d]:\twset=%zd, ts=%llu, %llu cyc/word, %lf cyc/byte, %lf MB/sec\n",
                i, wset, ts, ts / (wset/8) / NACCESSES, (double)ts / wset / NACCESSES,
                (wset * NACCESSES)/(ts/clck_per_sec())/1e6);
    }
#endif 


    return 0;
}
