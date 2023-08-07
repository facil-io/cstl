/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_PUBSUB             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                Pub/Sub Services for IPC / Server applications




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_PUBSUB) && !defined(H___FIO_PUBSUB___H) &&                     \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_PUBSUB___H

/* *****************************************************************************
Pub/Sub - message format
***************************************************************************** */

/** Message structure, as received by the `on_message` subscription callback. */
struct fio_msg_s {
  /** A connection (if any) to which the subscription belongs. */
  fio_s *io;
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
};

/* *****************************************************************************
Pub/Sub - Subscribe / Unsubscribe
***************************************************************************** */

/** Possible arguments for the fio_subscribe method. */
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

/**
 * Subscribes to a channel / filter pair.
 *
 * The on_unsubscribe callback will be called on failure.
 */
SFUNC void fio_subscribe(fio_subscribe_args_s args);

/**
 * Subscribes to a channel / filter pair.
 *
 * See `fio_subscribe_args_s` for details.
 */
#define fio_subscribe(...) fio_subscribe((fio_subscribe_args_s){__VA_ARGS__})

/**
 * Cancels an existing subscriptions.
 *
 * Accepts the same arguments as `fio_subscribe`, except the `udata` and
 * callback details are ignored (no need to provide `udata` or callback
 * details).
 *
 * If a `subscription_handle_ptr` was provided it should contain the value of
 * the subscription handle returned.
 *
 * Returns -1 if the subscription could not be found. Otherwise returns 0.
 */
SFUNC int fio_unsubscribe(fio_subscribe_args_s args);

/**
 * Cancels an existing subscriptions.
 *
 * Accepts the same arguments as `fio_subscribe`, except the `udata` and
 * callback details are ignored (no need to provide `udata` or callback
 * details).
 *
 * Returns -1 if the subscription could not be found. Otherwise returns 0.
 */
#define fio_unsubscribe(...)                                                   \
  fio_unsubscribe((fio_subscribe_args_s){__VA_ARGS__})

/* A callback for IO subscriptions - sends raw message data. */
FIO_SFUNC void FIO_ON_MESSAGE_SEND_MESSAGE(fio_msg_s *msg);

/* *****************************************************************************
Pub/Sub - Publish
***************************************************************************** */

/** A pub/sub engine data structure. See details later on. */
typedef struct fio_pubsub_engine_s fio_pubsub_engine_s;

/** Publishing and on_message callback arguments. */
typedef struct fio_publish_args_s {
  /** The pub/sub engine that should be used to forward this message. */
  fio_pubsub_engine_s const *engine;
  /** If `from` is specified, it will be skipped (won't receive message). */
  fio_s *from;
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

/**
 * Publishes a message to the relevant subscribers (if any).
 *
 * By default the message is sent using the `FIO_PUBSUB_DEFAULT` engine (set by
 * default to `FIO_PUBSUB_LOCAL` which publishes to all processes, including the
 * calling process).
 *
 * To limit the message only to other processes (exclude the calling process),
 * use the `FIO_PUBSUB_SIBLINGS` engine.
 *
 * To limit the message only to the calling process, use the
 * `FIO_PUBSUB_PROCESS` engine.
 *
 * To limit the message only to the root process, use the `FIO_PUBSUB_ROOT`
 * engine.
 */
SFUNC void fio_publish(fio_publish_args_s args);
/**
 * Publishes a message to the relevant subscribers (if any).
 *
 * By default the message is sent using the `FIO_PUBSUB_DEFAULT` engine (set by
 * default to `FIO_PUBSUB_LOCAL` which publishes to all processes, including the
 * calling process).
 *
 * To limit the message only to other processes (exclude the calling process),
 * use the `FIO_PUBSUB_SIBLINGS` engine.
 *
 * To limit the message only to the calling process, use the
 * `FIO_PUBSUB_PROCESS` engine.
 *
 * To limit the message only to the root process, use the `FIO_PUBSUB_ROOT`
 * engine.
 */
#define fio_publish(...) fio_publish((fio_publish_args_s){__VA_ARGS__})

/**
 * Defers the current callback, so it will be called again for the message.
 *
 * After calling this function, the `msg` object must NOT be accessed again.
 */
SFUNC void fio_pubsub_message_defer(fio_msg_s *msg);

/* *****************************************************************************
Pub/Sub - History and Event Replay - TODO!!!
***************************************************************************** */

/** Sets the maximum number of messages to be stored in the history store. */
// SFUNC void fio_pubsub_store_limit(size_t messages);

/* *****************************************************************************
Pub/Sub - defaults and builtin pub/sub engines
***************************************************************************** */

/** Flag bits for internal usage (message exchange network format). */
typedef enum {
  /* pub/sub messages */
  FIO___PUBSUB_JSON = 1,
  FIO___PUBSUB_PROCESS = 2,
  FIO___PUBSUB_ROOT = 4,
  FIO___PUBSUB_SIBLINGS = 8,
  FIO___PUBSUB_WORKERS = (8 | 2),
  FIO___PUBSUB_LOCAL = (8 | 4 | 2),
  FIO___PUBSUB_REMOTE = 16,
  FIO___PUBSUB_CLUSTER = (16 | 8 | 4 | 2),
  FIO___PUBSUB_REPLAY = 32, /* history replay message */

  /* internal subscribe / unsubscribe messages */
  FIO___PUBSUB_INTERNAL_MESSAGE = 128,
  FIO___PUBSUB_SUB = (128 | 1),
  FIO___PUBSUB_UNSUB = (128 | 2),
  FIO___PUBSUB_IDENTIFY = (128 | 4),
  FIO___PUBSUB_FORWARDER = (128 | 8),
  FIO___PUBSUB_PING = (128 | 16),

  FIO___PUBSUB_HISTORY_START = (128 | 16),
  FIO___PUBSUB_HISTORY_END = (128 | 32),
} fio___pubsub_msg_flags_e;

/** Used to publish the message exclusively to the root / master process. */
#define FIO_PUBSUB_ROOT ((fio_pubsub_engine_s *)FIO___PUBSUB_ROOT)
/** Used to publish the message only within the current process. */
#define FIO_PUBSUB_PROCESS ((fio_pubsub_engine_s *)FIO___PUBSUB_PROCESS)
/** Used to publish the message except within the current process. */
#define FIO_PUBSUB_SIBLINGS ((fio_pubsub_engine_s *)FIO___PUBSUB_SIBLINGS)
/** Used to publish the message for this process, its siblings and root. */
#define FIO_PUBSUB_LOCAL ((fio_pubsub_engine_s *)FIO___PUBSUB_LOCAL)
/** Used to publish the message to any possible publishers. */
#define FIO_PUBSUB_CLUSTER ((fio_pubsub_engine_s *)FIO___PUBSUB_CLUSTER)

#if defined(FIO_EXTERN) /* static definitions can't be easily repeated. */
/** The default engine (settable). Initial default is FIO_PUBSUB_CLUSTER. */
SFUNC const fio_pubsub_engine_s *FIO_PUBSUB_DEFAULT;

/**
 * The pattern matching callback used for pattern matching.
 *
 * Returns 1 on a match or 0 if the string does not match the pattern.
 *
 * By default, the value is set to `fio_glob_match` (see facil.io's C STL).
 */
SFUNC uint8_t (*FIO_PUBSUB_PATTERN_MATCH)(fio_str_info_s pattern,
                                          fio_str_info_s channel);
#endif

/* *****************************************************************************
Message metadata (advance usage API)
***************************************************************************** */

/**
 * The number of different metadata callbacks that can be attached.
 *
 * Effects performance.
 *
 * The default value should be enough for the following metadata objects:
 * - WebSocket server headers.
 * - WebSocket client (header + masked message copy).
 * - EventSource (SSE) encoded named channel and message.
 */
#ifndef FIO___PUBSUB_METADATA_STORE_LIMIT
#define FIO___PUBSUB_METADATA_STORE_LIMIT 4
#endif

/** Pub/Sub Metadata callback type. */
typedef void *(*fio_msg_metadata_fn)(fio_msg_s *);

/**
 * It's possible to attach metadata to facil.io pub/sub messages before they are
 * published.
 *
 * This allows, for example, messages to be encoded as network packets for
 * outgoing protocols (i.e., encoding for WebSocket transmissions), improving
 * performance in large network based broadcasting.
 *
 * Up to `FIO___PUBSUB_METADATA_STORE_LIMIT` metadata callbacks can be attached.
 *
 * The callback should return a `void *` pointer.
 *
 * To remove a callback, call `fio_message_metadata_remove` with the returned
 * value.
 *
 * The cluster messaging system allows some messages to be flagged as JSON and
 * this flag is available to the metadata callback.
 *
 * Returns zero (0) on success or -1 on failure.
 *
 * Multiple `fio_message_metadata_add` calls increase a reference count and
 * should be matched by the same number of `fio_message_metadata_remove`.
 */
SFUNC int fio_message_metadata_add(fio_msg_metadata_fn metadata_func,
                                   void (*cleanup)(void *));

/**
 * Removed the metadata callback.
 *
 * Removal might be delayed if live metatdata exists.
 */
SFUNC void fio_message_metadata_remove(fio_msg_metadata_fn metadata_func);

/** Finds the message's metadata, returning the data or NULL. */
SFUNC void *fio_message_metadata(fio_msg_s *msg,
                                 fio_msg_metadata_fn metadata_func);

/* *****************************************************************************
Pub/Sub Middleware and Extensions ("Engines")
***************************************************************************** */

/**
 * facil.io can be linked with external Pub/Sub services using "engines".
 *
 * Engines MUST provide the listed function pointers and should be attached
 * using the `fio_pubsub_attach` function.
 *
 * Engines that were connected / attached using `fio_pubsub_attach` MUST
 * disconnect / detach, before being destroyed, by using the `fio_pubsub_detach`
 * function.
 *
 * When an engine received a message to publish, it should call the
 * `fio_publish` function with the engine to which the message is forwarded.
 * i.e.:
 *
 *       fio_publish(
 *           .engine = FIO_PUBSUB_LOCAL,
 *           .channel = channel_name,
 *           .message = msg_body);
 *
 * IMPORTANT: The callbacks will be called by the main IO thread, so they should
 * never block. Long tasks should copy the data and scheduling an external task
 * (i.e., using `fio_srv_defer`).
 */
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

/**
 * Attaches an engine, so it's callback can be called by facil.io.
 *
 * The `(p)subscribe` callback will be called for every existing channel.
 *
 * This can be called multiple times resulting in re-running the `(p)subscribe`
 * callbacks.
 *
 * NOTE: engines are automatically detached from child processes but can be
 * safely used even so - messages are always forwarded to the engine attached to
 * the root (master) process.
 *
 * NOTE: engines should publish events to `FIO_PUBSUB_LOCAL`.
 */
SFUNC void fio_pubsub_attach(fio_pubsub_engine_s *engine);

/** Schedules an engine for Detachment, so it could be safely destroyed. */
SFUNC void fio_pubsub_detach(fio_pubsub_engine_s *engine);

/* *****************************************************************************
Pub/Sub Clustering and Security
***************************************************************************** */

/** Sets the current IPC socket address (can't be changed while running). */
SFUNC int fio_pubsub_ipc_url_set(char *str, size_t len);

/** Returns a pointer to the current IPC socket address. */
SFUNC const char *fio_pubsub_ipc_url(void);

/**
 * Sets a (possibly shared) secret for securing pub/sub communication.
 *
 * If `secret` is `NULL`, the environment variable `"SECRET"` will be used or.
 *
 * If secret is never set, a random secret will be generated.
 *
 * NOTE: secrets produce a SHA-512 Hash that is used to produce 256 bit keys.
 */
SFUNC void fio_pubsub_secret_set(char *secret, size_t len);

/** Auto-peer detection and pub/sub multi-machine clustering using `port`. */
SFUNC void fio_pubsub_broadcast_on_port(int16_t port);

/* *****************************************************************************



Pub/Sub Implementation



The implementation has a big number of interconnected modules:
- Distribution Channels (`fio_channel_s` and `FIO___PUBSUB_POSTOFFICE`)
- Subscriptions (`fio_subscription_s`)
- Metadata Management.
- External Distribution Engines (`fio_pubsub_engine_s`)
- Message format and their network exchange protocols (`fio___pubsub_message_s`)

Message wire format (as 64 bit numerals in little endien encoding):
[0] Message ID
[1] Publication time (milliseconds since epoch)
[2] 16 bit filter | 16 bit channel len | 24 bit message len | 8 bit flags
| --- encryption starts --- |
| X bytes - (channel name, + 1 NUL terminator) |
| Y bytes - (message data, + 1 NUL terminator) |
| --- encryption ends --- |
| 16 bytes - (optional) message MAC |
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

#undef FIO___PUBSUB_MESSAGE_HEADER
#define FIO___PUBSUB_MESSAGE_HEADER 24
/* header + 2 NUL bytes (message + channel) + 16 byte MAC */
#undef FIO___PUBSUB_MESSAGE_OVERHEAD_NET
#define FIO___PUBSUB_MESSAGE_OVERHEAD_NET (FIO___PUBSUB_MESSAGE_HEADER + 18)
/* extra 2 NUL bytes (after message & channel name) */
#undef FIO___PUBSUB_MESSAGE_OVERHEAD
#define FIO___PUBSUB_MESSAGE_OVERHEAD (FIO___PUBSUB_MESSAGE_OVERHEAD_NET + 2)

/* *****************************************************************************
Pub/Sub - defaults and builtin pub/sub engines
***************************************************************************** */

/** The default engine (settable). Initial default is FIO_PUBSUB_CLUSTER. */
SFUNC const fio_pubsub_engine_s *FIO_PUBSUB_DEFAULT = FIO_PUBSUB_CLUSTER;

/**
 * The pattern matching callback used for pattern matching.
 *
 * Returns 1 on a match or 0 if the string does not match the pattern.
 *
 * By default, the value is set to `fio_glob_match` (see facil.io's C STL).
 */
SFUNC uint8_t (*FIO_PUBSUB_PATTERN_MATCH)(fio_str_info_s,
                                          fio_str_info_s) = fio_glob_match;

/* a mock callback for subscriptions */
FIO_SFUNC void fio___subscription_mock_cb(fio_msg_s *msg) { (void)msg; }

/* A callback for IO subscriptions. */
FIO_SFUNC void fio___subscription_call_protocol(fio_msg_s *msg) {
  if (!msg->io)
    return;
  fio_protocol_s *p = fio_protocol_get(msg->io);
  FIO_ASSERT_DEBUG(p, "every IO object should have a protocol, always");
  p->on_pubsub(msg);
}

#ifndef FIO___PUBSUB_CLUSTER_BACKLOG
#define FIO___PUBSUB_CLUSTER_BACKLOG (1UL << 12)
#endif

/* *****************************************************************************
PostOffice Distribution types - Channel and Subscription Core Types
***************************************************************************** */

/** The Distribution Channel: manages subscriptions to named channels. */
typedef struct fio_channel_s {
  FIO_LIST_HEAD subscriptions;
  FIO_LIST_HEAD history;
  uint32_t name_len;
  int16_t filter;
  uint8_t is_pattern;
  char name[];
} fio_channel_s;

/** The Channel Map: maps named channels. */
FIO_SFUNC void fio___channel_on_create(fio_channel_s *ch);
FIO_SFUNC void fio___channel_on_destroy(fio_channel_s *ch);

/**
 * Reference counting: `fio_channel_dup(ch)` / `fio_channel_free(ch)`
 */
#define FIO_REF_NAME             fio_channel
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_DESTROY(ch)      fio___channel_on_destroy(&(ch))
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** The Subscription: contains subscriber data. */
typedef struct fio_subscription_s {
  FIO_LIST_NODE node;
  FIO_LIST_NODE history;
  FIO_LIST_NODE history_active;
  uint64_t replay_since;
  fio_s *io;
  fio_channel_s *channel;
  void (*on_message)(fio_msg_s *msg);
  void (*on_unsubscribe)(void *udata);
  void *udata;
} fio_subscription_s;

/**
 * Reference counting: `fio_subscription_dup(sb)` / `fio_subscription_free(sb)`
 */
FIO_SFUNC void fio___pubsub_subscription_on_destroy(fio_subscription_s *sub);
#define FIO_REF_NAME             fio_subscription
#define FIO_REF_DESTROY(obj)     fio___pubsub_subscription_on_destroy(&(obj))
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** The Message Container */
typedef struct {
  fio_msg_s data;
  void *metadata[FIO___PUBSUB_METADATA_STORE_LIMIT];
  uint8_t metadata_is_initialized; /* to compact this we need to change all? */
  char buf[];
} fio___pubsub_message_s;

/* returns the internal message object. */
FIO_IFUNC fio___pubsub_message_s *fio___pubsub_msg2internal(fio_msg_s *msg);

/** Callback called when a message is destroyed (reference counting). */
FIO_SFUNC void fio___pubsub_message_on_destroy(fio___pubsub_message_s *m);

/* Message reference counting */
#define FIO_REF_NAME             fio___pubsub_message
#define FIO_REF_DESTROY(obj)     fio___pubsub_message_on_destroy(&(obj))
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

typedef struct {
  size_t len;
  uint64_t uuid[2];
  fio___pubsub_message_s *msg;
  char buf[FIO___PUBSUB_MESSAGE_OVERHEAD_NET];
} fio___pubsub_message_parser_s;

FIO___LEAK_COUNTER_DEF(fio___pubsub_message_parser_s)

FIO_SFUNC fio___pubsub_message_parser_s *fio___pubsub_message_parser_new(void) {
  fio___pubsub_message_parser_s *p =
      (fio___pubsub_message_parser_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*p), 0);
  FIO_ASSERT_ALLOC(p);
  FIO___LEAK_COUNTER_ON_ALLOC(fio___pubsub_message_parser_s);
  *p = (fio___pubsub_message_parser_s){0};
  return p;
}

