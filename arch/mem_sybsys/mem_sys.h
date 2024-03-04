#ifndef MEM_SYS_H
#define MEM_SYS_H

#include <sys/types.h>

#define MEMSUBS_CACHE_LEVEL_MAX 32

void discover_caches(int *nlevels, size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX]);



#endif

