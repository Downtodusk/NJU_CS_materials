#include <stdio.h>
#include "tree.h"

extern FILE * yyin;
extern int Aerrors;
extern int Berrors;

int yyrestart(FILE* f);
int yyparse();

Node* root = NULL;

/* Main */
int main(int argc, char** argv){
    if(argc <= 1) { return 1; }
    FILE *f = fopen(argv[1], "r");
    if(!f){
        printf("Fail to open file");
        return 0;
    }
    yyrestart(f);
    yyparse();
    if(!(Aerrors+Berrors)){
        print_syntax_tree(root,0);
    }
    return 0;
}