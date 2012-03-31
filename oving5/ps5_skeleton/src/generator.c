#include <tree.h>
#include <generator.h>

typedef enum {
    NIL, CDQ, LEAVE, RET,                       // 0-operand
    LABEL, SYSLABEL,                            // Text placeholders
    PUSH, POP, MUL, DIV, DEC, NEG, CMPZERO,     // 1-operand arithmetic
    CALL, SYSCALL, JUMP, JUMPZERO, JUMPNONZ,    // 1-operand ctrlflow
    MOVE, ADD, SUB                              // 2-operand
} opcode_t;


/* Container for an opcode and up to two operands, and links to
 * previous/next operation
 */
typedef struct i {
	opcode_t op;
	char *operands[2];
	struct i *prev, *next;
} instruction_t;


/* Prototypes for functions to manipulate the instruction list */
static void instruction_init(instruction_t *instr, opcode_t op, ...);
static void instruction_append(instruction_t *next);
static void instruction_finalize(instruction_t *obsolete);
static void print_instructions(FILE *stream);

bool peephole = false;

/* Beginning and end of the instruction list */
static instruction_t *head = NULL, *tail = NULL;

/* Track the depth of how many scopes we're inside */
static int32_t depth = 1;
static int32_t labelMarker = 0;

static void instruction_init(instruction_t *instr, opcode_t op, ...) {
	va_list va;
	va_start(va, op);
	memset(instr, 0, sizeof(instruction_t));
	instr->op = op;
	switch (op) {
	case NIL:
	case CDQ:
	case LEAVE:
	case RET:
		break;
	case SYSLABEL:
	case SYSCALL:
	case LABEL:
	case CALL:
	case JUMP:
	case JUMPZERO:
	case JUMPNONZ:
	case PUSH:
	case POP:
	case MUL:
	case DIV:
	case DEC:
	case NEG:
	case CMPZERO: {
		char *ptr = (char *) va_arg(va, char *);
		instr->operands[0] = STRDUP(ptr);
	}
	break;
	case MOVE:
	case ADD:
	case SUB: {
		char *ptr_a = (char *) va_arg(va, char *);
		char *ptr_b = (char *) va_arg(va, char *);
		instr->operands[0] = STRDUP(ptr_a),
		                     instr->operands[1] = STRDUP(ptr_b);
	}
	break;
	}
	va_end(va);
}


static void instruction_append(instruction_t *instr) {
	instr->prev = tail, instr->next = NULL;
	tail->next = instr;
	tail = instr;
}


static void instruction_finalize(instruction_t *obsolete) {
	switch (obsolete->op) {
	case SYSLABEL:
	case SYSCALL:
	case LABEL:
	case CALL:
	case JUMP:
	case JUMPZERO:
	case JUMPNONZ:
	case PUSH:
	case POP:
	case MUL:
	case DIV:
	case DEC:
	case NEG:
	case CMPZERO:
		free(obsolete->operands[0]);
		break;
	case MOVE:
	case ADD:
	case SUB:
		free(obsolete->operands[0]), free(obsolete->operands[1]);
		break;
	}
	free(obsolete);
}


static void free_instructions(void) {
	instruction_t *i = head, *j;
	while (i != NULL) {
		j = i->next;
		instruction_finalize(i);
		i = j;
	}
}


/*
 * The following 3 definitions are an ugly hack for the sake of portability.
 * To emit correct assembly, we need a string which contains the name of the
 * file pointer of the standard output stream. The definiton in 'stdio.h',
 * however, is only required to be a macro which expands into the name which
 * this structure has in object code: since the assembler is unaware of this
 * text substitution, it must be done here, while we have the portable
 * macro-name available.
 * - The TXT macro turns its argument into a string.
 * - The EXPAND macro nests around TXT to force the preprocessor to expand two
 *   steps, instead of one.
 * - OUTFILE thus creates a string containing "__stdoutp", "stdout", or
 *   whatever the tool chain on your OS has called the file pointer internally.
 */
