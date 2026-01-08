## Pub/Sub - Publish/Subscribe Messaging

```c
#define FIO_PUBSUB
#include FIO_INCLUDE_FILE
```

The Pub/Sub module provides a publish/subscribe messaging system for facil.io applications. It enables decoupled communication between components through named channels, with support for pattern matching, message history, and cluster-wide distribution.

**Note**: The Pub/Sub module requires the IPC module (`FIO_IPC`) for inter-process communication. When using `include.h`, this dependency is handled automatically.

### Key Features

- **Channel-based messaging**: Subscribe to named channels and receive messages published to those channels
- **Pattern subscriptions**: Subscribe using glob patterns to match multiple channels
- **Process-local and cluster-wide**: Messages can be distributed to local workers or across all machines in a cluster
- **Message history**: Built-in caching with replay support for late-joining subscribers
- **IO integration**: Subscriptions can be tied to IO connections for automatic cleanup
- **Custom engines**: Extensible architecture for integrating external message brokers

### Pub/Sub Types

#### `fio_pubsub_msg_s`

```c
typedef struct fio_pubsub_msg_s {
  /** A connection (if any) to which the subscription belongs. */
  fio_io_s *io;
  /** The `udata` argument associated with the subscription. */
  void *udata;
  /** Milliseconds since epoch. */
  uint64_t timestamp;
  /** Message ID. */
  uint64_t id;
  /** Channel name (shared copy - do NOT mutate). */
  fio_buf_info_s channel;
  /** Message payload (shared copy - do NOT mutate). */
  fio_buf_info_s message;
  /** Channel namespace. Negative values are reserved. */
  int16_t filter;
} fio_pubsub_msg_s;
```

The message structure received by subscription callbacks.

**Members:**
- `io` - The IO connection associated with the subscription (may be NULL)
- `udata` - User data associated with the subscription
- `timestamp` - Message timestamp in milliseconds since epoch
- `id` - Unique message identifier
- `channel` - Channel name buffer (read-only, do not modify)
- `message` - Message payload buffer (read-only, do not modify)
- `filter` - Numerical namespace filter (negative values reserved for internal use)

**Note**: The `channel` and `message` buffers are shared copies. Do NOT mutate them.

#### `fio_pubsub_subscribe_args_s`

```c
typedef struct {
  /** The subscription owner - if none, the subscription is owned by the system. */
  fio_io_s *io;
  /** A named channel to which the message was sent. */
  fio_buf_info_s channel;
  /** The callback to be called for each message forwarded to the subscription. */
  void (*on_message)(fio_pubsub_msg_s *msg);
  /** An optional callback for when a subscription is canceled. */
  void (*on_unsubscribe)(void *udata);
  /** The opaque udata value is ignored and made available to the callbacks. */
  void *udata;
  /** The queue to which the callbacks should be routed. May be NULL. */
  fio_queue_s *queue;
  /** OPTIONAL: subscription handle return value for manual management. */
  uintptr_t *subscription_handle_ptr;
  /** Replay cached messages (if any) since supplied time in milliseconds. */
  uint64_t replay_since;
  /** A numerical namespace filter subscribers need to match. */
  int16_t filter;
  /** If set, pattern matching will be used (name is a pattern). */
  uint8_t is_pattern;
  /** If set, subscription will be limited to the root / master process. */
  uint8_t master_only;
} fio_pubsub_subscribe_args_s;
```

Arguments for subscribing to a channel.

