.data
.INTEGER: .string "%d"
.STRING0: .string "Sum is "
.STRING1: .string "You need at least one argument.\n"

.globl main

.text


foo:
    /* YOUR CODE HERE! */
    ret

main:
    /* Create a stack frame for main */
    pushl   %ebp
    movl    %esp,%ebp
    
    /* Check the number of arguments */
    movl    8(%ebp),%ebx
    cmp     $1,%ebx
    jg      args_ok
    
    /* Number of arguments was 0 (1 including program name, which is in argv[0]). Exit with error. */
    pushl   $.STRING1
    call printf
    addl    $4,%esp
    pushl   $1
    call exit

args_ok:

    /* Put the argv pointer into ebx */
    movl    12(%ebp),%ebx
    
    /* The second element of argv is our command line argument; increment ebx to point to that element */
    addl    $4,%ebx

    /* atoi(argv[1]) */
    pushl   (%ebx)
    call    atoi

    /* Rewind the stack to the state it was before pushing arguments onto it */
    addl    $4,%esp

    /* Push the return value from strtol onto the stack */
    pushl   %eax

    /* Call foo(), with one argument (top of stack) */
    call    foo

    /* Tear down the stack frame */
    leave

    /* exit(0) to indicate success */
    pushl   $0
    call    exit