#define TXT(x) #x
#define EXPAND(x) TXT(x)
#define OUTFILE EXPAND(stdout)


/* Convenience macro for appending a new instruction (with less typing) */
#define INSTR(o,...) do { \
		instruction_t *i;  \
		instruction_init ( \
		                   i = (instruction_t *)malloc(sizeof(instruction_t)), o,##__VA_ARGS__ \
		                 );\
		instruction_append ( i ); \
	} while ( false )

/* Convenience macros for making strings referring to registers & adr modes.
 * This is not even remotely necessary, but notation is a matter of taste.
 */

/* Constants */
#define C(r) "$"#r
/* Registers, direct */
#define R(r) "%"#r
/* Registers, indirect */
#define RI(r) "(%" #r ")"
/* Registers indirect, with offset */
#define RO(o,r) #o "(%" #r ")"

/* Another convenience, just recur into child nodes */
#define RECUR() do {                                \
		for ( int32_t i=0; i<root->n_children; i++ )    \
			generate ( stream, root->children[i] );     \
	} while ( false )

void generate(FILE *stream, node_t *root) {
	if (root == NULL)
		return;

	switch (root->type.index) {
	case PROGRAM:
		/* Output the data segment, start the text segment */
		strings_output(stream);
		fprintf(stream, ".text\n");

		/* Create the root of the main program list */
		instruction_init(
		    tail = head = (instruction_t *)malloc(sizeof(instruction_t)),
		    NIL
		);

		/* Generate code for all children */
		RECUR();

		/* Read arguments from command line */
		INSTR(SYSLABEL, "main");
		INSTR(PUSH, R(ebp));
		INSTR(MOVE, R(esp), R(ebp));
		INSTR(MOVE, RO(8, esp), R(esi));
		INSTR(DEC, R(esi));
		INSTR(JUMPZERO, "noargs");
		INSTR(MOVE, RO(12, ebp), R(ebx));
		INSTR(SYSLABEL, "pusharg");
		INSTR(ADD, C(4), R(ebx));
		INSTR(PUSH, C(10));
		INSTR(PUSH, C(0));
		INSTR(PUSH, RI(ebx));
		INSTR(SYSCALL, "strtol");
		INSTR(ADD, C(12), R(esp));
		INSTR(PUSH, R(eax));
		INSTR(DEC, R(esi));
		INSTR(JUMPNONZ, "pusharg");
		INSTR(SYSLABEL, "noargs");

		/* Call 1st function in VSL program, and exit w. returned value */
		INSTR(CALL, root->children[0]->children[0]->children[0]->data);

		INSTR(LEAVE);
		INSTR(PUSH, R(eax));
		INSTR(SYSCALL, "exit");

		print_instructions(stream);
		free_instructions();
		break;
	case FUNCTION: {
		/* Functions need to have labels and a scope, simply build another stack-frame,
		 * and output the label for the function-start.
		 */
		INSTR(LABEL, root->children[0]->data);

		/* Basic-stack-frame setup */
		INSTR(PUSH, R(ebp));
		INSTR(MOVE, R(esp), R(ebp));

		depth++;
		/* We don't want to recurse all the children here, the BLOCK is always at children[2],
		 * and we need that, however, children[0] is the function-name, which we already used
		 * for label, and don't need on the stack, and children[1] is the arguments, which will
		 * be handled by the caller, thus we don't need to (nor should we) do anything about those
		 * here.
		 */
		generate(stream, root->children[2]);

		/* In case th block didn't include a RETURN, which would break with at few concepts in VSL
		 * we put in a leave for our scope here too, as well as a fallback-return, but a return-statement
		 * will clear this up too, making these dead-code, making this safe to keep in either way.
		 */
		INSTR(LEAVE);
		INSTR(RET);

		depth--;
	}
	break;
	case BLOCK: {
		/* Blocks simply define another scope, so increase depth while recursing inside, and
		 * build another stack-frame.
		 */
		INSTR(PUSH, R(ebp));
		INSTR(MOVE, R(esp), R(ebp));
		depth++;
		RECUR();
		/* The same applies for this LEAVE as for the FUNCTION-leave, a RETURN-statement will do the same thing.
		INSTR(LEAVE);
		depth--;
		break;
		case PRINT_STATEMENT: {
		/* Print-statements combine two things: Static strings, and numeric variables/constants
		 * we simply look through the arguments, and print them in the most fitting way
		 * using puts for strings to avoid un-percentifying things that could be mistaken
		 * for format-strings by printf, however, printf is very well-suited to perform the
		 * int-to-string-conversion we need, so we use that for numerics
		 */
		for (int i = 0; i < root->n_children; i++) {
			if (root->children[i]->type.index == TEXT) {
				char temp[32];
				sprintf(temp, "$.STRING%d", *(int *)root->children[i]->data);
				INSTR(PUSH, "stdout");
				INSTR(PUSH, temp);
				INSTR(SYSCALL, "fputs");
				INSTR(ADD, C(8), R(esp));
			} else {
				generate(stream, root->children[i]);
				INSTR(PUSH, "$.INTEGER");
				INSTR(SYSCALL, "printf");
				INSTR(ADD, C(8), R(esp));
			}
		}
		/* 10 is LF, most operating systems (although not some rather popular ones) use this alone as
		 * newline, which will do for now, so the following lines simply move the cursor down.
		 */
		INSTR(PUSH, C(10));
		INSTR(SYSCALL, "putchar");
		INSTR(ADD, C(4), R(esp));
	}
	break;
	case DECLARATION:
		if (root->children[0]->type.index == VARIABLE_LIST) {
			/* Simply make room on the stack */
			for (int i = 0; i < root->children[0]->n_children; i++) {
				INSTR(PUSH, C(0));
			}
		}
		break;
	case EXPRESSION:
		/* Expressions don't always recur, as functions have data we don't like to recur fully on */
		if (root->n_children == 1) {
			/* Unary minus, simply negate the sub-node, which already is on the stack after the recursion */
			RECUR();
			INSTR(NEG, RO(0, ESP));
			/* The following 4 are just a simple matter of popping in the right order, and remembering to push
			 * Forgetting to push can cause all sorts of headaches
			 */
		} else if (strcmp(root->data, "+") == 0) {
			RECUR();
			INSTR(POP, R(EAX));
			INSTR(POP, R(EBX));
			INSTR(ADD, R(EAX), R(EBX));
			INSTR(PUSH, R(EBX));
		} else if (strcmp(root->data, "-") == 0) {
			RECUR();
			INSTR(POP, R(EAX));
			INSTR(POP, R(EBX));
			INSTR(SUB, R(EAX), R(EBX));
			INSTR(PUSH, R(EBX));
		} else if (strcmp(root->data, "*") == 0) {
			RECUR();
			INSTR(POP, R(EAX));
			INSTR(POP, R(EBX));
			INSTR(MUL, R(EAX), R(EBX));
			INSTR(PUSH, R(EAX));
		} else if (strcmp(root->data, "/") == 0) {
			RECUR();
			INSTR(MOVE, RO(4, ESP), R(EAX));
			/* Sign-extend */
			INSTR(CDQ);
			INSTR(DIV, RO(0, ESP));
			INSTR(PUSH, R(EAX));
		} else if (strcmp(root->data, "^") == 0) {
			RECUR();
			/* Ok, POW, isn't the prettiest piece of code I ever wrote,
			 * but it does atleast work, I _think_ it might have issues
			 * with negative exponents, but the listed functions in the
			 * enum at the top of this file did not include anything regarding
			 * the sign-flag, so I didn't bother too much with that. It will
			 * check for zero-exponents and bases to try to give correct answers for these
			 * this was implemented with rather ugly labels, but then again, some
			 * naming scheme had to be chosen. And this one should atleast work
			 * for multiple pow's in the same .vsl-file.
			 */

			/* Create label-strings */
			labelMarker++;
			char tempLabel[32];
			char endTempLabel[32];
			char tempCheck[32];
			sprintf(tempLabel, "pow_label%d", labelMarker);
			sprintf(endTempLabel, "end_pow_label%d", labelMarker);
			sprintf(tempCheck, "check_pow_label%d", labelMarker);

			/* Get expression-input */
			INSTR(POP, R(ECX));
			INSTR(POP, R(EAX));
			/* Duplicate the base, as we will need that each iteration */
			INSTR(MOVE, R(EAX), R(EBX));
			/* If the base is zero, we need to push 0 and end the expression */
			INSTR(CMPZERO, R(EAX));
			/* Otherwise we can just go about our business */
			INSTR(JUMPNONZ, tempCheck);
			INSTR(MOVE, C(0), R(EAX));
			INSTR(JUMP, endTempLabel);
			INSTR(SYSLABEL, tempCheck);
			/* Check if all we are doing is a simple ^0*/
			INSTR(CMPZERO, R(ECX));
			INSTR(JUMPNONZ, tempLabel);
			INSTR(MOVE, C(1), R(EAX));
			INSTR(JUMP, endTempLabel);
			INSTR(SYSLABEL, tempLabel);
			INSTR(DEC, R(ECX));
			INSTR(JUMPZERO, endTempLabel);
			INSTR(MUL, R(EBX), R(EAX));
			INSTR(JUMPNONZ, tempLabel);
			/* Endpoint, of the above while-loop, either we looped up something, or had EAX set priorly */
			INSTR(SYSLABEL, endTempLabel);
			INSTR(PUSH, R(EAX));

		} else if (strcmp(root->data, "F") == 0) {
			// NB We don't visit half the nodes here, we don't want to push the function-name on stack
			// In the same way that we don't really bother with the argument-nodes defined in FUNCTION-nodes
			// As they will always be pushed by the calling-expression.
			/* If the function has arguments, traverse them */
			if (root->children[1]) {
				for (int i = 0; i < root->children[1]->n_children; i++) {
					generate(stream, root->children[1]->children[i]);
				}
			}
			/* Perform the call */
			INSTR(CALL, root->children[0]->data);
			/* Unwind the stack */
			if (root->children[1]) {
				char temp_int[8];
				int rollback = root->children[1]->n_children * 4;
				sprintf(temp_int, "$%d", rollback);
				INSTR(ADD, temp_int, R(esp));
			}
			/* Put the result on the stack */
			INSTR(PUSH, R(eax));
		}

		break;
	case VARIABLE: {
		/* VARIABLEs aren't as easy as they look at first glance,
		 * they can exist more or less anywhere on the stack, and
		 * we need to know WHICH one of them we are interested in
		 * thus we ask the node, and iterate upwards on the stack until
		 * we find the correct depth
		 */
		int scopediff = depth - root->entry->depth;
		INSTR(MOVE, R(ebp), R(ebx));
		if (scopediff > 0) {
			for (int i = 0; i < scopediff; i++) {
				INSTR(MOVE, RI(ebx), R(ebx));
			}
		}

		/* Not really necessary to have 32-bytes here, then again, the
		 * why not? the memory will be reclaimed when the stack from
		 * this recursion winds back up anyhow, and 32 is nice and round.
		 */
		char temp[32];
		/* Push the data of the variable on top of the stack, for use by the next expression */
		sprintf(temp, "%d(%%ebx)", root->entry->stack_offset);
		INSTR(PUSH, temp);
	}
	break;
	case INTEGER: {
		/* INTEGER's are easy, just push them on the stack, but
		 * don't forget the $, otherwise you'll lose hours debugging
		 * that could otherwise have been used writing readable code
		 *
		 * Oh, and I guess we don't have int-constants larger than 8-siphers,
		 * this constant could of course be adjusted to be larger, but
		 * well, this should cover 32-bit, which is our target.
		 */
		char temp_int[8];
		sprintf(temp_int, "$%d", *(int32_t *)root->data);
		INSTR(PUSH, temp_int);
	}
	break;
	case ASSIGNMENT_STATEMENT: {
		/* Assignments have two children, the left hand one we don't care to recurse for
		 * as pushing that on the stack would be rather silly, instead, we look that up
		 * ourselves, and explicitly only recurse down the right-hand side of the assignment
		 * to compute the actual assignment.
		 */
		generate(stream, root->children[1]);
		int32_t target_offset = root->children[0]->entry->stack_offset;
		int32_t target_depth = root->children[0]->entry->depth;
		int32_t scopediff = depth - target_depth;

		/* Basically the same approach as was done with VARIABLES to find the correct place
		 * on the stack, but this time we don't push.
		 * Instead, we simply write to that stack-element.
		 */
		INSTR(MOVE, R(ebp), R(ebx));
		if (scopediff > 0) {
			for (int i = 0; i < scopediff; i++) {
				INSTR(MOVE, RI(ebx), R(ebx));
			}
		}
		char temp[32];
		sprintf(temp, "%d(%%ebx)", target_offset);
		/* Fish out the top-element of the stack (the RHS of our assignment) */
		INSTR(MOVE, RI(esp), R(eax));
		/* Write it back to the position we found in the loop above */
		INSTR(MOVE, R(eax), temp);
	}
	break;
	case RETURN_STATEMENT: {
		RECUR();
		INSTR(POP, R(eax));
		/* We have to leave all the nested scopes, which is easy to hardcode, as
		 * all functions exist on the same depth, we'll simply put enough leaves,
		 * we avoid crashing by having the ret-instruction placed here as well, thus
		 * avoiding falling back on any leave-instruction from the FUNCTION-node
		 * This isn't pretty, or optimal, but it works.
		 */
		for (int i = 1; i < depth; i++) {
			INSTR(LEAVE);
		}
		/* Avoid running any more code, and thus any additional leaves */
		INSTR(RET);
		break;
	}
	default:
		RECUR();
		break;
	}
}

