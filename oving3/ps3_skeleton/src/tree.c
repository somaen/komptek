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

node_t **merge_child_lists(node_t **mine_barn, int n_mine_barn, node_t **dine_barn, int n_dine_barn) {
	int vaare_barn_antall = n_mine_barn + n_dine_barn;
	node_t **vaare_barn = (node_t **)malloc(sizeof(node_t *) * (vaare_barn_antall));
	int j = 0;
	for (; j < n_dine_barn; j++) {
		vaare_barn[j] = dine_barn[j];
	}
	for (int i = 0; i < n_mine_barn; i++) {
		vaare_barn[j] = mine_barn[i];
		j++;
	}
	return vaare_barn;
}

void simplify_tree(node_t **simplified, node_t *root) {
	/* Certain children are nil, don't visit them */
	if (!root) {
		*simplified = NULL;
		return;
	}
	/* Perform the recursive calls DFS-style, possibly replacing a node by it's child */
	for (int i = 0; i < root->n_children; i++) {
		simplify_tree(simplified, root->children[i]);
		if (*simplified == NULL && root->type.index == DECLARATION_LIST) {
			for (int j = i; j < root->n_children; j++) {
				root->children[0] = root->children[1];
				root->n_children--;
				i--;
			}
		} else {
			root->children[i] = *simplified;
		}
	}

	node_t *keep;
	node_t **mine_barn;
	node_t **dine_barn;
	node_t **vaare_barn;
	int vaare_barn_antall;
	switch (root->type.index) {
	case PRINT_STATEMENT:
		assert(root->n_children == 1);
		mine_barn = root->children;
		keep = root->children[0];
		root->n_children = root->children[0]->n_children;;
		root->children = root->children[0]->children;
		free(mine_barn);
		free(keep->data);
		free(keep);
		*simplified = root;
		return;
	case DECLARATION_LIST:
	case FUNCTION_LIST:
	case STATEMENT_LIST:
	case PRINT_LIST:
	case EXPRESSION_LIST:
	case VARIABLE_LIST:
		if (root->children[0]->type.index == root->type.index) {
			dine_barn = root->children[0]->children;
			mine_barn = root->children;
			mine_barn++; /* Drop the sub-node */
			vaare_barn_antall =  root->n_children + root->children[0]->n_children - 1;
			keep = root->children[0]; /* The node we actually got rid of */
			vaare_barn = merge_child_lists(mine_barn, root->n_children - 1, dine_barn, root->children[0]->n_children);
			mine_barn--;
			free(mine_barn);
			root->children = vaare_barn;
			root->n_children = vaare_barn_antall;
			node_finalize(keep);
		}
		*simplified = root;
		return;
		/* Skal rydde: STATEMENT, PRINT_ITEM, PARAMETER_LIST, ARGUMENT_LIST */
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
		/* Case 1: Only an integer below us  */
		if (root->n_children == 1)  {
			if (root->data == NULL) {
				keep = root->children[0];
				node_finalize(root);
				*simplified = keep;
				return;
			}
			if (root->children[0]->type.index == INTEGER) {
				if ((((char *)root->data)[0]) == '-') {
					*((int *)root->children[0]->data) *= -1;
					*simplified = root->children[0];
					node_finalize(root);
					return;
				} 
			}
			*simplified = root;
			return;
		}
		/* Case 2: Multiple children, are all of them Integer? */
		else {
			char op_code = ((char *)root->data)[0];
			assert(root->n_children == 2);
			int running_result = 0;
			if (root->children[0]->type.index == INTEGER) {
				running_result = *((int *)root->children[0]->data);
				if (root->children[1]->type.index == INTEGER) {
					int32_t value = *((int *)root->children[1]->data);
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
				} else {
					*simplified = root;
					return;
				}
			} else {
				*simplified = root;
				return;
			}
			*((int *)root->children[0]->data) = running_result;
			keep = root->children[0];
			node_finalize(root);
			*simplified = keep;
			return;
		}
	default:
		/* Normal keepable node */
		*simplified = root;
		return;
	}
}
