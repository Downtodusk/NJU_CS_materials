#include "ir.h"
#include <assert.h>

// Temporary variable and label counters
int tempNum = 0;
int labelNum = 0;

/**
 * @brief Creates a new operand.
 *
 * This function allocates memory for a new operand and initializes its fields.
 * @return Operand A newly created operand.
 */
Operand newOperand()
{
    Operand op = (Operand)malloc(sizeof(Operand_t));
    assert(op != NULL && "Failed to allocate memory for Operand");
    op->type = NULL;
    op->is_addr = false;
    return op;
}

/**
 * @brief Creates a new temporary operand.
 *
 * This function creates a new operand of type TEMP_VAR and assigns a unique number to it.
 * @return Operand A newly created temporary operand.
 */
Operand newTemp()
{
    Operand op = newOperand();
    op->kind = TEMP_VAR;
    op->no = tempNum++;
    return op;
}

/**
 * @brief Creates a new variable operand with the given name.
 *
 * This function creates a new operand of type VARIABLE and assigns the provided name to it.
 * @param name The name of the variable.
 * @return Operand A newly created variable operand.
 */
Operand newVar(char *name)
{
    Operand op = newOperand();
    op->kind = VARIABLE;
    strcpy(op->name, name);
    return op;
}

/**
 * @brief Creates a new label operand.
 *
 * This function creates a new operand of type LABEL and assigns a unique label number to it.
 * @return Operand A newly created label operand.
 */
Operand newLabel()
{
    Operand op = newOperand();
    op->kind = LABEL;
    op->no = labelNum++;
    return op;
}

/**
 * @brief Creates a new constant operand.
 *
 * This function creates a new operand of type CONSTANT and assigns the given value to it.
 * @param val The value of the constant.
 * @return Operand A newly created constant operand.
 */
Operand newConst(int val)
{
    Operand op = newOperand();
    op->kind = CONSTANT;
    op->value = val;
    return op;
}

/**
 * @brief Creates a NULL operation for intercode.
 *
 * This function creates a NULL intercode, which is used as a placeholder.
 * @return InterCode A newly created NULL intercode.
 */
InterCode NULLCODE()
{
    InterCode nullcode = (InterCode)malloc(sizeof(InterCode_t));
    assert(nullcode != NULL && "Failed to allocate memory for InterCode");

    nullcode->kind = NULL_;
    nullcode->next = NULL;
    nullcode->prev = NULL;
    return nullcode;
}

/**
 * @brief Computes the size of a given type.
 *
 * This function recursively calculates the size of a type, taking into account array and structure types.
 * @param type The type whose size is to be computed.
 * @return int The size of the type in bytes.
 */
int getSizeof(Type type)
{
    if(type == NULL){
        return 4;
    }
    switch (type->kind)
    {
    case ARRAY:
    {
        int total = 1;
        for (int i = 0; i < type->array.dimension; ++i)
        {
            total *= type->array.size[i];
        }
        return total * getSizeof(type->array.elem);
    }

    case STRUCTURE:
    case STRUCT_DEF:
    {
        int total = 0;
        for (FieldList field = type->structure->members; field != NULL; field = field->next)
        {
            total += getSizeof(field->type);
        }
        return total;
    }

    default:
        return 4; // Default size for basic types like int
    }
}

/**
 * @brief Retrieves the operand corresponding to a variable name.
 *
 * This function searches for a variable in the symbol table, and returns its associated operand.
 * If the operand does not exist, it creates a new variable operand and returns it.
 * @param name The name of the variable.
 * @return Operand The operand associated with the variable.
 */
Operand getOperand(char *name)
{
    assert(name && "Variable name must not be NULL");

    Symbol sym = search_symbol_all(name);
    assert(sym && "Symbol not found in symbol table");

    if (sym->op)
    {
        // Operand already exists (e.g., for function parameters)
        return sym->op;
    }

    // Create new operand and associate with the symbol
    sym->op = newVar(name);
    sym->op->type = sym->type;

    return sym->op;
}

/**
 * @brief Copies the fields of one operand to another.
 *
 * This function performs a deep copy of the operand fields.
 * @param dst The destination operand.
 * @param src The source operand.
 */
void copyOperand(Operand dst, Operand src)
{
    assert(dst && "Destination operand must not be NULL");
    assert(src && "Source operand must not be NULL");

    dst->kind = src->kind;
    dst->type = src->type;
    dst->is_addr = src->is_addr;

    switch (src->kind)
    {
    case TEMP_VAR:
    case LABEL:
        dst->no = src->no;
        break;

    case CONSTANT:
        dst->value = src->value;
        break;

    case FUNC:
    case VARIABLE:
        strcpy(dst->name, src->name); // Assumes name has enough space
        break;

    default:
        dst->opr = src->opr;
        break;
    }
}

/**
 * @brief Handle operands that require address, create address operands for struct or array
 * @param op Original operand
 * @return Processed operand (could be original operand or newly created address operand)
 */
Operand handleAddressOperand(Operand op)
{
    assert(op && "Operand must not be NULL");

    // Return original operand if address is already handled or not required
    if (op->is_addr || !op->type ||
        (op->type->kind != STRUCTURE && op->type->kind != ARRAY))
    {
        return op;
    }

    Operand addr_op = newOperand();
    addr_op->is_addr = true;
    addr_op->type = op->type;

    if (op->kind == GET_VAL)
    {
        // Use the inner operand if it's already a value-get operation
        copyOperand(addr_op, op->opr);
    }
    else
    {
        addr_op->kind = GET_ADDR;
        addr_op->opr = op;
    }

    return addr_op;
}

/**
 * @brief Inserts one intercode after another.
 *
 * This function appends the source intercode after the destination intercode.
 * @param dst The destination intercode.
 * @param src The source intercode.
 */
void insertCode(InterCode dst, InterCode src)
{
    assert(dst != NULL && "Destination InterCode cannot be NULL");
    assert(src != NULL && "Source InterCode cannot be NULL");

    InterCode ptr = dst;
    while (ptr->next)
    {
        ptr = ptr->next;
    }
    ptr->next = src;
    src->prev = ptr;
}

/**
 * @brief Inserts one intercode after another, but direct to tail
 *
 * This function appends the source intercode after the destination intercode.
 * @param dst The destination intercode.
 * @param src The source intercode.
 * @param tail The tail of destination intercode.
 */
void insertCodeWithTail(InterCode dst, InterCode src, InterCode tail)
{
    assert(dst && "Destination InterCode cannot be NULL");
    assert(src && "Source InterCode cannot be NULL");

    InterCode insert_pos = tail ? tail : dst;

    // Traverse to the end if no tail is provided
    while (insert_pos->next)
    {
        insert_pos = insert_pos->next;
    }

    insert_pos->next = src;
    src->prev = insert_pos;
}

