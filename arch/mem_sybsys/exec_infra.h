#ifndef TEST_INFRA_H
#define TEST_INFRA_H

#include "platform.h"

#define USE_ALL_CORES ((uint32_t)(-1))

typedef int (exec_priv_init_cb_t)(void *data, void **priv_data);
typedef int (exec_priv_fini_cb_t)(void *priv_data);
typedef int (exec_loop_cb_t)(void *priv_data);
typedef struct {
    exec_priv_init_cb_t *priv_init;
    exec_priv_fini_cb_t *priv_fini;
    exec_loop_cb_t *run;
} exec_callbacks_t;

typedef struct exec_infra_desc_s exec_infra_desc_t;
typedef struct {
    int ctx_id;

    /* back-reference to main struct */
    exec_infra_desc_t *desc;

    /* individual resource assignment */
    topo_core_t *core;

    /* private timing results */
    int batch;
    uint64_t min_iter;
    uint64_t out_iter;
    uint64_t out_ticks;

    /* user-provided data */
    void *user_data;

    /* return status */
    int status;
} exec_thread_ctx_t;

typedef struct {
    uint32_t nthreads;
    volatile int barrier;
    exec_callbacks_t *cb;
    void *user_data;
    exec_thread_ctx_t *ctxs;
} exec_mt_ctx_t;

typedef struct exec_infra_desc_s {
    double run_time;
    int min_iter;
    char *test_arg;
    ssize_t focus_size;
    int bind_not_a_fail;
    int debug, quiet;
    exec_mt_ctx_t mt;
} exec_infra_desc_t;

/*  TODO: for DPU purpose it's fine to assing cores sequentially,
 *  for general case - some policy might be better (like one core from L2, L3, NUMA, etc..)
 */
int exec_assign_res(cache_struct_t *cache, exec_infra_desc_t *desc, int nthreads);
//int exec_set_user_cb(exec_infra_desc_t *desc, exec_loop_cb_t *cb);
static inline
int exec_get_ctx_cnt(exec_infra_desc_t *desc)
{
    return desc->mt.nthreads;
}
int exec_init_threads(exec_infra_desc_t *desc);
int exec_loop(exec_infra_desc_t *desc,
              exec_callbacks_t *cbs, void *user_data,
              uint64_t *out_iter, uint64_t *out_ticks);

typedef int (run_test_t)(cache_struct_t *cache, exec_infra_desc_t *desc);
void tests_reg(char *name, char *descr, run_test_t *cb);
void tests_print();
int exec_test(cache_struct_t *cache, char *name, exec_infra_desc_t *desc);

void exec_log_hdr(exec_infra_desc_t *desc);
void exec_log_data(cache_struct_t *cache, exec_infra_desc_t *desc, 
                    size_t bsize, size_t wset, uint64_t niter, uint64_t *ticks);

#endif