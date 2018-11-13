#include <stdio.h>
#include <stdint.h>

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

#define NITER 1000000
int main(int argc, char **argv)
{
    int num_stores, i;
    uint64_t ts, time;
    //count_init

    // warmup
    for(i = 0; i < 100; i++) {
        //asm_code
    }


    ts = rdtsc();
    for(i = 0; i < NITER; i++) {
        //asm_code
    }
    time = rdtsc() - ts;
    printf("%d:\t%lu\n", store_cnt, time / NITER);
    return 0;
}