## Pub/Sub

```c
#define FIO_PUBSUB
#include "fio-stl.h"
```

By defining `FIO_PUBSUB`, a Publisher / Subscriber extension can be added to the `FIO_SERVER`, resulting in powerful IPC and real-time data updates.

The [pub/sub paradigm](https://en.wikipedia.org/wiki/Publish–subscribe_pattern) allows for any number of real-time applications, including message-bus backends, chat applications (private / group chats), broadcasting, games, etc'.

### Paradigm

Publishers publish messages to delivery Channels, without any information about the potential recipients (if any).

Subscribers "listen" to messages from specific delivery Channels without any information about the publishers (if any).

Messages are broadcasted through delivery Channels to the different Subscribers.

Delivery Channels in facil.io are a combination of a named channel and a 16 bit numerical filter, allowing for 32,768 namespaces of named channels (negative numbers are reserved).

### Limitations

The internal Pub/Sub Letter Exchange Protocol imposes the following limitations on message exchange:

* Distribution Channel Names are limited to 2^16 bytes (65,536 bytes).

* Message payload is limited to 2^24 bytes (16,777,216 bytes == about 16Mb).

* Empty messages (no numerical filters, no channel, no message payload, no flags) are ignored.

* Subscriptions match delivery matches by both channel name (or pattern) and the numerical filter.

### Subscriptions - Receiving Messages

#### `fio_subscribe`

```c
void fio_subscribe(fio_subscribe_args_s args);
#define fio_subscribe(...) fio_subscribe((fio_subscribe_args_s){__VA_ARGS__})
```

Subscribes to a channel / filter pair.

The `on_unsubscribe` callback will be called on failure.

The `fio_subscribe` macro shadows the `fio_subscribe` function and allows the following named arguments to be set:

```c
typedef struct {
  /**
   * The subscription owner - if none, the subscription is owned by the system.
   *
   * Note:
   *
   * Both the system and the `io` objects each manage channel listing
   * which allows only a single subscription to the same channel.
   *
   * This means a single subscription per channel per IO and a single
   * subscription per channel for the global system unless managing the
   * subscription handle manually.
   */
  fio_io_s *io;
  /**
   * A named `channel` to which the message was sent.
   *
   * Subscriptions require a match by both channel name and namespace filter.
   */
  fio_buf_info_s channel;
  /**
   * The callback to be called for each message forwarded to the subscription.
   */
  void (*on_message)(fio_msg_s *msg);
  /** An optional callback for when a subscription is canceled. */
  void (*on_unsubscribe)(void *udata);
  /** The opaque udata value is ignored and made available to the callbacks. */
  void *udata;
  /** The queue to which the callbacks should be routed. May be NULL. */
  fio_queue_s *queue;
  /** Replay cached messages (if any) since supplied time in milliseconds. */
  uint64_t replay_since;
  /**
   * OPTIONAL: subscription handle return value - should be NULL when using
   * automatic memory management with the IO or global environment.
   *
   * When set, the `io` pointer will be ignored and the subscription object
   * handle will be written to the `subscription_handle_ptr` which MUST be
   * used when unsubscribing.
   *
   * NOTE: this could cause subscriptions and memory leaks unless properly
   * handled.
   */
  uintptr_t *subscription_handle_ptr;
  /**
   * A numerical namespace `filter` subscribers need to match.
   *
   * Negative values are reserved for facil.io framework extensions.
   *
   * Filer channels are bound to the processes and workers, they are NOT
   * forwarded to engines and can be used for inter process communication (IPC).
   */
  int16_t filter;
  /** If set, pattern matching will be used (name is a pattern). */
  uint8_t is_pattern;
  /** If set, subscription will be limited to the root / master process. */
  uint8_t master_only;
} fio_subscribe_args_s;
```

The `fio_msg_s` struct in the `on_message` callback contains the following information:

```c
typedef struct fio_msg_s {
  /** A connection (if any) to which the subscription belongs. */
  fio_io_s *io;
  /** The `udata` argument associated with the subscription. */
  void *udata;
  /** Message ID. */
  uint64_t id;
  /** Milliseconds since epoch. */
  uint64_t published;
  /**
   * A channel name, allowing for pub/sub patterns.
   *
   * NOTE: this is a shared copy - do NOT mutate the channel name string.
   */
  fio_buf_info_s channel;
  /**
   * The actual message.
   *
   * NOTE: this is a shared copy - do NOT mutate the message payload string.
   **/
  fio_buf_info_s message;
  /** Channel name namespace. Negative values are reserved. */
  int16_t filter;
  /** flag indicating if the message is JSON data or binary/text. */
  uint8_t is_json;
} fio_msg_s;
```

#### `fio_unsubscribe`

```c
int fio_unsubscribe(fio_subscribe_args_s args);
#define fio_unsubscribe(...) fio_unsubscribe((fio_subscribe_args_s){__VA_ARGS__})
```

Cancels an existing subscriptions.

Accepts the same arguments as [`fio_subscribe`](#fio_subscribe), except the `udata`, and callback details are ignored (no need to provide `udata` or callback details).

If a `subscription_handle_ptr` was provided it should contain the value of the subscription handle returned.

Returns -1 if the subscription could not be found. Otherwise returns 0.

The `fio_unsubscribe` macro shadows the `fio_unsubscribe` function and allows the same named arguments as the [`fio_subscribe`](#fio_subscribe) function.

#### `fio_pubsub_message_defer`

```c
void fio_pubsub_message_defer(fio_msg_s *msg);
```

Defers the current callback, so it will be called again for the same message.

After calling this function, the `msg` object must NOT be accessed again.

#### `FIO_PUBSUB_PATTERN_MATCH`

```c
extern uint8_t (*FIO_PUBSUB_PATTERN_MATCH)(fio_str_info_s pattern,
                                           fio_str_info_s channel);
```

A global variable controlling the pattern matching callback used for pattern matching.

The callback set **must** return 1 on a match or 0 if the string does not match the pattern.

By default, the value is set to `fio_glob_match` (see facil.io's C STL).

#### `FIO_ON_MESSAGE_SEND_MESSAGE`

```c
void FIO_ON_MESSAGE_SEND_MESSAGE(fio_msg_s *msg);
```

A callback for IO subscriptions that sends raw message data.

This can be used as the `on_message` callback when subscribing to forward the message payload directly to an IO connection.

### Publishing to Subscribers

#### `fio_publish`

```c
void fio_publish(fio_publish_args_s args);
#define fio_publish(...) fio_publish((fio_publish_args_s){__VA_ARGS__})
```

Publishes a message to the relevant subscribers (if any).

By default the message is sent using the `FIO_PUBSUB_DEFAULT` engine (set by default to `FIO_PUBSUB_CLUSTER` which publishes to all processes and connected cluster peers).

To limit the message only to other processes (exclude the calling process), use the `FIO_PUBSUB_SIBLINGS` engine.

To limit the message only to the calling process, use the `FIO_PUBSUB_PROCESS` engine.

To limit the message only to the root process, use the `FIO_PUBSUB_ROOT` engine.

To limit the message to local processes only (no cluster peers), use the `FIO_PUBSUB_LOCAL` engine.

The `fio_publish` macro shadows the `fio_publish` function and allows the following named arguments to be set:

```c
typedef struct fio_publish_args_s {
  /** The pub/sub engine that should be used to forward this message. */
  fio_pubsub_engine_s const *engine;
  /** If `from` is specified, it will be skipped (won't receive message)
   *  UNLESS a non-native `engine` is specified. */
  fio_io_s *from;
  /** Message ID (if missing, a random ID will be generated). */
  uint64_t id;
  /** Milliseconds since epoch (if missing, defaults to "now"). */
  uint64_t published;
  /** The target named channel. */
  fio_buf_info_s channel;
  /** The message body / content. */
  fio_buf_info_s message;
  /** A numeral namespace for channel names. Negative values are reserved. */
  int16_t filter;
  /** A flag indicating if the message is JSON data or not. */
  uint8_t is_json;
} fio_publish_args_s;
```

### History and Message Buffering

The pub/sub system supports optional message history that allows late-joining subscribers to replay messages published before their subscription.

#### Overview

History functionality enables:

* **Catch-up subscriptions**: Subscribers can replay missed messages using `replay_since` timestamp
* **Per-channel buffering**: Each channel maintains its own message history
* **Configurable retention**: Control history size by message count and/or age
* **Master-only storage**: History is stored only in the master process to avoid duplication
* **Worker IPC replay**: Worker processes request history from master via IPC
* **Automatic eviction**: Old messages are lazily evicted based on count and age limits

#### Storage Architecture

History storage has these key characteristics:

* **Master-only storage**: Only the master process (`fio_io_is_master()`) stores history to prevent memory duplication across worker processes
* **Per-channel linked lists**: Each channel maintains its own FIFO list of messages
* **Reference counting**: Messages are reference-counted and shared between history and active subscriptions
* **Lazy eviction**: Old messages are evicted during publish, not on a timer (O(1) operation)
* **Oldest timestamp cache**: Each channel caches the oldest message timestamp for O(1) lookup

#### Configuration

History can be configured globally with defaults that apply to all channels, and then overridden on a per-channel basis.

##### Configuration Macros

```c
#ifndef FIO_PUBSUB_HISTORY_DEFAULT_MAX_MESSAGES
#define FIO_PUBSUB_HISTORY_DEFAULT_MAX_MESSAGES 1024
#endif

#ifndef FIO_PUBSUB_HISTORY_DEFAULT_MAX_AGE_MS
#define FIO_PUBSUB_HISTORY_DEFAULT_MAX_AGE_MS 3600000ULL /* 1 hour */
#endif
```

These macros define the default limits when history is enabled without explicit configuration.

#### `fio_pubsub_history_enable`

```c
void fio_pubsub_history_enable(fio_pubsub_history_config_s config);
/* Named arguments using macro. */
#define fio_pubsub_history_enable(...) \
  fio_pubsub_history_enable((fio_pubsub_history_config_s){__VA_ARGS__})

typedef struct {
  size_t max_messages; /* 0 = default (1024) */
  uint64_t max_age_ms; /* 0 = default (3600000 = 1 hour) */
} fio_pubsub_history_config_s;
```

Enables history globally for all channels with the specified configuration.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
/* Enable with defaults */
fio_pubsub_history_enable(0);

/* Enable with custom limits */
fio_pubsub_history_enable(.max_messages = 500, .max_age_ms = 300000);
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `max_messages` | `size_t` | Maximum messages per channel; 0 uses `FIO_PUBSUB_HISTORY_DEFAULT_MAX_MESSAGES` (1024) |
| `max_age_ms` | `uint64_t` | Maximum message age in milliseconds; 0 uses `FIO_PUBSUB_HISTORY_DEFAULT_MAX_AGE_MS` (1 hour) |

**Note**: History is stored only in the master process. Worker processes request history from the master via IPC when a subscription with `replay_since` is created.

**Note**: Both limits are enforced - messages are evicted when either limit is exceeded.

#### `fio_pubsub_history_disable`

```c
void fio_pubsub_history_disable(void);
```

Disables history globally and frees all cached messages.

This clears history from all channels and prevents new messages from being stored.

#### `fio_pubsub_history_channel_set`

```c
int fio_pubsub_history_channel_set(fio_pubsub_history_channel_args_s args);
/* Named arguments using macro. */
#define fio_pubsub_history_channel_set(...) \
  fio_pubsub_history_channel_set((fio_pubsub_history_channel_args_s){__VA_ARGS__})

typedef struct {
  fio_buf_info_s channel;
  int16_t filter;
  size_t max_messages; /* 0 = use global default */
  uint64_t max_age_ms; /* 0 = use global default */
} fio_pubsub_history_channel_args_s;
```

Sets per-channel history configuration, overriding global defaults.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
/* Override limits for a specific channel */
fio_pubsub_history_channel_set(
    .channel = FIO_BUF_INFO1("important_channel"),
    .filter = 0,
    .max_messages = 10000,
    .max_age_ms = 7200000);  /* 2 hours */
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `channel` | `fio_buf_info_s` | The channel name to configure |
| `filter` | `int16_t` | The channel's numerical namespace filter |
| `max_messages` | `size_t` | Maximum messages for this channel; 0 uses global default |
| `max_age_ms` | `uint64_t` | Maximum message age for this channel; 0 uses global default |

**Returns:** 0 on success, -1 if the channel was not found.

**Note**: The channel must exist (have at least one subscriber) before configuration can be set.

#### `fio_pubsub_history_oldest`

```c
uint64_t fio_pubsub_history_oldest(fio_pubsub_history_oldest_args_s args);
/* Named arguments using macro. */
#define fio_pubsub_history_oldest(...) \
  fio_pubsub_history_oldest((fio_pubsub_history_oldest_args_s){__VA_ARGS__})

typedef struct {
  fio_buf_info_s channel;
  int16_t filter;
} fio_pubsub_history_oldest_args_s;
```

Gets the oldest available message timestamp for a channel in milliseconds since epoch.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
uint64_t oldest = fio_pubsub_history_oldest(
    .channel = FIO_BUF_INFO1("my_channel"),
    .filter = 0);

if (oldest) {
  /* Use oldest timestamp for replay_since */
  fio_subscribe(.channel = FIO_BUF_INFO1("my_channel"),
                .replay_since = oldest,
                .on_message = my_callback);
}
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `channel` | `fio_buf_info_s` | The channel name to query |
| `filter` | `int16_t` | The channel's numerical namespace filter |

**Returns:** The oldest available message timestamp in milliseconds since epoch, or 0 if:
- History is disabled
- The channel does not exist
- The channel has no messages in history

**Note**: This operation is O(1) as the oldest timestamp is cached per-channel.

### History Usage Examples

#### Basic History Usage

```c
/* Enable history at startup */
fio_pubsub_history_enable(.max_messages = 1000, .max_age_ms = 3600000);

/* Publish some messages */
for (int i = 0; i < 10; i++) {
  fio_publish(.channel = FIO_BUF_INFO1("news"),
              .message = FIO_BUF_INFO1("News update"));
}

/* Late subscriber replays all history */
fio_subscribe(.channel = FIO_BUF_INFO1("news"),
              .replay_since = 1,  /* Replay from beginning */
              .on_message = on_news);
```

#### Subscribing with Timestamp-Based Replay

```c
/* Get current time before going offline */
uint64_t disconnect_time = fio_time2milli(fio_time_real());

/* ... later, reconnect and replay missed messages */

fio_subscribe(.channel = FIO_BUF_INFO1("chat"),
              .replay_since = disconnect_time,
              .on_message = on_chat_message);
```

#### Per-Channel Configuration

```c
/* Enable history with moderate defaults */
fio_pubsub_history_enable(.max_messages = 100, .max_age_ms = 600000);

/* Create channels first by subscribing */
fio_subscribe(.channel = FIO_BUF_INFO1("critical"),
              .on_message = on_critical);
fio_subscribe(.channel = FIO_BUF_INFO1("debug"),
              .on_message = on_debug);

/* Configure critical channel to keep more history */
fio_pubsub_history_channel_set(
    .channel = FIO_BUF_INFO1("critical"),
    .max_messages = 10000,
    .max_age_ms = 86400000);  /* 24 hours */

/* Configure debug channel to keep minimal history */
fio_pubsub_history_channel_set(
    .channel = FIO_BUF_INFO1("debug"),
    .max_messages = 10,
    .max_age_ms = 60000);  /* 1 minute */
```

#### Query Oldest Available Timestamp

```c
/* Check what history is available before subscribing */
uint64_t oldest = fio_pubsub_history_oldest(
    .channel = FIO_BUF_INFO1("events"),
    .filter = 0);

if (oldest == 0) {
  /* No history available */
  fio_subscribe(.channel = FIO_BUF_INFO1("events"),
                .on_message = on_event);
} else if (oldest > my_last_seen) {
  /* Gap in history - some messages were evicted */
  FIO_LOG_WARNING("History gap detected, %llu messages may be lost",
                  (unsigned long long)(my_last_seen - oldest));
  fio_subscribe(.channel = FIO_BUF_INFO1("events"),
                .replay_since = oldest,
                .on_message = on_event);
} else {
  /* Full history available */
  fio_subscribe(.channel = FIO_BUF_INFO1("events"),
                .replay_since = my_last_seen,
                .on_message = on_event);
}
```

#### Multi-Process History Replay

```c
/* In master process: enable history and publish messages */
if (fio_io_is_master()) {
  fio_pubsub_history_enable(.max_messages = 500);
  
  fio_subscribe(.channel = FIO_BUF_INFO1("status"),
                .on_message = on_status);
  
  /* Publish periodic status updates */
  fio_run_every(.every = 1000, .fn = publish_status, .repetitions = -1);
}

/* In worker process: subscribe with replay */
fio_state_callback_add(FIO_CALL_ON_START, worker_subscribe, NULL);

void worker_subscribe(void *udata) {
  if (fio_io_is_worker()) {
    /* Worker requests history from master via IPC */
    fio_subscribe(.channel = FIO_BUF_INFO1("status"),
                  .replay_since = 1,  /* Replay all available history */
                  .on_message = on_status);
  }
}
```

### History IPC Architecture

When a worker process subscribes with `replay_since`, the following IPC exchange occurs:

1. **Worker sends history request**: Worker sends `FIO___PUBSUB_HISTORY_START` message to master with:
   - Channel name and filter
   - `replay_since` timestamp in the message ID field

2. **Master replays history**: Master iterates its channel history and sends matching messages:
   - Each message is marked with `FIO___PUBSUB_REPLAY` flag
   - Messages with `published >= replay_since` are sent

3. **Master sends completion marker**: Master sends `FIO___PUBSUB_HISTORY_END` message

4. **Worker processes replay**: Worker receives and delivers replay messages to subscriber

This design ensures workers don't duplicate history storage while still providing full replay functionality.

### Memory Management and Cleanup

#### Automatic Cleanup

- **Channel destruction**: When a channel has no subscribers and no history (or history is disabled), it is automatically destroyed
- **Fork handling**: Child processes properly free inherited memory to prevent leaks:
  - History messages are cleared via `fio___channel_on_destroy`
  - Subscriptions are explicitly freed via `fio___pubsub_free_channel_subscriptions`
  - Channel structures are destroyed after subscription cleanup

#### Reference Counting

Messages in history are reference-counted:
- Each history entry holds one reference
- Each active subscription delivery holds one reference
- Messages are freed only when all references are released

This prevents use-after-free and ensures proper memory lifecycle management.

### Pub/Sub Engines

The pub/sub system allows the delivery of messages through either internal or external services called "engines".

The default pub/sub engine can be set by setting the global `FIO_PUBSUB_DEFAULT` variable which is set to `FIO_PUBSUB_CLUSTER` by default.

External engines are funneled to the root / master process before their `publish` function is called, which means that even if `from` is specified, it will be ignored for any external engine.

#### `FIO_PUBSUB_ROOT`

```c
#define FIO_PUBSUB_ROOT ((fio_pubsub_engine_s *)FIO___PUBSUB_ROOT)
```

Used to publish the message exclusively to the root / master process.

#### `FIO_PUBSUB_PROCESS`

```c
#define FIO_PUBSUB_PROCESS ((fio_pubsub_engine_s *)FIO___PUBSUB_PROCESS)
```

Used to publish the message only within the current process.

#### `FIO_PUBSUB_SIBLINGS`

```c
#define FIO_PUBSUB_SIBLINGS ((fio_pubsub_engine_s *)FIO___PUBSUB_SIBLINGS)
```

Used to publish the message except within the current process.

#### `FIO_PUBSUB_LOCAL`

```c
#define FIO_PUBSUB_LOCAL ((fio_pubsub_engine_s *)FIO___PUBSUB_LOCAL)
```

Used to publish the message for this process, its siblings and root.

#### `FIO_PUBSUB_CLUSTER`

```c
#define FIO_PUBSUB_CLUSTER ((fio_pubsub_engine_s *)FIO___PUBSUB_CLUSTER)
```

Used to publish the message to any possible publishers, including connected cluster peers.

This is the default engine.

#### `fio_pubsub_engine_s`

```c
struct fio_pubsub_engine_s {
  /** Called after the engine was detached, may be used for cleanup. */
  void (*detached)(const fio_pubsub_engine_s *eng);
  /** Subscribes to a channel. Called ONLY in the Root (master) process. */
  void (*subscribe)(const fio_pubsub_engine_s *eng,
                    fio_buf_info_s channel,
                    int16_t filter);
  /** Subscribes to a pattern. Called ONLY in the Root (master) process. */
  void (*psubscribe)(const fio_pubsub_engine_s *eng,
                     fio_buf_info_s channel,
                     int16_t filter);
  /** Unsubscribes to a channel. Called ONLY in the Root (master) process. */
  void (*unsubscribe)(const fio_pubsub_engine_s *eng,
                      fio_buf_info_s channel,
                      int16_t filter);
  /** Unsubscribe to a pattern. Called ONLY in the Root (master) process. */
  void (*punsubscribe)(const fio_pubsub_engine_s *eng,
                       fio_buf_info_s channel,
                       int16_t filter);
  /** Publishes a message through the engine. Called by any worker / thread. */
  void (*publish)(const fio_pubsub_engine_s *eng, fio_msg_s *msg);
};
```

This is the (internal) structure of a facil.io pub/sub engine.

Only messages and unfiltered subscriptions (where filter == 0) will be forwarded to these "engines".

Engines MUST provide the listed function pointers and should be attached using the `fio_pubsub_attach` function.

Engines should disconnect / detach, before being destroyed, by using the `fio_pubsub_detach` function.

When an engine received a message to publish, it should call the `fio_publish` function with the built-in engine to which the message is forwarded.
i.e.:

```c
fio_publish(
    .engine = FIO_PUBSUB_LOCAL,
    .channel = channel_name,
    .message = msg_body );
```

Since only the master process guarantees to be subscribed to all the channels in the cluster, only the master process calls the `(un)(p)subscribe` callbacks.

**Note**: The callbacks will be called by the main IO thread, so they should never block. Long tasks should copy the data and schedule an external task (i.e., using `fio_io_defer`).

#### `fio_pubsub_attach`

```c
void fio_pubsub_attach(fio_pubsub_engine_s *engine);
```

Attaches an engine, so it's callback can be called by facil.io.

The `(p)subscribe` callback will be called for every existing channel.

This can be called multiple times resulting in re-running the `(p)subscribe` callbacks.

**Note**: engines are automatically detached from child processes but can be safely used even so - messages are always forwarded to the engine attached to the root (master) process.

**Note**: engines should publish events to `FIO_PUBSUB_LOCAL`.

#### `fio_pubsub_detach`

```c
void fio_pubsub_detach(fio_pubsub_engine_s *engine);
```

Schedules an engine for Detachment, so it could be safely destroyed.

### User Defined Pub/Sub Message Metadata

#### `fio_msg_metadata_fn`

```c
typedef void *(*fio_msg_metadata_fn)(fio_msg_s *);
```

Pub/Sub Metadata callback type.

The callback receives the message and should return a `void *` pointer to the metadata.

#### `fio_message_metadata_add`

```c
int fio_message_metadata_add(fio_msg_metadata_fn metadata_func, void (*cleanup)(void *));
```

It's possible to attach metadata to facil.io pub/sub messages before they are published.

This allows, for example, messages to be encoded as network packets for outgoing protocols (i.e., encoding for WebSocket transmissions), improving performance in large network based broadcasting.

Up to `FIO___PUBSUB_METADATA_STORE_LIMIT` metadata callbacks can be attached (default is 4).

The callback should return a `void *` pointer.

To remove a callback, call `fio_message_metadata_remove` with the returned value.

The cluster messaging system allows some messages to be flagged as JSON and this flag is available to the metadata callback.

Returns zero (0) on success or -1 on failure.

Multiple `fio_message_metadata_add` calls increase a reference count and should be matched by the same number of `fio_message_metadata_remove`.

#### `fio_message_metadata_remove`

```c
void fio_message_metadata_remove(fio_msg_metadata_fn metadata_func);
```

Removed the metadata callback.

Removal might be delayed if live metatdata exists.

#### `fio_message_metadata`

```c
void *fio_message_metadata(fio_msg_s *msg, fio_msg_metadata_fn metadata_func);
```

Finds the message's metadata, returning the data or NULL.

**Parameters:**
- `msg` - the message to retrieve metadata from
- `metadata_func` - the metadata callback function used when adding the metadata

**Note**: channels with non-zero filters don't have metadata attached.

### Pub/Sub Connectivity Helpers

#### `fio_pubsub_ipc_url_set`

```c
int fio_pubsub_ipc_url_set(char *str, size_t len);
```

Sets the current IPC socket address (can't be changed while running).

Returns -1 on error (i.e., server is already running or length is too long).

#### `fio_pubsub_ipc_url`

```c
const char *fio_pubsub_ipc_url(void);
```

Returns a pointer to the current IPC socket address.

#### `fio_pubsub_broadcast_on_port`

```c
void fio_pubsub_broadcast_on_port(int16_t port);
```

Enables auto-peer detection and pub/sub multi-machine clustering using the specified `port`.

This function sets up UDP broadcast for peer discovery and TCP connections for message exchange between cluster nodes.

**Parameters:**
- `port` - the port number to use for broadcasting and listening. If 0 or negative, defaults to 3333.

**Note**: This requires a shared secret to be set (not a random secret) for peer validation. The secret can be set using `fio_secret_set`.

**Note**: The `PUBSUB_PORT` environment variable can also be used to set the port automatically at startup.

-------------------------------------------------------------------------------
