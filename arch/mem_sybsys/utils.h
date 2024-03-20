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


static inline
void log_header()
{
    printf("[level]    [buf size]    [work set]  [iterations]       [ticks]   [BW (MB/s)]\n");
}

static inline
void log_output(int clevel, size_t bsize, size_t wset, uint64_t niter, uint64_t ticks)
{
    printf("[%5d]%14zd%14zd%14llu%14llu%14.1lf\n",
            clevel, bsize, wset, niter, ticks,
                (bsize * niter)/(ticks/clck_per_sec())/1e6);
}

#endif