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
  /** Channel name namespace. Negative values are reserved. */
  int16_t filter;
  /** flag indicating if the message is JSON data or binary/text. */
  uint8_t is_json;
} fio_msg_s;

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
/** for backwards compatibility */
#define pubsub_publish fio_publish

/**
 * Defers the current callback, so it will be called again for the message.
 *
 * After calling this function, the `msg` object must NOT be accessed again.
 */
SFUNC void fio_message_defer(fio_msg_s *msg);

/* *****************************************************************************
Pub/Sub - defaults and builtin pub/sub engines
***************************************************************************** */

typedef enum {
  /** Flag bits for internal usage (letter exchange network format). */
  FIO___PUBSUB_PROCESS = 1,
  FIO___PUBSUB_ROOT = 2,
  FIO___PUBSUB_SIBLINGS = 4,
  FIO___PUBSUB_WORKERS = (4 | 1),
  FIO___PUBSUB_LOCAL = (4 | 2 | 1),
  FIO___PUBSUB_REMOTE = 8,
  FIO___PUBSUB_CLUSTER = (8 | 4 | 2 | 1),
  FIO___PUBSUB_SUB = 16,
  FIO___PUBSUB_UNSUB = 32,
  FIO___PUBSUB_JSON = 128,
} fio___letter_flag_bits_e;

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
 * Message metadata (advance usage API)
 **************************************************************************** */

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
#ifndef FIO_PUBSUB_METADATA_LIMIT
#define FIO_PUBSUB_METADATA_LIMIT 4
#endif

/** Pub/Sub Metadata callback type. */
typedef void *(*fio_msg_metadata_fn)(fio_str_info_s ch,
                                     fio_str_info_s msg,
                                     int16_t filter,
                                     uint8_t is_json);

/**
 * It's possible to attach metadata to facil.io pub/sub messages before they are
 * published.
 *
 * This allows, for example, messages to be encoded as network packets for
 * outgoing protocols (i.e., encoding for WebSocket transmissions), improving
 * performance in large network based broadcasting.
 *
 * Up to `FIO_PUBSUB_METADATA_LIMIT` metadata callbacks can be attached.
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
 * Pub/Sub Middleware and Extensions ("Engines")
 **************************************************************************** */

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
 * (i.e., using `fio_defer`).
 */
struct fio_pubsub_engine_s {
  /** For internal facil.io use - initialize to zero(!) before calling `attach`
   */
  struct {
    FIO_LIST_NODE node;
  } internal_use_;
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
  void (*publish)(const fio_pubsub_engine_s *eng,
                  fio_buf_info_s channel,
                  fio_buf_info_s msg,
                  int16_t filter,
                  uint8_t is_json);
};

/**
 * Attaches an engine, so it's callback can be called by facil.io.
 *
 * The `(p)subscribe` callback will be called for every existing channel.
 *
 * This can be called multiple times resulting in re-running the `(p)subscribe`
 * callbacks.
 *
 * NOTE: the root (master) process will call `subscribe` for any channel in any
 * process, while all the other processes will call `subscribe` only for their
 * own channels. This allows engines to use the root (master) process as an
 * exclusive subscription process and publish to `FIO_PUBSUB_LOCAL`.
 */
SFUNC void fio_pubsub_attach(fio_pubsub_engine_s *engine);

/** Schedules an engine for Detachment, so it could be safely destroyed. */
SFUNC void fio_pubsub_detach(fio_pubsub_engine_s *engine);

/* *****************************************************************************
 * Pub/Sub Clustering and Security
 **************************************************************************** */

/** Returns the current IPC socket address (shouldn't be changed). */
SFUNC int fio_pubsub_ipc_url_set(char *str, size_t len);

/** Returns the current IPC socket address (shouldn't be changed). */
SFUNC const char *fio_pubsub_ipc_url(void);

/**
 * Sets a (possibly shared) secret for securing pub/sub communication.
 *
 * If `secret` is `NULL`, the environment variable `"SECRET"` will be used or,
 * if not set, a random secret will be generated.
 */
SFUNC void fio_pubsub_secret_set(char *secret, size_t len);

/* *****************************************************************************



Pub/Sub Implementation



The implementation has a big number of interconnected modules:
- Letters and their network exchange protocols (`fio_letter_s`)
- Distribution Channels (`fio_channel_s` and `FIO_POSTOFFICE`)
- Subscriptions (`fio_subscription_s`)
- External Distribution Engines (`fio_pubsub_engine_s`)
- Letter Metadata Management.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

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
FIO_SFUNC void fio_subscription___mock_cb(fio_msg_s *msg) { (void)msg; }
/* a default callback for IO subscriptions - sends message data. */
FIO_SFUNC void fio_subscription___send_cb(fio_msg_s *msg);

/* *****************************************************************************
 * Pub / Sub types, for internal use
 **************************************************************************** */

/** The Letter: contains the data to be distributed to subscribers. */
typedef struct fio_letter_s {
  void *from;
  uint8_t metadata_is_initialized; /* to compact this we need to change all? */
  void *metadata[FIO_PUBSUB_METADATA_LIMIT];
  char buf[];
} fio_letter_s;

/**
 * Reference counting: `fio_letter_dup(letter)` / `fio_letter_free(letter)`
 */
FIO_SFUNC void fio_letter_on_destroy(fio_letter_s *letter);
#define FIO_REF_NAME             fio_letter
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_DESTROY(obj)     fio_letter_on_destroy(&(obj))
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** The Distribution Channel: manages subscriptions to named channels. */
typedef struct fio_channel_s {
  FIO_LIST_HEAD subscriptions;
  uint32_t name_len;
  int16_t filter;
  uint8_t is_pattern;
  char name[];
} fio_channel_s;

/**
 * Reference counting: `fio_channel_dup(ch)` / `fio_channel_free(ch)`
 */
#define FIO_REF_NAME             fio_channel
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** The Subscription: contains subscriber data. */
typedef struct fio_subscription_s {
  FIO_LIST_NODE node;
  fio_s *io;
  fio_channel_s *channel;
  void (*on_message)(fio_msg_s *msg);
  void (*on_unsubscribe)(void *udata);
  void *udata;
} fio_subscription_s;

/**
 * Reference counting: `fio_subscription_dup(sb)` / `fio_subscription_free(sb)`
 */
FIO_SFUNC void fio_subscription_on_destroy(fio_subscription_s *sub);
#define FIO_REF_NAME             fio_subscription
#define FIO_REF_DESTROY(obj)     fio_subscription_on_destroy(&(obj))
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** The Channel Map: maps named channels. */
FIO_SFUNC void fio_channel_on_create(fio_channel_s *ch);
FIO_SFUNC void fio_channel_on_destroy(fio_channel_s *ch);
FIO_IFUNC int fio_channel___cmp(fio_channel_s *a, fio_channel_s *b) {
  /* when letter publishing, the channel name is stored in subscriptions.next */
  return a->filter == b->filter && a->name_len == b->name_len &&
         (!a->name_len ||
          !FIO_MEMCMP(
              a->name,
              (b->subscriptions.prev ? b->name : (char *)b->subscriptions.next),
              a->name_len));
}
FIO_IFUNC uint64_t fio_channel___hash(char *buf, size_t len, int16_t filter) {
  return fio_risky_hash(buf, len, (((size_t)filter << 17) | (size_t)filter));
}
#define FIO_MAP_NAME fio_channel_map
#define FIO_MAP_KEY  fio_channel_s *
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  do {                                                                         \
    (dest) = (src);                                                            \
    fio_channel_on_create((src));                                              \
  } while (0);
