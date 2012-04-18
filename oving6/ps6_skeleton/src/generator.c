#include <tree.h>
#include <generator.h>

typedef enum {
    NIL, CDQ, LEAVE, RET,                                // 0-operand
    LABEL, SYSLABEL,                                     // Text placeholders
    PUSH, POP, MUL, DIV, DEC, NEG, CMPZERO,              // 1-operand arithmetic
    CALL, SYSCALL, JUMP, JUMPLESS, JUMPZERO, JUMPNONZ,   // 1-operand ctrlflow
    MOVE, ADD, SUB, CMP, LSHIFT                          // 2-operand
} opcode_t;


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
static instruction_t *head = NULL, *tail = NULL;
static int32_t depth = 1;
static int32_t power_count = 0;
static int32_t if_count = 0;
static int32_t while_count = 0;
static int32_t while_depth = 0;

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
	case SUB:
	case CMP:
	case LSHIFT: {
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
	case CMP:
	case LSHIFT:
		free(obsolete->operands[0]), free(obsolete->operands[1]);
		break;
	}
	free(obsolete);
}


static void
free_instructions(void) {
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


#define INSTR(o,...) do { \
		instruction_t *i;  \
		instruction_init ( \
		                   i = (instruction_t *)malloc(sizeof(instruction_t)), o,##__VA_ARGS__ \
		                 );\
		instruction_append ( i ); \
	} while ( false )


