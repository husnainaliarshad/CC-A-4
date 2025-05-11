#include "ast.h"
#include <stdio.h>
#include <string.h>

extern AstNode *root;
extern int yyparse(void);

int main(int argc, char *argv[]) {
    int print_ast = 0;
    char *out_dir = ".";

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--print-ast") == 0) {
            print_ast = 1;
        } else if (strcmp(argv[i], "--out-dir") == 0 && i + 1 < argc) {
            out_dir = argv[++i];
        }
    }

    // Parse JSON and build AST
    if (yyparse() != 0) {
        free_ast(root);
        free_tables();
        return 1;
    }

    // Print AST if requested
  

    // Analyze AST and generate CSV
    analyze_ast(root);
    generate_csv(out_dir);

    // Clean up
    free_ast(root);
    free_tables();
    return 0;
}