/**
 * @brief Initializes the read and write functions in the symbol table.
 */
void initializeReadWriteFunctions()
{
    Symbol read_sym = search_symbol_all("read");
    Symbol write_sym = search_symbol_all("write");

    Operand read_op = newOperand();
    Operand write_op = newOperand();

    read_op->kind = FUNC;
    write_op->kind = FUNC;
    read_op->type = read_sym->type;
    write_op->type = write_sym->type;

    strcpy(read_op->name, "read");
    strcpy(write_op->name, "write");

    read_sym->op = read_op;
    write_sym->op = write_op;
}

/**
 * @brief Generates intermediate code for a program.
 *
 * This function generates the intermediate code for the given program's abstract syntax tree (AST).
 * It initializes the symbol table and sets up the read and write functions.
 * @param program The root node of the abstract syntax tree.
 * @return InterCode The generated intermediate code.
 */
InterCode generate_IR(Node *program)
{
    // Initialize read & write functions
    initializeReadWriteFunctions();

    // Generate intermediate code for the program
    InterCode code = translate_ExtDefList(program->first_child);
    formatCode(code);
    return code;
}

/**
 * @brief Translates the external definition list into intermediate code.
 *
 * This function traverses the list of external definitions and generates corresponding intermediate code.
 * @param node The node representing the external definition list.
 * @return InterCode The generated intermediate code for the external definition list.
 */
InterCode translate_ExtDefList(Node *node)
{
    assert(node && "AST Node cannot be NULL");

    // Base case: if the node is of type NULL, return NULLCODE
    if (node->type == NODE_TYPE_NULL)
    {
        return NULLCODE();
    }

    // Generate intermediate code for the first external definition
    InterCode code1 = translate_ExtDef(node->first_child);

    // Generate intermediate code for the rest of the external definitions
    InterCode code2 = translate_ExtDefList(node->first_child->next_sibling);

    insertCode(code1, code2);

    return code1;
}

/**
 * @brief Translates an external definition into intermediate code.
 *
 * This function handles both function declarations and definitions. It generates intermediate code
 * for a function definition, including function parameters and body. The function adds the function
 * to the symbol table and generates the corresponding intermediate code for the function.
 *
 * @param node The node representing the external definition.
 *
 * @return The intermediate code generated from the external definition.
 */
InterCode translate_ExtDef(Node *node)
{
    Type type = Specifier(node->first_child);
    if (strcmp(node->first_child->next_sibling->name, "FunDec") != 0)
    {
        // Handle global variable declarations (not considered in this context).
        return NULLCODE();
    }
    // Handle function definition
    Function func = FunDec(node->first_child->next_sibling);
    func->ret_type = type;
    func->defined = true;

    // Add function to the symbol table
    Type ftype = (Type)malloc(sizeof(Type_t));
    ftype->kind = FUNCTION;
    ftype->func = func;
    Symbol sym = create_symbol(func->name, ftype, 0);

    // Translate function name
    Operand place = newOperand();
    place->kind = FUNC;
    strcpy(place->name, sym->name);
    place->type = sym->type;
    InterCode code1 = translate_Function(func, place);
    sym->op = place;
    insert_symbol(sym);

    InterCode code2 = translate_CompSt(node->first_child->next_sibling->next_sibling);
    insertCode(code1, code2);
    return code1;
}

/**
 * @brief Translates a function into intermediate code.
 *
 * This function generates intermediate code for the function, including its declaration and
 * parameter handling. It translates function parameters and generates the corresponding code.
 *
 * @param func The function to be translated.
 * @param place The operand representing the function.
 *
 * @return The intermediate code for the function.
 */
InterCode translate_Function(Function func, Operand place)
{
    // Generate code for function declaration
    InterCode code1 = NULLCODE();
    code1->kind = FUNC_;
    code1->ops[0] = place;

    // Translate function parameters, if any
    if (func->param != NULL)
    {
        InterCode code2 = translate_ParamList(func->param);
        insertCode(code1, code2);
    }

    return code1;
}

/**
 * @brief Translates a list of function parameters into intermediate code.
 *
 * This function processes a list of parameters, generates intermediate code for each parameter,
 * and adds them to the symbol table.
 *
 * @param param The list of parameters to be translated.
 *
 * @return The intermediate code generated for the parameters.
 */
InterCode translate_ParamList(FieldList param)
{
    // Initialize operand for the parameter
    Operand var = newVar(param->name);
    var->type = param->type;

    // Set address flag for structures or arrays
    if (var->type->kind == STRUCTURE || var->type->kind == ARRAY)
    {
        var->is_addr = true;
    }

    // Add parameter to symbol table
    Symbol sym = create_symbol(param->name, param->type, 0);
    sym->op = var;
    insert_symbol(sym);

    // Generate intermediate code for the parameter
    InterCode code1 = NULLCODE();
    code1->kind = PARAM_;
    code1->ops[0] = var;

    // Handle next parameter recursively
    if (param->next != NULL)
    {
        InterCode code2 = translate_ParamList(param->next);
        insertCode(code1, code2);
    }

    return code1;
}

/**
 * @brief Translates a compound statement into intermediate code.
 *
 * This function processes a compound statement, generating intermediate code for the variable
 * declarations and statements within the compound statement.
 *
 * @param node The node representing the compound statement.
 *
 * @return The intermediate code generated for the compound statement.
 */
InterCode translate_CompSt(Node *node)
{
    InterCode code1 = translate_DefList(node->first_child->next_sibling);
    InterCode code2 = translate_StmtList(node->first_child->next_sibling->next_sibling);
    insertCode(code1, code2);
    return code1;
}

/**
 * @brief Translates a list of variable definitions into intermediate code.
 *
 * This function processes a list of variable definitions, generating corresponding intermediate code
 * for each definition.
 *
 * @param node The node representing the list of variable definitions.
 *
 * @return The intermediate code generated for the variable definitions.
 */
InterCode translate_DefList(Node *node)
{
    if (node->type == NODE_TYPE_NULL)
    {
        return NULLCODE();
    }
    InterCode code1 = translate_Def(node->first_child);
    InterCode code2 = translate_DefList(node->first_child->next_sibling);
    insertCode(code1, code2);
    return code1;
}

/**
 * @brief Translates a single variable definition into intermediate code.
 *
 * This function handles the translation of a single variable definition into intermediate code,
 * including the handling of arrays and structures.
 *
 * @param node The node representing the variable definition.
 *
 * @return The intermediate code generated for the variable definition.
 */
