%{
#include "syntax.tab.h"
#include <stdlib.h>

int Aerrors = 0;
int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option yylineno

id      {letter}({letter}|{digit})*
int     0|[1-9]{digit}*
float   ({digit}+\.{digit}+)

digit   [0-9]
space   [ \t\v\f]+
letter  [_a-zA-Z]

type    "int"|"float"
relop   ">"|"<"|">="|"<="|"=="|"!="
%%

";"     { yylval.node = createNode("SEMI", NODE_TYPE_TOKEN, yylineno, 0); return SEMI; }
","     { yylval.node = createNode("COMMA", NODE_TYPE_TOKEN, yylineno, 0); return COMMA; }
"="     { yylval.node = createNode("ASSIGNOP", NODE_TYPE_TOKEN, yylineno, 0); return ASSIGNOP; }
{relop} { yylval.node = createNode("RELOP", NODE_TYPE_TOKEN, yylineno, 0); return RELOP; }
"+"     { yylval.node = createNode("PLUS", NODE_TYPE_TOKEN, yylineno, 0); return PLUS; }
"-"     { yylval.node = createNode("MINUS", NODE_TYPE_TOKEN, yylineno, 0); return MINUS; }
"*"     { yylval.node = createNode("STAR", NODE_TYPE_TOKEN, yylineno, 0); return STAR; }
"/"     { yylval.node = createNode("DIV", NODE_TYPE_TOKEN, yylineno, 0); return DIV; }
"&&"    { yylval.node = createNode("AND", NODE_TYPE_TOKEN, yylineno, 0); return AND; }
"||"    { yylval.node = createNode("OR", NODE_TYPE_TOKEN, yylineno, 0); return OR; }
"."     { yylval.node = createNode("DOT", NODE_TYPE_TOKEN, yylineno, 0); return DOT; }
"!"     { yylval.node = createNode("NOT", NODE_TYPE_TOKEN, yylineno, 0); return NOT; }
{type}  { yylval.node = createNode("TYPE", NODE_TYPE_TYPE, yylineno, 0); strcpy(yylval.node->value.str, yytext); return TYPE; }
"("     { yylval.node = createNode("LP", NODE_TYPE_TOKEN, yylineno, 0); return LP; }
")"     { yylval.node = createNode("RP", NODE_TYPE_TOKEN, yylineno, 0); return RP; }
"["     { yylval.node = createNode("LB", NODE_TYPE_TOKEN, yylineno, 0); return LB; }
"]"     { yylval.node = createNode("RB", NODE_TYPE_TOKEN, yylineno, 0); return RB; }
"{"     { yylval.node = createNode("LC", NODE_TYPE_TOKEN, yylineno, 0); return LC; }
"}"     { yylval.node = createNode("RC", NODE_TYPE_TOKEN, yylineno, 0); return RC; }

struct  { yylval.node = createNode("STRUCT", NODE_TYPE_TOKEN, yylineno, 0); return STRUCT; }
return  { yylval.node = createNode("RETURN", NODE_TYPE_TOKEN, yylineno, 0); return RETURN; }
if      { yylval.node = createNode("IF", NODE_TYPE_TOKEN, yylineno, 0); return IF; }
else    { yylval.node = createNode("ELSE", NODE_TYPE_TOKEN, yylineno, 0); return ELSE; }
while   { yylval.node = createNode("WHILE", NODE_TYPE_TOKEN, yylineno, 0); return WHILE; }

{id}    { yylval.node = createNode("ID", NODE_TYPE_ID, yylineno, 0); strcpy(yylval.node->value.str, yytext); return ID; }
{int}   { yylval.node = createNode("INT", NODE_TYPE_INT, yylineno, 0); yylval.node->value.ival = atoi(yytext); return INT; }
{float} { yylval.node = createNode("FLOAT", NODE_TYPE_FLOAT, yylineno, 0); yylval.node->value.fval = strtof(yytext, NULL); return FLOAT; }
{space} { /* Do Nothing*/ }

[\n\r]  { yycolumn = 1; }

"//".*  { /* Do Nothing */ }
"/*"    {
    char pre = input(), next = input();
    while(next){
        if(pre == '*' && next == '/') break;
        pre = next;
        next = input();
    }
    if(!next){ printf("Error type B at line %d: Illegal /*.\n", yylineno); Aerrors++; }
}

.   { printf("Error type A at Line %d: Mysterious characters: \'%s\'\n", yylineno, yytext); Aerrors += 1; }
%%