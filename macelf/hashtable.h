#ifndef _INCL_HASHTABLE_H_
#define _INCL_HASHTABLE_H_

#include <stdint.h>

typedef struct HashTable HashTable;
typedef uint32_t (*HashTable_HashFn)(const void *key, void *data);
typedef int (*HashTable_KeyMatchFn)(const void *a, const void *b, void *data);
typedef void (*HashTable_NukeFn)(const void *key, const void *value, void *data);

HashTable *hash_create(void *data, const HashTable_HashFn hashfn,
                       const HashTable_KeyMatchFn keymatchfn,
                       const HashTable_NukeFn nukefn);

void hash_destroy(HashTable *table);
int hash_insert(HashTable *table, const void *key, const void *value);
int hash_remove(HashTable *table, const void *key);
int hash_find(const HashTable *table, const void *key, const void **_value);
int hash_iter(const HashTable *table, const void *key, const void **_value, void **iter);
int hash_iter_keys(const HashTable *table, const void **_key, void **iter);

uint32_t hash_hash_string(const void *sym, void *unused);
int hash_keymatch_string(const void *a, const void *b, void *unused);

#endif

// end of hashtable.h ...

