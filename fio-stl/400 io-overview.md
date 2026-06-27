# IO and HTTP — Group Overview (400-range)

```c
#define FIO_IO      /* IO Reactor */
#define FIO_IPC     /* Inter-Process Communication */
#define FIO_PUBSUB  /* Pub/Sub */
#define FIO_REDIS   /* Redis engine */
#define FIO_HTTP    /* HTTP server / client */
#include "fio-stl.h"
```

The 400-range is where facil.io becomes a networked server. A single-threaded
evented IO reactor sits at the foundation, optional TLS plugs into it as a
transport hook, IPC ferries work between worker processes, PubSub builds
broadcast semantics on top of IPC, Redis extends PubSub across machines, and
HTTP closes the loop with a production-grade server that speaks HTTP/1.x,
WebSocket, and SSE.

---

## Stack Diagram

```
 ┌──────────────────────────────────────────────────────────────────┐
 │                      HTTP  (439 http.h)                           │
 │          fio_http_listen · fio_http_connect                        │
 │          HTTP/1.x · WebSocket upgrades · SSE (EventSource)        │
 └──────┬─────────────────────────────────────────┬─────────────────┘
        │ uses (internal)                          │ optional
        ▼                                          ▼
 ┌──────────────────────────────┐    ┌─────────────────────────────┐
 │  HTTP parsers  (431)          │    │     Pub/Sub  (420)           │
 │  · HTTP/1.1 parser            │    │  subscribe · publish         │
 │  · WebSocket parser (RFC6455) │    │  pattern matching · replay   │
 │  · HTTP Handle  (internal)    │    │  pluggable engine interface  │
 └──────────────────────────────┘    └──────────────┬──────────────┘
                                                     │ engine
                                      ┌──────────────▼──────────────┐
                                      │     Redis  (422 redis.h)     │
                                      │  connects via IO (master)    │
                                      │  workers route through IPC   │
                                      └──────────────┬──────────────┘
                                                     │
                                      ┌──────────────▼──────────────┐
                                      │      IPC  (404 ipc.h)        │
                                      │  worker ↔ master messages    │
                                      │  cluster RPC (multi-machine) │
                                      │  encrypted: ChaCha20-Poly1305│
                                      └──────────────┬──────────────┘
                                                     │ built on
 ┌───────────────────────────────────────────────────▼──────────────┐
 │                     IO Reactor  (401–403)                          │
 │  fio_io_s · fio_io_protocol_s    epoll / kqueue / poll            │
 │  fio_io_listen · fio_io_connect  fio_io_defer · fio_io_run_every  │
 │  fio_io_async_s (background thread pools for non-IO tasks)        │
 └──────────────────────────────────┬───────────────────────────────┘
                                    │ transport hooks (fio_io_functions_s)
                                    ▼
                         ┌─────────────────────────┐
                         │       TLS  (405)          │
                         │  OpenSSL 3.x  (preferred) │
                         │  or native TLS 1.3        │
                         │  auto-selected at build   │
                         └─────────────────────────┘
```

---

## Layer-by-Layer

### IO Reactor — 401 · 402 · 403

**Enable with:** `#define FIO_IO`  
**Docs:** [./401 io api.md](./401 io api.md)

The event loop. `fio_io_start(workers)` blocks and runs forever (or until
`fio_io_stop()`). With `workers > 0` it forks child processes; the master stays
alive to manage IPC and listeners while workers handle connections.

Key types and functions:

- `fio_io_s` — opaque IO handle (connection). Never store raw; use
  `fio_io_dup` / `fio_io_free` for lifetime management.
- `fio_io_protocol_s` — struct of callbacks (`on_attach`, `on_data`,
  `on_ready`, `on_shutdown`, `on_timeout`, `on_close`) shared by all
  connections of the same protocol family.
