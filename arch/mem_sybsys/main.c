#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arch.h>
#include <mem_sys.h>

#define NACCESSES 1024

int main()
{
    int nlevels;
    size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX];
    char *buf = NULL;
    int i, j;

    discover_caches(&nlevels, cache_sizes);

    /* Allocate data buffer */
    buf = calloc(cache_sizes[nlevels - 1], 1);

    printf("Cache performance:\n");
    for (i = 0; i < nlevels; i++)
    {
        size_t wset = cache_sizes[i] - cache_sizes[i] / 1;
        uint64_t ts = rdtsc();
        for (j = 0; j < NACCESSES; j++)
        {
            /* get 90% of the cache size */
            memset(buf, j, wset);
        }
        ts = rdtsc() - ts;
        printf("[%d]:\t%llu cyc/byte", i, ts / wset / NACCESSES);
    }

    return 0;
}
