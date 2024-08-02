## Pub/Sub 

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

* Subscriptions match delivery interests by both channel name (or pattern) and a numerical filter.

### Subscriptions - Receiving Messages

#### `fio_subscribe`

```c
void fio_subscribe(subscribe_args_s args);
#define fio_subscribe(...) fio_subscribe((subscribe_args_s){__VA_ARGS__})
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
  fio_s *io;
  /**
   * A named `channel` to which the subscriber subscribes.
   *
   * Subscriptions require a match by both channel name and filter.
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
} subscribe_args_s;
```

The `fio_msg_s` struct in the `on_message` callback contains the following information:

```c
typedef struct fio_msg_s {
  /** A connection (if any) to which the subscription belongs. */
  fio_s *io;
  /**
   * A channel name, allowing for pub/sub patterns.
   *
   * NOTE: the channel and message strings should be considered immutable.
   */
  fio_str_info_s channel;
  /**
   * The actual message.
   *
   * NOTE: the channel and message strings should be considered immutable.
   **/
  fio_str_info_s message;
  /** The `udata` argument associated with the subscription. */
  void *udata;
  /** A unique message type. Negative values are reserved, 0 == pub/sub. */
  int16_t filter;
  /** flag indicating if the message is JSON data or binary/text. */
  uint8_t is_json;
} fio_msg_s;
```

#### `fio_unsubscribe`

```c
int fio_unsubscribe(subscribe_args_s args);
#define fio_unsubscribe(...) fio_unsubscribe((subscribe_args_s){__VA_ARGS__})
```

Cancels an existing subscriptions.

Accepts the same arguments as [`fio_subscribe`](fio_subscribe), except the `udata` and callback details are ignored (no need to provide `udata` or callback details).

If a `subscription_handle_ptr` was provided it should contain the value of the subscription handle returned.

Returns -1 if the subscription could not be found. Otherwise returns 0.

The `fio_unsubscribe` macro shadows the `fio_unsubscribe` function and allows the same named arguments as the [`fio_subscribe`](fio_subscribe) function.

#### `fio_message_defer`

```c
void fio_message_defer(fio_msg_s *msg);
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

### Publishing to Subscribers

#### `fio_publish`

```c
void fio_publish(fio_publish_args_s args);
#define fio_publish(...) fio_publish((fio_publish_args_s){__VA_ARGS__})
```

Publishes a message to the relevant subscribers (if any).

By default the message is sent using the `FIO_PUBSUB_DEFAULT` engine (set by default to `FIO_PUBSUB_LOCAL` which publishes to all processes, including the calling process).

If publishing to a channel with a non-zero `filter`, the pub/sub will default to `FIO_PUBSUB_LOCAL` and external engines will be ignored.

To limit the message only to other processes (exclude the calling process), use the `FIO_PUBSUB_SIBLINGS` engine.

To limit the message only to the calling process, use the `FIO_PUBSUB_PROCESS` engine.

To limit the message only to the root process, use the `FIO_PUBSUB_ROOT` engine.

The `fio_publish` macro shadows the `fio_publish` function and allows the following named arguments to be set:

```c
typedef struct fio_publish_args_s {
  /** The pub/sub engine that should be used to forward this message. */
  fio_pubsub_engine_s const *engine;
  /** If `from` is specified, it will be skipped (won't receive message)
   *  UNLESS a non-native `engine` is specified. */
  fio_s *from;
  /** The target named channel. Only published when filter == 0. */
  fio_buf_info_s channel;
  /** The message body / content. */
  fio_buf_info_s message;
  /** A numeral / internal channel. Negative values are reserved. */
  int16_t filter;
  /** A flag indicating if the message is JSON data or not. */
  uint8_t is_json;
} fio_publish_args_s;
```

### Pub/Sub Engines

The pub/sub system allows the delivery of messages through either internal or external services called "engines".

The default pub/sub engine can be set by setting the global `FIO_PUBSUB_DEFAULT` variable which is set to `FIO_PUBSUB_LOCAL` by default.

External engines are funneled to the root / master process before their `publish` function is called, which means that even if `from` is specified, it will be ignored for any external engine.

#### `FIO_PUBSUB_ROOT`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_ROOT;
```

Used to publish the message exclusively to the root / master process.

#### `FIO_PUBSUB_PROCESS`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_PROCESS;
```

Used to publish the message only within the current process.

#### `FIO_PUBSUB_SIBLINGS`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_SIBLINGS;
```

Used to publish the message except within the current process.

