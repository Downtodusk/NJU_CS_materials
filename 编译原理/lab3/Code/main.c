#include <stdio.h>
#include "tree.h"
#include "semantic.h"
#include "ir.h"

/** 
 * @file main.c
 * @brief Entry point of the C-- compiler.
 *
 * This file initializes the compilation process, including parsing,
 * semantic analysis, and optional intermediate representation (IR) generation.
 */

/* ----------------------------- */
/*    External Lexer/Parser     */
/* ----------------------------- */

extern FILE *yyin;         /**< Input file pointer from lexer */
extern int Aerrors;        /**< Syntax error counter */
extern int Berrors;        /**< Semantic error counter */
extern bool array_flag;    /**< Flag for disallowed array operations */
extern Symbol symbol_table[200];

/* Parser-related functions */
extern int yyrestart(FILE *f); /**< Reset the lexer input stream */
extern int yyparse();          /**< Start parsing the input file */

/** 
 * @brief Global root of the abstract syntax tree (AST) 
 */
Node *root = NULL;

/**
 * @brief Compiler main function
 *
 * Launches the compiler on a source file written in C--.
 * Optionally generates intermediate code if no errors are detected.
 *
 * @param argc Argument count
 * @param argv Argument values. argv[1] is the input source file, argv[2] (optional) is the IR output file
 * @return int 0 if successful, non-zero on failure
 */
int main(int argc, char **argv)
{
    // Check for input file argument
    if (argc <= 1)
    {
        fprintf(stderr, "Error: No input file specified.\n");
        return 1;
    }

    // Open the specified source file
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        fprintf(stderr, "Error: Failed to open file '%s'.\n", argv[1]);
        return 1;
    }

    // Start parsing
    yyrestart(f);
    yyparse();

    // Continue only if no errors occurred during parsing
    if (Aerrors + Berrors == 0)
    {
        // Perform semantic analysis
        start_semantic_analysis(root);

        // Generate intermediate code if requested and allowed
        if (Aerrors + Berrors == 0 && argc == 3 && !array_flag)
        {
            init_symbol_table(symbol_table);
            InterCode ir = generate_IR(root);
            writeInterCodeToFile(argv[2], ir);
        }
    }

    return 0;
}
