#ifndef INTERCODE_H
#define INTERCODE_H

#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"

/**
 * @file intercode.h
 * @brief Intermediate representation definitions and translation functions.
 */

/** @typedef Operand
 *  @brief Pointer to Operand_t, represents an operand in intermediate code.
 */
typedef struct Operand_ Operand_t;
typedef Operand_t *Operand;

/** @typedef InterCode
 *  @brief Pointer to InterCode_t, represents an intermediate code instruction.
 */
typedef struct InterCode_ InterCode_t;
typedef InterCode_t *InterCode;

/**
 * @struct Operand_
 * @brief Represents an operand in the intermediate code.
 */
struct Operand_
{
    /**
     * @enum
     * @brief Operand kinds.
     */
    enum
    {
        VARIABLE, /**< Variable */
        TEMP_VAR, /**< Temporary variable */
        CONSTANT, /**< Constant value */
        GET_ADDR, /**< Get address of a variable */
        GET_VAL,  /**< Get value from an address */
        LABEL,    /**< Label */
        FUNC      /**< Function name */
    } kind;

    union
    {
        int no;        /**< Temporary variable or label number */
        int value;     /**< Constant value */
        char name[32]; /**< Variable or function name */
        Operand opr;   /**< Nested operand (e.g., for address/value ops) */
    };

    Type type;    /**< Type of the operand */
    bool is_addr; /**< Whether this operand represents an address */
};

/**
 * @struct InterCode_
 * @brief Represents a single instruction in the intermediate code.
 */
struct InterCode_
{
    /**
     * @enum
     * @brief Intermediate code instruction kinds.
     */
    enum
    {
        LABEL_,   /**< LABEL x : */
        FUNC_,    /**< FUNCTION f : */
        ASSIGN_,  /**< x := y */
        PLUS_,    /**< x := y + z */
        SUB_,     /**< x := y - z */
        MULTI_,   /**< x := y * z */
        DIV_,     /**< x := y / z */
        MEM_,     /**< *x := y or x := *y */
        GOTO_,    /**< GOTO x */
        IF_GOTO_, /**< IF x [relop] y GOTO z */
        RETURN_,  /**< RETURN x */
        DEC_,     /**< DEC x [size] */
        ARG_,     /**< ARG x */
        CALL_,    /**< x := CALL f */
        PARAM_,   /**< PARAM x */
        READ_,    /**< READ x */
        WRITE_,   /**< WRITE x */
        NULL_     /**< Null or placeholder instruction */
    } kind;

    Operand ops[3]; /**< Operands used by the instruction (up to 3) */

    union
    {
        char relop[4]; /**< Relational operator (used in IF_GOTO) */
        int size;      /**< Memory size (used in DEC) */
    };

    InterCode prev; /**< Previous instruction (linked list) */
    InterCode next; /**< Next instruction (linked list) */
};

/**
 * @enum Operator
 * @brief Enumeration of arithmetic operators.
 */
enum Operator
{
    PLUS_OP,  /**< Addition */
    MINUS_OP, /**< Subtraction */
    STAR_OP,  /**< Multiplication */
    DIV_OP    /**< Division */
};

/**
 * @brief Print the intermediate code sequence to a file.
 * @param targetFilePath Name of the output file.
 * @param codeListHead Head of the intermediate code list.
 */
void writeInterCodeToFile(const char *targetFilePath, InterCode codeListHead);

/**
 * @brief Print an operand to the given file.
 * @param operand Operand to print.
 */
void renderOperandToString(Operand operand, char *buffer, size_t buffer_size);

/**
 * @brief Insert an instruction before another in the code list.
 * @param dst The instruction to insert before.
 * @param src The instruction to insert.
 */
void insertCode(InterCode dst, InterCode src);

/**
 * @brief Insert an instruction before another in the code list, but direct to tail.
 * @param dst The instruction to insert before.
 * @param src The instruction to insert.
 * @param tail The tail of the instrcution to insert
 */
void insertCodeWithTail(InterCode dst, InterCode src, InterCode tail);

/**
 * @brief Format the intermediate code (mainly for printing).
 * @param code Head of the intermediate code list.
 */
void formatCode(InterCode code);

/**
 * @brief get size of the type
 */
int getSizeof(Type type);

/**
 * @brief Generate intermediate code from the syntax tree.
 * @param program Root node of the syntax tree.
 * @return Head of the intermediate code list.
 */
InterCode generate_IR(Node *program);

/* --- Translation functions for different syntax nodes --- */

/** Translate external definition list. */
InterCode translate_ExtDefList(Node *node);

/** Translate a single external definition. */
InterCode translate_ExtDef(Node *node);

/** Translate function definition. */
InterCode translate_Function(Function func, Operand place);

/** Translate compound statement. */
InterCode translate_CompSt(Node *node);

/** Translate parameter list. */
InterCode translate_ParamList(FieldList param);

/** Translate definition list. */
InterCode translate_DefList(Node *node);

/** Translate a single definition. */
InterCode translate_Def(Node *node);

/** Translate declaration list. */
InterCode translate_DecList(Node *node, Type type);

/** Translate a single declaration. */
InterCode translate_Dec(Node *node, Type type);

/** Translate variable declaration. */
FieldList translate_VarDec(Node *node, Type type, Type arr_type, int layer);

/** Translate statement list. */
InterCode translate_StmtList(Node *node);

/** Translate a single statement. */
InterCode translate_Stmt(Node *node);

/** Translate a condition expression. */
InterCode translate_Condition(Node *exp, Operand labeltrue, Operand labelfalse);

/** Translate an expression. */
InterCode translate_Exp(Node *node, Operand place);

/** Translate function arguments. */
InterCode translate_Args(Node *node);

/**
 * @brief Generate arithmetic intermediate code.
 * @param dst Destination operand.
 * @param src1 First source operand.
 * @param src2 Second source operand.
 * @param op Arithmetic operation to perform.
 * @return Pointer to the generated instruction.
 */
InterCode arithmetic(Operand dst, Operand src1, Operand src2, enum Operator op);

#endif
