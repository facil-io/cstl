/* ************************************************************************* */
#ifndef H___FIO_CSTL_INCLUDE_ONCE___H /* Development inclusion - ignore line*/
#define FIO_PUBSUB                    /* Development inclusion - ignore line */
#include "400 server.h"               /* Development inclusion - ignore line */
#endif                                /* Development inclusion - ignore line */
/* *****************************************************************************




                Pub/Sub Services for IPC / Server applications




Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_PUBSUB) && !defined(H___FIO_PUBSUB___H) &&                     \
    !defined(FIO_STL_KEEP__)
#define H___FIO_PUBSUB___H
/* *****************************************************************************
Pub/Sub - defaults and builtin pub/sub engines
***************************************************************************** */

/** A pub/sub engine data structure. See details later on. */
typedef struct fio_pubsub_engine_s fio_pubsub_engine_s;

/** Used to publish the message exclusively to the root / master process. */
extern const fio_pubsub_engine_s *const FIO_PUBSUB_ROOT;
/** Used to publish the message only within the current process. */
extern const fio_pubsub_engine_s *const FIO_PUBSUB_PROCESS;
/** Used to publish the message except within the current process. */
extern const fio_pubsub_engine_s *const FIO_PUBSUB_SIBLINGS;
/** Used to publish the message for this process, its siblings and root. */
extern const fio_pubsub_engine_s *const FIO_PUBSUB_LOCAL;

/** The default engine (settable). Initial default is FIO_PUBSUB_LOCAL. */
extern const fio_pubsub_engine_s *FIO_PUBSUB_DEFAULT;

/**
 * The pattern matching callback used for pattern matching.
 *
 * Returns 1 on a match or 0 if the string does not match the pattern.
 *
 * By default, the value is set to `fio_glob_match` (see facil.io's C STL).
 */
extern uint8_t (*FIO_PUBSUB_PATTERN_MATCH)(fio_str_info_s pattern,
                                           fio_str_info_s channel);

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
  /** A unique message type. Negative values are reserved, 0 == global. */
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
   * A named `channel` to which the subscriber subscribes.
   *
   * Subscriptions require a match by both channel name and filter.
   */
  fio_str_info_s channel;
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
   * An additional numerical `filter` subscribers need to match.
   *
   * Negative values are reserved for facil.io framework extensions.
   *
   * Filer channels are bound to the processes and workers, they are NOT
   * forwarded to engines and can be used for inter process communication (IPC).
   */
  int32_t filter;
  /** If set, pattern matching will be used (name is a pattern). */
  uint8_t is_pattern;
} subscribe_args_s;

/**
 * Subscribes to a channel / filter pair.
 *
 * The on_unsubscribe callback will be called on failure.
 */
void fio_subscribe(subscribe_args_s args);

/**
 * Subscribes to a channel / filter pair.
 *
 * See `subscribe_args_s` for details.
 */
#define fio_subscribe(...) fio_subscribe((subscribe_args_s){__VA_ARGS__})

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
int fio_unsubscribe(subscribe_args_s args);

/**
 * Cancels an existing subscriptions.
 *
 * Accepts the same arguments as `fio_subscribe`, except the `udata` and
 * callback details are ignored (no need to provide `udata` or callback
 * details).
 *
 * Returns -1 if the subscription could not be found. Otherwise returns 0.
 */
#define fio_unsubscribe(...) fio_unsubscribe((subscribe_args_s){__VA_ARGS__})

/* *****************************************************************************
Pub/Sub - Publish
***************************************************************************** */