InterCode translate_Def(Node *node)
{
    Type type = Specifier(node->first_child);
    return translate_DecList(node->first_child->next_sibling, type);
}

/**
 * @brief Translates a list of variable declarations into intermediate code.
 *
 * This function processes a list of variable declarations, generating the corresponding intermediate
 * code for each variable.
 *
 * @param node The node representing the list of variable declarations.
 * @param type The type of the variables to be declared.
 *
 * @return The intermediate code generated for the variable declarations.
 */
InterCode translate_DecList(Node *node, Type type)
{
    InterCode code1 = translate_Dec(node->first_child, type);

    // If there are more declarations, generate and combine their code
    if (node->child_num == 3)
    {
        InterCode code2 = translate_DecList(node->first_child->next_sibling->next_sibling, type);
        insertCode(code1, code2);
    }

    return code1;
}

/**
 * @brief Translates a variable declaration into intermediate code.
 *
 * This function processes a variable declaration and generates the corresponding intermediate code
 * for the variable, including handling array and structure types.
 *
 * @param node The node representing the variable declaration.
 * @param type The type of the variable to be declared.
 *
 * @return The intermediate code generated for the variable declaration.
 */
InterCode translate_Dec(Node *node, Type type)
{
    FieldList variable = translate_VarDec(node->first_child, type, NULL, 0);

    // Update symbol table
    Symbol sym = create_symbol(variable->name, variable->type, 0);
    insert_symbol(sym);

    InterCode code1 = NULLCODE();

    // Generate code for array/structure declaration
    if (sym->type->kind == ARRAY || sym->type->kind == STRUCTURE)
    {
        Operand var = newVar(sym->name);
        var->type = sym->type;
        sym->op = var;

        code1->kind = DEC_;
        code1->ops[0] = var;
        code1->size = getSizeof(var->type);
    }

    // Handle assignment (VarDec = Exp)
    if (node->child_num == 3)
    {
        Operand var = getOperand(sym->name);
        sym->op = var;
        InterCode code2 = NULLCODE();
        code2->kind = ASSIGN_;
        code2->ops[0] = var;

        Operand place = newTemp();
        InterCode code3 = translate_Exp(node->first_child->next_sibling->next_sibling, place);
        code2->ops[1] = place;

        insertCode(code1, code3);
        insertCode(code1, code2);
    }

    return code1;
}

/**
 * @brief Translates a variable declaration node into a field list.
 *
 * This function handles both simple variable declarations and array declarations.
 * It recursively processes the array dimensions if necessary.
 *
 * @param node The node representing the variable declaration.
 * @param type The base type of the variable (used for non-array variables).
 * @param arr_type The type for arrays, including dimensions and sizes.
 * @param layer The current depth of array dimension. Used for recursive handling.
 *
 * @return A FieldList containing the translated variable declaration.
 */
FieldList translate_VarDec(Node *node, Type type, Type arr_type, int layer)
{
    FieldList variable;

    // Handle simple variable declaration (ID)
    if (node->child_num == 1)
    {
        variable = (FieldList)malloc(sizeof(FieldList_t));
        strcpy(variable->name, node->first_child->value.str);
        variable->type = (layer == 0) ? type : arr_type;
        return variable;
    }

    // Handle array declaration
    if (layer == 0)
    {
        // Initialize array type for the first dimension
        arr_type = (Type)malloc(sizeof(Type_t));
        arr_type->kind = ARRAY;
        arr_type->array.elem = type;
        arr_type->array.dimension = 1;
        arr_type->array.size[layer] = node->first_child->next_sibling->next_sibling->value.ival;
    }
    else
    {
        // Update array dimension and size recursively
        arr_type->array.dimension++;
        arr_type->array.size[layer] = node->first_child->next_sibling->next_sibling->value.ival;
    }

    return translate_VarDec(node->first_child, type, arr_type, layer + 1);
}

/**
 * @brief Translates a list of statements into intermediate code.
 *
 * This function processes a list of statements and generates the corresponding intermediate code.
 * The intermediate codes are linked together in sequence.
 *
 * @param node The node representing the statement list.
 *
 * @return The intermediate code for the translated statement list.
 */
InterCode translate_StmtList(Node *node)
{
    if (node->type == NODE_TYPE_NULL)
    {
        return NULLCODE();
    }

    InterCode head = translate_Stmt(node->first_child);
    InterCode tail = translate_StmtList(node->first_child->next_sibling);
    insertCode(head, tail);

    return head;
}

/**
 * @brief Translates a single statement into intermediate code.
 *
 * This function handles various types of statements such as expressions, compound statements, return, while loops,
 * and if-else statements, generating corresponding intermediate code.
 *
 * @param node The node representing the statement.
 *
 * @return The intermediate code representing the translated statement.
 */
InterCode translate_Stmt(Node *node)
{
    if (strcmp(node->first_child->name, "Exp") == 0)
    {
        Operand place = newTemp();
        // Direct return of the translation for expressions
        return translate_Exp(node->first_child, place);
    }
    else if (strcmp(node->first_child->name, "CompSt") == 0)
    {
        return translate_CompSt(node->first_child);
    }
    else if (strcmp(node->first_child->name, "RETURN") == 0)
    {
        // Handling return statement
        Operand place = newTemp();
        InterCode code1 = translate_Exp(node->first_child->next_sibling, place);
        InterCode code2 = NULLCODE();
        code2->kind = RETURN_;
        code2->ops[0] = place;
        insertCode(code1, code2);
        return code1;
    }
    else if (strcmp(node->first_child->name, "WHILE") == 0)
    {
        // Handling while loop
        Operand label1 = newLabel(), label2 = newLabel(), label3 = newLabel();
        InterCode code1 = NULLCODE();
        code1->kind = LABEL_;
        code1->ops[0] = label1;
        InterCode code2 = translate_Condition(node->first_child->next_sibling->next_sibling, label2, label3);
        InterCode code3 = NULLCODE();
        code3->kind = LABEL_;
        code3->ops[0] = label2;
        InterCode code4 = translate_Stmt(node->first_child->next_sibling->next_sibling->next_sibling->next_sibling);
        InterCode code5 = NULLCODE();
        code5->kind = GOTO_;
        code5->ops[0] = label1;
        InterCode code6 = NULLCODE();
        code6->kind = LABEL_;
        code6->ops[0] = label3;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        insertCode(code1, code5);
        insertCode(code1, code6);
        return code1;
    }
    else if (node->child_num == 5)
    {
        // Handling if statement
        Operand label1 = newLabel(), label2 = newLabel();
        InterCode code1 = translate_Condition(node->first_child->next_sibling->next_sibling, label1, label2);
        InterCode code2 = NULLCODE();
        code2->kind = LABEL_;
        code2->ops[0] = label1;
        InterCode code3 = translate_Stmt(node->first_child->next_sibling->next_sibling->next_sibling->next_sibling);
        InterCode code4 = NULLCODE();
        code4->kind = LABEL_;
        code4->ops[0] = label2;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        return code1;
    }
    else if (node->child_num == 7)
    {
        // Handling if-else statement
        Operand label1 = newLabel(), label2 = newLabel(), label3 = newLabel();
        InterCode code1 = translate_Condition(node->first_child->next_sibling->next_sibling, label1, label2);
        InterCode code2 = NULLCODE();
        code2->kind = LABEL_;
        code2->ops[0] = label1;
        InterCode code3 = translate_Stmt(node->first_child->next_sibling->next_sibling->next_sibling->next_sibling);
        InterCode code4 = NULLCODE();
        code4->kind = GOTO_;
        code4->ops[0] = label3;
        InterCode code5 = NULLCODE();
        code5->kind = LABEL_;
        code5->ops[0] = label2;
        InterCode code6 = translate_Stmt(node->first_child->next_sibling->next_sibling->next_sibling->next_sibling->next_sibling->next_sibling);
        InterCode code7 = NULLCODE();
        code7->kind = LABEL_;
        code7->ops[0] = label3;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        insertCode(code1, code5);
        insertCode(code1, code6);
        insertCode(code1, code7);
        return code1;
    }
    return NULLCODE();
}

