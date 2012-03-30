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
static void instruction_init ( instruction_t *instr, opcode_t op, ... );
static void instruction_append ( instruction_t *next );
static void instruction_finalize ( instruction_t *obsolete );
static void print_instructions ( FILE *stream );

bool peephole = false;

/* Beginning and end of the instruction list */
static instruction_t *head = NULL, *tail = NULL;

/* Track the depth of how many scopes we're inside */
static int32_t depth = 1;

static void instruction_init ( instruction_t *instr, opcode_t op, ... ) {
    va_list va;
    va_start ( va, op );
    memset ( instr, 0, sizeof(instruction_t) );
    instr->op = op;
    switch ( op )
    {
        case NIL: case CDQ: case LEAVE: case RET:
            break;
        case SYSLABEL: case SYSCALL: case LABEL: case CALL: case JUMP:
        case JUMPZERO: case JUMPNONZ: case PUSH: case POP: case MUL:
        case DIV: case DEC: case NEG: case CMPZERO:
            {
                char *ptr = (char *) va_arg ( va, char * );
                instr->operands[0] = STRDUP(ptr);
            }
            break;
        case MOVE: case ADD: case SUB:
            {
                char *ptr_a = (char *) va_arg ( va, char * );
                char *ptr_b = (char *) va_arg ( va, char * );
                instr->operands[0] = STRDUP(ptr_a),
                instr->operands[1] = STRDUP(ptr_b);
            }
            break;
    }
    va_end ( va );
}


static void instruction_append ( instruction_t *instr ) {
    instr->prev = tail, instr->next = NULL;
    tail->next = instr;
    tail = instr;
}


static void instruction_finalize ( instruction_t *obsolete ) {
    switch ( obsolete->op )
    {
        case SYSLABEL: case SYSCALL: case LABEL: case CALL: case JUMP:
        case JUMPZERO: case JUMPNONZ: case PUSH: case POP: case MUL:
        case DIV: case DEC: case NEG: case CMPZERO:
            free ( obsolete->operands[0] );
            break;
        case MOVE: case ADD: case SUB:
            free ( obsolete->operands[0] ), free ( obsolete->operands[1] );
            break;
    }
    free ( obsolete );
}


