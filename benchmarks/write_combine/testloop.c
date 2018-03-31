#include <stdio.h>
#include <stdint.h>
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
#define ITEMS 1<<24

static char A1[ITEMS];
static char A2[ITEMS];
static char A3[ITEMS];
static char A4[ITEMS];
static char A5[ITEMS];
static char A6[ITEMS];
static int items = 1<<24;
static int mask = 0;
static uint64_t iter = 10 * 1024 * 1024;

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

inline void
testloop1()
{
	uint64_t start, end;
	uint64_t i = iter;
	uint64_t iiter = 0;
	start = rdtsc();
	while(--i) {
		int slot = i & mask;
		char val = i & 0xff;
		A1[slot] = val;
		A2[slot] = val;
		A3[slot] = val;
		A4[slot] = val;
		A5[slot] = val;
		A6[slot] = val;
		iiter++;
	}
	end = rdtsc();
	printf("%lu\t%lu\n", iiter, end - start);
}

inline void
testloop2()
{
	uint64_t start, end;
	uint64_t i = iter;
	uint64_t iiter = 0;
	start = rdtsc();
	while(--i) {
		int slot = i & mask;
		char val = i & 0xff;
		A1[slot] = val;
		A2[slot] = val;
		A3[slot] = val;
		iiter++;
	}
	i = iter;
	while(--i) {
		int slot = i & mask;
		char val = i & 0xff;
		A4[slot] = val;
		A5[slot] = val;
		A6[slot] = val;
		iiter++;
	}
	end = rdtsc();
	printf("%lu\t%lu\n", iiter, end - start);
}

int
main(int ac, char **av)
{
	printf("cache line size: %li\n", cache_line_size());
	mask = items - 1;
	if(ac > 1) {
		iter = atoi(av[1]) * 1024 * 1024;
	}

	testloop1();
	testloop2();
		
	return 0;
}