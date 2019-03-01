#ifndef MY_PPC_H
#define MY_PPC_H

#define SMPLOCK "lock; "

static inline int
CAS(volatile int32_t *addr, int32_t oldval, int32_t newval)
{
    unsigned char ret;
    __asm__ __volatile__ (
        SMPLOCK "cmpxchgl %3,%2   \n\t"
        "sete     %0      \n\t"
        : "=qm" (ret), "+a" (oldval), "+m" (*addr)
        : "q"(newval)
        : "memory", "cc");
    return (int)ret;
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

#endif