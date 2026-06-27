# Pub/Sub — Publish/Subscribe Messaging (420 pubsub.h)

```c
#define FIO_PUBSUB
#include FIO_INCLUDE_FILE
```

> **Requires:** `FIO_IPC`. When using `include.h` the dependency is resolved automatically. When using the combined `fio-stl.h`, define `FIO_IPC` before `FIO_PUBSUB`.

Pub/Sub delivers messages to named channels across all worker processes and,
through pluggable engines, across machines or external brokers. It is the
broadcast layer that sits directly on top of IPC.

See [./400 io-overview.md](./400 io-overview.md) for where Pub/Sub fits in the
full IO stack, and [./404 ipc.md](./404 ipc.md) for the IPC transport it relies on.

---

## How it fits together

```
 publisher (any thread/process)
         │
         │  fio_pubsub_publish(...)
         ▼
   engine->publish()          ← default: fio_pubsub_engine_ipc()
         │
         │  IPC message (fio_ipc_local / fio_ipc_broadcast)
         ▼
  every process receives it
         │
         ├─ matches channel  → subscriber callbacks scheduled on queue
         ├─ matches patterns → pattern subscriber callbacks scheduled
         └─ history manager  → message stored for replay
```

Workers do not talk to each other directly. Every publish goes through the
master (via IPC), which fans the message out to all processes. Subscriptions
follow the same path in reverse: when a new channel is first subscribed in a
worker, the worker notifies the master so the master can track routing.

---

## Configuration Macros

Define these **before** including the header to override the defaults.

| Macro | Default | Meaning |
|-------|---------|---------|
| `FIO_PUBSUB_FUTURE_LIMIT_MS` | `60000` | Messages timestamped more than this many ms in the future are not delivered to subscribers immediately; they are handed to history managers instead. |
| `FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT` | `1 << 28` (256 MiB) | Default byte ceiling for the built-in in-memory history cache. |

---

## Types

### `fio_pubsub_msg_s` — the message

```c
typedef struct fio_pubsub_msg_s {
  fio_io_s      *io;        /* IO connection owning the subscription (may be NULL) */
  void          *udata;     /* opaque value set on subscribe                        */
  uint64_t       timestamp; /* ms since epoch                                       */
  uint64_t       id;        /* unique message ID                                    */
  fio_buf_info_s channel;   /* channel name  — shared, do NOT mutate               */
  fio_buf_info_s message;   /* message payload — shared, do NOT mutate             */
  int16_t        filter;    /* numerical namespace; negative values are reserved    */
} fio_pubsub_msg_s;
```

Delivered to `on_message` callbacks. The `channel` and `message` buffers are
shared references into the underlying IPC buffer — do not store pointers to
them beyond the callback unless you call `fio_pubsub_msg2ipc` / `fio_ipc_dup`
first.

### `fio_pubsub_subscribe_args_s` — subscribe / unsubscribe arguments

```c
typedef struct {
  fio_io_s      *io;                     /* subscription owner (NULL = global)      */
  fio_buf_info_s channel;                /* channel name or pattern                 */
  void         (*on_message)(fio_pubsub_msg_s *msg); /* required callback          */
  void         (*on_unsubscribe)(void *udata);       /* optional cleanup callback   */
  void          *udata;                  /* opaque user data for callbacks           */
  fio_queue_s   *queue;                  /* callback queue; NULL = IO queue          */
  uintptr_t     *subscription_handle_ptr; /* out: handle for manual management      */
  uint64_t       replay_since;           /* replay history since this ms timestamp   */
  int16_t        filter;                 /* numerical namespace (must match publish) */
  uint8_t        is_pattern;             /* non-zero = channel is a glob pattern     */
  uint8_t        master_only;            /* non-zero = subscription lives in master  */
} fio_pubsub_subscribe_args_s;
```

Both `fio_pubsub_subscribe` and `fio_pubsub_unsubscribe` use this struct. The
macro wrappers let you pass named arguments directly.

**Ownership rules:**

- When `io` is set, the subscription is stored in the IO object's environment
  and cancelled automatically when the connection closes. Only one subscription
  per `(channel, filter, is_pattern)` triple is allowed per IO.
