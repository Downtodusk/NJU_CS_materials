#include "semantic.h"
#include <assert.h>

#define TABLE_SIZE 200

/* Global variables */
Symbol symbol_table[TABLE_SIZE]; // Hash table for symbol storage
Symbol layersHead;               // Head of layer linked list
bool array_flag = false;         // Flag for array type checking

/**
 * Print semantic error message with variable arguments
 * @param error_type Error code
 * @param yylineno Line number where error occurs
 * @param msg Format string for error message
 */
void print_semantic_error(int error_type, int yylineno, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    printf("Error type %d at line %d: ", error_type, yylineno);
    vprintf(msg, args);
    printf("\n");
    va_end(args);
}

// Count number of children nodes in AST
int count_children(Node *node)
{
    int count = 0;
    for (Node *child = node->first_child; child; child = child->next_sibling)
    {
        count++;
    }
    return count;
}

/**
 * Hash function for symbol table
 * PJW hash algorithm variant
 * @param name String to be hashed
 * @return Hash value within TABLE_SIZE range
 */
unsigned int hash_pjw(char *name)
{
    unsigned int val = 0, i;
    for (; *name; ++name)
    {
        val = (val << 2) + *name;
        if ((i = val & ~0x3fff))
        {
            val = (val ^ (i >> 12)) & 0x3fff;
        }
    }
    return val % TABLE_SIZE;
}

/**
 * Initialize symbol table with:
 * 1. Empty hash table
 * 2. Layer structure
 * 3. Predefined read/write functions
 */
void init_symbol_table()
{
    // Initialize hash table slots
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        symbol_table[i] = NULL;
    }

    /* Initialize layer structure */
    layersHead = (Symbol)malloc(sizeof(Symbol_t));
    layersHead->hashNext = NULL;
    layersHead->layerNext = NULL;

    // Create global layer
    Symbol globalLayer = (Symbol)malloc(sizeof(Symbol_t));
    globalLayer->hashNext = NULL;
    globalLayer->layerNext = NULL;
    layersHead->hashNext = globalLayer;

    /* Add predefined read/write functions */
    Function read = (Function)malloc(sizeof(Function_t));
    Function write = (Function)malloc(sizeof(Function_t));

    // Both functions return int type
    Type tint = (Type)malloc(sizeof(Type_t));
    tint->kind = BASIC;
    tint->basic = TYPE_INT;

    read->ret_type = tint;
    write->ret_type = tint;
    read->param = NULL; // read() takes no parameters

    // write() takes one int parameter
    FieldList arg = (FieldList)malloc(sizeof(FieldList_t));
    arg->type = tint;
    write->param = arg;

    read->defined = true;
    write->defined = true;
    strcpy(read->name, "read");
    strcpy(write->name, "write");

    // Create symbol entries
    Type rtype = (Type)malloc(sizeof(Type_t));
    rtype->kind = FUNCTION;
    rtype->func = read;

    Type wtype = (Type)malloc(sizeof(Type_t));
    wtype->kind = FUNCTION;
    wtype->func = write;

    Symbol rsym = create_symbol(read->name, rtype, 0);
    Symbol wsym = create_symbol(write->name, wtype, 0);
    insert_symbol(rsym);
    insert_symbol(wsym);
}

/**
 * Create new symbol entry
 * @param str Symbol name
 * @param type Type information
 * @param yylineno Line number where symbol appears
 * @return Newly created symbol
 */
Symbol create_symbol(char *str, Type type, int yylineno)
{
    Symbol sym = (Symbol)malloc(sizeof(Symbol_t));
    strcpy(sym->name, str);
    sym->type = type;
    sym->lineno = yylineno;
    sym->hashNext = NULL;
    sym->op = NULL;
    return sym;
}

/* Symbol table operations */
void insert_symbol(Symbol sym)
{
    // Insert into hash table (chain hashing)
    unsigned int index = hash_pjw(sym->name);
    Symbol tail = symbol_table[index];
    symbol_table[index] = sym;
    sym->hashNext = tail;

    // Insert into current layer list
    Symbol curLayer = layersHead->hashNext;
    tail = curLayer->layerNext;
    curLayer->layerNext = sym;
    sym->layerNext = tail;
}