static void free_instructions ( void ) {
    instruction_t *i = head, *j;
    while ( i != NULL ) {
        j = i->next;
        instruction_finalize ( i );
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

void generate ( FILE *stream, node_t *root ) {
    if ( root == NULL )
        return;

    switch ( root->type.index ) {
        case PROGRAM:
            /* Output the data segment, start the text segment */
            strings_output ( stream );
            fprintf ( stream, ".text\n" );

            /* Create the root of the main program list */
            instruction_init (
                tail = head = (instruction_t *)malloc(sizeof(instruction_t)),
                NIL
            );

            /* Generate code for all children */
            RECUR();

            /* Read arguments from command line */
            INSTR ( SYSLABEL, "main" );
            INSTR ( PUSH, R(ebp) );
            INSTR ( MOVE, R(esp), R(ebp) );
            INSTR ( MOVE, RO(8,esp), R(esi) );
            INSTR ( DEC, R(esi) );
            INSTR ( JUMPZERO, "noargs" );
            INSTR ( MOVE, RO(12,ebp), R(ebx) );
            INSTR ( SYSLABEL, "pusharg" );
            INSTR ( ADD, C(4), R(ebx) );
            INSTR ( PUSH, C(10) );
            INSTR ( PUSH, C(0) );
            INSTR ( PUSH, RI(ebx) );
            INSTR ( SYSCALL, "strtol" );
            INSTR ( ADD, C(12), R(esp) );
            INSTR ( PUSH, R(eax) );
            INSTR ( DEC, R(esi) );
            INSTR ( JUMPNONZ, "pusharg" );
            INSTR ( SYSLABEL, "noargs" );
   
            /* Call 1st function in VSL program, and exit w. returned value */
            INSTR ( CALL, root->children[0]->children[0]->children[0]->data );

            INSTR ( LEAVE );
            INSTR ( PUSH, R(eax) );
            INSTR ( SYSCALL, "exit" );
            
            print_instructions ( stream );
            free_instructions ();
            break;
        case FUNCTION:
			//INSTR(SYSLABEL, root->children[0]->type.text);
			INSTR(LABEL, root->children[0]->data);
			INSTR(PUSH, R(ebp));
			INSTR(MOVE, R(esp), R(ebp));
/*			int args = 0;
			
			if (root->n_children > 1 && root->children[1] && root->children[1]->type.index == VARIABLE_LIST) {
				args = root->children[1]->n_children;
			}
			for (int i = 0; i < args; i++) {
				INSTR( PUSH, C(0));
			}
			*/
			depth++;
			generate(stream, root->children[2]);
	/*		char offset[8];
			sprintf(offset, "%d", 4*(args+1));
			
			INSTR(ADD, offset, R(ebp));*/
			INSTR(LEAVE);
			
			//print_instructions(stream);
			//free_instructions();
			depth--;
            break;
        case BLOCK:
	//		INSTR( SYSLABEL, "BLOCK");
			INSTR(PUSH, R(ebp));
			INSTR(MOVE, R(esp), R(ebp));
			depth++;
			RECUR();
			// TODO: Cleanup
			INSTR(LEAVE);
	//		INSTR( SYSLABEL, "ENDBLOCK");
			depth--;
            break;
        case PRINT_STATEMENT:
		{
			INSTR( SYSLABEL, "// PRINTSTATEMENT");
			for (int i = 0; i < root->n_children; i++) {
				if (root->children[i]->type.index == TEXT) {
//					root->children[i]->data
					char temp[32];
					sprintf(temp, "$.STRING%d", *(int*)root->children[i]->data);
					INSTR(PUSH, temp); 
				} else {
					generate(stream, root->children[i]);
				}
				INSTR(SYSCALL, "puts");
				INSTR(ADD, C(4), R(esp));
			}
			// Create formatstring
			// Push formatstring
		INSTR( SYSLABEL, "// END OF PRINTSTATEMENT");
	/*		char offset[8];
			// add back the stack-pointer to remove the arguments from it.
			sprintf(offset, "%d", 4*(root->n_children+1));
			INSTR(ADD, offset, R(esp)); // Might need to do (numChildren + 1) * 4 to include the format string*/
		}
            break;
        case DECLARATION:
	//		INSTR(SYSLABEL, "DECLARATION");
			if (root->children[0]->type.index == VARIABLE_LIST) {
				INSTR( PUSH, C(0));
			} else {
				INSTR(SYSLABEL, "UNKNOWN DECLARATION");
			}
		//	INSTR(SYSLABEL, "ENDDECLARATION");
            break;
        case EXPRESSION:
			RECUR();
/*			if (strcmp(root->data, "F") == 0) {
				// TODO push arguments
				if (root->n_children > 1 && root->children[1]->type.index == EXPRESSION_LIST) {
					for (int i = 0; i < root->children[1]->n_children;i++) {
						if (root->children[1]->children[i]->type.index == INTEGER) {
							int target_offset = root->children[1]->children[i]->data;
							sprintf(offset, "%d", (target_offset));
							INSTR(PUSH, offset, R(ebp));
						}
					}
				}
				INSTR(CALL, root->children[0]->data); 
			} else {
				INSTR(SYSLABEL, root->data);
			}*/
			
			if (strcmp(root->data, "+") == 0) {
				INSTR(POP, R(EAX)); // Might want to save the register.
				INSTR(ADD, R(EAX), RO(0, ESP));
			} else if (strcmp(root->data, "-") == 0) {
				INSTR(MOVE, RO(0, ESP), R(EAX));
				INSTR(SUB, RO(-4, ESP), R(EAX)); // TODO: Verify ordering
				INSTR(PUSH, R(EAX));
			} else if (strcmp(root->data, "*") == 0) {
				INSTR(POP, R(EAX)); // Might want to save the register.
				INSTR(MUL, RO(0, ESP), R(EAX));
				INSTR(PUSH, R(EAX));				
			} else if (strcmp(root->data, "/") == 0) {
				INSTR(MOVE, RO(0, ESP), R(EAX));
				INSTR(CDQ);
				INSTR(DIV, RO(-4, ESP)); // TODO: Verify ordering
				INSTR(PUSH, R(EAX));
			} else if (strcmp(root->data, "^") == 0) {
				return 0/0; //TODO
			} else if (strcmp(root->data, "F") == 0) {
		//		INSTR(SYSLABEL, "// FUNCTIONCALL");
				// NB! TODO
			if(root->children[1]) {	
				for (int i = 0; i < root->children[1]->n_children; i++) {
					generate(stream, root->children[1]->children[i]);
				}
				}
				INSTR(CALL, root->children[0]->data);
				// Roll back the arguments
		if(root->children[1]) {	
			char temp_int[8];
				int rollback = root->children[1]->n_children * 4;
				sprintf(temp_int, "%d", rollback);
				INSTR(ADD, R(esp), temp_int);
				INSTR(PUSH, R(eax));
		//		INSTR(SYSLABEL, "// END OF FUNCTIONCALL");
				}
			}
			
            break;
        case VARIABLE:
/*			sprintf(offset,  "%d", root->entry->stack_offset);
			printf("Variable %s with offset %d\n", root->data, root->entry->stack_offset);
			printf("Depth: %d\n", root->entry->depth);
			printf("Current depth: %d \n", depth);*/
				   
		{
			int scopediff = depth - root->entry->depth;
		//	printf("Scopediff: %d\n", scopediff);
			INSTR(MOVE, RI(ebp), R(ebx));
			if (scopediff > 0) {
				for (int i = 0; i < scopediff; i++) {
					INSTR(MOVE, RI(ebx), R(ebx));
				}
			}
			// TODO, scoping
			
				char temp[32];
			//	printf("%d(%%ebx)", root->entry->stack_offset);
				sprintf(temp, "%d(%%ebx)", root->entry->stack_offset);
				INSTR(PUSH, temp); // Temp
		}
            break;
        case INTEGER:
			//printf("Integer node with data: %d\n", *(int32_t*)root->data);
		{
			char temp_int[8];
			sprintf(temp_int, "$%d", *(int32_t*)root->data);
			INSTR(PUSH, temp_int);
		}
            break;
        case ASSIGNMENT_STATEMENT:
		{
			generate (stream, root->children[1]);
			int target_offset = root->children[0]->entry->stack_offset;
			int target_depth = root->children[0]->entry->depth;
			int scopediff = depth - target_depth;

			INSTR(MOVE, R(ebp), R(ebx));
			if (scopediff > 0) {
				for (int i = 0; i < scopediff; i++) {
					INSTR(MOVE, RI(ebx), R(ebx));
				}
			}
			char temp[32];
			//	printf("%d(%%ebx)", root->entry->stack_offset);
			sprintf(temp, "%d(%%ebx)", target_offset);
			//INSTR(PUSH, temp); // Temp
		//	INSTR(SYSLABEL, "// WOOOT");
			INSTR(MOVE, RI(esp), R(eax));
			INSTR(MOVE, R(eax), temp);
		}
            break;
        case RETURN_STATEMENT:
			RECUR();
			INSTR(POP, R(eax));
            break;
        default:
            RECUR();
            break;
    }
}

static void
print_instructions ( FILE *output )
{
    #define OUT(...) fprintf ( output,##__VA_ARGS__ )
    instruction_t *i = head;
    while ( i != NULL )
    {
        switch ( i->op )
        {
            case CDQ:       OUT("\tcdq\n"); break;
            case LEAVE:     OUT("\tleave\n"); break;
            case RET:       OUT("\tret\n" ); break;
            case SYSLABEL:  OUT("%s:\n", i->operands[0]); break;
            case SYSCALL:   OUT("\tcall\t%s\n", i->operands[0]); break;
            case LABEL:     OUT("_%s:\n", i->operands[0]); break;
            case CALL:      OUT("\tcall\t_%s\n", i->operands[0]); break;
            case JUMP:      OUT("\tjmp\t%s\n", i->operands[0]); break;
            case JUMPZERO:  OUT("\tjz\t%s\n", i->operands[0]); break;
            case JUMPNONZ:  OUT("\tjnz\t%s\n", i->operands[0]); break;
            case PUSH:      OUT("\tpushl\t%s\n", i->operands[0]); break;
            case POP:       OUT("\tpopl\t%s\n", i->operands[0]); break;
            case MUL:       OUT("\timull\t%s\n", i->operands[0]); break;
            case DIV:       OUT("\tidivl\t%s\n", i->operands[0]); break;
            case DEC:       OUT("\tdecl\t%s\n", i->operands[0]); break;
            case NEG:       OUT("\tnegl\t%s\n", i->operands[0]); break;
            case CMPZERO:   OUT("\tcmpl\t$0,%s\n", i->operands[0]); break;
            case MOVE: OUT("\tmovl\t%s,%s\n", i->operands[0], i->operands[1]);
                break;
            case ADD: OUT("\taddl\t%s,%s\n", i->operands[0], i->operands[1]);
                break;
            case SUB: OUT("\tsubl\t%s,%s\n", i->operands[0], i->operands[1]);
                break;
        }
        i = i->next;
    }
    #undef OUT
}
