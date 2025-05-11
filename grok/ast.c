#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static Table *tables = NULL;
static int id_counter = 1;

AstNode *create_object_node(KeyValuePairList *pairs) {
    AstNode *node = malloc(sizeof(AstNode));
    node->type = NODE_OBJECT;
    node->data.pairs = pairs;
    return node;
}

AstNode *create_array_node(AstNodeList *elements) {
    AstNode *node = malloc(sizeof(AstNode));
    node->type = NODE_ARRAY;
    node->data.elements = elements;
    return node;
}

AstNode *create_string_node(char *value) {
    AstNode *node = malloc(sizeof(AstNode));
    node->type = NODE_STRING;
    node->data.sval = strdup(value);
    return node;
}

AstNode *create_number_node(double value) {
    AstNode *node = malloc(sizeof(AstNode));
    node->type = NODE_NUMBER;
    node->data.dval = value;
    return node;
}

AstNode *create_boolean_node(int value) {
    AstNode *node = malloc(sizeof(AstNode));
    node->type = NODE_BOOLEAN;
    node->data.bval = value;
    return node;
}

AstNode *create_null_node() {
    AstNode *node = malloc(sizeof(AstNode));
    node->type = NODE_NULL;
    return node;
}

KeyValuePair *create_pair(char *key, AstNode *value) {
    KeyValuePair *pair = malloc(sizeof(KeyValuePair));
    pair->key = strdup(key);
    pair->value = value;
    pair->next = NULL;
    return pair;
}

KeyValuePairList *create_pair_list(KeyValuePair *pair, KeyValuePairList *next) {
    KeyValuePairList *list = malloc(sizeof(KeyValuePairList));
    list->pair = pair;
    list->next = next;
    return list;
}

AstNodeList *create_node_list(AstNode *node, AstNodeList *next) {
    AstNodeList *list = malloc(sizeof(AstNodeList));
    list->node = node;
    list->next = next;
    return list;
}

// Helper to create a table
Table *create_table(const char *name, char **columns, int column_count) {
    Table *table = malloc(sizeof(Table));
    table->schema = malloc(sizeof(TableSchema));
    table->schema->name = strdup(name);
    table->schema->columns = malloc(sizeof(char *) * column_count);
    for (int i = 0; i < column_count; i++)
        table->schema->columns[i] = strdup(columns[i]);
    table->schema->column_count = column_count;
    table->schema->next = NULL;
    table->rows = NULL;
    table->next = tables;
    tables = table;
    return table;
}

// Helper to add a row to a table
void add_row(Table *table, int id, char **values, int *foreign_keys) {
    TableRow *row = malloc(sizeof(TableRow));
    row->id = id;
    row->values = malloc(sizeof(char *) * table->schema->column_count);
    for (int i = 0; i < table->schema->column_count; i++)
        row->values[i] = values[i] ? strdup(values[i]) : strdup("");
    row->foreign_keys = foreign_keys ? memcpy(malloc(sizeof(int) * table->schema->column_count), foreign_keys, sizeof(int) * table->schema->column_count) : NULL;
    row->next = table->rows;
    table->rows = row;
}

// Helper to check if all array elements are scalars
int is_scalar_array(AstNode *array) {
    for (AstNodeList *elem = array->data.elements; elem; elem = elem->next) {
        if (elem->node->type == NODE_OBJECT || elem->node->type == NODE_ARRAY)
            return 0;
    }
    return 1;
}

// Helper to get object schema (keys)
void get_object_schema(AstNode *node, char ***keys, int *count) {
    *count = 0;
    for (KeyValuePairList *p = node->data.pairs; p; p = p->next)
        (*count)++;
    *keys = malloc(sizeof(char *) * *count);
    int i = 0;
    for (KeyValuePairList *p = node->data.pairs; p; p = p->next)
        (*keys)[i++] = p->pair->key;
}

// Helper to find or create a table by schema
Table *find_or_create_table(const char *name, char **columns, int column_count) {
    for (Table *t = tables; t; t = t->next) {
        if (strcmp(t->schema->name, name) == 0 && t->schema->column_count == column_count) {
            int match = 1;
            for (int i = 0; i < column_count; i++)
                if (strcmp(t->schema->columns[i], columns[i]) != 0) {
                    match = 0;
                    break;
                }
            if (match)
                return t;
        }
    }
    return create_table(name, columns, column_count);
}