// Search symbol in entire table (all layers)
Symbol search_symbol_all(char *str)
{
    unsigned int index = hash_pjw(str);
    Symbol sym = symbol_table[index];

    while (sym != NULL)
    {
        if (strcmp(sym->name, str) == 0)
        {
            break;
        }
        sym = sym->hashNext;
    }
    return sym;
}

// Search symbol only in current layer
Symbol search_symbol_layer(char *name)
{
    Symbol currentLayer = layersHead->hashNext;
    Symbol symbol = currentLayer->layerNext;

    while (symbol != NULL)
    {
        if (strcmp(symbol->name, name) == 0)
        {
            break;
        }
        symbol = symbol->layerNext;
    }
    return symbol;
}

// Search for function symbol specifically
Symbol search_symbol_func(char *name)
{
    unsigned int hash = hash_pjw(name);
    Symbol tmp = symbol_table[hash];

    while (tmp != NULL)
    {
        if (strcmp(tmp->name, name) == 0 && tmp->type->kind == FUNCTION)
        {
            break;
        }
        tmp = tmp->hashNext;
    }
    return tmp;
}

/* Layer management functions */
void enter_layer()
{
    Symbol currentLayer = (Symbol)malloc(sizeof(Symbol_t));
    currentLayer->hashNext = NULL;
    currentLayer->layerNext = NULL;
    Symbol tail = layersHead->hashNext;
    layersHead->hashNext = currentLayer;
    currentLayer->hashNext = tail;
}

void exit_layer()
{
    Symbol currentLayer = layersHead->hashNext;
    layersHead->hashNext = currentLayer->hashNext;

    // Remove all symbols in current layer
    Symbol symbol = currentLayer->layerNext;
    while (symbol != NULL)
    {
        del_single_symbol(symbol->name);
        symbol = symbol->layerNext;
    }
}

/**
 * Type comparison functions
 * Compare two field lists recursively
 */
bool fieldEqual(FieldList list1, FieldList list2)
{
    if (!list1 && !list2)
        return true;
    if (!list1 || !list2)
        return false;
    return typeEqual(list1->type, list2->type) && fieldEqual(list1->next, list2->next);
}

/**
 * Compare two types recursively
 * Handles all type kinds (BASIC, ARRAY, FUNCTION, STRUCTURE)
 */
bool typeEqual(Type type1, Type type2)
{
    if (!type1 || !type2)
        return true;
    if (type1->kind != type2->kind)
        return false;

    switch (type1->kind)
    {
    case BASIC:
        return type1->basic == type2->basic;
    case ARRAY:
        return typeEqual(type1->array.elem, type2->array.elem) &&
               type1->array.dimension == type2->array.dimension;
    case FUNCTION:
        return !strcmp(type1->func->name, type2->func->name) &&
               typeEqual(type1->func->ret_type, type2->func->ret_type) &&
               fieldEqual(type1->func->param, type2->func->param);
    default: // STRUCTURE or STRUCT_DEF
        return !strcmp(type1->structure->name, type2->structure->name) &&
               fieldEqual(type1->structure->members, type2->structure->members);
    }
}

/**
 * Semantic analysis entry point
 * @param root Root node of AST
 */
void start_semantic_analysis(Node *root)
{
    init_symbol_table();
    SemanticAnalysis(root);
}

// Main semantic analysis driver function
void SemanticAnalysis(Node *node)
{
    if (node == NULL)
        return;
    ExtDefList(node->first_child);
}

/* External definition list processing */
void ExtDefList(Node *node)
{
    if (node->first_child != NULL)
    {
        ExtDef(node->first_child);
        ExtDefList(node->first_child->next_sibling);
    }
}

/**
 * Process external definition (variable/function/struct)
 * @param node ExtDef node in AST
 */
