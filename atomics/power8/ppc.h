#ifndef MY_PPC_H
#define MY_PPC_H

static inline int
CAS(volatile int *addr, int oldval, int newval)
{
    int ret;
    __asm__ __volatile__ (
        "1: lwarx   %0, 0, %2  \n\t"
        "   cmpw    0, %0, %3  \n\t"
        "   bne-    2f         \n\t"
        "   stwcx.  %4, 0, %2  \n\t"
        "   bne-    1b         \n\t"
        "2:"
        : "=&r" (ret), "=m" (*addr)
        : "r" (addr), "r" (oldval), "r" (newval), "m" (*addr)
        : "cc", "memory");
        return (ret == oldval);
}

static inline int atomic_inc(volatile int *v, int inc)
{
   int t;
   __asm__ __volatile__(
       "1:   lwarx   %0, 0, %3    \n\t"
       "     add     %0, %2, %0   \n\t"
       "     stwcx.  %0, 0, %3    \n\t"
       "     bne-    1b           \n\t"
       : "=&r" (t), "=m" (*v)
       : "r" (inc), "r" (v), "m" (*v)
       : "cc");
   return t;
}

static inline int atomic_dec(volatile int *v, int dec)
{
   int t;

   __asm__ __volatile__(
      "1:   lwarx   %0,0,%3      \n\t"
      "     subf    %0,%2,%0     \n\t"
      "     stwcx.  %0,0,%3      \n\t"
      "     bne-    1b           \n\t"
      : "=&r" (t), "=m" (*v)
      : "r" (dec), "r" (v), "m" (*v)
      : "cc");
      return t;
}

#endif