/** Publishing and on_message callback arguments. */
typedef struct fio_publish_args_s {
  /** The pub/sub engine that should be used to forward this message. */
  fio_pubsub_engine_s const *engine;
  /** If `from` is specified, it will be skipped (won't receive message). */
  fio_s *from;
  /** The target named channel. Only published when filter == 0. */
  fio_str_info_s channel;
  /** The message body / content. */
  fio_str_info_s message;
  /** A numeral / internal channel. Negative values are reserved. */
  int32_t filter;
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
 * If publishing to a channel with a non-zero `filter`, the pub/sub will default
 * to `FIO_PUBSUB_LOCAL` and external engines will be ignored.
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
void fio_publish(fio_publish_args_s args);
/**
 * Publishes a message to the relevant subscribers (if any).
 *
 * By default the message is sent using the `FIO_PUBSUB_DEFAULT` engine (set by
 * default to `FIO_PUBSUB_LOCAL` which publishes to all processes, including the
 * calling process).
 *
 * If publishing to a channel with a non-zero `filter`, the pub/sub will default
 * to `FIO_PUBSUB_LOCAL` and external engines will be ignored.
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
void fio_message_defer(fio_msg_s *msg);

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
                                     uint8_t is_json);

/**
 * It's possible to attach metadata to facil.io named messages (filter == 0)
 * before they are published.
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
int fio_message_metadata_add(fio_msg_metadata_fn metadata_func,
                             void (*cleanup)(void *));

/**
 * Removed the metadata callback.
 *
 * Removal might be delayed if live metatdata exists.
 */
void fio_message_metadata_remove(fio_msg_metadata_fn metadata_func);

/**
 * Finds the message's metadata, returning the data or NULL.
 *
 * Note: channels with non-zero filters don't have metadata attached.
 */
void *fio_message_metadata(fio_msg_metadata_fn metadata_func);

/* *****************************************************************************
 * Cluster / Pub/Sub Middleware and Extensions ("Engines")
 **************************************************************************** */

/**
 * The number of different pub/sub "engines" that can be attached.
 *
 * This number may effect performance, especially when adding / removing
 * engines.
 */
#ifndef FIO_PUBSUB_ENGINE_LIMIT
#define FIO_PUBSUB_ENGINE_LIMIT 16
#endif

/**
 * facil.io can be linked with external Pub/Sub services using "engines".
 *
 * Only messages and unfiltered subscriptions (where filter == 0) will be
 * forwarded to these "engines".
 *
 * Engines MUST provide the listed function pointers and should be attached
 * using the `fio_pubsub_attach` function.
 *
 * Engines should disconnect / detach, before being destroyed, by using the
 * `fio_pubsub_detach` function.
 *
 * When an engine received a message to publish, it should call the
 * `pubsub_publish` function with the engine to which the message is forwarded.
 * i.e.:
 *
 *       pubsub_publish(
 *           .engine = FIO_PUBSUB_LOCAL,
 *           .channel = channel_name,
 *           .message = msg_body );
 *
 * Since only the master process guarantees to be subscribed to all the channels
 * in the cluster, only the master process calls the `(un)(p)subscribe`
 * callbacks.
 *
 * IMPORTANT: The `(un)(p)subscribe` callbacks might be called by the main
 * (master) thread, so they should never block except by scheduling an external
 * task using `fio_defer`.
 */
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

/**
 * Attaches an engine, so it's callback can be called by facil.io.
 *
 * The `subscribe` callback will be called for every existing channel.
 *
 * NOTE: the root (master) process will call `subscribe` for any channel in any
 * process, while all the other processes will call `subscribe` only for their
 * own channels. This allows engines to use the root (master) process as an
 * exclusive subscription process.
 */
void fio_pubsub_attach(fio_pubsub_engine_s *engine);

/** Detaches an engine, so it could be safely destroyed. */
void fio_pubsub_detach(fio_pubsub_engine_s *engine);

/**
 * Engines can ask facil.io to call the `(p)subscribe` callbacks for all active
 * channels.
 *
 * This allows engines that lost their connection to their Pub/Sub service to
 * resubscribe to all the currently active channels with the new connection.
 *
 * CAUTION: This is an evented task... try not to free the engine's memory while
 * re-subscriptions are under way.
 *
 * NOTE: the root (master) process will call `subscribe` for any channel in any
 * process, while all the other processes will call `subscribe` only for their
 * own channels. This allows engines to use the root (master) process as an
 * exclusive subscription process.
 */
void fio_pubsub_resubscribe_all(fio_pubsub_engine_s *eng);

/** Returns true (1) if the engine is attached to the system. */
int fio_pubsub_is_attached(fio_pubsub_engine_s *engine);

/* *****************************************************************************



Pub/Sub Implementation - static functions



***************************************************************************** */
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
Pub/Sub Implementation - externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
Minimal WebSocket Header Support
***************************************************************************** */
FIO_IFUNC size_t fio___websocket_header_len(uint64_t len) {
  if (len < 126)
    return len + 2;
  if (len < (1UL << 16))
    return len + 4;
  return len + 10;
}

FIO_IFUNC size_t fio___websocket_header_cpy(void *dest,
                                            uint64_t len,
                                            unsigned char is_binary,
                                            unsigned char rsv) {
  ((uint8_t *)dest)[0] = (1U << (!!is_binary)) | ((rsv & 7) << 4) | (1 << 7);
  if (len < 126) {
    /* head is 2 bytes */
    ((uint8_t *)dest)[1] = len;
    return 2;
  } else if (len < (1UL << 16)) {
    /* head is 4 bytes */
    ((uint8_t *)dest)[1] = 126;
    fio_u2buf16(((uint8_t *)dest + 2), len);
    return 4;
  }
  /* head is 10 bytes */
  ((uint8_t *)dest)[1] = 127;
  fio_u2buf64(((uint8_t *)dest + 2), len);
  return 10;
}

/* *****************************************************************************
Letter / Message Object
***************************************************************************** */

/*
Letter network format in bytes:
| 1 Byte Reserved | 1 Scope / type flags | 2 Bytes numerical Filter
| 4 bytes little endian channel length | 4 bytes little endian message length |
| 16 bytes UUID (8 + 8 byte values) |
| X bytes (channel length + 1 NUL terminator) |
| WebSocket Header (optimize cases where Message is sent using WebSockets)
| Y bytes (message length + 1 NUL terminator) |
*/

#define LETTER_HEADER_LENGTH 28 /* without NUL terminators */

enum {
  FIO_PUBSUB_PROCESS_BIT = 1,
  FIO_PUBSUB_ROOT_BIT = 2,
  FIO_PUBSUB_SIBLINGS_BIT = 4,
  FIO_PUBSUB_CLUSTER_BIT = 8,
  FIO_PUBSUB_PING_BIT = 64,
  FIO_PUBSUB_JSON_BIT = 128,
} letter_info_bits_e;

typedef struct {
  fio_s *from;
  void *metadata[FIO_PUBSUB_METADATA_LIMIT];
  char buf[];
} letter_s;

#define FIO_REF_NAME      letter
#define FIO_REF_FLEX_TYPE char
#define FIO_STL_KEEP__    1
#include __FILE__
#undef FIO_STL_KEEP__

/* allocates a new letter. */
FIO_IFUNC letter_s *letter_new(fio_s *from,
                               int16_t filter,
                               uint32_t channel_len,
                               uint32_t message_len) {
  size_t len = (LETTER_HEADER_LENGTH + 2) + message_len + channel_len;
  letter_s *l = letter_new2(len);
  FIO_ASSERT_ALLOC(l);
  l->from = from;
  filter = fio_ltole16(filter);
  channel_len = fio_ltole32(channel_len);
  message_len = fio_ltole32(message_len);
  FIO_MEMCPY16(l->buf + 2, &filter);
  FIO_MEMCPY32(l->buf + 4, &channel_len);
  FIO_MEMCPY32(l->buf + 8, &message_len);
  return l;
}

/* allocates a new letter, and writes data to header */
FIO_IFUNC letter_s *letter_author(fio_s *from,
                                  uint64_t server_id,
                                  int32_t filter,
                                  char *channel,
                                  uint32_t channel_len,
                                  char *message,
                                  uint32_t message_len,
                                  uint8_t flags) {
  static uint64_t counter = 0;
  letter_s *l = NULL;
  if ((channel_len >> 24) || (message_len >> 27))
    goto len_error;
  if (!counter)
    counter = (fio_rand64() << 24);
  l = letter_new(from, filter, channel_len, message_len);
  FIO_ASSERT_ALLOC(l);
  l->buf[0] = flags;
  FIO_MEMCPY8(l->buf + 12, &server_id);
  FIO_MEMCPY8(l->buf + 20, &counter);
  ++counter;
  if (channel_len && channel) {
    memcpy(l->buf + LETTER_HEADER_LENGTH, channel, channel_len);
  }
  l->buf[LETTER_HEADER_LENGTH + channel_len] = 0;
  if (message_len && message) {
    memcpy(l->buf + LETTER_HEADER_LENGTH + 1 + channel_len,
           message,
           message_len);
  }
  l->buf[(LETTER_HEADER_LENGTH + 1) + channel_len + message_len] = 0;
  return l;

len_error:
  FIO_LOG_ERROR("(pubsub) payload too big (channel length of %u bytes, message "
                "length of %u bytes)",
                (unsigned int)channel_len,
                (unsigned int)message_len);
  return NULL;
}

/* frees a letter's reference. */
#define letter_free letter_free2

/* returns 1 if a letter is bound to a filter, otherwise 0. */
FIO_IFUNC int16_t letter_is_filter(letter_s *l) {
  return !!(*(int16_t *)(l->buf + 2));
}

/* returns a letter's ID (may be 0 for internal letters) */
FIO_IFUNC uint64_t letter_id(letter_s *l) {
  return fio_buf2u64_little(l->buf + 12);
}

/* returns a letter's channel (if none, returns the filter's address) */
FIO_IFUNC char *letter_channel(letter_s *l) {
  return l->buf + LETTER_HEADER_LENGTH;
}

/* returns a letter's message length (if any) */
FIO_IFUNC size_t letter_message_len(letter_s *l) { return l->message_len; }

/* returns a letter's channel length (if any) */
FIO_IFUNC size_t letter_channel_len(letter_s *l) { return l->channel_len; }

/* returns a letter's filter (if any) */
FIO_IFUNC int32_t letter_filter(letter_s *l) { return l->filter; }

/* returns a letter's message */
FIO_IFUNC char *letter_message(letter_s *l) {
  return l->buf + LETTER_HEADER_LENGTH + 1 + l->channel_len;
}

/* returns a letter's length */
FIO_IFUNC size_t letter_len(letter_s *l) {
  return LETTER_HEADER_LENGTH + 2 + l->channel_len + l->message_len;
}

/* write a letter to an IO object */
FIO_IFUNC void letter_write(fio_s *io, letter_s *l) {
  if (io == l->from)
    return;
  fio_write2(io,
             .buf = (char *)letter_dup2(l),
             .offset = (uintptr_t)(((letter_s *)0)->buf),
             .len = letter_len(l),
             .dealloc = (void (*)(void *))letter_free2);
}

/* *****************************************************************************
Letter Reading, Parsing and Sending
***************************************************************************** */

/* a letter parser object */
typedef struct {
  letter_s *letter;
  size_t pos;
  char buf[LETTER_HEADER_LENGTH + 2]; /* minimal message length */
} letter_parser_s;

/* a new letter parser */
FIO_IFUNC letter_parser_s *letter_parser_new(void) {
  letter_parser_s *p = fio_malloc(sizeof(*p));
  FIO_ASSERT_ALLOC(p);
  p->letter = NULL;
  p->pos = 0;
  return p;
}

/* free a letter parser */
FIO_IFUNC void letter_parser_free(letter_parser_s *parser) {
  letter_free(parser->letter);
  fio_free(parser);
}

/* pings are ignored (no pong required) */
FIO_SFUNC void letter_read_ping_callback(letter_s *letter) { (void)letter; }

/* forwards letters to callback, returns 0. Returns -1 on error. */
FIO_IFUNC int letter_read(fio_s *io,
                          letter_parser_s *parser,
                          void (*callback)(letter_s *)) {
  void (*callbacks[2])(letter_s *) = {callback, letter_read_ping_callback};
  for (;;) {
    ssize_t r;
    if (parser->letter) {
      letter_s *const letter = parser->letter;
      const size_t to_read = letter_len(parser->letter);
      while (parser->pos < to_read) {
        r = fio_read(io, letter->buf + parser->pos, to_read - parser->pos);
        if (r <= 0)
          return 0;
        parser->pos += r;
      }
      callbacks[((letter->buf[0] & FIO_PUBSUB_PING_BIT) / FIO_PUBSUB_PING_BIT)](
          letter);
      letter_free(letter);
      parser->letter = NULL;
      parser->pos = 0;
    }
    r = fio_read(io,
                 parser->buf + parser->pos,
                 (LETTER_HEADER_LENGTH + 2) - parser->pos);
    if (r <= 0)
      return 0;
    parser->pos += r;
    if (parser->pos < LETTER_HEADER_LENGTH)
      return 0;
    uint32_t channel_len = fio_buf2u32_little(parser->buf + 4);
    channel_len &= 0xFFFFFF;
    uint32_t message_len = fio_buf2u32_little(parser->buf + 8);
    message_len &= 0xFFFFFFF;
    int32_t filter = fio_buf2u32_little(parser->buf + 8);
    parser->letter = letter_new(io, filter, channel_len, message_len);
    if (!parser->letter) {
      return -1;
    }
    memcpy(parser->letter->buf, parser->buf, parser->pos);
  }
}

/* *****************************************************************************
Pub/Sub - defaults and builtin pub/sub engines
***************************************************************************** */

/** Used to publish the message exclusively to the root / master process. */
const fio_pubsub_engine_s *const FIO_PUBSUB_ROOT =
    (fio_pubsub_engine_s *)FIO_PUBSUB_ROOT_BIT;
/** Used to publish the message only within the current process. */
const fio_pubsub_engine_s *const FIO_PUBSUB_PROCESS =
    (fio_pubsub_engine_s *)FIO_PUBSUB_PROCESS_BIT;
/** Used to publish the message except within the current process. */
const fio_pubsub_engine_s *const FIO_PUBSUB_SIBLINGS =
    (fio_pubsub_engine_s *)FIO_PUBSUB_SIBLINGS_BIT;
/** Used to publish the message for this process, its siblings and root. */
const fio_pubsub_engine_s *const FIO_PUBSUB_LOCAL =
    (fio_pubsub_engine_s *)(FIO_PUBSUB_SIBLINGS_BIT | FIO_PUBSUB_PROCESS_BIT |
                            FIO_PUBSUB_ROOT_BIT);
/** Used to publish the message to any possible publishers. */
const fio_pubsub_engine_s *const FIO_PUBSUB_CLUSTER =
    (fio_pubsub_engine_s *)(FIO_PUBSUB_CLUSTER_BIT | FIO_PUBSUB_SIBLINGS_BIT |
                            FIO_PUBSUB_PROCESS_BIT | FIO_PUBSUB_ROOT_BIT);

/** The default engine (settable). Initial default is FIO_PUBSUB_LOCAL. */
const fio_pubsub_engine_s *FIO_PUBSUB_DEFAULT =
    (fio_pubsub_engine_s *)(FIO_PUBSUB_CLUSTER_BIT | FIO_PUBSUB_SIBLINGS_BIT |
                            FIO_PUBSUB_PROCESS_BIT | FIO_PUBSUB_ROOT_BIT);

/**
 * The pattern matching callback used for pattern matching.
 *
 * Returns 1 on a match or 0 if the string does not match the pattern.
 *
 * By default, the value is set to `fio_glob_match` (see facil.io's C STL).
 */
uint8_t (*FIO_PUBSUB_PATTERN_MATCH)(fio_str_info_s,
                                    fio_str_info_s) = fio_glob_match;

/* *****************************************************************************
Channel / Subscription Objects
***************************************************************************** */

/* channels can be either named (filters + exact strings) or pattern based. */
typedef enum {
  CHANNEL_TYPE_NAMED,
  CHANNEL_TYPE_PATTERN,
  CHANNEL_TYPE_NONE,
} channel_type_e;

/* The channel object */
typedef struct {
  FIO_LIST_HEAD subscriptions;
  channel_type_e type;
  int32_t filter;
  size_t name_len;
  char *name;
} channel_s;

/* The subscription object */
typedef struct {
  FIO_LIST_NODE node;
  fio_s *io;
  channel_s *channel;
  void (*on_message)(fio_msg_s *msg);
  void (*on_unsubscribe)(void *udata);
  void *udata;
  uint32_t ref;
  fio_lock_i lock;
  uint8_t disabled; /* TODO: do we need this one? */
} subscription_s;

/* *****************************************************************************
Subscription Object API
***************************************************************************** */

/* a mock callback for subscriptions */
FIO_SFUNC void subscription_mock_cb(fio_msg_s *msg) { (void)msg; }

/* creates a new subscription */
FIO_IFUNC subscription_s *subscription_new(fio_s *io,
                                           channel_s *channel,
                                           void (*on_message)(fio_msg_s *),
                                           void (*on_unsubscribe)(void *),
                                           void *udata) {
  void (*const cb_ary[2])(fio_msg_s *) = {on_message, subscription_mock_cb};
  void *(*const alloc_fn[2])(size_t) = {fio_malloc, malloc};
  subscription_s *s = (alloc_fn[!io])(sizeof(*s));
  FIO_ASSERT_ALLOC(s);
  // FIO_LOG_DDEBUG2("(%d) allocated subscription: %p", fio_pid(), s);
  *s = (subscription_s){
      .node = FIO_LIST_INIT(s->node),
      .io = io,
      .channel = channel,
      .on_message = (cb_ary[!on_message]),
      .on_unsubscribe = on_unsubscribe,
      .udata = udata,
      .ref = 1,
  };
  return s;
}

/* we count subscription reference counts to make sure the udata is valid */
FIO_IFUNC subscription_s *subscription_dup(subscription_s *s) {
  FIO_ASSERT(fio_atomic_add_fetch(&s->ref, 1),
             "subscription reference count overflow detected!");
  return s;
}

/* calls the on_unsubscribe callback. */
FIO_SFUNC void subscription_on_unsubscribe___task(void *fnp, void *udata) {
  union {
    void *p;
    void (*fn)(void *udata);
  } u = {.p = fnp};
  u.fn(udata);
}

/* free the udata (and subscription) only after all callbacks return */
FIO_IFUNC void subscription_free(subscription_s *s) {
  if (fio_atomic_sub_fetch(&s->ref, 1))
    return;
  union {
    void *p;
    void (*fn)(void *udata);
  } u = {.fn = s->on_unsubscribe};
  FIO_LIST_REMOVE(&s->node);
  if (u.p) {
    fio_queue_push(fio___srv_tasks,
                   subscription_on_unsubscribe___task,
                   u.p,
                   s->udata);
  }
  void (*const free_fn[2])(void *) = {fio_free, free};
  free_fn[!s->io](s);
  // FIO_LOG_DDEBUG2("(%d) freed subscription: %p", fio_pid(), s);
}

/* *****************************************************************************
Message Delivery (to subscription)
***************************************************************************** */

/* an internal (temporary) message object. */
typedef struct {
  volatile size_t flag;
  letter_s *letter;
  fio_msg_s msg;
} fio_msg_internal_s;

/* returns the letter object associated with the message. */
FIO_IFUNC letter_s *fio_msg2letter(fio_msg_s *msg) {
  fio_msg_internal_s *mi = FIO_PTR_FROM_FIELD(fio_msg_internal_s, msg, msg);
  return mi->letter;
}

/** Defers the current callback, so it will be called again for the message. */
void fio_message_defer(fio_msg_s *msg) {
  fio_msg_internal_s *mi = FIO_PTR_FROM_FIELD(fio_msg_internal_s, msg, msg);
  mi->flag = 1;
}

/** calls the callback within a lock, using a fio_msg_s pointer. */
FIO_SFUNC void subscription_deliver__task(void *s_, void *l_) {
  subscription_s *s = (subscription_s *)s_;
  letter_s *l = (letter_s *)l_;
  fio_msg_internal_s mi = {
      .flag = 0,
      .letter = l,
      .msg =
          {
              .io = s->io,
              .channel =
                  {
                      .buf = letter_channel(l),
                      .len = letter_channel_len(l),
                  },
              .message =
                  {
                      .buf = letter_message(l),
                      .len = letter_message_len(l),
                  },
              .udata = s->udata,
              .filter = letter_filter(l),
              .is_json =
                  ((l->buf[0] & FIO_PUBSUB_JSON_BIT) / FIO_PUBSUB_JSON_BIT),
          },
  };
  s->on_message(&mi.msg);
  if (mi.flag)
    goto reschedule;

  fio_undup(s->io);
  subscription_free(s);
  letter_free(l);
  return;
reschedule:
  fio_queue_push(fio___srv_tasks, subscription_deliver__task, s, l);
}

/* schedules a letter delivery to a subscription */
FIO_IFUNC void subscription_deliver(subscription_s *s, letter_s *l) {
  if (s->disabled)
    return;
  if (s->io) {
    if (s->io == l->from || !fio_is_valid(s->io))
      return;
    fio_dup(s->io);
  }
  fio_queue_push(fio___srv_tasks,
                 subscription_deliver__task,
                 subscription_dup(s),
                 letter_dup2(l));
}

/* *****************************************************************************
Channel Object API
***************************************************************************** */

/* creates a new channel object. */
FIO_IFUNC channel_s *channel_new(channel_type_e channel_type,
                                 int32_t filter,
                                 char *name,
                                 size_t name_len) {
  channel_s *c = malloc(sizeof(*c) + name_len + 1);
  FIO_ASSERT_ALLOC(c);
  *c = (channel_s){
      .subscriptions = FIO_LIST_INIT(c->subscriptions),
      .type = channel_type,
      .filter = filter,
      .name_len = name_len,
      .name = (char *)(c + 1),
  };
  c->name[name_len] = 0;
  if (!name_len)
    return c;
  FIO_MEMCPY(c->name, name, name_len);
  // FIO_LOG_DDEBUG2("allocating a new channel: %s", c->name);
  return c;
}

/* frees a channel, making sure all subscriptions are destroyed. */
FIO_IFUNC void channel_free(channel_s *channel) {
  if (!channel)
    return;
  /* make sure no subscriptions are leaked during termination */
  FIO_LIST_EACH(subscription_s, node, &channel->subscriptions, s) {
    s->channel = NULL;
    subscription_free(s);
  }
  // FIO_LOG_DDEBUG2("freeing channel: %s", channel->name);
  free(channel);
}

/* tests to see if two channel objects are the same. */
FIO_IFUNC _Bool channel_is_eq(channel_s *a, channel_s *b) {
  return a->filter == b->filter && a->name_len == b->name_len &&
         !memcmp(a->name, b->name, a->name_len);
}

/* hashing helper (hashes a channel name using it's filter as "salt". */
FIO_IFUNC uint64_t channel2hash(channel_s ch) {
  return fio_risky_hash(ch.name, ch.name_len, ch.filter);
}

/* *****************************************************************************
Postoffice
***************************************************************************** */

FIO_SFUNC void postoffice_on_channel_added(channel_s *);
FIO_SFUNC void postoffice_on_channel_removed(channel_s *);

#define FIO_MAP_NAME          channel_store
#define FIO_MAP_KEY           channel_s *
#define FIO_MAP_KEY_CMP(a, b) channel_is_eq(a, b)
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  do {                                                                         \
    ((dest) = (src));                                                          \
    postoffice_on_channel_added((src));                                        \
  } while (0)
#define FIO_MAP_KEY_DESTROY(ch)                                                \
  do {                                                                         \
    postoffice_on_channel_removed((ch));                                       \
    channel_free((ch));                                                        \
  } while (0)
#define FIO_MAP_KEY_DISCARD(ch)                                                \
  do {                                                                         \
    channel_free((ch));                                                        \
  } while (0)
#define FIO_STL_KEEP__ 1
#include __FILE__
#undef FIO_STL_KEEP__

/** The postoffice data store */
static struct {
  uint8_t filter_local;
  uint8_t filter_ipc;
  uint8_t filter_cluster;
  channel_store_s channels[CHANNEL_TYPE_NONE];
  fio_pubsub_engine_s *engines[FIO_PUBSUB_ENGINE_LIMIT];
  char ipc_url[40];                 /* inter-process address  - buffer */
  char cluster_url[40];             /* machine cluster address - buffer */
  fio_protocol_s ipc;               /* inter-process communication protocol */
  fio_protocol_s ipc_listen;        /* accepts inter-process connections */
  fio_protocol_s cluster;           /* machine cluster communication protocol */
  fio_protocol_s cluster_listen;    /* accepts machine cluster connections */
  fio_protocol_s cluster_discovery; /* network cluster discovery */
  void *cluster_server_tls;
  void *cluster_client_tls;
  void *subscription_handles[8];
} postoffice = {
    .filter_local = (FIO_PUBSUB_PROCESS_BIT | FIO_PUBSUB_ROOT_BIT |
                     FIO_PUBSUB_SIBLINGS_BIT),
    .filter_ipc = (~(uint8_t)FIO_PUBSUB_PROCESS_BIT),
    .filter_cluster = FIO_PUBSUB_CLUSTER_BIT,
    .channels =
        {
            FIO_MAP_INIT,
            FIO_MAP_INIT,
        },
};

/* subscribe using a subscription object and a channel */
FIO_IFUNC void postoffice_subscribe(subscription_s *s) {
  if (!s)
    return;
  if (!s->channel || (s->io && !fio_is_valid(s->io)))
    goto error;
  const uint64_t hash = channel2hash(*s->channel);
  s->channel =
      channel_store_set_if_missing(&postoffice.channels[s->channel->type],
                                   hash,
                                   s->channel);
  FIO_LIST_PUSH(&s->channel->subscriptions, &s->node);
  return;

error:
  channel_free(s->channel);
  subscription_free(s);
}

/* unsubscribe using a subscription object */
FIO_IFUNC void postoffice_unsubscribe(subscription_s *s) {
  if (!s)
    return;
  channel_s *ch = s->channel;
  s->disabled = 1;
  FIO_LIST_REMOVE(&s->node);
  subscription_free(s);
  if (!ch || !FIO_LIST_IS_EMPTY(&ch->subscriptions))
    return;
  const uint64_t hash = channel2hash(ch[0]);
  channel_store_remove(&postoffice.channels[ch->type], hash, ch, NULL);
}

/* deliver a letter to all subscriptions in the relevant channels */
FIO_IFUNC void postoffice_deliver2process(letter_s *l) {
  channel_s ch_key = {
      .filter = letter_filter(l),
      .name_len = letter_channel_len(l),
      .name = letter_channel(l),
  };
  const uint64_t hash = channel2hash(ch_key);
  channel_s *ch = channel_store_get(postoffice.channels, hash, &ch_key);
  if (ch) {
    FIO_LIST_EACH(subscription_s, node, &ch->subscriptions, s) {
      subscription_deliver(s, l);
    }
  }
  if (!channel_store_count(&postoffice.channels[CHANNEL_TYPE_PATTERN]))
    return;

  fio_str_info_s name = {
      .buf = letter_channel(l),
      .len = letter_channel_len(l),
  };
  FIO_MAP_EACH(channel_store, &postoffice.channels[CHANNEL_TYPE_PATTERN], pos) {
    if (pos.key->filter != letter_filter(l))
      continue;
    fio_str_info_s pat = {
        .buf = pos.key->name,
        .len = pos.key->name_len,
    };
    if (FIO_PUBSUB_PATTERN_MATCH(pat, name)) {
      FIO_LIST_EACH(subscription_s, node, &pos.key->subscriptions, s) {
        subscription_deliver(s, l);
      }
    }
  }
}

FIO_SFUNC void postoffice_deliver2io___task(void *io_, void *l_) {
  letter_write(io_, l_);
  letter_free(l_);
  fio_free2(io_);
}

FIO_IFUNC void postoffice_deliver2ipc(letter_s *l) {
  FIO_LIST_EACH(fio_s, node, &postoffice.ipc.reserved.ios, io) {
    fio_queue_push(fio___srv_tasks,
                   postoffice_deliver2io___task,
                   fio_dup2(io),
                   letter_dup2(l));
  }
}

FIO_IFUNC void postoffice_deliver2cluster(letter_s *l) {
  FIO_LIST_EACH(fio_s, node, &postoffice.cluster.reserved.ios, io) {
    fio_queue_push(fio___srv_tasks,
                   postoffice_deliver2io___task,
                   fio_dup2(io),
                   letter_dup2(l));
  }
}

/* *****************************************************************************
Pub/Sub - Subscribe / Unsubscribe
***************************************************************************** */

/* perform subscription in system thread */
FIO_SFUNC void fio_subscribe___task(void *ch_, void *s_) {
  subscription_s *s = s_;
  s->channel = ch_;
  postoffice_subscribe(s);
}

/* runs in the system thread */
FIO_SFUNC void fio_unsubscribe___task(void *s, void *ignr_) {
  (void)ignr_;
  postoffice_unsubscribe(s);
}

/* perform subscription in an unknown thread */
FIO_SFUNC void fio_unsubscribe___env_cb(void *s_) {
  subscription_s *s = s_;
  s->disabled = 1;
  fio_queue_push(fio___srv_tasks, fio_unsubscribe___task, s);
}

void fio_subscribe___(void); /* sublimetext marker */
/**
 * Subscribes to either a filter OR a channel (never both).
 *
 * The on_unsubscribe callback will be called on failure.
 */
void fio_subscribe FIO_NOOP(subscribe_args_s args) {
  channel_s *ch = channel_new(
      fio_ct_if(args.is_pattern, CHANNEL_TYPE_PATTERN, CHANNEL_TYPE_NAMED),
      args.filter,
      args.channel.buf,
      args.channel.len);
  subscription_s *s = subscription_new(args.io,
                                       NULL,
                                       args.on_message,
                                       args.on_unsubscribe,
                                       args.udata);
  fio_queue_push(fio___srv_tasks, fio_subscribe___task, ch, s);

  if (!args.subscription_handle_ptr) {
    fio_env_set(args.io,
                .type = (-1LL - args.is_pattern - (!!args.filter)),
                .name = args.channel,
                .on_close = fio_unsubscribe___env_cb,
                .udata = s);
  } else {
    *args.subscription_handle_ptr = (uintptr_t)s;
  }
}

void fio_unsubscribe___(void); /* sublimetext marker */
/**
 * Cancels an existing subscriptions.
 *
 * Accepts the same arguments as `fio_subscribe`, except the `udata` and
 * callback details are ignored (no need to provide `udata` or callback
 * details).
 *
 * Returns -1 if the subscription could not be found. Otherwise returns 0.
 */
int fio_unsubscribe FIO_NOOP(subscribe_args_s args) {
  if (!args.subscription_handle_ptr) {
    return fio_env_remove(args.io,
                          .type = (-1LL - args.is_pattern - (!!args.filter)),
                          .name = args.channel);
  }
  ((subscription_s **)args.subscription_handle_ptr)[0]->disabled = 1;
  fio_queue_push(fio___srv_tasks,
                 fio_unsubscribe___task,
                 ((void **)args.subscription_handle_ptr)[0]);
  return 0;
}

/* *****************************************************************************
Pub/Sub - Publish
***************************************************************************** */
FIO_SFUNC void fio_publish___task(void *letter_, void *ignr_) {
  letter_s *l = letter_;
  if ((l->buf[0] & postoffice.filter_local))
    postoffice_deliver2process(l);
  if ((l->buf[0] & postoffice.filter_ipc))
    postoffice_deliver2ipc(l);
  if ((l->buf[0] & postoffice.filter_cluster))
    postoffice_deliver2cluster(l);
  letter_free(l);
  (void)ignr_;
}

/**
 * Publishes a message to the relevant subscribers (if any).
 *
 * See `fio_publish_args_s` for details.
 *
 * By default the message is sent using the FIO_PUBSUB_CLUSTER engine (all
 * processes, including the calling process).
 *
 * To limit the message only to other processes (exclude the calling process),
 * use the FIO_PUBSUB_SIBLINGS engine.
 *
 * To limit the message only to the calling process, use the
 * FIO_PUBSUB_PROCESS engine.
 *
 * To publish messages to the pub/sub layer, the `.filter` argument MUST be
 * equal to 0 or missing.
 */
void fio_publish___(void); /* SublimeText marker*/
void fio_publish FIO_NOOP(fio_publish_args_s args) {
  const fio_pubsub_engine_s *engines[3] = {
      FIO_PUBSUB_LOCAL,
      FIO_PUBSUB_DEFAULT,
  };
  if (!args.engine)
    args.engine = engines[(!args.filter)];
  if ((uintptr_t)(args.engine) > 0XFF) {
    if (!args.filter)
      goto external_engine;
    args.engine = FIO_PUBSUB_LOCAL;
  }
  letter_s *l =
      letter_author(args.from,
                    fio_rand64(),
                    args.filter,
                    args.channel.buf,
                    args.channel.len,
                    args.message.buf,
                    args.message.len,
                    (uint8_t)(uintptr_t)args.engine |
                        ((0x100 - args.is_json) & FIO_PUBSUB_JSON_BIT));
  fio_queue_push(fio___srv_tasks, fio_publish___task, l);
  return;
external_engine:
  args.engine->publish(args.engine, args.channel, args.message, args.is_json);
}

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
                                     uint8_t is_json);

