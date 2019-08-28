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

#define ucs_max(a,b) ((a > b) ? a : b);
#define ucs_min(a,b) ((a < b) ? a : b);

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


#define STRIDE 512

typedef struct buf_elem_s {
    struct buf_elem_s *next;
    char padding[STRIDE - sizeof(void*)];
} buf_elem_t;

volatile int tmp;
volatile void* result = 0;

//#define	ONE(b) { printf("b = %p\n", b); b = b->next; }
#define	ONE(b) { b = b->next; }
#define	FIVE(b)	ONE(b) ONE(b) ONE(b) ONE(b) ONE(b)
#define	TEN(b)	FIVE(b) FIVE(b)
#define	FIFTY(b)	TEN(b) TEN(b) TEN(b) TEN(b) TEN(b)
#define	HUNDRED(b)	FIFTY(b) FIFTY(b)


double perform_iter(buf_elem_t *buf, int size)
{
    uint64_t start;

    start = rdtsc();
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    HUNDRED(buf);
    result = buf;
    return (double)(rdtsc() - start) / 1000;
}


double measure_level_lat(int level)
{
    int i;
    uint64_t time = 0;
    int cls = cache_line_size();
    int buf_size = cache_sizes[level] / STRIDE;
    buf_elem_t *buf = memalign(64, cache_sizes[level]);
    double best;

    for(i = 1; i < buf_size; i++){
        buf[i].next = &buf[i - 1];
    }
    buf[0].next = &buf[buf_size - 1];

    /* Measure */
    best = perform_iter(buf, buf_size);
    for(i=0; i<1000; i++){
        time = perform_iter(buf, buf_size);
        best = ucs_min(best, time );
    }
    
    return best;
}

int main()
{
    int level;
    // Read cache hirarchy
    discover_caches();

    for(level = 0; level < cache_level_cnt; level++){
        double lat = measure_level_lat(level);
        printf("Cache Level %d: %lf cycles, %lf ns\n",
               level, lat, lat / ucs_x86_init_tsc_freq() * 1E9 );
    }
}
