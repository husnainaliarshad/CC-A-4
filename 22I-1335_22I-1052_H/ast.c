#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode* make_object(ASTNode* members) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for object\n");
        exit(1);
    }
    node->type = OBJ;
    int count = 0;
    ASTNode* temp = members;
    while (temp) {
        count++;
        temp = temp->data.object.values[1]; // Next in list
    }
    node->data.object.keys = malloc(count * sizeof(char*));
    node->data.object.values = malloc(count * sizeof(ASTNode*));
    if (!node->data.object.keys || !node->data.object.values) {
        fprintf(stderr, "Error: Memory allocation failed for object arrays\n");
        exit(1);
    }
    node->data.object.count = count;
    temp = members;
    for (int i = count - 1; i >= 0; i--) {
        if (!temp->data.object.keys[0] || !temp->data.object.values[0]) {
            fprintf(stderr, "Error: Invalid pair in object list\n");
            exit(1);
        }
        node->data.object.keys[i] = temp->data.object.keys[0];
        node->data.object.values[i] = temp->data.object.values[0];
        ASTNode* next = temp->data.object.values[1];
        free(temp->data.object.keys);
        free(temp->data.object.values);
        free(temp);
        temp = next;
    }
    return node;
}

ASTNode* make_array(ASTNode* elements) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for array\n");
        exit(1);
    }
    node->type = ARR;
    int count = 0;
    ASTNode* temp = elements;
    while (temp) {
        count++;
        temp = temp->data.array.elements[1]; // Next in list
    }
    node->data.array.elements = malloc(count * sizeof(ASTNode*));
    if (!node->data.array.elements) {
        fprintf(stderr, "Error: Memory allocation failed for array elements\n");
        exit(1);
    }
    node->data.array.count = count;
    temp = elements;
    for (int i = count - 1; i >= 0; i--) {
        if (!temp || !temp->data.array.elements || !temp->data.array.elements[0]) {
            fprintf(stderr, "Error: Invalid element in array list at index %d\n", i);
            exit(1);
        }
        fprintf(stderr, "make_array: copying element[%d]=%p\n", i, temp->data.array.elements[0]);
        if (temp->data.array.elements[0]->type == STR) {
            fprintf(stderr, "  type=STR, string='%s'\n", temp->data.array.elements[0]->data.string);
        }
        node->data.array.elements[i] = temp->data.array.elements[0];
        ASTNode* next = temp->data.array.elements[1];
        free(temp->data.array.elements);
        free(temp);
        temp = next;
    }
    return node;
}

ASTNode* make_string(char* string) {
    if (!string) {
        fprintf(stderr, "Error: Null string in make_string\n");
        exit(1);
    }
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for string\n");
        exit(1);
    }
    node->type = STR;
    node->data.string = strdup(string);
    if (!node->data.string) {
        fprintf(stderr, "Error: Memory allocation failed for string copy\n");
        exit(1);
    }
    fprintf(stderr, "make_string: created node=%p, string='%s'\n", node, node->data.string);
    return node;
}

ASTNode* make_number(char* number) {
    if (!number) {
        fprintf(stderr, "Error: Null number in make_number\n");
        exit(1);
    }
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for number\n");
        exit(1);
    }
    node->type = NUM;
    node->data.string = strdup(number);
    if (!node->data.string) {
        fprintf(stderr, "Error: Memory allocation failed for number copy\n");
        exit(1);
    }
    return node;
}

ASTNode* make_true() {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for true\n");
        exit(1);
    }
    node->type = TRU;
    return node;
}

ASTNode* make_false() {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for false\n");
        exit(1);
    }
    node->type = FALS;
    return node;
}

ASTNode* make_null() {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for null\n");
        exit(1);
    }
    node->type = NUL;
    return node;
}

ASTNode* make_pair(char* key, ASTNode* value) {
    if (!key || !value) {
        fprintf(stderr, "Error: Null key or value in make_pair\n");
        exit(1);
    }
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for pair\n");
        exit(1);
    }
    node->type = OBJ; // Temporary for pair
    node->data.object.keys = malloc(sizeof(char*));
    node->data.object.values = malloc(2 * sizeof(ASTNode*));
    if (!node->data.object.keys || !node->data.object.values) {
        fprintf(stderr, "Error: Memory allocation failed for pair arrays\n");
        exit(1);
    }
    node->data.object.keys[0] = strdup(key);
    node->data.object.values[0] = value;
    node->data.object.values[1] = NULL; // Next pointer
    node->data.object.count = 1;
    return node;
}

