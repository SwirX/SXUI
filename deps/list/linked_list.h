#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct item item;
typedef struct list list;

list* list_new();
item* list_pop(list *l, int index);
list* list_add(list *l, item *obj);
list* list_addall(list *list_a, list *list_b);
list* list_clear(list *l);
int list_len(list *l);
list* list_reverse(list *l);

#endif
