# kvdb - A Redis-like Key-Value Store in C

A high-performance, in-memory key-value database built entirely from scratch in C. It implements the Redis Serialization Protocol (RESP) and uses an `epoll`-based event loop to handle multiple concurrent clients efficiently.

## 🚀 Architecture & Features

- **Core Data Structure:** Custom Chained Hash Table with FNV-1a hashing, automatic resizing (load factor 0.75/0.10), and cached hashes.
- **Networking:** TCP Server using Linux `epoll` (Edge-Triggered) for non-blocking, highly concurrent multi-client support.
- **Protocol:** Full RESP parser supporting both Array (Bulk Strings) and Inline commands. Pipelining is supported.
- **Persistence:** Append-Only File (AOF) implementation for data durability. Data is safely restored upon server restart.
- **Expiry:** Lazy TTL (Time-To-Live) implementation for automatic key eviction.
- **DevOps & Testing:** Fully containerized with a multi-stage Dockerfile and tested automatically via GitHub Actions CI.

## 🛠️ Supported Commands

| Command | Usage | Description |
|---------|-------|-------------|
| `PING` | `PING` | Returns `+PONG` to check server health |
| `SET` | `SET key value` | Sets the string value of a key |
| `GET` | `GET key` | Gets the value of a key (returns `(nil)` if not found) |
| `DEL` | `DEL key` | Deletes a key and returns `(integer) 1` if successful |
| `EXPIRE`| `EXPIRE key seconds`| Sets a timeout on a key |
| `TTL` | `TTL key` | Returns the remaining time to live in seconds |

## 📊 Benchmarks

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

## ⚙️ How to Build & Run

### Local Build (Linux/WSL)
```bash
# Build the release binary
make clean && make release

# Start the server on port 6380
./build/kvdb --port 6380
