# facil.io C STL - TODO

Single source of truth for all pending work. Completed items go to git history / `AI-MEMORY.json`.

---

## High Priority

### HTTP Payload Parsing
- **Location**: `fio-stl/431 http handle.h` (line ~3175 TODO marker)
- `application/x-www-form-urlencoded` — standard form submission
- `multipart/form-data` — file uploads (parser exists in `004 multipart.h`)
- `application/json` — JSON body (parser exists in `004 json.h`, needs integration)

## Medium Priority

### IO Timer Queue Fix
- **Location**: `fio-stl/400 io api.h` and related IO files
- Fix all timer queues in IO module to use `fio_io_last_tick` rather than system time

### HTTP Client Improvements
- **Location**: `fio-stl/439 http.h`
- Connection pooling/reuse (TODO at line ~1286: "test for and attempt to re-use connection")
- `Connection: close` header handling (TODO at line ~2014)
- Review WS/SSE upgrade responses (TODO at line ~1106)
- `Expect` header support (TODO at line ~1486)
- Client streaming / chunked request bodies (currently disabled)

### SQLite3 Integration
- **Location**: `fio-stl/4XX sqlite.h` (new file)
- Lightweight wrapper around SQLite3 C API
- Makefile already has SQLite3 detection
- Use FIOBJ for result sets, JSON parser for JSON columns, IO queue for async ops
- API: `fio_sqlite_open`, `fio_sqlite_exec`, `fio_sqlite_query`, `fio_sqlite_close`

### Documentation
Modules missing markdown docs:
- AES (`153 aes.h`)
- ED25519 (`154 ed25519.h`)
- Mustache (`104 mustache.h`)

## Low Priority

### HTTP 103 Early Hints
- **Location**: `fio-stl/431 http handle.h`, `fio-stl/439 http.h`
- Status code string exists but no special handling

### JSON-based Local Database (LowDB-style)
- **Location**: `fio-stl/4XX localdb.h` (new file)
- JSON file-based DB with load/save, FIOBJ queries, atomic writes
- Dependencies all exist: JSON parser, FIOBJ, File I/O

### IPC/Cluster Improvements
- **Location**: `fio-stl/404 ipc.h`
- Add ping/keepalive for stale connection detection (TODO at lines ~1668, ~1675)
- No TLS for cluster connections (uses shared-secret encryption, no forward secrecy)

### Redis History Engine
- **Location**: `fio-stl/422 redis.h`
- Implement `fio_pubsub_history_s` interface backed by Redis (e.g., Redis Streams or sorted sets)
- Enables pub/sub message replay across restarts / new subscribers via Redis persistence

### Pub/Sub Improvements
- **Location**: `fio-stl/420 pubsub.h`
- Future message delivery timers not implemented (TODO at line ~1787)
- Single-process optimization — may require profiling first

### FIXME/TODO Comment Audit
- Search codebase for FIXME/TODO comments and address low-hanging fruit
- Known: `fio_ftoa` has FIXME for hex floating-point (`002 atol.h:985`)

---

## Performance Ideas (Research Required)

### Compression
- PCLMULQDQ/PMULL carryless-multiply CRC32 for near-memory-bandwidth gzip
- Multi-literal decode (3 per iteration) in deflate decompressor
- SIMD-accelerated string matching for brotli/deflate

### Crypto
- 5-bit windows for Ed25519 scalar multiplication
- Dedicated a=-1 doubling formula for Ed25519

### WebSocket Client
- Client-side permessage-deflate extension negotiation (client never sends the extension header)
- End-to-end WebSocket compression integration test

---

## Excluded (Deferred / Too Complex)

- HTTP/2 (entire protocol unimplemented)
- PostgreSQL integration
- Markdown parser
- Floating-point atof improvements (current uses strtod)
