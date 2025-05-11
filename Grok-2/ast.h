#ifndef AST_H
#define AST_H

typedef enum {
    OBJ,
    ARR,
    STR,
    NUM,
    TRU,
    FALS,
    NUL
} NodeType;

typedef struct ASTNode {
    NodeType type;
    union {
        struct { char** keys; struct ASTNode** values; int count; } object;
        struct { struct ASTNode** elements; int count; } array;
        char* string;
    } data;
} ASTNode;

ASTNode* make_object(ASTNode* members);
ASTNode* make_array(ASTNode* elements);
ASTNode* make_string(char* string);
ASTNode* make_number(char* number);
ASTNode* make_true();
ASTNode* make_false();
ASTNode* make_null();
ASTNode* make_pair(char* key, ASTNode* value);
ASTNode* make_pair_list(ASTNode* pair, ASTNode* list);
ASTNode* make_element_list(ASTNode* element, ASTNode* list);

void free_ast(ASTNode* node);
void print_ast_node(ASTNode* node, int indent);
void generate_csv(ASTNode* root, char* out_dir);
void cleanup_tables();

#endif