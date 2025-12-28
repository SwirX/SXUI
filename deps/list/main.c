#include "dynamic_list.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    list* l = list_new();
    
    // Add elements
    int* a = malloc(sizeof(int)); *a = 42;
    int* b = malloc(sizeof(int)); *b = 17;
    list_add(l, a);
    list_add(l, b);
    
    // Print with custom function
    void print_int(const void* p) { printf("%d", *(int*)p); }
    list_print(l, print_int);  // Output: [2] { 42, 17 }
    
    // Pop and cleanup
    int* popped = list_pop(l);
    free(popped);  // YOU own the memory!
    
    // Deep free remaining elements
    list_free_deep(l, free);  // Frees the list AND the ints
    
    return 0;
}