void ExtDef(Node *node)
{
    Type type = Specifier(node->first_child);
    if (!type)
        return;

    Node *sibling = node->first_child->next_sibling;

    // Handle different cases of external definitions
    if (strcmp(sibling->name, "SEMI") == 0)
    {
        return; // Just type declaration, no action needed
    }
    if (strcmp(sibling->name, "ExtDecList") == 0)
    {
        ExtDecList(sibling, type);
        return;
    }

    // Must be function definition at this point
    assert(strcmp(sibling->name, "FunDec") == 0);
    assert(strcmp(sibling->next_sibling->name, "CompSt") == 0);

    Function func = FunDec(sibling);
    func->ret_type = type;

    // Create function type
    Type func_type = (Type)malloc(sizeof(Type_t));
    func_type->kind = FUNCTION;
    func_type->func = func;

    // Check for duplicate definition
    Symbol sym = search_symbol_all(func->name);
    if (sym == NULL)
    {
        // New function definition
        Symbol newSymbol = create_symbol(func->name, func_type, func->lineno);
        insert_symbol(newSymbol);
        enter_layer();
        CompSt(sibling->next_sibling, func->ret_type, func->name);
        exit_layer();
    }
    else
    {
        // Error handling for duplicate definitions
        if (sym->type->kind != FUNCTION)
        {
            print_semantic_error(4, node->yylineno, "name repeat");
        }
        else
        {
            print_semantic_error(4, node->yylineno,
                                 "function repeat define: %d", sym->lineno);
        }
    }
}

/* Type specifier processing */
Type Specifier(Node *node)
{
    // Handle basic types (int/float)
    if (strcmp(node->first_child->name, "TYPE") == 0)
    {
        Type type = (Type)malloc(sizeof(Type_t));
        type->kind = BASIC;
        char *str = node->first_child->value.str;
        type->basic = (strcmp(str, "int") == 0) ? TYPE_INT : TYPE_FLOAT;
        return type;
    }
    // Handle struct types
    return StructSpecifier(node->first_child);
}

/* External declaration list processing */
void ExtDecList(Node *node, Type type)
{
    VarDec(node->first_child, false, type, 0);
    if (count_children(node) == 3)
    {
        ExtDecList(node->first_child->next_sibling->next_sibling, type);
    }
}

/**
 * Function declaration processing
 * @param node FunDec node in AST
 * @return Function descriptor
 */
Function FunDec(Node *node)
{
    Function func = (Function)malloc(sizeof(Function_t));
    strcpy(func->name, node->first_child->value.str);
    func->lineno = node->yylineno;
    func->param = NULL;

    // Handle parameter list if exists
    if (count_children(node) == 3)
    {
        func->param = NULL; // No parameters
    }
    else
    {
        enter_layer();
        func->param = VarList(node->first_child->next_sibling->next_sibling);
        exit_layer();
    }
    return func;
}

/* Struct type processing */
Type StructSpecifier(Node *node)
{
    Type type = (Type)malloc(sizeof(Type_t));
    type->kind = STRUCTURE;

    // Case 1: Struct Tag (existing struct)
    if (count_children(node) == 2)
    {
        Node *Tag = node->first_child->next_sibling;
        char *str = Tag->first_child->value.str;
        Symbol sym = search_symbol_all(str);

        if (sym == NULL || sym->type->kind != STRUCT_DEF)
        {
            print_semantic_error(17, node->yylineno, "undefined struct");
            return NULL;
        }
        type->structure = sym->type->structure;
    }
    // Case 2: Struct definition
    else
    {
        char *str = OptTag(node->first_child->next_sibling);

        if (strcmp(str, "") != 0)
        {
            // Check for duplicate struct name
            Symbol sym = search_symbol_all(str);
            if (sym != NULL)
            {
                print_semantic_error(16, node->yylineno, "Duplicated struct name");
                str = NULL;
                return NULL;
            }

            // Create new struct type
            Type new_type = (Type)malloc(sizeof(Type_t));
            new_type->kind = STRUCT_DEF;
            new_type->structure = (Structure)malloc(sizeof(Structure_t));
            strcpy(new_type->structure->name, str);

            // Process member definitions
            new_type->structure->members =
                DefList(node->first_child->next_sibling->next_sibling->next_sibling, true);

            // Check for duplicate field names
            check_dup_in_struct(new_type->structure->members);

            // Insert into symbol table
            Symbol newSymbol = create_symbol(str, new_type, node->yylineno);
            insert_symbol(newSymbol);
            type->structure = newSymbol->type->structure;
            free(str);
        }
        else
        {
            // Anonymous struct
            type->structure = (Structure)malloc(sizeof(Structure_t));
            strcpy(type->structure->name, str);
            enter_layer();
            type->structure->members =
                DefList(node->first_child->next_sibling->next_sibling->next_sibling, true);
            exit_layer();
            check_dup_in_struct(type->structure->members);
        }
    }
    return type;
}

