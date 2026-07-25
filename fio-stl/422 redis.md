# Redis — Pub/Sub Engine and Command Client (422 redis.h)

```c
#define FIO_REDIS
#include FIO_INCLUDE_FILE
```

> **Requires:** `FIO_IO`, `FIO_PUBSUB`, `FIO_FIOBJ`, `FIO_RESP3`. When using `include.h` all dependencies are resolved automatically.

The Redis module does two things: it acts as a **Pub/Sub engine** that connects facil.io's Pub/Sub system to a Redis server, enabling cross-machine message distribution; and it acts as a **Redis command client** for arbitrary commands (`GET`, `SET`, `INCR`, etc.).

See [./400 io-overview.md](./400 io-overview.md) for where Redis fits in the full IO stack, [./420 pubsub.md](./420 pubsub.md) for the Pub/Sub engine interface, [./404 ipc.md](./404 ipc.md) for the IPC transport workers use to reach the master, and [./250 fiobj.md](./250 fiobj.md) for how FIOBJ types map to RESP replies.

---

## Architecture

Only the **master** process opens a TCP connection to Redis. Workers never connect directly. Instead:

- Workers forward `fio_redis_send()` calls to the master via IPC; the master executes the command and replies back.
- Workers forward `fio_pubsub_publish()` calls to the master via IPC; the master sends `PUBLISH` to Redis.
- Incoming subscription messages arrive as RESP3 push frames on the master's connection and are fanned out to all workers via the normal Pub/Sub IPC infrastructure.

**RESP3 is required (Redis ≥ 6.0).** The engine negotiates the protocol with a `HELLO 3` handshake on every (re)connection — authentication folds into `HELLO`. There is **no RESP2 fallback**: if the handshake fails (old server, bad credentials), the engine logs a hard error and stops.

After `HELLO 3`, `SUBSCRIBE` no longer commandeers the connection, so the master maintains a **single TCP connection** per engine:

| Traffic | Handling |
|---|---|
| Commands (`fio_redis_send`), `PUBLISH`, `PING`, `HELLO` | Lock-step FIFO queue — one command in flight at a time |
| `SUBSCRIBE` / `PSUBSCRIBE` / `UNSUBSCRIBE` / `PUNSUBSCRIBE` | Fire-and-forget writes; confirmations arrive as RESP3 **push frames**, never command replies, so they cannot desynchronize the queue |
| Incoming pub/sub messages | RESP3 **push frames**, routed to the push handler and re-published locally via `fio_pubsub_engine_ipc()` (zero-copy from the read buffer) |

```
              Master Process
 ┌──────────────────────────────────────────┐
 │  connection ─────── Redis Server         │
 │  (commands / PUBLISH / PING / HELLO      │
 │   + SUBSCRIBE family / push frames)      │
 └──────────────────────────────────────────┘
         ▲                   │
         │ IPC               │ pub/sub IPC fan-out
         │                   ▼
 ┌────────────┐   ┌────────────┐   ┌────────────┐
 │  Worker 1  │   │  Worker 2  │   │  Worker N  │
 └────────────┘   └────────────┘   └────────────┘
```

In single-process mode (`fio_io_start(0)`) the process is both master and worker; all operations go directly to Redis with no IPC overhead.

---

## Setup

```c
#define FIO_LOG
#define FIO_REDIS
#include FIO_INCLUDE_FILE

int main(void) {
  /* Create the engine BEFORE fio_io_start() (before fork). */
  fio_pubsub_engine_s *redis = fio_redis_new(
      .url  = "redis://localhost:6379",
      .auth = "my_password"          /* optional */
  );

  /* Dup before attach — attach transfers your reference to the system. */
  fio_redis_dup(redis);              /* ref: 1 → 2 */
  fio_pubsub_engine_attach(redis);   /* system takes the original ref */

  fio_io_start(4);                   /* master connects; workers use IPC */

  fio_pubsub_engine_detach(redis);   /* system releases its ref: 2 → 1 */
  fio_redis_free(redis);             /* caller releases dup:  1 → 0 → destroy */
}
```

`fio_redis_new()` **must** be called before `fio_io_start()`. Creating an engine from a worker process is not supported.

---

## Configuration

### `FIO_REDIS_READ_BUFFER`

```c
#define FIO_REDIS_READ_BUFFER 65536   /* default: 64 KiB */
```

