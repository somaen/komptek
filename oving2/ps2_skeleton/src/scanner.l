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
%}

%option array
%option noyywrap
%option yylineno

%%
FUNC			{ RETURNTOKEN(FUNC); } 
\/\/.*\n		{ }
[\t|\n|\ ]+ 	{ }
PRINT			{ RETURNTOKEN(PRINT); }
RETURN			{ RETURNTOKEN(RETURN); }
CONTINUE		{ RETURNTOKEN(CONTINUE); }
IF				{ RETURNTOKEN(IF); }
THEN			{ RETURNTOKEN(THEN); }
ELSE			{ RETURNTOKEN(ELSE); }
FI				{ RETURNTOKEN(FI); }
WHILE			{ RETURNTOKEN(WHILE); }
DONE			{ RETURNTOKEN(DONE); }
DO				{ RETURNTOKEN(DO); }
VAR				{ RETURNTOKEN(VAR);}
"+"|","|"-"|"/"	{ RETURNTOKEN(yytext[0]); }
"**"			{ RETURNTOKEN(POWER); }
"*"				{ RETURNTOKEN(yytext[0]); }
":="			{ RETURNTOKEN(ASSIGN); }
[0-9]+			{ yylval=atoi(yytext); RETURNTOKEN(NUMBER); }
\"(\\.|[^\"])*\" { RETURNTOKEN(STRING); }
[A-Z|a-z|_][A-Z|a-z|_|0-9]*  { yylval=yytext; RETURNTOKEN(IDENTIFIER);}
.          		{ RETURNTOKEN(yytext[0]); }
%%