/* Optional tag processing */
char *OptTag(Node *node)
{
    char *name = (char *)malloc(sizeof(char) * 32);
    if (node->type == NODE_TYPE_NULL)
    {
        strcpy(name, ""); // Anonymous struct
    }
    else
    {
        strcpy(name, node->first_child->value.str);
    }
    return name;
}

/* Variable list processing */
FieldList VarList(Node *node)
{
    FieldList list = ParamDec(node->first_child);
    if (count_children(node) == 3)
    {
        if (list == NULL)
        {
            list = VarList(node->first_child->next_sibling->next_sibling);
        }
        else
        {
            list->next = VarList(node->first_child->next_sibling->next_sibling);
        }
    }
    return list;
}

/* Parameter declaration processing */
FieldList ParamDec(Node *node)
{
    Type type = Specifier(node->first_child);
    if (type == NULL)
    {
        return NULL;
    }
    return VarDec(node->first_child->next_sibling, false, type, 0);
}

/* Definition list processing */
FieldList DefList(Node *node, bool in_struct)
{
    if (node->type == NODE_TYPE_NULL)
    {
        return NULL;
    }

    FieldList list = Def(node->first_child, in_struct);
    if (list == NULL)
    {
        return DefList(node->first_child->next_sibling, in_struct);
    }

    // Append to the end of list
    FieldList ptr = list;
    while (ptr->next != NULL)
    {
        ptr = ptr->next;
    }
    ptr->next = DefList(node->first_child->next_sibling, in_struct);
    return list;
}

/* Single definition processing */
FieldList Def(Node *node, bool in_struct)
{
    Type type = Specifier(node->first_child);
    if (type == NULL)
    {
        return NULL;
    }
    return DecList(node->first_child->next_sibling, in_struct, type);
}

/* Declaration list processing */
FieldList DecList(Node *node, bool in_struct, Type type)
{
    FieldList list = Dec(node->first_child, in_struct, type);
    if (count_children(node) > 1)
    {
        if (list == NULL)
        {
            return DecList(node->first_child->next_sibling->next_sibling, in_struct, type);
        }
        FieldList ptr = list;
        while (ptr->next != NULL)
        {
            ptr = ptr->next;
        }
        ptr->next = DecList(node->first_child->next_sibling->next_sibling, in_struct, type);
    }
    return list;
}

/**
 * Single declaration processing
 * Handles both variable and struct field declarations
 */
FieldList Dec(Node *node, bool in_struct, Type type)
{
    FieldList list = VarDec(node->first_child, in_struct, type, 0);

    // Check for invalid initialization in struct
    if (in_struct && count_children(node) == 3)
    {
        print_semantic_error(15, node->yylineno,
                             "initialize member in struct defination");
    }

    // Check type matching for initialization
    if (list != NULL && count_children(node) == 3)
    {
        Type right = Exp(node->first_child->next_sibling->next_sibling);
        if (!typeEqual(list->type, right))
        {
            print_semantic_error(5, node->yylineno, "type mismatched");
        }
    }
    return list;
}

/**
 * Variable declaration processing
 * Handles both simple variables and array declarations
 * @param node VarDec node in AST
 * @param in_struct Whether declaration is inside struct
 * @param type Base type of variable
 * @param layer Current array dimension (0 for non-arrays)
 * @return Field list entry for the variable
 */
