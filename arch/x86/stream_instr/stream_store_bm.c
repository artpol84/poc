#include <immintrin.h>
#include <math.h> 
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

double ts(){
    struct timespec ts;
    double ret;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec + 1E-9 * ts.tv_nsec);
}

#define ELEM_SIZE (256 /*bits*/ / 8 /* bits per byte */ )

int main(int argc, char **argv) {

    double start, stop;
    char *input, *output;          // data pointers
    __m256i avx_in, *avx_out; // variables for AVX
    int i, j;

    size_t store_len = atoi(argv[1]) * 2;

    // allocate memory
    input = (char*) _mm_malloc (ELEM_SIZE,32);
    output = (char*) _mm_malloc (ELEM_SIZE * store_len,32);

    // initialize vectors //
    for(i=0;i<ELEM_SIZE;i++) {
        input[i] = i + 1;
    }

    avx_in = *((__m256i*)input);//_mm256_stream_load_si256((__m256i*)input);
    avx_out = (__m256i*)output;

    for(j=0; j<10000; j++) {
        for(i = 0; i < store_len; i++) {
            _mm256_stream_si256(avx_out + i, avx_in);
        }

        __asm ("sfence": : : "memory");
	struct timespec req = { 0, 100000 };
        nanosleep(&req, NULL);
    }

    _mm_free(input);
    _mm_free(output);

    return 0;
}