/**
 * @brief Translates a condition expression into intermediate code.
 *
 * This function generates intermediate code for conditional expressions such as relational operators and logical
 * operators (AND, OR). It handles the generation of `IF_GOTO_IR` and `GOTO_IR` codes.
 *
 * @param exp The expression representing the condition.
 * @param labeltrue The label to jump to if the condition is true.
 * @param labelfalse The label to jump to if the condition is false.
 *
 * @return The intermediate code representing the condition translation.
 */
InterCode translate_Condition(Node *exp, Operand labeltrue, Operand labelfalse)
{
    // Handle the NOT condition by inverting the true and false labels
    if (strcmp(exp->first_child->name, "NOT") == 0)
    {
        return translate_Condition(exp->first_child->next_sibling, labelfalse, labeltrue);
    }
    // Handling relational operations
    else if (exp->child_num > 2 && strcmp(exp->first_child->next_sibling->name, "RELOP") == 0)
    {
        Operand exp1 = newTemp();
        Operand exp2 = newTemp();
        InterCode code1 = translate_Exp(exp->first_child, exp1);
        InterCode code2 = translate_Exp(exp->first_child->next_sibling->next_sibling, exp2);

        // Generate the IF_GOTO and GOTO intermediate codes
        InterCode code3 = NULLCODE();
        code3->kind = IF_GOTO_;
        code3->ops[0] = exp1;
        code3->ops[1] = exp2;
        code3->ops[2] = labeltrue;
        strcpy(code3->relop, exp->first_child->next_sibling->value.str);

        InterCode code4 = NULLCODE();
        code4->kind = GOTO_;
        code4->ops[0] = labelfalse;

        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        return code1;
    }
    // Handle logical AND/OR operations
    else if (exp->child_num > 2 &&
             (strcmp(exp->first_child->next_sibling->name, "AND") == 0 ||
              strcmp(exp->first_child->next_sibling->name, "OR") == 0))
    {
        Operand tmp = newLabel();
        InterCode code1;
        // Depending on the operator, decide the label flow for AND/OR
        if (exp->first_child->next_sibling->name[0] == 'A')
        {
            code1 = translate_Condition(exp->first_child, tmp, labelfalse);
        }
        else
        {
            code1 = translate_Condition(exp->first_child, labeltrue, tmp);
        }

        // Create the label and continue the condition translation
        InterCode code2 = NULLCODE();
        code2->kind = LABEL_;
        code2->ops[0] = tmp;

        InterCode code3 = translate_Condition(exp->first_child->next_sibling->next_sibling, labeltrue, labelfalse);
        insertCode(code1, code2);
        insertCode(code1, code3);
        return code1;
    }
    else
    {
        // Single value as expression
        Operand tmp = newTemp();
        InterCode code1 = translate_Exp(exp, tmp);

        InterCode code2 = NULLCODE();
        code2->kind = IF_GOTO_;
        code2->ops[0] = tmp;
        code2->ops[1] = newConst(0); // Compare with 0 for boolean evaluation
        code2->ops[2] = labeltrue;
        strcpy(code2->relop, "!=");

        InterCode code3 = NULLCODE();
        code3->kind = GOTO_;
        code3->ops[0] = labelfalse;

        insertCode(code1, code2);
        insertCode(code1, code3);
        return code1;
    }
}

/**
 * @brief Translates an expression node into intermediate code.
 *
 * This function processes various types of expressions recursively and generates corresponding
 * intermediate code for each case. It handles assignments, arithmetic operations, logical operations,
 * function calls, array accesses, and struct field accesses.
 *
 * @param node The expression node to translate.
 * @param place The operand that will hold the result of the translation.
 * @return The intermediate code generated for the expression.
 */
