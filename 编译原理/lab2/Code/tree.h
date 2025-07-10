#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

extern int yylineno;
typedef struct Node Node;

enum NODE_TYPE {
    NODE_TYPE_NORMAL,
    NODE_TYPE_NULL,
    NODE_TYPE_INT,
    NODE_TYPE_FLOAT,
    NODE_TYPE_ID,
    NODE_TYPE_TYPE,
    NODE_TYPE_TOKEN
};

typedef enum NODE_TYPE NODE_TYPE;

struct Node {
    char *name;
    int yylineno;
    int child_num;
    NODE_TYPE type;
    union {
        int ival;
        float fval;
        char str[32];
    } value;
    Node* first_child;
    Node* next_sibling;
};


Node* createNode(char *name, NODE_TYPE state, int yylineno, int child_num, ...);
bool CHECK_ALL_NULL(Node* node);
void print_syntax_tree(Node* node, int index);