#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* name;
    char** columns;
    int num_columns;
    FILE* fp;
    int next_id;
} Table;

Table* tables[100];
int num_tables = 0;
char** key_sets = NULL;
int* table_indices = NULL;
int num_key_sets = 0;
char* output_dir = "."; // Define globally

char* join_keys(char** keys, int count) {
    int len = 0;
    for (int i = 0; i < count; i++) len += strlen(keys[i]) + 1;
    char* str = malloc(len);
    str[0] = '\0';
    for (int i = 0; i < count; i++) {
        strcat(str, keys[i]);
        if (i < count - 1) strcat(str, ",");
    }
    return str;
}

int compare_strings(const void* a, const void* b) {
    return strcmp(*(char**)a, *(char**)b);
}

int is_scalar(ASTNode* node) {
    return node && (node->type == STR || node->type == NUM || node->type == TRU || node->type == FALS || node->type == NUL);
}

char* value_to_string(ASTNode* node) {
    if (!node) return strdup("");
    switch (node->type) {
        case STR: return strdup(node->data.string);
        case NUM: return strdup(node->data.string);
        case TRU: return strdup("true");
        case FALS: return strdup("false");
        case NUL: return strdup("");
        default: return strdup("");
    }
}

int process_object(ASTNode* object, char* parent_table, int parent_id, char* array_key, int seq);

void process_array(ASTNode* array, char* parent_table, int parent_id, char* array_key) {
    if (!array) return; // Safety check

    char* table_name = malloc(strlen(array_key) + 5);
    sprintf(table_name, "%s.csv", array_key);
    int table_index = num_tables++;
    tables[table_index] = malloc(sizeof(Table));
    tables[table_index]->name = table_name;
    tables[table_index]->next_id = 1;

    // Handle empty array or scalar array
    if (array->data.array.count == 0 || (array->data.array.elements && array->data.array.elements[0] && is_scalar(array->data.array.elements[0]))) {
        tables[table_index]->columns = malloc(3 * sizeof(char*));
        tables[table_index]->columns[0] = strdup(parent_table ? parent_table : "main_id");
        tables[table_index]->columns[1] = strdup("index");
        tables[table_index]->columns[2] = strdup("value");
        tables[table_index]->num_columns = 3;
    } else {
        // Handle array of objects
        ASTNode* first = (array->data.array.count > 0 && array->data.array.elements) ? array->data.array.elements[0] : NULL;
        int col_count = 2; // main_id/parent_id and seq
        if (first && first->type == OBJ) {
            for (int i = 0; i < first->data.object.count; i++) {
                if (is_scalar(first->data.object.values[i])) col_count++;
                else if (first->data.object.values[i] && first->data.object.values[i]->type == OBJ) col_count++;
            }
        }
        tables[table_index]->columns = malloc(col_count * sizeof(char*));
        tables[table_index]->columns[0] = strdup(parent_table ? parent_table : "main_id");
        tables[table_index]->columns[1] = strdup("seq");
        int col_idx = 2;
        if (first && first->type == OBJ) {
            for (int i = 0; i < first->data.object.count; i++) {
                if (is_scalar(first->data.object.values[i])) {
                    tables[table_index]->columns[col_idx++] = strdup(first->data.object.keys[i]);
                } else if (first->data.object.values[i] && first->data.object.values[i]->type == OBJ) {
                    char* fk = malloc(strlen(first->data.object.keys[i]) + 4);
                    sprintf(fk, "%s_id", first->data.object.keys[i]);
                    tables[table_index]->columns[col_idx++] = fk;
                }
            }
        }
        tables[table_index]->num_columns = col_idx;
    }

    char* filepath = malloc(strlen(output_dir) + strlen(table_name) + 2);
    sprintf(filepath, "%s/%s", output_dir, table_name);
    tables[table_index]->fp = fopen(filepath, "w");
    if (!tables[table_index]->fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filepath);
        exit(1);
    }
    free(filepath);

    for (int i = 0; i < tables[table_index]->num_columns; i++) {
        fprintf(tables[table_index]->fp, "%s", tables[table_index]->columns[i]);
        if (i < tables[table_index]->num_columns - 1) fprintf(tables[table_index]->fp, ",");
    }
    fprintf(tables[table_index]->fp, "\n");

    for (int i = 0; i < array->data.array.count; i++) {
        if (!array->data.array.elements[i]) continue; // Skip null elements
        if (is_scalar(array->data.array.elements[i])) {
            char* val = value_to_string(array->data.array.elements[i]);
            fprintf(tables[table_index]->fp, "%d,%d,\"%s\"\n", parent_id, i, val);
            free(val);
        } else if (array->data.array.elements[i]->type == OBJ) {
            process_object(array->data.array.elements[i], table_name, parent_id, array_key, i);
        }
    }
}

