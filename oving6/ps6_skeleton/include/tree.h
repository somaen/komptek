#ifndef TREE_H
#define TREE_H

#include <stdarg.h>
#include "symtab.h"
#include "nodetypes.h"

/*
 * Macro for creating a heap-allocated duplicate of a string.
 * This macro mirrors the function 'strdup' (which is itself a pretty
 * common standard extension by GCC and many others). The function is not
 * part of the C99 standard because it allocates heap memory as a
 * side-effect, so it is reimplemented here in terms of std. calls.
 */
#define STRDUP(s) strncpy ( malloc ( strlen(s)+1 ), s, strlen(s)+1 )

/*
 * Basic data structure for syntax tree nodes.
 * Both the label data and the list of children are consistently allocated
 * in a dynamic fashion, even if data is just a single character, integer,
 * etc., because it simplifies using a recursive traversal of the tree, both
 * for decoration, printing and destruction.
 */
typedef struct n {
	nodetype_t type;        /* Type of this node */
	void *data;             /* Data label for terminals and expressions */
	symbol_t *entry;        /* Pointer to symtab entry */
	uint32_t n_children;    /* Number of children */
	struct n **children;    /* Pointers to child nodes */
} node_t;


/*
 *  Function prototypes: implementations are found in tree.c
 */
void node_init(
    node_t *n, nodetype_t type, void *data, uint32_t n_children, ...
);
void node_print(FILE *output, node_t *root, uint32_t nesting);
void node_finalize(node_t *discard);

void destroy_subtree(node_t *discard);
void simplify_tree(node_t **simplified, node_t *root);
void bind_names(node_t *root);

#endif
