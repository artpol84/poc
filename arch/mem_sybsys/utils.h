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

#define DO_1(op, buf, base_idx, val)        \
    buf[base_idx] op val;

#define DO_2(op, buf, base_idx, val)        \
    DO_1(op, buf, (base_idx), val);         \
    DO_1(op, buf, (base_idx) + 1, val);

#define DO_4(op, buf, base_idx, val)        \
    DO_2(op, buf, base_idx, val);           \
    DO_2(op, buf, (base_idx) + 2, val);

#define DO_8(op, buf, base_idx, val)        \
    DO_4(op, buf, (base_idx), val);         \
    DO_4(op, buf, (base_idx + 4), val);

#define DO_16(op, buf, base_idx, val)       \
    DO_8(op, buf, (base_idx), val);         \
    DO_8(op, buf, (base_idx + 8), val);

#endif