FieldList VarDec(Node *node, bool in_struct, Type type, int layer)
{
    // Case 1: Simple variable (ID)
    if (node->child_num == 1)
    {
        char *id = node->first_child->value.str;

        // Check for duplicate definition
        Symbol sym = search_symbol_all(id);
        Symbol symA = search_symbol_all(id);
        if (sym != NULL || (symA != NULL && symA->type->kind == STRUCT_DEF))
        {
            print_semantic_error(3, node->yylineno,
                                 "var name conflict (Var: %s)", symA->name);
            if (!in_struct)
                return NULL;
        }

        // Create field list entry
        FieldList list = (FieldList)malloc(sizeof(FieldList_t));
        strcpy(list->name, id);
        list->type = type;
        list->lineno = -1; // -1 indicates regular variable
        list->next = NULL;

        // Insert into symbol table if not struct member
        if (sym == NULL && !in_struct)
        {
            Symbol new_sym = create_symbol(id, list->type, node->yylineno);
            insert_symbol(new_sym);
        }
        else
        {
            list->lineno = node->yylineno; // Mark as struct member
        }
        return list;
    }
    // Case 2: Array declaration (first dimension)
    else if (layer == 0)
    {
        Type arr_type = (Type)malloc(sizeof(Type_t));
        arr_type->kind = ARRAY;
        arr_type->array.elem = type;
        arr_type->array.dimension = 1;
        arr_type->array.size[layer] = node->first_child->next_sibling->next_sibling->value.ival;
        return VarDec(node->first_child, in_struct, arr_type, layer + 1);
    }
    // Case 3: Array declaration (additional dimensions)
    else
    {
        array_flag = true;
        type->array.dimension++;
        type->array.size[layer] = node->first_child->next_sibling->next_sibling->value.ival;
        return VarDec(node->first_child, in_struct, type, layer + 1);
    }
}

/**
 * Compound statement processing
 * Handles variable definitions and statements within a scope
 * @param node CompSt node in AST
 * @param ret_type Expected return type for function (NULL if not in function)
 * @param funcName Current function name (NULL if not in function)
 */
void CompSt(Node *node, Type ret_type, char *funcName)
{
    // Add function parameters to symbol table
    if (funcName != NULL)
    {
        Symbol sym = search_symbol_func(funcName);
        FieldList params = sym->type->func->param;
        while (params != NULL)
        {
            Symbol parm = (Symbol)malloc(sizeof(Symbol_t));
            strcpy(parm->name, params->name);
            parm->type = params->type;
            if (parm->type->kind == ARRAY)
            {
                array_flag = true;
            }
            insert_symbol(parm);
            params = params->next;
        }
    }

    // Process local definitions and statements
    DefList(node->first_child->next_sibling, false);
    StmtList(node->first_child->next_sibling->next_sibling, ret_type);
}

/* Statement list processing */
void StmtList(Node *node, Type ret_type)
{
    if (node->type == NODE_TYPE_NULL)
        return;
    Stmt(node->first_child, ret_type);
    StmtList(node->first_child->next_sibling, ret_type);
}

/**
 * Single statement processing
 * Handles all statement types in the language
 * @param node Stmt node in AST
 * @param ret_type Expected return type for return statements
 */
void Stmt(Node *node, Type ret_type)
{
    Node *first = node->first_child;

    // Expression statement
    if (strcmp(first->name, "Exp") == 0)
    {
        Exp(first);
    }
    // Compound statement
    else if (strcmp(first->name, "CompSt") == 0)
    {
        enter_layer();
        CompSt(first, ret_type, NULL);
        exit_layer();
    }
    // Return statement
    else if (strcmp(first->name, "RETURN") == 0)
    {
        Type exp_type = Exp(first->next_sibling);
        if (!typeEqual(exp_type, ret_type))
        {
            print_semantic_error(8, node->yylineno, "error return type");
        }
    }
    // While statement
    else if (strcmp(first->name, "WHILE") == 0)
    {
        Exp(first->next_sibling->next_sibling);
        Stmt(first->next_sibling->next_sibling->next_sibling->next_sibling, ret_type);
    }
    // If or if-else statement
    else
    {
        Exp(first->next_sibling->next_sibling);
        Stmt(first->next_sibling->next_sibling->next_sibling->next_sibling, ret_type);
        if (count_children(node) == 7)
        {
            Stmt(first->next_sibling->next_sibling->next_sibling->next_sibling->next_sibling->next_sibling, ret_type);
        }
    }
}

