#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

Node* createNode(char *name, NODE_TYPE state, int yylineno, int child_num, ...){
    Node *node = (Node*)malloc(sizeof(Node));

    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    node->name = (char*)malloc(sizeof(char) * strlen(name) + 1);
    strcpy(node->name, name);
    node->type = state;
    node->yylineno = yylineno;
    node->child_num = child_num;
    node->first_child = NULL;
    node->next_sibling = NULL;
    if(node->type == NODE_TYPE_NULL || node->child_num == 0){
        return node;
    }
    va_list param;
    va_start(param, child_num);
    Node *prev = NULL;
    for (int i = 0; i < child_num; i++) {
        Node *child = va_arg(param, Node*);
        if (child == NULL) {
            fprintf(stderr, "Child node is NULL at level %d\n", i);
            continue;
        }
        // 如果是第一个孩子，设置为 first_child
        if (i == 0) {
            node->first_child = child;
        } 
        // 否则连接到前一个孩子的 next_sibling
        else if (prev != NULL) {
            prev->next_sibling = child;
        }
        prev = child;  // 更新前一个节点
    }
    va_end(param);

    return node;
}

bool CHECK_ALL_NULL(Node* node) {
    if (node == NULL) return true;
    if (node->type != NODE_TYPE_NULL) return false;
    Node *child = node->first_child;
    while (child != NULL) {
        if (!CHECK_ALL_NULL(child)) return false; // 发现非 NULL 子节点，返回 false
        child = child->next_sibling;
    }
    return true;
}

void print_syntax_tree(Node* node, int level) {
    if (node == NULL) return;

    // 检查当前节点及其子树是否包含非 NULL 节点
    if (!CHECK_ALL_NULL(node)) {
        for (int i = 0; i < level; i++) {
            printf("  ");
        }
        switch (node->type) {
            case NODE_TYPE_INT:
                printf("INT: %d", node->value.ival);
                break;
            case NODE_TYPE_FLOAT:
                printf("FLOAT: %lf", node->value.fval);
                break;
            case NODE_TYPE_ID:
                printf("ID: %s", node->value.str);
                break;
            case NODE_TYPE_TYPE:
                printf("TYPE: %s", node->value.str);
                break;
            case NODE_TYPE_NORMAL:
                printf("%s (%d)", node->name, node->yylineno);
                break;
            case NODE_TYPE_TOKEN:
                printf("%s", node->name);
                break;
            default:
                break;
        }
        printf("\n");
        // 递归处理子节点
        Node *child = node->first_child;
        while (child != NULL) {
            print_syntax_tree(child, level + 1);
            child = child->next_sibling;
        }
    } else {
        // 子树全为 NULL
        node->type = NODE_TYPE_NULL;
    }
}