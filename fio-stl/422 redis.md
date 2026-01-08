## Redis Pub/Sub Engine

```c
#define FIO_REDIS
#include FIO_INCLUDE_FILE
```

The Redis module provides a pub/sub engine that integrates with facil.io's pub/sub system, enabling distributed messaging across multiple server instances through Redis.

This module is designed for horizontal scaling scenarios where multiple application instances need to share pub/sub messages. When attached to the facil.io pub/sub system, all subscriptions and publications are automatically synchronized through Redis.

**Note**: This module requires the IO reactor (`FIO_IO`), FIOBJ types (`FIO_FIOBJ`), and RESP3 parser (`FIO_RESP3`) modules, which are automatically included.

**Note**: The engine is only active after the IO reactor starts (`fio_io_start()`). Commands sent before the reactor starts are queued and executed once the connection is established.

### Features

- **Dual Connection Model**: Maintains separate connections for publishing and subscribing to avoid protocol conflicts
- **Automatic Subscription Management**: SUBSCRIBE/PSUBSCRIBE commands are handled internally by the pub/sub engine
- **Command Queue**: Send arbitrary Redis commands with asynchronous callbacks
- **Authentication Support**: Optional AUTH command on connection
- **Automatic Reconnection**: Reconnects automatically on connection loss
- **Ping/Pong Keepalive**: Configurable keepalive interval to detect dead connections
- **Reference Counting**: Safe sharing of engine across multiple owners

### Quick Start

```c
#define FIO_LOG
#define FIO_REDIS
#include FIO_INCLUDE_FILE

/* Message handler */
void on_message(fio_pubsub_msg_s *msg) {
  FIO_LOG_INFO("Received on channel '%.*s': %.*s",
               (int)msg->channel.len, msg->channel.buf,
               (int)msg->message.len, msg->message.buf);
}

int main(void) {
  /* Create Redis engine (ref count = 1) */
  fio_pubsub_engine_s *redis = fio_redis_new(
      .url = "redis://localhost:6379"
  );
  
  /* Attach to pub/sub system (does NOT take ownership) */
  fio_pubsub_engine_attach(redis);
  
  /* Subscribe to a channel */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("my-channel"),
                .on_message = on_message);
  
  /* Start the IO reactor */
  fio_io_start(0);
  
  /* Cleanup - detach before freeing if attached */
  fio_pubsub_engine_detach(redis);
  fio_redis_free(redis);
  return 0;
}
```

------------------------------------------------------------

### Configuration Macros

#### `FIO_REDIS_READ_BUFFER`

```c
#define FIO_REDIS_READ_BUFFER 32768
```

Size of the read buffer for Redis connections in bytes. Default is 32768 (32KB).

Each Redis engine allocates two read buffers (one for each connection), so the total memory usage per engine is `FIO_REDIS_READ_BUFFER * 2` bytes plus overhead.

------------------------------------------------------------

### Types

#### `fio_redis_args_s`

```c
typedef struct {
  /**
   * Redis server URL.
   *
   * Supported formats:
   * - "redis://host:port"
   * - "redis://host" (default port 6379)
   * - "host:port" (no scheme)
   * - "host" (no scheme, default port 6379)
   * - NULL or empty → defaults to "localhost:6379"
   */
  const char *url;
  /** Redis server's password, if any (for AUTH command) */
  const char *auth;
  /** Length of auth string (0 = auto-detect with strlen) */
  size_t auth_len;
  /** Ping interval in seconds (0 = default 30 seconds) */
  uint8_t ping_interval;
} fio_redis_args_s;
```

Arguments for creating a Redis pub/sub engine.

**Members:**
- `url` - Redis server URL. Supports various formats including `redis://host:port`, `host:port`, or just `host`. Defaults to `"localhost:6379"` if NULL or empty.
- `auth` - Optional password for Redis AUTH command. Set to NULL if no authentication is required.
- `auth_len` - Length of the auth string. If 0, `strlen()` is used to determine the length.
- `ping_interval` - Keepalive ping interval in seconds. Defaults to 30 seconds if 0.

------------------------------------------------------------

### Reference Counting

The Redis engine uses reference counting for memory management:

- `fio_redis_new` creates an engine with reference count = 1
- `fio_redis_dup` increments the reference count and returns the engine
- `fio_redis_free` decrements the reference count; frees the engine when count reaches 0

