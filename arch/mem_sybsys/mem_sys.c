#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mem_sys.h>

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

void discover_caches(int *nlevels, size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX])
{
    FILE * p = 0;
    int idx = 0, level = -1;
    size_t size;
    int cache_level_cnt = 0;

    for(idx = 0; ; idx++){
        char path[1024] = "", buf[256];
        int ret;
        char symb;
        sprintf(path, "/sys/devices/system/cpu/cpu0/cache/index%d/level", idx);
        p = fopen(path, "r");
        if( !p ) {
            break;
//            if( level < 0 ) {
//                printf("WARNING: Cannot identify cache size, use 10M by default\n");
//                return (10 * 1024 * 1024);
//            } else {
//                printf("Last level cache: level=%d, size=%zd\n", level, size);
//                return size;
//            }
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
        cache_sizes[level-1] = size;
        cache_level_cnt++;
    }

    printf("Cache subsystem (CL size = %zd):\n", cache_line_size());
    for(level=0; level < cache_level_cnt; level++) {
        printf("Level: %d\tSize: %zd\n", level + 1, cache_sizes[level]);
    }

    /* Add memory into hirarchy */
    cache_sizes[cache_level_cnt] = cache_sizes[cache_level_cnt - 1] * 4;
    cache_level_cnt++;

    *nlevels = cache_level_cnt;
    return;
}
