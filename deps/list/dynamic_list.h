#pragma once
#include <stddef.h>

typedef struct list list;

// Constructor/Destructor
list* list_new(void);
void  list_free(list* l);
void  list_free_deep(list* l, void (*free_element)(void*));

// Basic ops (normal list shi pt1)
int  list_add(list* l, void* element);
int  list_insert(list* l, size_t index, void* element);
void list_clear(list* l);

// Accessors (normal list shi pt2)
void* list_get(const list* l, size_t index);
int   list_set(list* l, size_t index, void* element);
void* list_pop(list* l);
void* list_remove_at(list* l, size_t index);

// Queries (instead of accessing them directly ig)
size_t list_length(const list* l);
size_t list_capacity(const list* l);
int    list_is_empty(const list* l);

// Bulk ops (i feel like O(n) is the best i could make them)
int  list_addall(list* dest, const list* src);
int  list_reverse(list* l);
list* list_slice(const list* l, size_t start, size_t end); // is this bulk op?

// Searching functions (still thinking of new ones)
size_t list_index_of(const list* l, const void* element);
int   list_find(list* results, const list* l, int (*predicate)(const void*));
void*  list_find_first(const list* l, int (*predicate)(const void*));

// Utilities
void list_print(const list* l, void (*print_fn)(const void*));