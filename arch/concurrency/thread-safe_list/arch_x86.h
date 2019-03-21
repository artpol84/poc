#ifndef MY_PPC_H
#define MY_PPC_H

#include <stdint.h>

/* Derived from
 * https://github.com/open-mpi/ompi/blob/master/opal/include/opal/sys/x86_64/atomic.h
 */

#define MB()  __asm__ __volatile__ ("" : : : "memory")
#define RMB() __asm__ __volatile__ ("" : : : "memory")
#define WMB() __asm__ __volatile__ ("" : : : "memory")
#define ISYNC() __asm__ __volatile__ ("" : : : "memory")


#define SMPLOCK "lock; "

static inline int CAS(int64_t *addr, int64_t *oldval, int64_t newval)
{
   unsigned char ret;
   __asm__ __volatile__ (
                       SMPLOCK "cmpxchgq %3,%2   \n\t"
                               "sete     %0      \n\t"
                       : "=qm" (ret), "+a" (*oldval), "+m" (*addr)
                       : "q"(newval)
                       : "memory", "cc"
                       );

   return (int) ret;
}

static inline int atomic_inc(volatile int32_t* v, int i)
{
    int ret = i;
   __asm__ __volatile__(
        SMPLOCK "xaddl %1,%0"
        :"+m" (*v), "+r" (ret)
        :
        :"memory", "cc");
    return (ret+i);
}

static inline int64_t atomic_swap(int64_t *addr, int64_t newval)
{
    int64_t oldval;

    __asm__ __volatile__("xchgq %1, %0" :
             "=r" (oldval), "+m" (*addr) :
             "0" (newval) :
             "memory");
    return oldval;
}

#endif
