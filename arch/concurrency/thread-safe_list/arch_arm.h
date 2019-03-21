#ifndef MY_PPC_H
#define MY_PPC_H

#include <stdint.h>

/* Derived from
 * https://github.com/open-mpi/ompi/blob/master/opal/include/opal/sys/arm64/atomic.h
 */

#define MB()  __asm__ __volatile__ ("dmb sy" : : : "memory")
#define RMB() __asm__ __volatile__ ("dmb ld" : : : "memory")
#define WMB() __asm__ __volatile__ ("dmb st" : : : "memory")
#define ISYNC() __asm__ __volatile__ ("isb" : : : "memory")

static inline int CAS(int64_t *addr, int64_t *oldval, int64_t newval)
{
    int64_t prev;
    int tmp;
    int ret;

    __asm__ __volatile__ ("1:  ldaxr    %0, [%2]       \n"
                          "    cmp     %0, %3          \n"
                          "    bne     2f              \n"
                          "    stxr    %w1, %4, [%2]   \n"
                          "    cbnz    %w1, 1b         \n"
                          "2:                          \n"
                          : "=&r" (prev), "=&r" (tmp)
                          : "r" (addr), "r" (*oldval), "r" (newval)
                          : "cc", "memory");

    ret = (prev == *oldval);
    *oldval = prev;
    return ret;
}

#define OPAL_ASM_MAKE_ATOMIC(type, bits, name, inst, reg)                   \
    static inline type atomic_fetch_ ## name ## _ ## bits (type *addr, type value) \
    {                                                                   \
        type newval, old;                                               \
        int32_t tmp;                                                    \
                                                                        \
        __asm__ __volatile__("1:  ldxr   %" reg "1, [%3]        \n"     \
                             "    " inst "   %" reg "0, %" reg "1, %" reg "4 \n" \
                             "    stxr   %w2, %" reg "0, [%3]   \n"     \
                             "    cbnz   %w2, 1b         \n"            \
                             : "=&r" (newval), "=&r" (old), "=&r" (tmp) \
                             : "r" (addr), "r" (value)                  \
                             : "cc", "memory");                         \
                                                                        \
        return old;                                                     \
    }

OPAL_ASM_MAKE_ATOMIC(int32_t, 32, add, "add", "w")

static inline int32_t atomic_inc(int32_t *addr, int32_t value)
{
    return atomic_fetch_add_32(addr, value) + value;
}

static inline int64_t atomic_swap(int64_t *addr, int64_t newval)
{
    int64_t ret;
    int tmp;

    __asm__ __volatile__ ("1:  ldaxr   %0, [%2]        \n"
                          "    stlxr   %w1, %3, [%2]   \n"
                          "    cbnz    %w1, 1b         \n"
                          : "=&r" (ret), "=&r" (tmp)
                          : "r" (addr), "r" (newval)
                          : "cc", "memory");

    return ret;
}

#endif
