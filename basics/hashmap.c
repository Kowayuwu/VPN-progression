#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Another project unrelated to network. This is a SIMPLE hashmap implementation using C, it uses djb2 for hashing and open addressing + quadratic probing
to handle collision. It is capable of get, set, delete, and resize but only allows string as the key.
*/

#define STARTING_BUCKETS 8
#define MAX_KEY_SIZE 20
#define INIT_HASH_NUM 5381
#define CONSEC_COLLISION_LIMIT 10
#define ERR_NO_VALID_ITEM_INDEX -1
#define RESIZE_RATIO 2

typedef uint32_t Hash;

typedef struct Hashmap_item{
    char *key;
    void *value;
    Hash hash;
}Hashmap_item;

typedef struct Hashmap{
    Hashmap_item **items;
    uint32_t size;
}Hashmap;

// Implement hash function using djb2 algorithm
Hash hash_function(char *key){
    char *name_copy = key;
    Hash hash = INIT_HASH_NUM;

    int c;
    while((c = *name_copy++)){
        hash = ((hash << 5) + hash) + c; // this is equal to hash * 33 + c, apparently the number 33 is just better than other numbers
    }

    //printf("\nConverting %s to %d\n", key, hash);
    return hash;
}

Hashmap *Hashmap_new(uint32_t size){
    Hashmap *hashmap = malloc(sizeof(Hashmap));
    hashmap -> items = calloc(size, sizeof(Hashmap_item));
    hashmap -> size = size;

    return hashmap;
}

/*
Increase hashmap size by [ RESIZE_RATIO ], not doing anythihng fancy - just reallocate everything
*/
void *Hashmap_resize(Hashmap **old_hashmap){
    Hashmap *new_hashmap = Hashmap_new( ((*old_hashmap)->size) * RESIZE_RATIO);

    for(int i=0; i<(*old_hashmap)->size; i++){
        if((*old_hashmap)->items[i] == NULL) continue;
        Hashmap_set(new_hashmap, (*old_hashmap)->items[i]->key, (*old_hashmap)->items[i]->value);
    }

    // see if i can swap it in place
    Hashmap *old_hashmap_ptr_copy = *old_hashmap;
    *old_hashmap = new_hashmap;
    Hashmap_free(old_hashmap_ptr_copy);

    return;
    //return new_hashmap;
}

/*
Quadratic probing function
*/
int collision_probe_function(int base){
    return base*base;
}

/*
Get an item that exists in the item list (or at least we EXCPECT it to exist)
*/
int get_exist_item_index(Hashmap * hashmap, char *key){
    int collision_count = 0;
    int offset_base = 1;
    int index = hash_function(key) % (hashmap -> size);

    while(collision_count < CONSEC_COLLISION_LIMIT){
        collision_count++;

        if(hashmap -> items[index] == NULL){
            index = (index + collision_probe_function(offset_base)) % (hashmap -> size);
            offset_base ++;
            continue;
        }
            
        if(strcmp(hashmap -> items[index] -> key, key) == 0){
            return index;
        }

    }

    return ERR_NO_VALID_ITEM_INDEX;

}

/*
The item could be new or already exist in the item list.
Given a hashtable and the item, use quadratic probing to return a valid index that is empty if it's new.
Or return the index if the item is in the list.

If consecutive collision happens over [CONSEC_COLLISION_LIMIT] of time, then it triggers a hashtable resize
*/
int get_setter_index(Hashmap * hashmap, char *key){

    // There's no way (or I couldn't figure out now) to know whether a key is new or not before finding an empty spot
    int index_if_exist = get_exist_item_index(hashmap, key);
    if(index_if_exist != ERR_NO_VALID_ITEM_INDEX){
        return index_if_exist;
    }


    int offset_base = 1;
    int index = hash_function(key) % (hashmap -> size);
    uint8_t collision_count = 0;

    uint8_t need_resize = 0;

    while((hashmap -> items[index]) != NULL){
        index = (index + collision_probe_function(offset_base)) % (hashmap -> size);

        collision_count ++;
        offset_base ++;

        if(collision_count > CONSEC_COLLISION_LIMIT){
            // resize


            // re assign


            // if fail then quit?
            need_resize = 1;
            break;
        }
    }

    // idk handle resize here?
    if(need_resize) return ERR_NO_VALID_ITEM_INDEX;


    return index;
}



void Hashmap_free(Hashmap * hashmap){
    // TODO: free all items individually and their string key
    for(int i = 0; i<hashmap->size; i++){
        if(hashmap->items[i] != NULL){
            free(hashmap->items[i]->key);
            hashmap->items[i]->key = NULL;
            free(hashmap->items[i]);
            hashmap->items[i] = NULL;

        }
    }

    free(hashmap -> items);
    hashmap -> items = NULL;
    free(hashmap);
    hashmap = NULL;

    return;
}