**Important**: `fio_pubsub_engine_attach()` and `fio_pubsub_engine_detach()` do **NOT** affect the reference count. The caller who created the engine is responsible for calling `fio_redis_free()` when done.

```c
/* Example: sharing engine across multiple owners */
fio_pubsub_engine_s *redis = fio_redis_new(.url = "localhost");

/* Share with another component */
fio_pubsub_engine_s *shared = fio_redis_dup(redis);  /* ref = 2 */

/* Attach to pub/sub (does NOT increment ref) */
fio_pubsub_engine_attach(redis);

/* ... later, first owner is done ... */
fio_redis_free(redis);   /* ref = 1, engine still alive */

/* ... later, second owner is done - must detach before final free ... */
fio_pubsub_engine_detach(shared);
fio_redis_free(shared);  /* ref = 0, engine destroyed */
```

------------------------------------------------------------

### API Functions

#### `fio_redis_new`

```c
fio_pubsub_engine_s *fio_redis_new(fio_redis_args_s args);
/* Named arguments using macro. */
#define fio_redis_new(...) fio_redis_new((fio_redis_args_s){__VA_ARGS__})
```

Creates a Redis pub/sub engine with reference count = 1.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_pubsub_engine_s *redis = fio_redis_new(
    .url = "redis://redis.example.com:6379",
    .auth = "secret_password",
    .ping_interval = 30
);
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `url` | `const char *` | Redis server URL; defaults to `"localhost:6379"` |
| `auth` | `const char *` | Optional authentication password |
| `auth_len` | `size_t` | Length of auth string; 0 for auto-detect |
| `ping_interval` | `uint8_t` | Keepalive interval in seconds; defaults to 30 |

**Returns:** A pointer to the pub/sub engine on success, or NULL on error.

**Note**: The engine is only active after the IO reactor starts running (`fio_io_start()`). Connection attempts are deferred until the reactor is running.

**Note**: The caller owns the returned reference and must call `fio_redis_free()` when done. Attaching to pub/sub does NOT transfer ownership.

#### `fio_redis_dup`

```c
fio_pubsub_engine_s *fio_redis_dup(fio_pubsub_engine_s *engine);
```

Increments the reference count and returns the engine.

Use this when you need to share the engine across multiple owners. Each call to `fio_redis_dup()` must be balanced with a corresponding call to `fio_redis_free()`.

**Parameters:**
- `engine` - The Redis engine to duplicate

**Returns:** The same engine pointer with incremented reference count.

```c
/* Share engine with another component */
fio_pubsub_engine_s *shared = fio_redis_dup(redis);
pass_to_other_component(shared);

/* Other component must call fio_redis_free(shared) when done */
```

#### `fio_redis_free`

```c
void fio_redis_free(fio_pubsub_engine_s *engine);
```

Decrements the reference count. When count reaches 0, destroys the engine.

This function:
1. Decrements the reference count
2. If ref reaches 0:
   - Closes both Redis connections (publishing and subscription)
   - Frees any queued commands
   - Releases all allocated memory

**Important**: If the engine was attached to pub/sub via `fio_pubsub_engine_attach()`, you **MUST** call `fio_pubsub_engine_detach()` before calling `fio_redis_free()`.

**Note**: Safe to call with NULL (no-op).

**Note**: Any pending command callbacks will NOT be called after destruction.

#### `fio_redis_send`

```c
int fio_redis_send(fio_pubsub_engine_s *engine,
                   FIOBJ command,
                   void (*callback)(fio_pubsub_engine_s *e,
                                    FIOBJ reply,
                                    void *udata),
                   void *udata);
```

Sends a Redis command through the engine's publishing connection.

The command is sent asynchronously. When a reply is received from Redis, the callback is invoked with the parsed response.

**Parameters:**
- `engine` - The Redis engine returned by `fio_redis_new()`
- `command` - A FIOBJ array containing the command and its arguments
- `callback` - Optional callback function invoked when the reply is received
- `udata` - User data passed to the callback

**Returns:** 0 on success, -1 on error (invalid engine or command).

**Callback Signature:**
```c
void callback(fio_pubsub_engine_s *e, FIOBJ reply, void *udata);
```

The `reply` parameter is a FIOBJ object representing the Redis response:
- Simple strings and bulk strings become `FIOBJ_T_STRING`
- Integers become `FIOBJ_T_NUMBER`
- Arrays become `FIOBJ_T_ARRAY`
- Maps (RESP3) become `FIOBJ_T_HASH`
- Null becomes `fiobj_null()`
- Booleans become `fiobj_true()` or `fiobj_false()`
- Errors become `FIOBJ_T_STRING` (check logs for error messages)