- `fio_io_listen(...)` — attach a listening socket with a protocol.
- `fio_io_connect(...)` — connect as a client.
- `fio_io_attach_fd(fd, protocol, udata, tls)` — adopt a raw fd.
- `fio_io_read / fio_io_write / fio_io_close` — IO operations (safe only
  inside callbacks or deferred tasks).
- `fio_io_defer(task, u1, u2)` — schedule a task on the IO thread (thread-safe).
- `fio_io_run_every(...)` — schedule a recurring timer.

`fio_io_async_s` / `fio_io_async_attach(q, threads)` spins up a background
thread pool for CPU-heavy or blocking work that must not block the IO loop.

---

### TLS — 405 openssl.h · 405 tls13.h

**Enable with:** `#define FIO_IO` (TLS backends auto-include when available)  
**Docs:** [./405 openssl.md](./405 openssl.md) · [./405 tls13.md](./405 tls13.md) *(planned)*

TLS is plugged into the reactor through `fio_io_functions_s` — a set of
transport callbacks (`build_context`, `free_context`, `start`, `read`, `write`,
`flush`, `finish`, `cleanup`) that replace the default socket calls.

The `fio_io_tls_s` context carries certificates, ALPN negotiation entries, and
trusted CA chains. It is built from `fio_io_tls_new()` / `fio_io_tls_cert_add()`
/ `fio_io_tls_alpn_add()` / `fio_io_tls_trust_add()`, or parsed directly from a
URL query string:

```c
fio_io_listen(.url = "0.0.0.0:443/?tls=./certs/", .protocol = &my_proto);
```

Two backends are available — OpenSSL 3.x (`fio_openssl_io_functions()`) and
the native TLS 1.3 engine (`fio_tls13_io_functions()`). The default is
selected automatically at build time; override with
`fio_io_tls_default_functions(&my_io_functions)` before `fio_io_start`.

> **Disambiguation:** `405` documents IO-layer TLS integration — how TLS wraps
> network connections in the reactor. The standalone TLS 1.3 crypto library
> (key schedule, record layer, handshake state machines, certificate helpers)
> lives separately at [./190 tls13.md](./190 tls13.md) and is what `405` builds on.

---

### IPC — 404 ipc.h

**Enable with:** `#define FIO_IPC`  
**Docs:** [./404 ipc.md](./404 ipc.md) *(planned)*

Workers need to talk to the master. IPC handles this over a Unix socket (or
optionally TCP for multi-machine clusters). Messages are reference-counted,
encrypted in transit (ChaCha20-Poly1305, keyed from the process secret), and
routed by flags:

| Macro | Delivery |
|-------|----------|
| `fio_ipc_call(...)` | master only |
| `fio_ipc_local(...)` | master + all local workers |
| `fio_ipc_cluster(...)` | master on every machine |
| `fio_ipc_broadcast(...)` | master + workers on every machine |

For cross-machine RPC, register a non-zero op-code with
`fio_ipc_opcode_register(...)` before `fio_io_start`. Function-pointer messages
("fast path") work only within a single machine.

---

### Pub/Sub — 420 pubsub.h

**Enable with:** `#define FIO_PUBSUB` (requires `FIO_IPC`)  
**Docs:** [./420 pubsub.md](./420 pubsub.md) *(planned)*

Pub/Sub delivers messages to named channels across all workers and, via
registered engines, across machines. The core API:

```c
fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("news"),
                     .on_message = my_callback,
                     .io = client_io);

fio_pubsub_publish(.channel = FIO_BUF_INFO1("news"),
                   .message = FIO_BUF_INFO1("breaking!"));
```

Subscriptions can be owned by an `fio_io_s` (auto-cancelled on close) or by
the global environment. Pattern subscriptions and numeric namespace `filter`
values are supported. An `engine` field selects an external backend (Redis,
custom). Replay from a timestamp is supported when history engines are attached.

---

### Redis — 422 redis.h

**Enable with:** `#define FIO_REDIS` (requires `FIO_PUBSUB`)  
**Docs:** [./422 redis.md](./422 redis.md) *(planned)*

