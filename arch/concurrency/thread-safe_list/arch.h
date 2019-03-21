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



#endif