Read buffer size for the connection. The master allocates `FIO_REDIS_READ_BUFFER` bytes of buffer space inside each engine. Note that large replies do **not** need to fit the buffer: blob strings larger than `FIO_RESP3_STREAM_THRESHOLD` (see [./004 resp3.md](./004 resp3.md)) are streamed incrementally. The effective size cap is `payload_limit` (see below).

### `FIO_REDIS_MAX_BATCH`

```c
#define FIO_REDIS_MAX_BATCH 128   /* default: 128 messages */
```

Maximum number of complete messages processed per `on_data` event. When the cap is reached with more data buffered, processing continues in a deferred task, keeping any single event-loop callback small.

---

## Types

### `fio_redis_args_s`

```c
typedef struct {
  const char *url;          /* Redis server URL; NULL → "localhost:6379" */
  const char *auth;         /* Password for HELLO ... AUTH default <pwd>; NULL = no auth */
  size_t      auth_len;     /* Length of auth; 0 = strlen(auth) */
  uint8_t     ping_interval;/* Keepalive interval in seconds; 0 → 30 s */
  size_t      payload_limit;/* Per-message budget in bytes; 0 → 16 MiB */
} fio_redis_args_s;
```

**`url`** accepted formats:

| Format | Example |
|---|---|
| Scheme with port | `"redis://host:6380"` |
| Scheme, default port | `"redis://host"` |
| Host and port | `"host:6380"` |
| Host only | `"myredis"` |
| NULL or `""` | → `localhost:6379` |

**`ping_interval`** — the IO reactor sends a `PING` on the connection if it has been idle this many seconds. Default: **30 seconds**. The protocol error timeout (detecting a hung connection) is also governed by this value.

**`payload_limit`** — cumulative payload budget **per top-level Redis message**, in bytes. Default: **16 MiB** (`16 << 20`). The budget is charged as:

```
total = Σ(all string payload bytes) + 32 × (count of ALL objects)
```

Every object in the reply (String, Array, Map, Number, Bool, etc.) costs 32 bytes of budget; string payload bytes are charged in full — for fixed-length blob strings, **before** the buffer is allocated. A breach logs an error and disconnects the engine. This protects against hostile or corrupt servers declaring huge `$<len>` allocations or sending oversized replies. Pub/sub push frames share the same per-message budget.

---

## Reference Counting and Ownership

The engine uses reference counting (`FIO_REF`). Three things independently hold or transfer refs:

1. **Caller**: `fio_redis_new()` returns `ref = 1`. The caller owns this ref and must release it with `fio_redis_free()`.

2. **Pub/Sub system**: `fio_pubsub_engine_attach()` prepares the engine for Pub/Sub use. The Redis engine keeps its Pub/Sub reference on first `SUBSCRIBE` / `PSUBSCRIBE` and releases it from `on_detached`; call `fio_redis_dup()` before attach if you need to keep using the pointer after attaching.

3. **Internal deferred tasks**: The engine dups before scheduling an IO-deferred task and frees at the end. These are invisible to the caller.

**Usage patterns:**

```c
/* Pattern A — with Pub/Sub */
fio_pubsub_engine_s *redis = fio_redis_new(...); /* ref = 1 */
fio_redis_dup(redis);                            /* ref = 2 (keep a copy) */
fio_pubsub_engine_attach(redis);                 /* system takes original ref */
/* ...use pub/sub... */
fio_pubsub_engine_detach(redis);                 /* system releases → ref = 1 */
fio_redis_free(redis);                           /* caller releases → ref = 0 → destroy */

/* Pattern B — command client only, no pub/sub */
fio_pubsub_engine_s *redis = fio_redis_new(...); /* ref = 1 */
/* ...send commands... */
fio_redis_free(redis);                           /* ref = 0 → destroy */
```

You do **not** have to detach before freeing. If you free while the pub/sub system still holds its ref, the engine remains alive until detach (or shutdown) fires the `on_detached` callback.

---

## API

### `fio_redis_new`

```c
fio_pubsub_engine_s *fio_redis_new(fio_redis_args_s args);
#define fio_redis_new(...)  fio_redis_new((fio_redis_args_s){__VA_ARGS__})
```

Creates a Redis engine with `ref = 1`. The macro overload enables named arguments.

Returns a pointer to `fio_pubsub_engine_s` on success, `NULL` on allocation failure.

