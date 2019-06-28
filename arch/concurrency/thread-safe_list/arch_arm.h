#ifndef MY_ARM_H
#define MY_ARM_H

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
    static inline type _asm_atomic_fetch_ ## name ## _ ## bits (type *addr, type value) \
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
    return _asm_atomic_fetch_add_32(addr, value) + value;
}

static inline int64_t _asm_atomic_swap_ldst(int64_t *addr, int64_t newval)
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

static inline int64_t _asm_atomic_swap_lse(int64_t *addr, int64_t newval)
{
    int64_t ret;
    int tmp;

    __asm__ __volatile__ ("swpl %2, %0, [%1]\n"
                          : "=&r" (ret)
                          : "r" (addr), "r" (newval)
                          : "cc", "memory");
    return ret;
}


static inline int64_t atomic_swap(int64_t *addr, int64_t newval)
{
    return _asm_atomic_swap_lse(addr, newval);
}

#endif

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2010      IBM Corporation.  All rights reserved.
 * Copyright (c) 2010      ARM ltd.  All rights reserved.
 * Copyright (c) 2016-2018 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright     2019      Artem Y. Polyakov <artpol84@gmail.com>
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without specific
prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/