/**
 * It's possible to attach metadata to facil.io named messages (filter == 0)
 * before they are published.
 *
 * Returns zero (0) on success or -1 on failure.
 *
 * Multiple `fio_message_metadata_add` calls increase a reference count and
 * should be matched by the same number of `fio_message_metadata_remove`.
 */
int fio_message_metadata_add(fio_msg_metadata_fn metadata_func,
                             void (*cleanup)(void *));

/**
 * Removed the metadata callback.
 *
 * Removal might be delayed if live metatdata exists.
 */
void fio_message_metadata_remove(fio_msg_metadata_fn metadata_func);

/**
 * Finds the message's metadata, returning the data or NULL.
 *
 * Note: channels with non-zero filters don't have metadata attached.
 */
void *fio_message_metadata(fio_msg_metadata_fn metadata_func);

/* *****************************************************************************
 * Cluster / Pub/Sub Middleware and Extensions ("Engines")
 **************************************************************************** */

static void fio_pubsub_mock_detached(const fio_pubsub_engine_s *eng) {
  (void)eng;
}
static void fio_pubsub_mock_subscribe(const fio_pubsub_engine_s *eng,
                                      fio_str_info_s channel) {
  (void)eng;
  (void)channel;
}
static void fio_pubsub_mock_unsubscribe(const fio_pubsub_engine_s *eng,
                                        fio_str_info_s channel) {
  (void)eng;
  (void)channel;
}
static void fio_pubsub_mock_psubscribe(const fio_pubsub_engine_s *eng,
                                       fio_str_info_s channel) {
  (void)eng;
  (void)channel;
}
static void fio_pubsub_mock_punsubscribe(const fio_pubsub_engine_s *eng,
                                         fio_str_info_s channel) {
  (void)eng;
  (void)channel;
}
static void fio_pubsub_mock_publish(const fio_pubsub_engine_s *eng,
                                    fio_str_info_s channel,
                                    fio_str_info_s msg,
                                    uint8_t is_json) {
  (void)eng;
  (void)channel;
  (void)msg;
  (void)is_json;
}