The engine is typed as `fio_pubsub_engine_s *` so it can be passed directly to `fio_pubsub_engine_attach()`. Pass the same pointer to `fio_redis_dup()`, `fio_redis_free()`, and `fio_redis_send()`.

The connection is deferred to the IO reactor; the engine does nothing until `fio_io_start()` is called.

```c
fio_pubsub_engine_s *redis = fio_redis_new(
    .url           = "redis://10.0.0.5:6379",
    .auth          = "s3cr3t",
    .ping_interval = 60
);
```

### `fio_redis_dup`

```c
fio_pubsub_engine_s *fio_redis_dup(fio_pubsub_engine_s *engine);
```

Atomically increments the reference count and returns the engine. Returns `NULL` if `engine` is `NULL`.

Each `fio_redis_dup()` must be balanced with a `fio_redis_free()`.

### `fio_redis_free`

```c
void fio_redis_free(fio_pubsub_engine_s *engine);
```

Releases the caller's reference. When the count reaches zero, destroys the engine immediately: sets `running = 0`, closes the connection, drains the command queue (invoking any pending callbacks with `FIOBJ_INVALID`), and frees memory.

Safe to call with `NULL` (no-op).

### `fio_redis_state`

```c
typedef enum {
  FIO_REDIS_STATE_ERROR,
  FIO_REDIS_STATE_CONNECTING,
  FIO_REDIS_STATE_CONNECTED,
} fio_redis_state_e;

fio_redis_state_e fio_redis_state(const fio_pubsub_engine_s *engine);
```

Returns a Redis connection-state snapshot when called from the IO thread:

| State | Meaning |
|---|---|
| `FIO_REDIS_STATE_ERROR` | `engine` is `NULL`, or `HELLO 3` failed and disabled the engine. |
| `FIO_REDIS_STATE_CONNECTING` | No TCP socket is attached, or the socket is awaiting its `HELLO 3` reply. |
| `FIO_REDIS_STATE_CONNECTED` | The TCP socket is attached and its RESP3 `HELLO 3` handshake completed. |

This is an observability API; callers may send commands in every state. Commands queue until the handshake completes.

### `fio_redis_send`

```c
int fio_redis_send(fio_pubsub_engine_s *engine,
                   FIOBJ command,
                   void (*callback)(fio_pubsub_engine_s *e,
                                    FIOBJ reply,
                                    void *udata),
                   void *udata);
```

Sends a Redis command. `command` must be a `FIOBJ_T_ARRAY` whose elements are the command verb and arguments as `FIOBJ_T_STRING` (or numbers). `callback` is optional (pass `NULL` to fire-and-forget).

Returns `0` on success, `-1` if `engine` is `NULL` or `command` is not a FIOBJ array.

**On the master**: the command is serialized to RESP and queued on the connection. Commands are sent one at a time (lock-step); the next command is sent after the reply for the previous one arrives.

**On a worker**: the RESP bytes are forwarded to the master via IPC. The master executes the command, serializes the reply to RESP, and sends it back. The worker deserializes and calls the callback on its IO thread. This is transparent to the caller.

**Never** pass `SUBSCRIBE`, `PSUBSCRIBE`, `UNSUBSCRIBE`, or `PUNSUBSCRIBE` to `fio_redis_send()`. These are managed internally as fire-and-forget writes (their confirmations are RESP3 push frames, not command replies); sending them through the command queue would desynchronize the lock-step reply FIFO.

**Callback signature:**

```c
void my_callback(fio_pubsub_engine_s *e, FIOBJ reply, void *udata);
```

`reply` is `FIOBJ_INVALID` if the engine was destroyed or the connection was lost before the reply arrived. Otherwise it is a FIOBJ object:

| Redis response | FIOBJ type |
|---|---|
| Bulk string / simple string | `FIOBJ_T_STRING` |
| Integer | `FIOBJ_T_NUMBER` |
| Array | `FIOBJ_T_ARRAY` |
| Map (RESP3) | `FIOBJ_T_HASH` |
| Null | `fiobj_null()` |
| Boolean (RESP3) | `fiobj_true()` / `fiobj_false()` |
| Double (RESP3) | `FIOBJ_T_FLOAT` |
| Bignum (RESP3) | `FIOBJ_T_STRING` |
| Error | `FIOBJ_T_STRING` (warning logged) |

The callback owns nothing — `reply` is freed by the engine after the callback returns. Copy any data you need to keep.

---

## Building Commands

