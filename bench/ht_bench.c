#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// High-precision timing ke liye helper function
static double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void run_benchmark(size_t n) {
    printf("--- Benchmarking N = %zu ---\n", n);
    
    hashtable *ht = ht_create(16);
    char key[32];
    int val = 42;

    // 1. Insert Benchmark
    double start = get_time_sec();
    for (size_t i = 0; i < n; i++) {
        int len = snprintf(key, sizeof(key), "key%zu", i);
        ht_set(ht, key, len, &val, sizeof(val));
    }
    double end = get_time_sec();
    double insert_time = end - start;
    printf("Insert: %zu ops in %.3f sec (%.2f ops/sec)\n", 
           n, insert_time, n / insert_time);

    // 2. Lookup Benchmark
    start = get_time_sec();
    for (size_t i = 0; i < n; i++) {
        int len = snprintf(key, sizeof(key), "key%zu", i);
        ht_get(ht, key, len, NULL);
    }
    end = get_time_sec();
    double lookup_time = end - start;
    printf("Lookup: %zu ops in %.3f sec (%.2f ops/sec)\n\n", 
           n, lookup_time, n / lookup_time);

    ht_free(ht);
}

int main() {
    printf("=== Hash Table Benchmark ===\n");
    run_benchmark(1000);
    run_benchmark(100000);
    run_benchmark(1000000);
    
    return 0;
}
