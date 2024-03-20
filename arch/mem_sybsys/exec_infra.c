#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"
#include "exec_infra.h"

#define MAX_TESTS 128
struct {
    char *name;
    char *descr;
    run_test_t *cb;
} test_list[MAX_TESTS];
int test_list_size = 0;


int exec_loop(exec_infra_desc_t *desc, exec_loop_cb_t *cb, void *data,
                uint64_t *out_iter, uint64_t *out_ticks)
{
    int i, niter = 0;
    int ret = 0;
    uint64_t start, end;
    double cps = clck_per_sec();

    /* estimate # of iterations */
    start = rdtsc();
    do {
        ret += cb(data);
        end = rdtsc();
        niter++;
    } while( (end - start)/cps < desc->run_time || niter < desc->min_iter);

    if (ret) {
        return ret;
    }

    /* Run the main measurement loop */
    start = rdtsc();
    for(i = 0; i < niter; i++) {
        ret += cb(data);
    }
    end = rdtsc();

    if (ret) {
        return ret;
    }
    
    *out_iter = niter;
    *out_ticks = end - start;
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

int tests_exec(cache_struct_t *cache, char *name, exec_infra_desc_t *desc)
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
