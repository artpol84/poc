#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int errno;
#include <errno.h>
#include <time.h>

#include "arch.h"
#include "exec_infra.h"

#define MAX_TESTS 128

#define GET_TS()                            \
 ({                                         \
    struct timespec ts;                     \
    double ret;                             \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9 * ts.tv_nsec;    \
    ret;                                    \
})

struct {
    char *name;
    char *descr;
    run_test_t *cb;
} test_list[MAX_TESTS];
int test_list_size = 0;

int exec_assign_res(cache_struct_t *cache, exec_infra_desc_t *desc, int nthreads)
{
    int i;

    desc->mt.nthreads = nthreads;
    desc->mt.ctxs = calloc(nthreads, sizeof(desc->mt.ctxs[0]));
    assert(desc->mt.ctxs);
    
    for(i = 0; i < nthreads; i++) {
        desc->mt.ctxs[i].ctx_id = i;
        desc->mt.ctxs[i].desc = desc;
        desc->mt.ctxs[i].core = &cache->topo.core_subs.cores[i];
        desc->mt.ctxs[i].user_data = NULL;
    }

    return 0;
}

#if 0
int exec_set_user_cb(exec_infra_desc_t *desc, exec_loop_cb_t *cb)
{
    desc->mt.cb = cb;
    return 0;
}
#endif

int exec_set_user_ctx(exec_infra_desc_t *desc, int ctx_id, void *data)
{
    desc->mt.ctxs[ctx_id].user_data = data;
    return 0;
}


void *exec_loop_one(void *data)
{
    exec_thread_ctx_t *ctx = (exec_thread_ctx_t*)data;
    exec_infra_desc_t *desc = ctx->desc;
    exec_mt_ctx_t *mt = &desc->mt;
    void *priv_data;
    uint64_t start, end;
    double cps = clck_per_sec();
    int i, niter = 0, min_iter;
    int ret = 0;
    ctx->status = 0;
    int barrier_no = 1;
    double start_ts, end_ts;

    /* bind ourself */
    if (desc->debug) {
        char cpuset_buf[1024];
        hwloc_bitmap_taskset_snprintf (cpuset_buf, sizeof(cpuset_buf), ctx->core->cpuset);
        printf("Bind ctx id %d to logical idx = %d, cpuset=%s\n", ctx->ctx_id, ctx->core->core_id, cpuset_buf);
    }

    ret = hwloc_set_cpubind(ctx->core->topo->topology, ctx->core->cpuset, HWLOC_CPUBIND_THREAD);
    if (ret) {
        static int report = 1;
        if (report &&  !desc->quiet) {
            printf("Failed to bind a thread to the cpuset: errno = %s!\n",
                strerror(errno));
            report = 0;
        }
        if (!desc->bind_not_a_fail) {
            ctx->status = ret;
            return NULL;
        }
        /* mask the failure and move on */
        ret = 0;
    }

    /* initialize private structure */
    mt->cb->priv_init(mt->user_data, &priv_data);

    /* Warmup before the measurement */

    for(i = 0; i < 10; i++) {
        ret += mt->cb->run(priv_data);
    }

    /* ensure all threads are ready to execute the warmup */
    barrier_wait(&mt->barrier, (barrier_no++) * mt->nthreads);

    /* estimate # of iterations */
    start_ts = GET_TS();
    start = rdtsc();
    do {
        ret += mt->cb->run(priv_data);
        end = rdtsc();
        niter++;
    } while( (end - start)/cps < desc->run_time || niter < desc->min_iter);
    end_ts = GET_TS();
    
    printf("rdtsc time = %lf, clock time = %lf\n",
            ((end - start)/cps), end_ts - start_ts );

    if (ret) {
        ctx->status = ret;
        return NULL;
    }
    ctx->out_iter = niter;

    /* ensure all threads are ready to execute the performance part */
    barrier_wait(&mt->barrier, (barrier_no++) * mt->nthreads);

    /* pick the largest iter */
    ctx->min_iter = niter;
    for(i = 0; i < mt->nthreads; i++) {
        if (ctx->min_iter > mt->ctxs[i].out_iter) {
            ctx->min_iter = mt->ctxs[i].out_iter;
        }
        if (niter < mt->ctxs[i].out_iter) {
            niter = mt->ctxs[i].out_iter;
        }
    }

    /* Run the main measurement loop */
    start = rdtsc();
    for(i = 0; i < niter; i++) {
        ret += mt->cb->run(priv_data);
    }
    end = rdtsc();

    if (ret) {
        ctx->status = ret;
        return NULL;
    }
    
    ctx->out_iter = niter;
    ctx->out_ticks = end - start;

    /* ensure all threads have completed the performance part */
    barrier_wait(&mt->barrier, (barrier_no++) * mt->nthreads);


    mt->cb->priv_fini(priv_data);

    return NULL;
}