// Semantic analysis
void analyze_ast_node(AstNode *node, const char *parent_table, int parent_id, const char *field_name) {
    if (!node)
        return;

    switch (node->type) {
        case NODE_OBJECT: {
            // R1: Object → table row
            char **keys;
            int key_count;
            get_object_schema(node, &keys, &key_count);

            // Create table with id + keys
            char **columns = malloc(sizeof(char *) * (key_count + 1));
            columns[0] = "id";
            for (int i = 0; i < key_count; i++)
                columns[i + 1] = keys[i];
            Table *table = find_or_create_table(parent_table ? parent_table : "root", columns, key_count + 1);

            // Prepare row values
            char **values = calloc(key_count + 1, sizeof(char *));
            int *foreign_keys = calloc(key_count + 1, sizeof(int));
            int current_id = id_counter++;

            for (KeyValuePairList *p = node->data.pairs; p; p = p->next) {
                int col_idx = -1;
                for (int i = 1; i < table->schema->column_count; i++)
                    if (strcmp(table->schema->columns[i], p->pair->key) == 0) {
                        col_idx = i;
                        break;
                    }
                if (p->pair->value->type == NODE_OBJECT) {
                    // Nested object: create child table
                    char child_table[256];
                    snprintf(child_table, sizeof(child_table), "%s_%s", parent_table ? parent_table : "root", p->pair->key);
                    int child_id = id_counter;
                    analyze_ast_node(p->pair->value, child_table, child_id, p->pair->key);
                    foreign_keys[col_idx] = child_id;
                    values[col_idx] = malloc(16);
                    snprintf(values[col_idx], 16, "%d", child_id);
                } else if (p->pair->value->type == NODE_ARRAY && !is_scalar_array(p->pair->value)) {
                    // R2: Array of objects → child table
                    char child_table[256];
                    snprintf(child_table, sizeof(child_table), "%s_%s", parent_table ? parent_table : "root", p->pair->key);
                    analyze_ast_node(p->pair->value, child_table, current_id, p->pair->key);
                } else if (p->pair->value->type == NODE_ARRAY) {
                    // R3: Array of scalars → junction table
                    char junction_table[256];
                    snprintf(junction_table, sizeof(junction_table), "%s_%s", parent_table ? parent_table : "root", p->pair->key);
                    analyze_ast_node(p->pair->value, junction_table, current_id, p->pair->key);
                } else {
                    // R4: Scalars → columns
                    switch (p->pair->value->type) {
                        case NODE_STRING:
                            values[col_idx] = p->pair->value->data.sval;
                            break;
                        case NODE_NUMBER: {
                            values[col_idx] = malloc(32);
                            snprintf(values[col_idx], 32, "%.0f", p->pair->value->data.dval);
                            break;
                        }
                        case NODE_BOOLEAN:
                            values[col_idx] = p->pair->value->data.bval ? "true" : "false";
                            break;
                        case NODE_NULL:
                            values[col_idx] = "";
                            break;
                        default:
                            break;
                    }
                }
            }

            // Add row to table
            add_row(table, current_id, values, foreign_keys);
            free(columns);
            free(keys);
            break;
        }
        case NODE_ARRAY: {
            if (is_scalar_array(node)) {
                // R3: Array of scalars → junction table
                char junction_table[256];
                snprintf(junction_table, sizeof(junction_table), "%s_%s", parent_table ? parent_table : "root", field_name);
                char *columns[] = {"parent_id", "index", "value"};
                Table *table = find_or_create_table(junction_table, columns, 3);
                int idx = 0;
                for (AstNodeList *elem = node->data.elements; elem; elem = elem->next, idx++) {
                    char **values = calloc(3, sizeof(char *));
                    int *foreign_keys = calloc(3, sizeof(int));
                    values[0] = malloc(16);
                    snprintf(values[0], 16, "%d", parent_id);
                    values[1] = malloc(16);
                    snprintf(values[1], 16, "%d", idx);
                    switch (elem->node->type) {
                        case NODE_STRING:
                            values[2] = elem->node->data.sval;
                            break;
                        case NODE_NUMBER:
                            values[2] = malloc(32);
                            snprintf(values[2], 32, "%.0f", elem->node->data.dval);
                            break;
                        case NODE_BOOLEAN:
                            values[2] = elem->node->data.bval ? "true" : "false";
                            break;
                        case NODE_NULL:
                            values[2] = "";
                            break;
                        default:
                            break;
                    }
                    add_row(table, id_counter++, values, foreign_keys);
                }
            } else {
                // R2: Array of objects → child table
                int idx = 0;
                for (AstNodeList *elem = node->data.elements; elem; elem = elem->next, idx++) {
                    char **keys;
                    int key_count;
                    get_object_schema(elem->node, &keys, &key_count);
                    char **columns = malloc(sizeof(char *) * (key_count + 2));
                    columns[0] = "parent_id";
                    columns[1] = "seq";
                    for (int i = 0; i < key_count; i++)
                        columns[i + 2] = keys[i];
                    Table *table = find_or_create_table(field_name, columns, key_count + 2);
                    free(columns);
                    free(keys);
                    char child_table[256];
                    snprintf(child_table, sizeof(child_table), "%s", field_name);
                    analyze_ast_node(elem->node, child_table, parent_id, field_name);
                }
            }
            break;
        }
        default:
            break;
    }
}