void Hashmap_set(Hashmap *hashmap, char *key, void *value_addr){
    int index = get_setter_index(hashmap, key);

    if(index == ERR_NO_VALID_ITEM_INDEX){
        printf("Failed to get valid index for key [ %s ] during set()\n", key);

        // TODO: Do resize here and try to insert again or??



        
        return;
    }

    // New key
    if(hashmap -> items[index] == NULL){
        Hash hash = hash_function(key);
        Hashmap_item *item = malloc(sizeof(Hashmap_item));
        item -> hash = hash;
        item -> key = strdup(key);
        item -> value = value_addr;
        hashmap -> items[index] = item;
        return;
    }

    // Existing key
    hashmap -> items[index] -> value = value_addr;

    return;
}

void* Hashmap_get(Hashmap *hashmap, char *key){
    int collision_count = 0;
    int offset_base = 1;

    int index = get_exist_item_index(hashmap, key);

    if(index == ERR_NO_VALID_ITEM_INDEX){
        printf("Failed to loacate item for key [ %s ] during get()\n", key);
        return NULL;
    }
        

    return hashmap -> items[index] -> value;
}

void Hashmap_delete(Hashmap *hashmap, char *key){
    int index = get_exist_item_index(hashmap, key);

    if(index == ERR_NO_VALID_ITEM_INDEX){
        printf("Failed to loacate item for key [ %s ] during deletion()\n", key);
        return;
    }

    free(hashmap->items[index]->key);
    hashmap->items[index]->key = NULL;

    free(hashmap->items[index]);
    hashmap->items[index] = NULL;

    
    return;
}