- When `io` is NULL, the subscription is global — one per
  `(channel, filter, is_pattern)` triple for the whole process.
- When `subscription_handle_ptr` is set, the `io` field is ignored and the
  handle is written to the pointer. The caller is responsible for calling
  `fio_pubsub_unsubscribe(.subscription_handle_ptr = &handle)` explicitly.
  Forgetting to do so leaks the subscription.

**If `on_message` is NULL and `io` is set**, the subscription calls
`io->protocol->on_pubsub(msg)` automatically (protocol-level dispatch).

### `fio_pubsub_publish_args_s` — publish arguments

```c
typedef struct {
  fio_pubsub_engine_s const *engine;  /* NULL = default engine                    */
  fio_io_s                  *from;    /* exclude this IO from receiving the message */
  uint64_t                   id;      /* 0 = auto-generate random ID               */
  uint64_t                   timestamp; /* 0 = current reactor tick (ms)           */
  fio_buf_info_s             channel; /* target channel name                       */
  fio_buf_info_s             message; /* message payload                           */
  int16_t                    filter;  /* numerical namespace                        */
} fio_pubsub_publish_args_s;
```

---

## Subscribe / Unsubscribe

### `fio_pubsub_subscribe`

```c
void fio_pubsub_subscribe(fio_pubsub_subscribe_args_s args);
/* macro wrapper — enables named arguments: */
#define fio_pubsub_subscribe(...) \
  fio_pubsub_subscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})
```

Subscribe to a channel. The actual subscription is queued as a deferred task
on the IO thread, so it is safe to call from any thread.

```c
/* Global subscription */
fio_pubsub_subscribe(.channel    = FIO_BUF_INFO1("events"),
                     .on_message = handle_event);

/* Per-connection subscription — auto-cancelled on close */
fio_pubsub_subscribe(.io         = client_io,
                     .channel    = FIO_BUF_INFO1("user:123"),
                     .on_message = handle_user_msg,
                     .udata      = user_ctx);

/* Pattern subscription (glob) */
fio_pubsub_subscribe(.channel    = FIO_BUF_INFO1("chat:*"),
                     .on_message = handle_chat,
                     .is_pattern = 1);

/* Replay history from the last 5 minutes */
fio_pubsub_subscribe(.channel      = FIO_BUF_INFO1("news"),
                     .on_message   = handle_news,
                     .replay_since = fio_io_last_tick() - 5*60*1000);

/* Manual handle for explicit control */
uintptr_t sub_handle = 0;
fio_pubsub_subscribe(.channel                = FIO_BUF_INFO1("ticker"),
                     .on_message             = handle_tick,
                     .subscription_handle_ptr = &sub_handle);
/* … later … */
fio_pubsub_unsubscribe(.subscription_handle_ptr = &sub_handle);
```

> **Note:** pattern subscriptions do not support history replay (`replay_since`
> is silently ignored for patterns).

> **Note:** `master_only = 1` creates the subscription only in the master
> process and is automatically removed from forked workers after `fork()`.

### `fio_pubsub_unsubscribe`

```c
int fio_pubsub_unsubscribe(fio_pubsub_subscribe_args_s args);
/* macro wrapper: */
#define fio_pubsub_unsubscribe(...) \
  fio_pubsub_unsubscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})
```

Remove a subscription. Returns `0` on success, `-1` if not found.

Only `io`, `channel`, `filter`, `is_pattern`, and `subscription_handle_ptr`
are used for matching — `on_message` and `udata` are ignored.

```c
fio_pubsub_unsubscribe(.io      = client_io,
                       .channel = FIO_BUF_INFO1("user:123"));

fio_pubsub_unsubscribe(.channel    = FIO_BUF_INFO1("chat:*"),
                       .is_pattern = 1);
```

---

## Publish

### `fio_pubsub_publish`

```c
void fio_pubsub_publish(fio_pubsub_publish_args_s args);
/* macro wrapper: */
#define fio_pubsub_publish(...) \
  fio_pubsub_publish((fio_pubsub_publish_args_s){__VA_ARGS__})
```

Publish a message. Thread-safe, callable from any process or thread. In
workers the default engine routes the message through IPC to the master, which
fans it out to all processes.