FIO_SFUNC void fio___pubsub_message_parser_free(
    fio___pubsub_message_parser_s *p) {
  if (!p)
    return;
  fio___pubsub_message_free(p->msg);
  FIO_MEM_FREE_(p, sizeof(*p));
  FIO___LEAK_COUNTER_ON_FREE(fio___pubsub_message_parser_s);
}

/* *****************************************************************************
PostOffice Distribution types - The Distribution Channel Map
***************************************************************************** */

#define FIO___PUBSUB_CHANNEL_ENCODE_CAPA(filter_, is_pattern_)                 \
  (((size_t)(is_pattern_) << 16) | (size_t)(uint16_t)(filter_))

#define FIO___PUBSUB_CHANNEL2STR(ch)                                           \
  FIO_STR_INFO3(ch->name,                                                      \
                ch->name_len,                                                  \
                FIO___PUBSUB_CHANNEL_ENCODE_CAPA(ch->filter, ch->is_pattern))

FIO_IFUNC int fio___channel_cmp(fio_channel_s *ch, fio_str_info_s *s) {
  fio_str_info_s c = FIO___PUBSUB_CHANNEL2STR(ch);
  return FIO_STR_INFO_IS_EQ(c, s[0]);
}

FIO_SFUNC fio_channel_s *fio___channel_new_for_map(fio_str_info_s *s) {
  fio_channel_s *ch = fio_channel_new(s->len + 1);
  FIO_ASSERT_ALLOC(ch);
  *ch = (fio_channel_s){
      .subscriptions = FIO_LIST_INIT(ch->subscriptions),
      .history = FIO_LIST_INIT(ch->history),
      .name_len = (uint32_t)s->len,
      .filter = (int16_t)(s->capa & 0xFFFFUL),
      .is_pattern = (uint8_t)(s->capa >> 16),
  };
  FIO_MEMCPY(ch->name, s->buf, s->len);
  ch->name[s->len] = 0;
  fio___channel_on_create(ch);
  return ch;
}

#define FIO_MAP_NAME                  fio___channel_map
#define FIO_MAP_KEY                   fio_str_info_s
#define FIO_MAP_KEY_INTERNAL          fio_channel_s *
#define FIO_MAP_KEY_FROM_INTERNAL(k_) FIO___PUBSUB_CHANNEL2STR(k_)
#define FIO_MAP_KEY_COPY(dest, src)   ((dest) = fio___channel_new_for_map(&(src)))
#define FIO_MAP_KEY_CMP(a, b)         fio___channel_cmp((a), &(b))
#define FIO_MAP_HASH_FN(str)          fio_risky_hash(str.buf, str.len, str.capa)
#define FIO_MAP_KEY_DESTROY(key)      fio_channel_free((key))
#define FIO_MAP_KEY_DISCARD(key)
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
Pub/Sub Subscription destruction
***************************************************************************** */