**Members:**
- `io` - Optional IO connection owner. If set, subscription is cleaned up when the IO closes
- `channel` - Channel name to subscribe to
- `on_message` - Callback invoked for each received message
- `on_unsubscribe` - Optional callback when subscription is canceled
- `udata` - User data passed to callbacks
- `queue` - Optional queue for callback execution (defaults to IO queue)
- `subscription_handle_ptr` - If set, returns handle for manual subscription management
- `replay_since` - Request message replay from history since this timestamp
- `filter` - Numerical namespace filter (must match publisher's filter)
- `is_pattern` - If true, channel name is treated as a glob pattern
- `master_only` - If true, subscription exists only in the master process

**Note**: When `io` is set, only one subscription per channel/filter/pattern combination is allowed per IO. When `io` is NULL, the subscription is global.

#### `fio_pubsub_publish_args_s`

```c
typedef struct {
  struct fio_pubsub_engine_s const *engine;
  fio_io_s *from;
  uint64_t id;
  uint64_t timestamp;
  fio_buf_info_s channel;
  fio_buf_info_s message;
  int16_t filter;
} fio_pubsub_publish_args_s;
```

Arguments for publishing a message.

**Members:**
- `engine` - Publishing engine (NULL = default engine)
- `from` - Optional source IO (excluded from receiving the message)
- `id` - Optional message ID (0 = auto-generate)
- `timestamp` - Optional timestamp (0 = current time)
- `channel` - Channel name to publish to
- `message` - Message payload
- `filter` - Numerical namespace filter

### Subscribe / Unsubscribe

#### `fio_pubsub_subscribe`

```c
void fio_pubsub_subscribe(fio_pubsub_subscribe_args_s args);
#define fio_pubsub_subscribe(...)                                              \
  fio_pubsub_subscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})
```

Subscribe to a channel.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
/* Simple subscription */
fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("events"),
                     .on_message = handle_event);

/* Subscription tied to an IO connection */
fio_pubsub_subscribe(.io = client_io,
                     .channel = FIO_BUF_INFO1("user:123"),
                     .on_message = handle_user_message,
                     .udata = user_context);

/* Pattern subscription */
fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("chat:*"),
                     .on_message = handle_chat,
                     .is_pattern = 1);

/* Subscription with history replay */
fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("news"),
                     .on_message = handle_news,
                     .replay_since = last_seen_timestamp);
```

**Note**: In worker processes, subscriptions are tracked by the master process for proper message routing.

#### `fio_pubsub_unsubscribe`

```c
int fio_pubsub_unsubscribe(fio_pubsub_subscribe_args_s args);
#define fio_pubsub_unsubscribe(...)                                            \
  fio_pubsub_unsubscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})
```

Unsubscribe from a channel.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
/* Unsubscribe from a channel */
fio_pubsub_unsubscribe(.io = client_io,
                       .channel = FIO_BUF_INFO1("user:123"),
                       .filter = 0);
```

**Returns:** 0 on success, -1 if subscription not found.

**Note**: The `on_message` callback is ignored during unsubscription. Only `io`, `channel`, `filter`, and `is_pattern` are used for matching.

### Publish

#### `fio_pubsub_publish`

```c
void fio_pubsub_publish(fio_pubsub_publish_args_s args);
#define fio_pubsub_publish(...)                                                \
  fio_pubsub_publish((fio_pubsub_publish_args_s){__VA_ARGS__})
```

Publish a message to a channel.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
/* Simple publish */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("events"),
                   .message = FIO_BUF_INFO1("something happened"));

/* Publish with filter namespace */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("updates"),
                   .message = FIO_BUF_INFO2(data, data_len),
                   .filter = 42);

/* Publish excluding sender */
fio_pubsub_publish(.channel = FIO_BUF_INFO1("chat:room1"),
                   .message = FIO_BUF_INFO1("hello"),
                   .from = sender_io);

/* Publish via cluster engine (all machines) */
fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                   .channel = FIO_BUF_INFO1("global"),
                   .message = FIO_BUF_INFO1("cluster-wide message"));
```

**Note**: The default engine is `fio_pubsub_engine_ipc()` (local machine only). Use `fio_pubsub_engine_cluster()` for cluster-wide distribution.

#### `fio_pubsub_defer`

```c
void fio_pubsub_defer(fio_pubsub_msg_s *msg);
```

Pushes execution of the `on_message` callback to the end of the queue.

Call this from within an `on_message` callback to defer processing:

```c
void handle_message(fio_pubsub_msg_s *msg) {
  if (should_defer()) {
    fio_pubsub_defer(msg);
    return;
  }
  // Process message...
}
```

**Note**: The message and subscription references are automatically managed.

### IO Callback Helper

#### `FIO_ON_MESSAGE_SEND_MESSAGE`

```c
void FIO_ON_MESSAGE_SEND_MESSAGE(fio_pubsub_msg_s *msg);
```

A pre-built callback for IO subscriptions that sends the raw message data to the IO connection.

Use this as the `on_pubsub` callback in your protocol or as the `on_message` callback for subscriptions:

```c
/* In protocol definition */
fio_io_protocol_s MY_PROTOCOL = {
    .on_attach = my_on_attach,
    .on_data = my_on_data,
    .on_pubsub = FIO_ON_MESSAGE_SEND_MESSAGE,  /* Auto-send messages to client */
};