InterCode translate_Exp(Node *node, Operand place)
{
    assert(node != NULL && "AST Node cannot be NULL");
    assert(place != NULL && "Operand 'place' cannot be NULL");

    // Handle assignment expressions: Exp ASSIGN Exp
    if (node->child_num == 3 && strcmp(node->first_child->next_sibling->name, "ASSIGNOP") == 0)
    {
        Operand left = newTemp();
        Operand right = newTemp();
        InterCode code1 = translate_Exp(node->first_child, left);
        InterCode code2 = translate_Exp(node->first_child->next_sibling->next_sibling, right);
        InterCode code3 = NULLCODE();
        code3->kind = ASSIGN_;
        code3->ops[0] = left;
        code3->ops[1] = right;

        // Handle array-to-array assignments
        if (right->type != NULL && right->type->kind == ARRAY && !right->is_addr)
        {
            code3->ops[1] = handleAddressOperand(right);
            if (left->type != NULL && left->type->kind == ARRAY && !left->is_addr && node->first_child->child_num == 1)
            {
                // Left value is an array, mark it as address
                Symbol sym = search_symbol_all(node->first_child->first_child->value.str);
                sym->op->is_addr = true;
            }
        }

        InterCode code4 = NULLCODE();
        code4->kind = ASSIGN_;
        code4->ops[0] = place;
        code4->ops[1] = left;

        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        return code1;
    }
    // Handle logical and relational operations: Exp && || ><= Exp
    else if (node->child_num >= 2 && (strcmp(node->first_child->name, "NOT") == 0 ||
                                      strcmp(node->first_child->next_sibling->name, "AND") == 0 ||
                                      strcmp(node->first_child->next_sibling->name, "OR") == 0 ||
                                      strcmp(node->first_child->next_sibling->name, "RELOP") == 0))
    {
        InterCode code1 = NULLCODE();
        code1->kind = ASSIGN_;
        code1->ops[0] = place;
        code1->ops[1] = newConst(0);
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        InterCode code2 = translate_Condition(node, label1, label2);
        InterCode code3 = NULLCODE();
        code3->kind = LABEL_;
        code3->ops[0] = label1;
        InterCode code4 = NULLCODE();
        code4->kind = ASSIGN_;
        code4->ops[0] = place;
        code4->ops[1] = newConst(1);
        InterCode code5 = NULLCODE();
        code5->kind = LABEL_;
        code5->ops[0] = label2;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        insertCode(code1, code5);
        return code1;
    }
    // Handle arithmetic operations: Exp +-*/ Exp
    else if (node->child_num == 3 && (strcmp(node->first_child->next_sibling->name, "PLUS") == 0 ||
                                      strcmp(node->first_child->next_sibling->name, "MINUS") == 0 ||
                                      strcmp(node->first_child->next_sibling->name, "STAR") == 0 ||
                                      strcmp(node->first_child->next_sibling->name, "DIV") == 0))
    {
        Operand tmp1 = newTemp();
        Operand tmp2 = newTemp();
        InterCode code1 = translate_Exp(node->first_child, tmp1);
        InterCode code2 = translate_Exp(node->first_child->next_sibling->next_sibling, tmp2);
        InterCode code3 = NULL;
        switch (node->first_child->next_sibling->name[0])
        {
        case 'P':
            code3 = arithmetic(place, tmp1, tmp2, PLUS_OP);
            break;
        case 'M':
            code3 = arithmetic(place, tmp1, tmp2, MINUS_OP);
            break;
        case 'S':
            code3 = arithmetic(place, tmp1, tmp2, STAR_OP);
            break;
        case 'D':
            code3 = arithmetic(place, tmp1, tmp2, DIV_OP);
            break;
        }
        insertCode(code1, code2);
        insertCode(code1, code3);
        return code1;
    }
    // Handle parentheses around expressions: (Exp)
    else if (strcmp(node->first_child->name, "LP") == 0)
    {
        return translate_Exp(node->first_child->next_sibling, place);
    }
    // Handle unary negation: - Exp
    else if (strcmp(node->first_child->name, "MINUS") == 0)
    {
        Operand tmp = newTemp();
        InterCode code1 = translate_Exp(node->first_child->next_sibling, tmp);
        InterCode code2 = arithmetic(place, newConst(0), tmp, MINUS_OP);
        insertCode(code1, code2);
        return code1;
    }
    // Handle function calls: ID() or ID(ARGS)
    else if (node->child_num > 1 && strcmp(node->first_child->next_sibling->name, "LP") == 0)
    {
        Symbol sym = search_symbol_all(node->first_child->value.str);
        Operand func = sym->op;

        // Handle special read/write functions
        if (strcmp(func->name, "read") == 0)
        {
            InterCode read_code = NULLCODE();
            read_code->kind = READ_;
            read_code->ops[0] = place;
            return read_code;
        }
        else if (strcmp(func->name, "write") == 0)
        {
            Operand tmp = newTemp();
            InterCode code1 = translate_Exp(node->first_child->next_sibling->next_sibling->first_child, tmp);
            InterCode code2 = NULLCODE();
            code2->kind = WRITE_;
            code2->ops[0] = tmp;
            insertCode(code1, code2);
            return code1;
        }

        // General function call
        InterCode call_code = NULLCODE();
        call_code->kind = CALL_;
        call_code->ops[0] = place;
        call_code->ops[1] = func;
        if (node->child_num == 4)
        {
            InterCode arg_code = translate_Args(node->first_child->next_sibling->next_sibling);
            insertCode(arg_code, call_code);
            return arg_code;
        }
        return call_code;
    }
    // Handle array element access: Exp[Exp]
    else if (node->child_num > 1 && strcmp(node->first_child->next_sibling->name, "LB") == 0)
    {
        Operand tmp = newTemp();
        Operand arr_addr = NULL;
        Operand addr = NULL;
        Operand index = newTemp();
        Operand offset = newTemp();

        // Get array base address
        InterCode code1 = translate_Exp(node->first_child, tmp);
        if (tmp->kind == GET_VAL)
        {
            arr_addr = tmp->opr;
            arr_addr->is_addr = true;
        }
        else if (!tmp->is_addr)
        {
            arr_addr = newOperand();
            arr_addr->kind = GET_ADDR;
            arr_addr->opr = tmp;
            arr_addr->is_addr = true;
        }
        else
        {
            arr_addr = tmp;
        }

        // Handle multidimensional arrays
        if (tmp->type->array.dimension > 1)
        {
            Type low_arr = (Type)malloc(sizeof(Type_t));
            low_arr->kind = ARRAY;
            low_arr->array.dimension = tmp->type->array.dimension - 1;
            low_arr->array.elem = tmp->type->array.elem;
            for (int i = 0; i < low_arr->array.dimension; i++)
            {
                low_arr->array.size[i] = tmp->type->array.size[i];
            }
            arr_addr->type = low_arr;
        }
        else
        {
            arr_addr->type = tmp->type;
        }

        // Calculate the offset
        InterCode code2 = translate_Exp(node->first_child->next_sibling->next_sibling, index);
        insertCode(code1, code2);
        if (index->value == 0)
        {
            addr = arr_addr;
        }
        else
        {
            addr = newTemp();
            addr->is_addr = true;
            int move = 1;
            for (int i = 0; i < arr_addr->type->array.dimension - 1; i++)
            {
                move = move * arr_addr->type->array.size[i];
            }
            InterCode code3 = arithmetic(offset, index, newConst(move * getSizeof(arr_addr->type->array.elem)), STAR_OP);
            InterCode code4 = arithmetic(addr, arr_addr, offset, PLUS_OP);
            insertCode(code1, code3);
            insertCode(code1, code4);
        }

        if (addr->kind == GET_ADDR)
        {
            Operand tmp = newTemp();
            InterCode code5 = NULLCODE();
            code5->kind = ASSIGN_;
            code5->ops[0] = tmp;
            code5->ops[1] = addr;
            insertCode(code1, code5);
            place->kind = GET_VAL;
            place->opr = tmp;
            place->type = arr_addr->type->array.elem;
        }
        else
        {
            place->kind = GET_VAL;
            place->opr = addr;
            place->type = arr_addr->type->array.elem;
        }
        return code1;
    }
    // Handle struct field access: Exp.ID
    else if (node->child_num > 1 && strcmp(node->first_child->next_sibling->name, "DOT") == 0)
    {
        Operand tmp = newTemp();
        Operand exp_addr = NULL;
        Operand addr = NULL;
        Operand offset = newConst(0);

        InterCode code1 = translate_Exp(node->first_child, tmp);
        if (tmp->kind == GET_VAL)
        {
            exp_addr = tmp->opr;
            exp_addr->type = tmp->type;
            exp_addr->is_addr = true;
        }
        else if (!tmp->is_addr)
        {
            exp_addr = newOperand();
            exp_addr->kind = GET_ADDR;
            exp_addr->opr = tmp;
            exp_addr->type = tmp->type;
            exp_addr->is_addr = true;
        }
        else
        {
            exp_addr = tmp;
        }

        FieldList ptr = exp_addr->type->structure->members;
        char *id = node->first_child->next_sibling->next_sibling->value.str;
        Type ret_type = NULL;
        while (ptr != NULL)
        {
            if (strcmp(ptr->name, id) != 0)
            {
                offset->value += getSizeof(ptr->type);
                ptr = ptr->next;
            }
            else
            {
                ret_type = ptr->type;
                break;
            }
        }

        if (offset->value != 0)
        {
            addr = newTemp();
            addr->is_addr = true;
            InterCode code2 = arithmetic(addr, exp_addr, offset, PLUS_OP);
            insertCode(code1, code2);
        }
        else
        {
            addr = exp_addr;
        }

        if (addr->kind == GET_ADDR)
        {
            Operand tmp = newTemp();
            InterCode code3 = NULLCODE();
            code3->kind = ASSIGN_;
            code3->ops[0] = tmp;
            code3->ops[1] = addr;
            insertCode(code1, code3);
            place->kind = GET_VAL;
            place->opr = tmp;
            place->type = ret_type;
        }
        else
        {
            place->kind = GET_VAL;
            place->opr = addr;
            place->type = ret_type;
        }
        place->is_addr = false;
        return code1;
    }
    // Handle variable access: ID
    else if (node->child_num == 1 && strcmp(node->first_child->name, "ID") == 0)
    {
        copyOperand(place, getOperand(node->first_child->value.str));
        return NULLCODE();
    }
    // Handle literals: INT or FLOAT
    else if (strcmp(node->first_child->name, "INT") == 0 ||
             strcmp(node->first_child->name, "FLOAT") == 0)
    {
        copyOperand(place, newConst(node->first_child->value.ival));
        return NULLCODE();
    }
    return NULLCODE();
}

