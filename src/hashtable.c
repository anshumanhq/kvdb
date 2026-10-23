#include "hashtable.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>   /* time() ke liye */

#define HT_INITIAL_SIZE 16
#define HT_LOAD_HIGH    0.75
#define HT_LOAD_LOW     0.10

static uint64_t fnv1a(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static size_t next_pow2(size_t n)
{
    if (n < 2) return 2;
    n--;
    n |= n >> 1;  n |= n >> 2;  n |= n >> 4;
    n |= n >> 8;  n |= n >> 16; n |= n >> 32;
    return n + 1;
}

hashtable *ht_create(size_t initial_capacity)
{
    hashtable *ht = calloc(1, sizeof(*ht));
    if (!ht) return NULL;

    size_t sz = next_pow2(initial_capacity < HT_INITIAL_SIZE
                          ? HT_INITIAL_SIZE : initial_capacity);
    ht->buckets = calloc(sz, sizeof(entry *));
    if (!ht->buckets) { free(ht); return NULL; }

    ht->size = sz;
    ht->mask = sz - 1;
    ht->used = 0;
    return ht;
}

static void entry_free(entry *e)
{
    if (!e) return;
    free(e->key);
    free(e->value);
    free(e);
}

void ht_free(hashtable *ht)
{
    if (!ht) return;
    for (size_t i = 0; i < ht->size; i++) {
        entry *e = ht->buckets[i];
        while (e) {
            entry *n = e->next;
            entry_free(e);
            e = n;
        }
    }
    free(ht->buckets);
    free(ht);
}

static int ht_resize(hashtable *ht, size_t new_size)
{
    entry **nb = calloc(new_size, sizeof(entry *));
    if (!nb) return -1;

    size_t nmask = new_size - 1;
    for (size_t i = 0; i < ht->size; i++) {
        entry *e = ht->buckets[i];
        while (e) {
            entry *next = e->next;
            size_t idx = e->hash & nmask;
            e->next = nb[idx];
            nb[idx] = e;
            e = next;
        }
    }
    free(ht->buckets);
    ht->buckets = nb;
    ht->size = new_size;
    ht->mask = nmask;
    return 0;
}

int ht_set(hashtable *ht,
           const char *key, size_t key_len,
           const void *val, size_t val_len)
{
    uint64_t h = fnv1a(key, key_len);
    size_t idx = h & ht->mask;

    /* Update path */
    for (entry *e = ht->buckets[idx]; e; e = e->next) {
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0) {
            void *nv = malloc(val_len);
            if (!nv) return -1;
            memcpy(nv, val, val_len);
            free(e->value);
            e->value = nv;
            e->val_len = val_len;
            e->expire_at = 0; /* Update karne pe TTL reset */
            return 0;
        }
    }

    /* Insert path */
    entry *e = calloc(1, sizeof(*e));
    if (!e) return -1;

    e->key = malloc(key_len);
    if (!e->key) { free(e); return -1; }
    memcpy(e->key, key, key_len);

    e->value = malloc(val_len);
    if (!e->value) { free(e->key); free(e); return -1; }
    memcpy(e->value, val, val_len);

    e->key_len = key_len;
    e->val_len = val_len;
    e->hash    = h;
    e->expire_at = 0; /* Default: no expiry */
    e->next    = ht->buckets[idx];
    ht->buckets[idx] = e;
    ht->used++;

    if ((double)ht->used / (double)ht->size > HT_LOAD_HIGH) {
        (void)ht_resize(ht, ht->size * 2);
    }
    return 1;
}

void *ht_get(hashtable *ht, const char *key, size_t key_len, size_t *out_len)
{
    uint64_t h = fnv1a(key, key_len);
    size_t idx = h & ht->mask;

    entry **prev = &ht->buckets[idx]; // Double pointer to delete safely
    for (entry *e = *prev; e; prev = &e->next, e = e->next) {
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0) {
            
            // Lazy Expiry: Delete from memory if expired
            if (e->expire_at > 0 && e->expire_at <= time(NULL)) {
                *prev = e->next;
                entry_free(e);
                ht->used--;
                return NULL;
            }
            
            if (out_len) *out_len = e->val_len;
            return e->value;
        }
    }
    return NULL;
}

int ht_del(hashtable *ht, const char *key, size_t key_len)
{
    uint64_t h = fnv1a(key, key_len);
    size_t idx = h & ht->mask;

    entry **prev = &ht->buckets[idx];
    for (entry *e = *prev; e; prev = &e->next, e = e->next) {
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0) {
            *prev = e->next;
            entry_free(e);
            ht->used--;

            if (ht->size > HT_INITIAL_SIZE &&
                (double)ht->used / (double)ht->size < HT_LOAD_LOW) {
                (void)ht_resize(ht, ht->size / 2);
            }
            return 1;
        }
    }
    return 0;
}

size_t ht_count(const hashtable *ht) { return ht->used; }

/* TTL Functions */

int ht_set_expire(hashtable *ht, const char *key, size_t key_len, int64_t expire_at)
{
    uint64_t h = fnv1a(key, key_len);
    size_t idx = h & ht->mask;

    for (entry *e = ht->buckets[idx]; e; e = e->next) {
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0) {
            /* Agar key pehle se expire ho chuki hai, toh naya set na karein */
            if (e->expire_at > 0 && e->expire_at <= time(NULL)) {
                return 0; 
            }
            e->expire_at = expire_at;
            return 1;
        }
    }
    return 0;
}

int64_t ht_get_expire(hashtable *ht, const char *key, size_t key_len)
{
    uint64_t h = fnv1a(key, key_len);
    size_t idx = h & ht->mask;

    for (entry *e = ht->buckets[idx]; e; e = e->next) {
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0) {
            
            if (e->expire_at > 0 && e->expire_at <= time(NULL)) {
                return -2; /* Expired */
            }
            if (e->expire_at == 0) {
                return -1; /* No TTL set */
            }
            return e->expire_at; /* Unix timestamp */
        }
    }
    return -2; /* Key not found (Redis standard) */
}