/**
 * Engines can ask facil.io to call the `subscribe` callback for all active
 * channels.
 */
static void fio_pubsub_resubscribe_all___task(void *engine, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *e = (fio_pubsub_engine_s *)engine;
  FIO_MAP_EACH(channel_store, &postoffice.channels[CHANNEL_TYPE_NAMED], pos) {
    if (pos.key->filter)
      continue;
    fio_str_info_s ch = {pos.key->name, pos.key->name_len};
    e->subscribe(e, ch);
  }
  FIO_MAP_EACH(channel_store, &postoffice.channels[CHANNEL_TYPE_PATTERN], pos) {
    if (pos.key->filter)
      continue;
    fio_str_info_s ch = {pos.key->name, pos.key->name_len};
    e->psubscribe(e, ch);
  }
}

/** Attaches an engine, so it's callback can be called by facil.io. */
static void fio_pubsub_attach___task(void *engine, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *e = (fio_pubsub_engine_s *)engine;
  if (!e->detached)
    e->detached = fio_pubsub_mock_detached;
  if (!e->subscribe)
    e->subscribe = fio_pubsub_mock_subscribe;
  if (!e->unsubscribe)
    e->unsubscribe = fio_pubsub_mock_unsubscribe;
  if (!e->psubscribe)
    e->psubscribe = fio_pubsub_mock_psubscribe;
  if (!e->punsubscribe)
    e->punsubscribe = fio_pubsub_mock_punsubscribe;
  if (!e->publish)
    e->publish = fio_pubsub_mock_publish;

  for (int i = 0; i < FIO_PUBSUB_ENGINE_LIMIT; ++i) {
    if (postoffice.engines[i])
      continue;
    postoffice.engines[i] = engine;
    fio_pubsub_resubscribe_all___task(engine, NULL);
    return;
  }
  e->detached(e);
}