/* calls the on_unsubscribe callback. */
FIO_SFUNC void fio___pubsub_subscription_on_destroy__task(void *fnp,
                                                          void *udata) {
  union {
    void *p;
    void (*fn)(void *udata);
  } u = {.p = fnp};
  u.fn(udata);
}

FIO_SFUNC void fio___pubsub_subscription_on_destroy(fio_subscription_s *s) {
  if (s->on_unsubscribe) {
    union {
      void *p;
      void (*fn)(void *udata);
    } u = {.fn = s->on_unsubscribe};
    fio_queue_push(fio_srv_queue(),
                   fio___pubsub_subscription_on_destroy__task,
                   u.p,
                   s->udata);
  }
}
/* *****************************************************************************
Pub/Sub Subscription map (for mapping Master only subscriptions)
***************************************************************************** */

/** Performs Housekeeping and defers the on_unsubscribe callback. */
FIO_IFUNC void fio___pubsub_subscription_unsubscribe(fio_subscription_s *s);

/* define a helper map to manage master only subscription. */
#define FIO_MAP_KEY_KSTR
#define FIO_MAP_NAME             fio___postoffice_msmap
#define FIO_MAP_VALUE            fio_subscription_s *
#define FIO_MAP_VALUE_DESTROY(s) fio___pubsub_subscription_unsubscribe(s)
#define FIO___RECURSIVE_INCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
Pub/Sub Remote Connection Uniqueness
***************************************************************************** */

/* Managing Remote Connection Uniqueness */
#define FIO_MAP_NAME fio___pubsub_broadcast_connected
#define FIO_MAP_KEY  uint64_t
#define FIO___RECURSIVE_INCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
Pub/Sub Engine Map
***************************************************************************** */

/* Managing Remote Connection Uniqueness */
#define FIO_MAP_NAME        fio___pubsub_engines
#define FIO_MAP_KEY         fio_pubsub_engine_s *
#define FIO_MAP_HASH_FN(e)  fio_risky_ptr(e)
#define FIO_MAP_RECALC_HASH 1
#define FIO_MAP_KEY_DESTROY(e)                                                 \
  do {                                                                         \
    e->detached(e);                                                            \
    e = NULL;                                                                  \
  } while (0)
#define FIO_MAP_KEY_DISCARD(e)
#define FIO___RECURSIVE_INCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
Message Uniqueness Map for filtering remote connection broadcasts
***************************************************************************** */

/* Managing Remote Connection Uniqueness */
#define FIO_MAP_NAME             fio___pubsub_message_map
#define FIO_MAP_KEY              fio___pubsub_message_s *
#define FIO_MAP_KEY_COPY(d_, e_) (d_ = fio___pubsub_message_dup(e_))
#define FIO_MAP_KEY_DESTROY(e)   fio___pubsub_message_free(e)
#define FIO_MAP_HASH_FN(m)       fio_risky_num(m->data.id, m->data.published)
#define FIO_MAP_RECALC_HASH      1
#define FIO_MAP_LRU              FIO___PUBSUB_CLUSTER_BACKLOG
#define FIO_MAP_KEY_DISCARD(e)
#define FIO___RECURSIVE_INCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
Pub/Sub Post Office State
***************************************************************************** */
#ifndef FIO___IPC_LEN
#define FIO___IPC_LEN 256
#endif

