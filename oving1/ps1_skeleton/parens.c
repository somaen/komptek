#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#ifndef INIT_STACKSIZE
#define INIT_STACKSIZE 2
#endif


/*
 * Define a structure for tracking positions in text in
 * terms of line number and position on line
 */
typedef struct {
    int32_t line, count;
} position_t;

/* Pointer to the array we will use as a stack */
position_t *parens;

/* Controls the return value at program termination */
bool input_ok = true;

int32_t
    size = INIT_STACKSIZE,
    top = -1;


void
push ( position_t p )
{
    /* TODO:
     * Put 'p' at the top of the 'parens' stack.
     * Grow the size of the stack if it is too small.
     */
}


bool
pop ( position_t *p )
{
    /* TODO:
     * Take the top element off of the stack, and put it in the
     * location at 'p'. If the stack shrinks below zero, there's been
     * more ')'-s than '('-s, so return false to let caller know the stack
     * was empty.
     */
}


void
check ( void )
{
    /* TODO:
     * This is called after all input has been read.
     * If there are any elements left on stack, there have been more '('-s
     * than ')'-s. Emit suitable error message(s) on std. error, and cause
     * the program to return a failure code
     */
}


int
main ( int argc, char **argv )
{
    int32_t c = getchar();

    position_t
        now = { .line = 1, .count = 1 },   /* Track where we are in the input */
        balance;                           /* Space for the matching position
                                               when parentheses are closed */

    parens = (position_t *) malloc ( size * sizeof(position_t) );
    while ( ! feof(stdin) )
    {
        /* TODO:
         * - Manipulate the stack according to parentheses
         * - Update 'now' position according to read chars and line breaks
         */
        now.count += 1;
        c = getchar();
    }
    check();
    printf ( "Total of %d lines\n", now.line-1 );
    free ( parens );
    exit ( (input_ok) ? EXIT_SUCCESS : EXIT_FAILURE );
}
