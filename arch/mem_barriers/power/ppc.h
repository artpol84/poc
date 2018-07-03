#ifndef MY_PPC_H
#define MY_PPC_H

static inline void hwsync()
{
    int ret;
    asm volatile ("hwsync;" : : :);
}

static inline void lwsync()
{
    int ret;
    asm volatile ("lwsync;" : : :);
}

static inline void isync()
{
    int ret;
    asm volatile ("isync;" : : :);
}

#endif