#ifndef KVDB_HASHTABLE_H
#define KVDB_HASHTABLE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Chained hash table — Redis ke dict.c ka simplified version.
 */
typedef struct entry {
    char    *key;
    size_t   key_len;
    void    *value;
    size_t   val_len;
    struct entry *next;
    uint64_t hash;
    int64_t  expire_at; /* 0 = no expiry, >0 = Unix timestamp */
} entry;

typedef struct {
    entry  **buckets;
    size_t   size;
    size_t   used;
    size_t   mask;
} hashtable;

hashtable *ht_create(size_t initial_capacity);
void       ht_free(hashtable *ht);

int ht_set(hashtable *ht,
           const char *key, size_t key_len,
           const void *val, size_t val_len);

void *ht_get(hashtable *ht,
             const char *key, size_t key_len,
             size_t *out_len);

int ht_del(hashtable *ht, const char *key, size_t key_len);

size_t ht_count(const hashtable *ht);

/* TTL functions */
int     ht_set_expire(hashtable *ht, const char *key, size_t key_len, int64_t expire_at);
int64_t ht_get_expire(hashtable *ht, const char *key, size_t key_len);

#endif