Redis integration as a PubSub engine *and* as a standalone command client.

Only the master process connects to the Redis server — workers route all
commands and publishes through IPC. This avoids multiplying Redis connections
and preserves correct subscribe/unsubscribe semantics.

```c
/* attach as a pub/sub engine */
fio_pubsub_engine_s *r = fio_redis_new(.url = "redis://localhost:6379");
/* send arbitrary Redis commands (not SUBSCRIBE/UNSUBSCRIBE) */
fio_redis_send(r, my_fiobj_command_array, my_reply_cb, NULL);
```

The return type is `fio_pubsub_engine_s *` — attach it to a pub/sub
subscription via the `engine` field. Reference counting
(`fio_redis_dup` / `fio_redis_free`) governs the engine lifetime.
All mutation runs on the IO thread via `fio_io_defer`.

---

### HTTP Parsers — 431 http1 parser.h · 431 websocket parser.h

**Enable with:** `#define FIO_HTTP1_PARSER` / `#define FIO_WEBSOCKET_PARSER`  
**Docs:** [./431 http1 parser.md](./431 http1 parser.md) · [./431 websocket parser.md](./431 websocket parser.md) *(planned)*

Zero-allocation, event-driven parsers — no internal buffering, no heap.

- **HTTP/1.1 parser** (`fio_http1_parse`): parses request/response lines,
  headers, and bodies (including chunked). Fires user-implemented callbacks
  (`fio_http1_on_method`, `fio_http1_on_url`, `fio_http1_on_header`,
  `fio_http1_on_body_chunk`, `fio_http1_on_complete`, …).

- **WebSocket parser** (`fio_websocket_parse`): RFC 6455 frame parser.
  Zero-allocation, cache-sized. Produces typed events
  (`FIO_WEBSOCKET_EV_DATA_CHUNK`, `FIO_WEBSOCKET_EV_CONTROL`,
  `FIO_WEBSOCKET_EV_MESSAGE_END`, `FIO_WEBSOCKET_EV_ERROR`).

These are the building blocks used internally by the HTTP layer. Direct use
is for protocol-level work or embedding the parsers in a custom IO protocol.

**HTTP Handle** (`431 http handle.h`, `#define FIO_HTTP_HANDLE`) is an
internal module — the `fio_http_s` request/response state object with header
cache, body (RAM or file), and logging support. It is fully covered by the
HTTP documentation; there is no separate public handle doc.

---

### HTTP — 439 http.h

**Enable with:** `#define FIO_HTTP`  
**Docs:** [./439 http.md](./439 http.md) *(planned)*

The top-level server and client. `fio_http_listen` registers an HTTP service
on the IO reactor; `fio_http_connect` opens an HTTP client connection.

```c
fio_http_listen("0.0.0.0:3000", .on_http = handle_request);
fio_io_start(0); /* or fio_io_start(4) for 4 worker processes */
```

The `fio_http_settings_s` struct wires up:
- `on_http` — HTTP request/response callback
- `on_open` / `on_message` / `on_close` — WebSocket lifecycle
- `on_eventsource` — SSE event callback
- `on_authenticate_websocket` / `on_authenticate_sse` — upgrade guards
- `tls` / `tls_io_func` — optional TLS context
- `queue` — optional `fio_io_async_s` for off-thread response work
- `public_folder` — static file serving with `gz` pre-compressed support

HTTP handles (`fio_http_s`) are created per request and carry headers, body,
and state. The `fio_http_s` lifetime is managed by the library; callbacks
receive a pointer valid only for the duration of the call (or while an
explicit reference is held).

---

## Threading and Reactor Model

The IO reactor is **single-threaded per process**. All protocol callbacks
(`on_data`, `on_ready`, etc.) run in the reactor thread of the process that
owns the connection. There is no locking required for IO operations.

**Process model** (when `fio_io_start(workers)` is called with `workers > 0`):