FIO_SFUNC void fio___pubsub_protocol_on_attach(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_data_master(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_data_worker(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_data_remote(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_close(void *udata);

static struct FIO___PUBSUB_POSTOFFICE {
  fio_u128 uuid;
  fio_u512 secret;
  fio___channel_map_s channels;
  fio___channel_map_s patterns;
  struct {
    uint8_t publish;
    uint8_t local;
    uint8_t remote;
  } filter;
  uint8_t secret_is_random;
  fio___pubsub_engines_s engines;
  FIO_LIST_NODE history_active;
  FIO_LIST_NODE history_waiting;
  fio___postoffice_msmap_s master_subscriptions;
  fio___pubsub_broadcast_connected_s remote_uuids;
  fio___pubsub_message_map_s remote_messages;
  fio___pubsub_message_map_s history_messages;
  struct {
    fio_protocol_s ipc;
    fio_protocol_s remote;
  } protocol;
  fio_s *broadcaster;
  struct {
    fio_msg_metadata_fn build;
    void (*cleanup)(void *);
    size_t ref;
  } metadata[FIO___PUBSUB_METADATA_STORE_LIMIT];
  char ipc_url[FIO___IPC_LEN];
} FIO___PUBSUB_POSTOFFICE = {
    .filter =
        {
            .publish = (FIO___PUBSUB_PROCESS | FIO___PUBSUB_ROOT),
            .local = (FIO___PUBSUB_SIBLINGS),
            .remote = FIO___PUBSUB_REMOTE,
        },
    .protocol =
        {
            .ipc =
                {
                    .on_attach = fio___pubsub_protocol_on_attach,
                    .on_data = fio___pubsub_protocol_on_data_master,
                    .on_close = fio___pubsub_protocol_on_close,
                    .on_timeout = fio_touch,
                },
            .remote =
                {
                    .on_attach = fio___pubsub_protocol_on_attach,
                    .on_data = fio___pubsub_protocol_on_data_remote,
                    .on_close = fio___pubsub_protocol_on_close,
                    .on_timeout = NULL,
                },
        },
};

/** Returns the secret key for a message with stated `rndm` value. */
FIO_IFUNC const void *fio___pubsub_secret_key(uint64_t rndm) {
  return (void *)&FIO___PUBSUB_POSTOFFICE.secret.u8[rndm & 15];
}

/* *****************************************************************************
PostOffice Helpers
***************************************************************************** */

/** Sets the current IPC socket address (shouldn't be changed while running). */
SFUNC int fio_pubsub_ipc_url_set(char *str, size_t len) {
  if (fio_srv_is_running() || len >= FIO___IPC_LEN)
    return -1;
  fio_str_info_s url =
      FIO_STR_INFO3(FIO___PUBSUB_POSTOFFICE.ipc_url, 0, FIO___IPC_LEN);
  fio_string_write2(&url, NULL, FIO_STRING_WRITE_STR2(str, len));
  return 0;
}
/** Returns the current IPC socket address (shouldn't be changed). */
SFUNC const char *fio_pubsub_ipc_url(void) {
  return FIO___PUBSUB_POSTOFFICE.ipc_url;
}

/** Sets a (possibly shared) secret for securing pub/sub communication. */
SFUNC void fio_pubsub_secret_set(char *str, size_t len) {
  uint64_t fallback_secret = 0;
  FIO___PUBSUB_POSTOFFICE.secret_is_random = 0;
  if (!str || !len) {
    if ((str = getenv("SECRET"))) {
      const char *secret_length = getenv("SECRET_LENGTH");
      len = secret_length ? fio_atol((char **)&secret_length) : 0;
      if (!len)
        len = strlen(str);
    } else {
      fallback_secret = fio_rand64();
      str = (char *)&fallback_secret;
      len = sizeof(fallback_secret);
      FIO___PUBSUB_POSTOFFICE.secret_is_random = 1;
    }
  }
  FIO___PUBSUB_POSTOFFICE.secret = fio_sha512(str, len);
}

/* *****************************************************************************
Postoffice History Control
***************************************************************************** */

FIO_SFUNC void fio___pubub_on_history_start(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!FIO_LIST_IS_EMPTY(&FIO___PUBSUB_POSTOFFICE.history_active))
    return;
  FIO_LIST_EACH(fio_subscription_s,
                history_active,
                &FIO___PUBSUB_POSTOFFICE.history_active,
                s) {
    FIO_LIST_REMOVE(&s->history);
    FIO_LIST_REMOVE(&s->history_active);
    FIO_LIST_PUSH(&s->channel->history, &s->history);
    FIO_LIST_PUSH(&FIO___PUBSUB_POSTOFFICE.history_active, &s->history_active);
  }
}

FIO_SFUNC void fio___pubub_on_history_end(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  FIO_LIST_EACH(fio_subscription_s,
                history_active,
                &FIO___PUBSUB_POSTOFFICE.history_active,
                s) {
    FIO_LIST_REMOVE(&s->history);
    FIO_LIST_REMOVE(&s->history_active);
  }
}

/* *****************************************************************************
Postoffice Metadata Control
***************************************************************************** */

/* Returns zero (0) on success or -1 on failure. */
SFUNC int fio_message_metadata_add(fio_msg_metadata_fn metadata_func,
                                   void (*cleanup)(void *)) {
  for (size_t i = 0; i < FIO___PUBSUB_METADATA_STORE_LIMIT;
       ++i) { /* test existing */
    if (fio_atomic_add(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1) &&
        metadata_func == FIO___PUBSUB_POSTOFFICE.metadata[i].build)
      return 0;
    fio_atomic_sub(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1);
  }
  for (size_t i = 0; i < FIO___PUBSUB_METADATA_STORE_LIMIT;
       ++i) { /* insert if available */
    if (fio_atomic_add(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1)) {
      fio_atomic_sub(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1);
      continue;
    }
    FIO___PUBSUB_POSTOFFICE.metadata[i].build = metadata_func;
    FIO___PUBSUB_POSTOFFICE.metadata[i].cleanup = cleanup;
    return 0;
  }
  return -1;
}

/**
 * Removed the metadata callback.
 *
 * Removal might be delayed if live metatdata
 * exists.
 */
SFUNC void fio_message_metadata_remove(fio_msg_metadata_fn metadata_func) {
  for (size_t i = 0; i < FIO___PUBSUB_METADATA_STORE_LIMIT;
       ++i) { /* test existing */
    if (fio_atomic_add(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1) &&
        metadata_func == FIO___PUBSUB_POSTOFFICE.metadata[i].build) {
      fio_atomic_sub(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1);
    }
    fio_atomic_sub(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1);
  }
}

/** Finds the message's metadata, returning the data or NULL. */
SFUNC void *fio_message_metadata(fio_msg_s *msg,
                                 fio_msg_metadata_fn metadata_func) {
  for (size_t i = 0; i < FIO___PUBSUB_METADATA_STORE_LIMIT;
       ++i) { /* test existing */
    if (FIO___PUBSUB_POSTOFFICE.metadata[i].ref &&
        metadata_func == FIO___PUBSUB_POSTOFFICE.metadata[i].build) {
      return fio___pubsub_msg2internal(msg)->metadata[i];
    }
  }
  return NULL;
}

/* *****************************************************************************
Listening to Local Connections (IPC)
***************************************************************************** */

#if defined(DEBUG)
#define FIO___PUBSUB_HIDE_FROM_LOG 0
#else
#define FIO___PUBSUB_HIDE_FROM_LOG 1
#endif
/** Starts listening to IPC connections on a local socket. */
FIO_IFUNC void fio___pubsub_ipc_listen(void *ignr_) {
  (void)ignr_;
  if (fio_srv_is_worker()) {
    FIO_LOG_DEBUG2("(pub/sub) IPC socket skipped - no workers are spawned.");
    return;
  }
  FIO_ASSERT(fio_srv_listen(.url = FIO___PUBSUB_POSTOFFICE.ipc_url,
                            .protocol = &FIO___PUBSUB_POSTOFFICE.protocol.ipc,
                            .on_root = 1,
                            .hide_from_log = FIO___PUBSUB_HIDE_FROM_LOG),
             "(pub/sub) couldn't open a socket for IPC\n\t\t%s",
             FIO___PUBSUB_POSTOFFICE.ipc_url);
}
#undef FIO___PUBSUB_HIDE_FROM_LOG

/* *****************************************************************************
Postoffice Constructor / Destructor
***************************************************************************** */

/* listens for IPC connections. */
FIO_SFUNC void fio___pubsub_ipc_listen(void *);
/* protocol functions. */
FIO_SFUNC void fio___pubsub_protocol_on_attach(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_data_master(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_data_worker(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_data_remote(fio_s *io);
FIO_SFUNC void fio___pubsub_protocol_on_close(void *udata);

FIO_SFUNC void fio___pubsub_at_exit(void *ignr_) {
  (void)ignr_;
  fio___postoffice_msmap_destroy(&FIO___PUBSUB_POSTOFFICE.master_subscriptions);
  fio___pubsub_broadcast_connected_destroy(
      &FIO___PUBSUB_POSTOFFICE.remote_uuids);
  fio___pubsub_message_map_destroy(&FIO___PUBSUB_POSTOFFICE.remote_messages);
  fio___pubsub_message_map_destroy(&FIO___PUBSUB_POSTOFFICE.history_messages);
  fio___pubsub_engines_destroy(&FIO___PUBSUB_POSTOFFICE.engines);
}

/** Callback called by the letter protocol entering a child processes. */
FIO_SFUNC void fio___pubsub_on_enter_child(void *ignr_) {
  (void)ignr_;
  FIO___PUBSUB_POSTOFFICE.protocol.ipc.on_data =
      fio___pubsub_protocol_on_data_worker;

  FIO___PUBSUB_POSTOFFICE.filter.publish = FIO___PUBSUB_PROCESS;
  FIO___PUBSUB_POSTOFFICE.filter.local =
      (FIO___PUBSUB_SIBLINGS | FIO___PUBSUB_ROOT);
  FIO___PUBSUB_POSTOFFICE.filter.remote = 0;
  fio___postoffice_msmap_destroy(&FIO___PUBSUB_POSTOFFICE.master_subscriptions);
  fio___pubsub_engines_destroy(&FIO___PUBSUB_POSTOFFICE.engines);
  if (!fio_srv_attach_fd(fio_sock_open2(FIO___PUBSUB_POSTOFFICE.ipc_url,
                                        FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK),
                         &FIO___PUBSUB_POSTOFFICE.protocol.ipc,
                         NULL,
                         NULL)) {
    FIO_LOG_FATAL("%d couldn't connect to pub/sub socket @ %s",
                  fio_srv_pid(),
                  FIO___PUBSUB_POSTOFFICE.ipc_url);
    fio_thread_kill(fio_srv_root_pid(), SIGINT);
    FIO_ASSERT(0, "fatal error encountered");
  }
}

FIO_CONSTRUCTOR(fio_postoffice_init) {
  FIO___PUBSUB_POSTOFFICE.engines = (fio___pubsub_engines_s)FIO_MAP_INIT;
  fio_pubsub_secret_set(NULL, 0); /* allocate a random secret */
  for (size_t i = 0; i < sizeof(FIO___PUBSUB_POSTOFFICE.uuid) / 8; ++i)
    FIO___PUBSUB_POSTOFFICE.uuid.u64[i] = fio_rand64();
  fio_str_info_s url =
      FIO_STR_INFO3(FIO___PUBSUB_POSTOFFICE.ipc_url, 0, FIO___IPC_LEN);

  fio_string_write2(&url,
                    NULL,
                    FIO_STRING_WRITE_STR1((char *)"priv://facil_io_tmp_"),
                    FIO_STRING_WRITE_HEX(fio_rand64()),
                    FIO_STRING_WRITE_STR1((char *)".sock"));
  fio_state_callback_add(FIO_CALL_PRE_START, fio___pubsub_ipc_listen, NULL);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___pubsub_on_enter_child, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___pubsub_at_exit, NULL);
  /* TODO!!! */
  FIO___PUBSUB_POSTOFFICE.protocol.ipc = (fio_protocol_s){
      .on_attach = fio___pubsub_protocol_on_attach,
      .on_data = fio___pubsub_protocol_on_data_master,
      .on_close = fio___pubsub_protocol_on_close,
      .on_timeout = fio_touch,
  };
  FIO___PUBSUB_POSTOFFICE.protocol.remote = (fio_protocol_s){
      .on_attach = fio___pubsub_protocol_on_attach,
      .on_data = fio___pubsub_protocol_on_data_remote,
      .on_close = fio___pubsub_protocol_on_close,
      .on_timeout = fio_touch,
  };
}

/* *****************************************************************************
Subscription Setup
***************************************************************************** */

/** Completes the subscription request. */
FIO_IFUNC void fio___pubsub_subscribe_task(void *sub_, void *ignr_) {
  fio_subscription_s *sub = (fio_subscription_s *)sub_;
  union {
    FIO_LIST_HEAD *ls;
    fio_str_info_s *str;
  } uptr = {.ls = &sub->node};
  fio_str_info_s ch_name = *uptr.str;
  sub->node = FIO_LIST_INIT(sub->node);
  sub->history = FIO_LIST_INIT(sub->history);
  sub->history_active = FIO_LIST_INIT(sub->history_active);

  fio_channel_s **ch_ptr =
      fio___channel_map_node2key_ptr(fio___channel_map_set_ptr(
          &FIO___PUBSUB_POSTOFFICE.channels + (ch_name.capa >> 16),
          ch_name));
  fio_bstr_free(ch_name.buf);
  if (FIO_UNLIKELY(!ch_ptr))
    goto no_channel;
  sub->channel = ch_ptr[0];
  FIO_LIST_PUSH(&(ch_ptr[0]->subscriptions), &sub->node);
  if (sub->replay_since) {
    FIO_LIST_PUSH(&FIO___PUBSUB_POSTOFFICE.history_waiting, &sub->history);
    /* TODO: publish history request event to the cluster. */
  }
  return;
no_channel:
  fio___pubsub_subscription_unsubscribe(sub);
  (void)ignr_;
}

/** Unsubscribes a node and destroys the channel if no more subscribers. */
FIO_IFUNC void fio___pubsub_unsubscribe_task(void *sub_, void *ignr_) {
  fio_subscription_s *sub = (fio_subscription_s *)sub_;
  fio_channel_s *ch = sub->channel;
  fio___channel_map_s *map;
  FIO_LIST_REMOVE(&sub->node);
  FIO_LIST_REMOVE(&sub->history);
  FIO_LIST_REMOVE(&sub->history_active);
  if (FIO_UNLIKELY(!ch))
    goto no_channel;

  if (FIO_LIST_IS_EMPTY(&ch->subscriptions)) {
    map = &FIO___PUBSUB_POSTOFFICE.channels + ch->is_pattern;
    fio___channel_map_remove(map, FIO___PUBSUB_CHANNEL2STR(ch), NULL);
    if (!fio___channel_map_count(map))
      fio___channel_map_destroy(map);
  }
  sub->channel = NULL;

no_channel:
  fio_subscription_free(sub);
  return;
  (void)ignr_;
}

/** Performs Housekeeping and defers the on_unsubscribe callback. */
FIO_IFUNC void fio___pubsub_subscription_unsubscribe(fio_subscription_s *s) {
  if (!s)
    return;
  s->on_message = fio___subscription_mock_cb;
  fio_queue_push(fio_srv_queue(),
                 fio___pubsub_unsubscribe_task,
                 (void *)s,
                 NULL);
}

/** Subscribes to a named channel in the numerical filter's namespace. */
void fio_subscribe___(void); /* sublimetext marker */
SFUNC void fio_subscribe FIO_NOOP(fio_subscribe_args_s args) {
  fio_subscription_s *s = NULL;
  union {
    FIO_LIST_HEAD *ls;
    fio_str_info_s *str;
  } uptr;
  if (args.channel.len > 0xFFFFUL)
    goto sub_error;
  s = fio_subscription_new();
  if (!s)
    goto sub_error;

  *s = (fio_subscription_s){
      .io = args.io,
      .on_message =
          (args.on_message ? args.on_message
                           : (args.io ? fio___subscription_call_protocol
                                      : fio___subscription_mock_cb)),
      .on_unsubscribe = args.on_unsubscribe,
      .udata = args.udata,
      .replay_since = args.replay_since,
  };
  args.is_pattern = !!args.is_pattern; /* make sure this is either 1 or zero */
  uptr.ls = &s->node;
  *uptr.str = FIO_STR_INFO3(
      (args.channel.len
           ? fio_bstr_write(NULL, args.channel.buf, args.channel.len)
           : NULL),
      args.channel.len,
      FIO___PUBSUB_CHANNEL_ENCODE_CAPA(args.filter, args.is_pattern));

  fio_srv_defer(fio___pubsub_subscribe_task, (void *)s, NULL);

  if (args.master_only && !args.io)
    goto is_master_only;
  if (!args.subscription_handle_ptr) {
    fio_env_set(args.io,
                .type = (intptr_t)(0LL - (((2ULL | (!!args.is_pattern)) << 16) |
                                          (uint16_t)args.filter)),
                .name = args.channel,
                .on_close =
                    (void (*)(void *))fio___pubsub_subscription_unsubscribe,
                .udata = s);
    return;
  }
  *args.subscription_handle_ptr = (uintptr_t)s;
  return;

is_master_only:
  if (fio_srv_is_master()) {
    fio___postoffice_msmap_set(
        &FIO___PUBSUB_POSTOFFICE.master_subscriptions,
        fio_risky_hash(args.channel.buf, args.channel.len, args.filter),
        FIO_STR_INFO3(args.channel.buf, args.channel.len, (size_t)-1),
        s,
        NULL);
  } else {
    fio_channel_free(s->channel);
    fio_subscription_free(s);
    FIO_LOG_WARNING(
        "%d master-only subscription attempt on a non-master process: %.*s",
        fio_srv_pid(),
        (int)args.channel.len,
        args.channel.buf);
  }
  return;

sub_error:
  FIO_LOG_ERROR("%d (pubsub) subscription/channel cannot be created?"
                "\n\t%zu bytes long\n\t%.*s...",
                fio_srv_pid(),
                args.channel.len,
                (int)(args.channel.len > 10 ? 7 : args.channel.len),
                args.channel.buf);
  FIO_LOG_ERROR("failed to allocate a new subscription");
  if (args.on_unsubscribe) {
    union {
      void *p;
      void (*fn)(void *udata);
    } u = {.fn = args.on_unsubscribe};
    fio_queue_push(fio_srv_queue(),
                   fio___pubsub_subscription_on_destroy__task,
                   u.p,
                   args.udata);
  }
  return;
}

/** Cancels an existing subscriptions. */
void fio_unsubscribe___(void); /* sublimetext marker */
int fio_unsubscribe FIO_NOOP(fio_subscribe_args_s args) {
  if (args.master_only && !args.io)
    goto is_master_only;
  if (!args.subscription_handle_ptr) {
    return fio_env_remove(
        args.io,
        .type = (intptr_t)(0LL - (((2ULL | (!!args.is_pattern)) << 16) |
                                  (uint16_t)args.filter)),
        .name = args.channel);
  }
  fio___pubsub_subscription_unsubscribe(
      *(fio_subscription_s **)args.subscription_handle_ptr);
  return 0;

is_master_only:
  return fio___postoffice_msmap_remove(
      &FIO___PUBSUB_POSTOFFICE.master_subscriptions,
      fio_risky_hash(args.channel.buf, args.channel.len, args.filter),
      FIO_STR_INFO3(args.channel.buf, args.channel.len, (size_t)-1),
      NULL);
}

/* *****************************************************************************
Pub/Sub Message Distribution (local process)
***************************************************************************** */

/* performs the subscription callback */
FIO_IFUNC void fio___subscription_on_message_task(void *s_, void *m_) {
  fio_subscription_s *s = (fio_subscription_s *)s_;
  fio___pubsub_message_s *m = (fio___pubsub_message_s *)m_;
  struct {
    fio_msg_s msg;
    fio___pubsub_message_s *m;
    uintptr_t flag;
  } container = {
      .msg = m->data,
      .m = m,
  };
  container.msg.io = s->io;
  container.msg.udata = s->udata;
  container.msg.is_json = !!(container.msg.is_json & FIO___PUBSUB_JSON);
  s->on_message(&container.msg);
  s->udata = container.msg.udata;
  if (container.flag)
    goto reschedule;
  fio_subscription_free(s);
  fio___pubsub_message_free(m);
  return;
reschedule:
  fio_queue_push(fio_srv_queue(), fio___subscription_on_message_task, s_, m_);
}

/* returns the internal message object. */
FIO_IFUNC fio___pubsub_message_s *fio___pubsub_msg2internal(fio_msg_s *msg) {
  return *(fio___pubsub_message_s **)(msg + 1);
}

/** Defers the current callback, so it will be called again for the message. */
SFUNC void fio_pubsub_message_defer(fio_msg_s *msg) {
  ((uintptr_t *)(msg + 1))[1] = 1;
}

/* distributes a message to all of a channel's subscribers */
FIO_SFUNC void fio___pubsub_channel_deliver_task(void *ch_, void *m_) {
  fio_channel_s *ch = (fio_channel_s *)ch_;
  fio___pubsub_message_s *m = (fio___pubsub_message_s *)m_;
  FIO_LIST_HEAD *head = (&ch->subscriptions);
  _Bool is_history = !!(m->data.is_json & FIO___PUBSUB_REPLAY);
  head += is_history;
  if (m->data.io) { /* move as many `if` statements as possible out of loops. */
    if (is_history) {
      FIO_LIST_EACH(fio_subscription_s, node, head, s) {
        if (m->data.io != s->io && m->data.published >= s->replay_since)
          fio_queue_push(
              fio_srv_queue(),
              (void (*)(void *, void *))fio___subscription_on_message_task,
              fio_subscription_dup(s),
              fio___pubsub_message_dup(m));
      }
    } else {
      FIO_LIST_EACH(fio_subscription_s, node, head, s) {
        if (m->data.io != s->io)
          fio_queue_push(
              fio_srv_queue(),
              (void (*)(void *, void *))fio___subscription_on_message_task,
              fio_subscription_dup(s),
              fio___pubsub_message_dup(m));
      }
    }
  } else {
    if (is_history) {
      FIO_LIST_EACH(fio_subscription_s, node, head, s) {
        if (m->data.published >= s->replay_since)
          fio_queue_push(
              fio_srv_queue(),
              (void (*)(void *, void *))fio___subscription_on_message_task,
              fio_subscription_dup(s),
              fio___pubsub_message_dup(m));
      }
    } else {
      FIO_LIST_EACH(fio_subscription_s, node, head, s) {
        fio_queue_push(
            fio_srv_queue(),
            (void (*)(void *, void *))fio___subscription_on_message_task,
            fio_subscription_dup(s),
            fio___pubsub_message_dup(m));
      }
    }
  }
  fio___pubsub_message_free(m);
  fio_channel_free(ch);
}

/** Callback called when a letter is destroyed (reference counting). */
FIO_SFUNC void fio___pubsub_message_metadata_init(fio___pubsub_message_s *m);
/** distributes a message to all matching channels */
FIO_SFUNC void fio___pubsub_message_deliver(fio___pubsub_message_s *m) {
  fio___pubsub_message_metadata_init(m); /* metadata initialization */
  fio_str_info_s ch_name =
      FIO_STR_INFO3(m->data.channel.buf,
                    m->data.channel.len,
                    FIO___PUBSUB_CHANNEL_ENCODE_CAPA(m->data.filter, 0));
  fio_channel_s **ch_ptr = fio___channel_map_node2key_ptr(
      fio___channel_map_get_ptr(&FIO___PUBSUB_POSTOFFICE.channels, ch_name));
  if (ch_ptr)
    fio_queue_push(fio_srv_queue(),
                   fio___pubsub_channel_deliver_task,
                   fio_channel_dup(ch_ptr[0]),
                   fio___pubsub_message_dup(m));
  FIO_MAP_EACH(fio___channel_map, &FIO___PUBSUB_POSTOFFICE.patterns, i) {
    if (i.node->key->filter == m->data.filter &&
        FIO_PUBSUB_PATTERN_MATCH(i.key, ch_name))
      fio_queue_push(fio_srv_queue(),
                     fio___pubsub_channel_deliver_task,
                     fio_channel_dup(i.node->key),
                     fio___pubsub_message_dup(m));
  }
}

FIO_SFUNC void fio___pubsub_message_deliver_task(void *m_, void *ignr_) {
  fio___pubsub_message_deliver((fio___pubsub_message_s *)m_);
  fio___pubsub_message_free((fio___pubsub_message_s *)m_);
  (void)ignr_;
}

/* *****************************************************************************
Pub/Sub Message Type (internal data carrying structure)
***************************************************************************** */

/** Callback called when a letter is destroyed (reference counting). */
FIO_SFUNC void fio___pubsub_message_metadata_init(fio___pubsub_message_s *m) {
  if (fio_atomic_or(&m->metadata_is_initialized, 1)) {
    return;
  }
  fio_msg_s msg = m->data;
  msg.is_json &= FIO___PUBSUB_JSON;
  for (size_t i = 0; i < FIO___PUBSUB_METADATA_STORE_LIMIT; ++i) {
    if (fio_atomic_add(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1)) {
      m->metadata[i] = FIO___PUBSUB_POSTOFFICE.metadata[i].build(&msg);
      continue;
    }
    fio_atomic_sub(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1);
  }
}

/** Callback called when a letter is destroyed (reference counting). */
FIO_SFUNC void fio___pubsub_message_metadata_free(fio___pubsub_message_s *m) {
  if (!m->metadata_is_initialized)
    return;
  for (size_t i = 0; i < FIO___PUBSUB_METADATA_STORE_LIMIT; ++i) {
    if (fio_atomic_add(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1)) {
      FIO___PUBSUB_POSTOFFICE.metadata[i].cleanup(m->metadata[i]);
      fio_atomic_sub(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1);
    }
    fio_atomic_sub(&FIO___PUBSUB_POSTOFFICE.metadata[i].ref, 1);
  }
  m->metadata_is_initialized = 0;
}

/** Callback called when a letter is destroyed (reference counting). */
FIO_SFUNC void fio___pubsub_message_on_destroy(fio___pubsub_message_s *m) {
  fio___pubsub_message_metadata_free(m);
}

FIO_IFUNC fio___pubsub_message_s *fio___pubsub_message_alloc(void *header) {
  fio___pubsub_message_s *m;
  const size_t channel_len = fio_buf2u16_le((char *)header + 18);
  const size_t message_len = fio_buf2u24_le((char *)header + 20);
  m = fio___pubsub_message_new(((channel_len + message_len) << 1) +
                               FIO___PUBSUB_MESSAGE_OVERHEAD);
  FIO_ASSERT_ALLOC(m);
  m->data = (fio_msg_s){
      .channel = FIO_BUF_INFO2(m->buf, channel_len),
      .message = FIO_BUF_INFO2(m->buf + channel_len + 1, message_len),
      .udata = m->buf + channel_len + message_len + 2,
  };
  return m;
}

FIO_IFUNC fio___pubsub_message_s *fio___pubsub_message_author(
    fio_publish_args_s args) {
  fio___pubsub_message_s *m =
      fio___pubsub_message_new(((args.message.len + args.channel.len) << 1) +
                               FIO___PUBSUB_MESSAGE_OVERHEAD);
  FIO_ASSERT_ALLOC(m);
  m->data = (fio_msg_s){
      .io = args.from,
      .id = args.id ? args.id : fio_rand64(),
      .published = args.published ? args.published
                                  : (uint64_t)fio_time2milli(fio_time_real()),
      .channel = FIO_BUF_INFO2(m->buf, args.channel.len),
      .message = FIO_BUF_INFO2(m->buf + args.channel.len + 1, args.message.len),
      .filter = args.filter,
      .is_json = args.is_json,
  };
  if (args.channel.len)
    FIO_MEMCPY(m->data.channel.buf, args.channel.buf, args.channel.len);
  m->data.channel.buf[args.channel.len] = 0;
  if (args.message.buf)
    FIO_MEMCPY(m->data.message.buf, args.message.buf, args.message.len);
  m->data.message.buf[args.message.len] = 0;
  return m;
}

FIO_SFUNC void fio___pubsub_message_encrypt(fio___pubsub_message_s *m) {
  if (m->data.udata)
    return;
  const void *k = fio___pubsub_secret_key(m->data.id);
  const uint64_t nonce[2] = {fio_risky_num(m->data.id, 0), m->data.published};
  uint8_t *pos = (uint8_t *)(m->data.message.buf + m->data.message.len + 1);
  m->data.udata = (void *)pos;
  fio_u2buf64_le(pos, m->data.id);
  pos += 8;
  fio_u2buf64_le(pos, m->data.published);
  pos += 8;
  fio_u2buf16_le(pos, m->data.filter);
  pos += 2;
  fio_u2buf16_le(pos, m->data.channel.len);
  pos += 2;
  fio_u2buf24_le(pos, m->data.message.len);
  pos += 3;
  *(pos++) = m->data.is_json;
  const size_t enc_len = m->data.channel.len + m->data.message.len + 2;
  FIO_MEMCPY(pos, m->data.channel.buf, enc_len);
  if (enc_len == 2)
    return;
  pos += enc_len;
  fio_chacha20_poly1305_enc(
      pos,
      (void *)((char *)(m->data.udata) + FIO___PUBSUB_MESSAGE_HEADER),
      m->data.channel.len + m->data.message.len + 2,
      m->data.udata,
      FIO___PUBSUB_MESSAGE_HEADER,
      k,
      nonce);
}

FIO_SFUNC int fio___pubsub_message_decrypt(fio___pubsub_message_s *m) {
  if (m->data.id)
    return 0;
  if (!m->data.udata)
    return -1;
  uint8_t *pos = (uint8_t *)(m->data.udata);
  m->data.id = fio_buf2u64_le(pos);
  pos += 8;
  m->data.published = fio_buf2u64_le(pos);
  pos += 8;
  m->data.filter = fio_buf2u16_le(pos);
  pos += 2;
  m->data.channel = FIO_BUF_INFO2(m->buf, fio_buf2u16_le(pos));
  pos += 2;
  m->data.message =
      FIO_BUF_INFO2(m->buf + m->data.channel.len + 1, fio_buf2u24_le(pos));
  pos += 3;
  m->data.is_json = *(pos++);
  const void *k = fio___pubsub_secret_key(m->data.id);
  uint64_t nonce[2] = {fio_risky_num(m->data.id, 0), m->data.published};
  const size_t enc_len = m->data.channel.len + m->data.message.len + 2;
  FIO_MEMCPY(m->buf, pos, enc_len);
  if (enc_len == 2)
    return 0;
  pos += enc_len;
  return fio_chacha20_poly1305_dec(pos,
                                   m->buf,
                                   m->data.channel.len + m->data.message.len +
                                       2,
                                   m->data.udata,
                                   FIO___PUBSUB_MESSAGE_HEADER,
                                   k,
                                   nonce);
}

FIO_IFUNC void fio___pubsub_message_is_dirty(fio___pubsub_message_s *m) {
  m->data.udata = NULL;
}

/* *****************************************************************************
Pub/Sub Message Object - IO helpers
***************************************************************************** */

FIO_IFUNC void fio___pubsub_message_write2io(fio_s *io, void *m_) {
  fio___pubsub_message_s *m = (fio___pubsub_message_s *)m_;
  if (io == m->data.io)
    return;
  FIO_LOG_DDEBUG2("%d (pubsub) sending IPC/peer message.", fio_srv_pid());
  fio___pubsub_message_encrypt(m);
  fio_write2(io,
             .buf = fio___pubsub_message_dup(m),
             .len = (m->data.message.len + m->data.channel.len +
                     FIO___PUBSUB_MESSAGE_OVERHEAD_NET),
             .offset = ((uintptr_t)(m->data.udata) - (uintptr_t)(m)),
             .dealloc = (void (*)(void *))fio___pubsub_message_free);
}

/* A callback for IO subscriptions - sends raw message data. */
FIO_SFUNC void FIO_ON_MESSAGE_SEND_MESSAGE(fio_msg_s *msg) {
  if (!msg || !msg->message.len)
    return;
  fio___pubsub_message_s *m = fio___pubsub_msg2internal(msg);
  fio_write2(msg->io,
             .buf = fio___pubsub_message_dup(m),
             .len = msg->message.len,
             .offset = (size_t)(msg->message.buf - (char *)m),
             .dealloc = (void (*)(void *))fio___pubsub_message_free);
}

/* *****************************************************************************
Pub/Sub Message Routing
***************************************************************************** */

FIO_SFUNC void fio___pubsub_message_route(fio___pubsub_message_s *m) {
  fio___pubsub_message_parser_s *p;
  unsigned flags = m->data.is_json;
  FIO_LOG_DDEBUG2("%d (pubsub) routing message (%x)",
                  fio_srv_pid(),
                  (int)m->data.is_json);

  if (flags & FIO___PUBSUB_INTERNAL_MESSAGE)
    goto is_special_message;

  if ((FIO___PUBSUB_POSTOFFICE.filter.publish & flags))
    fio_queue_push(fio_srv_queue(),
                   fio___pubsub_message_deliver_task,
                   fio___pubsub_message_dup(m));

  if ((FIO___PUBSUB_POSTOFFICE.filter.local & flags))
    fio_protocol_each(&FIO___PUBSUB_POSTOFFICE.protocol.ipc,
                      fio___pubsub_message_write2io,
                      m);

  if ((FIO___PUBSUB_POSTOFFICE.filter.remote & flags))
    fio_protocol_each(&FIO___PUBSUB_POSTOFFICE.protocol.remote,
                      fio___pubsub_message_write2io,
                      m);
  return;

is_special_message:
  FIO_LOG_DDEBUG2("%d (pubsub) internal subscription/ID message received",
                  fio_srv_pid());
  switch (flags) {
  case FIO___PUBSUB_SUB:
    fio_subscribe(.io = m->data.io,
                  .channel = m->data.channel,
                  .filter = m->data.filter,
                  .is_pattern = (uint8_t)(m->data.id - 1),
                  .on_message = fio___subscription_mock_cb);
    return;
  case FIO___PUBSUB_UNSUB:
    fio_unsubscribe(.io = m->data.io,
                    .channel = m->data.channel,
                    .filter = m->data.filter,
                    .is_pattern = (uint8_t)(m->data.id - 1),
                    .on_message = fio___subscription_mock_cb);
    return;

  case FIO___PUBSUB_IDENTIFY:
    p = (fio___pubsub_message_parser_s *)fio_udata_get(m->data.io);
    if (p) {
      p->uuid[0] = m->data.id;
      p->uuid[0] = m->data.published;
    }
    return;
  case FIO___PUBSUB_FORWARDER: /* fall through */
  case (FIO___PUBSUB_FORWARDER | FIO___PUBSUB_JSON):
    if (FIO___PUBSUB_POSTOFFICE.filter.remote) { /* root process */
      fio___pubsub_message_is_dirty(m);
      m->data.message.len -= 8;
      m->data.is_json &= FIO___PUBSUB_JSON;
      fio_pubsub_engine_s *e = (fio_pubsub_engine_s *)(uintptr_t)fio_buf2u64u(
          m->data.message.buf + m->data.message.len);
      m->data.message.buf[m->data.message.len] = 0;
      e->publish(e, &m->data);
    } else { /* child process */
      fio_protocol_each(&FIO___PUBSUB_POSTOFFICE.protocol.ipc,
                        fio___pubsub_message_write2io,
                        m);
    }
    return;

  case FIO___PUBSUB_HISTORY_START:
    FIO_LOG_DDEBUG2("%d (pubsub) internal history start message received",
                    fio_srv_pid());
    /* TODO! */
    return;
  case FIO___PUBSUB_HISTORY_END:
    FIO_LOG_DDEBUG2("%d (pubsub) internal history end message received",
                    fio_srv_pid());
    /* TODO! */
    return;
  }
  return;
}

/* *****************************************************************************
Pub/Sub - Publish
***************************************************************************** */

FIO_SFUNC void fio___publish_message_task(void *m_, void *ignr_) {
  (void)ignr_;
  fio___pubsub_message_s *m = (fio___pubsub_message_s *)m_;
  fio___pubsub_message_route(m);
  fio___pubsub_message_free(m);
}

/** Publishes a message to the relevant subscribers (if any). */
void fio_publish___(void); /* SublimeText marker*/
void fio_publish FIO_NOOP(fio_publish_args_s args) {
  if (FIO_UNLIKELY(args.channel.len > 0xFFFFUL)) {
    FIO_LOG_ERROR("%d (pubsub) channel name too long (%zu bytes)",
                  fio_srv_pid(),
                  args.channel.len);
    return;
  }
  if (FIO_UNLIKELY(args.message.len > 0xFFFFFFUL)) {
    FIO_LOG_ERROR("%d (pubsub) message payload too large (%zu bytes)",
                  fio_srv_pid(),
                  args.message.len);
    return;
  }
  fio___pubsub_message_s *m;
  fio_msg_s msg;

  if (!args.engine) {
    args.engine = FIO_PUBSUB_DEFAULT;
    if (!args.engine)
      args.engine = FIO_PUBSUB_DEFAULT = FIO_PUBSUB_CLUSTER;
    if (args.filter < 0)
      args.engine = FIO_PUBSUB_LOCAL;
  }
  if ((uintptr_t)args.engine > 0xFFUL)
    goto external_engine;

  m = fio___pubsub_message_author(args);
  m->data.is_json = ((!!args.is_json) | ((uint8_t)(uintptr_t)args.engine));

  fio_srv_defer(fio___publish_message_task, m, NULL);
  return;

external_engine:

  msg.message = args.message;
  args.message.buf = NULL;
  args.message.len += 8;

  m = fio___pubsub_message_author(args);
  m->data.is_json = ((!!args.is_json) | ((uint8_t)FIO___PUBSUB_FORWARDER));
  FIO_MEMCPY(m->data.message.buf, msg.message.buf, msg.message.len);
  fio_u2buf64u(m->data.message.buf + msg.message.len, (uintptr_t)args.engine);
  fio_srv_defer(fio___publish_message_task, m, NULL);
}

/* *****************************************************************************
Pub/Sub Message on-the-wire parsing
***************************************************************************** */

FIO_IFUNC void fio___pubsub_message_parse(
    fio_s *io,
    void (*cb)(fio_s *, fio___pubsub_message_s *)) {
  fio___pubsub_message_parser_s *parser =
      (fio___pubsub_message_parser_s *)fio_udata_get(io);
  if (!parser)
    return;
  size_t existing = parser->len;
  if (!parser->msg) {
    while (existing < FIO___PUBSUB_MESSAGE_HEADER) { /* get message length */
      size_t consumed = fio_read(io,
                                 parser->buf + existing,
                                 FIO___PUBSUB_MESSAGE_OVERHEAD_NET - existing);
      if (!consumed) {
        parser->len = existing;
        return;
      }
      existing += consumed;
    }
    parser->msg = fio___pubsub_message_alloc(parser->buf);
    FIO_MEMCPY(parser->msg->data.udata, parser->buf, existing);
  }
  /* known message length, read to end and publish */
  fio___pubsub_message_s *m = parser->msg;
  const size_t needed = m->data.channel.len + m->data.message.len +
                        FIO___PUBSUB_MESSAGE_OVERHEAD_NET;
  FIO_LOG_DDEBUG2("%d (pubsub) parsing IPC/peer message (%zu/%zu bytes)",
                  fio_srv_pid(),
                  existing,
                  needed);
  while (existing < needed) {
    size_t consumed =
        fio_read(io, (char *)m->data.udata + existing, needed - existing);
    if (!consumed) {
      parser->len = existing;
      return;
    }
    existing += consumed;
  }
  parser->msg = NULL;
  parser->len = 0;
  m->data.io = io;
  if (fio___pubsub_message_decrypt(m)) {
    FIO_LOG_SECURITY("%d (pubsub) message decryption error", fio_srv_pid());
    fio_close_now(io);
  } else {
    cb(io, m);
  }
  fio___pubsub_message_free(m);
  return; /* consume no more than 1 message at a time */
}

/* *****************************************************************************
Pub/Sub Protocols
***************************************************************************** */

FIO_SFUNC void fio___pubsub_on_message_master(fio_s *io,
                                              fio___pubsub_message_s *msg) {
  fio___pubsub_message_route(msg);
  (void)io;
}
FIO_SFUNC void fio___pubsub_on_message_worker(fio_s *io,
                                              fio___pubsub_message_s *msg) {
  fio___pubsub_message_route(msg);
  (void)io;
}
FIO_SFUNC void fio___pubsub_on_message_remote(fio_s *io,
                                              fio___pubsub_message_s *msg) {
  fio___pubsub_message_map_s *map = &FIO___PUBSUB_POSTOFFICE.remote_messages;
  map += !!(msg->data.is_json & FIO___PUBSUB_REPLAY);
  if (fio___pubsub_message_map_set(map, msg) != msg)
    return; /* already received */
  fio___pubsub_message_route(msg);
  (void)io;
}
FIO_SFUNC void fio___pubsub_protocol_on_attach(fio_s *io) {
  fio_udata_set(io, fio___pubsub_message_parser_new());
}
FIO_SFUNC void fio___pubsub_protocol_on_data_master(fio_s *io) {
  fio___pubsub_message_parse(io, fio___pubsub_on_message_master);
}
FIO_SFUNC void fio___pubsub_protocol_on_data_worker(fio_s *io) {
  fio___pubsub_message_parse(io, fio___pubsub_on_message_worker);
}
FIO_SFUNC void fio___pubsub_protocol_on_data_remote(fio_s *io) {
  fio___pubsub_message_parse(io, fio___pubsub_on_message_remote);
}
FIO_SFUNC void fio___pubsub_protocol_on_close(void *udata) {
  fio___pubsub_message_parser_s *p = (fio___pubsub_message_parser_s *)udata;
  if (p->uuid[0] || p->uuid[1]) {
    // TODO!: fio___pubsub_broadcast_hello(fio_s *io)
    fio___pubsub_broadcast_connected_remove(
        &FIO___PUBSUB_POSTOFFICE.remote_uuids,
        p->uuid[0],
        p->uuid[1],
        NULL);
  }
  fio___pubsub_message_parser_free(p);
}

void fio___pubsub_protocol_on_timeout(fio_s *io) {
  static const uint8_t ping_msg[FIO___PUBSUB_MESSAGE_OVERHEAD] = {
      [23] = FIO___PUBSUB_PING};
  fio_write2(io, .buf = (void *)ping_msg, .len = FIO___PUBSUB_MESSAGE_OVERHEAD);
}

/* *****************************************************************************
Pub/Sub Engine Support Implementation
***************************************************************************** */

static void fio___pubsub_mock_detached(const fio_pubsub_engine_s *eng) {
  (void)eng;
}
static void fio___pubsub_mock_sub_unsub(const fio_pubsub_engine_s *eng,
                                        fio_buf_info_s channel,
                                        int16_t filter) {
  (void)eng, (void)channel, (void)filter;
}
static void fio___pubsub_mock_publish(const fio_pubsub_engine_s *eng,
                                      fio_msg_s *msg) {
  (void)eng, (void)msg;
}

static void fio___pubsub_attach_task(void *engine_, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *engine = (fio_pubsub_engine_s *)engine_;
  if (!engine->detached)
    engine->detached = fio___pubsub_mock_detached;
  if (!engine->subscribe)
    engine->subscribe = fio___pubsub_mock_sub_unsub;
  if (!engine->unsubscribe)
    engine->unsubscribe = fio___pubsub_mock_sub_unsub;
  if (!engine->psubscribe)
    engine->psubscribe = fio___pubsub_mock_sub_unsub;
  if (!engine->punsubscribe)
    engine->punsubscribe = fio___pubsub_mock_sub_unsub;
  if (!engine->publish)
    engine->publish = fio___pubsub_mock_publish;
  fio___pubsub_engines_set(&FIO___PUBSUB_POSTOFFICE.engines, engine);
  FIO_MAP_EACH(fio___channel_map, &FIO___PUBSUB_POSTOFFICE.channels, i) {
    engine->subscribe(engine,
                      FIO_BUF_INFO2(i.key.buf, i.key.len),
                      (i.key.capa >> 16));
  }
  FIO_MAP_EACH(fio___channel_map, &FIO___PUBSUB_POSTOFFICE.patterns, i) {
    engine->psubscribe(engine,
                       FIO_BUF_INFO2(i.key.buf, i.key.len),
                       (i.key.capa >> 16));
  }
}

FIO_SFUNC void fio___pubsub_detach_task(void *engine, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *e = (fio_pubsub_engine_s *)engine;
  fio___pubsub_engines_remove(&FIO___PUBSUB_POSTOFFICE.engines, e, NULL);
}

/** Attaches an engine, so it's callback can be called by facil.io. */
SFUNC void fio_pubsub_attach(fio_pubsub_engine_s *engine) {
  if (!engine)
    return;
  fio_srv_defer(fio___pubsub_attach_task, engine, NULL);
}

/** Schedules an engine for Detachment, so it could be safely destroyed. */
SFUNC void fio_pubsub_detach(fio_pubsub_engine_s *engine) {
  fio_queue_push(fio_srv_queue(), fio___pubsub_detach_task, engine, NULL);
}

/* *****************************************************************************
Channel Creation / Destruction Callback (notifying engines)
***************************************************************************** */

/** Callback for when a channel is created. */
FIO_IFUNC void fio___channel_on_create(fio_channel_s *ch) {
  fio_buf_info_s name = FIO_BUF_INFO2(ch->name, ch->name_len);
  FIO_LOG_DDEBUG2("%d (pubsub) %s created, filter %d, length %zu bytes: %s",
                  fio_srv_pid(),
                  (ch->is_pattern ? "pattern" : "channel"),
                  (int)ch->filter,
                  (size_t)ch->name_len,
                  name.buf);
  FIO_MAP_EACH(fio___pubsub_engines, &FIO___PUBSUB_POSTOFFICE.engines, i) {
    (&i.key->subscribe + ch->is_pattern)[0](i.key, name, ch->filter);
  }
  if (!FIO___PUBSUB_POSTOFFICE.filter.remote) { /* inform root process */
    FIO_LOG_DDEBUG2("informing root process of new channel.");
    fio___pubsub_message_s *m =
        fio___pubsub_message_author((fio_publish_args_s){
            .id = (uint64_t)(ch->is_pattern + 1),
            .filter = ch->filter,
            .channel = FIO_BUF_INFO2(ch->name, ch->name_len),
            .is_json = FIO___PUBSUB_SUB,
        });
    if (m) {
      fio_protocol_each(&FIO___PUBSUB_POSTOFFICE.protocol.ipc,
                        fio___pubsub_message_write2io,
                        m);
      fio___pubsub_message_free(m);
    }
  }
}
/** Callback for when a channel is destroy. */
FIO_IFUNC void fio___channel_on_destroy(fio_channel_s *ch) {
  fio_buf_info_s name = FIO_BUF_INFO2(ch->name, ch->name_len);

  FIO_MAP_EACH(fio___pubsub_engines, &FIO___PUBSUB_POSTOFFICE.engines, i) {
    (&i.key->unsubscribe + ch->is_pattern)[0](i.key, name, ch->filter);
  }

  if (!FIO___PUBSUB_POSTOFFICE.filter.remote) { /* inform root process */
    fio___pubsub_message_s *m =
        fio___pubsub_message_author((fio_publish_args_s){
            .id = (uint64_t)(ch->is_pattern + 1),
            .filter = ch->filter,
            .channel = FIO_BUF_INFO2(ch->name, ch->name_len),
            .is_json = FIO___PUBSUB_UNSUB,
        });
    if (m) {
      fio_protocol_each(&FIO___PUBSUB_POSTOFFICE.protocol.ipc,
                        fio___pubsub_message_write2io,
                        m);
      fio___pubsub_message_free(m);
    }
  }

  FIO_LOG_DDEBUG2("%d (pubsub) %s destroyed, filter %d, length %zu bytes: %s",
                  fio_srv_pid(),
                  (ch->is_pattern ? "pattern" : "channel"),
                  (int)ch->filter,
                  (size_t)ch->name_len,
                  name.buf);
}

/* *****************************************************************************
Broadcasting for remote connections
***************************************************************************** */

FIO_IFUNC fio_u512 fio___pubsub_broadcast_compose(uint64_t tick,
                                                  uint64_t *ext) {
  /* [0-1]  Sender's 128 bit UUID
   * [2]    Random nonce
   * [3]    Timestamp in milliseconds
   * [4-5]  MAC
   * [6-7]  Receiver's 128 bit UUID (optional)
   */
  fio_u512 u = {0};
  uint64_t hello_rand = fio_rand64();
  const void *k = fio___pubsub_secret_key(hello_rand);
  u.u64[0] = FIO___PUBSUB_POSTOFFICE.uuid.u64[0];
  u.u64[1] = FIO___PUBSUB_POSTOFFICE.uuid.u64[1];
  u.u64[2] = fio_ltole64(hello_rand); /* persistent endienes required for k */
  u.u64[3] = fio_ltole64(tick);
  fio_poly1305_auth(u.u64 + 4, k, NULL, 0, u.u64, 32);
  if (!ext)
    return u;
  u.u64[6] = ext[0];
  u.u64[7] = ext[1];
  return u;
}

FIO_SFUNC void fio___pubsub_broadcast_hello(fio_s *io) {

  if (!fio_srv_is_running() || !(io = FIO___PUBSUB_POSTOFFICE.broadcaster))
    return;
  static int64_t last_hello = 0;
  int64_t this_hello = fio_srv_last_tick();
  if (last_hello == this_hello)
    return;
  fio_u512 u = fio___pubsub_broadcast_compose((last_hello = this_hello), NULL);
  struct sockaddr_in addr = (struct sockaddr_in){
      .sin_family = AF_INET,
      .sin_port = fio_lton16((uint16_t)(uintptr_t)fio_udata_get(io)),
      .sin_addr.s_addr = INADDR_BROADCAST, // inet_addr("255.255.255.255"),
  };
  FIO_LOG_DEBUG2("(pub/sub) sending broadcast.");
  sendto(fio_fd_get(io), u.u8, 48, 0, (struct sockaddr *)&addr, sizeof(addr));
}

FIO_SFUNC int fio___pubsub_broadcast_hello_validate(uint64_t *hello) {
  uint64_t mac[2] = {0};
  /* test server UUID (ignore self generated messages) */
  if (hello[0] == FIO___PUBSUB_POSTOFFICE.uuid.u64[0] &&
      hello[1] == FIO___PUBSUB_POSTOFFICE.uuid.u64[1])
    return -1;
  /* test time window */
  mac[0] = fio_srv_last_tick();
  if (mac[0] > fio_ltole64(hello[3]) + 8192 ||
      mac[0] + 8192 < fio_ltole64(hello[3])) {
    FIO_LOG_SECURITY(
        "(pubsub-broadcast) timing error - possible replay attack?");
    return -1;
  }
  /* test for duplicate connections */
  if (fio___pubsub_broadcast_connected_get(
          &FIO___PUBSUB_POSTOFFICE.remote_uuids,
          hello[0],
          hello[1])) {
    FIO_LOG_DEBUG2("(pubsub-broadcast) Prevented duplicate connection!");
    return -1;
  }
  /* test MAC */
  const void *k = fio___pubsub_secret_key(fio_ltole64(hello[2]));
  fio_poly1305_auth(mac, k, NULL, 0, hello, 32);
  if (mac[0] != hello[4] || mac[1] != hello[5]) {
    FIO_LOG_SECURITY("(pubsub-broadcast) MAC failure - under attack?");
    return -1;
  }
  return 0;
}
/* *****************************************************************************
Letter Listening to Remote Connections - TODO!
*****************************************************************************
*/
FIO_SFUNC void fio___pubsub_broadcast_on_attach(fio_s *io) {
  fio___pubsub_broadcast_hello((FIO___PUBSUB_POSTOFFICE.broadcaster = io));
}
FIO_SFUNC void fio___pubsub_broadcast_on_close(void *ignr_) {
  FIO___PUBSUB_POSTOFFICE.broadcaster = NULL;
  (void)ignr_;
}

FIO_SFUNC void fio___pubsub_broadcast_on_data(fio_s *io) {
  uint64_t buf[16];
  struct sockaddr from[2];
  socklen_t from_len = sizeof(from);
  ssize_t len;
  int should_say_hello = 0;
  while ((len = recvfrom(fio_fd_get(io), buf, 128, 0, from, &from_len)) > 0) {
    if (len != 48 && len != 64) {
      FIO_LOG_WARNING(
          "pub/sub peer detection received invalid packet (%zu bytes)!",
          len);
      continue;
    }
    if (fio___pubsub_broadcast_hello_validate(buf)) {
      FIO_LOG_WARNING(
          "pub/sub peer detection received invalid packet payload!");
      return;
    }
    if (len == 48) {
      FIO_LOG_DDEBUG2("detected peer (1st trip), sending UUID");
      fio_u512 u = fio___pubsub_broadcast_compose(fio_srv_last_tick(), buf);
      sendto(fio_fd_get(io), u.u8, 64, 0, (struct sockaddr *)from, from_len);
      /* new peer in system? maybe there's more... */
      should_say_hello |= (fio___pubsub_broadcast_connected_get(
                               &FIO___PUBSUB_POSTOFFICE.remote_uuids,
                               buf[0],
                               buf[1]) == 0);
      continue;
    }
    FIO_LOG_DDEBUG2("detected peer (roundtrip), should now connect");

    if (fio___pubsub_broadcast_connected_get(
            &FIO___PUBSUB_POSTOFFICE.remote_uuids,
            buf[0],
            buf[1]) == buf[1])
      continue; /* skip connection, already exists. */
    /* TODO: fixme! */
    from->sa_family = AF_INET;
    int fd = fio_sock_open_remote((struct addrinfo *)from, 1);
    if (fd == 1) {
      FIO_LOG_ERROR("couldn't connect to cluster peer: %s", strerror(errno));
      continue;
    }
    fio___pubsub_broadcast_connected_set(&FIO___PUBSUB_POSTOFFICE.remote_uuids,
                                         buf[0],
                                         buf[1]);
    fio_srv_attach_fd(fd, &FIO___PUBSUB_POSTOFFICE.protocol.remote, NULL, NULL);
  }
  if (should_say_hello)
    fio___pubsub_broadcast_hello(io);
}

FIO_SFUNC void fio___pubsub_broadcast_on_incoming(fio_s *io) {
  int fd;
  fio___pubsub_message_s *m = fio___pubsub_message_author(
      (fio_publish_args_s){.id = FIO___PUBSUB_POSTOFFICE.uuid.u64[0],
                           .published = FIO___PUBSUB_POSTOFFICE.uuid.u64[1],
                           .is_json = FIO___PUBSUB_IDENTIFY});
  while ((fd = accept(fio_fd_get(io), NULL, NULL)) != -1) {
    FIO_LOG_DDEBUG2("accepting a cluster peer connection");
    fio_s *client = fio_srv_attach_fd(fd,
                                      &FIO___PUBSUB_POSTOFFICE.protocol.remote,
                                      NULL,
                                      NULL);
    fio___pubsub_message_write2io(client, m);
  }
  fio___pubsub_message_free(m);
}

SFUNC void fio___pubsub_broadcast_on_port(void *port_) {
  int16_t port = (int16_t)(uintptr_t)port_;
  static fio_protocol_s broadcast = {
      .on_attach = fio___pubsub_broadcast_on_attach,
      .on_data = fio___pubsub_broadcast_on_data,
      .on_close = fio___pubsub_broadcast_on_close,
      .on_timeout = fio_touch,
  };
  static fio_protocol_s accept_remote = {
      .on_data = fio___pubsub_broadcast_on_incoming,
      .on_timeout = fio_touch,
  };
  if (FIO___PUBSUB_POSTOFFICE.secret_is_random) {
    FIO_LOG_ERROR(
        "Listening to cluster peer connections failed!"
        "\n\tUsing a random (non-shared) secret, cannot validate peers.");
    return;
  }
  if (!port || port < 0)
    port = 3333;
  FIO_STR_INFO_TMP_VAR(url, 32);
  url.buf[0] = ':';
  url.len = 1;
  fio_string_write_u(&url, NULL, (uint64_t)port);

  int fd_udp =
      fio_sock_open(NULL,
                    url.buf + 1,
                    FIO_SOCK_UDP | FIO_SOCK_NONBLOCK | FIO_SOCK_SERVER);
  int fd_tcp =
      fio_sock_open(NULL,
                    url.buf + 1,
                    FIO_SOCK_TCP | FIO_SOCK_NONBLOCK | FIO_SOCK_SERVER);
  FIO_ASSERT(fd_udp != -1 && fd_tcp != -1, "couldn't open broadcast socket!");
  {
    int enabled = 1;
    setsockopt(fd_udp, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));
    enabled = 1;
    setsockopt(fd_udp, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
  }
  fio_srv_attach_fd(fd_udp, &broadcast, port_, NULL);
  fio_srv_attach_fd(fd_tcp, &accept_remote, NULL, NULL);
  return;
}

/** Auto-peer detection and pub/sub multi-machine clustering using `port`. */
SFUNC void fio_pubsub_broadcast_on_port(int16_t port) {
  fio_state_callback_add(FIO_CALL_PRE_START,
                         fio___pubsub_broadcast_on_port,
                         (void *)(uintptr_t)port);
}

/* *****************************************************************************
Pub/Sub Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_PUBSUB
#endif /* FIO_PUBSUB */
