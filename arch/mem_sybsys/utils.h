#ifndef UTILS_H
#define UTILS_H

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
    })

#define ROUND_UP(x, b) (((x / b) + !!(x % b)) * b)

typedef void (exec_loop_cb_t)(void *data);
void exec_loop(double min_time, exec_loop_cb_t *cb, void *data,
                uint64_t *out_iter, uint64_t *out_ticks);


#define DO_2(x, idx_formula, base_idx)   \
    x[idx_formula(base_idx)] += 1;       \
                                         \
    x[idx_formula(base_idx + 1)] += 1;   \

#define DO_4(x, idx_formula, base_idx)  \
    DO_2(x,idx_formula,(base_idx));       \
    DO_2(x,idx_formula,(base_idx) + 2);   \

#define DO_8(x, idx_formula, base_idx)  \
    DO_4(x,idx_formula,(base_idx));       \
    DO_4(x,idx_formula,(base_idx + 4));   \

#define DO_16(x, idx_formula, base_idx)  \
    DO_8(x,idx_formula,(base_idx));       \
    DO_8(x,idx_formula,(base_idx + 8));   \


#endif