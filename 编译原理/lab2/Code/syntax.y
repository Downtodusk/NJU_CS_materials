%locations

%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "tree.h"
    #include "lex.yy.c"

    extern Node* root;
    extern int yylineno;
    
    int Berrors = 0;

    void yyerror(const char *msg);
%}

%union {
    struct Node* node;// all type should be Node to build parTree
}

%token <node>   INT FLOAT ID 
                SEMI COMMA ASSIGNOP RELOP
                PLUS MINUS STAR DIV
                AND OR DOT NOT TYPE
                LP RP LB RB LC RC
                IF ELSE WHILE STRUCT RETURN

%right  ASSIGNOP
%left   OR
%left   AND
%left   RELOP
%left   PLUS MINUS
%left   STAR DIV
%right  NOT
%left   LP RP LB RB DOT

%nonassoc   LOWER_THAN_ELSE
%nonassoc   ELSE

%type <node>    Program 
                ExtDefList ExtDef ExtDecList
                Specifier 
%type <node>    StructSpecifier 
                OptTag Tag 
                VarDec FunDec VarList ParamDec
%type <node>    CompSt 
                StmtList Stmt 
                DefList Def 
                DecList Dec 
                Exp 
                Args

%%

Program : ExtDefList    {$$=createNode("Program", NODE_TYPE_NORMAL, @$.first_line, 1, $1); root = $$; }
        ;

ExtDefList  : ExtDef ExtDefList {$$=createNode("ExtDefList", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
            | /* epsilon */     {$$=createNode("ExtDefList", NODE_TYPE_NULL, @$.first_line, 0); }
            ;

ExtDef  : Specifier ExtDecList SEMI {$$=createNode("ExtDef", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
        | Specifier SEMI            {$$=createNode("ExtDef", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
        | Specifier FunDec CompSt   {$$=createNode("ExtDef", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
        | error SEMI                {Berrors++;}
        ;

ExtDecList  : VarDec                    {$$=createNode("ExtDecList", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
            | VarDec COMMA ExtDecList   {$$=createNode("ExtDecList", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
            ;

Specifier   : TYPE              {$$=createNode("Specifier", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
            | StructSpecifier   {$$=createNode("Specifier", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
            ;

StructSpecifier : STRUCT OptTag LC DefList RC   {$$=createNode("StructSpecifier", NODE_TYPE_NORMAL, @$.first_line, 5, $1, $2, $3, $4, $5); }
                | STRUCT Tag                    {$$=createNode("StructSpecifier", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
                ;

OptTag  : ID            {$$=createNode("OptTag", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
        | /* epsilon */ {$$=createNode("OptTag", NODE_TYPE_NULL, @$.first_line, 0); }
        ;

Tag : ID    {$$=createNode("Tag", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
    ;

VarDec  : ID                {$$=createNode("VarDec", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
        | VarDec LB INT RB  {$$=createNode("VarDec", NODE_TYPE_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
        ;

FunDec  : ID LP VarList RP  {$$=createNode("FunDec", NODE_TYPE_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
        | ID LP RP          {$$=createNode("FunDec", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
        | ID LP error RP    {Berrors++;}
        | error RP          {Berrors++;}
        ;

VarList : ParamDec COMMA VarList    {$$=createNode("VarList", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
        | ParamDec                  {$$=createNode("VarList", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
        ;

ParamDec    : Specifier VarDec  {$$=createNode("ParamDec", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
            | Specifier error   {Berrors++;}
            ;

CompSt  : LC DefList StmtList RC    {$$=createNode("CompSt", NODE_TYPE_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
        | error RC                  {Berrors++;}
        ;

StmtList    : Stmt StmtList {$$=createNode("StmtList", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
            | /* epsilon */ {$$=createNode("StmtList", NODE_TYPE_NULL, @$.first_line, 0); }
            ;

Stmt: Exp SEMI                                  {$$=createNode("Stmt", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
    | CompSt                                    {$$=createNode("Stmt", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
    | RETURN Exp SEMI                           {$$=createNode("Stmt", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   {$$=createNode("Stmt", NODE_TYPE_NORMAL, @$.first_line, 5, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt               {$$=createNode("Stmt", NODE_TYPE_NORMAL, @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt                      {$$=createNode("Stmt", NODE_TYPE_NORMAL, @$.first_line, 5, $1, $2, $3, $4, $5); }
    | Exp error                                 {Berrors++;}
    | error SEMI                                {Berrors++;}
    ;

DefList : Def DefList   {$$=createNode("DefList", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
        | /* epsilon */ {$$=createNode("DefList", NODE_TYPE_NULL, @$.first_line, 0); }
        ;

Def : Specifier DecList SEMI    {$$=createNode("Def", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Specifier error SEMI      {Berrors++;}
    | error SEMI                {Berrors++;}
    ;

DecList : Dec               {$$=createNode("DecList", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
        | Dec COMMA DecList {$$=createNode("DecList", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
        ;

Dec : VarDec                {$$=createNode("Dec", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
    | VarDec ASSIGNOP Exp   {$$=createNode("Dec", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    ;

Exp : Exp ASSIGNOP Exp  {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp AND Exp       {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp OR Exp        {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp RELOP Exp     {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp PLUS Exp      {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp MINUS Exp     {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp STAR Exp      {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp DIV Exp       {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | LP Exp RP         {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | MINUS Exp         {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
    | NOT Exp           {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 2, $1, $2); }
    | ID LP Args RP     {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
    | ID LP RP          {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp LB Exp RB     {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
    | Exp DOT ID        {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | ID                {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
    | INT               {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
    | FLOAT             {$$=createNode("Exp", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
    | error             {Berrors++;}
    ;

Args: Exp COMMA Args    {$$=createNode("Args", NODE_TYPE_NORMAL, @$.first_line, 3, $1, $2, $3); }
    | Exp               {$$=createNode("Args", NODE_TYPE_NORMAL, @$.first_line, 1, $1); }
    ;
%%

void yyerror(const char *msg){
    printf("Error type B at Line %d : %s\n",yylineno,msg);
}