#include "symtab.h"


static hash_t **scopes;
static symbol_t **values;
static char **strings;
static int32_t scopes_size = 16, scopes_index = -1;
static int32_t values_size = 16, values_index = -1;
static int32_t strings_size = 16, strings_index = -1;


void
symtab_init ( void )
{
}


void
symtab_finalize ( void )
{
}


int32_t
strings_add ( char *str )
{
    return 0;
}


void
strings_output ( FILE *stream )
{
}


void
scope_add ( void )
{
}


void
scope_remove ( void )
{
}


void
symbol_insert ( char *key, symbol_t *value )
{
#ifdef DUMP_SYMTAB
fprintf ( stderr, "Inserting (%s,%d)\n", key, value->stack_offset );
#endif
}


void
symbol_get ( symbol_t **value, char *key )
{
    symbol_t *result = NULL;
#ifdef DUMP_SYMTAB
    if ( result != NULL )
        fprintf ( stderr, "Retrieving (%s,%d)\n", key, result->stack_offset );
#endif
}
