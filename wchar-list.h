
#ifndef name_LIST_H
#define name_LIST_H

#include <stddef.h>
#include <stdbool.h>

typedef struct wchar_node_s wcharNode;

struct wchar_node_s {
    wcharNode* next;
    wchar_t*  name;
};

typedef struct wchar_list_s {
    wcharNode* head;
    size_t    count;
} wcharList;

void wchar_list_init(wcharList* list);
bool wchar_list_add_node(wcharList* list, const wchar_t* name);
void wchar_list_free_all(wcharList *list);
wcharNode* wchar_list_remove_node(wcharList* list, size_t index);
wcharNode* wchar_list_remove_random_node(wcharList* list);
void wchar_node_free(wcharNode* node);

#endif