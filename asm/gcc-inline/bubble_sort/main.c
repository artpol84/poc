#include <stdio.h>

void sort(int size, int *array)
{
    asm volatile (
        "/* allocate space for the local variables:\n\t"
        "char *arr:     -8(%rbp)\n\t"
        "int arr_size:   -12(%rbp)\n\t"
        "int i:          -16(%rbp)\n\t"

        "subq $16, %rsp\t\n"

        "/* eax - array size\n\t"
        "ebx - outer loop index\n\t"
        "ecx - inner loop index\n\t"

        "xor %eax, %eax\n\t"
        "movl %edi, %eax\n\t"
        "movq %rax, %r8\n\t"
        "subq $1, %r8\n\t"
        "sort_loop1:\n\t"
        "mov $0, %rax\n\t"
        "cmpq %r8, %rax\n\t"
        "jnl sort_exit\n\t"

        "sort_loop2:\n\t"
        "cmpq %r8, %rax\n\t"
        "jnl sort_loop2_exit\n\t"

        "leaq (%rsi, %rax, 4), %r10\n\t"
        "movl (%r10), %ebx\n\t"
        "movl 4(%r10), %ecx\n\t"

        "cmpl %ecx, %ebx\n\t"
        "jng sort_loop2_next\n\t"
    
        "movl %ecx, (%r10)\n\t"
        "movl %ebx, 4(%r10)\n\t"

        "sort_loop2_next:\n\t"
    
        "addq $1, %rax\n\t"
        "jmp sort_loop2\n\t"

        "sort_loop2_exit:\n\t"
        "subq $1, %r8\n\t"
        "jmp sort_loop1\n\t"
    
        "sort_exit:\n\t"
        "addq $16, %rsp\n\t",
    

}



int main()
{
    int array[] = { 5, 1, 3, 8, 3, 2, 10 };
    int size = sizeof(array) / sizeof(array[0]);
    int i;

    sort(size, array);

    printf("Result: ");
    for(i = 0; i < size; i++){
        printf("%d ", array[i]);
    }
    printf("\n");
    return 0;
}
