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
char* output_dir = ".";

char* join_keys(char** keys, int count) {
    int len = 0;
    for (int i = 0; i < count; i++) len += (keys[i] ? strlen(keys[i]) : 0) + 1;
    char* str = malloc(len + 1);
    if (!str) {
        fprintf(stderr, "Error: Memory allocation failed for join_keys\n");
        exit(1);
    }
    str[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (keys[i]) {
            strcat(str, keys[i]);
            if (i < count - 1) strcat(str, ",");
        }
    }
    return str;
}

int compare_strings(const void* a, const void* b) {
    return strcmp(*(char**)a, *(char**)b);
}

int is_scalar(ASTNode* node) {
    if (!node) {
        fprintf(stderr, "Warning: Null node in is_scalar\n");
        return 0;
    }
    // Check if node looks like a string pointer
    char* ptr = (char*)node;
    if (ptr[0] >= 32 && ptr[0] <= 126 && ptr[1] >= 32 && ptr[1] <= 126) {
        fprintf(stderr, "Error: Invalid ASTNode in is_scalar, looks like string '%s'\n", ptr);
        exit(1);
    }
    return (node->type == STR || node->type == NUM || node->type == TRU || node->type == FALS || node->type == NUL);
}

char* value_to_string(ASTNode* node) {
    if (!node) {
        fprintf(stderr, "Warning: Null node in value_to_string\n");
        return strdup("");
    }
    // Check if node looks like a string pointer
    char* ptr = (char*)node;
    if (ptr[0] >= 32 && ptr[0] <= 126 && ptr[1] >= 32 && ptr[1] <= 126) {
        fprintf(stderr, "Error: Invalid ASTNode in value_to_string, looks like string '%s'\n", ptr);
        exit(1);
    }
    switch (node->type) {
        case STR: return strdup(node->data.string ? node->data.string : "");
        case NUM: return strdup(node->data.string ? node->data.string : "");
        case TRU: return strdup("true");
        case FALS: return strdup("false");
        case NUL: return strdup("");
        default:
            fprintf(stderr, "Warning: Invalid node type in value_to_string\n");
            return strdup("");
    }
}

int process_object(ASTNode* object, char* parent_table, int parent_id, char* array_key, int seq);

