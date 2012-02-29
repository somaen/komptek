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

/* Simple convenience function to make the code in handle-expression more readable */
char get_char_data(const node_t *node) {
	return *((char *)(node->data));
}

int get_int_data(node_t *node) {
	return *((int *)node->data);
}

void set_int_data(node_t *node, int val) {
	*((int *)node->data) = val;
}

/* Expressions have a few cases, the first one is similar to the 4 above, in that it
   should have it's children pushed up, there is however a special-case for unary-minus
   since unary-minus needs to get it's effect pushed down to the INTEGER-node it contains
*/
node_t *handle_expression(node_t *root) {
	node_t *keep;
	/* Case 1: Only an integer below us  */
	if (root->n_children == 1)  {
		if (root->data == NULL) {
			keep = root->children[0];
			node_finalize(root);
			return keep;
		}
		if (root->children[0]->type.index == INTEGER) {
			/* If the data-field is '-' we need to negate the INTEGER-child */
			if (get_char_data(root) == '-') {
				set_int_data(root->children[0], get_int_data(root->children[0]) * (-1));
				keep = root->children[0];
				node_finalize(root);
				return keep;
			}
		}
		return root;
	}
	/* Case 2: Multiple children, are all of them Integer? */
	else {
		char op_code = ((char *)root->data)[0];
		assert(root->n_children == 2);
		int running_result = 0;
		/* Catch the case where the left node is INTEGER */
		if (root->children[0]->type.index == INTEGER) {
			running_result = get_int_data(root->children[0]);
			if (root->children[1]->type.index == INTEGER) {
				int32_t value = get_int_data(root->children[1]);
				switch (op_code) {
				case '+':
					running_result += value;
					break;
				case '-':
					running_result -= value;
					break;
				case '*':
					running_result *= value;
					break;
				case '/':
					running_result = running_result / value;
					break;
				default:
					printf("Error: %c\n", op_code);
					assert(0);
					break;
				}
				node_finalize(root->children[1]);
				set_int_data(root->children[0], running_result);
				keep = root->children[0];
				node_finalize(root);
				return keep;
			}
		}
		/* If we didn't return in the double-if, either the left or right side wasn't INTEGER
		   so, we can safely just keep this node unchanged
		*/
		return root;
	}
}

/* This function merges two node's children, with dine_barn preceeding mine_barn
   then sets those children as the children of root */
void merge_child_lists(node_t **mine_barn, int n_mine_barn, node_t **dine_barn, int n_dine_barn, node_t *root) {
	uint32_t vaare_barn_antall = n_mine_barn + n_dine_barn;
	node_t **vaare_barn = (node_t **)malloc(sizeof(node_t *) * (vaare_barn_antall));
	uint32_t j = 0;
	for (; j < n_dine_barn; j++) {
		vaare_barn[j] = dine_barn[j];
	}
	for (uint32_t i = 0; i < n_mine_barn; i++) {
		vaare_barn[j] = mine_barn[i];
		j++;
	}
	free(root->children);
	root->children = vaare_barn;
	root->n_children = vaare_barn_antall;
}



void simplify_tree(node_t **simplified, node_t *root) {
	/* Certain children are nil, don't visit them */
	if (!root) {
		*simplified = NULL;
		return;
	}
	node_t *keep;
	/* Perform the recursive calls DFS-style, possibly replacing a node by it's child */
	for (uint32_t i = 0; i < root->n_children; i++) {
		simplify_tree(simplified, root->children[i]);
		if (*simplified == NULL && root->type.index == DECLARATION_LIST) {
			keep = root->children[1];
			realloc(root->children, sizeof(node_t *));
			root->children[0] = keep;
			root->n_children = 1;
			i--;
		} else {
			root->children[i] = *simplified;
		}
	}
	node_t *purge;
	node_t **mine_barn;
	node_t **dine_barn;
	node_t **vaare_barn;
	int vaare_barn_antall;
	switch (root->type.index) {
		/* Fold a PRINT_STATEMENT into it's parent PRINT_ITEM by moving the children */
	case PRINT_STATEMENT:
		assert(root->n_children == 1);
		purge = root->children[0];
		merge_child_lists(NULL, 0, root->children[0]->children, root->children[0]->n_children, root);
		node_finalize(purge);
		*simplified = root;
		return;
		/* The following lists might have equal sublists, check for them, and move their children up
		   making sure that their_children preceed our children */
	case DECLARATION_LIST:
	case FUNCTION_LIST:
	case STATEMENT_LIST:
	case PRINT_LIST:
	case EXPRESSION_LIST:
	case VARIABLE_LIST:
		if (root->children[0]->type.index == root->type.index) {
			dine_barn = root->children[0]->children;
			mine_barn = root->children;
			purge = root->children[0]; /* The node we actually got rid of */
			merge_child_lists(mine_barn + 1, root->n_children - 1, dine_barn, root->children[0]->n_children, root);
			node_finalize(purge);
		}
		*simplified = root;
		return;
		/* The following nodes can be simply removed, pushing their child one step up */
	case STATEMENT:
	case PRINT_ITEM:
	case PARAMETER_LIST:
	case ARGUMENT_LIST:
		/* We have one of the prunable types, delete the node, and return it's child
		   thus replacing it by it's parent when the returns traverse back up */
		assert(root->n_children == 1);
		keep = root->children[0];
		node_finalize(root);
		*simplified = keep;
		return;
	case EXPRESSION:
		*simplified = handle_expression(root);
		return;
	default:
		/* Normal keepable node */
		*simplified = root;
		return;
	}
}