static void fio_pubsub_detach___task_called(void *engine, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *e = (fio_pubsub_engine_s *)engine;
  e->detached(e);
}

/** Detaches an engine, so it could be safely destroyed. */
static void fio_pubsub_detach___task(void *engine, void *ignr_) {
  (void)ignr_;
  fio_pubsub_engine_s *e = (fio_pubsub_engine_s *)engine;
  int i = 0;

  do {
    if (postoffice.engines[i] == e)
      break;
  } while ((++i) < FIO_PUBSUB_ENGINE_LIMIT);

  while (i < (FIO_PUBSUB_ENGINE_LIMIT - 1) && postoffice.engines[i + 1]) {
    postoffice.engines[i] = postoffice.engines[i + 1];
    ++i;
  }
  fio_queue_push(fio___srv_tasks, fio_pubsub_detach___task_called, e);
  postoffice.engines[i] = NULL;
}

/**
 * Attaches an engine, so it's callback can be called by facil.io.
 */
void fio_pubsub_attach(fio_pubsub_engine_s *engine) {
  fio_queue_push(fio___srv_tasks, fio_pubsub_attach___task, engine);
}

/** Detaches an engine, so it could be safely destroyed. */
void fio_pubsub_detach(fio_pubsub_engine_s *engine) {
  fio_queue_push(fio___srv_tasks, fio_pubsub_detach___task, engine);
}