/**
 * @brief Translates arguments for a function call.
 * @param node The node representing the argument list.
 * @return The intermediate code generated for the arguments.
 */
InterCode translate_Args(Node *node)
{
    // Translate the first argument expression
    Operand argOp = newTemp();
    InterCode code = translate_Exp(node->first_child, argOp);

    // Recursively translate remaining arguments if they exist
    if (node->child_num == 3)
    {
        Node *restArgs = node->first_child->next_sibling->next_sibling;
        InterCode restCode = translate_Args(restArgs);
        insertCode(code, restCode);
    }

    // Create ARG instruction for current argument
    InterCode argCode = NULLCODE();
    argCode->kind = ARG_;
    argCode->ops[0] = handleAddressOperand(argOp);
    insertCode(code, argCode);

    return code;
}

/**
 * @brief Generates code to assign the value of an address operand to a temporary variable.
 * @param address The address-like operand.
 * @param target The temporary operand to receive the value.
 * @return The intermediate code representing target := address.
 */
InterCode transform_addr(Operand address, Operand target)
{
    InterCode code = NULLCODE();
    code->kind = ASSIGN_;
    code->ops[0] = target;
    code->ops[1] = address;
    return code;
}

/**
 * @brief Determines whether the operand involves a memory-related operation.
 * @param op The operand to inspect.
 * @return True if it is an address-fetch or value-dereference operand.
 */
bool is_memop(Operand op)
{
    switch (op->kind)
    {
    case GET_ADDR:
    case GET_VAL:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Generates intermediate code for arithmetic operations.
 * @param dst The destination operand.
 * @param src1 The first source operand.
 * @param src2 The second source operand.
 * @param op The arithmetic operation.
 * @return The head of the intermediate code chain.
 */
InterCode arithmetic(Operand dst, Operand src1, Operand src2, enum Operator op)
{
    assert(dst && src1 && src2);

    InterCode code = NULLCODE();
    switch (op)
    {
    case PLUS_OP:
        code->kind = PLUS_;
        break;
    case MINUS_OP:
        code->kind = SUB_;
        break;
    case STAR_OP:
        code->kind = MULTI_;
        break;
    case DIV_OP:
        code->kind = DIV_;
        break;
    default:
        assert(0 && "Unsupported arithmetic operator");
    }

    InterCode head = NULL;
    InterCode tail = NULL;

    Operand actual1 = src1;
    if (is_memop(src1))
    {
        actual1 = newTemp();
        InterCode trans1 = transform_addr(src1, actual1);
        head = tail = trans1;
    }

    Operand actual2 = src2;
    if (is_memop(src2))
    {
        actual2 = newTemp();
        InterCode trans2 = transform_addr(src2, actual2);
        if (head)
            insertCode(tail, trans2), tail = trans2;
        else
            head = tail = trans2;
    }

    code->ops[0] = dst;
    code->ops[1] = actual1;
    code->ops[2] = actual2;

    if (head)
        insertCode(tail, code);
    else
        head = code;

    return head;
}

/**
 * @brief Formats and eliminates references and dereferencing in the code.
 * @param code The intermediate code to format.
 */
void formatCode(InterCode code)
{
    assert(code != NULL && "InterCode list cannot be NULL");

    InterCode curr = code, prev = NULL;
    while (curr != NULL)
    {
        switch (curr->kind)
        {
        case ASSIGN_:
            // Check if both operands are memory operations
            if (is_memop(curr->ops[0]) && is_memop(curr->ops[1]))
            {
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[1], tmp);
                curr->ops[1] = tmp;
                mid->next = curr;
                mid->prev = prev;
                if (prev)
                {
                    prev->next = mid;
                }
                curr->prev = mid;
                prev = mid;
            }
            break;

        case IF_GOTO_:
            // Handle both operands for memory operations
            if (is_memop(curr->ops[0]))
            {
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[0], tmp);
                curr->ops[0] = tmp;
                mid->next = curr;
                mid->prev = prev;
                if (prev)
                {
                    prev->next = mid;
                }
                curr->prev = mid;
                prev = mid;
            }
            if (is_memop(curr->ops[1]))
            {
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[1], tmp);
                curr->ops[1] = tmp;
                mid->next = curr;
                mid->prev = prev;
                if (prev)
                {
                    prev->next = mid;
                }
                curr->prev = mid;
                prev = mid;
            }
            break;

        case RETURN_:
            // Handle memory operation for return
            if (is_memop(curr->ops[0]))
            {
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[0], tmp);
                curr->ops[0] = tmp;
                mid->next = curr;
                mid->prev = prev;
                if (prev)
                {
                    prev->next = mid;
                }
                curr->prev = mid;
                prev = mid;
            }
            break;

        case READ_: // Fixed the syntax issue here
        case WRITE_:
            if (is_memop(curr->ops[0]) || curr->ops[0]->kind == CONSTANT)
            {
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[0], tmp);
                curr->ops[0] = tmp;
                mid->next = curr;
                mid->prev = prev;
                if (prev)
                {
                    prev->next = mid;
                }
                curr->prev = mid;
                prev = mid;
            }
            break;

        case NULL_:
            // Remove NULL_ node from the list
            if (prev == NULL)
            {
                break; // No operation if prev is NULL (start of list)
            }
            InterCode ptr = curr;
            prev->next = ptr->next;
            if (ptr->next)
            {
                ptr->next->prev = prev;
            }
            curr = prev; // Move curr to prev as the current node is removed
            break;

        default:
            break;
        }

        // Move to the next node in the list
        prev = curr;
        curr = curr->next;
    }
}

