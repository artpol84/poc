#include <stdio.h>

int main()
{
    int a = 10, b = 5, c;
    
    asm volatile ( 
        "addl %[var2], %[var1]\n"
        : [var1] "=r" (c)
        : [var2] "r" (a), "[var1]" (b)
    );
    printf("c = %d\n", c);
}