/* Or in subscription */
fio_pubsub_subscribe(.io = client_io,
                     .channel = FIO_BUF_INFO1("updates"),
                     .on_message = FIO_ON_MESSAGE_SEND_MESSAGE);
```

### Engines

Engines control how messages are distributed. The module provides two built-in engines:

#### `fio_pubsub_engine_ipc`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_ipc(void);
```

Returns the built-in engine for publishing to the local process group (master + workers).

Messages are distributed via IPC to all processes on the local machine only.

**Returns:** Pointer to the IPC engine.

#### `fio_pubsub_engine_cluster`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_cluster(void);
```

Returns the built-in engine for multi-machine cluster publishing.

Messages are distributed to all processes on all machines in the cluster. Requires `fio_ipc_cluster_listen()` to be called for cluster connectivity.

**Returns:** Pointer to the cluster engine.

#### `fio_pubsub_engine_default`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_default(void);
```

Returns the current default engine.

**Returns:** Pointer to the current default engine.

**Note**: The default engine is automatically set to the cluster engine when `fio_ipc_cluster_listen()` is called before `fio_io_start()`.

#### `fio_pubsub_engine_default_set`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_default_set(
    fio_pubsub_engine_s const *engine);
```

Sets the default engine for publishing.

**Parameters:**
- `engine` - The engine to use as default (NULL restores to IPC engine)

**Returns:** The engine that was set.

```c
/* Use cluster engine by default */
fio_pubsub_engine_default_set(fio_pubsub_engine_cluster());