void process_array(ASTNode* array, char* parent_table, int parent_id, char* array_key) {
    if (!array || array->type != ARR) {
        fprintf(stderr, "Error: Invalid array node\n");
        return;
    }
    if (!array_key) {
        fprintf(stderr, "Warning: Null array_key, using default\n");
        array_key = "array";
    }

    char* table_name = malloc(strlen(array_key) + 5);
    if (!table_name) {
        fprintf(stderr, "Error: Memory allocation failed for table_name\n");
        exit(1);
    }
    sprintf(table_name, "%s.csv", array_key);
    int table_index = num_tables++;
    tables[table_index] = malloc(sizeof(Table));
    if (!tables[table_index]) {
        fprintf(stderr, "Error: Memory allocation failed for table\n");
        exit(1);
    }
    tables[table_index]->name = table_name;
    tables[table_index]->next_id = 1;

    // Check if array contains scalars or objects
    int is_scalar_array = 0;
    if (array->data.array.count > 0 && array->data.array.elements && array->data.array.elements[0]) {
        is_scalar_array = is_scalar(array->data.array.elements[0]);
    }

    if (array->data.array.count == 0 || is_scalar_array) {
        tables[table_index]->columns = malloc(3 * sizeof(char*));
        if (!tables[table_index]->columns) {
            fprintf(stderr, "Error: Memory allocation failed for columns\n");
            exit(1);
        }
        tables[table_index]->columns[0] = strdup(parent_table ? parent_table : "main_id");
        tables[table_index]->columns[1] = strdup("index");
        tables[table_index]->columns[2] = strdup("value");
        tables[table_index]->num_columns = 3;
    } else {
        ASTNode* first = (array->data.array.count > 0 && array->data.array.elements) ? array->data.array.elements[0] : NULL;
        int col_count = 2; // parent_id and seq
        if (first && first->type == OBJ) {
            for (int i = 0; i < first->data.object.count; i++) {
                if (first->data.object.values[i] && is_scalar(first->data.object.values[i])) col_count++;
                else if (first->data.object.values[i] && first->data.object.values[i]->type == OBJ) col_count++;
            }
        }
        tables[table_index]->columns = malloc(col_count * sizeof(char*));
        if (!tables[table_index]->columns) {
            fprintf(stderr, "Error: Memory allocation failed for columns\n");
            exit(1);
        }
        tables[table_index]->columns[0] = strdup(parent_table ? parent_table : "main_id");
        tables[table_index]->columns[1] = strdup("seq");
        int col_idx = 2;
        if (first && first->type == OBJ) {
            for (int i = 0; i < first->data.object.count; i++) {
                if (first->data.object.values[i] && is_scalar(first->data.object.values[i])) {
                    tables[table_index]->columns[col_idx++] = strdup(first->data.object.keys[i] ? first->data.object.keys[i] : "unknown");
                } else if (first->data.object.values[i] && first->data.object.values[i]->type == OBJ) {
                    char* fk = malloc(strlen(first->data.object.keys[i] ? first->data.object.keys[i] : "unknown") + 4);
                    if (!fk) {
                        fprintf(stderr, "Error: Memory allocation failed for foreign key\n");
                        exit(1);
                    }
                    sprintf(fk, "%s_id", first->data.object.keys[i] ? first->data.object.keys[i] : "unknown");
                    tables[table_index]->columns[col_idx++] = fk;
                }
            }
        }
        tables[table_index]->num_columns = col_idx;
    }

    char* filepath = malloc(strlen(output_dir) + strlen(table_name) + 2);
    if (!filepath) {
        fprintf(stderr, "Error: Memory allocation failed for filepath\n");
        exit(1);
    }
    sprintf(filepath, "%s/%s", output_dir, table_name);
    tables[table_index]->fp = fopen(filepath, "w");
    if (!tables[table_index]->fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filepath);
        exit(1);
    }
    free(filepath);

    for (int i = 0; i < tables[table_index]->num_columns; i++) {
        fprintf(tables[table_index]->fp, "%s", tables[table_index]->columns[i] ? tables[table_index]->columns[i] : "unknown");
        if (i < tables[table_index]->num_columns - 1) fprintf(tables[table_index]->fp, ",");
    }
    fprintf(tables[table_index]->fp, "\n");

    for (int i = 0; i < array->data.array.count; i++) {
        if (!array->data.array.elements || !array->data.array.elements[i]) {
            fprintf(stderr, "Warning: Skipping null element at index %d\n", i);
            continue;
        }
        // Check if the pointer looks like a string
        char* ptr = (char*)array->data.array.elements[i];
        if (ptr[0] >= 32 && ptr[0] <= 126 && ptr[1] >= 32 && ptr[1] <= 126) {
            fprintf(stderr, "Error: Invalid ASTNode at array index %d, looks like string '%s'\n", i, ptr);
            exit(1);
        }
        if (is_scalar(array->data.array.elements[i])) {
            char* val = value_to_string(array->data.array.elements[i]);
            fprintf(tables[table_index]->fp, "%d,%d,\"%s\"\n", parent_id, i, val ? val : "");
            if (val) free(val);
        } else if (array->data.array.elements[i]->type == OBJ) {
            process_object(array->data.array.elements[i], table_name, parent_id, array_key, i);
        } else {
            fprintf(stderr, "Warning: Skipping invalid element at index %d\n", i);
        }
    }
}

