
.data

/* define variables */
hello_str:
    .string "Hello world!\n"

.text
	.globl	main
	.type	main, @function

main:
    /* epilog */
    pushq %rbp
    movq %rsp, %rbp
    
    movq $hello_str, %rdi 
    call puts

    pop %rbp
    movl $0, %eax
    ret

    
    

