#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
This program tests the performance characteristics of write combining 
On an intel CPU only 4 writes at a time will be combined so we should see 
a big performance improvement if we only write to distinct memory locations 
<=4 times per iteration of a loop

see:
http://mechanical-sympathy.blogspot.com/2011/07/write-combining.html

This file was taken from:
https://gist.github.com/billywhizz/1086581
*/

uint64_t narrays = 0, niters = 0;
char **data = NULL;
int nitems = 0;
int want_cache_flush = 0;
char *flush_array = NULL;
size_t flush_array_sz = 0;
size_t cache_sizes[32];
int cache_level_cnt = 0;
int iters_start = 1000;
int iters[32];
int want_sfence = 0;

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

    /* Add memory into hirarchy */
    cache_sizes[cache_level_cnt] = cache_sizes[cache_level_cnt - 1] * 4;
    iters[cache_level_cnt] = iters[cache_level_cnt - 1] / 5;
    cache_level_cnt++;

    return;
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


static inline uint64_t
rdtsc()
{
	unsigned long a, d;
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	return a | ((uint64_t)d << 32);
}

/* A core loop that allows to try all kinds of optimizations */
void runloop_regular(int afirst, int alast, int ifirst, int ilast, int iinc)
{
    int it, a;

    for(it = ifirst; it < ilast; it+=iinc){
        for(a = afirst; a < alast; a++) {
            data[a][it] = it;
        }
    }
}

/* A core loop that allows to try all kinds of optimizations */
void runloop_sfence(int afirst, int alast, int ifirst, int ilast, int iinc)
{
    int it, a;

    for(it = ifirst; it < ilast; it+=iinc){
        for(a = afirst; a < alast; a++) {
            data[a][it] = it;
        }
		asm volatile ("sfence" ::: "memory");
    }
}

void runloop(int afirst, int alast, int ifirst, int ilast, int iinc)
{
	if( want_sfence ){
		runloop_sfence(afirst, alast, ifirst, ilast, iinc);
	} else {
		runloop_regular(afirst, alast, ifirst, ilast, iinc);
	}
}


uint64_t testloop1()
{
    int i, warmup = niters;
    uint64_t start, end, result = 0;

    for(i = 0; i < niters + warmup; i++) {
        if(want_cache_flush) {
            flush_cache();
        }
        start = rdtsc();
        /* Perform access multiple times */
        runloop(0, narrays, 0, nitems, 1);
        end = rdtsc();
        if( i >= warmup ) {
            result += end - start;
        }
    }
    return result;
}

uint64_t testloop2()
{
    int i, warmup = niters;
    uint64_t start, end, result = 0;


    for(i = 0; i < niters + warmup; i++) {
        if(want_cache_flush) {
            flush_cache();
        }
        start = rdtsc();
        /* Perform access multiple times */
        runloop(0, narrays / 2, 0, nitems, 1);
        runloop(narrays / 2, narrays, 0, nitems, 1);
        end = rdtsc();
        if( i >= warmup ) {
            result += end - start;
        }
    }
    return result;
}

uint64_t testloop3(int stride)
{
    int i, j, warmup = niters;
    uint64_t start, end, result = 0;


    for(i = 0; i < niters + warmup; i++) {
        if(want_cache_flush) {
            flush_cache();
        }
        start = rdtsc();
        /* First access first half of arrays */
        for(j=0; j < stride; j++){
            runloop(0, narrays / 2, j, nitems, stride);
        }
        /* Now second half of arrays */
        for(j=0; j < stride; j++){
            runloop(narrays / 2, narrays, j, nitems, stride);
        }
        end = rdtsc();
        if( i >= warmup ) {
            result += end - start;
        }
    }
    return result;
}

int
main(int argc, char **argv)
{
	int i;
    int cache_line = cache_line_size();
    int level;
    discover_caches();
    printf("cache line size: %d\n", cache_line);
    if( argc < 2 ) {
        printf("Usage: <prog> <narrays> [sfence]");
		return 0;
	}

	if( argc > 2 && !strcmp(argv[2], "sfence") ){
		want_sfence = 1;
	}

	narrays = atoi(argv[1]);
    data = calloc(narrays, sizeof(*data));

    for(level = 0; level < cache_level_cnt; level++) {
        uint64_t result;
        niters = iters[level];

        printf("Fit data to the level %d of memory hirarchy (%zdB)\n",
               level + 1, cache_sizes[level]);

        nitems = cache_sizes[level] / narrays;
        for(i = 0; i < narrays; i++ ){
            data[i] = calloc(nitems + cache_sizes[level], sizeof(*data[0]));
        }
        flush_array_sz = cache_sizes[level] * 2;
        flush_array = calloc(flush_array_sz, sizeof(char));


//      printf("\t#1 WOUT cache flush:\n");
        want_cache_flush = 0;
        result = testloop1();
        printf("\tseq:\tstride=1\t%lu cycles/B\n", result / niters / nitems / narrays);
        result = testloop2();
        printf("\tsplit2:\tstride=1\t%lu cycles/B\n", result / niters / nitems / narrays);
        for(i=2; i<=cache_line; i*=2) {
            result = testloop3(i);
            printf("\tsplit2:\tstride=%d\t%lu cycles/B\n", i, result / niters / nitems / narrays);
        }

//        printf("\t#2 WITH cache flush:\n");
//        want_cache_flush = 1;
//        result = testloop1();
//        printf("\t\tseq:\tstride=1\t%lu cycles/B\n", result / niters / nitems / narrays);
//        result = testloop2();
//        printf("\t\tsplit2:\tstride=1\t%lu cycles/B\n", result / niters / nitems / narrays);
//        for(i=2; i<=cache_line; i*=2) {
//            result = testloop3(i);
//            printf("\t\tsplit2:\tstride=%d\t%lu cycles/B\n", i, result / niters / nitems / narrays);
//        }

        for(i = 0; i < narrays; i++ ){
            free(data[i]);
        }
        free(flush_array);
    }
	return 0;
}
