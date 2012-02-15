#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef INIT_STACKSIZE
#define INIT_STACKSIZE 2
#endif

/*
 * Define a structure for tracking positions in text in
 * terms of line number and position on line
 */
typedef struct {
	int32_t line;
	int32_t count;
} position_t;

// Pointer to the array we will use as a stack
position_t *parens;

// Controls the return value at program termination
bool input_ok = true;

int32_t size = INIT_STACKSIZE;
int32_t top = -1;


void push(position_t p) {
	top++;
	// Reallocate if necessary
	if (top >= size) {
		// Grow by a factor of 2, similar to C++'s std::vector
		size *= 2;
		position_t *temp = (position_t *) realloc(parens, size * sizeof(position_t));
		if (temp != NULL) {
			parens = temp;
		} else {
			fprintf(stderr, "Error reallocating stack at line %d col: %d for new size %d",
			        p.line, p.count, size);
		}
	}
	// Copy p onto the stack
	memcpy(parens + top, &p, sizeof(position_t));
}


bool pop(position_t *p) {
	top--;
	if (top < -1) {
		top = -1;
		return false;
	}
	memcpy(p, parens + top + 1, sizeof(position_t));
	return true;
}


void check(void) {
	// Make sure we error for all unmatched elements on the stack
	while (top >= 0) {
		fprintf(stderr, "ERROR: Unmatched ( on %d %d\n", parens[top].line, parens[top].count);
		input_ok = false;
		top--;
	}
}


int main(int argc, char **argv) {
	int32_t c = getchar();

	position_t now = { .line = 1, .count = 1 };   /* Track where we are in the input */
	position_t balance;                           /* Space for the matching position
                                                     when parentheses are closed */
	position_t matching;
	parens = (position_t *) malloc(size * sizeof(position_t));
	while (! feof(stdin)) {
		now.count += 1;
		c = getchar();

		// Handle the three characters we care about:
		if (c == '(') {
			push(now);
		} else if (c == ')') {
			if (!pop(&matching)) {
				fprintf(stderr, "ERROR: ) at line: %d  col: %d matches no previous (\n", now.line, now.count);
				input_ok = false;
				/* We could go on, to detect any further errors
				 * But well, the task asked for verifying balance in some direction
				 * and we currently can't have any other case than imbalance with too many )'s
				 */
				exit(EXIT_FAILURE);
			} else {
				/*              printf(") at line: %d col: %d matches ( at line: %d col: %d\n",
				                           now.line, now.count, matching.line, matching.count);*/
			}
			// Reset now for next line.
		} else if (c == '\n') {
			now.line++;
			now.count = 1;
		}
	}
	check();
	printf("Total of %d lines\n", now.line - 1);
	free(parens);
	exit((input_ok) ? EXIT_SUCCESS : EXIT_FAILURE);
}
