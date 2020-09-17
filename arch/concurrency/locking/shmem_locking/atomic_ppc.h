#ifndef MY_PPC_H
#define MY_PPC_H

/* work-aroud for bug in PGI 16.5-16.7 */
/*
#if defined(__PGI)
#define OPAL_ASM_VALUE64(x) (void *)(intptr_t) (x)
#else
#define OPAL_ASM_VALUE64(x) x
#endif
*/

static inline int CAS(volatile int64_t *addr, int64_t oldval, int64_t newval)
{
    int ret;
    __asm__ __volatile__ (
        "1: ldarx   %0, 0, %2  \n\t"
        "   cmpd    0, %0, %3  \n\t"
        "   bne-    2f         \n\t"
        "   stdcx.  %4, 0, %2  \n\t"
        "   bne-    1b         \n\t"
        "2:"
        : "=&r" (ret), "=m" (*addr)
//        : "r" (addr), "r" (OPAL_ASM_VALUE64(oldval)), "r" (OPAL_ASM_VALUE64(newval)), "m" (*addr)
        : "r" (addr), "r" (oldval), "r" (newval), "m" (*addr)
        : "cc", "memory");
    return (ret == oldval);
}

static inline int64_t atomic_swap(int64_t *addr, int64_t newval)
{
    int64_t oldval;
    __asm__ __volatile__(
        "1:   ldarx   %0, 0, %2    \n\t"
        "     stdcx.  %3, 0, %2    \n\t"
        "     bne-    1b           \n\t"
        : "=&r" (oldval), "=m" (*addr)
//        : "r" (addr), "r" (OPAL_ASM_VALUE64(newval))
        : "r" (addr), "r" (newval)
        : "cc", "memory");
    return oldval;
}

#endif
