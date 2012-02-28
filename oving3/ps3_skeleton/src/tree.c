#include <assert.h>
#include "tree.h"

#ifdef DUMP_TREES
void node_print(FILE *output, node_t *root, uint32_t nesting) {
	if (root != NULL) {
		fprintf(output, "%*c%s", nesting, ' ', root->type.text);
		if (root->type.index == INTEGER)
			fprintf(output, "(%d)", *((int32_t *)root->data));
		if (root->type.index == VARIABLE || root->type.index == EXPRESSION) {
			if (root->data != NULL)
				fprintf(output, "(\"%s\")", (char *)root->data);
			else
				fprintf(output, "%p", root->data);
		}
		fputc('\n', output);
		for (int32_t i = 0; i < root->n_children; i++)
			node_print(output, root->children[i], nesting + 1);
	} else
		fprintf(output, "%*c%p\n", nesting, ' ', root);
}
#endif

void node_init(node_t *nd, nodetype_t type, void *data, uint32_t n_children, ...) {
	va_list child_list;
	*nd = (node_t) {
		type, data, NULL, n_children,
		      (node_t **) malloc(n_children * sizeof(node_t *))
	};
	va_start(child_list, n_children);
	for (uint32_t i = 0; i < n_children; i++)
		nd->children[i] = va_arg(child_list, node_t *);
	va_end(child_list);
}

void node_finalize(node_t *discard) {
	if (discard != NULL) {
		free(discard->data), free(discard->children);
		free(discard);
	}
}

void destroy_subtree(node_t *discard) {
	if (discard != NULL) {
		for (uint32_t i = 0; i < discard->n_children; i++)
			destroy_subtree(discard->children[i]);
		node_finalize(discard);
	}
}

node_t *prune_redundant_node(node_t *node) {

}

void simplify_tree(node_t **simplified, node_t *root) {
	/* TODO: implement the simplifications of the tree here */
	
	/*
	 * Skal rydde: STATEMENT, PRINT_ITEM, PARAMETER_LIST, ARGUMENT_LIST
	 */
	
	/* Certain children are nil, don't visit them */
	if (!root) {
		*simplified = NULL;
		return;
	}
	/* Perform the recursive calls DFS-style, possibly replacing a node by it's child */
	for (int i = 0; i < root->n_children; i++) {
		simplify_tree(simplified, root->children[i]);
		root->children[i] = *simplified; 
	}

	node_t *keep;
	switch (root->type.index) {
		case STATEMENT:
		case PRINT_ITEM:
		case PARAMETER_LIST:
		case ARGUMENT_LIST:
			/* We have one of the prunable types, delete the node, and return it's child
			   thus replacing it by it's parent when the returns traverse back up */
			assert(root->n_children == 1);
			keep = root->children[0];
			free(root);
			*simplified = keep; 
			return;
		default:
			/* Normal keepable node */
			*simplified = root;
			return;
	}
}
