#include "symtab.h"


static hash_t **scopes;
static symbol_t **values;
static char **strings;
static int32_t scopes_size = 16, scopes_index = -1;
static int32_t values_size = 16, values_index = -1;
static int32_t strings_size = 16, strings_index = -1;

/* Called once before tree-traversal, sets up
dynamic storage for a table of scopes, list of symbol_t-structures
, and a table of strings. Pointers, sizes and limits of these arrays are
already declared, but their size will be dynamically managed.*/
void symtab_init(void) {
	scopes = (hash_t**)malloc(sizeof(hash_t*) * scopes_size);
	values = (symbol_t**)malloc(sizeof(symbol_t*) * values_size);
	strings = (char**)malloc(sizeof(char*) * strings_size);
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
	printf("Should add %s\n", str);
	int length = strlen(str);
	strings_index++;
	if (strings_index >= strings_size) {
		strings_size *= 2; /* Follow the resizing convention of std::vector */
		strings = (char**)realloc(strings, strings_size * sizeof(char*));
	}
	char* temp = (char*)malloc(sizeof(char) * length);

	strings[strings_index] = temp;

	return strings_index;
}

/* Dumps the contents of the table to a provided output-stream */
void strings_output(FILE *stream) {
	printf("Strings output\n");
	fprintf(".data\n");
	for (int i = 0; i <= strings_index; i++) {
		fprintf(".STRINGS: %s\n", strings[i]);
	}
	fprintf(".globl main\n");
}


void scope_add(void) {
	printf("scope add\n");
}


void scope_remove(void) {
	printf("scope_remove\n");
}


void symbol_insert(char *key, symbol_t *value) {
#ifdef DUMP_SYMTAB
	fprintf(stderr, "Inserting (%s,%d)\n", key, value->stack_offset);
#endif
}


void symbol_get(symbol_t **value, char *key) {
	symbol_t *result = NULL;
#ifdef DUMP_SYMTAB
	if (result != NULL)
		fprintf(stderr, "Retrieving (%s,%d)\n", key, result->stack_offset);
#endif
}
