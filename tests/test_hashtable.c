#include "hashtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        exit(1); \
    } \
} while (0)

/* Basic semantics: insert, get, update, delete, double-delete */
static void test_basic(void)
{
    hashtable *ht = ht_create(16);
    CHECK(ht != NULL, "ht_create");

    CHECK(ht_set(ht, "hello", 5, "world", 5) == 1, "insert new");
    size_t vlen = 0;
    void *v = ht_get(ht, "hello", 5, &vlen);
    CHECK(v && vlen == 5 && memcmp(v, "world", 5) == 0, "get after set");

    CHECK(ht_set(ht, "hello", 5, "there", 5) == 0, "update existing");
    v = ht_get(ht, "hello", 5, &vlen);
    CHECK(v && vlen == 5 && memcmp(v, "there", 5) == 0, "get updated");

    CHECK(ht_del(ht, "hello", 5) == 1, "delete");
    CHECK(ht_get(ht, "hello", 5, NULL) == NULL, "get after delete");
    CHECK(ht_del(ht, "hello", 5) == 0, "double delete");

    ht_free(ht);
    printf("test_basic: PASS\n");
}

/* Binary-safe keys + values (null bytes allowed) */
static void test_binary(void)
{
    hashtable *ht = ht_create(16);
    CHECK(ht != NULL, "create");

    char k[] = { 'a', 0, 'b', 0, 'c' };
    char v[] = { 1, 2, 0, 4, 0, 6 };
    CHECK(ht_set(ht, k, sizeof(k), v, sizeof(v)) == 1, "binary insert");

    size_t vlen = 0;
    void *got = ht_get(ht, k, sizeof(k), &vlen);
    CHECK(got && vlen == sizeof(v) && memcmp(got, v, sizeof(v)) == 0,
          "binary get");

    ht_free(ht);
    printf("test_binary: PASS\n");
}

/* 1M inserts, sab get, half delete, baaki verify */
static void test_load(void)
{
    const int N = 1000000;
    hashtable *ht = ht_create(16);
    CHECK(ht != NULL, "create");

    char k[32];
    for (int i = 0; i < N; i++) {
        int n = snprintf(k, sizeof(k), "key%09d", i);
        CHECK(n > 0, "snprintf");
        CHECK(ht_set(ht, k, (size_t)n, &i, sizeof(i)) == 1, "insert");
    }
    CHECK(ht_count(ht) == (size_t)N, "count after inserts");

    for (int i = 0; i < N; i++) {
        int n = snprintf(k, sizeof(k), "key%09d", i);
        size_t vlen = 0;
        int *v = ht_get(ht, k, (size_t)n, &vlen);
        CHECK(v && vlen == sizeof(int) && *v == i, "verify all");
    }

    for (int i = 0; i < N; i += 2) {
        int n = snprintf(k, sizeof(k), "key%09d", i);
        CHECK(ht_del(ht, k, (size_t)n) == 1, "delete even");
    }
    CHECK(ht_count(ht) == (size_t)N / 2, "count after delete-half");

    for (int i = 0; i < N; i++) {
        int n = snprintf(k, sizeof(k), "key%09d", i);
        void *v = ht_get(ht, k, (size_t)n, NULL);
        if (i % 2 == 0) CHECK(v == NULL, "deleted key gone");
        else            CHECK(v != NULL, "remaining key present");
    }

    ht_free(ht);
    printf("test_load (1M): PASS\n");
}

/* Growth aur shrink triggers — ASan yeh verify karega ki
 * resize paths memory leak nahi kar rahe. */
static void test_grow_shrink(void)
{
    hashtable *ht = ht_create(16);
    CHECK(ht != NULL, "create");

    char k[32];
    for (int i = 0; i < 1000; i++) {
        int n = snprintf(k, sizeof(k), "k%d", i);
        CHECK(ht_set(ht, k, (size_t)n, "v", 1) == 1, "insert");
    }
    for (int i = 0; i < 1000; i++) {
        int n = snprintf(k, sizeof(k), "k%d", i);
        CHECK(ht_del(ht, k, (size_t)n) == 1, "delete");
    }
    CHECK(ht_count(ht) == 0, "empty after delete all");

    ht_free(ht);
    printf("test_grow_shrink: PASS\n");
}

int main(void)
{
    test_basic();
    test_binary();
    test_grow_shrink();
    test_load();          /* last — 1M slow hai */
    printf("\nALL TESTS PASS\n");
    return 0;
}