/**
 * Engines can ask facil.io to call the `subscribe` callback for all active
 * channels.
 */
void fio_pubsub_resubscribe_all(fio_pubsub_engine_s *engine) {
  fio_queue_push(fio___srv_tasks, fio_pubsub_resubscribe_all___task, engine);
}

/** Returns true (1) if the engine is attached to the system. */
int fio_pubsub_is_attached(fio_pubsub_engine_s *engine) {
  for (int i = 0; i < FIO_PUBSUB_ENGINE_LIMIT; ++i) {
    if (postoffice.engines[i] == engine)
      return 1;
  }
  return 0;
}

/* *****************************************************************************
Pub/Sub IPC and Cluster `fio_protocol_s` callbacks
***************************************************************************** */

FIO_SFUNC void pubsub_cluster_on_letter(letter_s *l) {
  /* TODO: test for and discard duplicates */
  postoffice_deliver2process(l);
  postoffice_deliver2ipc(l);
  postoffice_deliver2cluster(l);
}

FIO_SFUNC void pubsub_ipc_on_letter_master(letter_s *l) {
  if ((l->buf[0] & FIO_PUBSUB_ROOT_BIT))
    postoffice_deliver2process(l);
  if ((l->buf[0] & FIO_PUBSUB_SIBLINGS_BIT))
    postoffice_deliver2ipc(l);
  if ((l->buf[0] & FIO_PUBSUB_CLUSTER_BIT))
    postoffice_deliver2cluster(l);
}

FIO_SFUNC void pubsub_ipc_on_letter_worker(letter_s *l) {
  postoffice_deliver2process(l);
}

FIO_SFUNC void pubsub_ipc_on_data_master(fio_s *io) {
  letter_read(io, fio_udata_get(io), pubsub_ipc_on_letter_master);
}

FIO_SFUNC void pubsub_ipc_on_data_worker(fio_s *io) {
  letter_read(io, fio_udata_get(io), pubsub_ipc_on_letter_worker);
}

FIO_SFUNC void pubsub_cluster_on_data(fio_s *io) {
  if (letter_read(io, fio_udata_get(io), pubsub_ipc_on_letter_master))
    fio_close(io);
}

FIO_SFUNC void pubsub_ipc_on_close(void *udata) { letter_parser_free(udata); }

FIO_SFUNC void pubsub_ipc_on_close_in_child(void *udata) {
  letter_parser_free(udata);
  if (fio_srv_is_running()) {
    FIO_LOG_ERROR(
        "(%d) pub/sub connection to master process lost while running.",
        (int)fio___srvdata.pid);
    fio_stop();
  }
}

FIO_SFUNC void pubsub_connection_ping(fio_s *io) {
  letter_s *l =
      letter_author(NULL, 0, -1, NULL, 0, NULL, 0, FIO_PUBSUB_PING_BIT);
  letter_write(io, l);
  letter_free(l);
}

FIO_SFUNC void pubsub_cluster_on_close(void *udata) {
  /* TODO: allow re-connections from this address */
  letter_parser_free(udata);
}

FIO_SFUNC void pubsub_listener_ipc_on_close(void *udata) {
  (void)udata;
  if (fio___srvdata.root_pid == fio___srvdata.pid)
    FIO_LOG_DEBUG2("(%d) PostOffice stopped listening for IPC @ %s",
                   fio___srvdata.pid,
                   postoffice.ipc_url);
}

