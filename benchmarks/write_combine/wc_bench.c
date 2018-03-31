#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib.h>

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

#define ARRAY_ITEMS ( 1 << 24 )
uint64_t narrays = 0, niters = 0;
char **data = NULL;
int nitems = ARRAY_ITEMS;
int mask = ARRAY_ITEMS - 1;

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

inline uint64_t
rdtsc()
{
	unsigned long a, d;
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	return a | ((uint64_t)d << 32);
}

/* A core loop that allows to try all kinds of optimizations */
void runloop(int afirst, int alast, int ifirst, int ilast, int iinc)
{
    int it, a;

    for(it = ifirst; it < ilast; it+=iinc){
        for(a = afirst; a < alast; a++) {
            int idx = it & mask;
            data[a][idx] = it;
//            printf("data[%d][%d]\n", a, idx);
        }
    }
}

void testloop1()
{
    int i;
    uint64_t start, end;

    start = rdtsc();
    for(i = 0; i < niters; i++) {
        /* Perform access multiple times */
        runloop(0, narrays, 0, nitems, 1);
    }
	end = rdtsc();
    printf("seq:\t%lu\n", end - start);
}

void testloop2()
{
    int i;
    uint64_t start, end;

    start = rdtsc();
    for(i = 0; i < niters; i++) {
        /* Perform access multiple times */
        runloop(0, narrays / 2, 0, nitems, 1);
        runloop(narrays / 2, narrays, 0, nitems, 1);
    }
    end = rdtsc();
    printf("split2:\t%lu\n", end - start);
}

void testloop3(int stride)
{
    int i, j;
    uint64_t start, end;

    start = rdtsc();
    for(i = 0; i < niters; i++) {
        /* First access first half of arrays */
        for(j=0; j < stride; j++){
            runloop(0, narrays / 2, j, nitems, stride);
        }
        /* Now second half of arrays */
        for(j=0; j < stride; j++){
            runloop(narrays / 2, narrays, j, nitems, stride);
        }
    }
    end = rdtsc();
    printf("split2:\tstride=%d %lu\n", stride, end - start);
}

int
main(int argc, char **argv)
{
	int i;
	int cline = cache_line_size();
	printf("cache line size: %d\n", cline);
	if( argc < 3 ) {
		printf("Usage: <prog> <narrays> <niters>");
		return 0;
	}

	narrays = atoi(argv[1]);
	niters = atoi(argv[2]);

	data = calloc(narrays, sizeof(*data));
	for(i = 0; i < narrays; i++ ){
        data[i] = calloc(nitems, sizeof(*data[0]));
	}

	testloop1();
	testloop2();
    for(i=1; i<=cline; i*=2) {
        testloop3(i);
    }

	return 0;
}