```c
/* Simple publish */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("events"),
                   .message = FIO_BUF_INFO1("something happened"));

/* With namespace filter */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("updates"),
                   .message = FIO_BUF_INFO2(data, data_len),
                   .filter  = 42);

/* Exclude the sender (echo suppression) */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("chat:room1"),
                   .message = FIO_BUF_INFO1("hello"),
                   .from    = sender_io);

/* Cluster-wide (all machines) */
fio_pubsub_publish(.engine  = fio_pubsub_engine_cluster(),
                   .channel = FIO_BUF_INFO1("global"),
                   .message = FIO_BUF_INFO1("cluster alert"));
```

### `fio_pubsub_defer`

```c
void fio_pubsub_defer(fio_pubsub_msg_s *msg);
```

Call from inside `on_message` to push the *same* callback invocation to the
end of the queue. The message and subscription references are managed
automatically.

```c
void on_message(fio_pubsub_msg_s *msg) {
  if (not_ready_yet()) {
    fio_pubsub_defer(msg); /* try again later */
    return;
  }
  /* process msg… */
}
```

---

## Namespace Filters

The `filter` field (`int16_t`) is a numerical namespace. Publishers and
subscribers must use the same `filter` value to match. Negative values are
reserved for internal use.

Filters are useful when two unrelated subsystems share channel names. Using
`filter = 1` for one and `filter = 2` for the other keeps messages isolated
without any naming conventions.

```c
/* System 1 uses filter 1 */
fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("status"), .filter = 1, …);
fio_pubsub_publish  (.channel = FIO_BUF_INFO1("status"), .filter = 1, …);

/* System 2 uses filter 2 — completely separate, same channel name */
fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("status"), .filter = 2, …);
fio_pubsub_publish  (.channel = FIO_BUF_INFO1("status"), .filter = 2, …);
```

---

## IO Subscription Helper

### `FIO_ON_MESSAGE_SEND_MESSAGE`

```c
void FIO_ON_MESSAGE_SEND_MESSAGE(fio_pubsub_msg_s *msg);
```

A ready-made `on_message` callback that writes `msg->message` directly to
`msg->io`. Zero-copy: it duplicates the underlying IPC buffer reference and
hands it to the IO write queue.

```c
/* Wire clients to a channel with one line */
fio_pubsub_subscribe(.io         = client_io,
                     .channel    = FIO_BUF_INFO1("feed"),
                     .on_message = FIO_ON_MESSAGE_SEND_MESSAGE);
```

It can also serve as the protocol-level `on_pubsub` callback:

```c
fio_io_protocol_s MY_PROTOCOL = {
    .on_attach = my_on_attach,
    .on_data   = my_on_data,
    .on_pubsub = FIO_ON_MESSAGE_SEND_MESSAGE,
};
```

---

## Pattern Matching

### `fio_pubsub_match_fn_set`

```c
void fio_pubsub_match_fn_set(uint8_t (*match_cb)(fio_str_info_s pattern,
                                                  fio_str_info_s name));
```

Override the function used to test pattern subscriptions. Pass `NULL` to
restore the default (`fio_glob_match`), which supports `*`, `?`, and `[sets]`.

```c
uint8_t my_regex_match(fio_str_info_s pattern, fio_str_info_s name) {
  /* … custom logic … */
  return matched ? 1 : 0;
}
fio_pubsub_match_fn_set(my_regex_match);

fio_pubsub_match_fn_set(NULL); /* restore glob */
```

---

## Engines

Engines decide *how* messages are distributed. The default engine sends
messages to the local process group (master + all workers on the current
machine). Swap the engine to reach external brokers or remote machines.

### Built-in engines

| Function | Scope |
|----------|-------|
| `fio_pubsub_engine_ipc()` | master + all workers on **this machine** only |
| `fio_pubsub_engine_cluster()` | master + workers on **all machines** in the cluster |

```c
fio_pubsub_engine_s const *fio_pubsub_engine_ipc(void);
fio_pubsub_engine_s const *fio_pubsub_engine_cluster(void);
```

The default engine starts as `fio_pubsub_engine_ipc()` and is automatically
upgraded to `fio_pubsub_engine_cluster()` at `PRE_START` time when
`fio_ipc_cluster_port()` returns a non-zero value (i.e., when cluster IPC is
configured before `fio_io_start`).