Commands are FIOBJ arrays of strings (and optionally numbers). Build them, pass to `fio_redis_send()`, then free them — the engine serializes to RESP before returning.

```c
/* SET mykey "hello" */
FIOBJ cmd = fiobj_array_new();
fiobj_array_push(cmd, fiobj_str_new_cstr("SET", 3));
fiobj_array_push(cmd, fiobj_str_new_cstr("mykey", 5));
fiobj_array_push(cmd, fiobj_str_new_cstr("hello", 5));
fio_redis_send(redis, cmd, NULL, NULL);   /* fire-and-forget */
fiobj_free(cmd);

/* GET mykey */
static void on_get(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  fio_str_info_s s = fiobj2cstr(reply);
  printf("value: %.*s\n", (int)s.len, s.buf);
  (void)e; (void)udata;
}

cmd = fiobj_array_new();
fiobj_array_push(cmd, fiobj_str_new_cstr("GET", 3));
fiobj_array_push(cmd, fiobj_str_new_cstr("mykey", 5));
fio_redis_send(redis, cmd, on_get, NULL);
fiobj_free(cmd);

/* INCR counter */
static void on_incr(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  printf("counter now: %ld\n", (long)fiobj2i(reply));
  (void)e; (void)udata;
}

cmd = fiobj_array_new();
fiobj_array_push(cmd, fiobj_str_new_cstr("INCR", 4));
fiobj_array_push(cmd, fiobj_str_new_cstr("counter", 7));
fio_redis_send(redis, cmd, on_incr, NULL);
fiobj_free(cmd);
```

Array replies (e.g., `LRANGE`, `HGETALL`):

```c
static void on_hgetall(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  size_t n = fiobj_array_count(reply);
  for (size_t i = 0; i + 1 < n; i += 2) {
    fio_str_info_s k = fiobj2cstr(fiobj_array_get(reply, (int32_t)i));
    fio_str_info_s v = fiobj2cstr(fiobj_array_get(reply, (int32_t)(i + 1)));
    printf("  %.*s = %.*s\n", (int)k.len, k.buf, (int)v.len, v.buf);
  }
  (void)e; (void)udata;
}

cmd = fiobj_array_new();
fiobj_array_push(cmd, fiobj_str_new_cstr("HGETALL", 7));
fiobj_array_push(cmd, fiobj_str_new_cstr("myhash", 6));
fio_redis_send(redis, cmd, on_hgetall, NULL);
fiobj_free(cmd);
```

See [./250 fiobj.md](./250 fiobj.md) for FIOBJ construction and inspection helpers.

---

## Pub/Sub Integration

When attached via `fio_pubsub_engine_attach()`, the Redis engine implements the `fio_pubsub_engine_s` interface:

| Pub/Sub event | Redis action |
|---|---|
| New channel subscription | `SUBSCRIBE channel` (fire-and-forget) |
| New pattern subscription | `PSUBSCRIBE pattern` (fire-and-forget) |
| Channel unsubscribe | `UNSUBSCRIBE channel` (fire-and-forget) |
| Pattern unsubscribe | `PUNSUBSCRIBE pattern` (fire-and-forget) |
| `fio_pubsub_publish(...)` | `PUBLISH channel message` through the command queue (workers route via IPC) |
| Incoming Redis push message | Re-published locally with `fio_pubsub_engine_ipc()` to fan out to all processes (zero-copy from the read buffer) |

The `filter` parameter from facil.io's Pub/Sub system is ignored by the Redis engine — Redis does not support numeric filter namespaces.

Example: subscribing to Redis channels through facil.io's Pub/Sub API:

```c
static fio_pubsub_engine_s *redis_engine; /* global, set before fork */

static void on_message(fio_pubsub_msg_s *msg) {
  printf("channel: %.*s  message: %.*s\n",
         (int)msg->channel.len, msg->channel.buf,
         (int)msg->message.len, msg->message.buf);
}

/* Called after IO reactor starts (FIO_CALL_ON_START) */
static void on_start(void *udata) {
  /* Regular subscription — Redis engine sends SUBSCRIBE */
  fio_pubsub_subscribe(.channel    = FIO_BUF_INFO1("alerts"),
                       .on_message = on_message);

  /* Pattern subscription — Redis engine sends PSUBSCRIBE */
  fio_pubsub_subscribe(.channel    = FIO_BUF_INFO1("events:*"),
                       .on_message = on_message,
                       .is_pattern = 1);

  /* Publish via Redis — specify the engine to route through Redis PUBLISH.
   * Without .engine the default IPC engine is used (local cluster only). */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("alerts"),
                     .message = FIO_BUF_INFO1("server started"),
                     .engine  = redis_engine);
  (void)udata;
}

int main(void) {
  redis_engine = fio_redis_new(.url = "localhost:6379");
  fio_redis_dup(redis_engine);            /* keep a copy before attaching */
  fio_pubsub_engine_attach(redis_engine); /* system takes the original ref */
  fio_state_callback_add(FIO_CALL_ON_START, on_start, NULL);
  fio_io_start(4);
  fio_pubsub_engine_detach(redis_engine);
  fio_redis_free(redis_engine);
}
```

