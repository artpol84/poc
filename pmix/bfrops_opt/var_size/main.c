#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

int pack_size(uint64_t size, uint8_t out_buf[9])
{
    uint64_t tmp = size;
    int idx = 0;
    while(tmp && idx < 8) {
        uint8_t val = tmp % 128;
        tmp /= 128;
        if( UNLIKELY(tmp) ) {
            val += 128;
        }
        out_buf[idx++] = val;
    }

    /* If we have leftover (VERY unlikely) */
    if (UNLIKELY(8 == idx && tmp)) {
        out_buf[idx++] = tmp;
    }
    return idx;
}

uint64_t unpack_size(uint8_t in_buf[])
{
    uint64_t size = 0, multiplyer = 1;
    int idx = 0;
    uint8_t val = 0;
    do {
        val = in_buf[idx++];
        size = size + multiplyer * (val % 128);
        multiplyer *= 128;
    } while( UNLIKELY((val / 128) && (idx < 8)) );

    /* If we have leftover (VERY unlikely) */
    if (UNLIKELY(8 == idx && (val / 128))) {
        val = in_buf[idx++];
        size = size + multiplyer * val;
    }
    return size;
}


int main()
{
    uint64_t size;

    for(size = (uint64_t)(-1) - 10000000000; size < (uint64_t)(-1); size++) {
        uint8_t buf[9];
        int s = pack_size(size, buf);
        uint64_t nsize = unpack_size(buf);
        if( nsize != size){
            abort();
        }
	if( 0 == (size%100000) ){
	    printf("size = %lu\n", size);
	}
    }
    printf("OK\n");
    return 0;
}
