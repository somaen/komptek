#include "symtab.h"


static hash_t **scopes;
static symbol_t **values;
static char **strings;
static int32_t scopes_size = 16, scopes_index = -1;
static int32_t values_size = 16, values_index = -1;
static int32_t strings_size = 16, strings_index = -1;
static int32_t stack_offset = 0;

/* Called once before tree-traversal, sets up
dynamic storage for a table of scopes, list of symbol_t-structures
, and a table of strings. Pointers, sizes and limits of these arrays are
already declared, but their size will be dynamically managed.*/
void symtab_init(void) {
	scopes = (hash_t **)malloc(sizeof(hash_t *) * scopes_size);
	values = (symbol_t **)malloc(sizeof(symbol_t *) * values_size);
	strings = (char **)malloc(sizeof(char *) * strings_size);
}

/* Called at end of execution, to remove tables and contents */
void symtab_finalize(void) {
	for (int i = 0; i <= scopes_index; i++) {
		free(scopes[i]);
	}
	free(scopes);
	for (int i = 0; i <= values_index; i++) {
		free(values[i]);
	}
	free(values);
	for (int i = 0; i <= strings_index; i++) {
		free(strings[i]);
	}
	free(strings);
}

/* Appends a string to the table, resizing if appropriate,
returns the index of the added string.*/
int32_t strings_add(char *str) {
	int length = strlen(str) + 1;
	strings_index++;
	if (strings_index >= strings_size) {
		strings_size *= 2; /* Follow the resizing convention of std::vector */
		strings = (char **)realloc(strings, strings_size * sizeof(char *));
	}
	char *temp = (char *)malloc(sizeof(char) * length);
	strcpy(temp, str);
	strings[strings_index] = temp;

	return strings_index;
}

/* Dumps the contents of the table to a provided output-stream */
void strings_output(FILE *stream) {
	fprintf(stream, ".data\n");
	fprintf(stream, ".INTEGER: .string \"%%d \"\n");
	for (int i = 0; i <= strings_index; i++) {
		fprintf(stream, ".STRING%d: .string %s\n", i, strings[i]);
	}
	fprintf(stream, ".globl main\n");
}

void scope_add(void) {
	scopes_index++;
	if (scopes_index > scopes_size) {
		scopes_size *= 2;
		scopes = (hash_t **)realloc(scopes, scopes_size * sizeof(hash_t *));
		exit(0);
	}
	scopes[scopes_index] = ght_create(8);
	stack_offset = 0;
}

void scope_remove(void) {
	/* iterator code for freeing the value-items, cut'n-pasted from the libghthash-documentation:
	 * http://www.bth.se/people/ska/sim_home/libghthash_mk2_doc/ght__hash__table_8h.html#a28
	 * (But it's in no way critical to this task, just needed to properly clean up after ourselves)
	 */
	ght_iterator_t iterator;
	void *p_key;
	void *p_e;
	for (p_e = ght_first(scopes[scopes_index], &iterator, &p_key); p_e; p_e = ght_next(scopes[scopes_index], &iterator, &p_key)) {
		free(p_e);
	}
	ght_finalize(scopes[scopes_index]);
	scopes_index--;
}

void symbol_insert(char *key, symbol_t *value) {
	value->depth = scopes_index;
	ght_insert(scopes[scopes_index], value, strlen(key), key);
#ifdef DUMP_SYMTAB
	//fprintf(stderr, "Inserting (%s,%d) depth(%d)\n", key, value->stack_offset, value->depth);
	fprintf(stderr, "Inserting (%s,%d)\n", key, value->stack_offset, value->depth);
#endif
}

void symbol_get(symbol_t **value, char *key) {
	symbol_t *result = NULL;
	for (int i = scopes_index; i >= 0; i--) {
		result = ght_get(scopes[i], strlen(key), key);
		if (result)
			break;
	}
#ifdef DUMP_SYMTAB
	if (result != NULL)
		fprintf(stderr, "Retrieving (%s,%d)\n", key, result->stack_offset);
#endif
}
