#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define STARTING_CAPACITY 8
#define EXPAND_RATIO 2

/*
I know this repo is about network stuff and this is not network related, but I don't want to create a new repo just for this
so here it is - writing a python-like list in C.
*/

typedef struct DA {
    void **items;
    int size; // indicates how many items there are in the items array
    int capacity; // this is the memory size, everytime we reach capacity we double (our EXPAND_RATIO) the memory size and relocate
} DA;


DA* DA_new (void) {
    // Allocate and return a new dynamic array
    DA *dynamic_array = malloc(sizeof(DA));

    dynamic_array -> capacity = STARTING_CAPACITY;
    dynamic_array -> size = 0;
    dynamic_array -> items = malloc(sizeof(void*) * dynamic_array->capacity);

    return dynamic_array;
}


// Return the number of items in the dynamic array
int DA_size(DA *da) {
    return da->size;
}

void DA_push (DA* da, void* x) {
    // Expand the memory if we reached the capacity limit
    if(da->size == da->capacity){
        da->items = realloc(da->items, sizeof(void*) * da->capacity * EXPAND_RATIO);

        // NOTE: ENOMEM means insufficient memory is available 
        if(da->items == NULL && errno == ENOMEM){
            perror("realloc failed, insufficient memory.");
            return;
        }

        da->capacity *= EXPAND_RATIO;
    }
    
    // This causes buffer overflow "Void * arithmetic is not defined" - https://stackoverflow.com/questions/4019671/can-i-do-arithmetic-on-void-pointers-in-c
    /*
    void** next_item_ptr = da->items + da->size;
    *next_item_ptr = x;
    da->size += 1;
    */

    da->items[da->size++] = x;

    return;
}

void* DA_pop(DA *da) {
    
    if(da->size <= 0){
        printf("No items to pop, the list is empty\n");
        return NULL;
    }
    return da->items[--da->size];
}

void DA_set(DA *da, void *x, int i) {
    // Check if index is valid
    if(i < 0 || i >= (da -> size)){
        printf("Set index out of range\n");
        return;
    }

    da->items[i] = x;
    return;
}

void* DA_get(DA *da, int i) {
    // Check if index is valid
    if(i < 0 || i >= (da -> size)){
        printf("Get index out of range\n");
        return NULL;
    }

    return  da->items[i];
}


void DA_free(DA *da) {
    free(da->items);
    da->items = NULL;
    free(da);
    da = NULL;

    return;
}

// Some test to see if it works
int main() {
    DA* da = DA_new();

    assert(DA_size(da) == 0);

    // Basic push and pop test
    int x = 5;
    float y = 12.4;
    DA_push(da, &x);
    DA_push(da, &y);
    assert(DA_size(da) == 2);

    assert(DA_pop(da) == &y);
    assert(DA_size(da) == 1);

    assert(DA_pop(da) == &x);
    assert(DA_size(da) == 0);
    assert(DA_pop(da) == NULL);

    // Basic set/get test
    DA_push(da, &x);
    DA_set(da, &y, 0);
    assert(DA_get(da, 0) == &y);
    DA_pop(da);
    assert(DA_size(da) == 0);

    // Expansion test
    DA *da2 = DA_new(); // Use another DA to show it doesn't get overriden
    DA_push(da2, &x);
    int i, n = 100 * STARTING_CAPACITY, arr[n];
    for (i = 0; i < n; i++) {
      arr[i] = i;
      DA_push(da, &arr[i]);
    }
    assert(DA_size(da) == n);
    for (i = 0; i < n; i++) {
      assert(DA_get(da, i) == &arr[i]);
    }
    for (; n; n--)
      DA_pop(da);
    assert(DA_size(da) == 0);
    assert(DA_pop(da2) == &x); // This will fail if da doesn't expand

    DA_free(da);
    DA_free(da2);
    printf("OK NICE\n");
}
