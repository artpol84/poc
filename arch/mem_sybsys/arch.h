#ifndef ARCH_H
#define ARCH_H

#ifdef __x86_64__ 
#include "arch_x86.h"
#endif

#ifdef __aarch64__
#include "arch_arm.h"
#endif

/**********************************************************************
 *
 * Memory Barriers
 *
 *********************************************************************/

static inline void atomic_mb (void)
{
    MB();
}

static inline void atomic_rmb (void)
{
    RMB();
}

static inline void atomic_wmb (void)
{
    WMB();
}

static inline void atomic_isync (void)
{
    ISYNC();
}

static void barrier_init(volatile int *ptr, int start) 
{
    *ptr = start;
    atomic_mb();
}

static void barrier_wait(volatile int *ptr, int expect)
{
    /* Indicate that we have approached the barrier */
    atomic_inc(ptr, 1);
    atomic_wmb();

    /* wait for all others to approach the barrier */
    while(*ptr < expect) {}
    atomic_mb();
}


#endif

/*
Copyright 2019 Artem Y. Polyakov <artpol84@gmail.com>
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