#define FIO_MAP_KEY_CMP(a, b) fio_channel___cmp((a), (b))
#define FIO_MAP_KEY_DESTROY(key)                                               \
  do {                                                                         \
    fio_channel_on_destroy(key);                                               \
    fio_channel_free((key));                                                   \
  } while (0)
#define FIO_MAP_KEY_DISCARD(key) fio_channel_free((key))
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************



                          The Letter Object &
Protocol



The Letter Object & Protocol is the internal message exchange protocol and
format used by the pub/sub service and API. Access is given for zero-copy uses.

The letter exchange protocol imposes the following limitations on message
exchange:

* Channel Names are limited to 2^16 bytes (65,536 bytes).

* Message payload is limited to 2^24 bytes (16,777,216 bytes == about 16Mb).

* Totally Empty messages are ignored.

Letter network format in bytes (16 byte header + 2 NUL bytes):
| [ 0-7 ] 8 bytes - Message ID |
| [ 8-9 ] 2 Bytes - numerical Filter |
| [10-11] 2 bytes - little endian channel length |
| [12-14] 3 bytes - little endian message length |
| [ 15 ]  1 Bytes - flags |
| X bytes - (channel length + 1 NUL terminator) |
| Y bytes - (message length + 1 NUL terminator) |

Total overhead: 18 bytes (16 byte header + 2 NUL terminators)

TODO!: update structure to:
 - add encryption
 - add timestamp
 - message ID should include an origin tag? counter?

New letter network format in bytes (16 byte header + 2 NUL bytes):
| [  0-7  ] 8 bytes  - Message ID                |
| [  8-11 ] 4 bytes  - Encrypted Segment Length  |
| [ 12-28 ] 16 bytes - Message MAC (AHEAD)       |
| ----- Encryption Starts ----                   |
| 8 byte header:                                 |
| [ 0-1 ] 2 Bytes - little endian filter number  |
| [ 2-3 ] 2 bytes - little endian channel length |
| [ 4-6 ] 3 bytes - little endian message length |
| [  7  ] 1 Bytes - flags                        |
| ----- Message Payload ----                     |
| X bytes, channel name + 1 NUL terminator byte  |
| Y bytes, message + 1 NUL terminator byte       |

***************************************************************************** */

/* *****************************************************************************
Letter Type API & Callbacks
***************************************************************************** */

/** Allocates a new letter wrapper to be filled
 * from an existing buffer. */
FIO_IFUNC fio_letter_s *fio_letter_new_read(const char *head);
/* allocates a new, fully composed, letter. */
FIO_IFUNC fio_letter_s *fio_letter_new_compose(fio_buf_info_s channel,
                                               fio_buf_info_s message,
                                               int16_t filter,
                                               uint8_t flags);
/** initializes a letter's metadata. */
FIO_SFUNC void fio_letter_initialize_metadata(fio_letter_s *l);
/** Returns a letter's message length (if any) */
FIO_IFUNC size_t fio_letter_message_len(fio_letter_s *l);
/** Returns a letter's channel length (if any) */
FIO_IFUNC size_t fio_letter_channel_len(fio_letter_s *l);
/** Returns a letter's channel. */
FIO_IFUNC fio_str_info_s fio_letter_channel(fio_letter_s *l);
/** Returns a letter's message. */
FIO_IFUNC fio_str_info_s fio_letter_message(fio_letter_s *l);
/** Returns a letter's numerical filter. */
FIO_IFUNC int16_t fio_letter_filter(fio_letter_s *l);
/** Returns the letter's flags (8 bits allowing for 8 distinct flags). */
FIO_IFUNC uint8_t fio_letter_flags(fio_letter_s *l);
/** Returns a letter's ID (8 bytes random number) */
FIO_IFUNC uint64_t fio_letter_id(fio_letter_s *l);
/* returns the letter object associated with the public message object. */
FIO_IFUNC fio_letter_s *fio_msg2letter(fio_msg_s *msg);

#define FIO___LETTER_HEADER_LENGTH 16 /* without NUL terminators */
#define FIO___LETTER_MINIMAL_LEN   (FIO___LETTER_HEADER_LENGTH + 2) /* 2xNUL */

/* *****************************************************************************
Letter Protocol API & Callbacks
***************************************************************************** */

/** Callback called by the letter protocol when a letter arrives @ master. */
FIO_SFUNC void fio___letter_on_recieved_root(fio_letter_s *letter);
/** Callback called by the letter protocol when a letter arrives @ child. */
FIO_SFUNC void fio___letter_on_recieved_child(fio_letter_s *letter);
/** Starts listening to IPC connections on a local socket. */
FIO_IFUNC void fio___pubsub_ipc_listen(void *ignr_);

/** Write a letter to a specific IO object */
FIO_IFUNC void fio_letter_write(fio_s *io, fio_letter_s *l);
/** Returns a letter's total network length */
FIO_IFUNC size_t fio_letter_len(fio_letter_s *l);

/** Listen to remote letter exchange clients (cluster letter exchange). */
FIO_IFUNC int fio_letter_remote_listen(const char *url, uint64_t app_key[2]);
/** Connect to remote letter exchange server (cluster letter exchange). */
FIO_IFUNC int fio_letter_remote_connect(const char *url, uint64_t app_key[2]);

/* *****************************************************************************
Channel Delivery API & Callbacks
***************************************************************************** */

/** Distributes letters to the channel's subscribers. */
FIO_SFUNC void fio___channel_deliver_task(void *ch_, void *l_);

/** Distributes letters to the distribution channels. */
FIO_IFUNC void fio___channel_deliver(fio_letter_s *letter);

#ifndef FIO_POSTOFFICE_THREAD_LOCK
/** Controls if the channel subscribe API is published or not. */
#define FIO_POSTOFFICE_THREAD_LOCK 0
#endif

/* *****************************************************************************
Subscription Type API
***************************************************************************** */

/* unsubscribes and defers the callback. */
FIO_IFUNC void fio___subscription_unsubscribe(fio_subscription_s *s);

/* *****************************************************************************
Subscription Management Tasks
***************************************************************************** */

/* The task to subscribe to a channel (called by `fio_defer`). */
FIO_SFUNC void fio___subscribe_task(void *ch_, void *sub_);

/** Unsubscribes a node and destroys the channel
 * if no more subscribers. */
FIO_IFUNC void fio___unsubscribe_task(void *ch_, void *sub_);

/** Delivers a letter to a subscription */
FIO_IFUNC void fio___subscription_on_message_task(void *s, void *l);

/** publishes a letter to the expecting processes by letter flags. */
FIO_SFUNC void fio___publish_letter_task(void *l_, void *ignr_);

/* *****************************************************************************
Postoffice State
***************************************************************************** */
#ifndef FIO___IPC_LEN
#define FIO___IPC_LEN 256
#endif

