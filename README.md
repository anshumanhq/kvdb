# kvdb - A Redis-like Key-Value Store in C

A high-performance, in-memory key-value database built from scratch in C. 
It implements the Redis Serialization Protocol (RESP) and uses an `epoll`-based event loop for handling multiple concurrent clients efficiently.

## Architecture & Features
- **Core Data Structure:** Chained Hash Table with FNV-1a hashing, automatic resizing (load factor 0.75/0.10), and cached hashes.
- **Networking:** TCP Server using Linux `epoll` (Edge-Triggered) for non-blocking, multi-client support.
- **Protocol:** Full RESP parser supporting both Array (Bulk Strings) and Inline commands.
- **Commands Supported:** `PING`, `SET`, `GET`, `DEL`, `ECHO`.

## Benchmarks

### Phase 1: Hash Table (1M Keys, single-threaded)
| Operation | Ops/sec | Time (sec) |
|-----------|---------|------------|
| Insert    | ~1,044,000 | 0.957 |
| Lookup    | ~2,943,000 | 0.340 |

### Phase 3: Network Server (`redis-benchmark`, 50 concurrent clients, 100k requests)
| Command | Throughput (ops/sec) | p50 Latency (ms) | p99 Latency (ms) |
|---------|----------------------|------------------|------------------|
| PING    | ~30,800              | 0.879            | 3.079            |
| SET     | ~33,700              | 0.647            | 3.855            |

*Tested on: Ubuntu 24.04 (WSL2), GCC 13.2.0, -O2 optimization.*
