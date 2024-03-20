#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <mem_sys.h>

static size_t flush_array_sz = 0;
static char *flush_array = NULL;

size_t 
cache_line_size()
{
	FILE * p = 0;
	p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
	unsigned int i = 0;
	if (p) {
		fscanf(p, "%d", &i);
		fclose(p);
	}
	return i;
}

int caches_discover(cache_struct_t *cache)
{
    FILE * p = 0;
    int idx = 0, level = -1;
    size_t size;

    cache->cl_size = cache_line_size();

    cache->nlevels = 0;

    for(idx = 0; ; idx++){
        char path[1024] = "", buf[256];
        int ret;
        char symb;
        sprintf(path, "/sys/devices/system/cpu/cpu0/cache/index%d/level", idx);

        if (idx >= MEMSUBS_CACHE_LEVEL_MAX) {
            printf("Too many cache levels, only support %d", MEMSUBS_CACHE_LEVEL_MAX);
            return -1;
        }

        p = fopen(path, "r");
        if( !p ) {
            break;
        }
        fscanf(p, "%d", &level);

        sprintf(path, "/sys/devices/system/cpu/cpu0/cache/index%d/type", idx);
        p = fopen(path, "r");
        if( !p ) {
            printf("ERROR: Unexpected sysfs format\n\n");
            exit(0);
        }
        fscanf(p, "%s", buf);
        if( !( !strcasecmp(buf,"Data") || !strcasecmp(buf,"Unified")) ) {
            /* This is not a cache we interested in */
            continue;
        }

        sprintf(path, "/sys/devices/system/cpu/cpu0/cache/index%d/size", idx);
        p = fopen(path, "r");
        if( !p ) {
            printf("ERROR: Unexpected sysfs format\n\n");
            exit(0);
        }
        ret = fscanf(p, "%zd%c", &size, &symb);
        if( ret > 1 ) {
            switch(symb) {
            case 'K':
                size *= 1024;
                break;
            case 'M':
                size *= 1024 * 1024;
                break;
            }
        }
        cache->cache_sizes[level-1] = size;
        cache->nlevels++;
    }

    printf("Cache subsystem (CL size = %zd):\n", cache_line_size());
    for(level=0; level < cache->nlevels; level++) {
        printf("Level: %d\tSize: %zd\n", level + 1, cache->cache_sizes[level]);
    }

    if (!cache->nlevels) {
         return 0;
    }

    /* Add memory into hirarchy */
    cache->cache_sizes[cache->nlevels] = cache->cache_sizes[cache->nlevels - 1] * 8;
    flush_array_sz = cache->cache_sizes[cache->nlevels];
    cache->nlevels++;

    flush_array = calloc(flush_array_sz, 1);

    return 0;
}

int caches_detect_level(cache_struct_t *cache, size_t buf_size)
{
    int i;
    for(i = 0; i < cache->nlevels; i++) {
        if( buf_size < cache->cache_sizes[i]) {
            return i;
        }
    }

    /* RAM */
    return cache->nlevels - 1;
}

void caches_set_default(cache_struct_t *cache)
{
    cache->cl_size = 64;
    cache->nlevels = 4;

    cache->cache_sizes[0] = 32*1024;
    cache->cache_sizes[1] = 1024*1024;
    cache->cache_sizes[2] = 32*1024*1024;
    cache->cache_sizes[3] = cache->cache_sizes[2] * 8;
}

void flush_cache()
{
    int i;
    static int count = 0;

    count++;
    for(i=0; i< flush_array_sz; i++){
        flush_array[i] = count;
    }
}