int process_object(ASTNode* object, char* parent_table, int parent_id, char* array_key, int seq) {
    if (!object || object->type != OBJ) return 0; // Safety check

    char** keys = object->data.object.keys;
    int num_keys = object->data.object.count;
    qsort(keys, num_keys, sizeof(char*), compare_strings);
    char* key_set = join_keys(keys, num_keys);

    int table_index = -1;
    for (int i = 0; i < num_key_sets; i++) {
        if (strcmp(key_sets[i], key_set) == 0) {
            table_index = table_indices[i];
            break;
        }
    }

    if (table_index == -1) {
        table_index = num_tables++;
        tables[table_index] = malloc(sizeof(Table));
        tables[table_index]->name = parent_table ? strdup(parent_table) : strdup("main.csv");
        tables[table_index]->next_id = 1;

        int col_count = 1; // id
        for (int i = 0; i < num_keys; i++) {
            if (is_scalar(object->data.object.values[i])) col_count++;
            else if (object->data.object.values[i] && object->data.object.values[i]->type == OBJ) col_count++;
        }
        tables[table_index]->columns = malloc(col_count * sizeof(char*));
        tables[table_index]->columns[0] = strdup("id");
        int col_idx = 1;
        for (int i = 0; i < num_keys; i++) {
            if (is_scalar(object->data.object.values[i])) {
                tables[table_index]->columns[col_idx++] = strdup(keys[i]);
            } else if (object->data.object.values[i] && object->data.object.values[i]->type == OBJ) {
                char* fk = malloc(strlen(keys[i]) + 4);
                sprintf(fk, "%s_id", keys[i]);
                tables[table_index]->columns[col_idx++] = fk;
            }
        }
        tables[table_index]->num_columns = col_idx;

        char* filepath = malloc(strlen(output_dir) + strlen(tables[table_index]->name) + 2);
        sprintf(filepath, "%s/%s", output_dir, tables[table_index]->name);
        tables[table_index]->fp = fopen(filepath, "w");
        if (!tables[table_index]->fp) {
            fprintf(stderr, "Error: Cannot open file %s\n", filepath);
            exit(1);
        }
        free(filepath);

        for (int i = 0; i < col_count; i++) {
            fprintf(tables[table_index]->fp, "%s", tables[table_index]->columns[i]);
            if (i < col_count - 1) fprintf(tables[table_index]->fp, ",");
        }
        fprintf(tables[table_index]->fp, "\n");

        key_sets = realloc(key_sets, (num_key_sets + 1) * sizeof(char*));
        table_indices = realloc(table_indices, (num_key_sets + 1) * sizeof(int));
        key_sets[num_key_sets] = key_set;
        table_indices[num_key_sets] = table_index;
        num_key_sets++;
    } else {
        free(key_set);
    }

    int id = tables[table_index]->next_id++;
    FILE* fp = tables[table_index]->fp;
    fprintf(fp, "%d", id);

    for (int i = 0; i < num_keys; i++) {
        ASTNode* value = object->data.object.values[i];
        if (!value) continue; // Skip null values
        if (is_scalar(value)) {
            char* str = value_to_string(value);
            fprintf(fp, ",\"%s\"", str);
            free(str);
        } else if (value->type == OBJ) {
            int child_id = process_object(value, NULL, 0, NULL, 0);
            fprintf(fp, ",%d", child_id);
        } else if (value->type == ARR) {
            process_array(value, tables[table_index]->name, id, object->data.object.keys[i]);
        }
    }
    fprintf(fp, "\n");

    if (parent_table && array_key) {
        char* filepath = malloc(strlen(output_dir) + strlen(parent_table) + 2);
        sprintf(filepath, "%s/%s", output_dir, parent_table);
        FILE* parent_fp = fopen(filepath, "a");
        if (!parent_fp) {
            fprintf(stderr, "Error: Cannot open file %s\n", filepath);
            exit(1);
        }
        fprintf(parent_fp, "%d,%d", parent_id, seq);
        for (int i = 0; i < num_keys; i++) {
            ASTNode* value = object->data.object.values[i];
            if (!value) continue;
            if (is_scalar(value)) {
                char* str = value_to_string(value);
                fprintf(parent_fp, ",\"%s\"", str);
                free(str);
            } else if (value->type == OBJ) {
                int child_id = process_object(value, NULL, 0, NULL, 0);
                fprintf(parent_fp, ",%d", child_id);
            }
        }
        fprintf(parent_fp, "\n");
        fclose(parent_fp);
        free(filepath);
    }

    return id;
}

void generate_csv(ASTNode* root, char* out_dir) {
    if (!root) return;
    output_dir = out_dir;
    if (root->type == OBJ) {
        process_object(root, NULL, 0, NULL, 0);
    } else if (root->type == ARR) {
        process_array(root, NULL, 0, "root");
    }
}

void cleanup_tables() {
    for (int i = 0; i < num_tables; i++) {
        if (tables[i]->fp) fclose(tables[i]->fp);
        for (int j = 0; j < tables[i]->num_columns; j++) {
            free(tables[i]->columns[j]);
        }
        free(tables[i]->columns);
        free(tables[i]->name);
        free(tables[i]);
    }
    for (int i = 0; i < num_key_sets; i++) {
        free(key_sets[i]);
    }
    free(key_sets);
    free(table_indices);
}