// Some tests
int main() {
    Hashmap *h = Hashmap_new(STARTING_BUCKETS);

    // Basic get/set functionality
    int a = 5;
    float b = 7.2;
    Hashmap_set(h, "item a", &a);
    Hashmap_set(h, "item b", &b);
    assert(Hashmap_get(h, "item a") == &a);
    assert(Hashmap_get(h, "item b") == &b);

    // Using the same key should override the previous value
    int c = 20;
    Hashmap_set(h, "item a", &c);
    assert(Hashmap_get(h, "item a") == &c);

    // Basic delete functionality
    Hashmap_delete(h, "item a");
    assert(Hashmap_get(h, "item a") == NULL);

    // Handle collisions correctly
    // Note: this doesn't necessarily test expansion if linked list approach is used for collision handling
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



/*
Reference, this is how cpython implement resizing
*/

// /*
// Restructure the table by allocating a new table and reinserting all
// items again.  When entries have been deleted, the new table may
// actually be smaller than the old one.
// If a table is split (its keys and hashes are shared, its values are not),
// then the values are temporarily copied into the table, it is resized as
// a combined table, then the me_value slots in the old table are NULLed out.
// After resizing, a table is always combined.

// This function supports:
//  - Unicode split -> Unicode combined or Generic
//  - Unicode combined -> Unicode combined or Generic
//  - Generic -> Generic
// */
// static int
// dictresize(PyDictObject *mp,
//            uint8_t log2_newsize, int unicode)
// {
//     assert(can_modify_dict(mp));

//     PyDictKeysObject *oldkeys, *newkeys;
//     PyDictValues *oldvalues;

//     if (log2_newsize >= SIZEOF_SIZE_T*8) {
//         PyErr_NoMemory();
//         return -1;
//     }
//     assert(log2_newsize >= PyDict_LOG_MINSIZE);

//     oldkeys = mp->ma_keys;
//     oldvalues = mp->ma_values;

//     if (!DK_IS_UNICODE(oldkeys)) {
//         unicode = 0;
//     }

//     ensure_shared_on_resize(mp);
//     /* NOTE: Current odict checks mp->ma_keys to detect resize happen.
//      * So we can't reuse oldkeys even if oldkeys->dk_size == newsize.
//      * TODO: Try reusing oldkeys when reimplement odict.
//      */

//     /* Allocate a new table. */
//     newkeys = new_keys_object(log2_newsize, unicode);
//     if (newkeys == NULL) {
//         return -1;
//     }
//     // New table must be large enough.
//     assert(newkeys->dk_usable >= mp->ma_used);

//     Py_ssize_t numentries = mp->ma_used;

//     if (oldvalues != NULL) {
//         LOCK_KEYS(oldkeys);
//         PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
//         /* Convert split table into new combined table.
//          * We must incref keys; we can transfer values.
//          */
//         if (newkeys->dk_kind == DICT_KEYS_GENERAL) {
//             // split -> generic
//             PyDictKeyEntry *newentries = DK_ENTRIES(newkeys);

//             for (Py_ssize_t i = 0; i < numentries; i++) {
//                 int index = get_index_from_order(mp, i);
//                 PyDictUnicodeEntry *ep = &oldentries[index];
//                 assert(oldvalues->values[index] != NULL);
//                 newentries[i].me_key = Py_NewRef(ep->me_key);
//                 newentries[i].me_hash = unicode_get_hash(ep->me_key);
//                 newentries[i].me_value = oldvalues->values[index];
//             }
//             build_indices_generic(newkeys, newentries, numentries);
//         }
//         else { // split -> combined unicode
//             PyDictUnicodeEntry *newentries = DK_UNICODE_ENTRIES(newkeys);

//             for (Py_ssize_t i = 0; i < numentries; i++) {
//                 int index = get_index_from_order(mp, i);
//                 PyDictUnicodeEntry *ep = &oldentries[index];
//                 assert(oldvalues->values[index] != NULL);
//                 newentries[i].me_key = Py_NewRef(ep->me_key);
//                 newentries[i].me_value = oldvalues->values[index];
//             }
//             build_indices_unicode(newkeys, newentries, numentries);
//         }
//         UNLOCK_KEYS(oldkeys);
//         set_keys(mp, newkeys);
//         dictkeys_decref(oldkeys, IS_DICT_SHARED(mp));
//         set_values(mp, NULL);
//         if (oldvalues->embedded) {
//             assert(oldvalues->embedded == 1);
//             assert(oldvalues->valid == 1);
//             invalidate_and_clear_inline_values(oldvalues);
//         }
//         else {
//             free_values(oldvalues, IS_DICT_SHARED(mp));
//         }
//     }
//     else {  // oldkeys is combined.
//         if (oldkeys->dk_kind == DICT_KEYS_GENERAL) {
//             // generic -> generic
//             assert(newkeys->dk_kind == DICT_KEYS_GENERAL);
//             PyDictKeyEntry *oldentries = DK_ENTRIES(oldkeys);
//             PyDictKeyEntry *newentries = DK_ENTRIES(newkeys);
//             if (oldkeys->dk_nentries == numentries) {
//                 memcpy(newentries, oldentries, numentries * sizeof(PyDictKeyEntry));
//             }
//             else {
//                 PyDictKeyEntry *ep = oldentries;
//                 for (Py_ssize_t i = 0; i < numentries; i++) {
//                     while (ep->me_value == NULL)
//                         ep++;
//                     newentries[i] = *ep++;
//                 }
//             }
//             build_indices_generic(newkeys, newentries, numentries);
//         }
//         else {  // oldkeys is combined unicode
//             PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
//             if (unicode) { // combined unicode -> combined unicode
//                 PyDictUnicodeEntry *newentries = DK_UNICODE_ENTRIES(newkeys);
//                 if (oldkeys->dk_nentries == numentries && mp->ma_keys->dk_kind == DICT_KEYS_UNICODE) {
//                     memcpy(newentries, oldentries, numentries * sizeof(PyDictUnicodeEntry));
//                 }
//                 else {
//                     PyDictUnicodeEntry *ep = oldentries;
//                     for (Py_ssize_t i = 0; i < numentries; i++) {
//                         while (ep->me_value == NULL)
//                             ep++;
//                         newentries[i] = *ep++;
//                     }
//                 }
//                 build_indices_unicode(newkeys, newentries, numentries);
//             }
//             else { // combined unicode -> generic
//                 PyDictKeyEntry *newentries = DK_ENTRIES(newkeys);
//                 PyDictUnicodeEntry *ep = oldentries;
//                 for (Py_ssize_t i = 0; i < numentries; i++) {
//                     while (ep->me_value == NULL)
//                         ep++;
//                     newentries[i].me_key = ep->me_key;
//                     newentries[i].me_hash = unicode_get_hash(ep->me_key);
//                     newentries[i].me_value = ep->me_value;
//                     ep++;
//                 }
//                 build_indices_generic(newkeys, newentries, numentries);
//             }
//         }

//         set_keys(mp, newkeys);

//         if (oldkeys != Py_EMPTY_KEYS) {
// #ifdef Py_REF_DEBUG
//             _Py_DecRefTotal(_PyThreadState_GET());
// #endif
//             assert(oldkeys->dk_kind != DICT_KEYS_SPLIT);
//             assert(oldkeys->dk_refcnt == 1);
//             free_keys_object(oldkeys, IS_DICT_SHARED(mp));
//         }
//     }

//     STORE_KEYS_USABLE(mp->ma_keys, mp->ma_keys->dk_usable - numentries);
//     STORE_KEYS_NENTRIES(mp->ma_keys, numentries);
//     ASSERT_CONSISTENT(mp);
//     return 0;
// }
