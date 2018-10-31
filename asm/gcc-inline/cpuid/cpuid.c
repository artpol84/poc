#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define NUM_REGS 4

uint64_t call_cpuid(uint64_t func, uint64_t ret_regs[NUM_REGS])
{
    uint64_t tmp = 0;

    asm volatile (
        "movq %[func], %%rax\n"
        "cpuid \n"
        "movq %[out], %%r10 \n"
        "movq %%rax, (%%r10) \n"
        "addq $8, %%r10 \n"
        "movq %%rbx, (%%r10) \n"
        "addq $8, %%r10 \n"
        "movq %%rcx, (%%r10) \n"
        "addq $8, %%r10 \n"
        "movq %%rdx, (%%r10) \n"
        : 
        : [func] "r" (func), [out] "r" (ret_regs)
        : "rax", "rbx", "rcx", "rdx", "r10");
/*
    asm volatile (
        "movq %[out], %%r10 \n"
        "movq %%r10, %[tmp]\n"
        : [tmp] "=r" (tmp)
        : [out] "r" (ret_regs)
        : "rax", "rbx", "rcx", "rdx", "r10");
    printf("out = %lx, tmp = %lx\n", (uint64_t)ret_regs, tmp);
*/
}


int main(int argc, char **argv)
{
    uint64_t regs[NUM_REGS];

    /* Get CPU capabilities */
    uint64_t ret = call_cpuid(0, regs);
    char str[13] = { 0 };
    memcpy(str, (char*)(regs + 1), 4);
    memcpy(str + 4, (char*)(regs + 3), 4);
    memcpy(str + 8, (char*)(regs + 2), 4);
    printf("Vendor ID: %s\n", str);
    printf("max leaf = %ld\n", regs[0]);

    /* Get PMU */
    ret = call_cpuid(0xa, regs);
    printf("RAX = %lx\n", regs[0]);
    printf("RBX = %lx\n", regs[1]);
    printf("RCX = %lx\n", regs[2]);
    printf("RDX = %lx\n", regs[3]);

}