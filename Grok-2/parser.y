%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern FILE* yyin;
extern int yylex();
void yyerror(const char* s);
ASTNode* root;
int print_ast_flag = 0;
char* output_dir = ".";
%}

%union {
    ASTNode* node;
    char* string;
}

%token <string> STRING NUMBER
%token TRUE FALSE NULL_VAL
%type <node> json value object members pair array elements

%%
json: value { root = $1; }
;

value: object { $$ = $1; }
     | array { $$ = $1; }
     | STRING { $$ = make_string($1); }
     | NUMBER { $$ = make_number($1); }
     | TRUE { $$ = make_true(); }
     | FALSE { $$ = make_false(); }
     | NULL_VAL { $$ = make_null(); }
;

object: '{' '}' { $$ = make_object(NULL); }
      | '{' members '}' { $$ = make_object($2); }
;

members: pair { $$ = $1; }
       | members ',' pair { $$ = make_pair_list($3, $1); }
;

pair: STRING ':' value { $$ = make_pair($1, $3); }
;

array: '[' ']' { $$ = make_array(NULL); }
     | '[' elements ']' { $$ = make_array($2); }
;

elements: value { $$ = $1; }
        | elements ',' value { $$ = make_element_list($3, $1); }
;

%%

int main(int argc, char* argv[]) {
    char* input_file = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--print-ast") == 0) {
            print_ast_flag = 1;
        } else if (strcmp(argv[i], "--out-dir") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (!input_file) {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Error: Invalid arguments\n");
            exit(1);
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: No input file provided\n");
        exit(1);
    }

    yyin = fopen(input_file, "r");
    if (!yyin) {
        fprintf(stderr, "Error: Cannot open input file %s\n", input_file);
        exit(1);
    }

    if (yyparse() != 0) {
        fclose(yyin);
        exit(1);
    }
    fclose(yyin);

    if (print_ast_flag) {
        print_ast_node(root, 0);
    }

    generate_csv(root, output_dir);

    free_ast(root);
    cleanup_tables();
    return 0;
}