static void print_instructions(FILE *output) {
#define OUT(...) fprintf ( output,##__VA_ARGS__ )
	instruction_t *i = head;
	while (i != NULL) {
		switch (i->op) {
		case CDQ:
			OUT("\tcdq\n");
			break;
		case LEAVE:
			OUT("\tleave\n");
			break;
		case RET:
			OUT("\tret\n");
			break;
		case SYSLABEL:
			OUT("%s:\n", i->operands[0]);
			break;
		case SYSCALL:
			OUT("\tcall\t%s\n", i->operands[0]);
			break;
		case LABEL:
			OUT("_%s:\n", i->operands[0]);
			break;
		case CALL:
			OUT("\tcall\t_%s\n", i->operands[0]);
			break;
		case JUMP:
			OUT("\tjmp\t%s\n", i->operands[0]);
			break;
		case JUMPZERO:
			OUT("\tjz\t%s\n", i->operands[0]);
			break;
		case JUMPNONZ:
			OUT("\tjnz\t%s\n", i->operands[0]);
			break;
		case PUSH:
			OUT("\tpushl\t%s\n", i->operands[0]);
			break;
		case POP:
			OUT("\tpopl\t%s\n", i->operands[0]);
			break;
		case MUL:
			OUT("\timull\t%s\n", i->operands[0]);
			break;
		case DIV:
			OUT("\tidivl\t%s\n", i->operands[0]);
			break;
		case DEC:
			OUT("\tdecl\t%s\n", i->operands[0]);
			break;
		case NEG:
			OUT("\tnegl\t%s\n", i->operands[0]);
			break;
		case CMPZERO:
			OUT("\tcmpl\t$0,%s\n", i->operands[0]);
			break;
		case MOVE:
			OUT("\tmovl\t%s,%s\n", i->operands[0], i->operands[1]);
			break;
		case ADD:
			OUT("\taddl\t%s,%s\n", i->operands[0], i->operands[1]);
			break;
		case SUB:
			OUT("\tsubl\t%s,%s\n", i->operands[0], i->operands[1]);
			break;
		}
		i = i->next;
	}
#undef OUT
}
