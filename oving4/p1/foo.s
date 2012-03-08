.data
.INTEGER: .string "%d"
.STRING0: .string "Sum is "
.STRING1: .string "You need at least one argument.\n"
.PRINTSTRING: .string "Sum is %d.\n"

.globl main

.text

/* Could just as easily have used EBX for temporaries, and ECX for counting instead of actually
   using the local i-variable (which would follow x86-convention), but I decided against it, as
   this Problem Set aimed at understanding stackframes. This solution is thus not the fastest,
   as it will require memory-access for each access to i (or well, atleast cache-hit).  */
foo:
	pushl	%ebp
	mov 	%esp, %ebp		/* Move stack top to function frame base */
	pushl	$0				/* int sum = 0; */
	pushl	$0				/* int i = 1; (decremented by one to simplify loop jumps) */
foo_for:
	addl 	$1, -8(%ebp)	/* i++; (Cheating a bit by doing this first, and preinitializing it to one less) */
	movl	8(%ebp), %edx	/* Argument int N */
	cmp		%edx, -8(%ebp)	/* i < N */
	jge		foo_end			/* Inverted conditionals are quite common in assembly, and they make sense */
	movl	-8(%ebp), %eax	/* Put i in EAX */
	cdq						/* Sign extend */
	movl	$3, %ecx		/* Load 3 to ECX */
	divl	%ecx			/* i % 3 */
	movl	$0, %ecx		/* Load 0 to ECX */
	cmp		%edx, %ecx		/* i % 3 == 0 */
	je		foo_for_increment	/* C has short-circuited ||, so skip ahead to the inner-if */	
	movl	-8(%ebp), %eax		/* Put i in EAX */
	cdq							/* Sign extend */
	movl	$5, %ecx			/* Load 5 to ECX */
	divl	%ecx				/* i % 5 */
	movl	$0, %ecx			/* ECX = 0 */
	cmp		%edx, %ecx			/* i % 5 == 0 */
	je		foo_for_increment
	jmp foo_for
foo_for_increment:
	addl	$1, -4(%ebp)		/* sum += 1 */
	jmp foo_for

foo_end:
	movl	-4(%ebp), %eax		/* Return value (i) -> EAX */
	addl	$8, %esp			/* Wind back stack */
	leave
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

	pushl %eax				/* Returned from foo */
	pushl $.PRINTSTRING
	call printf
	addl $8, %esp	/* Wind back the stack, removing the arguments */

    /* Tear down the stack frame */
    leave

    /* exit(0) to indicate success */
    pushl   $0
    call    exit