**Warning**: NEVER use `fio_redis_send` for subscription commands (`SUBSCRIBE`, `PSUBSCRIBE`, `UNSUBSCRIBE`, `PUNSUBSCRIBE`). These commands are handled internally by the pub/sub engine through the subscription connection. Using them with `fio_redis_send` will violate the Redis protocol and cause connection errors.

------------------------------------------------------------

### Usage Examples

#### Basic Redis Commands

```c
#define FIO_LOG
#define FIO_REDIS
#include FIO_INCLUDE_FILE

/* Callback for GET command */
void on_get_reply(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  fio_str_info_s value = fiobj2cstr(reply);
  FIO_LOG_INFO("GET result: %.*s", (int)value.len, value.buf);
  (void)e; (void)udata;
}

/* Callback for SET command */
void on_set_reply(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  fio_str_info_s status = fiobj2cstr(reply);
  FIO_LOG_INFO("SET result: %.*s", (int)status.len, status.buf);
  (void)e; (void)udata;
}

/* Callback for INCR command */
void on_incr_reply(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  intptr_t new_value = fiobj2i(reply);
  FIO_LOG_INFO("INCR result: %ld", (long)new_value);
  (void)e; (void)udata;
}

void send_redis_commands(fio_pubsub_engine_s *redis) {
  FIOBJ cmd;
  
  /* SET mykey "Hello, Redis!" */
  cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("SET", 3));
  fiobj_array_push(cmd, fiobj_str_new_cstr("mykey", 5));
  fiobj_array_push(cmd, fiobj_str_new_cstr("Hello, Redis!", 13));
  fio_redis_send(redis, cmd, on_set_reply, NULL);
  fiobj_free(cmd);
  
  /* GET mykey */
  cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("GET", 3));
  fiobj_array_push(cmd, fiobj_str_new_cstr("mykey", 5));
  fio_redis_send(redis, cmd, on_get_reply, NULL);
  fiobj_free(cmd);
  
  /* INCR counter */
  cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("INCR", 4));
  fiobj_array_push(cmd, fiobj_str_new_cstr("counter", 7));
  fio_redis_send(redis, cmd, on_incr_reply, NULL);
  fiobj_free(cmd);
}
```

#### Pub/Sub Integration with facil.io

```c
#define FIO_LOG
#define FIO_REDIS
#include FIO_INCLUDE_FILE

static fio_pubsub_engine_s *redis_engine = NULL;

/* Handle incoming pub/sub messages */
void on_pubsub_message(fio_pubsub_msg_s *msg) {
  FIO_LOG_INFO("Channel: %.*s | Message: %.*s",
               (int)msg->channel.len, msg->channel.buf,
               (int)msg->message.len, msg->message.buf);
}

/* Called when server starts */
void on_start(void *udata) {
  (void)udata;
  
  /* Subscribe to channels - Redis engine handles SUBSCRIBE automatically */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("notifications"),
                .on_message = on_pubsub_message);
  
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("events:*"),
                .on_message = on_pubsub_message,
                .is_pattern = 1);  /* Pattern subscription uses PSUBSCRIBE */
  
  FIO_LOG_INFO("Subscribed to channels via Redis");
}

/* Publish a message (from any worker process) */
void broadcast_message(const char *channel, const char *message) {
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(channel),
              .message = FIO_BUF_INFO1(message),
              .engine = FIO_PUBSUB_CLUSTER);  /* Uses Redis if attached */
}

int main(void) {
  /* Create Redis engine (ref count = 1) */
  redis_engine = fio_redis_new(.url = "redis://localhost:6379");
  if (!redis_engine) {
    FIO_LOG_FATAL("Failed to create Redis engine");
    return 1;
  }
  
  /* Attach to pub/sub (does NOT take ownership) */
  fio_pubsub_engine_attach(redis_engine);
  
  /* Register startup callback */
  fio_state_callback_add(FIO_CALL_ON_START, on_start, NULL);
  
  /* Start the server */
  fio_io_start(0);
  
  /* Cleanup - detach before freeing if attached */
  fio_pubsub_engine_detach(redis_engine);
  fio_redis_free(redis_engine);
  return 0;
}
```

#### Authentication

