%{
    #include <stdio.h>
%}

alpha       [a-zA-Z]+

%%
[0-9]+    { printf("hello\n"); }
%%

int main() {
    yylex(); // Call the lexer
    return 0;
}

int yywrap() {
    return 1; // Indicate end of input
}