### Default engine accessors

```c
fio_pubsub_engine_s const *fio_pubsub_engine_default(void);
fio_pubsub_engine_s const *fio_pubsub_engine_default_set(
    fio_pubsub_engine_s const *engine);
```

`fio_pubsub_engine_default_set(NULL)` restores the auto-selected default
(cluster engine if cluster port is set, IPC engine otherwise).

### Custom engine interface

```c
typedef struct fio_pubsub_engine_s {
  void (*detached)   (const struct fio_pubsub_engine_s *eng);
  void (*subscribe)  (const struct fio_pubsub_engine_s *eng,
                      const fio_buf_info_s channel, int16_t filter);
  void (*psubscribe) (const struct fio_pubsub_engine_s *eng,
                      const fio_buf_info_s channel, int16_t filter);
  void (*unsubscribe)(const struct fio_pubsub_engine_s *eng,
                      const fio_buf_info_s channel, int16_t filter);
  void (*punsubscribe)(const struct fio_pubsub_engine_s *eng,
                       const fio_buf_info_s channel, int16_t filter);
  void (*publish)    (const struct fio_pubsub_engine_s *eng,
                      const fio_pubsub_msg_s *msg);
} fio_pubsub_engine_s;
```

**Execution context:**
- `publish` — called from any thread or process
- all other callbacks — called from the **master process**, **IO thread** only

Callbacks must not block. Defer slow operations with `fio_io_defer`.

Missing callbacks are filled with safe defaults on `fio_pubsub_engine_attach`: subscription callbacks become no-ops; a NULL `publish` falls back to local IPC delivery.

### `fio_pubsub_engine_attach` / `fio_pubsub_engine_detach`

```c
void fio_pubsub_engine_attach(fio_pubsub_engine_s *engine);
void fio_pubsub_engine_detach(fio_pubsub_engine_s *engine);
```

`attach` registers the engine and immediately notifies it of all existing
channel and pattern subscriptions (via `subscribe` / `psubscribe`).

`detach` removes the engine and calls its `detached` callback. If the detached
engine was the current default, the default is automatically reset.

```c
/* Example: a custom engine that bridges to NATS */
static fio_pubsub_engine_s nats_engine = {
    .subscribe   = nats_subscribe,
    .psubscribe  = nats_subscribe,   /* reuse for patterns if needed */
    .unsubscribe = nats_unsubscribe,
    .punsubscribe= nats_unsubscribe,
    .publish     = nats_publish,
    .detached    = nats_cleanup,
};

/* attach before or after fio_io_start — both work */
fio_pubsub_engine_attach(&nats_engine);
fio_pubsub_engine_default_set(&nats_engine);
```

See [./422 redis.md](./422 redis.md) *(planned)* for the built-in Redis engine,
which is a production example of this interface.

---

## History / Replay

The history system caches messages for late-joining subscribers. History lives
in the **master process only** and is separate from the engine interface.

When `replay_since` is set on a subscription, the subscriber receives all
cached messages with `timestamp >= replay_since` before live messages begin.

### Built-in in-memory cache

```c
fio_pubsub_history_s const *fio_pubsub_history_cache(size_t size_limit);
```

Returns the built-in cache manager and sets its byte ceiling. The built-in
cache is initialized by default but **not attached automatically**; call
`fio_pubsub_history_attach(fio_pubsub_history_cache(size), priority)` to begin
storing messages for replay.

`size_limit = 0` inherits from environment variables in order:

1. `WEBSITE_MEMORY_LIMIT_MB` (× 1 MiB)
2. `WEBSITE_MEMORY_LIMIT_KB` (× 1 KiB)
3. `WEBSITE_MEMORY_LIMIT` (bytes)
4. `FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT` (256 MiB)

When the cache overflows its limit, the oldest messages across all channels
are evicted first.

```c
/* Attach the built-in cache with a 64 MiB ceiling */
fio_pubsub_history_attach(fio_pubsub_history_cache(64 * 1024 * 1024), 100);
```

### Attach / detach history managers

```c
int  fio_pubsub_history_attach(const fio_pubsub_history_s *manager,
                               uint8_t priority);
void fio_pubsub_history_detach(const fio_pubsub_history_s *manager);
```