/* master only subscription map */
#if 0
#define FIO___PUBSUB_MASTER_MAP_HASH(s)                                        \
  fio_channel___hash((s[0]->channel->subscriptions.prev                        \
                          ? s[0]->channel->name                                \
                          : (char *)s[0]->channel->subscriptions.next),        \
                     s[0]->channel->name_len,                                  \
                     s[0]->channel->filter)
#define FIO___PUBSUB_MASTER_MAP_CMP(a, b)                                      \
  fio_channel___cmp(a[0]->channel, b[0]->channel)
FIO_TYPEDEF_IMAP_ARRAY(fio___postoffice_msmap,
                       fio_subscription_s *,
                       uint32_t,
                       FIO___PUBSUB_MASTER_MAP_HASH,
                       FIO___PUBSUB_MASTER_MAP_CMP,
                       FIO_IMAP_ALWAYS_VALID)
#undef FIO___PUBSUB_MASTER_MAP_HASH
#undef FIO___PUBSUB_MASTER_MAP_CMP
#else
#define FIO_MAP_KEY_KSTR
#define FIO_MAP_NAME             fio___postoffice_msmap
#define FIO_MAP_VALUE            fio_subscription_s *
#define FIO_MAP_VALUE_DESTROY(s) fio___subscription_unsubscribe(s)
#define FIO___RECURSIVE_INCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#endif

static struct FIO_POSTOFFICE {
#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_TYPE lock;
#endif
  fio_channel_map_s channels;
  fio_channel_map_s patterns;
  FIO_LIST_NODE engines;
  fio_protocol_s *siblings_protocol;
  fio___postoffice_msmap_s master_subscriptions;
  size_t secret[32 / sizeof(size_t)];
  uint8_t publish_filter;
  uint8_t local_send_filter;
  uint8_t remote_send_filter;
  char ipc_url[FIO___IPC_LEN];
} FIO_POSTOFFICE = {
#if FIO_POSTOFFICE_THREAD_LOCK
    .lock = FIO___LOCK_INIT,
#endif
    .channels = FIO_MAP_INIT,
    .patterns = FIO_MAP_INIT,
    .publish_filter = (FIO___PUBSUB_PROCESS | FIO___PUBSUB_ROOT),
    .local_send_filter = (FIO___PUBSUB_SIBLINGS),
    .remote_send_filter = FIO___PUBSUB_REMOTE,
};

/** Callback called when entering a child processes. */
FIO_SFUNC void fio___postoffice_on_enter_child(void *ignr_);
/* *****************************************************************************



                          Letter Metadata
                          Implementation



***************************************************************************** */

static struct {
  fio_msg_metadata_fn build;
  void (*cleanup)(void *);
  size_t ref;
} FIO_PUBSUB_METADATA[FIO_PUBSUB_METADATA_LIMIT];