/**
 * Expression processing
 * Handles all expression types and returns their type
 * @param node Exp node in AST
 * @return Type of the expression
 */
Type Exp(Node *node)
{
    Node *first = node->first_child;

    // Identifier cases
    if (strcmp(first->name, "ID") == 0)
    {
        char *str = first->value.str;
        Symbol sym = search_symbol_all(str);

        // Simple variable reference
        if (count_children(node) == 1)
        {
            if (sym == NULL)
            {
                print_semantic_error(1, node->yylineno, "undefined var '%s'", str);
                return NULL;
            }
            return sym->type;
        }
        // Function call
        else
        {
            if (sym == NULL)
            {
                print_semantic_error(2, node->yylineno, "undefined function");
                return NULL;
            }
            if (sym->type->kind != FUNCTION)
            {
                print_semantic_error(11, node->yylineno, "() for other type var");
                return NULL;
            }

            // Check argument matching
            FieldList args = (count_children(node) == 4) ? Args(first->next_sibling->next_sibling) : NULL;
            FieldList params = sym->type->func->param;
            if (!fieldEqual(args, params))
            {
                print_semantic_error(9, node->yylineno, "args and params mismatched");
            }
            return sym->type->func->ret_type;
        }
    }

    // Literal cases
    if (strcmp(node->first_child->name, "INT") == 0)
    {
        Type type = (Type)malloc(sizeof(Type_t));
        type->kind = BASIC;
        type->basic = TYPE_INT;
        return type;
    }
    if (strcmp(node->first_child->name, "FLOAT") == 0)
    {
        Type type = (Type)malloc(sizeof(Type_t));
        type->kind = BASIC;
        type->basic = TYPE_FLOAT;
        return type;
    }

    // Parenthesized expression
    if (strcmp(node->first_child->name, "LP") == 0)
    {
        return Exp(node->first_child->next_sibling);
    }

    // Unary operators
    if (strcmp(node->first_child->name, "MINUS") == 0)
    {
        Type exp_type = Exp(node->first_child->next_sibling);
        if (exp_type != NULL && exp_type->kind != BASIC)
        {
            print_semantic_error(7, node->yylineno, "illegal -");
            return NULL;
        }
        return exp_type;
    }
    if (strcmp(node->first_child->name, "NOT") == 0)
    {
        Type exp_type = Exp(first->next_sibling);
        if (exp_type != NULL && (exp_type->kind != BASIC || exp_type->basic != TYPE_INT))
        {
            print_semantic_error(7, node->yylineno, "only INT");
            return NULL;
        }
        return exp_type;
    }

    Node *second = first->next_sibling;

    // Assignment expression
    if (strcmp(second->name, "ASSIGNOP") == 0)
    {
        Type right = Exp(second->next_sibling);
        Type left = Exp(first);
        Node *left_child = first;

        // Check valid lvalue
        if (!((count_children(left_child) == 1 && strcmp(left_child->first_child->name, "ID") == 0) ||
              (count_children(left_child) == 3 && strcmp(left_child->first_child->next_sibling->name, "DOT") == 0) ||
              (count_children(left_child) == 4 && strcmp(left_child->first_child->next_sibling->name, "LB") == 0)))
        {
            print_semantic_error(6, node->yylineno, "right exp in left");
            return NULL;
        }

        // Check type compatibility
        if (left != NULL && right != NULL && (!typeEqual(left, right) || left->kind == FUNCTION))
        {
            print_semantic_error(5, node->yylineno, "mismatched type");
            return NULL;
        }
        return left;
    }

    // Logical operators (AND/OR)
    if (strcmp(second->name, "AND") == 0 || strcmp(second->name, "OR") == 0)
    {
        Type left = Exp(first);
        Type right = Exp(second->next_sibling);
        if (left != NULL && right != NULL)
        {
            if (left->kind == BASIC && right->kind == BASIC &&
                left->basic == TYPE_INT && right->basic == TYPE_INT)
            {
                return left;
            }
            print_semantic_error(7, node->yylineno, "only INT");
        }
        return NULL;
    }

    // Structure field access
    if (strcmp(second->name, "DOT") == 0)
    {
        Type stk = Exp(first);
        if (stk == NULL)
            return NULL;

        if (stk->kind != STRUCTURE && stk->kind != STRUCT_DEF)
        {
            print_semantic_error(13, node->yylineno, "illegal .");
            return NULL;
        }

        // Search for field in structure
        char *str = second->next_sibling->value.str;
        FieldList ptr = stk->structure->members;
        while (ptr != NULL)
        {
            if (strcmp(ptr->name, str) == 0)
            {
                return ptr->type;
            }
            ptr = ptr->next;
        }
        print_semantic_error(14, node->yylineno, "mismatch '%s'", str);
        return NULL;
    }

    // Array access
    if (strcmp(second->name, "LB") == 0)
    {
        Type base = Exp(first);
        Type offset = Exp(second->next_sibling);
        if (base == NULL || offset == NULL)
            return NULL;

        if (base->kind != ARRAY)
        {
            print_semantic_error(10, node->yylineno, "illegal []");
            return NULL;
        }
        if (offset->kind != BASIC || offset->basic != TYPE_INT)
        {
            print_semantic_error(12, node->yylineno, "index must be INT");
            return base->array.elem;
        }

        // Handle multi-dimensional arrays
        if (base->array.dimension == 1)
        {
            return base->array.elem;
        }
        else
        {
            Type type = (Type)malloc(sizeof(Type_t));
            type->kind = ARRAY;
            type->array.dimension = base->array.dimension - 1;
            type->array.elem = base->array.elem;
            return type;
        }
    }

    // Relational and arithmetic operators
    Type left = Exp(first);
    Type right = Exp(second->next_sibling);
    if (left == NULL || right == NULL)
        return NULL;

    // Type checking for operators
    if (left->kind != BASIC || right->kind != BASIC)
    {
        print_semantic_error(7, node->yylineno, "only basic type for math");
        return NULL;
    }
    if (left->basic != right->basic)
    {
        print_semantic_error(7, node->yylineno, "mismatched type");
        return NULL;
    }

    // Relational operators return int
    if (strcmp(node->first_child->next_sibling->name, "RELOP") == 0)
    {
        Type type = (Type)malloc(sizeof(Type_t));
        type->kind = BASIC;
        type->basic = TYPE_INT;
        return type;
    }
    return left;
}

