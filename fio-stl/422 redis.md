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

Only the **master** process opens TCP connections to Redis. Workers never connect directly. Instead:

- Workers forward `fio_redis_send()` calls to the master via IPC; the master executes the command and replies back.
- Workers forward `fio_pubsub_publish()` calls to the master via IPC; the master sends `PUBLISH` to Redis.
- Incoming subscription messages arrive on the master's subscription connection and are fanned out to all workers via the normal Pub/Sub IPC infrastructure.

The master maintains two separate TCP connections:

| Connection | Purpose |
|---|---|
| **pub_conn** | Commands (`fio_redis_send`), `PUBLISH`, `PING`, `AUTH` |
| **sub_conn** | `SUBSCRIBE` / `PSUBSCRIBE` push messages only |

Redis protocol requires this split: a connection in subscription mode cannot execute regular commands.

```
              Master Process
 ┌──────────────────────────────────────────┐
 │  pub_conn ──────────────────────────┐    │
 │  (commands / PUBLISH / PING / AUTH) │    │
 │                                     ▼    │
 │  sub_conn ──────── Redis Server          │
 │  (SUBSCRIBE / PSUBSCRIBE push)      ▲    │
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
#define FIO_REDIS_READ_BUFFER 32768   /* default: 32 KiB */
```

Read buffer size per connection. The master allocates `FIO_REDIS_READ_BUFFER × 2` bytes of buffer space inside each engine (one slice per connection). Increase this for workloads with very large individual Redis replies.

---

## Types

### `fio_redis_args_s`

```c
typedef struct {
  const char *url;          /* Redis server URL; NULL → "localhost:6379" */
  const char *auth;         /* AUTH password; NULL = no auth */
  size_t      auth_len;     /* Length of auth; 0 = strlen(auth) */
  uint8_t     ping_interval;/* Keepalive interval in seconds; 0 → 30 s */
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

**`ping_interval`** — the IO reactor sends a `PING` on each connection if it has been idle this many seconds. Default: **30 seconds**. The protocol error timeout (detecting a hung connection) is also governed by this value.

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

Releases the caller's reference. When the count reaches zero, destroys the engine immediately: sets `running = 0`, closes both connections, drains the command queue (invoking any pending callbacks with `FIOBJ_INVALID`), and frees memory.

Safe to call with `NULL` (no-op).

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

**On the master**: the command is serialized to RESP and queued on `pub_conn`. Commands are sent one at a time (pipelined within the queue); the next command is sent after the reply for the previous one arrives.

**On a worker**: the RESP bytes are forwarded to the master via IPC. The master executes the command, serializes the reply to RESP, and sends it back. The worker deserializes and calls the callback on its IO thread. This is transparent to the caller.

**Never** pass `SUBSCRIBE`, `PSUBSCRIBE`, `UNSUBSCRIBE`, or `PUNSUBSCRIBE` to `fio_redis_send()`. These are managed internally on the subscription connection; sending them via the publishing connection violates the Redis protocol.

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
| New channel subscription | `SUBSCRIBE channel` on `sub_conn` |
| New pattern subscription | `PSUBSCRIBE pattern` on `sub_conn` |
| Channel unsubscribe | `UNSUBSCRIBE channel` on `sub_conn` |
| Pattern unsubscribe | `PUNSUBSCRIBE pattern` on `sub_conn` |
| `fio_pubsub_publish(...)` | `PUBLISH channel message` on `pub_conn` (workers route via IPC) |
| Incoming Redis push message | Re-published locally with `fio_pubsub_engine_ipc()` to fan out to all processes |

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
| `fio_redis_send()` | Queued on `pub_conn` | Serialized → IPC → master → Redis → IPC reply → worker callback |
| `fio_pubsub_publish()` | `PUBLISH` on `pub_conn` | Forwarded via IPC; master sends `PUBLISH` |
| Subscription messages | Received on `sub_conn`, fanned out via Pub/Sub IPC | Delivered by Pub/Sub IPC from master |

Detection uses `fio_io_is_master()`. In single-process mode `fio_io_start(0)` everything uses the master path.

Worker processes inherit the engine pointer after fork. A `FIO_CALL_IN_CHILD` hook swaps the engine's `publish` vtable pointer to the IPC-forwarding implementation, so workers never accidentally write directly to a Redis socket.

See [./404 ipc.md](./404 ipc.md) for IPC internals.

---

## Reconnect and Failure Behavior

- **Connection loss**: The `on_close` callback logs a warning and schedules a reconnect via `fio_io_defer()`. Reconnection is attempted after a brief delay; on failure, it retries again.
- **Queued commands**: Commands in the queue on the master stay queued and are flushed once the publishing connection is re-established.
- **Resubscription**: When the subscription connection reconnects, the engine iterates the current Pub/Sub channel maps and re-sends all active `SUBSCRIBE` / `PSUBSCRIBE` commands directly. This restores subscription state without touching ref counts or re-attaching the engine.
- **Keepalive**: A `PING` is sent on each idle connection after `ping_interval` seconds. If the publishing connection has unacknowledged commands when the timeout fires, the connection is forcibly closed and reconnected.
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
- Redis's numeric filter namespaces (`filter` field in Pub/Sub) are not supported. All Redis pub/sub operates with `filter = 0`.
- Single-node Redis only. Redis Cluster requires connecting to the correct shard or using a proxy.
- Very large individual replies must fit within `FIO_REDIS_READ_BUFFER`. Increase the macro if needed.
