%{
#include "parser.h"
#include "tree.h"

#define DUMP_TOKENS

#ifdef DUMP_TOKENS
    #define RETURNTOKEN(t) do {                                          \
        fprintf ( stderr, "TOKEN ( %d,\t'%s' )\n", t, yytext );     \
        return t;                                                   \
    } while ( 0 )
#else
    #define RETURNTOKEN(t) return t
#endif
#undef RETURNTOKEN(t)
#define RETURNTOKEN(t) {}
%}

%option array
%option noyywrap
%option yylineno


%%
FUNC		RETURNTOKEN(FUNC); 
[{|}|(|)]			{ }
\/\/.*\n		{ }
[\t|\n|\ ]+ 	{ }
\[			{ }
\]			{ }
PRINT		{ printf("PRINT"); RETURNTOKEN(PRINT); }
RETURN		{ printf("RETURN"); RETURNTOKEN(RETURN); }
CONTINUE	{ printf("CONTINUE"); RETURNTOKEN(CONTINUE); }
IF			{ printf("IF");	RETURNTOKEN(IF); }
THEN		{ printf("THEN"); RETURNTOKEN(THEN); }
ELSE		{ printf("ELSE"); RETURNTOKEN(ELSE); }
[FI]			{ RETURNTOKEN(FI); RETURNTOKEN(FI); }
WHILE		{ printf("WHILE"); RETURNTOKEN(WHILE); }
DO			{ printf("DO"); RETURNTOKEN(DO); }
DONE		{ printf("NUMBER"); RETURNTOKEN(NUMBER); }
VAR			{ printf("VAR"); RETURNTOKEN(VAR);}
"+"			{ printf("ADD"); }
","			{ printf(" , "); }
"**"		{ printf("POW"); }
"*"			{ printf("MULTIPLY"); }
":="		{ printf("ASSIGN"); }
"/"			{ printf("DIVIDE"); }
"-"			{ printf("SUBTRACT"); }
[0-9]+		{ printf("NUMBER(%s)\n", yytext); }
\"(\\.|[^\"])*\" { printf("STRING(%s)\n", yytext); }
[A-Z|a-z|_][A-Z|a-z|_|0-9]*  { printf("IDENT(%s)", yytext); RETURNTOKEN(IDENTIFIER);}
.           { printf("UNKNOWN"); }
\s+ 		{}
%%