#### `FIO_PUBSUB_LOCAL`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_LOCAL;
```

Used to publish the message for this process, its siblings and root.

#### `fio_pubsub_engine_s`

```c
struct fio_pubsub_engine_s {
  /** Called after the engine was detached, may be used for cleanup. */
  void (*detached)(const fio_pubsub_engine_s *eng);
  /** Subscribes to a channel. Called ONLY in the Root (master) process. */
  void (*subscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Unsubscribes to a channel. Called ONLY in the Root (master) process. */
  void (*unsubscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Subscribes to a pattern. Called ONLY in the Root (master) process. */
  void (*psubscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Unsubscribe to a pattern. Called ONLY in the Root (master) process. */
  void (*punsubscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Publishes a message through the engine. Called by any worker / thread. */
  void (*publish)(const fio_pubsub_engine_s *eng,
                  fio_str_info_s channel,
                  fio_str_info_s msg,
                  uint8_t is_json);
};
```

This is the (internal) structure of a facil.io pub/sub engine.

Only messages and unfiltered subscriptions (where filter == 0) will be forwarded to these "engines".

Engines MUST provide the listed function pointers and should be attached using the `fio_pubsub_attach` function.

Engines should disconnect / detach, before being destroyed, by using the `fio_pubsub_detach` function.

When an engine received a message to publish, it should call the `pubsub_publish` function with the built-in engine to which the message is forwarded.
i.e.:

```c
pubsub_publish(
    .engine = FIO_PUBSUB_LOCAL,
    .channel = channel_name,
    .message = msg_body );
```

Since only the master process guarantees to be subscribed to all the channels in the cluster, only the master process calls the `(un)(p)subscribe` callbacks.

**Note**: The `(un)(p)subscribe` callbacks might be called by the main (master) thread, so they should never block except by scheduling an external task using `fio_srv_defer`.

#### `fio_pubsub_attach`

```c
void fio_pubsub_attach(fio_pubsub_engine_s *engine);
```

Attaches an engine, so it's callback can be called by facil.io.

The `(p)subscribe` callback will be called for every existing channel.

NOTE: the root (master) process will call `subscribe` for any channel in any process, while all the other processes will call `subscribe` only for their own channels. This allows engines to use the root (master) process as an exclusive subscription process.


#### `fio_pubsub_detach`

```c
void fio_pubsub_detach(fio_pubsub_engine_s *engine);
```

Detaches an engine, so it could be safely destroyed.

#### `fio_pubsub_resubscribe_all`

```c
void fio_pubsub_resubscribe_all(fio_pubsub_engine_s *eng);
```

Engines can ask facil.io to call the `(p)subscribe` callbacks for all active channels.

This allows engines that lost their connection to their Pub/Sub service to resubscribe to all the currently active channels with the new connection.

**CAUTION**: This is an evented task... try not to free the engine's memory while re-subscriptions are under way.

**NOTE**: the root (master) process will call `(p)subscribe` for any channel in any process, while all the other processes will call `subscribe` only for their own channels. This allows engines to use the root (master) process as an exclusive subscription process.

#### `fio_pubsub_is_attached`

```c
int fio_pubsub_is_attached(fio_pubsub_engine_s *engine);
```

Returns true (`1`) if the engine is attached to the system.

### User Defined Pub/Sub Message Metadata

#### `fio_msg_metadata_fn`

```c
typedef void *(*fio_msg_metadata_fn)(fio_str_info_s ch,
                                     fio_str_info_s msg,
                                     uint8_t is_json);
```

Pub/Sub Metadata callback type.

#### `fio_message_metadata_add`

```c
int fio_message_metadata_add(fio_msg_metadata_fn metadata_func, void (*cleanup)(void *));
```

It's possible to attach metadata to facil.io named messages (filter == 0) before they are published.

This allows, for example, messages to be encoded as network packets for outgoing protocols (i.e., encoding for WebSocket transmissions), improving performance in large network based broadcasting.

Up to `FIO_PUBSUB_METADATA_LIMIT` metadata callbacks can be attached.

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
void *fio_message_metadata(fio_msg_metadata_fn metadata_func);
```


Finds the message's metadata, returning the data or NULL.

Note: channels with non-zero filters don't have metadata attached.

### Pub/Sub Connectivity Helpers

#### `fio_pubsub_ipc_url_set`

```c
int fio_pubsub_ipc_url_set(char *str, size_t len);
```

Returns the current IPC socket address (cannot be changed after `fio_srv_start` was called).

Returns -1 on error (i.e., server is already running or length is too long).

#### `fio_pubsub_ipc_url`

```c
const char *fio_pubsub_ipc_url(void);
```

Returns the current IPC socket address (shouldn't be changed).


#### `fio_pubsub_secret_set`

```c
void fio_pubsub_secret_set(char *secret, size_t len);
```

Sets a (possibly shared) secret for securing pub/sub communication.

If `secret` is `NULL`, the environment variable `"SECRET"` will be used or, if not set, a random secret will be generated.

-------------------------------------------------------------------------------