/* Returns zero (0) on success or -1 on failure. */
SFUNC int fio_message_metadata_add(fio_msg_metadata_fn metadata_func,
                                   void (*cleanup)(void *)) {
  for (size_t i = 0; i < FIO_PUBSUB_METADATA_LIMIT; ++i) { /* test existing */
    if (fio_atomic_add(&FIO_PUBSUB_METADATA[i].ref, 1) &&
        metadata_func == FIO_PUBSUB_METADATA[i].build)
      return 0;
    fio_atomic_sub(&FIO_PUBSUB_METADATA[i].ref, 1);
  }
  for (size_t i = 0; i < FIO_PUBSUB_METADATA_LIMIT;
       ++i) { /* insert if available */
    if (fio_atomic_add(&FIO_PUBSUB_METADATA[i].ref, 1)) {
      fio_atomic_sub(&FIO_PUBSUB_METADATA[i].ref, 1);
      continue;
    }
    FIO_PUBSUB_METADATA[i].build = metadata_func;
    FIO_PUBSUB_METADATA[i].cleanup = cleanup;
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
  for (size_t i = 0; i < FIO_PUBSUB_METADATA_LIMIT; ++i) { /* test existing */
    if (fio_atomic_add(&FIO_PUBSUB_METADATA[i].ref, 1) &&
        metadata_func == FIO_PUBSUB_METADATA[i].build) {
      fio_atomic_sub(&FIO_PUBSUB_METADATA[i].ref, 1);
    }
    fio_atomic_sub(&FIO_PUBSUB_METADATA[i].ref, 1);
  }
}

/** Finds the message's metadata, returning the data or NULL. */
SFUNC void *fio_message_metadata(fio_msg_s *msg,
                                 fio_msg_metadata_fn metadata_func) {
  for (size_t i = 0; i < FIO_PUBSUB_METADATA_LIMIT; ++i) { /* test existing */
    if (FIO_PUBSUB_METADATA[i].ref &&
        metadata_func == FIO_PUBSUB_METADATA[i].build) {
      return fio_msg2letter(msg)->metadata[i];
    }
  }
  return NULL;
}

/** Callback called when a letter is destroyed (reference counting). */
FIO_SFUNC void fio_letter_on_destroy(fio_letter_s *l) {
  if (!l->metadata_is_initialized)
    return;
  for (size_t i = 0; i < FIO_PUBSUB_METADATA_LIMIT; ++i) {
    if (fio_atomic_add(&FIO_PUBSUB_METADATA[i].ref, 1)) {
      FIO_PUBSUB_METADATA[i].cleanup(l->metadata[i]);
      fio_atomic_sub(&FIO_PUBSUB_METADATA[i].ref, 1);
    }
    fio_atomic_sub(&FIO_PUBSUB_METADATA[i].ref, 1);
  }
}

/** Callback called when a letter is destroyed (reference counting). */
FIO_SFUNC void fio_letter_initialize_metadata(fio_letter_s *l) {
  if (fio_atomic_or(&l->metadata_is_initialized, 1)) {
    return;
  }
  for (size_t i = 0; i < FIO_PUBSUB_METADATA_LIMIT; ++i) {
    if (fio_atomic_add(&FIO_PUBSUB_METADATA[i].ref, 1)) {
      l->metadata[i] = FIO_PUBSUB_METADATA[i].build(
          fio_letter_channel(l),
          fio_letter_message(l),
          fio_letter_filter(l),
          ((fio_letter_flags(l) & FIO___PUBSUB_JSON) == FIO___PUBSUB_JSON));
      continue;
    }
    fio_atomic_sub(&FIO_PUBSUB_METADATA[i].ref, 1);
  }
}

/* *****************************************************************************



                                Letter / Message Object
                                    Implementation



***************************************************************************** */

/* allocates a new letter wrapper to be filled from an existing buffer. */
FIO_IFUNC fio_letter_s *fio_letter_new_read(const char *head) {
  fio_letter_s *l = NULL;
  uint32_t channel_len = fio_buf2u16_le(head + 10);
  uint32_t message_len = fio_buf2u32_le(head + 12) & 0x00FFFFFFUL;
  size_t len = FIO___LETTER_MINIMAL_LEN + channel_len + message_len;
  l = fio_letter_new(len);
  FIO_ASSERT_ALLOC(l);
  return l;
}

/* allocates a new, fully composed, letter. */
FIO_IFUNC fio_letter_s *fio_letter_new_compose(fio_buf_info_s channel,
                                               fio_buf_info_s message,
                                               int16_t filter,
                                               uint8_t flags) {
  /* TODO: update to add encryption */
  fio_letter_s *l = NULL;
  if (!(channel.len | message.len | (uint32_t)filter | (uint32_t)flags))
    return l;
  size_t len = FIO___LETTER_MINIMAL_LEN + channel.len + message.len;
  uint64_t message_id = fio_rand64();
  union {
    uint8_t *u8;
    uint16_t *u16;
    uint32_t *u32;
    uint64_t *u64;
  } u;
  if ((channel.len > 0xFFFFULL) | (message.len > 0xFFFFFFULL))
    goto len_error;

  l = fio_letter_new(len);
  FIO_ASSERT_ALLOC(l);

  u.u8 = (uint8_t *)l->buf;
  u.u64[0] = message_id;
  u.u16[4] = fio_ltole16(filter);
  u.u16[5] = fio_ltole16(channel.len);
  u.u32[3] = fio_ltole32((message.len | (((uint32_t)flags) << 24)));
  if (channel.len && channel.buf) {
    FIO_MEMCPY(l->buf + FIO___LETTER_HEADER_LENGTH, channel.buf, channel.len);
  }
  l->buf[FIO___LETTER_HEADER_LENGTH + channel.len] = 0;
  if (message.len && message.buf) {
    FIO_MEMCPY(l->buf + FIO___LETTER_HEADER_LENGTH + 1 + channel.len,
               message.buf,
               message.len);
  }
  l->buf[(FIO___LETTER_HEADER_LENGTH + 1) + channel.len + message.len] = 0;
  return l;

len_error:
  FIO_LOG_ERROR("(pubsub) payload too big - exceeds the "
                "16Mb limit!\n\t"
                "Channel name length: %u bytes\n\t"
                "Message data length: %u bytes",
                (unsigned int)channel.len,
                (unsigned int)message.len);
  return l;
}

/* returns a letter's message length (if any) */
FIO_IFUNC size_t fio_letter_message_len(fio_letter_s *l) {
  return (size_t)(0x00FFFFFFULL & fio_buf2u32_le(l->buf + 12));
}

/* returns a letter's channel length (if any) */
FIO_IFUNC size_t fio_letter_channel_len(fio_letter_s *l) {
  return (size_t)(fio_buf2u16_le(l->buf + 10));
}

/* returns a letter's channel. */
FIO_IFUNC fio_str_info_s fio_letter_channel(fio_letter_s *l) {
  return (fio_str_info_s){.buf = (l->buf + FIO___LETTER_HEADER_LENGTH),
                          .len = fio_letter_channel_len(l)};
}

/* returns a letter's message. */
FIO_IFUNC fio_str_info_s fio_letter_message(fio_letter_s *l) {
  return (fio_str_info_s){.buf = (l->buf + FIO___LETTER_HEADER_LENGTH + 1 +
                                  fio_letter_channel_len(l)),
                          .len = fio_letter_message_len(l)};
}

/* returns a letter's numerical filter. */
FIO_IFUNC int16_t fio_letter_filter(fio_letter_s *l) {
  return (int16_t)fio_ltole16(*(uint16_t *)(l->buf + 8));
}

/* returns the letter's flags (8 bits allowing
 * for 8 distinct flags). */
FIO_IFUNC uint8_t fio_letter_flags(fio_letter_s *l) {
  return (uint8_t)(l->buf[15]);
}

/* returns a letter's ID (8 bytes random number)
 */
FIO_IFUNC uint64_t fio_letter_id(fio_letter_s *l) {
  return fio_buf2u32u(l->buf);
}

/* returns a letter's length */
FIO_IFUNC size_t fio_letter_len(fio_letter_s *l) {
  return FIO___LETTER_HEADER_LENGTH + 2 + fio_letter_message_len(l) +
         fio_letter_channel_len(l);
}

/* *****************************************************************************



                  Letter Exchange Protocol
(Networking / IPC)



***************************************************************************** */

/* *****************************************************************************
Letter Sending
***************************************************************************** */

/* write a letter to an IO object */
FIO_IFUNC void fio_letter_write(fio_s *io, fio_letter_s *l) {
  if ((void *)io == l->from)
    return;
  fio_write2(io,
             .buf = (char *)fio_letter_dup(l),
             .offset = (uintptr_t)(((fio_letter_s *)0)->buf),
             .len = fio_letter_len(l),
             .dealloc = (void (*)(void *))fio_letter_free);
}

/* a default callback for IO subscriptions - sends message data. */
FIO_SFUNC void fio_subscription___send_cb(fio_msg_s *msg) {
  if (!msg->message.len)
    return;
  fio_letter_s *l = fio_msg2letter(msg);
  fio_write2(msg->io,
             .buf = fio_letter_dup(l),
             .len = fio_letter_message_len(l),
             .offset = sizeof(*l) + (FIO___LETTER_HEADER_LENGTH + 1 +
                                     fio_letter_channel_len(l)),
             .dealloc = (void (*)(void *))fio_letter_free);
}

/* *****************************************************************************
Letter Reading and Parsing
***************************************************************************** */

/* a letter parser object */
typedef struct {
  fio_letter_s *letter;
  size_t pos;
  char buf[FIO___LETTER_MINIMAL_LEN]; /* minimal
                                         message
                                         length */
} fio_letter_parser_s;

/* a new letter parser */
FIO_IFUNC fio_letter_parser_s *fio_letter_parser_new(void) {
  fio_letter_parser_s *p =
      (fio_letter_parser_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*p), 0);
  FIO_ASSERT_ALLOC(p);
  p->letter = NULL;
  p->pos = 0;
  return p;
}

/* free a letter parser */
FIO_IFUNC void fio_letter_parser_free(fio_letter_parser_s *p) {
  if (!p)
    return;
  fio_letter_free(p->letter);
  FIO_MEM_FREE_(p, sizeof(*p));
}

/* forwards letters to callback, returns 0.
 * Returns -1 on error. */
FIO_IFUNC int fio___letter_read(fio_s *io, void (*callback)(fio_letter_s *)) {
  fio_letter_parser_s *parser = (fio_letter_parser_s *)fio_udata_get(io);
  for (;;) {
    ssize_t r;
    if (parser->letter) {
      fio_letter_s *const letter = parser->letter;
      const size_t to_read = fio_letter_len(parser->letter);
      while (parser->pos < to_read) {
        r = fio_read(io, letter->buf + parser->pos, to_read - parser->pos);
        if (r <= 0)
          return 0;
        parser->pos += r;
      }
      callback(letter);
      fio_letter_free(letter);
      parser->letter = NULL;
      parser->pos = 0;
    }
  read_header:
    r = fio_read(io,
                 parser->buf + parser->pos,
                 FIO___LETTER_MINIMAL_LEN - parser->pos);
    if (r <= 0)
      return 0;
    parser->pos += r;
    if (parser->pos < FIO___LETTER_HEADER_LENGTH)
      return 0;
    union {
      uint8_t *u8;
      uint32_t *u32;
      uint64_t *u64;
    } u = {.u8 = (uint8_t *)parser->buf};
    if ((u.u64[0] | u.u64[1])) {
      parser->letter = fio_letter_new_read(parser->buf);
      if (!parser->letter)
        goto error;
      parser->letter->from = (void *)io;
      FIO_MEMCPY(parser->letter->buf, parser->buf, parser->pos);
      continue;
    }
    parser->pos = 0; /* skip PING: all zero header */
    goto read_header;
  }

error:
  fio_close(io);
  return -1;
}

