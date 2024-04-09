#ifndef UCX_MEM_BW_H

#include "exec_infra.h"

#define TEST_UCX "ucx_mem_bw"
#define TEST_UCX_DESCR "UCX memory bandwidth test (no modifiers)"
int run_ucx_mem_bw(cache_struct_t *cache, exec_infra_desc_t *desc);

#define TEST_MBW "mbw_block"
#define TEST_MBW_DESCR "MBW block test replica (optionally, block size as a modifier)"
int run_mbw(cache_struct_t *cache, exec_infra_desc_t *desc);

#endif