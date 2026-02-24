#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define STARTING_BUCKETS 8
#define MAX_KEY_SIZE 20

typedef struct Hashmap{
    void **items;
    uint8_t *index_used;
    int size;
}Hashmap;

int hash_function(char *name){
    char *name_copy = name;
    // TODO: implement a better hash function
    int c = *name_copy++;
    int hash = 0;
    while(c){
        hash += c;
        c = *name_copy++;
        printf(" %d ", c);
    }

    // TODO: handle collision here?
    
    hash %= STARTING_BUCKETS;
    printf("\nConverting %s to %d\n", name, hash);
    return hash;
}

Hashmap *Hashmap_new(){
    Hashmap *hashmap = malloc(sizeof(Hashmap));
    hashmap -> items = malloc(sizeof(void *) * STARTING_BUCKETS);
    hashmap -> index_used = malloc(sizeof(uint8_t) * STARTING_BUCKETS);
    hashmap -> size = 0;

    return hashmap;
}

void Hashmap_free(Hashmap * hashmap){
    free(hashmap -> items);
    hashmap -> items = NULL;
    free(hashmap -> index_used);
    hashmap -> index_used = NULL;
    free(hashmap);
    hashmap = NULL;

    return;
}

void Hashmap_set(Hashmap *hashmap, char *name, void *value_addr){
    int index = hash_function(name);

    // TODO should handle collision
    hashmap -> items[index] = value_addr;
    hashmap -> index_used[index] = 1;

    return;
}

void* Hashmap_get(Hashmap *hashmap, char *name){
    int index = hash_function(name);

    return hashmap -> items[index];
}

void Hashmap_delete(Hashmap *hashmap, char *name){
    int index = hash_function(name);
    hashmap -> items[index] = NULL;
    
    return;
}

// Some tests
int main() {
    Hashmap *h = Hashmap_new();

    // basic get/set functionality
    int a = 5;
    float b = 7.2;
    Hashmap_set(h, "item a", &a);
    Hashmap_set(h, "item b", &b);
    assert(Hashmap_get(h, "item a") == &a);
    assert(Hashmap_get(h, "item b") == &b);

    // using the same key should override the previous value
    int c = 20;
    Hashmap_set(h, "item a", &c);
    assert(Hashmap_get(h, "item a") == &c);

    // basic delete functionality
    Hashmap_delete(h, "item a");
    assert(Hashmap_get(h, "item a") == NULL);

    // handle collisions correctly
    // note: this doesn't necessarily test expansion
    int i, n = STARTING_BUCKETS * 10, ns[n];
    char key[MAX_KEY_SIZE];
    for (i = 0; i < n; i++) {
        ns[i] = i;
        sprintf(key, "item %d", i);
        Hashmap_set(h, key, &ns[i]);
    }
    for (i = 0; i < n; i++) {
        sprintf(key, "item %d", i);
        assert(Hashmap_get(h, key) == &ns[i]);
    }

    Hashmap_free(h);
    printf("NICE\n");
}
