#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode* make_object(ASTNode* members) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = OBJ;
    int count = 0;
    ASTNode* temp = members;
    while (temp) {
        count++;
        temp = temp->data.object.values[1]; // Next in list
    }
    node->data.object.keys = malloc(count * sizeof(char*));
    node->data.object.values = malloc(count * sizeof(ASTNode*));
    node->data.object.count = count;
    temp = members;
    for (int i = count - 1; i >= 0; i--) {
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
    node->type = ARR;
    int count = 0;
    ASTNode* temp = elements;
    while (temp) {
        count++;
        temp = temp->data.array.elements[1];
    }
    node->data.array.elements = malloc(count * sizeof(ASTNode*));
    node->data.array.count = count;
    temp = elements;
    for (int i = count - 1; i >= 0; i--) {
        node->data.array.elements[i] = temp->data.array.elements[0];
        ASTNode* next = temp->data.array.elements[1];
        free(temp->data.array.elements);
        free(temp);
        temp = next;
    }
    return node;
}

ASTNode* make_string( char* string) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = STR;
    node->data.string = string;
    return node;
}

ASTNode* make_number(char* number) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NUM;
    node->data.string = number;
    return node;
}

ASTNode* make_true() {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = TRU;
    return node;
}

ASTNode* make_false() {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = FALS;
    return node;
}

ASTNode* make_null() {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NUL;
    return node;
}

ASTNode* make_pair(char* key, ASTNode* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = OBJ; // Temporary for pair
    node->data.object.keys = malloc(sizeof(char*));
    node->data.object.values = malloc(2 * sizeof(ASTNode*));
    node->data.object.keys[0] = key;
    node->data.object.values[0] = value;
    node->data.object.values[1] = NULL; // Next pointer
    node->data.object.count = 1;
    return node;
}

ASTNode* make_pair_list(ASTNode* pair, ASTNode* list) {
    pair->data.object.values[1] = list;
    return pair;
}

ASTNode* make_element_list(ASTNode* element, ASTNode* list) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = ARR; // Temporary for list
    node->data.array.elements = malloc(2 * sizeof(ASTNode*));
    node->data.array.elements[0] = element;
    node->data.array.elements[1] = list;
    node->data.array.count = 1;
    return node;
}

void free_ast(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case OBJ:
            for (int i = 0; i < node->data.object.count; i++) {
                free(node->data.object.keys[i]);
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
            free(node->data.string);
            break;
        default:
            break;
    }
    free(node);
}

void print_ast_node(ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    switch (node->type) {
        case OBJ:
            printf("OBJECT\n");
            for (int i = 0; i < node->data.object.count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("\"%s\": ", node->data.object.keys[i]);
                print_ast_node(node->data.object.values[i], indent + 2);
            }
            break;
        case ARR:
            printf("ARRAY\n");
            for (int i = 0; i < node->data.array.count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("[%d]: ", i);
                print_ast_node(node->data.array.elements[i], indent + 2);
            }
            break;
        case STR:
            printf("STRING \"%s\"\n", node->data.string);
            break;
        case NUM:
            printf("NUMBER %s\n", node->data.string);
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
    }
}