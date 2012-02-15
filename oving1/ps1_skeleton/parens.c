#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef INIT_STACKSIZE
#define INIT_STACKSIZE 2
#endif

void debugprint(const char *text, ...) {
	va_list debugargs;
#ifdef DEBUG_BUILD
	printf(text, args);
#endif
}

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

int32_t size = INIT_STACKSIZE;
int32_t top = -1;


void push(position_t p) {
	top++;
	if (top > size) {
		debugprint("Stack overflow at %d %d, reallocating", p.line, p.count);

		// Grow by a factor of 2, similar to C++'s std::vector
		size *= 2;
		parens = realloc(parens, size * sizeof(position_t));
	}
	// Copy p onto the stack
	memcpy(parens + top, &p, sizeof(position_t));
}


bool pop(position_t *p) {
	memcpy(p, parens + top, sizeof(position_t));
	top--;
	if (top < -1) {
		debugprint("Pop returning error for %d", top);
		top = -1;
		return false;
	}
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

		/* Handle the three characters we care about:
		 * The debugprints here are really unneccessary, but were
		 * helpfull for solving the task.
		 */
		if (c == '(') {
			push(now);
		} else if (c == ')') {
			debugprint("Stack height: %d\n", top);
			if (!pop(&matching)) {
				fprintf(stderr, "ERROR: ) at line: %d  col: %d matches no previous (\n", now.line, now.count);
				input_ok = false;
				/* We could go on, to detect any further errors
				 * But well, the task asked for verifying balance in some direction
				 * and we currently can't have any other case than imbalance with too many )'s
				 */
				exit(EXIT_FAILURE);
			} else {
				debugprint(") at line: %d col: %d matches ( at line: %d col: %d\n",
				           now.line, now.count, matching.line, matching.count);
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
	debugprint("Success: %d\n", input_ok);
	exit((input_ok) ? EXIT_SUCCESS : EXIT_FAILURE);
}