Multiple managers can be active simultaneously. All receive `push()` on every
published message. For replay, managers are consulted in **descending priority
order** via `oldest()` to find a manager with enough history coverage; `replay()`
is called on the selected manager once.

### Custom history manager interface

```c
typedef struct fio_pubsub_history_s {
  void     (*detached)(const struct fio_pubsub_history_s *hist);
  int      (*push)    (const struct fio_pubsub_history_s *hist,
                       fio_pubsub_msg_s *msg);
  int      (*replay)  (const struct fio_pubsub_history_s *hist,
                       fio_buf_info_s channel,
                       int16_t filter,
                       uint64_t since,
                       void (*on_message)(fio_pubsub_msg_s *msg, void *udata),
                       void (*on_done)(void *udata),
                       void *udata);
  uint64_t (*oldest)  (const struct fio_pubsub_history_s *hist,
                       fio_buf_info_s channel,
                       int16_t filter);
} fio_pubsub_history_s;
```

**Execution context:** all callbacks — **master process**, **IO thread** only.
Do not block; use `fio_io_defer` for slow operations.

`replay` **must** call `on_done(udata)` when finished (success or failure) —
the IPC reply mechanism depends on it.

`replay` returns `0` if it handled the request. Returning `-1` does **not**
trigger fallback; use `oldest()` to return `UINT64_MAX` when the manager cannot
cover that channel/filter.

`oldest` returns `UINT64_MAX` if no history is available for the channel.

Missing callbacks are replaced with stubs on `fio_pubsub_history_attach`.

### `fio_pubsub_history_push_all`

```c
void fio_pubsub_history_push_all(fio_pubsub_msg_s *msg);
```

Push a message to all attached history managers without delivering it locally.
Use this from a custom engine when the engine receives a message that should be
stored for replay but is not routed through the normal publish path.

---

## Advanced: IPC Buffer Access

### `fio_pubsub_msg2ipc` / `fio_pubsub_ipc2msg`

```c
fio_ipc_s      *fio_pubsub_msg2ipc(fio_pubsub_msg_s *msg);
fio_pubsub_msg_s fio_pubsub_ipc2msg(fio_ipc_s *ipc);
```

`fio_pubsub_msg2ipc` returns (and detaches) the underlying `fio_ipc_s` for
the message. Use with `fio_ipc_dup` to keep the buffer alive beyond the
callback, enabling true zero-copy deferral.

`fio_pubsub_ipc2msg` is the inverse: reconstruct a `fio_pubsub_msg_s` from a
raw IPC buffer.

> **Fragile.** These functions bypass the normal lifecycle. Incorrect use will
> cause use-after-free bugs. Prefer `fio_pubsub_defer` for simple deferral.

---

## Execution Context Summary

| Operation | Where it runs |
|-----------|---------------|
| `fio_pubsub_publish` | any thread, any process |
| `fio_pubsub_subscribe` / `fio_pubsub_unsubscribe` | any thread, any process |
| `on_message` callback | IO thread, any process (per `queue`) |
| `on_unsubscribe` callback | IO thread, any process |
| Engine `subscribe` / `unsubscribe` / `psubscribe` / `punsubscribe` | **master**, IO thread |
| Engine `publish` | any thread, any process |
| History `push` / `replay` / `oldest` / `detached` | **master**, IO thread |

Callbacks must **not block** — offload slow work with `fio_io_defer` or
route to a background `fio_io_async_s` pool.

---

## Examples

### Chat server — per-connection subscription

```c
#define FIO_PUBSUB
#include FIO_INCLUDE_FILE

static void on_data(fio_io_s *io) {
  char buf[256];
  size_t len = fio_io_read(io, buf, sizeof(buf) - 1);
  if (!len) return;
  /* Publish whatever the client sends to the "chat" channel */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("chat"),
                     .message = FIO_BUF_INFO2(buf, len),
                     .from    = io); /* suppress echo */
}

static void on_attach(fio_io_s *io) {
  /* Every new client gets live chat messages forwarded to it */
  fio_pubsub_subscribe(.io         = io,
                       .channel    = FIO_BUF_INFO1("chat"),
                       .on_message = FIO_ON_MESSAGE_SEND_MESSAGE);
}

static fio_io_protocol_s CHAT_PROTOCOL = {
    .on_attach  = on_attach,
    .on_data    = on_data,
    .on_timeout = fio_io_touch,
};

int main(void) {
  fio_io_listen(.url = "0.0.0.0:3000", .protocol = &CHAT_PROTOCOL);
  fio_io_start(4); /* 4 worker processes */
  return 0;
}
```