/**
 * Process function arguments
 * @param node Args node in AST
 * @return Linked list of argument types
 */
FieldList Args(Node *node)
{
    FieldList list = (FieldList)malloc(sizeof(FieldList_t));
    list->type = Exp(node->first_child); // Get type of first argument

    // Process remaining arguments if present
    if (count_children(node) == 3)
    {
        list->next = Args(node->first_child->next_sibling->next_sibling);
    }
    else
    {
        list->next = NULL;
    }
    return list;
}

/**
 * Check for duplicate field names in struct definition
 * @param head Head of field list to check
 */
void check_dup_in_struct(FieldList head)
{
    FieldList outer = head;

    while (outer != NULL)
    {
        FieldList inner = outer->next;
        FieldList pre_inner = outer;

        while (inner != NULL)
        {
            if (strcmp(outer->name, inner->name) == 0)
            {
                // Found duplicate field
                print_semantic_error(15, inner->lineno,
                                     "duplicated field '%s'", inner->name);

                // Remove duplicate from list
                pre_inner->next = inner->next;
                inner = pre_inner->next;
            }
            else
            {
                pre_inner = inner;
                inner = inner->next;
            }
        }
        outer = outer->next;
    }
}

/* Helper function to delete a single symbol */
void del_single_symbol(char *name)
{
    unsigned int hash = hash_pjw(name);
    Symbol pre = NULL;
    Symbol tmp = symbol_table[hash];

    // Find symbol in hash chain
    while (tmp != NULL)
    {
        if (strcmp(tmp->name, name) == 0)
        {
            break;
        }
        pre = tmp;
        tmp = tmp->hashNext;
    }

    // Remove from hash table
    if (tmp != NULL)
    {
        if (pre == NULL)
        {
            symbol_table[hash] = tmp->hashNext;
        }
        else
        {
            pre->hashNext = tmp->hashNext;
        }
    }
}