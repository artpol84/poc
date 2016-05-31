
.text
	.globl	sort
	.type	sort, @function

sort:
    /* prolog */
    pushq %rbp
    movq %rsp, %rbp
    /* allocate space for the local variables:
        char *arr:      -8(%rbp)
        int arr_size:   -12(%rbp)
        int i:          -16(%rbp)
    */
    subq $16, %rsp

    /* eax - array size
       ebx - outer loop index
       ecx - inner loop index
    */
    xor %eax, %eax
    movl %edi, %eax
    movq %rax, %r8
    subq $1, %r8 
sort_loop1:
    mov $0, %rax
    cmpq %r8, %rax
    jnl sort_exit
    
sort_loop2:
    cmpq %r8, %rax
    jnl sort_loop2_exit

    leaq (%rsi, %rax, 4), %r10
    movl (%r10), %ebx
    movl 4(%r10), %ecx

    cmpl %ecx, %ebx
    jng sort_loop2_next
    
    movl %ecx, (%r10)
    movl %ebx, 4(%r10)

sort_loop2_next:
    
    addq $1, %rax
    jmp sort_loop2

sort_loop2_exit:
    subq $1, %r8
    jmp sort_loop1
    
sort_exit:
    /* epilog */
    addq $16, %rsp   
    popq %rbp
    ret
