#ifndef MEM_SYS_H
#define MEM_SYS_H

#include <sys/types.h>

#define MEMSUBS_CACHE_LEVEL_MAX 32

typedef struct {
    int nlevels;
    size_t cl_size;
    size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX];
} cache_struct_t;


int caches_discover(cache_struct_t *cache);
void caches_set_default(cache_struct_t *cache);
void flush_cache();

#endif