int process_object(ASTNode* object, char* parent_table, int parent_id, char* array_key, int seq) {
    if (!object || object->type != OBJ) {
        fprintf(stderr, "Error: Invalid object node\n");
        return 0;
    }

    char** keys = object->data.object.keys;
    int num_keys = object->data.object.count;
    if (!keys || num_keys <= 0) {
        fprintf(stderr, "Warning: Empty or invalid object\n");
        return 0;
    }

    qsort(keys, num_keys, sizeof(char*), compare_strings);
    char* key_set = join_keys(keys, num_keys);

    int table_index = -1;
    for (int i = 0; i < num_key_sets; i++) {
        if (key_sets[i] && key_set && strcmp(key_sets[i], key_set) == 0) {
            table_index = table_indices[i];
            break;
        }
    }

    if (table_index == -1) {
        table_index = num_tables++;
        tables[table_index] = malloc(sizeof(Table));
        if (!tables[table_index]) {
            fprintf(stderr, "Error: Memory allocation failed for table\n");
            exit(1);
        }
        tables[table_index]->name = parent_table ? strdup(parent_table) : strdup("main.csv");
        tables[table_index]->next_id = 1;

        int col_count = 1; // id
        for (int i = 0; i < num_keys; i++) {
            if (object->data.object.values[i] && is_scalar(object->data.object.values[i])) col_count++;
            else if (object->data.object.values[i] && object->data.object.values[i]->type == OBJ) col_count++;
        }
        tables[table_index]->columns = malloc(col_count * sizeof(char*));
        if (!tables[table_index]->columns) {
            fprintf(stderr, "Error: Memory allocation failed for columns\n");
            exit(1);
        }
        tables[table_index]->columns[0] = strdup("id");
        int col_idx = 1;
        for (int i = 0; i < num_keys; i++) {
            if (object->data.object.values[i] && is_scalar(object->data.object.values[i])) {
                tables[table_index]->columns[col_idx++] = strdup(keys[i] ? keys[i] : "unknown");
            } else if (object->data.object.values[i] && object->data.object.values[i]->type == OBJ) {
                char* fk = malloc(strlen(keys[i] ? keys[i] : "unknown") + 4);
                if (!fk) {
                    fprintf(stderr, "Error: Memory allocation failed for foreign key\n");
                    exit(1);
                }
                sprintf(fk, "%s_id", keys[i] ? keys[i] : "unknown");
                tables[table_index]->columns[col_idx++] = fk;
            }
        }
        tables[table_index]->num_columns = col_idx;

        char* filepath = malloc(strlen(output_dir) + strlen(tables[table_index]->name) + 2);
        if (!filepath) {
            fprintf(stderr, "Error: Memory allocation failed for filepath\n");
            exit(1);
        }
        sprintf(filepath, "%s/%s", output_dir, tables[table_index]->name);
        tables[table_index]->fp = fopen(filepath, "w");
        if (!tables[table_index]->fp) {
            fprintf(stderr, "Error: Cannot open file %s\n", filepath);
            exit(1);
        }
        free(filepath);

        for (int i = 0; i < col_count; i++) {
            fprintf(tables[table_index]->fp, "%s", tables[table_index]->columns[i] ? tables[table_index]->columns[i] : "unknown");
            if (i < col_count - 1) fprintf(tables[table_index]->fp, ",");
        }
        fprintf(tables[table_index]->fp, "\n");

        key_sets = realloc(key_sets, (num_key_sets + 1) * sizeof(char*));
        table_indices = realloc(table_indices, (num_key_sets + 1) * sizeof(int));
        if (!key_sets || !table_indices) {
            fprintf(stderr, "Error: Memory allocation failed for key_sets/table_indices\n");
            exit(1);
        }
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
        if (!value) {
            fprintf(stderr, "Warning: Skipping null value for key %s\n", keys[i] ? keys[i] : "unknown");
            fprintf(fp, ",\"\"");
            continue;
        }
        if (is_scalar(value)) {
            char* str = value_to_string(value);
            fprintf(fp, ",\"%s\"", str ? str : "");
            if (str) free(str);
        } else if (value->type == OBJ) {
            int child_id = process_object(value, NULL, 0, NULL, 0);
            fprintf(fp, ",%d", child_id);
        } else if (value->type == ARR) {
            process_array(value, tables[table_index]->name, id, keys[i] ? keys[i] : "unknown");
            fprintf(fp, ",\"\"");
        } else {
            fprintf(stderr, "Warning: Skipping invalid value for key %s\n", keys[i] ? keys[i] : "unknown");
            fprintf(fp, ",\"\"");
        }
    }
    fprintf(fp, "\n");

    if (parent_table && array_key) {
        char* filepath = malloc(strlen(output_dir) + strlen(parent_table) + 2);
        if (!filepath) {
            fprintf(stderr, "Error: Memory allocation failed for filepath\n");
            exit(1);
        }
        sprintf(filepath, "%s/%s", output_dir, parent_table);
        FILE* parent_fp = fopen(filepath, "a");
        if (!parent_fp) {
            fprintf(stderr, "Error: Cannot open file %s\n", filepath);
            exit(1);
        }
        fprintf(parent_fp, "%d,%d", parent_id, seq);
        for (int i = 0; i < num_keys; i++) {
            ASTNode* value = object->data.object.values[i];
            if (!value) {
                fprintf(parent_fp, ",\"\"");
                continue;
            }
            if (is_scalar(value)) {
                char* str = value_to_string(value);
                fprintf(parent_fp, ",\"%s\"", str ? str : "");
                if (str) free(str);
            } else if (value->type == OBJ) {
                int child_id = process_object(value, NULL, 0, NULL, 0);
                fprintf(parent_fp, ",%d", child_id);
            } else {
                fprintf(parent_fp, ",\"\"");
            }
        }
        fprintf(parent_fp, "\n");
        fclose(parent_fp);
        free(filepath);
    }

    return id;
}

void generate_csv(ASTNode* root, char* out_dir) {
    if (!root) {
        fprintf(stderr, "Error: Null root node\n");
        return;
    }
    output_dir = out_dir;
    if (root->type == OBJ) {
        process_object(root, NULL, 0, NULL, 0);
    } else if (root->type == ARR) {
        process_array(root, NULL, 0, "root");
    } else {
        fprintf(stderr, "Error: Invalid root node type\n");
    }
}

void cleanup_tables() {
    for (int i = 0; i < num_tables; i++) {
        if (tables[i]) {
            if (tables[i]->fp) fclose(tables[i]->fp);
            for (int j = 0; j < tables[i]->num_columns; j++) {
                if (tables[i]->columns[j]) free(tables[i]->columns[j]);
            }
            free(tables[i]->columns);
            if (tables[i]->name) free(tables[i]->name);
            free(tables[i]);
        }
    }
    for (int i = 0; i < num_key_sets; i++) {
        if (key_sets[i]) free(key_sets[i]);
    }
    free(key_sets);
    free(table_indices);
}