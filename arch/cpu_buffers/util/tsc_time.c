#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})


uint64_t rdtsc() {
    uint64_t ts;
    asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
        "shl $32, %%rdx\n\t"  // Shift the upper bits left.
        "or %%rdx, %0"        // 'Or' in the lower bits.
        : "=a" (ts)
        : 
        : "rdx");
    return ts;
}


int main()
{
    struct timespec ts;
    double start_time, time;
    uint64_t start_tsc, cycles;
    
    start_time = GET_TS();
    start_tsc = rdtsc();
    sleep(1);
    time = GET_TS() - start_time;
    cycles = rdtsc() - start_tsc;
    
    printf("Time = %lf, cycles = %lu, sec/cycle = %lf, freq = %lf\n", time, cycles, time / cycles * 1E9, cycles/time/1E9);
    

}