FIO_SFUNC void pubsub_listener_cluster_on_close(void *udata) {
  (void)udata;
  if (fio___srvdata.root_pid == fio___srvdata.pid)
    FIO_LOG_DEBUG2("(%d) PostOffice stopped listening for remote machines @ %s",
                   fio___srvdata.pid,
                   postoffice.cluster_url);
}

FIO_SFUNC void pubsub_ipc_on_new_connection(fio_s *io) {
  if (fio___srvdata.root_pid != fio___srvdata.pid)
    return;
  int fd = accept(io->fd, NULL, NULL);
  if (fd == -1)
    return;
  io = fio_attach_fd(fd, fio_udata_get(io), letter_parser_new(), NULL);
  FIO_LOG_DDEBUG2("(%d) PostOffice accepted connection @ %p (%d).",
                  (int)fio___srvdata.pid,
                  io,
                  io->fd);
}

/* *****************************************************************************
PostOffice add / remove hooks
***************************************************************************** */

/* callback called when a channel is added to a channel_store map */
FIO_SFUNC void postoffice_on_channel_added(channel_s *ch) {
  FIO_LOG_DDEBUG2("(%d) PostOffice opened channel (filter %ld): %.*s",
                  fio___srvdata.pid,
                  (long)ch->filter,
                  (int)ch->name_len,
                  ch->name);
  if (ch->filter)
    return;
  fio_str_info_s cmds[] = {
      {"subscribe", 9},
      {"psubscribe", 10},
  };
  fio_publish(.engine = FIO_PUBSUB_ROOT,
              .filter = -1,
              .channel = cmds[ch->type == CHANNEL_TYPE_PATTERN],
              .message = {ch->name, ch->name_len});
}

/* callback called when a channel is freed from a channel_store map */
FIO_SFUNC void postoffice_on_channel_removed(channel_s *ch) {
  FIO_LOG_DDEBUG2("(%d) PostOffice closed channel (filter %ld): %.*s",
                  fio___srvdata.pid,
                  (long)ch->filter,
                  (int)ch->name_len,
                  ch->name);
  if (ch->filter)
    return;
  fio_str_info_s cmds[] = {
      {"unsubscribe", 11},
      {"punsubscribe", 12},
  };
  fio_publish(.engine = FIO_PUBSUB_ROOT,
              .filter = -1,
              .channel = cmds[ch->type == CHANNEL_TYPE_PATTERN],
              .message = {ch->name, ch->name_len});
}

/* registers a child's subscribe event and calls subscribe for engines */
FIO_SFUNC void postoffice_on_global_subscribe(fio_msg_s *msg) {
  /* child process subscribe ? */
  if (fio_msg2letter(msg)->from) {
    fio_subscribe(.io = fio_msg2letter(msg)->from,
                  .channel = msg->message,
                  .is_pattern = 0);
    return;
  }
  /* global channel created. */
  FIO_LOG_DDEBUG2("(%d) PostOffice global channel %s: %s",
                  (int)fio___srvdata.pid,
                  msg->channel.buf,
                  msg->message.buf);
  for (int i = 0; i < FIO_PUBSUB_ENGINE_LIMIT && postoffice.engines[i]; ++i) {
    postoffice.engines[i]->subscribe(postoffice.engines[i], msg->message);
  }
}

/* registers a child's psubscribe event and calls psubscribe for engines */
FIO_SFUNC void postoffice_on_global_psubscribe(fio_msg_s *msg) {
  /* child process subscribe ? */
  if (fio_msg2letter(msg)->from) {
    fio_subscribe(.io = fio_msg2letter(msg)->from,
                  .channel = msg->message,
                  .is_pattern = 1);
    return;
  }
  /* global channel created. */
  FIO_LOG_DDEBUG2("(%d) PostOffice global channel %s: %s",
                  (int)fio___srvdata.pid,
                  msg->channel.buf,
                  msg->message.buf);
  for (int i = 0; i < FIO_PUBSUB_ENGINE_LIMIT && postoffice.engines[i]; ++i) {
    postoffice.engines[i]->psubscribe(postoffice.engines[i], msg->message);
  }
}

/* registers a child's unsubscribe event and calls unsubscribe for engines */
FIO_SFUNC void postoffice_on_global_unsubscribe(fio_msg_s *msg) {
  /* child process subscribe ? */
  if (fio_msg2letter(msg)->from) {
    fio_unsubscribe(.io = fio_msg2letter(msg)->from,
                    .channel = msg->message,
                    .is_pattern = 0);
    return;
  }
  /* channel removed. */
  FIO_LOG_DDEBUG2("(%d) PostOffice global channel %s: %s",
                  (int)fio___srvdata.pid,
                  msg->channel.buf,
                  msg->message.buf);
  for (int i = 0; i < FIO_PUBSUB_ENGINE_LIMIT && postoffice.engines[i]; ++i) {
    postoffice.engines[i]->unsubscribe(postoffice.engines[i], msg->message);
  }
}

/* registers a child's pubsubscribe event and calls pubsubscribe for engines */
FIO_SFUNC void postoffice_on_global_pubsubscribe(fio_msg_s *msg) {
  /* child process subscribe ? */
  if (fio_msg2letter(msg)->from) {
    fio_unsubscribe(.io = fio_msg2letter(msg)->from,
                    .channel = msg->message,
                    .is_pattern = 1);
    return;
  }
  /* global channel removed. */
  FIO_LOG_DDEBUG2("(%d) PostOffice global channel %s: %s",
                  (int)fio___srvdata.pid,
                  msg->channel.buf,
                  msg->message.buf);
  for (int i = 0; i < FIO_PUBSUB_ENGINE_LIMIT && postoffice.engines[i]; ++i) {
    postoffice.engines[i]->punsubscribe(postoffice.engines[i], msg->message);
  }
}

/* *****************************************************************************
Pub/Sub Initialization / Cleanup callbacks
***************************************************************************** */

FIO_SFUNC void postoffice___pre_start(void *ignr_) {
  (void)ignr_;
  postoffice.filter_local = FIO_PUBSUB_PROCESS_BIT | FIO_PUBSUB_ROOT_BIT;
  postoffice.filter_ipc = FIO_PUBSUB_SIBLINGS_BIT;
  postoffice.filter_cluster = FIO_PUBSUB_CLUSTER_BIT;
  postoffice.ipc.on_data = pubsub_ipc_on_data_master;
  postoffice.ipc.on_close = pubsub_ipc_on_close;
  postoffice.ipc.on_timeout = pubsub_connection_ping;
  postoffice.ipc.timeout = 300;
  postoffice.ipc_listen.on_data = pubsub_ipc_on_new_connection;
  postoffice.ipc_listen.on_close = pubsub_listener_ipc_on_close;
  postoffice.ipc_listen.on_timeout = FIO_PING_ETERNAL;
  postoffice.cluster.on_data = pubsub_cluster_on_data;
  postoffice.cluster.on_close = pubsub_ipc_on_close;
  postoffice.cluster.on_timeout = pubsub_connection_ping;
  postoffice.cluster.timeout = 300;
  postoffice.cluster_listen.on_data = pubsub_ipc_on_new_connection;
  postoffice.cluster_listen.on_close = pubsub_listener_cluster_on_close;
  postoffice.cluster_listen.on_timeout = FIO_PING_ETERNAL;
  if (fio___srvdata.workers) {
    int fd =
        fio_sock_open2(postoffice.ipc_url, FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);
    FIO_ASSERT(fd != -1,
               "failed to create IPC listening socket.",
               (int)fio_data.pid);
    fio_attach_fd(fd, &postoffice.ipc_listen, &postoffice.ipc, NULL);
    FIO_LOG_DEBUG2("listening for pub/sub IPC @ %s", postoffice.ipc_url);
  }
}

