#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

/*
 Based on https://github.com/artpol84/poc/blob/master/arch/write_combine/wc_bench.c
*/

size_t cache_sizes[32];
char *flush_bufs[32];
int cache_level_cnt = 0;
int iters_start = 1000;
int iters[32];


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

void discover_caches()
{
    FILE * p = 0;
    int idx = 0, level = -1;
    size_t size;

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
        if( (level-1) == 0 ){
            iters[level - 1] = iters_start;
        } else {
            iters[level - 1] = iters[level - 2] / 5;
        }
    }

    printf("Cache subsystem:\n");
    for(level=0; level < cache_level_cnt; level++) {
        printf("Level: %d\tSize: %zd\n", level + 1, cache_sizes[level]);
    }

    return;
}

void init_cache_flush()
{
    int i;
    for(i = 0; i < cache_level_cnt; i++){
        flush_bufs[i] = memalign(64, 2 * cache_sizes[i]);
    }
}

void flush_cache(int level)
{
    int i;
    for(i=0; i< 2 * cache_sizes[level]; i++){
        flush_bufs[level][i] = 0x0;
    }
}

#define ucs_max(a,b) ((a > b) ? a : b);

/* Extracted from the UCX project */
double ucs_x86_tsc_freq_from_cpu_model()
{
    char buf[256];
    char model[256];
    char *rate;
    char newline[2];
    double ghz, max_ghz;
    FILE* f;
    int rc;
    int warn;

    f = fopen("/proc/cpuinfo","r");
    if (!f) {
        return -1;
    }

    warn = 0;
    max_ghz = 0.0;
    while (fgets(buf, sizeof(buf), f)) {
        rc = sscanf(buf, "model name : %s", model);
        if (rc != 1) {
            continue;
        }

        rate = strrchr(buf, '@');
        if (rate == NULL) {
            continue;
        }

        rc = sscanf(rate, "@ %lfGHz%[\n]", &ghz, newline);
        if (rc != 2) {
            continue;
        }

        max_ghz = ucs_max(max_ghz, ghz);
        if (max_ghz != ghz) {
            abort();
            break;
        }
    }
    fclose(f);

    return max_ghz * 1e9;
}

/* Extracted from the UCX project */
double ucs_get_cpuinfo_clock_freq(const char *header, double scale)
{
    double value = 0.0;
    double m;
    int rc;
    FILE* f;
    char buf[256];
    char fmt[256];
    int warn;

    f = fopen("/proc/cpuinfo","r");
    if (!f) {
        return 0.0;
    }

    snprintf(fmt, sizeof(fmt), "%s : %%lf ", header);

    warn = 0;
    while (fgets(buf, sizeof(buf), f)) {

        rc = sscanf(buf, fmt, &m);
        if (rc != 1) {
            continue;
        }

        if (value == 0.0) {
            value = m;
            continue;
        }

        if (value != m) {
            value = ucs_max(value,m);
            abort();
        }
    }
    fclose(f);

    return value * scale;
}

/* Extracted from the UCX project */
static double ucs_x86_init_tsc_freq()
{
    double result;

    result = ucs_x86_tsc_freq_from_cpu_model();
    if (result <= 0.0) {
        result = ucs_get_cpuinfo_clock_freq("cpu MHz", 1e6);
    }

    if (result > 0.0) {
        return result;
    }

err_disable_rdtsc:
    return -1;
}

uint64_t rdtsc()
{
	unsigned long a, d;
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	return a | ((uint64_t)d << 32);
}

volatile int tmp;

uint64_t perform_iter(int level, char *buf)
{
    uint64_t start;
    int i;
    int cls = cache_line_size();
    int step = cls * 32;
    int nprobes = cache_sizes[level] / step;
    int indexes[nprobes];

    for(i = 0; i < nprobes; i++) {
        indexes[i] = i;
    }

    for(i = 0; i < 8 * nprobes; i++) {
        int idx1 = rand() % nprobes;
        int idx2 = rand() % nprobes;
        int tmp = indexes[idx1];
        indexes[idx1] = indexes[idx2];
        indexes[idx2] = tmp;
    }

    flush_cache(level);
    start = rdtsc();
    for(i = 0; i < nprobes; i++){
        tmp += buf[indexes[i] * step];
    }
    return (rdtsc() - start);
}


double measure_level_lat(int level)
{
    int i;
    uint64_t time = 0;
    int cls = cache_line_size();
    int step = cls * 32;
    int nprobes = cache_sizes[level] / step;
    char *buf = memalign(64, cache_sizes[level]);
    
    /* warmup */
    for(i=0; i<10; i++){
        perform_iter(level, buf);
    }

    /* Measure */
    for(i=0; i<1000; i++){
        time += perform_iter(level, buf);
    }
    
    return (double)time / 1000 / nprobes;
}

int main()
{
    int level;
    // Read cache hirarchy
    discover_caches();

    // allocate cache flush buffers
    init_cache_flush();

//    for(level = 0; level < cache_level_cnt; level++){
      level = 1;
    {
        double lat = measure_level_lat(level);
        printf("Cache Level %d: %lf cycles, %lf ns\n",
               level, lat, lat / ucs_x86_init_tsc_freq() * 1E9 );
    }
}
