#include <stdio.h>

int main()
{
    int a = 10, b = 5, c;
    
    asm volatile ( 
        "movl %1, %%eax\n"
        "movl %2, %%ebx\n"
        "addl %%eax, %%ebx\n"
        "movl %%ebx, %0\n"
        : "=r" (c)
        : "r" (a), "r" (b)
    );
    printf("c = %d\n", c);
}