/* Restore to local IPC */
fio_pubsub_engine_default_set(NULL);
```

### Custom Engines

#### `fio_pubsub_engine_s`

```c
typedef struct fio_pubsub_engine_s {
  /** Called when engine is detached */
  void (*detached)(const struct fio_pubsub_engine_s *eng);
  /** Called when a subscription is created */
  void (*subscribe)(const struct fio_pubsub_engine_s *eng,
                    const fio_buf_info_s channel,
                    int16_t filter);
  /** Called when a pattern subscription is created */
  void (*psubscribe)(const struct fio_pubsub_engine_s *eng,
                     const fio_buf_info_s channel,
                     int16_t filter);
  /** Called when a subscription is removed */
  void (*unsubscribe)(const struct fio_pubsub_engine_s *eng,
                      const fio_buf_info_s channel,
                      int16_t filter);
  /** Called when a pattern subscription is removed */
  void (*punsubscribe)(const struct fio_pubsub_engine_s *eng,
                       const fio_buf_info_s channel,
                       int16_t filter);
  /** Called when a message is published */
  void (*publish)(const struct fio_pubsub_engine_s *eng,
                  const fio_pubsub_msg_s *msg);
} fio_pubsub_engine_s;
```

Engine structure for external pub/sub backends (e.g., Redis, NATS).

**Callbacks:**
- `detached` - Called when the engine is detached from the system
- `subscribe` - Called when a new channel subscription is created
- `psubscribe` - Called when a new pattern subscription is created
- `unsubscribe` - Called when a channel subscription is removed
- `punsubscribe` - Called when a pattern subscription is removed
- `publish` - Called when a message is published via this engine

**Execution Context:**
- The `publish` callback can be called from any thread/process
- Subscription callbacks are called from the MASTER process only
- Subscription callbacks are called from the main event loop thread
- Callbacks MUST NOT block (defer long operations)

#### `fio_pubsub_engine_attach`

```c
void fio_pubsub_engine_attach(fio_pubsub_engine_s *engine);
```

Attach an engine to the pub/sub system.

**Parameters:**
- `engine` - The engine to attach

The engine will be notified of all existing subscriptions upon attachment.

**Note**: Missing callbacks are automatically filled with no-op defaults.

#### `fio_pubsub_engine_detach`

```c
void fio_pubsub_engine_detach(fio_pubsub_engine_s *engine);
```

Detach an engine from the pub/sub system.

**Parameters:**
- `engine` - The engine to detach

The engine's `detached` callback will be called after removal.

### History Management

The history system allows caching messages for replay to late-joining subscribers.

#### `fio_pubsub_history_s`

```c
typedef struct fio_pubsub_history_s {
  /** Cleanup callback - called when history manager is detached */
  void (*detached)(const struct fio_pubsub_history_s *hist);
  /** Stores a message in history. Returns 0 on success, -1 on error. */
  int (*push)(const struct fio_pubsub_history_s *hist, fio_pubsub_msg_s *msg);
  /** Replay messages since timestamp. Returns 0 if handled, -1 if cannot replay. */
  int (*replay)(const struct fio_pubsub_history_s *hist,
                fio_buf_info_s channel,
                int16_t filter,
                uint64_t since,
                void (*on_message)(fio_pubsub_msg_s *msg, void *udata),
                void (*on_done)(void *udata),
                void *udata);
  /** Get oldest available timestamp. Returns UINT64_MAX if no history. */
  uint64_t (*oldest)(const struct fio_pubsub_history_s *hist,
                     fio_buf_info_s channel,
                     int16_t filter);
} fio_pubsub_history_s;
```

History storage interface for message caching and replay.

**Callbacks:**
- `detached` - Called when the history manager is detached
- `push` - Store a message in history (called on publish)
- `replay` - Replay messages since a timestamp to a callback
- `oldest` - Get the oldest available message timestamp

**Execution Context:**
- All callbacks are called from the MASTER process only
- Callbacks are called from the main event loop thread
- Callbacks MUST NOT block (defer long operations)

#### `fio_pubsub_history_attach`

```c
int fio_pubsub_history_attach(const fio_pubsub_history_s *manager, uint8_t priority);
```

Attach a history manager with the given priority.

**Parameters:**
- `manager` - The history manager to attach
- `priority` - Priority level (higher = tried first for replay)

**Returns:** 0 on success, -1 on error.

Multiple history managers can be attached. All managers receive `push()` calls. For `replay()`, managers are tried in priority order until one handles the request.

#### `fio_pubsub_history_detach`

```c
void fio_pubsub_history_detach(const fio_pubsub_history_s *manager);
```

Detach a history manager.

**Parameters:**
- `manager` - The history manager to detach

#### `fio_pubsub_history_cache`

```c
const fio_pubsub_history_s *fio_pubsub_history_cache(size_t size_limit);
```

Get the built-in in-memory history manager.

**Parameters:**
- `size_limit` - Maximum cache size in bytes (0 = use default)

**Returns:** Pointer to the built-in cache history manager.

The default size limit is determined by:
1. `WEBSITE_MEMORY_LIMIT_MB` environment variable (in MB)
2. `WEBSITE_MEMORY_LIMIT_KB` environment variable (in KB)
3. `WEBSITE_MEMORY_LIMIT` environment variable (in bytes)
4. `FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT` (256 MB)

```c
/* Use built-in cache with 64MB limit */
fio_pubsub_history_attach(fio_pubsub_history_cache(64 * 1024 * 1024), 100);
```

#### `fio_pubsub_history_push_all`

```c
void fio_pubsub_history_push_all(fio_pubsub_msg_s *msg);
```

Pushes a message to all history containers.

Use this from a custom engine if the message needs to be saved to history but is never delivered locally.

**Parameters:**
- `msg` - The message to push to history

### Pattern Matching

#### `fio_pubsub_match_fn_set`

```c
void fio_pubsub_match_fn_set(uint8_t (*match_cb)(fio_str_info_s pattern,
                                                  fio_str_info_s name));
