#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

uint64_t rdtsc()
{
	unsigned long a, d;
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	return a | ((uint64_t)d << 32);
}


#define	ONE(exp) { exp; }
#define	FIVE(exp)	ONE(exp) ONE(exp) ONE(exp) ONE(exp) ONE(exp)
#define	TEN(exp) FIVE(exp) FIVE(exp)
#define	FIFTY(exp)	TEN(exp) TEN(exp) TEN(exp) TEN(exp) TEN(exp)
#define	HUNDRED(exp) FIFTY(exp) FIFTY(exp)
#define	FIVEHUNDRED(exp)    HUNDRED(exp) HUNDRED(exp) HUNDRED(exp) HUNDRED(exp) HUNDRED(exp)
#define	THOUSAND(exp)	FIVEHUNDRED(exp) FIVEHUNDRED(exp) 


static inline void delay(int niter)
{
    int i;
/*
    for( i = 0; i < niter; i++) {
        ONE
    }
*/
    ONE( asm volatile ( "nop\n" : : : "memory") );
}

#define delay_1 ONE( asm volatile ( "nop\n" : : : "memory") )

int main()
{
    int i;
    uint64_t start;


    start = rdtsc();
    THOUSAND( delay_1);
    printf("iter=1, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( ONE(delay_1));
    printf("iter=1, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( FIVE(delay_1));
    printf("iter=5, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( FIVE(delay_1) delay_1);
    printf("iter=6, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( delay_1 delay_1 delay_1 delay_1 delay_1 delay_1);
    printf("iter=6, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( FIVE(delay_1) delay_1 delay_1);
    printf("iter=7, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( FIVE(delay_1) delay_1 delay_1 delay_1);
    printf("iter=8, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( TEN(delay_1));
    printf("iter=10, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( FIFTY(delay_1));
    printf("iter=50, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    start = rdtsc();
    THOUSAND( HUNDRED(delay_1));
    printf("iter=100, cycles=%lf\n",  (rdtsc() - start) / 1000.0);

    
}