/**
 * @brief Appends a new intermediate code instruction to the end of a list.
 * @param headRef Pointer to the head of the list.
 * @param tailRef Pointer to the tail of the list.
 * @param codeToAdd The new code node to add.
 */
void addCodeToList(InterCode *headRef, InterCode *tailRef, InterCode codeToAdd)
{
    if (!codeToAdd)
        return;

    codeToAdd->prev = *tailRef; // Link the new node to the current tail.
    codeToAdd->next = NULL;

    if (*tailRef)
    {
        (*tailRef)->next = codeToAdd; // If list is not empty, update the current tail.
    }
    else
    {
        *headRef = codeToAdd; // If list is empty, the new node is the head.
    }

    *tailRef = codeToAdd; // Update the tail pointer.
}

/**
 * @brief Inserts a code sequence before a specified node in the list.
 * @param codeToInsert The head of the code sequence to insert.
 * @param insertBeforeNode The node before which the sequence should be inserted.
 * @param headRef Pointer to the list head, updated if insertion occurs at the beginning.
 * @return The tail of the inserted sequence.
 */
InterCode insertCodeSequenceBefore(InterCode codeToInsert, InterCode insertBeforeNode, InterCode *headRef)
{
    if (!codeToInsert || !insertBeforeNode)
        return NULL;

    // Find the tail of the sequence to insert
    InterCode tailOfInsert = codeToInsert;
    while (tailOfInsert->next)
    {
        tailOfInsert = tailOfInsert->next;
    }

    // Link sequence start
    if (insertBeforeNode->prev)
    {
        insertBeforeNode->prev->next = codeToInsert;
    }
    else
    {
        *headRef = codeToInsert; // Inserting at the head
    }

    codeToInsert->prev = insertBeforeNode->prev;

    // Link sequence end
    tailOfInsert->next = insertBeforeNode;
    insertBeforeNode->prev = tailOfInsert;

    return tailOfInsert; // Return the tail of the inserted sequence
}

/**
 * @brief Simplifies complex operands (like memory accesses) by introducing temporaries.
 * @param op The operand to potentially simplify.
 * @param current The instruction containing the operand.
 * @param lastSignificantNode Pointer to the last non-temporary-generating node before current.
 * This is where the new temporary assignment will be inserted before.
 * @param headRef Pointer to the list head, potentially updated by insertion.
 * @return The simplified operand (either the original or a new temporary).
 * @details Generates and inserts assignment code (e.g., t1 = *addr) before 'current'
 * if 'op' is complex (based on is_memop or other criteria).
 * Updates '*lastSignificantNode' to the inserted code.
 */
Operand resolveComplexOperand(Operand op, InterCode current, InterCode *lastSignificantNode, InterCode *headRef)
{
    if (!op)
        return NULL;

    bool needs_simplification = is_memop(op) || ((current->kind == READ_ || current->kind == WRITE_) && op->kind == CONSTANT);

    if (needs_simplification)
    {
        Operand temp_operand = newTemp();
        InterCode transformation_code = transform_addr(op, temp_operand); // Generates: temp_operand = op

        assert(transformation_code && "Failed to generate transformation code for complex operand.");

        InterCode insertedTail = insertCodeSequenceBefore(transformation_code, current, headRef);
        return temp_operand; // Use the new temporary variable
    }

    return op;
}

/**
 * @brief Optimizes and restructures the intermediate code list.
 * @param listHead The head of the intermediate code list to process.
 * @return The head of the potentially modified list.
 * @details Iterates through the list, removes NULL_IR instructions,
 * and simplifies complex operands using resolveComplexOperand.
 * This version processes operands directly within the main loop.
 */
