#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "nodetypes.h"
#include "tree.h"

/*
 * Root node of the program syntax tree, and parsing function generated by
 * bison - both of these live in 'parser.o'
 */
extern node_t *root;
extern int yyparse(void);

/* This is the main program, its only visible interface is the entry point. */
int main(int argc, char **argv);
