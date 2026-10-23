# kvdb - A Redis-like Key-Value Store in C

A high-performance, in-memory key-value database built entirely from scratch in C. 
It implements the Redis Serialization Protocol (RESP), uses an `epoll`-based event loop for handling thousands of concurrent clients, and supports data persistence via Append-Only File (AOF) and Time-To-Live (TTL) expirations.

## 🏗️ Architecture & Features

- **Core Data Structure:** Chained Hash Table with FNV-1a hashing, automatic resizing (load factor 0.75/0.10), and cached hashes for O(1) average time complexity.
- **Networking:** Non-blocking TCP Server using Linux `epoll` (Edge-Triggered) for high-performance, multi-client support on a single thread.
- **Protocol:** Custom RESP parser supporting both Array (Bulk Strings) and Inline commands, compatible with standard clients like `redis-cli`.
- **Persistence:** Append-Only File (AOF) implementation. Every write operation is logged and replayed on startup to ensure data durability.
- **Expiry:** Lazy expiration using TTL (Time-To-Live) to automatically invalidate and hide expired keys.
- **Deployment:** Multi-stage Dockerfile for lightweight, production-ready containerization.
- **CI/CD:** Automated testing via GitHub Actions (runs ASan, UBSan, Valgrind, and Docker build tests).

### Supported Commands
```text
PING
SET <key> <value>
GET <key>
DEL <key>
EXPIRE <key> <seconds>
TTL <key>
```

## 📊 Benchmarks

### Phase 1: Core Hash Table (1M Keys, single-threaded, without network)
| Operation | Ops/sec | Time (sec) |
|-----------|---------|------------|
| Insert    | ~1,044,000 | 0.957 |
| Lookup    | ~2,943,000 | 0.340 |

### Phase 3: Network Server (Using `redis-benchmark`, 50 concurrent clients, 100k requests)
| Command | Throughput (ops/sec) | p50 Latency (ms) | p99 Latency (ms) |
|---------|----------------------|------------------|------------------|
| PING    | ~26,400              | 0.783            | 6.647            |
| GET     | ~27,000              | 1.247            | 4.407            |
| SET     | ~14,700              | 2.975            | 6.367            |

*Tested on: Ubuntu 24.04 (WSL2), GCC 13.2.0, -O2 optimization.*

## 🚀 Build & Run

### Local Setup (Linux/WSL)
```bash
# Build the debug version (with AddressSanitizer)
make debug

# Run the server (default port: 6380)
./build/kvdb --port 6380
```

### Using Docker
```bash
# Build the Docker image
docker build -t kvdb .

# Run the container
docker run -d --name my-kvdb -p 6380:6380 kvdb
```

## 🧪 Testing

Connect using standard `redis-cli`:
```bash
redis-cli -p 6380

127.0.0.1:6380> SET user anshuman
OK
127.0.0.1:6380> GET user
"anshuman"
127.0.0.1:6380> EXPIRE user 5
(integer) 1
127.0.0.1:6380> TTL user
(integer) 5
```

## 🧠 What I'd Do Differently (Future Work)
If I were to take this project to the next level, I would:
1. **Implement RDB Persistence:** Add `fork()`-based copy-on-write snapshotting alongside AOF for faster restarts.
2. **Add Transactions:** Implement `MULTI`/`EXEC` to allow atomic execution of grouped commands.
3. **Pub/Sub:** Add a publish-subscribe messaging pattern.
4. **Sharded Locking:** Move from a single-threaded event loop to a multi-threaded reactor pattern with sharded locks for better CPU utilization on heavy write loads.
