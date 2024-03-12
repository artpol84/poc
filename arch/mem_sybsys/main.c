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
    uint64_t *buf;
} baccess_data_t;

void cb_strided_access(void *in_data)
{
    baccess_data_t *data = (baccess_data_t*)in_data;
    size_t k, l;

    for (k = 0; k < data->stride; k++)
    {
        for (l = 0; l < data->buf_size / data->stride; l++)
        {
            data->buf[l * data->stride + k] += 1;
        }
    }
}

void run_buf_strided_access(size_t stride)
{
    baccess_data_t data;
    int i;

    for (i = 0; i < nlevels; i++)
    {
        /* get 90% of the cache size */
        size_t wset = ROUND_UP((cache_sizes[i] - cache_sizes[i] / 10), cl_size);
        uint64_t ticks;
        uint64_t niter;
        size_t esize = sizeof(data.buf[0]);

        data.buf_size = wset / esize;
        data.stride = ROUND_UP(stride, esize) / esize;
        data.buf = calloc(data.buf_size, esize);

        memset(data.buf, 1, data.buf_size);

        exec_loop(min_run_time, cb_strided_access, (void*)&data, &niter, &ticks);
        
        printf("[%d]:\twset=%zd, bsize=%zd, niter=%llu, ticks=%llu, %lf MB/sec\n",
                i, wset, data.buf_size, niter, ticks,
                (data.buf_size * esize * niter)/(ticks/clck_per_sec())/1e6);
        
        free(data.buf);
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
    }

    printf("Recreate UCX memory BW performance:\n");
    run_ucx_mem_bw();

    printf("Sequential access to a buffer:\n");
    run_buf_strided_access(1);

    printf("Strided access to a buffer (jumping over to the next cache line every access):\n");
    run_buf_strided_access(cl_size);


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