```c
#define FIO_LOG
#define FIO_REDIS
#include FIO_INCLUDE_FILE

int main(void) {
  /* Connect with password authentication */
  fio_pubsub_engine_s *redis = fio_redis_new(
      .url = "redis://redis.example.com:6379",
      .auth = "my_redis_password"
  );
  
  if (!redis) {
    FIO_LOG_ERROR("Failed to create Redis engine");
    return 1;
  }
  
  /* For Redis 6+ ACL authentication (username:password), 
   * you may need to send AUTH command manually after connection */
  
  fio_pubsub_engine_attach(redis);
  fio_io_start(0);
  fio_pubsub_engine_detach(redis);
  fio_redis_free(redis);
  return 0;
}
```

#### Handling Complex Replies

```c
#define FIO_LOG
#define FIO_REDIS
#include FIO_INCLUDE_FILE

/* Handle HGETALL reply (returns array of field-value pairs) */
void on_hgetall_reply(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  if (FIOBJ_TYPE(reply) != FIOBJ_T_ARRAY) {
    FIO_LOG_WARNING("Unexpected reply type");
    return;
  }
  
  size_t count = fiobj_array_count(reply);
  FIO_LOG_INFO("Hash has %zu fields:", count / 2);
  
  for (size_t i = 0; i + 1 < count; i += 2) {
    fio_str_info_s field = fiobj2cstr(fiobj_array_get(reply, (int32_t)i));
    fio_str_info_s value = fiobj2cstr(fiobj_array_get(reply, (int32_t)(i + 1)));
    FIO_LOG_INFO("  %.*s = %.*s",
                 (int)field.len, field.buf,
                 (int)value.len, value.buf);
  }
  (void)e; (void)udata;
}

/* Handle MGET reply (returns array of values) */
void on_mget_reply(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  if (FIOBJ_TYPE(reply) != FIOBJ_T_ARRAY) {
    FIO_LOG_WARNING("Unexpected reply type");
    return;
  }
  
  size_t count = fiobj_array_count(reply);
  for (size_t i = 0; i < count; ++i) {
    FIOBJ item = fiobj_array_get(reply, (int32_t)i);
    if (FIOBJ_TYPE(item) == FIOBJ_T_NULL) {
      FIO_LOG_INFO("Key %zu: (nil)", i);
    } else {
      fio_str_info_s value = fiobj2cstr(item);
      FIO_LOG_INFO("Key %zu: %.*s", i, (int)value.len, value.buf);
    }
  }
  (void)e; (void)udata;
}
```

------------------------------------------------------------

### Architecture Notes

#### Dual Connection Model

The Redis engine maintains two separate TCP connections to the Redis server:

1. **Publishing Connection**: Used for sending commands (`fio_redis_send`) and PUBLISH operations. This connection operates in request-response mode.

2. **Subscription Connection**: Used exclusively for SUBSCRIBE/PSUBSCRIBE operations. Once a connection enters subscription mode, it can only receive pub/sub messages and cannot execute regular commands.

This separation is required by the Redis protocol - a connection in subscription mode cannot execute regular commands, and mixing the two would cause protocol errors.

```
┌─────────────────────────────────────────────────────────────┐
│                    facil.io Application                      │
├─────────────────────────────────────────────────────────────┤
│                    Redis Pub/Sub Engine                      │
│  ┌─────────────────────┐    ┌─────────────────────────────┐ │
│  │ Publishing Connection│    │ Subscription Connection     │ │
│  │ - fio_redis_send()  │    │ - SUBSCRIBE/PSUBSCRIBE      │ │
│  │ - PUBLISH commands  │    │ - Receives pub/sub messages │ │
│  │ - Regular commands  │    │ - Pattern matching          │ │
│  └──────────┬──────────┘    └──────────────┬──────────────┘ │
└─────────────┼───────────────────────────────┼───────────────┘
              │                               │
              └───────────────┬───────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │   Redis Server  │
                    └─────────────────┘
```

#### Ownership and Attach/Detach

The `fio_pubsub_engine_attach()` and `fio_pubsub_engine_detach()` functions do **NOT** transfer ownership of the engine. They simply register or unregister the engine with the pub/sub system.

The caller who created the engine with `fio_redis_new()` is responsible for calling `fio_redis_free()` when the engine is no longer needed. **Important**: If the engine was attached via `fio_pubsub_engine_attach()`, you **MUST** call `fio_pubsub_engine_detach()` before calling `fio_redis_free()`.

#### Command Queue

