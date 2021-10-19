#include <stdlib.h>
#include <string.h>

#include "wchar-list.h"
#include "utils.h"

void wchar_list_init(wcharList* list) {
    list->count = 0;
    list->head  = NULL;
}

bool wchar_list_add_node(wcharList* list, const wchar_t* name) {
    size_t    str_len   = wcslen(name);
    size_t    str_bytes = sizeof(wchar_t) * (str_len + 1); // + 1 for NULL terminator
    wcharNode* new_node = malloc(sizeof(wcharNode));
    if(new_node == NULL) {
        return false;
    }
    new_node->name = malloc(str_bytes);
    if(new_node->name == NULL) {
        free(new_node);
        return false;
    }
    memcpy(new_node->name, name, str_bytes);
    new_node->name[str_len] = '\0';
    new_node->next = list->head;
    list->head = new_node;
    list->count++;
    return true;
}

void wchar_node_free(wcharNode* node) {
    free(node->name);
    free(node);
}

void wchar_list_free_all(wcharList *list) {
    wcharNode* cur = list->head;
    while(cur != NULL) {
        wcharNode* next = cur->next;
        wchar_node_free(cur);
        cur = next;
    }
    list->head  = NULL;
    list->count = 0;
}

wcharNode* wchar_list_remove_node(wcharList* list, size_t index) {
    // can't remove past the end of a list
    if(index >= list->count) {
        return NULL;
    }
    // take advantage of the fact the the first entry of structs
    // are allways guaranteed to be offset zero in a C struct and
    // if we place the same type in the same place we can avoid
    // having a conditional check for handling handling index 0
    // for the head condition this only works if structs share the
    // same type for the first entry, anything else is UB.
    wcharNode* prev = (wcharNode*)list;
    wcharNode* cur  = list->head;
    size_t    i     = 0;
    while(cur != NULL) {
        if(i == index) {
            break;
        }
        prev = cur;
        cur  = cur->next;
        i++;
    }
    prev->next = cur->next;
    list->count--;
    return cur;
}

wcharNode* wchar_list_remove_random_node(wcharList* list) {
    uint64_t stop_index = rand64_less_than(list->count);
    return wchar_list_remove_node(list, stop_index);
}