```

Sets the pattern matching function for pattern subscriptions.

**Parameters:**
- `match_cb` - Pattern matching function (NULL restores default)

**Returns:** Nothing.

The default pattern matching function is `fio_glob_match`, which supports:
- `*` - Match any sequence of characters
- `?` - Match any single character
- `[abc]` - Match any character in the set
- `[a-z]` - Match any character in the range

```c
/* Use custom pattern matching */
uint8_t my_matcher(fio_str_info_s pattern, fio_str_info_s name) {
  // Custom matching logic...
  return matches ? 1 : 0;
}
fio_pubsub_match_fn_set(my_matcher);

/* Restore default glob matching */
fio_pubsub_match_fn_set(NULL);
```

### Advanced: IPC Message Access

#### `fio_pubsub_msg2ipc`

```c
fio_ipc_s *fio_pubsub_msg2ipc(fio_pubsub_msg_s *msg);
```

Returns the underlying IPC message buffer carrying the message data.

**Parameters:**
- `msg` - The pub/sub message

**Returns:** Pointer to the underlying IPC message.

This allows message deferral (use `fio_ipc_dup`) and tighter control over the message's lifetime.

**Note**: The IPC message is detached from its originating IO.

#### `fio_pubsub_ipc2msg`

```c
fio_pubsub_msg_s fio_pubsub_ipc2msg(fio_ipc_s *ipc);
```

Extract a pub/sub message from an IPC message.

**Parameters:**
- `ipc` - The IPC message

**Returns:** A `fio_pubsub_msg_s` structure with fields populated from the IPC message.

### Configuration Macros

#### `FIO_PUBSUB_FUTURE_LIMIT_MS`

```c
#ifndef FIO_PUBSUB_FUTURE_LIMIT_MS
#define FIO_PUBSUB_FUTURE_LIMIT_MS 60000ULL
#endif
```

Maximum time in milliseconds to allow "future" messages to be delivered.

Messages with timestamps beyond this limit are not delivered to subscribers but are still pushed to history managers for future delivery.

#### `FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT`

```c
#ifndef FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT
#define FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT (1ULL << 28)
#endif
```

Default cache size limit for the built-in history manager (256 MB).

### Examples

#### Basic Pub/Sub

```c
#define FIO_LOG
#define FIO_PUBSUB
#include FIO_INCLUDE_FILE

void on_message(fio_pubsub_msg_s *msg) {
  printf("Received on '%.*s': %.*s\n",
         (int)msg->channel.len, msg->channel.buf,
         (int)msg->message.len, msg->message.buf);
}

void on_start(void *arg) {
  (void)arg;
  /* Subscribe to channel */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("events"),
                       .on_message = on_message);
}

int publish_event(void *a, void *b) {
  (void)a; (void)b;
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("events"),
                     .message = FIO_BUF_INFO1("hello world"));
  return -1;  /* One-shot timer */
}

int main(void) {
  fio_state_callback_add(FIO_CALL_ON_START, on_start, NULL);
  fio_io_run_every(.fn = publish_event, .every = 100, .repetitions = 1);
  fio_io_start(2);
  return 0;
}
```

#### Time Server with Pub/Sub

```c
#define FIO_LOG
#define FIO_PUBSUB
#define FIO_TIME
#include FIO_INCLUDE_FILE

/* Protocol subscribes clients to time channel */
void time_on_attach(fio_io_s *io) {
  fio_pubsub_subscribe(.io = io,
                       .channel = FIO_BUF_INFO1("time"),
                       .on_message = FIO_ON_MESSAGE_SEND_MESSAGE);
}

fio_io_protocol_s TIME_PROTOCOL = {
    .on_attach = time_on_attach,
    .on_timeout = fio_io_touch,
};

/* Timer publishes current time */
int publish_time(void *a, void *b) {
  (void)a; (void)b;
  char buf[32];
  size_t len = fio_time2iso(buf, fio_time_real().tv_sec);
  buf[len++] = '\r';
  buf[len++] = '\n';
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("time"),
                     .message = FIO_BUF_INFO2(buf, len));
  return 0;
}

