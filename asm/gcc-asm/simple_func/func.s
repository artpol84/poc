
.data

/* define variables */
out_fmt:
    .string "Got %d!\n"

.text
	.globl	main
	.type	main, @function

print_my:
    /* epilog */
    pushq %rbp
    movq %rsp, %rbp
    
    movl %edi, %esi
    movl $out_fmt, %edi
    call printf

    popq %rbp
    ret

main:
    /* epilog */
    pushq %rbp
    movq %rsp, %rbp
    
    movl $10, %edi
    call print_my

    pop %rbp
    movl $0, %eax
    ret

    
    

