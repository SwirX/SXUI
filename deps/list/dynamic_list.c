#include "dynamic_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list {
    void** data;
    size_t capacity;
    size_t length;
};

static void _OOM_ERR(void) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
}

static int _check_bounds(const list* l, size_t index) {
    if (index >= l->length) return -1;
    return 0;
}

list* list_new(void) {
    list* l = malloc(sizeof(list));
    if (!l) _OOM_ERR();
    
    l->capacity = 1;
    l->data = malloc(l->capacity * sizeof(void*));
    if (!l->data) _OOM_ERR();
    
    l->length = 0;
    return l;
}

void list_free(list* l) {
    if (!l) return;
    free(l->data);
    free(l);
}

void list_free_deep(list* l, void (*free_element)(void*)) {
    if (!l) return;
    if (free_element) {
        for (size_t i = 0; i < l->length; i++) {
            free_element(l->data[i]);
        }
    }
    free(l->data);
    free(l);
}

int list_add(list* l, void* element) {
    if (!l) return -1;
    
    if (l->length == l->capacity) {
        size_t new_cap = l->capacity * 2;
        void** new_data = realloc(l->data, new_cap * sizeof(void*));
        if (!new_data) return -1;  // Let caller handle OOM
        l->data = new_data;
        l->capacity = new_cap;
    }
    
    l->data[l->length++] = element;
    return 0;
}

// learned memmove just for this (should work if everything is alright)
int list_insert(list* l, size_t index, void* element) {
    if (!l) return -1;
    if (index > l->length) return -1;
    
    if (l->length == l->capacity) {
        size_t new_cap = l->capacity * 2;
        void** new_data = realloc(l->data, new_cap * sizeof(void*));
        if (!new_data) return -1;
        l->data = new_data;
        l->capacity = new_cap;
    }
    
    memmove(&l->data[index + 1], &l->data[index], 
            (l->length - index) * sizeof(void*));
    
    l->data[index] = element;
    l->length++;
    return 0;
}

void list_clear(list* l) {
    if (!l) return;
    l->length = 0;
}

void* list_get(const list* l, size_t index) {
    if (!l || _check_bounds(l, index) < 0) return NULL;
    return l->data[index];
}

int list_set(list* l, size_t index, void* element) {
    if (!l || _check_bounds(l, index) < 0) return -1;
    l->data[index] = element;
    return 0;
}

void* list_pop(list* l) {
    if (!l || l->length == 0) return NULL;
    void* item = l->data[--l->length];
    l->data[l->length] = NULL;
    return item;
}

void* list_remove_at(list* l, size_t index) {
    if (!l || _check_bounds(l, index) < 0) return NULL;
    
    void* item = l->data[index];
    memmove(&l->data[index], &l->data[index + 1], 
            (l->length - index - 1) * sizeof(void*));
    
    l->length--;
    l->data[l->length] = NULL;
    return item;
}

size_t list_length(const list* l) {
    return l ? l->length : 0;
}

size_t list_capacity(const list* l) {
    return l ? l->capacity : 0;
}

int list_is_empty(const list* l) {
    return !l || l->length == 0;
}

int list_addall(list* dest, const list* src) {
    if (!dest || !src) return -1;
    if (src->length == 0) return 0;
    
    size_t needed = dest->length + src->length;
    if (needed > dest->capacity) {
        size_t new_cap = dest->capacity;
        while (new_cap < needed) new_cap *= 2;
        void** new_data = realloc(dest->data, new_cap * sizeof(void*));
        if (!new_data) return -1;
        dest->data = new_data;
        dest->capacity = new_cap;
    }
    
    memcpy(&dest->data[dest->length], src->data, src->length * sizeof(void*));
    dest->length += src->length;
    return 0;
}

int list_reverse(list* l) {
    if (!l) return -1;
    for (size_t i = 0; i < l->length / 2; i++) {
        void* tmp = l->data[i];
        l->data[i] = l->data[l->length - 1 - i];
        l->data[l->length - 1 - i] = tmp;
    }
    return 0;
}

size_t list_index_of(const list* l, const void* element) {
    if (!l) return (size_t)-1;
    for (size_t i = 0; i < l->length; i++) {
        if (l->data[i] == element) return i;
    }
    return (size_t)-1;
}

list* list_slice(const list* l, size_t start, size_t end) {
    if (!l || start > end || start >= l->length) return NULL;
    
    if (end > l->length) end = l->length;  // Exclusive end is idiomatic
    
    size_t size = end - start;
    list* slice = list_new();  // Uses proper constructor
    if (!slice) return NULL;
    
    // Bulk allocate exactly what we need
    void** new_data = malloc(size * sizeof(void*));
    if (!new_data) {
        list_free(slice);
        return NULL;
    }
    
    memcpy(new_data, &l->data[start], size * sizeof(void*));
    free(slice->data);  // Free the 1-element placeholder
    slice->data = new_data;
    slice->length = size;
    slice->capacity = size;
    
    return slice;
}

int list_find(list* results, const list* l, int (*predicate)(const void*)) {
    if (!results || !l || !predicate) return -1;
    
    for (size_t i = 0; i < l->length; i++) {
        if (predicate(l->data[i])) {
            if (list_add(results, l->data[i]) < 0) return -1;  // Propagate errors
        }
    }
    return 0;
}

void list_print(const list* l, void (*print_fn)(const void*)) {
    if (!l) return;
    printf("[%zu] { ", l->length);
    for (size_t i = 0; i < l->length; i++) {
        if (print_fn) print_fn(l->data[i]);
        else printf("%p", l->data[i]);  // Fallback: print address
        if (i < l->length - 1) printf(", ");
    }
    printf(" }\n");
}