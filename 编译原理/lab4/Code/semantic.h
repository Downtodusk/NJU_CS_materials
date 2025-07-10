#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tree.h"

/**
 * @file semantic.h
 * @brief Defines the type system and symbol table for semantic analysis.
 */

/**
 * @def TYPE_INT
 * @brief Integer type identifier.
 */
#define TYPE_INT 1

/**
 * @def TYPE_FLOAT
 * @brief Floating-point type identifier.
 */
#define TYPE_FLOAT 2

/* Forward declarations for type aliases */
typedef struct Operand_ Operand_t;
typedef Operand_t *Operand;
typedef struct Type_ Type_t;
typedef Type_t *Type;
typedef struct FieldList_ FieldList_t;
typedef FieldList_t *FieldList;
typedef struct Structure_ Structure_t;
typedef struct Function_ Function_t;
typedef struct Symbol_ Symbol_t;
typedef Structure_t *Structure;
typedef Function_t *Function;
typedef Symbol_t *Symbol;

/**
 * @struct Type_
 * @brief Represents all possible types in the language.
 */
struct Type_
{
    enum
    {
        BASIC,      /**< Basic type (int or float) */
        ARRAY,      /**< Array type */
        STRUCTURE,  /**< Structure variable type */
        STRUCT_DEF, /**< Structure definition */
        FUNCTION    /**< Function type */
    } kind;

    union
    {
        int basic; /**< For BASIC: TYPE_INT or TYPE_FLOAT */

        struct
        {
            Type elem;     /**< Element type */
            int dimension; /**< Number of dimensions */
            int size[10];  /**< Sizes for each dimension */
        } array;

        Structure structure; /**< For STRUCTURE or STRUCT_DEF */
        Function func;       /**< For FUNCTION */
    };
};

/**
 * @struct Symbol_
 * @brief Symbol table entry for variables, structs, and functions.
 */
struct Symbol_
{
    char name[32];    /**< Symbol name */
    Type type;        /**< Type information */
    int lineno;       /**< Line number of declaration */
    Symbol hashNext;  /**< Next symbol in hash chain */
    Symbol layerNext; /**< Next symbol in current layer */
    Operand op;       /**< Intermediate code operand */
};

/**
 * @struct Structure_
 * @brief Represents a structure definition or instance.
 */
struct Structure_
{
    char name[32];     /**< Structure name */
    FieldList members; /**< List of member fields */
};

/**
 * @struct Function_
 * @brief Contains metadata for a function declaration/definition.
 */
struct Function_
{
    char name[32];   /**< Function name */
    Type ret_type;   /**< Return type */
    int lineno;      /**< Declaration line number */
    FieldList param; /**< Function parameter list */
    bool defined;    /**< True if function is defined */
};

/**
 * @struct FieldList_
 * @brief Field list used for function parameters and struct members.
 */
struct FieldList_
{
    char name[32];  /**< Field name */
    Type type;      /**< Field type */
    int lineno;     /**< Line number of declaration */
    FieldList next; /**< Next field in the list */
};

/* ------------------------ */
/* Symbol Table Operations  */
/* ------------------------ */

/**
 * @brief Initialize the symbol table.
 */
void init_symbol_table();

/**
 * @brief Create a new symbol.
 * @param str Symbol name
 * @param type Symbol type
 * @param lineno Declaration line number
 * @return New symbol instance
 */
Symbol create_symbol(char *str, Type type, int lineno);

/**
 * @brief Insert a symbol into the symbol table.
 * @param sym Symbol to insert
 */
void insert_symbol(Symbol sym);

/**
 * @brief Search for a symbol in all scopes.
 * @param str Symbol name
 * @return Matching symbol or NULL
 */
Symbol search_symbol_all(char *str);

/**
 * @brief Search for a symbol in the current scope only.
 * @param str Symbol name
 * @return Matching symbol or NULL
 */
Symbol search_symbol_layer(char *str);

/**
 * @brief Search specifically for a function symbol.
 * @param str Function name
 * @return Matching function symbol or NULL
 */
Symbol search_symbol_func(char *str);

/**
 * @brief Enter a new scope layer.
 */
void enter_layer();

/**
 * @brief Exit the current scope layer.
 */
void exit_layer();

/**
 * @brief Delete a symbol by name from current scope.
 * @param str Symbol name
 */
void del_single_symbol(char *str);

/* ------------------------ */
/* Type System Utilities    */
/* ------------------------ */

/**
 * @brief Compare two field lists for structural equality.
 * @param list1 First field list
 * @param list2 Second field list
 * @return true if equal, false otherwise
 */
bool fieldEqual(FieldList list1, FieldList list2);

/**
 * @brief Compare two types for equality.
 * @param type1 First type
 * @param type2 Second type
 * @return true if equal, false otherwise
 */
bool typeEqual(Type type1, Type type2);

/**
 * @brief Check for duplicate fields in a struct.
 * @param head Head of the field list
 */
void check_dup_in_struct(FieldList head);

/* ------------------------ */
/* Semantic Analysis Phases */
/* ------------------------ */

/**
 * @brief Entry point of semantic analysis.
 * @param root Root node of the syntax tree
 */
void start_semantic_analysis(Node *root);

/**
 * @brief Perform recursive semantic analysis on a node.
 * @param node Syntax tree node
 */
void SemanticAnalysis(Node *node);

void ExtDefList(Node *node);
void ExtDef(Node *node);
Type Specifier(Node *node);
void ExtDecList(Node *node, Type type);
Function FunDec(Node *node);
Type StructSpecifier(Node *node);
char *OptTag(Node *node);
FieldList VarList(Node *node);
FieldList ParamDec(Node *node);
FieldList DefList(Node *node, bool in_struct);
FieldList Def(Node *node, bool in_struct);
FieldList DecList(Node *node, bool in_struct, Type type);
FieldList Dec(Node *node, bool in_struct, Type type);
FieldList VarDec(Node *node, bool in_struct, Type type, int layer);
void CompSt(Node *node, Type ret_type, char *funcName);
void StmtList(Node *node, Type ret_type);
void Stmt(Node *node, Type ret_type);
Type Exp(Node *node);
FieldList Args(Node *node);

#endif // SEMANTIC_H
