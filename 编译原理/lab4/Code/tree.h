#ifndef TREE_H_
#define TREE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

/**
 * @file tree.h
 * @brief Defines structures and functions for building and managing the abstract syntax tree (AST).
 */

/* ------------------------ */
/*     Type Definitions     */
/* ------------------------ */

/**
 * @enum NODE_TYPE
 * @brief Enumeration of possible node types in the syntax tree.
 *
 * Each value represents a different kind of terminal or non-terminal node
 * that can appear in the abstract syntax tree.
 */
typedef enum NODE_TYPE
{
    NODE_TYPE_NORMAL, ///< Regular non-terminal node
    NODE_TYPE_NULL,   ///< Empty production (epsilon)
    NODE_TYPE_INT,    ///< Integer literal node
    NODE_TYPE_FLOAT,  ///< Floating-point literal node
    NODE_TYPE_ID,     ///< Identifier node
    NODE_TYPE_TYPE,   ///< Type specifier node (e.g., "int", "float")
    NODE_TYPE_TOKEN   ///< Terminal token node (generic)
} NODE_TYPE;

/* ------------------------ */
/*    Structure Definitions */
/* ------------------------ */

/**
 * @struct Node
 * @brief Node structure for the abstract syntax tree.
 *
 * Represents a single node in the syntax tree, including metadata such as name,
 * type, and children. It supports both terminal and non-terminal symbols.
 */
typedef struct Node
{
    char *name;     /**< Name of the grammar symbol or token */
    int yylineno;   /**< Line number in source code */
    int child_num;  /**< Number of child nodes */
    NODE_TYPE type; /**< Type of the node (terminal or non-terminal) */

    /**
     * @union value
     * @brief Holds the value associated with terminal nodes.
     *
     * Only applicable when type is NODE_TYPE_INT, NODE_TYPE_FLOAT, or other terminals.
     */
    union
    {
        int ival;     /**< Integer value */
        float fval;   /**< Floating-point value */
        char str[32]; /**< Identifier or type string */
    } value;

    struct Node *first_child;  /**< Pointer to the first child node */
    struct Node *next_sibling; /**< Pointer to the next sibling node */
} Node;

/* ------------------------ */
/*   Function Declarations  */
/* ------------------------ */

/**
 * @brief Create and initialize a new syntax tree node.
 *
 * Allocates memory for a node and sets its properties. For non-terminal nodes,
 * links the provided child nodes as siblings in declaration order.
 *
 * @param name Name of the grammar symbol or token.
 * @param type Node classification (see NODE_TYPE).
 * @param yylineno Line number in the source code.
 * @param child_num Number of child nodes.
 * @param ... Variable argument list of child Node* pointers.
 * @return Pointer to the newly created Node.
 */
Node *createNode(char *name, NODE_TYPE type, int yylineno, int child_num, ...);

/**
 * @brief Check if a node and all its descendants are null nodes.
 *
 * Used to identify productions that reduce to empty or insignificant constructs.
 *
 * @param node Root of the subtree to examine.
 * @return true if all nodes are of type NODE_TYPE_NULL, false otherwise.
 */
bool CHECK_ALL_NULL(Node *node);

/**
 * @brief Print the syntax tree in an indented, readable format.
 *
 * Recursively traverses the syntax tree and prints each node, showing the tree
 * structure visually using indentation.
 *
 * @param node Root of the subtree to print.
 * @param level Current indentation level.
 */
void print_syntax_tree(Node *node, int level);

#endif // TREE_H_
