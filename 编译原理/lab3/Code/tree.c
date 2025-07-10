#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

/**
 * @file tree.c
 * @brief Syntax tree construction and printing for the C-- compiler.
 *
 * This module provides utilities for building the abstract syntax tree (AST),
 * checking for null subtrees, and visualizing the syntax tree structure.
 */

/**
 * @brief Create a new AST node.
 *
 * Allocates and initializes a node with its name, type, line number, and a list of children.
 * All children are linked as siblings in left-to-right order.
 *
 * @param name Name of the node.
 * @param state Node type (e.g., terminal, non-terminal).
 * @param yylineno Line number where the node is defined.
 * @param child_num Number of children.
 * @param ... Variadic list of child node pointers.
 * @return Pointer to the newly created Node.
 */
Node *createNode(char *name, NODE_TYPE state, int yylineno, int child_num, ...)
{
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    node->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(node->name, name);
    node->type = state;
    node->yylineno = yylineno;
    node->child_num = child_num;
    node->first_child = NULL;
    node->next_sibling = NULL;

    // No children case
    if (node->type == NODE_TYPE_NULL || child_num == 0)
        return node;

    // Link child nodes
    va_list param;
    va_start(param, child_num);
    Node *prev = NULL;
    for (int i = 0; i < child_num; i++)
    {
        Node *child = va_arg(param, Node *);
        if (child == NULL)
        {
            fprintf(stderr, "Child node is NULL at position %d\n", i);
            continue;
        }

        if (i == 0)
            node->first_child = child;
        else if (prev != NULL)
            prev->next_sibling = child;

        prev = child;
    }
    va_end(param);

    return node;
}

/**
 * @brief Check if the subtree rooted at a node is entirely null.
 *
 * Recursively checks whether the node and all its descendants are of type `NODE_TYPE_NULL`.
 *
 * @param node Root node of the subtree.
 * @return true if all nodes are null; false otherwise.
 */
bool CHECK_ALL_NULL(Node *node)
{
    if (node == NULL)
        return true;
    if (node->type != NODE_TYPE_NULL)
        return false;

    Node *child = node->first_child;
    while (child != NULL)
    {
        if (!CHECK_ALL_NULL(child))
            return false;
        child = child->next_sibling;
    }
    return true;
}

/**
 * @brief Print the abstract syntax tree.
 *
 * Recursively prints the tree in an indented format. Each node is printed with its type and value,
 * if applicable. Null subtrees are not printed.
 *
 * @param node Root of the syntax tree.
 * @param level Current depth level (used for indentation).
 */
void print_syntax_tree(Node *node, int level)
{
    if (node == NULL)
        return;

    if (!CHECK_ALL_NULL(node))
    {
        for (int i = 0; i < level; i++)
            printf("  ");

        switch (node->type)
        {
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

        Node *child = node->first_child;
        while (child != NULL)
        {
            print_syntax_tree(child, level + 1);
            child = child->next_sibling;
        }
    }
    else
    {
        node->type = NODE_TYPE_NULL;
    }
}