ASTNode* make_pair_list(ASTNode* pair, ASTNode* list) {
    if (!pair) {
        fprintf(stderr, "Warning: Null pair in make_pair_list\n");
        return list;
    }
    ASTNode* new_pair = malloc(sizeof(ASTNode));
    if (!new_pair) {
        fprintf(stderr, "Error: Memory allocation failed for pair list\n");
        exit(1);
    }
    new_pair->type = OBJ;
    new_pair->data.object.keys = malloc(sizeof(char*));
    new_pair->data.object.values = malloc(2 * sizeof(ASTNode*));
    if (!new_pair->data.object.keys || !new_pair->data.object.values) {
        fprintf(stderr, "Error: Memory allocation failed for pair list arrays\n");
        exit(1);
    }
    new_pair->data.object.keys[0] = pair->data.object.keys[0];
    new_pair->data.object.values[0] = pair->data.object.values[0];
    new_pair->data.object.values[1] = list; // Link to the rest of the list
    new_pair->data.object.count = 1;
    free(pair->data.object.keys);
    free(pair->data.object.values);
    free(pair);
    return new_pair;
}

ASTNode* make_element_list(ASTNode* element, ASTNode* list) {
    fprintf(stderr, "make_element_list: element=%p, list=%p\n", element, list);
    if (!element) {
        fprintf(stderr, "Warning: Null element in make_element_list\n");
        return list;
    }
    fprintf(stderr, "Element type=%d", element->type);
    if (element->type == STR) {
        fprintf(stderr, ", string='%s'\n", element->data.string ? element->data.string : "null");
    } else if (element->type == NUM) {
        fprintf(stderr, ", number='%s'\n", element->data.string ? element->data.string : "null");
    } else {
        fprintf(stderr, "\n");
    }
    ASTNode* new_element = malloc(sizeof(ASTNode));
    if (!new_element) {
        fprintf(stderr, "Error: Memory allocation failed for element list\n");
        exit(1);
    }
    new_element->type = ARR; // Temporary for list
    new_element->data.array.elements = malloc(2 * sizeof(ASTNode*));
    if (!new_element->data.array.elements) {
        fprintf(stderr, "Error: Memory allocation failed for element list array\n");
        exit(1);
    }
    new_element->data.array.elements[0] = element;
    new_element->data.array.elements[1] = list; // Link to the rest of the list
    new_element->data.array.count = 1;
    fprintf(stderr, "Created element list node: elements[0]=%p\n", element);
    return new_element;
}

void free_ast(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case OBJ:
            for (int i = 0; i < node->data.object.count; i++) {
                if (node->data.object.keys[i]) free(node->data.object.keys[i]);
                free_ast(node->data.object.values[i]);
            }
            free(node->data.object.keys);
            free(node->data.object.values);
            break;
        case ARR:
            for (int i = 0; i < node->data.array.count; i++) {
                free_ast(node->data.array.elements[i]);
            }
            free(node->data.array.elements);
            break;
        case STR:
        case NUM:
            if (node->data.string) free(node->data.string);
            break;
        default:
            break;
    }
    free(node);
}

void print_ast_node(ASTNode* node, int indent) {
    if (!node) {
        for (int i = 0; i < indent; i++) printf("  ");
        printf("NULL\n");
        return;
    }
    for (int i = 0; i < indent; i++) printf("  ");
    switch (node->type) {
        case OBJ:
            printf("OBJECT (count=%d)\n", node->data.object.count);
            for (int i = 0; i < node->data.object.count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("\"%s\": ", node->data.object.keys[i] ? node->data.object.keys[i] : "null");
                print_ast_node(node->data.object.values[i], indent + 2);
            }
            break;
        case ARR:
            printf("ARRAY (count=%d)\n", node->data.array.count);
            for (int i = 0; i < node->data.array.count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("[%d]: ", i);
                if (!node->data.array.elements[i]) {
                    printf("NULL\n");
                    continue;
                }
                char* ptr = (char*)node->data.array.elements[i];
                if (ptr[0] >= 32 && ptr[0] <= 126 && ptr[1] >= 32 && ptr[1] <= 126) {
                    fprintf(stderr, "Error: Invalid ASTNode at array index %d, looks like string '%s'\n", i, ptr);
                    exit(1);
                }
                print_ast_node(node->data.array.elements[i], indent + 2);
            }
            break;
        case STR:
            printf("STRING \"%s\"\n", node->data.string ? node->data.string : "null");
            break;
        case NUM:
            printf("NUMBER %s\n", node->data.string ? node->data.string : "null");
            break;
        case TRU:
            printf("TRUE\n");
            break;
        case FALS:
            printf("FALSE\n");
            break;
        case NUL:
            printf("NULL\n");
            break;
        default:
            printf("UNKNOWN TYPE (%d)\n", node->type);
            break;
    }
}