/* *****************************************************************************
Remote Letter Processing - validate unique delivery.
***************************************************************************** */

#define FIO_OMAP_NAME           fio___letter_map
#define FIO_MAP_KEY             uint64_t
#define FIO_MAP_LRU             (1ULL << 16)
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

static struct {
  fio___letter_map_s map;
} fio___letter_validation = {
    .map = FIO_MAP_INIT,
};

FIO_SFUNC void fio___on_letter_remote(fio_letter_s *l) {
  const uint64_t hash = fio_risky_hash(l->buf, fio_letter_len(l), 0);
  const uint64_t letter_id = fio_letter_id(l);
  if (letter_id ==
      fio___letter_map_get(&fio___letter_validation.map, hash, letter_id))
    return;
  fio___letter_map_set(&fio___letter_validation.map, hash, letter_id);
  fio___letter_on_recieved_root(l);
}

/* *****************************************************************************
Letter Protocol - Handshake Callbacks (TODO!)
***************************************************************************** */

/* *****************************************************************************
Letter Protocol - Handshake Callbacks (TODO!)
***************************************************************************** */

// /** Called when an IO is attached to a protocol. */
// void (*on_attach)(fio_s *io);
// /** Called when a data is available. */
// void (*on_data)(fio_s *io);
// /** called once all pending `fio_write` calls are finished. */
// void (*on_ready)(fio_s *io);
// /** Called after the connection was closed, and pending tasks completed. */
// void (*on_close)(void *udata);

/* *****************************************************************************
Letter Protocol Callbacks
***************************************************************************** */

FIO_SFUNC void fio___letter_on_recieved_root(fio_letter_s *l) {
  fio_queue_push(fio_srv_queue(), fio___publish_letter_task, fio_letter_dup(l));
  (void)l;
}

FIO_SFUNC void fio___letter_on_recieved_child(fio_letter_s *l) {
  fio___channel_deliver(l);
  (void)l;
}

FIO_SFUNC void fio___letter_on_attach(fio_s *io) {
  fio_letter_parser_s *p = fio_letter_parser_new();
  if (!p) {
    fio_close(io);
    return;
  }
  fio_udata_set(io, p);
}

FIO_SFUNC void fio___letter_on_data_ipc_master(fio_s *io) {
  fio___letter_read(io, fio___letter_on_recieved_root);
}
FIO_SFUNC void fio___letter_on_data_ipc_child(fio_s *io) {
  fio___letter_read(io, fio___letter_on_recieved_child);
}
FIO_SFUNC void fio___letter_on_close(void *p) {
  fio_letter_parser_free((fio_letter_parser_s *)p);
}
FIO_SFUNC void fio___letter_on_close_in_child(void *p) {
  fio___letter_on_close(p);
  if (!fio_srv_is_running())
    return;
  fio_srv_stop();
  FIO_LOG_FATAL("(%d) lost connection with manager process, shutting down!",
                (int)fio_thread_getpid());
}
FIO_SFUNC void fio___letter_on_timeout(fio_s *io) {
  static const char ping_buf[FIO___LETTER_MINIMAL_LEN] = {0};
  fio_write2(io, .buf = (char *)ping_buf, .len = FIO___LETTER_MINIMAL_LEN);
}

FIO_SFUNC void fio___letter_on_data_remote(fio_s *io) {
  fio___letter_read(io, fio___on_letter_remote);
}

/* TODO: app-key + ed25519 per-connection key
 * exchange + ChaCha/Poly */
static fio_protocol_s FIO_LETTER_PROTOCOL_REMOTE = {
    .on_attach = fio___letter_on_attach,
    .on_data = fio___letter_on_data_remote,
    .on_close = fio___letter_on_close,
    .on_timeout = fio___letter_on_timeout,
};
static fio_protocol_s FIO_LETTER_PROTOCOL_IPC_MASTER = {
    .on_attach = fio___letter_on_attach,
    .on_data = fio___letter_on_data_ipc_master,
    .on_close = fio___letter_on_close,
    .on_timeout = fio___letter_on_timeout,
};
static fio_protocol_s FIO_LETTER_PROTOCOL_IPC_CHILD = {
    .on_attach = fio___letter_on_attach,
    .on_data = fio___letter_on_data_ipc_child,
    .on_close = fio___letter_on_close_in_child,
    .on_timeout = fio___letter_on_timeout,
};

/* *****************************************************************************
Letter Listening to Local Connections (IPC)
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
    FIO_LOG_DEBUG2("(pub/sub) IPC socket skipped "
                   "- no workers are spawned.");
    return;
  }
  FIO_ASSERT(fio_srv_listen(.url = FIO_POSTOFFICE.ipc_url,
                            .protocol = &FIO_LETTER_PROTOCOL_IPC_MASTER,
                            .on_root = 1,
                            .hide_from_log = FIO___PUBSUB_HIDE_FROM_LOG),
             "(pub/sub) "
             "couldn't open a socket for IPC\n\t\t%s",
             FIO_POSTOFFICE.ipc_url);
}
#undef FIO___PUBSUB_HIDE_FROM_LOG
/* *****************************************************************************
Letter Listening to Remote Connections - TODO!
***************************************************************************** */

/** Listen to remote letter exchange clients
 * (cluster letter exchange). */
FIO_IFUNC int fio_letter_remote_listen(const char *url, uint64_t app_key[2]) {
  (void)url; /* TODO!  */
  (void)app_key;
  return 0;
}
/** Connect to remote letter exchange server
 * (cluster letter exchange). */
FIO_IFUNC int fio_letter_remote_connect(const char *url, uint64_t app_key[2]) {
  (void)url; /* TODO! */
  (void)app_key;
  return 0;
}

/* *****************************************************************************



                        Channel Delivery API & Callbacks



***************************************************************************** */

FIO_IFUNC fio_channel_s *fio_channel_new_named(fio_buf_info_s name,
                                               int16_t filter,
                                               uint8_t is_pattern) {
  fio_channel_s *ch = NULL;
  if (!name.buf)
    name.len = 0;
  if (name.len > 0xFFFFUL)
    return ch;
  ch = fio_channel_new(name.len + 1);
  FIO_ASSERT_ALLOC(ch);
  *ch = (fio_channel_s){
      .subscriptions = FIO_LIST_INIT(ch->subscriptions),
      .name_len = (uint32_t)name.len,
      .filter = filter,
      .is_pattern = !!is_pattern, /* MUST be 1 or 0 */
  };
  if (name.len) {
    FIO_MEMCPY(ch->name, name.buf, name.len);
  }
  ch->name[name.len] = 0;
  return ch;
}

/** To be used in the fio_letter_on_composed
 * callback to distribute letters. */
