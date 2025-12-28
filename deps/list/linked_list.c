#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct item {
    int type;
    void* value;
    struct item* prev;
    struct item* next;
} item;

typedef struct list {
    item* first;
    item* last;
    int length;
} list;

// helper functions:
item* create_item(list *l, int type, void* value ) {
}

list* list_new() {
    list* l = malloc(sizeof(list));
    if (l == NULL) {
        fprintf(stderr, "Out Of Memory!");
        exit(1);
    }

    l->last = NULL;
    l->length = 0;
    l->first = NULL;

    return l;
}

item* list_get(list *l, int index) {
    if (index >= l->length) {
        fprintf(stderr, "Index Out Of Bounds!");
        exit(1);
    }
    item* c_item = l->first;
    while (c_item != NULL && c_item->index != index) {
        c_item = c_item->next;
    }
    return c_item;
}

list* list_add(list *l, item *item) {
    item->index = l->length;
    item->next = NULL;
    item->prev = l->last;

    if (l->last != NULL) {
        l->last->next = item;
    } else {
        l->first = item;
    }

    l->last = item;
    l->length++;

    return l;
}

list* list_addall(list *list_a, list *list_b) {
    item* elem = list_b->first;
    while (elem->next != NULL) {
        elem->index += list_a->length;
        elem = elem->next;
    }
    list_b->last->index += list_a->length;
    list_a->last->next = list_b->first;
    list_a->last = list_b->last;
    list_a->length += list_b->length;

    return list_a;
}

list* list_clear(list* l) {
    for (int i=0; i < l->length; i++) {
        if (l->last != NULL){
            item* ele = l->last->prev;
            free(l->last);
            l->last = ele;
        }
    }

    l->length = 0;
    l->first = NULL;

    return l;
}

//[1] idx = 0

item* list_pop(list *l, int index) {
    if (index >= l->length) {
        fprintf(stderr, "Index Out Of Bounds!");
        exit(1);
    }
    item* curr = l->first;
    if (index == l->length-1) {
        curr = l->last;
    } else {
        curr = list_get(l, index);
    }
    item* next = curr->next;
    item* prev = curr->prev;
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
        next->index = index;
    }
    l->length--;
    item* ele = next->next;
    for (int i=next->index+1; i<l->length; i++){
        ele->index = i;
        ele = ele->next;
    }
    l->last = ele;
    return curr;
}
