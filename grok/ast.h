#ifndef AST_H
#define AST_H

typedef enum {
    NODE_OBJECT,
    NODE_ARRAY,
    NODE_STRING,
    NODE_NUMBER,
    NODE_BOOLEAN,
    NODE_NULL
} NodeType;

typedef struct KeyValuePair {
    char *key;
    struct AstNode *value;
    struct KeyValuePair *next;
} KeyValuePair;

typedef struct KeyValuePairList {
    KeyValuePair *pair;
    struct KeyValuePairList *next;
} KeyValuePairList;

typedef struct AstNodeList {
    struct AstNode *node;
    struct AstNodeList *next;
} AstNodeList;

typedef struct AstNode {
    NodeType type;
    union {
        KeyValuePairList *pairs; // OBJECT
        AstNodeList *elements;   // ARRAY
        char *sval;              // STRING
        double dval;             // NUMBER
        int bval;                // BOOLEAN
    } data;
} AstNode;

typedef struct TableSchema {
    char *name;
    char **columns;
    int column_count;
    struct TableSchema *next;
} TableSchema;

typedef struct TableRow {
    int id;
    char **values;
    int *foreign_keys; // Store FK values
    struct TableRow *next;
} TableRow;

typedef struct Table {
    TableSchema *schema;
    TableRow *rows;
    struct Table *next;
} Table;

// AST creation functions
AstNode *create_object_node(KeyValuePairList *pairs);
AstNode *create_array_node(AstNodeList *elements);
AstNode *create_string_node(char *value);
AstNode *create_number_node(double value);
AstNode *create_boolean_node(int value);
AstNode *create_null_node();
KeyValuePair *create_pair(char *key, AstNode *value);
KeyValuePairList *create_pair_list(KeyValuePair *pair, KeyValuePairList *next);
AstNodeList *create_node_list(AstNode *node, AstNodeList *next);

// Semantic analysis and CSV generation
void analyze_ast(AstNode *root);
void print_ast(AstNode *node, int indent);
void generate_csv(const char *out_dir);
void free_ast(AstNode *node);
void free_tables(void); // Declaration for free_tables

#endif