FIO_IFUNC void fio___channel_deliver(fio_letter_s *l) {
  fio_letter_initialize_metadata(l); /* lazy metadata initialization */
  const fio_str_info_s ch_name = fio_letter_channel(l);
  const int16_t filter = fio_letter_filter(l);
  fio_channel_s cpy = {
      .name_len = (uint32_t)ch_name.len,
      .filter = filter,
      .subscriptions.next = (FIO_LIST_NODE *)ch_name.buf,
  };
  const uint64_t hash = fio_channel___hash(ch_name.buf, ch_name.len, filter);

#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_LOCK(FIO_POSTOFFICE.lock);
#endif
  fio_channel_s *ch = fio_channel_map_get(&FIO_POSTOFFICE.channels, hash, &cpy);
  /* TODO: use NULL channel as catch all? */
  // if (!ch) {
  //   cpy.name_len = 0;
  //   ch =
  //   fio_channel_map_get(&FIO_POSTOFFICE.channels,
  //                            fio_channel___hash(NULL,
  //                            0, filter),
  //                            &cpy);
  // }
  if (ch)
    fio_queue_push(fio_srv_queue(),
                   fio___channel_deliver_task,
                   fio_channel_dup(ch),
                   fio_letter_dup(l));
  FIO_MAP_EACH(fio_channel_map, &FIO_POSTOFFICE.patterns, i) {
    if (i.key && i.key->filter == filter &&
        FIO_PUBSUB_PATTERN_MATCH(FIO_STR_INFO2(i.key->name, i.key->name_len),
                                 ch_name))
      fio_queue_push(fio_srv_queue(),
                     fio___channel_deliver_task,
                     fio_channel_dup(i.key),
                     fio_letter_dup(l));
  }
#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_UNLOCK(FIO_POSTOFFICE.lock);
#endif
}

/* *****************************************************************************



                          Subscription / Management Tasks



***************************************************************************** */

/* calls the on_unsubscribe callback. */
FIO_SFUNC void fio___subscription_on_destroy__task(void *fnp, void *udata) {
  union {
    void *p;
    void (*fn)(void *udata);
  } u = {.p = fnp};
  u.fn(udata);
}

FIO_SFUNC void fio_subscription_on_destroy(fio_subscription_s *s) {
  if (s->on_unsubscribe) {
    union {
      void *p;
      void (*fn)(void *udata);
    } u = {.fn = s->on_unsubscribe};
    fio_queue_push(fio_srv_queue(),
                   fio___subscription_on_destroy__task,
                   u.p,
                   s->udata);
  }
}

/**
 * Reference counting: `fio_channel_dup(letter)`
 * / `fio_channel_free(letter)`
 */

FIO_SFUNC void fio___subscribe_task(void *ch_, void *sub_) {
  fio_channel_s *ch = (fio_channel_s *)ch_;
  fio_subscription_s *sub = (fio_subscription_s *)sub_;

#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_LOCK(FIO_POSTOFFICE.lock);
#endif

  fio_channel_map_s *map = &FIO_POSTOFFICE.channels + ch->is_pattern;
  const uint64_t hash = fio_channel___hash(ch->name, ch->name_len, ch->filter);
  ch = fio_channel_map_set_if_missing(map, hash, ch);
  if (!ch)
    goto unknown_error;
  sub->channel = fio_channel_dup(ch);
  FIO_LIST_PUSH(&ch->subscriptions, &sub->node);

#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_UNLOCK(FIO_POSTOFFICE.lock);
#endif
  return;

unknown_error:
  FIO_LOG_FATAL("%d (pubsub) channel couldn't "
                "be added to map!",
                fio___srvdata.pid);
#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_UNLOCK(FIO_POSTOFFICE.lock);
#endif
}

/** Unsubscribes a node and destroys the channel
 * if no more subscribers. */
FIO_IFUNC void fio___unsubscribe_task(void *ch_, void *sub_) {
  fio_channel_s *ch = (fio_channel_s *)ch_;
  fio_subscription_s *sub = (fio_subscription_s *)sub_;
  fio_channel_map_s *map;
  if (!ch)
    goto no_channel;

#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_LOCK(FIO_POSTOFFICE.lock);
#endif

  FIO_LIST_REMOVE(&sub->node);
  if (FIO_LIST_IS_EMPTY(&ch->subscriptions)) {
    map = &FIO_POSTOFFICE.channels + ch->is_pattern;
    fio_channel_map_remove(
        map,
        fio_channel___hash(ch->name, ch->name_len, ch->filter),
        ch,
        NULL);
    if (!fio_channel_map_count(map))
      fio_channel_map_destroy(map);
  }

#if FIO_POSTOFFICE_THREAD_LOCK
  FIO___LOCK_UNLOCK(FIO_POSTOFFICE.lock);
#endif

  fio_channel_free(ch);
no_channel:
  fio_subscription_free(sub);
  return;
}

/* *****************************************************************************



                            Subscription Type API



***************************************************************************** */

/** Defers the on_unsubscribe callback. */
FIO_IFUNC void fio___subscription_unsubscribe(fio_subscription_s *s) {
  if (!s)
    return;
  s->on_message = fio_subscription___mock_cb;
  fio_queue_push(fio_srv_queue(),
                 fio___unsubscribe_task,
                 (void *)(s->channel),
                 (void *)s);
}

FIO_IFUNC void fio___subscription_on_message_task(void *s_, void *l_) {
  fio_subscription_s *s = (fio_subscription_s *)s_;
  fio_letter_s *l = (fio_letter_s *)l_;
  struct {
    fio_msg_s msg;
    fio_letter_s *l;
    uintptr_t flag;
  } m = {
      .msg =
          {
              .io = s->io,
              .channel = fio_letter_channel(l),
              .message = fio_letter_message(l),
              .udata = s->udata,
              .filter = fio_letter_filter(l),
              .is_json = !!(fio_letter_flags(l) & FIO___PUBSUB_JSON),
          },
      .l = l,
  };
  s->on_message(&m.msg);
  s->udata = m.msg.udata;
  if (m.flag)
    goto reschedule;
  fio_subscription_free(s);
  fio_letter_free(l);
  return;
reschedule:
  fio_queue_push(fio_srv_queue(), fio___subscription_on_message_task, s_, l_);
}

/* returns the letter object associated with the
 * message. */
FIO_IFUNC fio_letter_s *fio_msg2letter(fio_msg_s *msg) {
  return *(fio_letter_s **)(msg + 1);
}

/** Defers the current callback, so it will be
 * called again for the message. */
SFUNC void fio_message_defer(fio_msg_s *msg) {
  ((uintptr_t *)(msg + 1))[1] = 1;
}

/* delivers a letter to all of a channel's
 * subscribers */
FIO_SFUNC void fio___channel_deliver_task(void *ch_, void *l_) {
  fio_channel_s *ch = (fio_channel_s *)ch_;
  fio_letter_s *l = (fio_letter_s *)l_;
  if (l->from) {
    FIO_LIST_EACH(fio_subscription_s, node, &ch->subscriptions, s) {
      if (l->from != s->io)
        fio_queue_push(
            fio_srv_queue(),
            (void (*)(void *, void *))fio___subscription_on_message_task,
            fio_subscription_dup(s),
            fio_letter_dup(l));
    }
  } else {
    FIO_LIST_EACH(fio_subscription_s, node, &ch->subscriptions, s) {
      fio_queue_push(
          fio_srv_queue(),
          (void (*)(void *, void *))fio___subscription_on_message_task,
          fio_subscription_dup(s),
          fio_letter_dup(l));
    }
  }
  fio_letter_free(l);
  fio_channel_free(ch);
}