FIO_SFUNC void postoffice_forked_child(void *ignr_) {
  (void)ignr_;
  for (fio_protocol_s *pr = &postoffice.ipc;
       pr <= &postoffice.cluster_discovery;
       ++pr) {
    FIO_LIST_EACH(fio_s, timeouts, &pr->reserved.ios, io) {
      fio_sock_close(io->fd);
      io->fd = -1;
      fio_close_now(io);
    }
  }
  for (int i = 0; postoffice.subscription_handles[i]; ++i) {
    fio_unsubscribe(.subscription_handle_ptr =
                        postoffice.subscription_handles + i);
    postoffice.subscription_handles[i] = 0;
  }
  // perform all callbacks before replacing them
  while (!fio_queue_perform(fio___srv_tasks))
    ;

  postoffice.ipc.on_data = pubsub_ipc_on_data_worker;
  postoffice.ipc.on_close = pubsub_ipc_on_close_in_child;
  postoffice.filter_local = FIO_PUBSUB_PROCESS_BIT;
  postoffice.filter_ipc = ~(uint8_t)FIO_PUBSUB_PROCESS_BIT;
  postoffice.ipc.timeout = -1;
  postoffice.filter_cluster = 0;
  int fd =
      fio_sock_open2(postoffice.ipc_url, FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK);
  FIO_ASSERT(fd != -1,
             "(%d) failed to connect to master process.",
             (int)fio_data.pid);
  fio_s *io = fio_attach_fd(fd, &postoffice.ipc, letter_parser_new(), NULL);
  (void)io;
  FIO_LOG_DDEBUG2("(%d) connecting IPC to PostOffice %p (fd %d)",
                  (int)fio_data.pid,
                  (void *)io,
                  fd);
}

FIO_SFUNC void postoffice_on_finish(void *ignr_) {
  (void)ignr_;
  postoffice.filter_local = FIO_PUBSUB_PROCESS_BIT | FIO_PUBSUB_ROOT_BIT;
}

FIO_SFUNC void postoffice_at_exit(void *ignr_) {
  (void)ignr_;
  for (int i = 0; i < FIO_PUBSUB_ENGINE_LIMIT; ++i) {
    if (!postoffice.engines[i])
      continue;
    postoffice.engines[i]->detached(postoffice.engines[i]);
    postoffice.engines[i] = NULL;
  }
  for (int i = 0; i < CHANNEL_TYPE_NONE; ++i) {
    channel_store_destroy(postoffice.channels + i);
  }
  if (!fio_data.is_master)
    return;

  if (!memcmp(postoffice.ipc_url, "unix:", 5) |
      !memcmp(postoffice.ipc_url, "file:", 5)) {
    fio_url_s url =
        fio_url_parse(postoffice.ipc_url, strlen(postoffice.ipc_url));
    FIO_LOG_DDEBUG2("(%d) PostOffice unlinking %s", fio_data.pid, url.path.buf);
    unlink(url.path.buf);
  }
}

FIO_SFUNC void postoffice_initialize(void) {
  for (fio_protocol_s *pr = &postoffice.ipc;
       pr <= &postoffice.cluster_discovery;
       ++pr) {
    fio_protocol_validate(pr);
    pr->reserved.flags |= 1;
    FIO_LIST_PUSH(&fio_data.protocols, &pr->reserved.protocols);
  }
  fio_state_callback_add(FIO_CALL_PRE_START, postoffice___pre_start, NULL);
  fio_state_callback_add(FIO_CALL_IN_CHILD, postoffice_forked_child, NULL);
  fio_state_callback_add(FIO_CALL_ON_FINISH, postoffice_on_finish, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, postoffice_at_exit, NULL);
  subscribe_args_s to_subscribe[] = {
      {
          .filter = -1,
          .channel = {"subscribe", 9},
          .on_message = postoffice_on_global_subscribe,
      },
      {
          .filter = -1,
          .channel = {"psubscribe", 10},
          .on_message = postoffice_on_global_psubscribe,
      },
      {
          .filter = -1,
          .channel = {"unsubscribe", 11},
          .on_message = postoffice_on_global_unsubscribe,
      },
      {
          .filter = -1,
          .channel = {"punsubscribe", 12},
          .on_message = postoffice_on_global_pubsubscribe,
      },
      {0},
  };
  for (int i = 0; to_subscribe[i].on_message; ++i) {
    to_subscribe[i].subscription_handle_ptr =
        postoffice.subscription_handles + i;
    fio_subscribe FIO_NOOP(to_subscribe[i]);
  }
  size_t pos = 0;
#if FIO_OS_POSIX
  memcpy(postoffice.ipc_url, "unix://./fio-pubsub-", 20);
  pos += 20;
  pos += fio_ltoa(postoffice.ipc_url + pos, ((uint64_t)fio_rand64() >> 24), 32);
  memcpy(postoffice.ipc_url + pos, ".sock", 5);
  pos += 5;
#else
  memcpy(postoffice.ipc_url, "tcp://127.0.0.1:9999", 20);
  pos += 20;
#endif
  postoffice.ipc_url[pos] = 0;
}

/* *****************************************************************************
Pub/Sub Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_letter)(void) {
  struct test_info {
    uint64_t id;
    char *channel;
    char *msg;
    int32_t filter;
  } test_info[] = {
      {
          42,
          "My Channel",
          "My channel Message",
          0,
      },
      {
          0,
          NULL,
          "My filter Message",
          1,
      },
      {0},
  };
  for (int i = 0;
       test_info[i].msg || test_info[i].channel || test_info[i].filter;
       ++i) {
    letter_s *l =
        letter_author((fio_s *)(test_info + i),
                      test_info[i].id,
                      test_info[i].filter,
                      test_info[i].channel,
                      (test_info[i].channel ? strlen(test_info[i].channel) : 0),
                      test_info[i].msg,
                      (test_info[i].msg ? strlen(test_info[i].msg) : 0),
                      (uint8_t)(uintptr_t)FIO_PUBSUB_LOCAL);
    FIO_ASSERT(letter_id(l) == test_info[i].id,
               "message ID identity error, %llu != %llu",
               letter_id(l),
               test_info[i].id);
    FIO_ASSERT(letter_is_filter(l) == !!test_info[i].filter,
               "letter filter flag author error");
    if (letter_is_filter(l)) {
      FIO_ASSERT(letter_filter(l) == test_info[i].filter,
                 "filter identity error %d != %d",
                 letter_filter(l),
                 test_info[i].filter);
    }
    if (test_info[i].msg) {
      FIO_ASSERT(letter_message_len(l) == strlen(test_info[i].msg),
                 "letter message length error");
      FIO_ASSERT(
          !memcmp(letter_message(l), test_info[i].msg, letter_message_len(l)),
          "message identity error (%s != %.*s)",
          test_info[i].msg,
          (int)letter_message_len(l),
          letter_message(l));
    } else {
      FIO_ASSERT(!letter_message_len(l),
                 "letter message length error %d != 0",
                 letter_message_len(l));
    }
    if (test_info[i].channel) {
      FIO_ASSERT(letter_channel_len(l) == strlen(test_info[i].channel),
                 "letter channel length error");
      FIO_ASSERT(letter_channel(l) && !memcmp(letter_channel(l),
                                              test_info[i].channel,
                                              letter_channel_len(l)),
                 "channel identity error (%s != %.*s)",
                 test_info[i].channel,
                 (int)l->channel_len,
                 letter_channel(l));
    } else {
      FIO_ASSERT(!letter_channel_len(l), "letter channel length error");
    }
    letter_free(l);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub)(void) {
  /*
   * TODO: test Pub/Sub here
   */
  FIO_NAME_TEST(stl, pubsub_letter)();
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Pub/Sub Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_PUBSUB
#endif /* FIO_PUBSUB */
