#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arch.h>
#include <mem_sys.h>

#define NACCESSES 16

int main()
{
    int nlevels;
    size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX];
    char *buf = NULL;
    int i, j;
    volatile uint64_t sum, val = 4;
    size_t cl_size = cache_line_size();

    discover_caches(&nlevels, cache_sizes);

    /* Allocate data buffer */
    buf = calloc(cache_sizes[nlevels - 1], 1);

    printf("Cache performance:\n");
    for (i = 0; i < nlevels; i++)
    {
        size_t wset = cache_sizes[i] - cache_sizes[i] / 10;
        uint64_t ts = rdtsc();
        for (j = 0; j < NACCESSES; j++)
        {
            /* get 90% of the cache size */
//            memset(buf, j, wset);
            size_t k,l;
            
            for(k = 0; k < cl_size; k++){
        	for(l = 0; l < wset / cl_size; l++){
                    buf[l * cl_size + k] += 1;
                }
            }
        }
        ts = rdtsc() - ts;
        
        for(j = 0; j < wset; j++){
            sum += buf[j];
        }
        
        printf("[%d]:\twset=%zd, ts=%llu, %llu cyc/word, %llu cyc/byte, %llu B/sec\n",
                i, wset, ts, ts / (wset/8) / NACCESSES, ts / wset / NACCESSES
                (wset * NACCESSES)/(ts/clck_per_sec()));
    }
    
    

    return 0;
}