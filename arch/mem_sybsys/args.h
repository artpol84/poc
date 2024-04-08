#ifndef MEM_SUBS_ARGS_H
#define MEM_SUBS_ARGS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "ucx_mem_bw.h"

typedef struct common_args_s {
    double run_time;
    int min_iter;
    char *test, *modifier;
    ssize_t buf_size_focus;
    int get_cache_info;
    uint32_t nthreads;
    int bind_not_a_fail;
    int debug;
    int quiet;
} common_args_t;

void args_common_usage(char *cmd);
void args_common_process(int argc, char **argv, common_args_t *args);

#endif