Commands sent via `fio_redis_send()` are queued internally and sent one at a time, waiting for each reply before sending the next command. This ensures proper correlation between commands and their responses.

The queue is processed in FIFO order. If the connection is lost, queued commands remain in the queue and are sent after reconnection.

------------------------------------------------------------

### Error Handling

#### Connection Loss

When a connection is lost:

1. The engine logs a warning message
2. Automatic reconnection is attempted after a brief delay
3. On the subscription connection, all active subscriptions are re-established via `fio_pubsub_engine_attach()`
4. Queued commands on the publishing connection are sent after reconnection

#### Authentication Failures

If Redis authentication fails, the connection will be closed by Redis. The engine will attempt to reconnect, but authentication will fail again. Check your Redis logs and ensure the correct password is configured.

#### Protocol Errors

If a RESP protocol error is detected (malformed data from Redis), the connection is closed immediately and reconnection is attempted. This typically indicates a network issue or a bug.

#### Logging

The Redis module uses facil.io's logging system:

- `FIO_LOG_DEBUG` - Connection establishment, command sending
- `FIO_LOG_WARNING` - Connection loss, unexpected responses, errors from Redis
- `FIO_LOG_ERROR` - Protocol errors, allocation failures

------------------------------------------------------------

### Thread Safety

The Redis engine is thread-safe. All internal state modifications are delegated to the IO queue using `fio_io_defer()`, ensuring single-threaded execution of state changes. This prevents race conditions without requiring locks.

**Public API thread safety:**
- `fio_redis_new()` - Thread-safe (defers connection to IO thread)
- `fio_redis_dup()` - Thread-safe (uses atomic reference counting)
- `fio_redis_free()` - Thread-safe (defers cleanup to IO thread)
- `fio_redis_send()` - Thread-safe (defers command queuing to IO thread)

**Internal operations that run on the IO thread:**
- Command queue management (add, remove, send)
- Connection state changes (connect, disconnect, reconnect)
- Protocol callbacks (on_attach, on_data, on_close, on_timeout)
- Pub/sub engine callbacks (subscribe, publish, etc.)

The FIOBJ objects passed to callbacks are **not** thread-safe. If you need to share reply data across threads, make a copy of the data within the callback.

**Note**: The pub/sub callbacks (`on_message`) are called from the IO reactor thread. Long-running operations should be deferred to avoid blocking the reactor.

------------------------------------------------------------

### Integration with Pub/Sub Engines

The Redis engine implements the `fio_pubsub_engine_s` interface:

```c
struct fio_pubsub_engine_s {
  void (*detached)(const fio_pubsub_engine_s *eng);
  void (*subscribe)(const fio_pubsub_engine_s *eng, fio_buf_info_s channel, int16_t filter);
  void (*psubscribe)(const fio_pubsub_engine_s *eng, fio_buf_info_s channel, int16_t filter);
  void (*unsubscribe)(const fio_pubsub_engine_s *eng, fio_buf_info_s channel, int16_t filter);
  void (*punsubscribe)(const fio_pubsub_engine_s *eng, fio_buf_info_s channel, int16_t filter);
  void (*publish)(const fio_pubsub_engine_s *eng, fio_pubsub_msg_s *msg);
};
```

When attached via `fio_pubsub_engine_attach()`:

- `subscribe` sends Redis `SUBSCRIBE` command
- `psubscribe` sends Redis `PSUBSCRIBE` command  
- `unsubscribe` sends Redis `UNSUBSCRIBE` command
- `punsubscribe` sends Redis `PUNSUBSCRIBE` command
- `publish` sends Redis `PUBLISH` command via the command queue

Messages received from Redis subscriptions are forwarded to local subscribers via `fio_pubsub_publish()` with `fio_pubsub_engine_ipc()` engine.

**Note**: The `filter` parameter is ignored by the Redis engine. Redis does not support facil.io's numeric filter namespaces.

------------------------------------------------------------

### Limitations

- **Filter Namespaces**: Redis does not support facil.io's numeric filter feature. All Redis pub/sub operates with filter = 0.

- **Message Size**: Limited by Redis's maximum bulk string size and `FIO_REDIS_READ_BUFFER`. Very large messages may require increasing the buffer size.

- **Binary Data**: Channel names and messages can contain binary data, but some Redis tools may not display them correctly.

- **Cluster Mode**: This module connects to a single Redis instance. For Redis Cluster, you would need to connect to the appropriate node or use a Redis proxy.

------------------------------------------------------------
