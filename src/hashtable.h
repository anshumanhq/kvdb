#ifndef KVDB_HASHTABLE_H
#define KVDB_HASHTABLE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Chained hash table — Redis ke dict.c ka simplified version.
 * Har bucket ek linked list hai. Collision pe list mein prepend.
 *
 * Design choices (interview mein defend karne ke liye):
 *  - Separate chaining (open addressing nahi) => delete simple, load factor >1 allowed
 *  - size hamesha power of 2 => h % size ko h & mask se replace kar sakte hain
 *  - hash entry mein cache => rehash pe recompute nahi karna
 *  - key/value length-prefixed => binary-safe (null bytes allowed)
 */

typedef struct entry {
    char    *key;
    size_t   key_len;
    void    *value;
    size_t   val_len;
    struct entry *next;
    uint64_t hash;        /* FNV-1a hash, cached — resize pe reuse hoga */
} entry;

typedef struct {
    entry  **buckets;
    size_t   size;        /* buckets ki count, hamesha power of 2 */
    size_t   used;        /* kitne entries hain total */
    size_t   mask;        /* size - 1 (h & mask == h % size) */
} hashtable;

hashtable *ht_create(size_t initial_capacity);
void       ht_free(hashtable *ht);

/*
 * Insert ya update.
 * Returns: 1 = naya insert, 0 = existing update, -1 = alloc failure
 */
int ht_set(hashtable *ht,
           const char *key, size_t key_len,
           const void *val, size_t val_len);

/*
 * Lookup. Value ka pointer return, *out_len mein length.
 * Miss pe NULL. Returned pointer sirf tab tak valid jab tak
 * corresponding key delete/update na ho.
 */
void *ht_get(hashtable *ht,
             const char *key, size_t key_len,
             size_t *out_len);

/* Delete. 1 = mila aur delete hua, 0 = nahi mila */
int ht_del(hashtable *ht, const char *key, size_t key_len);

size_t ht_count(const hashtable *ht);

#endif