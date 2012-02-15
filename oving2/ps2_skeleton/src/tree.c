#include "tree.h"
#include <string.h>

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
	nd->children = (node_t *) malloc(sizeof(node_t) * n_children);
	if (data) {
		int len = strlen(data);
		nd->data = malloc(len+1);
		strcpy(nd->data, data);
	} else {
		nd->data = NULL;
	}
	nd->type = type;
	nd->n_children = n_children;

	va_list varargs;
	va_start(varargs, n_children);
	for (int i=0; i < nd->n_children;i++) {
		node_t* child = va_arg(varargs, node_t*);
		nd->children[i] = child;
	}
	va_end(varargs);
}


void node_finalize(node_t *discard) {
	if (discard) {
		free(discard->data);
		free(discard);
	}
}


void destroy_subtree(node_t *discard) {
	if (!discard)
		return;
	for (int i=0; i < discard->n_children; i++) {
		destroy_subtree((node_t*)discard->children + i);
	}
	node_finalize(discard);
}