int exec_loop(exec_infra_desc_t *desc,
              exec_callbacks_t *cbs, void *user_data,
              uint64_t *out_iter, uint64_t *out_ticks)
{
    exec_mt_ctx_t *mt = &desc->mt;
    pthread_t *threads;
    int i;
    int status = 0;
    
    /* set the user-defined callback for thread routine usage */
    mt->cb = cbs;
    mt->user_data = user_data;

    barrier_init(&mt->barrier, 0);

    /* Start all requested threads */
    threads = calloc(mt->nthreads, sizeof(threads[0]));
    for(i = 0; i < mt->nthreads; i++) {
        pthread_create(&threads[i], NULL, exec_loop_one, &mt->ctxs[i]);
    }

    /* Wait for the test to finish */
    for(i = 0; i < mt->nthreads; i++) {
        void *tmp;
        pthread_join(threads[i], &tmp);
    }


    for(i = 0; i < mt->nthreads; i++) {
        if (mt->ctxs[i].status) {
            printf("Thread %d failed with status %d\n", i, mt->ctxs[i].status);
            if (!status) {
                status = mt->ctxs[i].status;
            }
        }
    }

    if (status) {
        return status;
    }

    *out_iter = mt->ctxs[0].out_iter;
    for(i = 0; i < mt->nthreads; i++) {
        out_ticks[i] = mt->ctxs[i].out_ticks;
    }
    return 0;
}

void tests_reg(char *name, char *descr, run_test_t *cb)
{
    if (test_list_size >= MAX_TESTS) {
        abort();
    }

    test_list[test_list_size].name = name;
    test_list[test_list_size].descr = descr;
    test_list[test_list_size].cb = cb;
    test_list_size++;
}

int exec_test(cache_struct_t *cache, char *name, exec_infra_desc_t *desc)
{
    int i;
    run_test_t *cb = NULL;

    for( i = 0; i < test_list_size; i++) {
        if (!strcmp(name, test_list[i].name)) {
            cb = test_list[i].cb;
            break;
        }
    }
    if (!cb) {
        printf("Test '%s' wasn't found\n", name);
        return -1;
    }

    return cb(cache, desc);
}

void tests_print()
{
    int i;
    printf("[name]              [parameters]\n");

    for(i = 0; i < test_list_size; i++) {
        printf("%-20.20s%s\n", test_list[i].name, test_list[i].descr);
    }
}


void exec_log_data(cache_struct_t *cache, exec_infra_desc_t *desc, 
                    size_t bsize, size_t wset, uint64_t niter, uint64_t *ticks)
{
    exec_mt_ctx_t *mt = &desc->mt;
    int level = caches_detect_level(cache, wset);
    int i;
    uint64_t min_ticks, max_ticks, avg_ticks;

    min_ticks = ticks[0];
    max_ticks = ticks[0];
    avg_ticks = ticks[0];
    for(i = 1; i < mt->nthreads; i++) {
        if (min_ticks > ticks[i]) {
            min_ticks = ticks[i];
        }
        if (max_ticks < ticks[i]) {
            max_ticks = ticks[i];
        }
        avg_ticks += ticks[i];
    }

    printf("[%5d]%14zd%14zd%14llu%14llu%14llu%14llu%14.1lf\n",
            level, bsize, wset, niter, min_ticks, max_ticks, avg_ticks,
                (bsize * niter * mt->nthreads)/(max_ticks/clck_per_sec())/1e6);
}