#define C(r) "$"#r
#define R(r) "%"#r
#define RI(r) "(%" #r ")"
#define RO(o,r) #o "(%" #r ")"

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

		/* Parse arguments from command line */
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

	case FUNCTION:
		INSTR(LABEL, root->children[0]->data);
		INSTR(PUSH, R(ebp));
		INSTR(MOVE, R(esp), R(ebp));

		depth += 1;
		generate(stream, root->children[2]);
		INSTR(LEAVE);
		depth -= 1;

		INSTR(RET);
		break;

	case BLOCK:
		INSTR(PUSH, R(ebp));
		INSTR(PUSH, R(ebp));
		INSTR(MOVE, R(esp), R(ebp));

		depth += 1;
		RECUR();
		INSTR(LEAVE);
		depth -= 1;
		break;

	case PRINT_STATEMENT:
		for (int32_t i = 0; i < root->n_children; i++) {
			node_t *item = root->children[i];
			if (item->type.index == TEXT) {
				char constant[19]; // $.STRING%d, minus, 10 digits
				sprintf(constant, "$.STRING%d", *((int32_t *)item->data));
				INSTR(PUSH, OUTFILE);
				INSTR(PUSH, constant);
				INSTR(SYSCALL, "fputs");
				INSTR(PUSH, C(0x20));
				INSTR(SYSCALL, "putchar");
				INSTR(ADD, C(8), R(esp));
			} else {
				generate(stream, item);
				INSTR(PUSH, C(.INTEGER));
				INSTR(SYSCALL, "printf");
				INSTR(ADD, C(4), R(esp));
			}
		}
		INSTR(PUSH, C(0x0A));
		INSTR(SYSCALL, "putchar");
		INSTR(ADD, C(4), R(esp));
		break;

	case DECLARATION:
		for (int32_t i = 0; i < root->children[0]->n_children; i++) {
			if (root->children[0]->children[i]->n_children == 0) {
				INSTR(PUSH, C(0));
			} else { // We have an array, thus we need to:
				// Get the length of it
				int arraySize = (*(int *)root->children[0]->children[i]->children[0]->data);
				// Create a pointer to the first element
				INSTR(MOVE, R(esp), R(ecx));
				// ESP is where the pointer will be stored, increment it.
				INSTR(ADD, C(4), R(ecx));
				// Push that to the stack
				INSTR(PUSH, R(ecx));
				// Move the stack-pointer, this was mostly done this way out of convenience (counting visually in the dark hours,
				// instead of relying on my math to work fine will fighting jetlag)
				for (int i = 0; i < arraySize; i++) {
					INSTR(PUSH, C(0));  // Could have just moved ESP too, but this way we ensure 0's in all elements.
				}
			}
		}
		break;

	case EXPRESSION:
		if (root->n_children == 1 && root->data != NULL) {
			RECUR();
			INSTR(POP, R(eax));
			INSTR(NEG, R(eax));
			INSTR(PUSH, R(eax));
		} else if (root->n_children == 2) {
			if (*((char *)(root->data)) == 'F') {
				RECUR();
				int32_t
				expected_args = root->children[0]->entry->n_args,
				actual_args = (root->children[1] == NULL) ?
				              0 : root->children[1]->n_children;
				if (expected_args != actual_args) {
					fprintf(stderr,
					        "Error: function '%s' expects %d arguments, "
					        "but is called with %d.\n",
					        (char *)root->children[0]->data,
					        expected_args, actual_args
					       );
					exit(EXIT_FAILURE);
				}
				/* Call function */
				INSTR(CALL, root->children[0]->data);

				/* Remove parameters, if they exist */
				if (root->children[1] != NULL) {
					char constant[12];
					sprintf(constant,
					        "$%d", 4 * root->children[1]->n_children
					       );
					INSTR(ADD, constant, R(esp));
				}
				/* Push returned value */
				INSTR(PUSH, R(eax));
			}
			//Array lookup
			else if (*((char *)(root->data)) == 'A') {
				// Put the details on the stack, in order: Pointer, Index
				RECUR();
				// Fetch index
				INSTR(POP, R(edx));
				// Fetch pointer
				INSTR(POP, R(ecx));
				// Multiply by 4
				INSTR(LSHIFT, C(2), R(edx));
				// Combine index and pointer
				INSTR(ADD, R(edx), R(ecx));
				// Deref and push
				INSTR(PUSH, RI(ecx));

			} else {
				RECUR();
				INSTR(POP, R(ebx));
				INSTR(POP, R(eax));
				switch (*((char *)root->data)) {
				case '+':
					INSTR(ADD, R(ebx), R(eax));
					break;
				case '-':
					INSTR(SUB, R(ebx), R(eax));
					break;
				case '*':
					INSTR(CDQ);
					INSTR(MUL, R(ebx));
					break;
				case '/':
					INSTR(CDQ);
					INSTR(DIV, R(ebx));
					break;
				case '^': {
					/* Power */
					char startlabel[15], endlabel[15];
					sprintf(startlabel, "_power%d", ++power_count);
					sprintf(endlabel, "_endpower%d", power_count);

					/* Check for base == 1 */
					INSTR(CMP, C(1), R(eax));
					INSTR(JUMPZERO, endlabel);

					/* Check for exponent < 0 */
					INSTR(MOVE, R(eax), R(ecx));
					INSTR(MOVE, C(0), R(eax));
					INSTR(CMPZERO, R(ebx));
					INSTR(JUMPLESS, endlabel);

					/* Normal case */
					INSTR(MOVE, C(1), R(eax));
					INSTR(CDQ);
					INSTR(LABEL, startlabel + 1);
					INSTR(CMPZERO, R(ebx));
					INSTR(JUMPZERO, endlabel);
					INSTR(MUL, R(ecx));
					INSTR(SUB, C(1), R(ebx));
					INSTR(JUMP, startlabel);
					INSTR(LABEL, endlabel + 1);
					break;
				}
				}
				INSTR(PUSH, R(eax));
			}
		}
		break;

	case VARIABLE:
		if (root->entry->label == NULL) {
			/* Find the variable on stack */
			char var_offset[19];
			sprintf(var_offset, "%d(%%ecx)", root->entry->stack_offset);

			/* Start from record's ebp */
			INSTR(MOVE, R(ebp), R(ecx));

			/* If var. was defined at other nesting level, unwind
			 * the records (here, using ecx for temps)
			 */
			for (int u = 0; u < (depth - (root->entry->depth)); u++)
				INSTR(MOVE, RO(4, ecx), R(ecx));

			/* Once we have the right record, look up the variable */
			INSTR(PUSH, var_offset);
		}
		break;

	case INTEGER: {
		char constant[13]; /* At most $, minus sign and 10 digits */
		sprintf(constant, "$%d\0", *((int32_t *)root->data));
		INSTR(PUSH, constant);
	}
	break;

	case ASSIGNMENT_STATEMENT:
		generate(stream, root->children[1]);
		INSTR(POP, R(eax));

		if (root->n_children == 3) { // Only case with 3 children is the one with indexed
			// First, get our data, in order: Pointer, Index, Assignment-value
			RECUR();
			// Fetch Assignment-value
			INSTR(POP, R(ebx));
			// Fetch index
			INSTR(POP, R(edx));
			// Multiply index by 4
			INSTR(LSHIFT, C(2), R(edx));
			// Fetch pointer
			INSTR(POP, R(ecx));
			// Combine pointer and index
			INSTR(ADD, R(edx), R(ecx));
			// Store assignment value at the location pointed at.
			INSTR(MOVE, R(ebx), RI(ecx));
			// We are done
			break;
		} else {
			INSTR(MOVE, C(0), R(edx));
		}

		/* Unwind stack if appropriate */
		node_t *target = root->children[0];
		INSTR(MOVE, R(ebp), R(ecx));
		for (int u = 0; u < (depth - (target->entry->depth)); u++) {
			INSTR(MOVE, RO(4, ecx), R(ecx));
		}
		/* Offset the base-pointer for the correct stack-frame down by the index-amount
		 * thus a[3] becomes a[0], and we don't need to do anything about the stack-offset
		 * (well, we already did anyway, by skewing the stack-top in ecx).
		 */
		if (root->n_children == 3) {
			INSTR(ADD, R(edx), R(ecx));
		}
		{
			char offsz[19];
			sprintf(offsz, "%d(%%ecx)", target->entry->stack_offset);
			INSTR(MOVE, R(eax), offsz);
		}

		break;

	case RETURN_STATEMENT:
		RECUR();
		INSTR(POP, R(eax));
		for (int32_t u = 0; u < depth - 1; u++)
			INSTR(LEAVE);
		INSTR(RET);
		break;

		/* TODO: implement conditionals, loops and continues */
	case IF_STATEMENT: {
		char elseLabel[16];
		char endifLabel[16];
		// Generate labels
		sprintf(elseLabel, "_elseLabel%d", if_count);
		sprintf(endifLabel, "_endifLabel%d", if_count++);
		// Generate the expression, putting the result on stack
		generate(stream, root->children[0]);
		// Compare the result to 0
		INSTR(MOVE, RI(esp), R(eax));
		INSTR(MOVE, C(0), R(ebx));
		INSTR(CMP, R(eax), R(ebx));
		// If (0) goto elseLabel
		INSTR(JUMPZERO, elseLabel);
		// Generate the if-block (falling through from the above)
		generate(stream, root->children[1]);
		// Two cases, either we have an else, or we don't
		if (root->n_children == 3) {
			// Skip over the else-block if we did the if-part
			INSTR(JUMP, endifLabel);
			INSTR(LABEL, elseLabel + 1);
			generate(stream, root->children[2]);
			INSTR(LABEL, endifLabel + 1);
		} else {
			// Yes, we do use the elseLabel for if's without
			// else's, mainly since it doesn't harm, and makes
			// the code a bit shorter.
			INSTR(LABEL, elseLabel + 1);
		}
	}
	break;

	case WHILE_STATEMENT: {
		char endLabel[16];
		char expLabel[16];
		// While-depth is necessary to know how many scopes to unroll when Continuing
		while_depth = depth;
		// Generate labels
		sprintf(endLabel, "_endWhile%d", while_count);
		sprintf(expLabel, "_startWhile%d", while_count);
		INSTR(LABEL, expLabel + 1);
		// Generate expression (AFTER label, since it needs to be done every iteration)
		generate(stream, root->children[0]);
		INSTR(MOVE, RI(esp), R(eax));
		INSTR(MOVE, C(0), R(ebx));
		INSTR(CMP, R(eax), R(ebx));
		// end the while if it fails.
		INSTR(JUMPZERO, endLabel);
		generate(stream, root->children[1]);
		// Hard-jump to top, to verify conditional, if it fails, we'll jump to the endLabel anyhow.
		INSTR(JUMP, expLabel);
		INSTR(LABEL, endLabel + 1);
		while_count++; // Necessary to avoid duplicate labels (and for continue)
	}
	break;

	case NULL_STATEMENT: {
		// Solved by simply knowing that any Continue will be inside a WHILE
		// Thus the last set while_count will be the label to jump to.
		char whileLabel[16];
		sprintf(whileLabel, "_startWhile%d", while_count);
		// Unroll to the last set while_depth (or to be specific, the diff from current-depth)
		for (int i = 0; i < (depth - while_depth); i++) {
			INSTR(LEAVE);
		}
		// Then do as Van Halen told you.
		INSTR(JUMP, whileLabel);
	}
	break;


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
		case CMP:
			OUT("\tcmpl\t%s,%s\n", i->operands[0], i->operands[1]);
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
		case LSHIFT:
			OUT("\tshl\t%s,%s\n", i->operands[0], i->operands[1]);
			break;
		}
		i = i->next;
	}
#undef OUT
}