See [./420 pubsub.md](./420 pubsub.md) for the full Pub/Sub API and engine interface.

---

## Multi-Process Behavior

The IPC routing is automatic and transparent:

| Call | On master | On worker |
|---|---|---|
| `fio_redis_send()` | Queued on the connection | Serialized → IPC → master → Redis → IPC reply → worker callback |
| `fio_pubsub_publish()` | `PUBLISH` through the command queue | Forwarded via IPC; master sends `PUBLISH` |
| Subscription messages | Received as RESP3 push frames, fanned out via Pub/Sub IPC | Delivered by Pub/Sub IPC from master |

Detection uses `fio_io_is_master()`. In single-process mode `fio_io_start(0)` everything uses the master path.

Worker processes inherit the engine pointer after fork. A `FIO_CALL_IN_CHILD` hook swaps the engine's `publish` vtable pointer to the IPC-forwarding implementation, so workers never accidentally write directly to a Redis socket.

See [./404 ipc.md](./404 ipc.md) for IPC internals.

---

## Reconnect and Failure Behavior

- **Connection loss**: The `on_close` callback logs a warning and schedules a reconnect via `fio_io_defer()`. Reconnection is attempted after a brief delay; on failure, it retries again.
- **Handshake**: Every (re)connection starts with `HELLO 3` (auth folded in) as the first queued command; its map reply is consumed by the normal reply FIFO. Handshake failure is a hard engine error (the engine stops; there is no RESP2 fallback).
- **Queued commands**: Commands in the queue on the master stay queued and are flushed once the connection is re-established (after `HELLO`).
- **Resubscription**: After the `HELLO` handshake, the engine iterates the current Pub/Sub channel maps and re-sends all active `SUBSCRIBE` / `PSUBSCRIBE` commands directly (single batched write, fire-and-forget). This restores subscription state without touching ref counts or re-attaching the engine.
- **Keepalive**: A `PING` is queued on the connection after `ping_interval` seconds idle. If the connection has unacknowledged commands when the timeout fires, the connection is forcibly closed and reconnected.
- **Engine destroyed while commands are pending**: All queued commands have their callbacks invoked immediately with `reply = FIOBJ_INVALID`.

---

## Thread Safety

All public API functions are thread-safe:

| Function | Mechanism |
|---|---|
| `fio_redis_new()` | Defers connection to IO thread |
| `fio_redis_dup()` | Atomic reference count increment |
| `fio_redis_free()` | Atomic reference count decrement; destroy runs on caller thread |
| `fio_redis_send()` | Defers queue insertion to IO thread (master); IPC on workers |

Internal state (command queue, connection pointers) is only accessed from the IO thread. No locks are needed internally.

FIOBJ objects passed to callbacks are **not** thread-safe. Copy any data you need to keep or use outside the callback.

---

## Limitations

- `fio_redis_new()` must be called before `fio_io_start()` (i.e., before fork). Creating engines from worker processes is not supported.
- **RESP3 required**: Redis >= 6.0 (or a RESP3-capable server such as Valkey). RESP2-only servers and RESP2-only proxies are not supported (the `HELLO 3` handshake fails hard).
- Redis's numeric filter namespaces (`filter` field in Pub/Sub) are not supported. All Redis pub/sub operates with `filter = 0`.
- Single-node Redis only. Redis Cluster requires connecting to the correct shard or using a proxy.
- Replies and push messages are bounded by `payload_limit` (default 16 MiB cumulative per message), not by `FIO_REDIS_READ_BUFFER` — blob strings larger than the read buffer are streamed incrementally.
- Chunked (`$?`) strings are rejected inside push frames (Redis never emits them; the command-reply path supports them).
