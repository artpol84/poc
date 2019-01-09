#ifndef X86_64_H
#define X86_64_H

// NOTE: x86_64 only for now
#define MB() __asm__ __volatile__("": : :"memory")
static inline void mb(void)
{
    MB();
}

static inline void rmb(void)
{
    MB();
}

static inline void wmb(void)
{
    MB();
}

#endif // X86_64_H
