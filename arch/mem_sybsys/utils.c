#include <stdio.h>
#include <stdint.h>

#include "arch.h"
#include "utils.h"

int exec_loop(double min_time, exec_loop_cb_t *cb, void *data,
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
    } while( (end - start)/cps < min_time);

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

#if 0
/* A core loop that allows to try all kinds of optimizations */
void access(int afirst, int alast, int ifirst, int ilast, int iinc)
{
    int it, a;

    for(it = ifirst; it < ilast; it+=iinc){
        for(a = afirst; a < alast; a++) {
            data[a][it] = it;
        }
    }
}
#endif