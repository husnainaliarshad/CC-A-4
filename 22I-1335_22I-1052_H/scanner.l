%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

int line = 1;
int column = 1;

void update_position(char* text) {
    for (char* p = text; *p; p++) {
        if (*p == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }
}

char* process_string(char* text) {
    int len = strlen(text);
    char* str = malloc(len - 1); // Exclude quotes
    int j = 0;
    for (int i = 1; i < len - 1; i++) {
        if (text[i] == '\\') {
            i++;
            switch (text[i]) {
                case '"': case '\\': case '/': str[j++] = text[i]; break;
                case 'n': str[j++] = '\n'; break;
                case 't': str[j++] = '\t'; break;
                case 'r': str[j++] = '\r'; break;
                case 'b': str[j++] = '\b'; break;
                case 'f': str[j++] = '\f'; break;
                default: str[j++] = text[i];
            }
        } else {
            str[j++] = text[i];
        }
    }
    str[j] = '\0';
    return str;
}
%}

%option noyywrap

%%
[ \t\r]+        { update_position(yytext); }
\n              { line++; column = 1; }
"{"             { update_position(yytext); return '{'; }
"}"             { update_position(yytext); return '}'; }
"["             { update_position(yytext); return '['; }
"]"             { update_position(yytext); return ']'; }
","             { update_position(yytext); return ','; }
":"             { update_position(yytext); return ':'; }
"true"          { update_position(yytext); return TRUE; }
"false"         { update_position(yytext); return FALSE; }
"null"          { update_position(yytext); return NULL_VAL; }
\"(\\.|[^\"\\])*\" { 
    update_position(yytext); 
    char* processed = process_string(yytext); 
    yylval.string = processed; 
    return STRING; 
}
-?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)? { 
    update_position(yytext); 
    yylval.string = strdup(yytext); 
    return NUMBER; 
}
.               { fprintf(stderr, "Error: Invalid character '%s' at line %d, column %d\n", yytext, line, column); exit(1); }
%%

void yyerror(const char* s) {
    fprintf(stderr, "Error: %s at line %d, column %d\n", s, line, column);
    exit(1);
}