```
 master process
  ├── owns IPC listener socket
  ├── runs PRE_START state callbacks
  ├── forwards cluster / pub/sub to workers
  └── worker process × N
       ├── each gets its own IO reactor loop
       ├── accepts connections (listeners are dup'd per worker)
       └── communicates back via IPC socket
```

Workers are forked; the master sentinel thread monitors each child and
respawns crashed workers automatically (configurable via
`FIO_ASSERT_DEBUG` in debug builds to prevent respawn masking crashes).

**Deferred work** (`fio_io_defer`) posts tasks onto the reactor queue —
safe to call from any thread. The reactor processes the queue after each
poll cycle.

**Timers** (`fio_io_run_every`) schedule repeating or one-shot callbacks
aligned to the reactor tick (millisecond resolution). Timer precision depends
on the poll timeout and reactor load.

**Background threads** (`fio_io_async_s`): for blocking or CPU-intensive
work that must not stall the IO loop, attach a thread pool with
`fio_io_async_attach(q, thread_count)`. The pool starts and stops with the
reactor automatically.

**Connection lifecycle:**

```
fd created (accept / connect)
    │
    └─► fio_io_attach_fd → on_attach
            │
            ├─ on_data       (data available to read)
            ├─ on_ready      (outgoing buffer drained)
            ├─ on_timeout    (idle too long; default: close)
            │
            ├─ on_shutdown   (reactor shutting down)
            └─ on_close      (connection closed — cleanup here)
```

Outgoing writes are buffered. When the buffer exceeds
`FIO_IO_THROTTLE_LIMIT` (default 2 MiB), `on_data` events are suspended
until the buffer drains ("throttling"). The shutdown sequence gives
connections up to `FIO_IO_SHUTDOWN_TIMEOUT` (default 15 s) to flush before
force-closing.

**`fio_io_env_set` / `fio_io_env_get`**: attach named objects to the
connection's lifetime. The `on_close` callback fires automatically when the
connection closes, enabling RAII-style per-connection resource management.

---

## TLS Disambiguation

There are **two separate TLS documents** in this library:

| Document | What it covers |
|----------|----------------|
| [`./405 tls13.md`](./405 tls13.md) *(planned)* | IO-layer TLS integration — how TLS is plugged into the IO reactor via `fio_io_functions_s`. Use this when setting up TLS listeners or clients. |
| [`./190 tls13.md`](./190 tls13.md) | Standalone TLS 1.3 crypto library — key schedule, record layer, handshake state machines, alerts, KeyUpdate, certificate helpers. The raw machinery. |

If you are adding TLS to a server or client, start with the `405` docs.  
If you are implementing a custom TLS backend or need direct access to the TLS 1.3 internals, start with `./190 tls13.md`.

---

## Documentation Index

| Header | Module | Doc |
|--------|--------|-----|
| `401 io api.h` | IO Reactor API | [./401 io api.md](./401 io api.md) |
| `402 io types.h` | IO Types (internal) | covered by IO docs |
| `403 io reactor.h` | IO Reactor cycle (internal) | covered by IO docs |
| `404 ipc.h` | IPC | [./404 ipc.md](./404 ipc.md) |
| `405 openssl.h` | OpenSSL TLS backend | [./405 openssl.md](./405 openssl.md) |
| `405 tls13.h` | Native TLS 1.3 IO backend | [./405 tls13.md](./405 tls13.md) |
| `420 pubsub.h` | Pub/Sub | [./420 pubsub.md](./420 pubsub.md) |
| `422 redis.h` | Redis engine | [./422 redis.md](./422 redis.md) |
| `431 http1 parser.h` | HTTP/1.1 parser | [./431 http1 parser.md](./431 http1 parser.md) |
| `431 websocket parser.h` | WebSocket parser | [./431 websocket parser.md](./431 websocket parser.md) |
| `431 http handle.h` | HTTP Handle (internal) | covered by HTTP docs |
| `439 http.h` | HTTP server / client | [./439 http.md](./439 http.md) |
