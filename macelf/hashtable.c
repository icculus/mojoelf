/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hashtable.h"

typedef struct HashItem
{
    const void *key;
    const void *value;
    struct HashItem *next;
} HashItem;

struct HashTable
{
    HashItem **table;
    uint32_t table_len;
    void *data;
    HashTable_HashFn hash;
    HashTable_KeyMatchFn keymatch;
    HashTable_NukeFn nuke;
};

static inline uint32_t calc_hash(const HashTable *table, const void *key)
{
    return table->hash(key, table->data) & (table->table_len-1);
} // calc_hash

int hash_find(const HashTable *table, const void *key, const void **_value)
{
    HashItem *i;
    void *data = table->data;
    const uint32_t hash = calc_hash(table, key);
    HashItem *prev = NULL;
    for (i = table->table[hash]; i != NULL; i = i->next)
    {
        if (table->keymatch(key, i->key, data))
        {
            if (_value != NULL)
                *_value = i->value;

            // Matched! Move to the front of list for faster lookup next time.
            if (prev != NULL)
            {
                assert(prev->next == i);
                prev->next = i->next;
                i->next = table->table[hash];
                table->table[hash] = i;
            } // if

            return 1;
        } // if

        prev = i;
    } // for

    return 0;
} // hash_find

int hash_iter(const HashTable *table, const void *key,
              const void **_value, void **iter)
{
    HashItem *item = *iter;
    if (item == NULL)
        item = table->table[calc_hash(table, key)];
    else
        item = item->next;

    while (item != NULL)
    {
        if (table->keymatch(key, item->key, table->data))
        {
            *_value = item->value;
            *iter = item;
            return 1;
        } // if
        item = item->next;
    } // while

    // no more matches.
    *_value = NULL;
    *iter = NULL;
    return 0;
} // hash_iter

int hash_iter_keys(const HashTable *table, const void **_key, void **iter)
{
    HashItem *item = *iter;
    int idx = 0;

    if (item != NULL)
    {
        const HashItem *orig = item;
        item = item->next;
        if (item == NULL)
            idx = calc_hash(table, orig->key) + 1;
    } // if

    while (!item && (idx < table->table_len))
        item = table->table[idx++];  // skip empty buckets...

    if (item == NULL)  // no more matches?
    {
        *_key = NULL;
        *iter = NULL;
        return 0;
    } // if

    *_key = item->key;
    *iter = item;
    return 1;
} // hash_iter_keys

int hash_insert(HashTable *table, const void *key, const void *value)
{
    HashItem *item = NULL;
    const uint32_t hash = calc_hash(table, key);
    if (hash_find(table, key, NULL))
        return 0;

    // !!! FIXME: grow and rehash table if it gets too saturated.
    item = (HashItem *) malloc(sizeof (HashItem));
    if (item == NULL)
        return -1;

    item->key = key;
    item->value = value;
    item->next = table->table[hash];
    table->table[hash] = item;

    return 1;
} // hash_insert

HashTable *hash_create(void *data, const HashTable_HashFn hashfn,
              const HashTable_KeyMatchFn keymatchfn,
              const HashTable_NukeFn nukefn)
{
    const uint32_t initial_table_size = 0xFFFF;
    const uint32_t alloc_len = sizeof (HashItem *) * initial_table_size;
    HashTable *table = (HashTable *) malloc(sizeof (HashTable));
    if (table == NULL)
        return NULL;
    memset(table, '\0', sizeof (HashTable));

    table->table = (HashItem **) malloc(alloc_len);
    if (table->table == NULL)
    {
        free(table);
        return NULL;
    } // if

    memset(table->table, '\0', alloc_len);
    table->table_len = initial_table_size;
    table->data = data;
    table->hash = hashfn;
    table->keymatch = keymatchfn;
    table->nuke = nukefn;
    return table;
} // hash_create

void hash_destroy(HashTable *table)
{
    uint32_t i;
    void *data = table->data;
    for (i = 0; i < table->table_len; i++)
    {
        HashItem *item = table->table[i];
        while (item != NULL)
        {
            HashItem *next = item->next;
            table->nuke(item->key, item->value, data);
            free(item);
            item = next;
        } // while
    } // for

    free(table->table);
    free(table);
} // hash_destroy

int hash_remove(HashTable *table, const void *key)
{
    HashItem *item = NULL;
    HashItem *prev = NULL;
    void *data = table->data;
    const uint32_t hash = calc_hash(table, key);
    for (item = table->table[hash]; item != NULL; item = item->next)
    {
        if (table->keymatch(key, item->key, data))
        {
            if (prev != NULL)
                prev->next = item->next;
            else
                table->table[hash] = item->next;

            table->nuke(item->key, item->value, data);
            free(item);
            return 1;
        } // if

        prev = item;
    } // for

    return 0;
} // hash_remove


// this is djb's xor hashing function.
static inline uint32_t hash_string_djbxor(const char *str, size_t len)
{
    register uint32_t hash = 5381;
    while (len--)
        hash = ((hash << 5) + hash) ^ *(str++);
    return hash;
} // hash_string_djbxor

static inline uint32_t hash_string(const char *str, size_t len)
{
    return hash_string_djbxor(str, len);
} // hash_string

uint32_t hash_hash_string(const void *sym, void *data)
{
    (void) data;
    return hash_string((const char*) sym, strlen((const char *) sym));
} // hash_hash_string

int hash_keymatch_string(const void *a, const void *b, void *data)
{
    (void) data;
    return (strcmp((const char *) a, (const char *) b) == 0);
} // hash_keymatch_string

// end of hashtable.c ...