InterCode optimizeAndRestructureCode(InterCode listHead)
{
    InterCode current = listHead, previous = NULL, newHead = listHead;

    while (current != NULL)
    {
        InterCode nextNode = current->next; // Save next node pointer

        // Skip NULL_IR nodes
        if (current->kind == NULL_)
        {
            if (previous)
            {
                previous->next = nextNode;
            }
            else
            {
                newHead = nextNode;
            }
            if (nextNode)
            {
                nextNode->prev = previous;
            }
            current = nextNode;
            continue;
        }

        Operand *op_to_simplify[3] = {NULL, NULL, NULL};
        int count = 0;

        // Decide which operands to simplify
        switch (current->kind)
        {
        case ASSIGN_:
            op_to_simplify[count++] = &current->ops[1]; // RHS first
            if (is_memop(current->ops[0]))
                op_to_simplify[count++] = &current->ops[0]; // LHS is memory access
            break;
        case MEM_:
            op_to_simplify[count++] = &current->ops[1]; // Simplify value source
            break;
        case PLUS_:
        case SUB_:
        case MULTI_:
        case DIV_:
            op_to_simplify[count++] = &current->ops[1];
            op_to_simplify[count++] = &current->ops[2];
            break;
        case IF_GOTO_:
            op_to_simplify[count++] = &current->ops[0];
            op_to_simplify[count++] = &current->ops[1];
            break;
        case RETURN_:
        case ARG_:
        case WRITE_:
        case READ_:
            op_to_simplify[count++] = &current->ops[0];
            break;
        default:
            break;
        }

        // Simplify operands
        InterCode lastNodeBeforeCurrent = previous; // Track insertion point
        for (int i = 0; i < count; ++i)
        {
            if (*op_to_simplify[i])
            {
                Operand simplified_op = resolveComplexOperand(*op_to_simplify[i], current, &previous, &newHead);
                *op_to_simplify[i] = simplified_op;
            }
        }

        // Move to the next node
        previous = current;
        current = nextNode;
    }
    return newHead;
}

/**
 * @brief Formats a single operand into a string representation.
 * @param op The operand to format.
 * @param buffer The character buffer to write the string into.
 * @param buffer_size The size of the buffer.
 * @details Uses snprintf for safe string formatting. Handles recursive calls for GET_ADDR/GET_VAL.
 */
void renderOperandToString(Operand operand, char *buffer, size_t buffer_size)
{
    if (operand == NULL)
    {
        snprintf(buffer, buffer_size, "null_op");
        return;
    }

    char nested_buffer[128]; // Adjust size as needed

    switch (operand->kind)
    {
    case VARIABLE:
        snprintf(buffer, buffer_size, "v%s", operand->name);
        break;
    case TEMP_VAR:
        snprintf(buffer, buffer_size, "t%d", operand->no);
        break;
    case CONSTANT:
        snprintf(buffer, buffer_size, "#%d", operand->value);
        break;
    case LABEL:
        snprintf(buffer, buffer_size, "label%d", operand->no);
        break;
    case FUNC:
        snprintf(buffer, buffer_size, "%s", operand->name);
        break;
    case GET_ADDR:
        renderOperandToString(operand->opr, nested_buffer, sizeof(nested_buffer));
        snprintf(buffer, buffer_size, "&%s", nested_buffer);
        break;
    case GET_VAL:
        renderOperandToString(operand->opr, nested_buffer, sizeof(nested_buffer));
        snprintf(buffer, buffer_size, "*%s", nested_buffer);
        break;
    default:
        snprintf(buffer, buffer_size, "invalid_op(%d)", operand->kind);
        break;
    }
}

/**
 * @brief Helper function to render a single intermediate code instruction.
 * @param current_code The current intermediate code instruction.
 * @param outFile The file pointer where the instruction will be written.
 */
void renderInterCode(InterCode current_code, FILE *outFile)
{
    char opStr1[128], opStr2[128], opStr3[128]; // Buffers for operand strings
    opStr1[0] = opStr2[0] = opStr3[0] = '\0';   // Clear buffers

    // Render operand strings
    for (int i = 0; i < 3; ++i)
    {
        if (current_code->ops[i])
        {
            renderOperandToString(current_code->ops[i], (i == 0) ? opStr1 : (i == 1) ? opStr2
                                                                                     : opStr3,
                                  sizeof(opStr1));
        }
    }

    // Write instruction to file
    switch (current_code->kind)
    {
    case LABEL_:
        fprintf(outFile, "LABEL %s :", opStr1);
        break;
    case FUNC_:
        fprintf(outFile, "\nFUNCTION %s :", opStr1);
        break;
    case ASSIGN_:
        fprintf(outFile, "  %s := %s", opStr1, opStr2);
        break;
    case PLUS_:
        fprintf(outFile, "  %s := %s + %s", opStr1, opStr2, opStr3);
        break;
    case SUB_:
        fprintf(outFile, "  %s := %s - %s", opStr1, opStr2, opStr3);
        break;
    case MULTI_:
        fprintf(outFile, "  %s := %s * %s", opStr1, opStr2, opStr3);
        break;
    case DIV_:
        fprintf(outFile, "  %s := %s / %s", opStr1, opStr2, opStr3);
        break;
    case MEM_:
        fprintf(outFile, "  *%s := %s", opStr1, opStr2);
        break;
    case GOTO_:
        fprintf(outFile, "  GOTO %s", opStr1);
        break;
    case IF_GOTO_:
        fprintf(outFile, "  IF %s %s %s GOTO %s", opStr1, current_code->relop, opStr2, opStr3);
        break;
    case RETURN_:
        fprintf(outFile, "  RETURN %s", opStr1);
        break;
    case DEC_:
        fprintf(outFile, "  DEC %s %d", opStr1, current_code->size);
        break;
    case ARG_:
        fprintf(outFile, "  ARG %s", opStr1);
        break;
    case CALL_:
        fprintf(outFile, "  %s := CALL %s", opStr1, opStr2);
        break;
    case PARAM_:
        fprintf(outFile, "  PARAM %s", opStr1);
        break;
    case READ_:
        fprintf(outFile, "  READ %s", opStr1);
        break;
    case WRITE_:
        fprintf(outFile, "  WRITE %s", opStr1);
        break;
    default:
        fprintf(outFile, "  # Unknown IR Kind (%d)", current_code->kind);
        break;
    }
    fprintf(outFile, "\n"); // Newline after each instruction
}

/**
 * @brief Writes the intermediate code list to a file.
 * @param targetFilePath The path to the output file.
 * @param codeListHead The head of the intermediate code list.
 * @details Uses fprintf for formatted output and renderOperandToString helper.
 * Includes basic error handling for file operations.
 */
void writeInterCodeToFile(const char *targetFilePath, InterCode codeListHead)
{
    assert(targetFilePath != NULL && "File path cannot be NULL");
    assert(codeListHead != NULL && "InterCode list cannot be NULL");

    FILE *outFile = fopen(targetFilePath, "w");
    assert(outFile != NULL && "Failed to open file for writing");

    InterCode current_code = codeListHead;
    while (current_code != NULL)
    {
        // Skip NULL_IR instructions in the output
        if (current_code->kind == NULL_)
        {
            current_code = current_code->next;
            continue;
        }

        // Render the current instruction to the file
        renderInterCode(current_code, outFile);

        // Move to the next instruction
        current_code = current_code->next;
    }

    fclose(outFile);

    // This is a regular output message (not an error), so printf is appropriate.
    printf("Intermediate code successfully written to %s\n", targetFilePath);
}