### Pattern subscription

```c
static void on_any_chat(fio_pubsub_msg_s *msg) {
  printf("[%.*s]: %.*s\n",
         (int)msg->channel.len, msg->channel.buf,
         (int)msg->message.len, msg->message.buf);
}

fio_pubsub_subscribe(.channel    = FIO_BUF_INFO1("chat:*"),
                     .on_message = on_any_chat,
                     .is_pattern = 1);

/* Matches "chat:room1", "chat:lobby", "chat:anything" */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("chat:room1"),
                   .message = FIO_BUF_INFO1("hello room"));
```

### History replay (late subscriber)

```c
static void on_news(fio_pubsub_msg_s *msg) {
  printf("[%llu] %.*s\n",
         (unsigned long long)msg->timestamp,
         (int)msg->message.len, msg->message.buf);
}

/* Subscribe and receive all messages from the last 10 minutes */
fio_pubsub_subscribe(.channel      = FIO_BUF_INFO1("news"),
                     .on_message   = on_news,
                     .replay_since = fio_io_last_tick() - 10*60*1000);
```

Attach the built-in cache, or add a custom persistence layer:

```c
/* Attach built-in cache with a 32 MiB ceiling */
fio_pubsub_history_attach(fio_pubsub_history_cache(32 * 1024 * 1024), 50);

/* Attach a custom database-backed manager at higher priority */
fio_pubsub_history_attach(&my_db_history_manager, 100);
```

### Cluster-wide publish

```c
int main(void) {
  /* Enable cluster RPC on port 9999 — must be set before fio_io_start */
  fio_ipc_cluster_listen(9999);
  /* Default engine auto-upgrades to cluster engine at PRE_START */

  fio_io_start(4);
  return 0;
}

/* Anywhere, after startup — goes to every machine in the cluster */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("invalidate"),
                   .message = FIO_BUF_INFO1("cache_key_42"));

/* Or explicitly select the engine */
fio_pubsub_publish(.engine  = fio_pubsub_engine_cluster(),
                   .channel = FIO_BUF_INFO1("invalidate"),
                   .message = FIO_BUF_INFO1("cache_key_42"));
```

### Custom engine (skeleton)

```c
static void my_engine_subscribe(const fio_pubsub_engine_s *eng,
                                const fio_buf_info_s channel,
                                int16_t filter) {
  /* tell external broker to subscribe */
  (void)eng; (void)filter;
  external_broker_subscribe(channel.buf, channel.len);
}

static void my_engine_publish(const fio_pubsub_engine_s *eng,
                              const fio_pubsub_msg_s *msg) {
  /* forward to external broker */
  (void)eng;
  external_broker_publish(msg->channel.buf, msg->channel.len,
                          msg->message.buf, msg->message.len);
}

static void my_engine_cleanup(const fio_pubsub_engine_s *eng) {
  (void)eng;
  external_broker_disconnect();
}

static fio_pubsub_engine_s my_engine = {
    .subscribe    = my_engine_subscribe,
    .psubscribe   = my_engine_subscribe, /* same logic for patterns, or differentiate */
    .unsubscribe  = NULL,                /* NULL → filled with no-op on attach         */
    .punsubscribe = NULL,
    .publish      = my_engine_publish,
    .detached     = my_engine_cleanup,
};

/* Register before or after fio_io_start */
fio_pubsub_engine_attach(&my_engine);
fio_pubsub_engine_default_set(&my_engine);
```

---

## Related

- [./400 io-overview.md](./400 io-overview.md) — full IO/IPC/PubSub stack diagram
- [./404 ipc.md](./404 ipc.md) — IPC transport, `fio_ipc_cluster_listen`, `fio_ipc_s` lifetime
- [./422 redis.md](./422 redis.md) *(planned)* — Redis as a pub/sub engine and command client