/* *****************************************************************************



                      Public Subscribe / Unsubscribe API



***************************************************************************** */

/** Subscribes to a named channel in the
 * numerical filter's namespace. */
void fio_subscribe___(void); /* sublimetext marker */
SFUNC void fio_subscribe FIO_NOOP(fio_subscribe_args_s args) {
  fio_subscription_s *s = fio_subscription_new();
  if (!s)
    goto sub_error;
  *s = (fio_subscription_s){
      .io = args.io,
      .on_message = (args.on_message ? args.on_message
                                     : (args.io ? fio_subscription___send_cb
                                                : fio_subscription___mock_cb)),
      .on_unsubscribe = args.on_unsubscribe,
      .udata = args.udata,
  };
  s->channel =
      fio_channel_new_named(args.channel, args.filter, args.is_pattern);
  if (!s->channel)
    goto channel_error;
  fio_queue_push(fio_srv_queue(),
                 fio___subscribe_task,
                 (void *)s->channel,
                 (void *)s);

  if (args.master_only && !args.io)
    goto is_master_only;
  if (!args.subscription_handle_ptr) {
    fio_env_set(args.io,
                .type = (intptr_t)(0LL - (((2ULL | (!!args.is_pattern)) << 16) |
                                          (uint16_t)args.filter)),
                .name = args.channel,
                .on_close = (void (*)(void *))fio___subscription_unsubscribe,
                .udata = s);
    return;
  }
  *args.subscription_handle_ptr = (uintptr_t)s;
  return;

is_master_only:
  if (fio_srv_is_master()) {
    fio___postoffice_msmap_set(
        &FIO_POSTOFFICE.master_subscriptions,
        fio_channel___hash(args.channel.buf, args.channel.len, args.filter),
        FIO_STR_INFO3(args.channel.buf, args.channel.len, (size_t)-1),
        s,
        NULL);
  } else {
    fio_channel_free(s->channel);
    fio_subscription_free(s);
    FIO_LOG_WARNING(
        "(%d) master-only subscription attempt on a non-master process: %.*s",
        fio___srvdata.pid,
        (int)args.channel.len,
        args.channel.buf);
  }
  return;

channel_error:
  FIO_LOG_ERROR("(%d) (pubsub) channel cannot be "
                "created?\n\t%zu bytes long\n\t%.*s",
                fio___srvdata.pid,
                args.channel.len,
                (int)args.channel.len,
                args.channel.buf);
  fio_subscription_free(s);
  return;

sub_error:
  FIO_LOG_ERROR("failed to allocate a new subscription");
  if (args.on_unsubscribe) {
    union {
      void *p;
      void (*fn)(void *udata);
    } u = {.fn = args.on_unsubscribe};
    fio_queue_push(fio_srv_queue(),
                   fio___subscription_on_destroy__task,
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
  fio___subscription_unsubscribe(
      *(fio_subscription_s **)args.subscription_handle_ptr);
  return 0;

is_master_only:
  return fio___postoffice_msmap_remove(
      &FIO_POSTOFFICE.master_subscriptions,
      fio_channel___hash(args.channel.buf, args.channel.len, args.filter),
      FIO_STR_INFO3(args.channel.buf, args.channel.len, (size_t)-1),
      NULL);
}

/* *****************************************************************************
Pub/Sub - Publish
***************************************************************************** */

FIO_SFUNC void fio___publish_letter_task(void *l_, void *ignr_) {
  (void)ignr_;
  fio_letter_s *l = (fio_letter_s *)l_;
  if ((fio_letter_flags(l) & FIO_POSTOFFICE.publish_filter))
    fio___channel_deliver(l);
  if ((fio_letter_flags(l) & FIO_POSTOFFICE.local_send_filter)) {
    fio_protocol_each(FIO_POSTOFFICE.siblings_protocol,
                      (void (*)(fio_s *, void *))fio_letter_write,
                      l_);
  }
  if ((fio_letter_flags(l) & FIO_POSTOFFICE.remote_send_filter)) {
    /* deliver to remote connections... all of
     * them? yes, we are the source. */
    fio_protocol_each(&FIO_LETTER_PROTOCOL_REMOTE,
                      (void (*)(fio_s *, void *))fio_letter_write,
                      l_);
  }
  fio_letter_free(l);
}

/** Publishes a message to the relevant
 * subscribers (if any). */
void fio_publish___(void); /* SublimeText marker*/
void fio_publish FIO_NOOP(fio_publish_args_s args) {
  fio_letter_s *l;
  if (!args.engine)
    args.engine = FIO_PUBSUB_DEFAULT;
  if ((uintptr_t)(args.engine) > 0XFF) {
    if (!args.filter)
      goto external_engine;
    args.engine = FIO_PUBSUB_LOCAL;
  }
  if (!args.engine)
    args.engine = FIO_PUBSUB_DEFAULT = FIO_PUBSUB_CLUSTER;
  l = fio_letter_new_compose(
      args.channel,
      args.message,
      (int16_t)args.filter,
      (uint8_t)((uintptr_t)args.engine |
                ((0x100U - args.is_json) & FIO___PUBSUB_JSON)));
  l->from = args.from;
  fio_defer(fio___publish_letter_task, l, NULL);
  return;
external_engine:
  args.engine->publish(args.engine,
                       args.channel,
                       args.message,
                       args.filter,
                       args.is_json);
}

/* *****************************************************************************



                      Post Office State Management



***************************************************************************** */

/** Returns the current IPC socket address (shouldn't be changed). */
SFUNC int fio_pubsub_ipc_url_set(char *str, size_t len) {
  if (fio_srv_is_running() || len >= FIO___IPC_LEN)
    return -1;
  fio_str_info_s url = FIO_STR_INFO3(FIO_POSTOFFICE.ipc_url, 0, FIO___IPC_LEN);
  fio_string_write2(&url, NULL, FIO_STRING_WRITE_STR2(str, len));
  return 0;
}
/** Returns the current IPC socket address (shouldn't be changed). */
SFUNC const char *fio_pubsub_ipc_url(void) { return FIO_POSTOFFICE.ipc_url; }

/** Sets a (possibly shared) secret for securing pub/sub communication. */
SFUNC void fio_pubsub_secret_set(char *str, size_t len) {
  uint64_t fallback_secret = 0;
  if (!str || !len) {
    if ((str = getenv("SECRET"))) {
      len = strlen(str);
    } else {
      fallback_secret = fio_rand64();
      str = (char *)&fallback_secret;
      len = sizeof(fallback_secret);
    }
  }
  fio_u256 scrt = fio_sha256(str, len);
  FIO_ASSERT(sizeof(scrt) == sizeof(FIO_POSTOFFICE.secret),
             "(pubsub) secret length error!");
  for (size_t i = 0;
       i < sizeof(FIO_POSTOFFICE.secret) / sizeof(FIO_POSTOFFICE.secret[0]);
       ++i)
    FIO_POSTOFFICE.secret[i] = scrt.uz[i];
}

FIO_CONSTRUCTOR(fio_postoffice_init) {
  FIO_POSTOFFICE.engines = FIO_LIST_INIT(FIO_POSTOFFICE.engines);
  FIO_POSTOFFICE.siblings_protocol = &FIO_LETTER_PROTOCOL_IPC_MASTER;
  fio_str_info_s url = FIO_STR_INFO3(FIO_POSTOFFICE.ipc_url, 0, FIO___IPC_LEN);
  fio_pubsub_secret_set(NULL, 0); /* allocate a random secret */
  fio_string_write2(&url,
                    NULL,
                    FIO_STRING_WRITE_STR1((char *)"priv://facil_io_tmp_"),
                    FIO_STRING_WRITE_HEX(fio_rand64()),
                    FIO_STRING_WRITE_STR1((char *)".sock"));
  fio_state_callback_add(FIO_CALL_PRE_START, fio___pubsub_ipc_listen, NULL);
  fio_state_callback_add(FIO_CALL_IN_CHILD,
                         fio___postoffice_on_enter_child,
                         NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT,
                         (void (*)(void *))fio___letter_map_destroy,
                         (void *)(&fio___letter_validation.map));
  fio_state_callback_add(FIO_CALL_AT_EXIT,
                         (void (*)(void *))fio___postoffice_msmap_destroy,
                         (void *)(&FIO_POSTOFFICE.master_subscriptions));
  fio_state_callback_add(FIO_CALL_IN_CHILD,
                         (void (*)(void *))fio___postoffice_msmap_destroy,
                         (void *)(&FIO_POSTOFFICE.master_subscriptions));
}

/** Callback called by the letter protocol
 * entering a child processes. */
FIO_SFUNC void fio___postoffice_on_enter_child(void *ignr_) {
  (void)ignr_;
  FIO_POSTOFFICE.publish_filter = FIO___PUBSUB_PROCESS;
  FIO_POSTOFFICE.local_send_filter =
      (FIO___PUBSUB_SIBLINGS | FIO___PUBSUB_ROOT);
  FIO_POSTOFFICE.remote_send_filter = 0;
  FIO_POSTOFFICE.siblings_protocol = &FIO_LETTER_PROTOCOL_IPC_CHILD;
  if (fio_srv_connect(FIO_POSTOFFICE.ipc_url,
                      &FIO_LETTER_PROTOCOL_IPC_CHILD,
                      NULL,
                      NULL)) {
    FIO_LOG_FATAL("(%d) couldn't connect to pub/sub socket @ %s",
                  fio___srvdata.pid,
                  FIO_POSTOFFICE.ipc_url);
    fio_thread_kill(fio___srvdata.root_pid, SIGINT);
    FIO_ASSERT(0, "fatal error encountered");
  }
}

/* *****************************************************************************



                    Letter Engine Support Implementation



***************************************************************************** */

static void fio_pubsub_mock_detached(const fio_pubsub_engine_s *eng) {
  (void)eng;
}
static void fio_pubsub_mock_sub_unsub(const fio_pubsub_engine_s *eng,
                                      fio_buf_info_s channel,
                                      int16_t filter) {
  (void)eng;
  (void)channel;
  (void)filter;
}
static void fio_pubsub_mock_publish(const fio_pubsub_engine_s *eng,
                                    fio_buf_info_s channel,
                                    fio_buf_info_s msg,
                                    int16_t filter,
                                    uint8_t is_json) {
  (void)eng;
  (void)channel;
  (void)msg;
  (void)filter;
  (void)is_json;
}

/** Callback for when a channel is created. */
FIO_IFUNC void fio_channel_on_create(fio_channel_s *ch) {
  fio_buf_info_s name = FIO_BUF_INFO2(ch->name, ch->name_len);
  FIO_LOG_DEBUG2("%d (pubsub) channel created (filter %d, "
                 "length %zu bytes): %s",
                 fio___srvdata.pid,
                 (int)ch->filter,
                 (size_t)ch->name_len,
                 name.buf);
  FIO_LIST_EACH(fio_pubsub_engine_s,
                internal_use_.node,
                &FIO_POSTOFFICE.engines,
                e) {
    (&e->subscribe + ch->is_pattern)[0](e, name, ch->filter);
  }
}
/** Callback for when a channel is destroy. */
FIO_IFUNC void fio_channel_on_destroy(fio_channel_s *ch) {
  fio_buf_info_s name = FIO_BUF_INFO2(ch->name, ch->name_len);
  FIO_LIST_EACH(fio_pubsub_engine_s,
                internal_use_.node,
                &FIO_POSTOFFICE.engines,
                e) {
    (&e->unsubscribe + ch->is_pattern)[0](e, name, ch->filter);
  }
  FIO_LOG_DEBUG2("%d (pubsub) channel destroyed (filter "
                 "%d, length %zu bytes): %s",
                 fio___srvdata.pid,
                 (int)ch->filter,
                 (size_t)ch->name_len,
                 name.buf);
}

static void fio_pubsub_attach___task(void *engine_, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *engine = (fio_pubsub_engine_s *)engine_;
  if (!engine->detached)
    engine->detached = fio_pubsub_mock_detached;
  if (!engine->subscribe)
    engine->subscribe = fio_pubsub_mock_sub_unsub;
  if (!engine->unsubscribe)
    engine->unsubscribe = fio_pubsub_mock_sub_unsub;
  if (!engine->psubscribe)
    engine->psubscribe = fio_pubsub_mock_sub_unsub;
  if (!engine->punsubscribe)
    engine->punsubscribe = fio_pubsub_mock_sub_unsub;
  if (!engine->publish)
    engine->publish = fio_pubsub_mock_publish;
  if (!engine->internal_use_.node.next) {
    FIO_LIST_PUSH(&FIO_POSTOFFICE.engines, &engine->internal_use_.node);
  }
  FIO_MAP_EACH(fio_channel_map, &FIO_POSTOFFICE.channels, i) {
    engine->subscribe(engine,
                      FIO_BUF_INFO2(i.key->name, i.key->name_len),
                      i.key->filter);
  }
  FIO_MAP_EACH(fio_channel_map, &FIO_POSTOFFICE.patterns, i) {
    engine->psubscribe(engine,
                       FIO_BUF_INFO2(i.key->name, i.key->name_len),
                       i.key->filter);
  }
}

/** Attaches an engine, so it's callback can be called by facil.io. */
SFUNC void fio_pubsub_attach(fio_pubsub_engine_s *engine) {
  if (!engine)
    return;
  fio_defer(fio_pubsub_attach___task, engine, NULL);
}

FIO_SFUNC void fio_pubsub_detach___task(void *engine, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *e = (fio_pubsub_engine_s *)engine;
  if (e->internal_use_.node.next) {
    FIO_LIST_REMOVE(&e->internal_use_.node);
    e->internal_use_.node.next = e->internal_use_.node.prev = NULL;
  }
  e->detached(e);
}

/** Schedules an engine for Detachment, so it could be safely destroyed. */
SFUNC void fio_pubsub_detach(fio_pubsub_engine_s *engine) {
  fio_queue_push(fio_srv_queue(), fio_pubsub_detach___task, engine, NULL);
}

/* *****************************************************************************
Pub/Sub Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_PUBSUB
#endif /* FIO_PUBSUB */