int main(void) {
  fio_io_run_every(.fn = publish_time, .every = 1000, .repetitions = -1);
  fio_io_listen(.protocol = &TIME_PROTOCOL);
  fio_io_start(0);
  return 0;
}
```

#### Pattern Subscriptions

```c
#define FIO_PUBSUB
#include FIO_INCLUDE_FILE

void on_chat_message(fio_pubsub_msg_s *msg) {
  printf("Chat [%.*s]: %.*s\n",
         (int)msg->channel.len, msg->channel.buf,
         (int)msg->message.len, msg->message.buf);
}

void setup_subscriptions(void *arg) {
  (void)arg;
  /* Subscribe to all chat rooms using pattern */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("chat:*"),
                       .on_message = on_chat_message,
                       .is_pattern = 1);
}

void send_chat(const char *room, const char *message) {
  char channel[64];
  int len = snprintf(channel, sizeof(channel), "chat:%s", room);
  fio_pubsub_publish(.channel = FIO_BUF_INFO2(channel, (size_t)len),
                     .message = FIO_BUF_INFO1(message));
}

int main(void) {
  fio_state_callback_add(FIO_CALL_ON_START, setup_subscriptions, NULL);
  fio_io_start(2);
  return 0;
}
```

#### Cluster-Wide Pub/Sub

```c
#define FIO_PUBSUB
#include FIO_INCLUDE_FILE

void on_cluster_message(fio_pubsub_msg_s *msg) {
  printf("[%d] Cluster message: %.*s\n",
         fio_io_pid(),
         (int)msg->message.len, msg->message.buf);
}

void setup(void *arg) {
  (void)arg;
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("cluster-events"),
                       .on_message = on_cluster_message);
}

int main(void) {
  /* Enable cluster communication */
  fio_ipc_cluster_listen(9999);
  
  /* Default engine will be cluster engine */
  fio_state_callback_add(FIO_CALL_ON_START, setup, NULL);
  fio_io_start(4);
  return 0;
}
```

#### History Replay

```c
#define FIO_PUBSUB
#include FIO_INCLUDE_FILE

void on_message(fio_pubsub_msg_s *msg) {
  printf("[%llu] %.*s\n",
         (unsigned long long)msg->timestamp,
         (int)msg->message.len, msg->message.buf);
}

void late_subscriber(void *arg) {
  (void)arg;
  uint64_t five_minutes_ago = fio_io_last_tick() - (5 * 60 * 1000);
  
  /* Subscribe and replay messages from the last 5 minutes */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("news"),
                       .on_message = on_message,
                       .replay_since = five_minutes_ago);
}

int main(void) {
  /* Attach built-in cache history manager */
  fio_pubsub_history_attach(fio_pubsub_history_cache(0), 100);
  
  fio_state_callback_add(FIO_CALL_ON_START, late_subscriber, NULL);
  fio_io_start(2);
  return 0;
}
```

### Thread Safety and Execution Context

- `fio_pubsub_subscribe`, `fio_pubsub_unsubscribe`, and `fio_pubsub_publish` are thread-safe
- Subscription callbacks (`on_message`, `on_unsubscribe`) execute on the IO reactor thread
- Engine callbacks execute on the MASTER process only
- History manager callbacks execute on the MASTER process only
- Callbacks MUST NOT block - defer long operations using `fio_io_defer`

### Migration from Previous API

The Pub/Sub module was completely rewritten. Key changes:

| Old API | New API |
|---------|---------|
| `fio_msg_s` | `fio_pubsub_msg_s` |
| `msg->published` | `msg->timestamp` |
| `msg->is_json` | Removed (deprecated) |
| `fio_subscribe()` | `fio_pubsub_subscribe()` |
| `fio_unsubscribe()` | `fio_pubsub_unsubscribe()` |
| `fio_publish()` | `fio_pubsub_publish()` |
| `FIO_PUBSUB_ROOT` | Use `fio_pubsub_engine_ipc()` |
| `FIO_PUBSUB_CLUSTER` | Use `fio_pubsub_engine_cluster()` |
| `fio_pubsub_ipc_url_set()` | Use `fio_ipc_url_set()` |
| `fio_pubsub_broadcast_on_port()` | Use `fio_ipc_cluster_listen()` |

------------------------------------------------------------
