#ifndef TEST_INFRA_H
#define TEST_INFRA_H

#include "mem_sys.h"

typedef struct {
    double run_time;
    char *test_arg;
    ssize_t focus_size;
} exec_infra_desc_t;

typedef int (run_test_t)(cache_struct_t *cache, exec_infra_desc_t *desc);
void tests_reg(char *name, char *descr, run_test_t *cb);
void tests_print();
int tests_exec(cache_struct_t *cache, char *name, exec_infra_desc_t *desc);

#endif