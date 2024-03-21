#ifndef BASIC_ACCESS_H
#define BASIC_ACCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arch.h"
#include "platform.h"
#include "utils.h"
#include "exec_infra.h"

#define TEST_CACHE "cache:strided"
#define TEST_CACHE_DESCR \
    "Evaluate cahce performance. Modifier: <op>:<st>:<un> where:\n"     \
    " * <op> operation type: 'a' (assign), 'i' (increment) or "         \
    "'m' (multiply)\n"                                                  \
    " * <st> defines stride, can be any integer,"                       \
    " interesting values are 1 and 64 (cache line)\n"                   \
    " * <un> loop unrolling level, can be: 1, 2, 4, 8, 16"
int run_buf_strided_access(cache_struct_t *cache, exec_infra_desc_t *desc);

#endif