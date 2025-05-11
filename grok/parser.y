%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylex(void);
extern void yyerror(const char *msg);
extern YYLTYPE yylloc;

AstNode *root = NULL;
%}

%union {
    double dval;
    int ival;
    char *sval;
    AstNode *node;
    KeyValuePair *pair;
    KeyValuePairList *pair_list;
    AstNodeList *node_list;
}

%token LBRACE RBRACE LBRACKET RBRACKET COLON COMMA
%token <ival> TRUE FALSE
%token <dval> NUMBER
%token <sval> STRING
%token NULLVAL

%type <node> value object array string number boolean null
%type <pair> pair
%type <pair_list> pair_list
%type <node_list> value_list

%locations

%%

input: value { root = $1; }
     ;

value: object { $$ = $1; }
     | array  { $$ = $1; }
     | string { $$ = $1; }
     | number { $$ = $1; }
     | boolean { $$ = $1; }
     | null   { $$ = $1; }
     ;

object: LBRACE pair_list RBRACE { $$ = create_object_node($2); }
      | LBRACE RBRACE           { $$ = create_object_node(NULL); }
      ;

pair_list: pair                  { $$ = create_pair_list($1, NULL); }
         | pair COMMA pair_list  { $$ = create_pair_list($1, $3); }
         ;

pair: STRING COLON value { $$ = create_pair($1, $3); }
    ;

array: LBRACKET value_list RBRACKET { $$ = create_array_node($2); }
     | LBRACKET RBRACKET           { $$ = create_array_node(NULL); }
     ;

value_list: value                   { $$ = create_node_list($1, NULL); }
          | value COMMA value_list  { $$ = create_node_list($1, $3); }
          ;

string: STRING { $$ = create_string_node($1); }
      ;

number: NUMBER { $$ = create_number_node($1); }
      ;

boolean: TRUE  { $$ = create_boolean_node($1); }
       | FALSE { $$ = create_boolean_node($1); }
       ;

null: NULLVAL { $$ = create_null_node(); }
    ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "Error: %s at line %d, column %d\n", msg, yylloc.first_line, yylloc.first_column);
}