void analyze_ast(AstNode *root) {
    if (!root)
        return;
    analyze_ast_node(root, NULL, 0, "root");
}

void print_ast(AstNode *node, int indent) {
    if (!node)
        return;
    for (int i = 0; i < indent; i++)
        printf("  ");
    switch (node->type) {
        case NODE_OBJECT:
            printf("OBJECT:\n");
            for (KeyValuePairList *p = node->data.pairs; p; p = p->next) {
                for (int i = 0; i < indent + 1; i++)
                    printf("  ");
                printf("%s:\n", p->pair->key);
                print_ast(p->pair->value, indent + 2);
            }
            break;
        case NODE_ARRAY:
            printf("ARRAY:\n");
            for (AstNodeList *e = node->data.elements; e; e = e->next)
                print_ast(e->node, indent + 1);
            break;
        case NODE_STRING:
            printf("STRING: \"%s\"\n", node->data.sval);
            break;
        case NODE_NUMBER:
            printf("NUMBER: %f\n", node->data.dval);
            break;
        case NODE_BOOLEAN:
            printf("BOOLEAN: %s\n", node->data.bval ? "true" : "false");
            break;
        case NODE_NULL:
            printf("NULL\n");
            break;
    }
}

void generate_csv(const char *out_dir) {
    for (Table *table = tables; table; table = table->next) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s.csv", out_dir, table->schema->name);
        FILE *fp = fopen(filepath, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open file %s\n", filepath);
            exit(1);
        }

        // Write header
        for (int i = 0; i < table->schema->column_count; i++) {
            fprintf(fp, "%s", table->schema->columns[i]);
            if (i < table->schema->column_count - 1)
                fprintf(fp, ",");
        }
        fprintf(fp, "\n");

        // Write rows
        for (TableRow *row = table->rows; row; row = row->next) {
            fprintf(fp, "%d", row->id);
            for (int i = 1; i < table->schema->column_count; i++) {
                fprintf(fp, ",");
                if (row->foreign_keys && row->foreign_keys[i]) {
                    fprintf(fp, "%d", row->foreign_keys[i]);
                } else {
                    char *value = row->values[i];
                    if (value && strlen(value) > 0) {
                        if (strchr(value, ',') || strchr(value, '"')) {
                            fprintf(fp, "\"");
                            for (char *c = value; *c; c++) {
                                if (*c == '"')
                                    fprintf(fp, "\"");
                                fprintf(fp, "%c", *c);
                            }
                            fprintf(fp, "\"");
                        } else {
                            fprintf(fp, "%s", value);
                        }
                    }
                }
            }
            fprintf(fp, "\n");
            fflush(fp); // Stream output
        }
        fclose(fp);
    }
}

void free_ast(AstNode *node) {
    if (!node)
        return;
    switch (node->type) {
        case NODE_OBJECT:
            for (KeyValuePairList *p = node->data.pairs, *next; p; p = next) {
                next = p->next;
                free(p->pair->key);
                free_ast(p->pair->value);
                free(p->pair);
                free(p);
            }
            break;
        case NODE_ARRAY:
            for (AstNodeList *e = node->data.elements, *next; e; e = next) {
                next = e->next;
                free_ast(e->node);
                free(e);
            }
            break;
        case NODE_STRING:
            free(node->data.sval);
            break;
        default:
            break;
    }
    free(node);
}

// Free tables
void free_tables(void) {
    for (Table *t = tables, *next; t; t = next) {
        next = t->next;
        free(t->schema->name);
        for (int i = 0; i < t->schema->column_count; i++)
            free(t->schema->columns[i]);
        free(t->schema->columns);
        free(t->schema);
        for (TableRow *r = t->rows, *rnext; r; r = rnext) {
            rnext = r->next;
            for (int i = 0; i < t->schema->column_count; i++)
                free(r->values[i]);
            free(r->values);
            free(r->foreign_keys);
            free(r);
        }
        free(t);
    }
    tables = NULL;
}