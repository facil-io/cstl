/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_PUBSUB             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                Pub/Sub Services v2 (Uses FIO_IPC for Transport)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_PUBSUB) && !defined(H___FIO_PUBSUB___H) &&                     \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_PUBSUB___H

/* FIO_PUBSUB requires FIO_IPC for inter-process communication.
 * If FIO_IPC is not already included, we trigger an error.
 * The user should define FIO_IPC before including, or use include.h
 * which handles the proper ordering. */
#ifndef H___FIO_IPC___H
#error "FIO_PUBSUB requires FIO_IPC. Define FIO_IPC before FIO_PUBSUB."
#endif

#ifndef FIO_PUBSUB_FUTURE_LIMIT_MS
/**
 * Maximum time in milliseconds to allow "future" messages to be delivered.
 *
 * If the module detects someone publishing a future message, it will refuse to
 * deliver the message to subscribers.
 *
 * Instead, the module will notify the history engines, allowing them to buffer
 * future messages for future delivery.
 * */
#define FIO_PUBSUB_FUTURE_LIMIT_MS 60000ULL
#endif
/* *****************************************************************************
Pub/Sub2 - Message Format
***************************************************************************** */

/** Message structure, as received by the `on_message` subscription callback. */
struct fio_pubsub_msg_s {
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
};
typedef struct fio_pubsub_msg_s fio_pubsub_msg_s;

/* *****************************************************************************
Pub/Sub2 - Subscribe / Publish API
***************************************************************************** */

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
  void (*on_message)(fio_pubsub_msg_s *msg);
  /** An optional callback for when a subscription is canceled. */
  void (*on_unsubscribe)(void *udata);
  /** The opaque udata value is ignored and made available to the callbacks. */
  void *udata;
  /** The queue to which the callbacks should be routed. May be NULL. */
  fio_queue_s *queue;
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
  /** Replay cached messages (if any) since supplied time in milliseconds. */
  uint64_t replay_since;
  /** A numerical namespace `filter` subscribers need to match. */
  int16_t filter;
  /** If set, pattern matching will be used (name is a pattern). */
  uint8_t is_pattern;
  /** If set, subscription will be limited to the root / master process. */
  uint8_t master_only;
} fio_pubsub_subscribe_args_s;

/**
 * Subscribe to a channel.
 *
 * In worker processes, this uses IPC to notify the master.
 */
SFUNC void fio_pubsub_subscribe(fio_pubsub_subscribe_args_s args);

#define fio_pubsub_subscribe(...)                                              \
  fio_pubsub_subscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})

/**
 * Unsubscribe from a channel.
 *
 * Returns 0 on success, -1 if subscription not found.
 */
SFUNC int fio_pubsub_unsubscribe(fio_pubsub_subscribe_args_s args);

#define fio_pubsub_unsubscribe(...)                                            \
  fio_pubsub_unsubscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})

/** Publish arguments */
typedef struct {
  struct fio_pubsub_engine_s const *engine;
  fio_io_s *from;
  uint64_t id;
  uint64_t timestamp;
  fio_buf_info_s channel;
  fio_buf_info_s message;
  int16_t filter;
} fio_pubsub_publish_args_s;

/**
 * Publish a message to a channel.
 *
 * In worker processes, this uses IPC to notify the master.
 */
SFUNC void fio_pubsub_publish(fio_pubsub_publish_args_s args);

#define fio_pubsub_publish(...)                                                \
  fio_pubsub_publish((fio_pubsub_publish_args_s){__VA_ARGS__})

/** Pushes execution of the on_message callback to the end of the queue. */
SFUNC void fio_pubsub_defer(fio_pubsub_msg_s *msg);

SFUNC void fio_pubsub_match_fn_set(uint8_t (*match_cb)(fio_str_info_s,
                                                       fio_str_info_s));

/* *****************************************************************************
Pub/Sub2 - available IO on_pubsub callback
***************************************************************************** */

/** A callback for IO subscriptions - sends raw message data. */
FIO_SFUNC void FIO_ON_MESSAGE_SEND_MESSAGE(fio_pubsub_msg_s *msg);

/* *****************************************************************************
Pub/Sub2 - Fragile - access the underlying implementation for advance use-cases
***************************************************************************** */

/**
 * Returns the underlying IPC message buffer carrying the message data.
 *
 * This allows message deferral (use fio_ipc_dup) and tighter control over the
 * message's lifetime.
 * */
SFUNC fio_ipc_s *fio_pubsub_msg2ipc(fio_pubsub_msg_s *msg);

/** Extract pub/sub message from IPC message */
SFUNC fio_pubsub_msg_s fio_pubsub_ipc2msg(fio_ipc_s *ipc);

/* *****************************************************************************
Pub/Sub2 - Engine Interface
***************************************************************************** */

/**
 * Engine structure for external pub/sub backends.
 *
 * EXECUTION CONTEXT:
 * - Publish callback can be called from any thread / process.
 * - Subscription callbacks are called from the MASTER process only
 * - Subscription callbacks are called from the main event loop thread
 * - Callbacks MUST NOT block (defer long operations)
 */
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

/** Attach an engine to the pub/sub system */
SFUNC void fio_pubsub_engine_attach(fio_pubsub_engine_s *engine);

/** Detach an engine from the pub/sub system */
SFUNC void fio_pubsub_engine_detach(fio_pubsub_engine_s *engine);

/** Returns the builtin engine for publishing to the process group (IPC). */
SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_ipc(void);

/** Returns the builtin engine for multi-machine cluster publishing (RPC). */
SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_cluster(void);

/** Returns the current default engine associated with the pub/sub system. */
SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_default(void);

/** Sets the current default engine associated with the pub/sub system. */
SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_default_set(
    fio_pubsub_engine_s const *engine);

/* *****************************************************************************
Pub/Sub2 - History Manager Interface
***************************************************************************** */

/**
 * History storage interface - completely separate from engines.
 *
 * EXECUTION CONTEXT:
 * - All callbacks are called from the MASTER process only
 * - Callbacks are called from the main event loop thread
 * - Callbacks MUST NOT block (defer long operations)
 * - Callbacks SHOULD NOT be called directly by the user
 */
typedef struct fio_pubsub_history_s {
  /** Cleanup callback - called when history manager is detached */
  void (*detached)(const struct fio_pubsub_history_s *hist);
  /**
   * Stores a message in history.
   *
   * Returns 0 on success, -1 on error.
   * Called when a message is published (master only).
   */
  int (*push)(const struct fio_pubsub_history_s *hist, fio_pubsub_msg_s *msg);

  /**
   * Replay messages since timestamp by calling callback for each.
   *
   * Returns 0 if replay was handled, -1 if this manager cannot replay.
   *
   * MUST call the `on_done` callback to handle possible cleanup.
   */
  int (*replay)(const struct fio_pubsub_history_s *hist,
                fio_buf_info_s channel,
                int16_t filter,
                uint64_t since,
                void (*on_message)(fio_pubsub_msg_s *msg, void *udata),
                void (*on_done)(void *udata),
                void *udata);

  /**
   * Get oldest available timestamp for a channel.
   *
   * Returns UINT64_MAX if no history available.
   */
  uint64_t (*oldest)(const struct fio_pubsub_history_s *hist,
                     fio_buf_info_s channel,
                     int16_t filter);

} fio_pubsub_history_s;

/**
 * Attach a history manager with the given priority.
 *
 * Multiple history managers can be attached. All managers receive push()
 * calls when messages are published. For replay(), managers are tried in
 * priority order (highest first) until one can handle the request.
 */
SFUNC int fio_pubsub_history_attach(const fio_pubsub_history_s *manager,
                                    uint8_t priority);

/** Detach a history manager. */
SFUNC void fio_pubsub_history_detach(const fio_pubsub_history_s *manager);

/**
 * Pushes a pub/sub message to all history containers.
 *
 * Use ONLY by an engine, if the message needs to be saved to the history
 * managers in the process but is never delivered locally.
 * */
SFUNC void fio_pubsub_history_push_all(fio_pubsub_msg_s *msg);

#ifndef FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT
/** The default cache limit - 256Mb */
#define FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT (1ULL << 28)
#endif
/**
 * Get the built-in in-memory history manager and set it's byte-size limit.
 *
 * A zero value size_limit will be replaced with the following default:
 * - Environment value of WEBSITE_MEMORY_LIMIT_MB * 1024 * 1024
 * - Environment value of WEBSITE_MEMORY_LIMIT_KB
 * - Environment value of WEBSITE_MEMORY_LIMIT
 * - FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT
 * */
SFUNC fio_pubsub_history_s const *fio_pubsub_history_cache(size_t size_limit);

/* *****************************************************************************
Pub/Sub2 - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
#define FIO___RECURSIVE_INCLUDE 1

/* Op-code for cluster messages (in library's reserved range 0xFF000000) */
#define FIO___PUBSUB_OPCODE_PUBLISH 0xFF505501

/* *****************************************************************************
Channel Type - Reference Counting via FIO_REF
***************************************************************************** */

/* Channel structure */
typedef struct fio_pubsub_channel_s {
  FIO_LIST_HEAD subscriptions;
  uint32_t name_len;
  int16_t filter;
  uint8_t is_pattern;
  char name[];
} fio_pubsub_channel_s;

FIO_SFUNC void fio___pubsub_channel_on_create(fio_pubsub_channel_s *ch);
FIO_SFUNC void fio___pubsub_channel_on_destroy(fio_pubsub_channel_s *ch);

/* Channel reference counting */
#define FIO_REF_NAME             fio_pubsub_channel
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_DESTROY(ch)      fio___pubsub_channel_on_destroy(&(ch))
#define FIO_REF_CONSTRUCTOR_ONLY 1
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Subscription Type - Reference Counting via FIO_REF
***************************************************************************** */

/* Subscription structure */
typedef struct fio_pubsub_subscription_s {
  FIO_LIST_NODE node;
  fio_io_s *io;
  void (*on_message)(fio_pubsub_msg_s *msg);
  void (*on_unsubscribe)(void *udata);
  void *udata;
  fio_queue_s *queue;
  uint64_t replay_since;
  fio_pubsub_channel_s *channel;
  int16_t filter;      /* Channel filter (temporary before channel is set) */
  uint8_t is_pattern;  /* Pattern flag (temporary before channel is set) */
  uint8_t master_only; /* If true, subscription exists only in master process */
} fio_pubsub_subscription_s;

FIO_SFUNC void fio___pubsub_subscription_on_destroy(
    fio_pubsub_subscription_s *s);

/* Subscription reference counting */
#define FIO_REF_NAME         fio_pubsub_subscription
#define FIO_REF_DESTROY(obj) fio___pubsub_subscription_on_destroy(&(obj))
#include FIO_INCLUDE_FILE

/* Subscription reference counting helpers - thread management */
FIO_SFUNC void fio___pubsub_subscription_free_task(void *s, void *ignr_) {
  fio_pubsub_subscription_free2(s);
  (void)ignr_;
}

FIO_SFUNC void fio_pubsub_subscription_free(fio_pubsub_subscription_s *s) {
  fio_io_defer(fio___pubsub_subscription_free_task, s, NULL);
}

FIO_IFUNC fio_pubsub_subscription_s *fio_pubsub_subscription_dup(
    fio_pubsub_subscription_s *s) {
  return fio_pubsub_subscription_dup2(s);
}

/** Compute environment type key for subscription storage. */
FIO_IFUNC intptr_t fio___pubsub_channel_env_type(int16_t filter,
                                                 uint8_t is_pattern) {
  return (intptr_t)(0LL - (((2ULL | (!!is_pattern)) << 16) | (uint16_t)filter));
}

/** Stub on_message handler - used for unsubscribed subscriptions */
FIO_SFUNC void fio___pubsub_on_message_stub(fio_pubsub_msg_s *msg) {
  (void)msg;
}

/* *****************************************************************************
Engine Collection
***************************************************************************** */

/* Engine array */
FIO_TYPEDEF_IMAP_ARRAY(fio___pubsub_engines,
                       fio_pubsub_engine_s *,
                       uint32_t,
                       fio_risky_ptr,
                       FIO_IMAP_SIMPLE_CMP,
                       FIO_IMAP_VALID_NON_ZERO)

/* *****************************************************************************
History Collection
***************************************************************************** */

/* History manager entry with priority */
typedef struct {
  fio_pubsub_history_s *manager;
  uint8_t priority;
} fio___pubsub_history_entry_s;

FIO_SFUNC uint64_t
fio___pubsub_history_entry_hash(fio___pubsub_history_entry_s *e) {
  return fio_risky_ptr(e->manager);
}
FIO_SFUNC int fio___pubsub_history_entry_cmp(fio___pubsub_history_entry_s *a,
                                             fio___pubsub_history_entry_s *b) {
  return a->manager == b->manager;
}
FIO_SFUNC int fio___pubsub_history_entry_valid(
    fio___pubsub_history_entry_s *e) {
  return e->manager != NULL;
}

FIO_TYPEDEF_IMAP_ARRAY(fio___pubsub_history_managers,
                       fio___pubsub_history_entry_s,
                       uint32_t,
                       fio___pubsub_history_entry_hash,
                       fio___pubsub_history_entry_cmp,
                       fio___pubsub_history_entry_valid)

/* Sort history managers by priority (descending - highest first) */
#define FIO_SORT_NAME            fio___pubsub_history_entry
#define FIO_SORT_TYPE            fio___pubsub_history_entry_s
#define FIO_SORT_IS_BIGGER(a, b) ((a).priority < (b).priority)
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Wire Format - IPC Message Payloads
***************************************************************************** */

/**
 * Wire format for history request payload in fio_ipc_s.data[]
 *
 * NOTE: filter is carried in ipc->flags (cast int16_t to uint16_t)
 */
typedef struct {
  uint64_t replay_since; /* Timestamp to replay from */
  uint32_t channel_len;  /* Channel name length */
  char channel[];        /* Channel name */
} fio___pubsub_history_request_s;

/* History request header size without flex array padding */
#define FIO___PUBSUB_HISTORY_REQUEST_HEADER_LEN                                \
  FIO_PTR_FIELD_OFFSET(fio___pubsub_history_request_s, channel)

/* *****************************************************************************
Channel Map
***************************************************************************** */

/* Channel map helpers */
#define FIO___PUBSUB_CHANNEL_ENCODE_CAPA(filter_, is_pattern_)                 \
  (((size_t)(is_pattern_) << 16) | (size_t)(uint16_t)(filter_))

#define FIO___PUBSUB_CHANNEL2STR(ch)                                           \
  FIO_STR_INFO3(ch->name,                                                      \
                ch->name_len,                                                  \
                FIO___PUBSUB_CHANNEL_ENCODE_CAPA(ch->filter, ch->is_pattern))

FIO_IFUNC int fio___channel_cmp(fio_pubsub_channel_s *ch, fio_str_info_s s) {
  fio_str_info_s c = FIO___PUBSUB_CHANNEL2STR(ch);
  return c.capa == s.capa && FIO_STR_INFO_IS_EQ(c, s);
}

FIO_IFUNC fio_pubsub_channel_s *fio___pubsub_channel_new_for_map(
    fio_str_info_s s) {
  fio_pubsub_channel_s *ch = fio_pubsub_channel_new(s.len + 1);
  FIO_ASSERT_ALLOC(ch);
  *ch = (fio_pubsub_channel_s){
      .subscriptions = FIO_LIST_INIT(ch->subscriptions),
      .name_len = (uint32_t)s.len,
      .filter = (int16_t)(s.capa & 0xFFFFUL),
      .is_pattern = (uint8_t)(s.capa >> 16),
  };
  if (s.buf && s.len)
    FIO_MEMCPY(ch->name, s.buf, s.len);
  ch->name[s.len] = 0;
  fio___pubsub_channel_on_create(ch);
  return ch;
}

/* Channel map type */
#define FIO_UMAP_NAME                 fio___pubsub_channel_map
#define FIO_MAP_KEY                   fio_str_info_s
#define FIO_MAP_KEY_INTERNAL          fio_pubsub_channel_s *
#define FIO_MAP_KEY_FROM_INTERNAL(k_) FIO___PUBSUB_CHANNEL2STR(k_)
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  ((dest) = fio___pubsub_channel_new_for_map((src)))
#define FIO_MAP_KEY_CMP(a, b)    fio___channel_cmp((a), (b))
#define FIO_MAP_HASH_FN(str)     fio_risky_hash(str.buf, str.len, str.capa)
#define FIO_MAP_KEY_DESTROY(key) fio_pubsub_channel_free((key))
#define FIO_MAP_KEY_DISCARD(key)
#include FIO_INCLUDE_FILE

/* *****************************************************************************
History Buffer Types
***************************************************************************** */

#define FIO___PUBSUB_HISTORY_CHANNEL_HASH(o)                                   \
  (fio_risky_num((o)[0]->len, (o)[0]->timestamp) + (o)[0]->id)
#define FIO___PUBSUB_HISTORY_CHANNEL_CMP(a, b)                                 \
  ((a)[0]->len == (b)[0]->len && (a)[0]->id == (b)[0]->id &&                   \
   (a)[0]->timestamp == (b)[0]->timestamp)

/* History Message Array per Channel */
FIO_TYPEDEF_IMAP_ARRAY(fio___pubsub_history_channel,
                       fio_ipc_s *,
                       uint64_t,
                       FIO___PUBSUB_HISTORY_CHANNEL_HASH,
                       FIO___PUBSUB_HISTORY_CHANNEL_CMP,
                       FIO_IMAP_VALID_NON_ZERO)

/* Sort history managers by priority (descending - highest first) */
#define FIO_SORT_NAME            fio___pubsub_history_channel
#define FIO_SORT_TYPE            fio_ipc_s *
#define FIO_SORT_IS_BIGGER(a, b) ((a)->timestamp > (b)->timestamp)
#include FIO_INCLUDE_FILE

#undef FIO___PUBSUB_HISTORY_CHANNEL_HASH
#undef FIO___PUBSUB_HISTORY_CHANNEL_CMP

FIO_IFUNC size_t fio___pubsub_history_cache_ipc_size(fio_ipc_s *ipc) {
  return ipc->len + sizeof(*ipc) + 8;
}

FIO_IFUNC int fio___pubsub_history_channel_push(
    fio___pubsub_history_channel_s *map,
    fio_ipc_s *msg) {
  fio_ipc_s **pos = fio___pubsub_history_channel_get(map, msg);
  if (pos) {
    FIO_LOG_DDEBUG2("(%d) History message push filtered (existing): %p-%p",
                    (void *)msg->timestamp,
                    (void *)msg->id);
    return -1;
  }
  pos = fio___pubsub_history_channel_set(map, fio_ipc_dup(msg), 1);
  fio___pubsub_history_channel_sort(map->ary, map->w);
  if (*pos != msg) /* did the order change? */
    fio___pubsub_history_channel_rehash(map);
  return 0;
}

FIO_IFUNC size_t
fio___pubsub_history_channel_pop(fio___pubsub_history_channel_s *map) {
  size_t r = 0;
  if (!map->count)
    return r;
  fio_ipc_s *old = map->ary[0];
  r = fio___pubsub_history_cache_ipc_size(old);
  fio___pubsub_history_channel_remove(map, old);
  fio_ipc_free(old);
  return r;
}

FIO_IFUNC void fio___pubsub_history_channel_destroy_all(
    fio___pubsub_history_channel_s *map) {
  FIO_IMAP_EACH(fio___pubsub_history_channel, map, i)
  fio_ipc_free(map->ary[i]);
  fio___pubsub_history_channel_destroy(map);
}

/* History map type */
#define FIO_UMAP_NAME        fio___pubsub_history_map
#define FIO_MAP_KEY          fio_str_info_s
#define FIO_MAP_KEY_INTERNAL char *

#define FIO_MAP_KEY_FROM_INTERNAL(k)                                           \
  ((fio_str_info_s){.buf = k + sizeof(size_t),                                 \
                    .len = fio_bstr_len(k) - sizeof(size_t),                   \
                    .capa = fio_buf2zu(k)})
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  ((dest) = fio_bstr_write2(                                                   \
       NULL,                                                                   \
       FIO_STRING_WRITE_STR2((char *)&(src).capa, sizeof(size_t)),             \
       FIO_STRING_WRITE_STR2((src).buf, (src).len)))
#define FIO_MAP_KEY_CMP(a, b)                                                  \
  ((b).capa == fio_buf2zu((a)) &&                                              \
   (fio_bstr_len((a)) - sizeof(size_t)) == (b).len &&                          \
   !FIO_MEMCMP((a) + sizeof(size_t), (b).buf, (b).len))

#define FIO_MAP_HASH_FN(str)     fio_risky_hash(str.buf, str.len, str.capa)
#define FIO_MAP_KEY_DESTROY(key) fio_bstr_free((key))
#define FIO_MAP_KEY_DISCARD(key)

#define FIO_MAP_VALUE fio___pubsub_history_channel_s
#define FIO_MAP_VALUE_DESTROY(val)                                             \
  fio___pubsub_history_channel_destroy_all(&(val))

#include FIO_INCLUDE_FILE

typedef struct {
  size_t limit;
  size_t size;
  fio___pubsub_history_map_s map;
} fio___pubsub_history_cache_s;
/* TODO: remove older messages as necessary */

FIO_SFUNC void fio___pubsub_history_cache_destroy(
    fio___pubsub_history_cache_s *h) {
  h->size = 0;
  fio___pubsub_history_map_destroy(&h->map);
}

FIO_IFUNC fio___pubsub_history_channel_s *fio___pubsub_history_cache_channel(
    fio___pubsub_history_cache_s *cache,
    fio_buf_info_s channel,
    int16_t filter) {
  fio_str_info_s cname =
      FIO_STR_INFO3(channel.buf, channel.len, (size_t)(uint16_t)filter);
  fio___pubsub_history_map_node_s *pos =
      fio___pubsub_history_map_get_ptr(&cache->map, cname);
  if (!pos)
    return NULL;

  return &pos->value;
}
FIO_IFUNC fio___pubsub_history_channel_s *
fio___pubsub_history_cache_channel_new(fio___pubsub_history_cache_s *cache,
                                       fio_buf_info_s channel,
                                       int16_t filter) {
  fio_str_info_s cname =
      FIO_STR_INFO3(channel.buf, channel.len, (size_t)(uint16_t)filter);
  fio___pubsub_history_map_node_s *pos =
      fio___pubsub_history_map_set_ptr(&cache->map,
                                       cname,
                                       (fio___pubsub_history_channel_s){0},
                                       NULL,
                                       0);
  if (!pos)
    return NULL;
  return &pos->value;
}

/* *****************************************************************************
Global State
***************************************************************************** */

static struct {
  fio___pubsub_channel_map_s channels;
  fio___pubsub_channel_map_s patterns;
  fio___pubsub_engines_s engines;
  fio___pubsub_history_managers_s history_managers;
  uint8_t (*match_fn)(fio_str_info_s pattern, fio_str_info_s name);
  fio_pubsub_engine_s *default_engine;
  fio_pubsub_engine_s local_engine;
  fio_pubsub_engine_s cluster_engine;
  fio_pubsub_history_s cache_manager;
  fio___pubsub_history_cache_s cache;
} FIO___PUBSUB_POSTOFFICE;

/* *****************************************************************************
Pattern Matching & Defaults
***************************************************************************** */

SFUNC void fio_pubsub_match_fn_set(uint8_t (*match_cb)(fio_str_info_s,
                                                       fio_str_info_s)) {
  if (!match_cb)
    match_cb = fio_glob_match;
  FIO___PUBSUB_POSTOFFICE.match_fn = match_cb;
}

/** Returns the current default engine associated with the pub/sub system. */
SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_default(void) {
  return FIO___PUBSUB_POSTOFFICE.default_engine;
}

FIO_SFUNC void fio___pubsub_pre_start(void *ignr_);
/** Sets the current default engine associated with the pub/sub system. */
SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_default_set(
    fio_pubsub_engine_s const *engine) {
  if (!engine)
    engine = fio_pubsub_engine_ipc();
  FIO___PUBSUB_POSTOFFICE.default_engine = (fio_pubsub_engine_s *)engine;
  if (!fio_io_is_running())
    fio_state_callback_remove(FIO_CALL_PRE_START, fio___pubsub_pre_start, NULL);
  return engine;
}

/* *****************************************************************************
Initialization & Cleanup
***************************************************************************** */

FIO_SFUNC void fio___pubsub_cleanup(void *ignr_) {
  /* Cleanup engines.
   * Note: FIO_IMAP_EACH is safe on empty/destroyed arrays (w == 0).
   * fio___pubsub_engines_destroy is safe to call multiple times. */
  FIO_IMAP_EACH(fio___pubsub_engines, &FIO___PUBSUB_POSTOFFICE.engines, i) {
    fio_pubsub_engine_s *e = FIO___PUBSUB_POSTOFFICE.engines.ary[i];
    e->detached(e);
  }
  fio___pubsub_engines_destroy(&FIO___PUBSUB_POSTOFFICE.engines);

  /* Cleanup history managers.
   * Note: FIO_IMAP_EACH is safe on empty/destroyed arrays (w == 0).
   * fio___pubsub_history_managers_destroy is safe to call multiple times. */
  FIO_IMAP_EACH(fio___pubsub_history_managers,
                &FIO___PUBSUB_POSTOFFICE.history_managers,
                i) {
    fio_pubsub_history_s *m =
        FIO___PUBSUB_POSTOFFICE.history_managers.ary[i].manager;
    m->detached(m);
  }
  fio___pubsub_history_managers_destroy(
      &FIO___PUBSUB_POSTOFFICE.history_managers);
  /* clearns and destroys the history cache, just in case */
  FIO___PUBSUB_POSTOFFICE.cache_manager.detached(
      &FIO___PUBSUB_POSTOFFICE.cache_manager);
  (void)ignr_;
}

FIO_SFUNC void fio___pubsub_at_exit(void *ignr_) {
  fio___pubsub_cleanup(ignr_);
  fio___pubsub_channel_map_destroy(&FIO___PUBSUB_POSTOFFICE.channels);
  fio___pubsub_channel_map_destroy(&FIO___PUBSUB_POSTOFFICE.patterns);
}

FIO_SFUNC void fio___pubsub_on_fork_test_unsubscribe_master(
    fio_pubsub_channel_s *ch) {
  /** Loops through every node in the linked list except the head. */
  FIO_LIST_EACH(fio_pubsub_subscription_s, node, &ch->subscriptions, sub) {
    if (sub->master_only) {
      fio_io_env_remove(
          sub->io,
          .type = fio___pubsub_channel_env_type(ch->filter, ch->is_pattern),
          .name = FIO_BUF_INFO2(ch->name, ch->name_len));
    }
  }
}

FIO_SFUNC void fio___pubsub_on_fork(void *ignr_) {
  (void)ignr_;
  if (fio_io_is_master())
    return;
  /* Clean up engines and history managers inherited from master */
  fio___pubsub_cleanup(ignr_);

  /* Unsubscribe from master_only subscriptions.
   * These should be removed from workers - remove them and clean them up,
   * calling on_unsubscribe (to free any associated resources). */
  FIO_MAP_EACH(fio___pubsub_channel_map, &FIO___PUBSUB_POSTOFFICE.channels, i) {
    fio___pubsub_on_fork_test_unsubscribe_master(i.node->key);
  }
  FIO_MAP_EACH(fio___pubsub_channel_map, &FIO___PUBSUB_POSTOFFICE.patterns, i) {
    fio___pubsub_on_fork_test_unsubscribe_master(i.node->key);
  }
}

FIO_SFUNC void fio___pubsub_pre_start(void *ignr_) {
  (void)ignr_;
  uint16_t port = fio_ipc_cluster_port();
  if (port && FIO___PUBSUB_POSTOFFICE.default_engine ==
                  (fio_pubsub_engine_s *)fio_pubsub_engine_ipc())
    FIO___PUBSUB_POSTOFFICE.default_engine =
        (fio_pubsub_engine_s *)fio_pubsub_engine_cluster();
}

/* Forward declaration for opcode handler */
FIO_SFUNC void fio___pubsub_engine_ipc_publish_deliver(fio_ipc_s *ipc);

void fio___pubsub_init____(void); /* IDE Marker */
FIO_CONSTRUCTOR(fio___pubsub_init) {
  /* Initialize defaults */
  FIO___PUBSUB_POSTOFFICE.match_fn = fio_glob_match;
  FIO___PUBSUB_POSTOFFICE.default_engine =
      (fio_pubsub_engine_s *)fio_pubsub_engine_ipc();
  fio_pubsub_engine_cluster(); /* initialize cluster engine */
  fio_pubsub_history_cache(0); /* initialize history limit */
  /* Register op-code for receiving messages */
  if (fio_ipc_opcode_register(.opcode = FIO___PUBSUB_OPCODE_PUBLISH,
                              .call =
                                  fio___pubsub_engine_ipc_publish_deliver)) {
    FIO_LOG_ERROR("fio_pubsub_cluster_init: failed to register op-code");
    return;
  }

  fio_pubsub_history_cache(FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT);
  fio_state_callback_add(FIO_CALL_PRE_START, fio___pubsub_pre_start, NULL);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___pubsub_on_fork, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___pubsub_at_exit, NULL);
}

/* *****************************************************************************
IPC to fio_pubsub_msg_s Format Helper
***************************************************************************** */

/** Extract pub/sub message from IPC message */
FIO_IFUNC fio_pubsub_msg_s fio___pubsub_ipc2msg(fio_ipc_s *ipc) {
  return (fio_pubsub_msg_s){
      .timestamp = ipc->timestamp,
      .id = ipc->id,
      .channel = FIO_BUF_INFO2(ipc->data + 8, fio_buf2u32u(ipc->data)),
      .message = FIO_BUF_INFO2(ipc->data + 9 + fio_buf2u32u(ipc->data),
                               fio_buf2u32u(ipc->data + 4)),
      .filter = (int16_t)ipc->flags,
  };
}

/** Extract pub/sub message from IPC message */
SFUNC fio_pubsub_msg_s fio_pubsub_ipc2msg(fio_ipc_s *ipc) {
  return fio___pubsub_ipc2msg(ipc);
}

/* *****************************************************************************
Message Delivery to Subscriptions
***************************************************************************** */

/** Container for deferred message delivery using IPC message directly */
typedef struct {
  fio_pubsub_subscription_s *sub;
  fio_ipc_s *ipc;
  fio_pubsub_msg_s msg;
  volatile uintptr_t defer_flag;
} fio___pubsub_message_container_s;

FIO_SFUNC void fio___pubsub_subscription_ipc_deliver(void *sub_, void *ipc_) {
  fio___pubsub_message_container_s data = {
      .sub = (fio_pubsub_subscription_s *)sub_,
      .ipc = (fio_ipc_s *)ipc_,
      .msg = fio___pubsub_ipc2msg((fio_ipc_s *)ipc_)};

  data.msg.udata = data.sub->udata;
  data.msg.io = data.sub->io;
  data.sub->on_message(&data.msg);
  data.sub->udata = data.msg.udata;
  if (data.defer_flag) {
    fio_queue_push(data.sub->queue,
                   fio___pubsub_subscription_ipc_deliver,
                   sub_,
                   ipc_);
    return;
  }
  fio_pubsub_subscription_free(data.sub);
  fio_ipc_free(data.ipc);
}

SFUNC void fio_pubsub_defer(fio_pubsub_msg_s *msg) {
  *(volatile uintptr_t *)(msg + 1) = 1;
}

FIO_IFUNC fio_ipc_s *fio___pubsub_msg2ipc(fio_pubsub_msg_s *msg) {
  /* the channel name is 8 bytes after the `data` field starts in the IPC */
  char *data_field = msg->channel.buf - sizeof(uint32_t[2]);
  fio_ipc_s *ipc = FIO_PTR_FROM_FIELD(fio_ipc_s, data, data_field);
  return ipc;
}

/** Returns the underlying IPC message buffer carrying the message data */
SFUNC fio_ipc_s *fio_pubsub_msg2ipc(fio_pubsub_msg_s *msg) {
  fio_ipc_s *ipc = fio___pubsub_msg2ipc(msg);
  fio_ipc_detach(ipc);
  return ipc;
}

/* *****************************************************************************
Message Delivery to History Managers
***************************************************************************** */

FIO_SFUNC void fio___pubsub_history_ipc_push(fio_ipc_s *ipc) {
  fio_pubsub_msg_s msg = fio___pubsub_ipc2msg(ipc);
  msg.io = NULL;

  /* update history engines (if any) about message */
  FIO_IMAP_EACH(fio___pubsub_history_managers,
                &FIO___PUBSUB_POSTOFFICE.history_managers,
                i) {
    fio_pubsub_history_s *mgr =
        FIO___PUBSUB_POSTOFFICE.history_managers.ary[i].manager;
    mgr->push(mgr, &msg);
  }
}

SFUNC void fio_pubsub_history_push_all(fio_pubsub_msg_s *msg) {
  fio_ipc_call(.call = fio___pubsub_history_ipc_push,
               .timestamp = msg->timestamp,
               .id = msg->id,
               .flags = (uint16_t)msg->filter,
               .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)&msg->channel.len,
                                                  sizeof(msg->channel.len)),
                                    FIO_BUF_INFO2((char *)&msg->message.len,
                                                  sizeof(msg->message.len)),
                                    msg->channel,
                                    FIO_BUF_INFO2((char *)"\0", 1),
                                    msg->message,
                                    FIO_BUF_INFO2((char *)"\0", 1)));
}

/* *****************************************************************************
Channel Callbacks
***************************************************************************** */

FIO_SFUNC void fio___pubsub_notify_master_of_subscription(fio_ipc_s *ipc) {
  fio_buf_info_s chname = FIO_BUF_INFO2(ipc->data + 5, fio_buf2u32u(ipc->data));
  uint8_t is_pattern = ipc->data[4];
  int16_t filter = (int16_t)ipc->flags;
  fio_pubsub_subscribe(.io = ipc->from,
                       .channel = chname,
                       .on_message = fio___pubsub_on_message_stub,
                       .filter = filter,
                       .is_pattern = is_pattern);
}

FIO_SFUNC void fio___pubsub_notify_master_of_unsubscription(fio_ipc_s *ipc) {
  fio_buf_info_s chname = FIO_BUF_INFO2(ipc->data + 5, fio_buf2u32u(ipc->data));
  uint8_t is_pattern = ipc->data[4];
  int16_t filter = (int16_t)ipc->flags;
  fio_pubsub_unsubscribe(.io = ipc->from,
                         .channel = chname,
                         .on_message = fio___pubsub_on_message_stub,
                         .filter = filter,
                         .is_pattern = is_pattern);
}

FIO_SFUNC void fio___pubsub_channel_on_create(fio_pubsub_channel_s *ch) {
  fio_buf_info_s name = FIO_BUF_INFO2(ch->name, ch->name_len);
  ch->is_pattern = !!ch->is_pattern;
  FIO_IMAP_EACH(fio___pubsub_engines, &FIO___PUBSUB_POSTOFFICE.engines, i) {
    (&FIO___PUBSUB_POSTOFFICE.engines.ary[i]->subscribe)[ch->is_pattern](
        FIO___PUBSUB_POSTOFFICE.engines.ary[i],
        name,
        ch->filter);
  }
  if (!fio_io_is_master()) {
    fio_ipc_call(.call = fio___pubsub_notify_master_of_subscription,
                 .flags = (uint16_t)ch->filter,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)&ch->name_len, sizeof(ch->name_len)),
                     FIO_BUF_INFO2((char *)&ch->is_pattern, 1),
                     FIO_BUF_INFO2(ch->name, ch->name_len)));
    FIO_LOG_DDEBUG2("(%d) Notifying master of channel subscription: %.*s",
                    fio_io_pid(),
                    (int)ch->name_len,
                    ch->name);
  }
}

FIO_SFUNC void fio___pubsub_channel_on_destroy(fio_pubsub_channel_s *ch) {
  fio_buf_info_s name = FIO_BUF_INFO2(ch->name, ch->name_len);
  FIO_IMAP_EACH(fio___pubsub_engines, &FIO___PUBSUB_POSTOFFICE.engines, i) {
    (&FIO___PUBSUB_POSTOFFICE.engines.ary[i]->unsubscribe)[ch->is_pattern](
        FIO___PUBSUB_POSTOFFICE.engines.ary[i],
        name,
        ch->filter);
  }
  if (!fio_io_is_master()) {
    fio_ipc_call(.call = fio___pubsub_notify_master_of_unsubscription,
                 .flags = (uint16_t)ch->filter,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)&ch->name_len, sizeof(ch->name_len)),
                     FIO_BUF_INFO2((char *)&ch->is_pattern, 1),
                     FIO_BUF_INFO2(ch->name, ch->name_len)));
    FIO_LOG_DDEBUG2("(%d) Notifying master of channel ignored: %.*s",
                    fio_io_pid(),
                    (int)ch->name_len,
                    ch->name);
  }
}

/* *****************************************************************************
Core Builtin Pub/Sub Engine for all process publishing
***************************************************************************** */

/** Called when engine is detached - nothing to do here */
FIO_SFUNC void fio___pubsub_engine_ipc_noop(const fio_pubsub_engine_s *eng) {
  (void)eng;
}

FIO_SFUNC void fio___pubsub_engine_ipc_sub_noop(const fio_pubsub_engine_s *eng,
                                                const fio_buf_info_s channel,
                                                int16_t filter) {
  (void)eng;
  (void)channel;
  (void)filter;
}

/** Distributes an IPC message to all of a channel's subscribers */
FIO_SFUNC void fio___pubsub_engine_ipc_deliver2channel(fio_pubsub_channel_s *ch,
                                                       fio_ipc_s *ipc) {
  FIO_LOG_DEBUG2(
      "(%d) channel_deliver_ipc: ch=%p subscriptions.next=%p .prev=%p",
      fio_io_pid(),
      (void *)ch,
      (void *)ch->subscriptions.next,
      (void *)ch->subscriptions.prev);
  const fio_io_s *skip = ipc->from;
  FIO_LIST_EACH(fio_pubsub_subscription_s, node, &ch->subscriptions, s) {
    /* Skip if publisher is sending to itself (IO is set)*/
    if (skip && s->io == skip)
      continue;
    fio_queue_push(s->queue,
                   fio___pubsub_subscription_ipc_deliver,
                   fio_pubsub_subscription_dup(s),
                   fio_ipc_dup(ipc));
  }
}

/** Called in every process for message delivery */
FIO_SFUNC void fio___pubsub_engine_ipc_publish_deliver(fio_ipc_s *ipc) {
  fio_pubsub_msg_s msg = fio___pubsub_ipc2msg(ipc);
  fio_str_info_s ch_name =
      FIO_STR_INFO3(msg.channel.buf,
                    msg.channel.len,
                    FIO___PUBSUB_CHANNEL_ENCODE_CAPA(msg.filter, 0));
  /* future scheduling - delivered only to the history buffer - stop here */
  if (ipc->timestamp >
      (uint64_t)(fio_io_last_tick() + FIO_PUBSUB_FUTURE_LIMIT_MS))
    return;

  /* find channel match, schedule subscription tasks */
  fio_pubsub_channel_s **ch_ptr = fio___pubsub_channel_map_node2key_ptr(
      fio___pubsub_channel_map_get_ptr(&FIO___PUBSUB_POSTOFFICE.channels,
                                       ch_name));

  if (ch_ptr) {
    FIO_LOG_DDEBUG2("(%d) channel found, delivering to %.*s",
                    fio_io_pid(),
                    (int)ch_ptr[0]->name_len,
                    ch_ptr[0]->name);
    fio___pubsub_engine_ipc_deliver2channel(*ch_ptr, ipc);
  }

  /* Check pattern subscriptions */
  FIO_MAP_EACH(fio___pubsub_channel_map, &FIO___PUBSUB_POSTOFFICE.patterns, i) {
    fio_pubsub_channel_s *pch = i.node->key;
    if (pch->filter == msg.filter &&
        FIO___PUBSUB_POSTOFFICE.match_fn(
            FIO_STR_INFO2(pch->name, pch->name_len),
            ch_name))
      fio___pubsub_engine_ipc_deliver2channel(pch, ipc);
  }

  /* Push to history manager - must be now, to sync history and publishing */
  fio___pubsub_history_ipc_push(ipc);
  return;
}

/** Called when a message is published - local IPC only (master + workers) */
FIO_SFUNC void fio___pubsub_engine_ipc_publish(const fio_pubsub_engine_s *eng,
                                               const fio_pubsub_msg_s *msg) {
  uint32_t header[2] = {(uint32_t)msg->channel.len, (uint32_t)msg->message.len};
  /*
   * fio_ipc_local sends to master + all workers on local machine.
   */
  fio_ipc_local(.call = fio___pubsub_engine_ipc_publish_deliver,
                .timestamp = msg->timestamp,
                .id = msg->id,
                .flags = (uint16_t)msg->filter,
                .data =
                    FIO_IPC_DATA(FIO_BUF_INFO2((char *)header, sizeof(header)),
                                 msg->channel,
                                 FIO_BUF_INFO2((char *)"\0", 1),
                                 msg->message,
                                 FIO_BUF_INFO2((char *)"\0", 1)));
  (void)eng; /* unused */
}

/** Called when a message is published - local IPC only (master + workers) */
FIO_SFUNC void fio___pubsub_engine_cluster_publish(
    const fio_pubsub_engine_s *eng,
    const fio_pubsub_msg_s *msg) {
  uint32_t header[2] = {(uint32_t)msg->channel.len, (uint32_t)msg->message.len};
  /*
   * fio_ipc_local sends to master + all workers on local machine.
   */
  fio_ipc_broadcast(.opcode = FIO___PUBSUB_OPCODE_PUBLISH,
                    .timestamp = msg->timestamp,
                    .id = msg->id,
                    .flags = (uint16_t)msg->filter,
                    .data = FIO_IPC_DATA(
                        FIO_BUF_INFO2((char *)header, sizeof(header)),
                        msg->channel,
                        FIO_BUF_INFO2((char *)"\0", 1),
                        msg->message,
                        FIO_BUF_INFO2((char *)"\0", 1)));
  (void)eng; /* unused */
}

SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_ipc(void) {
  /* just in case someone played nasty with our previously returned pointer */
  FIO___PUBSUB_POSTOFFICE.local_engine = (fio_pubsub_engine_s){
      .detached = fio___pubsub_engine_ipc_noop,
      .subscribe = fio___pubsub_engine_ipc_sub_noop,
      .psubscribe = fio___pubsub_engine_ipc_sub_noop,
      .unsubscribe = fio___pubsub_engine_ipc_sub_noop,
      .punsubscribe = fio___pubsub_engine_ipc_sub_noop,
      .publish = fio___pubsub_engine_ipc_publish,
  };
  return &FIO___PUBSUB_POSTOFFICE.local_engine;
}

/** Returns the builtin engine for multi-machine cluster publishing (RPC). */
SFUNC fio_pubsub_engine_s const *fio_pubsub_engine_cluster(void) {
  /* just in case someone played nasty with our previously returned pointer */
  FIO___PUBSUB_POSTOFFICE.cluster_engine = (fio_pubsub_engine_s){
      .detached = fio___pubsub_engine_ipc_noop,
      .subscribe = fio___pubsub_engine_ipc_sub_noop,
      .psubscribe = fio___pubsub_engine_ipc_sub_noop,
      .unsubscribe = fio___pubsub_engine_ipc_sub_noop,
      .punsubscribe = fio___pubsub_engine_ipc_sub_noop,
      .publish = fio___pubsub_engine_cluster_publish,
  };
  return &FIO___PUBSUB_POSTOFFICE.cluster_engine;
}

/* *****************************************************************************
Engine Attach/Detach
***************************************************************************** */

FIO_SFUNC void fio___pubsub_attach_task(void *e, void *_ignr) {
  (void)_ignr;
  fio_pubsub_engine_s *engine = (fio_pubsub_engine_s *)e;
  fio___pubsub_engines_set(&FIO___PUBSUB_POSTOFFICE.engines, engine, 1);

  /* Notify engine of all existing channels */
  FIO_MAP_EACH(fio___pubsub_channel_map, &FIO___PUBSUB_POSTOFFICE.channels, i) {
    fio_buf_info_s name = FIO_BUF_INFO2(i.key.buf, i.key.len);
    engine->subscribe(engine, name, (int16_t)(i.key.capa & 0xFFFF));
  }
  FIO_MAP_EACH(fio___pubsub_channel_map, &FIO___PUBSUB_POSTOFFICE.patterns, i) {
    fio_buf_info_s name = FIO_BUF_INFO2(i.key.buf, i.key.len);
    engine->psubscribe(engine, name, (int16_t)(i.key.capa & 0xFFFF));
  }
}

FIO_SFUNC void fio___pubsub_detach_task(void *e, void *_ignr) {
  (void)_ignr;
  fio_pubsub_engine_s *engine = (fio_pubsub_engine_s *)e;
  fio___pubsub_engines_remove(&FIO___PUBSUB_POSTOFFICE.engines, engine);
  if (engine->detached)
    engine->detached(engine);
}

SFUNC void fio_pubsub_engine_attach(fio_pubsub_engine_s *engine) {
  if (!engine)
    return;
  fio_pubsub_engine_s *defaults =
      (fio_pubsub_engine_s *)fio_pubsub_engine_ipc();

  /* Fill missing callbacks with defaults */
  for (size_t i = 0; i < sizeof(fio_pubsub_engine_s) / (sizeof(void *)); ++i) {
    if (!((void **)engine)[i])
      ((void **)engine)[i] = ((void **)defaults)[i];
  }
  if (fio_io_is_running())
    fio_io_defer(fio___pubsub_attach_task, engine, NULL);
  else
    fio___pubsub_attach_task(engine, NULL);
}

SFUNC void fio_pubsub_engine_detach(fio_pubsub_engine_s *engine) {
  if (!engine)
    return;
  if (fio_io_is_running())
    fio_io_defer(fio___pubsub_detach_task, engine, NULL);
  else
    fio___pubsub_detach_task(engine, NULL);
}
/* *****************************************************************************
Publish Implementation
***************************************************************************** */

void fio_pubsub_publish___(void); /* IDE Marker */
SFUNC void fio_pubsub_publish FIO_NOOP(fio_pubsub_publish_args_s args) {
  if (!args.engine)
    args.engine = FIO___PUBSUB_POSTOFFICE.default_engine;

  fio_pubsub_msg_s msg = {
      .io = args.from,
      .id = args.id ? args.id : fio_rand64(),
      .timestamp = args.timestamp ? args.timestamp : fio_io_last_tick(),
      .channel = args.channel,
      .message = args.message,
      .filter = args.filter,
  };
  args.engine->publish(args.engine, &msg);
}

/* *****************************************************************************
Unsubscribe Implementation
***************************************************************************** */

/** Unsubscribes a node (deferred task). */
FIO_SFUNC void fio___pubsub_unsubscribe_task(void *sub_, void *ignr_) {
  fio_pubsub_subscription_s *sub = (fio_pubsub_subscription_s *)sub_;

  FIO_LIST_REMOVE(&sub->node);
  sub->node = FIO_LIST_INIT(sub->node);
  sub->on_message = fio___pubsub_on_message_stub;

  FIO_LOG_DDEBUG2("(%d) unsubscribed performed for %p -> %.*s (ref: %zu)",
                  fio_io_pid(),
                  (void *)sub,
                  (int)sub->channel->name_len,
                  sub->channel->name,
                  fio_pubsub_subscription_references(sub));

  fio_pubsub_subscription_free2(sub);
  /* fio_pubsub_channel_free(ch) - called by `destroy` when freed */
  (void)ignr_;
}

/** Unsubscribe callback for environment on_close */
FIO_SFUNC void fio___pubsub_subscription_env_unsubscribe(void *sub_) {
  fio_pubsub_subscription_s *sub = (fio_pubsub_subscription_s *)sub_;
  sub->on_message = fio___pubsub_on_message_stub;
  FIO_LOG_DDEBUG2("(%d) unsubscribed called for %p -> %.*s (ref: %zu)",
                  fio_io_pid(),
                  (void *)sub,
                  (int)sub->channel->name_len,
                  sub->channel->name,
                  fio_pubsub_subscription_references(sub));

  fio_queue_push(fio_io_queue(),
                 fio___pubsub_unsubscribe_task,
                 (void *)sub,
                 NULL);
}

int fio_pubsub_unsubscribe___(void); /* IDE Marker */
SFUNC int fio_pubsub_unsubscribe FIO_NOOP(fio_pubsub_subscribe_args_s args) {
  return fio_io_env_remove(
      args.io,
      .type = fio___pubsub_channel_env_type(args.filter, !!args.is_pattern),
      .name = args.channel);
}

/* *****************************************************************************
Subscribe Implementation
***************************************************************************** */
FIO_SFUNC void fio___pubsub_request_history(fio_pubsub_subscription_s *sub);

/* A callback for IO subscriptions - sends raw message data. */
FIO_SFUNC void FIO_ON_MESSAGE_SEND_MESSAGE(fio_pubsub_msg_s *msg) {
  if (!msg || !msg->message.len)
    return;
  fio_ipc_s *ipc = fio___pubsub_msg2ipc(msg);
  fio_io_write2(msg->io,
                .buf = fio_ipc_dup(ipc),
                .len = msg->message.len,
                .offset = (size_t)(msg->message.buf - (char *)ipc),
                .dealloc = (void (*)(void *))fio_ipc_free);
}

/* A callback for IO subscriptions. */
FIO_SFUNC void fio___subscription_call_protocol(fio_pubsub_msg_s *msg) {
  if (!msg->io)
    return;
  fio_io_protocol_s *p = fio_io_protocol(msg->io);
  FIO_ASSERT_DEBUG(p, "every IO object should have a protocol, always");
  p->on_pubsub((fio_pubsub_msg_s *)msg);
}

/** Completes the subscription request (deferred task). */
FIO_SFUNC void fio___pubsub_subscribe_task(void *sub_, void *bstr_) {
  fio_pubsub_subscription_s *sub = (fio_pubsub_subscription_s *)sub_;
  char *bstr = (char *)bstr_;
  fio_str_info_s ch_name = fio_bstr_info(bstr);
  ch_name.capa = FIO___PUBSUB_CHANNEL_ENCODE_CAPA(sub->filter, sub->is_pattern);

  fio_pubsub_channel_s **ch_ptr =
      fio___pubsub_channel_map_node2key_ptr(fio___pubsub_channel_map_set_ptr(
          &FIO___PUBSUB_POSTOFFICE.channels + sub->is_pattern,
          ch_name));
  fio_bstr_free(bstr);

  if (FIO_UNLIKELY(!ch_ptr))
    goto no_channel;
  sub->channel = fio_pubsub_channel_dup(ch_ptr[0]);
  FIO_LIST_PUSH(&(ch_ptr[0]->subscriptions), &sub->node);
  /* Handle history replay if requested */
  if (sub->replay_since) {
    /* request history from master via IPC - works on both master and client */
    if (!sub->is_pattern)
      fio___pubsub_request_history(sub);
    else
      FIO_LOG_ERROR(
          "(%d) (pubsub) cannot request history in pattern subscriptions: %.*s",
          fio_io_pid(),
          (int)(ch_ptr[0]->name_len),
          ch_ptr[0]->name);
  }

  FIO_LOG_DDEBUG2("(%d) subscribed for %p -> %.*s (ref: %zu)",
                  fio_io_pid(),
                  (void *)sub,
                  (int)sub->channel->name_len,
                  sub->channel->name,
                  fio_pubsub_subscription_references(sub));
  fio_pubsub_subscription_free2(sub);
  return;

no_channel:
  fio_pubsub_subscription_free2(sub);
}

void fio_pubsub_subscribe___(void); /* IDE Marker */
SFUNC void fio_pubsub_subscribe FIO_NOOP(fio_pubsub_subscribe_args_s args) {
  fio_pubsub_subscription_s *s = NULL;
  char *bstr = NULL;

  if (args.channel.len > 0xFFFFUL)
    goto sub_error;

  if (args.master_only && !fio_io_is_master())
    goto sub_error;

  s = fio_pubsub_subscription_new2();
  if (!s)
    goto sub_error;

  bstr = args.channel.len
             ? fio_bstr_write(NULL, args.channel.buf, args.channel.len)
             : NULL;
  if (args.channel.len && !bstr)
    goto sub_error_free_sub;

  if (!args.queue || !args.on_message)
    args.queue = fio_io_queue();

  *s = (fio_pubsub_subscription_s){
      .node = FIO_LIST_INIT(s->node),
      .io = args.io,
      .on_message =
          (args.on_message ? args.on_message
                           : (args.io ? fio___subscription_call_protocol
                                      : fio___pubsub_on_message_stub)),
      .on_unsubscribe = (args.on_unsubscribe
                             ? args.on_unsubscribe
                             : (void (*)(void *))fio___pubsub_on_message_stub),
      .udata = args.udata,
      .queue = args.queue,
      .replay_since = args.replay_since,
      .channel = NULL,
      .filter = args.filter,
      .is_pattern = !!args.is_pattern,
      .master_only = args.master_only,
  };

  fio_io_defer(fio___pubsub_subscribe_task,
               fio_pubsub_subscription_dup(s),
               bstr);

  if (args.subscription_handle_ptr) {
    *args.subscription_handle_ptr = (uintptr_t)s;
    return;
  }

  /* Store subscription in environment for later retrieval */
  fio_io_env_set(
      args.io,
      .type = fio___pubsub_channel_env_type(args.filter, args.is_pattern),
      .name = args.channel,
      .udata = s,
      .on_close = fio___pubsub_subscription_env_unsubscribe);
  return;

sub_error:
  FIO_LOG_ERROR("(%d) pub/sub subscription/channel cannot be created?",
                fio_io_pid());
  if (args.on_unsubscribe)
    args.on_unsubscribe(args.udata);
  return;
sub_error_free_sub:
  FIO_LOG_ERROR("(%d) pub/sub subscription/channel cannot be created?",
                fio_io_pid());
  fio_pubsub_subscription_free(s);
}

/* *****************************************************************************
Subscription Cleanup
***************************************************************************** */

FIO_SFUNC void fio___pubsub_subscription_on_destroy_task(void *tsk,
                                                         void *udata) {
  union {
    void *tsk;
    void (*cb)(void *udata);
  } u = {.tsk = tsk};
  u.cb(udata);
}

FIO_SFUNC void fio___pubsub_subscription_on_destroy(
    fio_pubsub_subscription_s *s) {

  FIO_LOG_DDEBUG2("(%d) subscription destroyed for %p -> %.*s",
                  fio_io_pid(),
                  (void *)s,
                  (int)s->channel->name_len,
                  s->channel->name);

  if (s->node.next != &s->node)
    FIO_LIST_REMOVE(&(s->node));

  union {
    void *tsk;
    void (*cb)(void *udata);
  } u = {.cb = s->on_unsubscribe};
  fio_queue_push(s->queue,
                 fio___pubsub_subscription_on_destroy_task,
                 u.tsk,
                 s->udata);

  fio_pubsub_channel_s *c = s->channel;
  /* no more subscribers - remove channel from channel collection */
  if (!FIO_LIST_IS_EMPTY(&c->subscriptions))
    fio___pubsub_channel_map_remove(
        (&FIO___PUBSUB_POSTOFFICE.channels + c->is_pattern),
        FIO_STR_INFO3(
            c->name,
            c->name_len,
            FIO___PUBSUB_CHANNEL_ENCODE_CAPA(c->filter, c->is_pattern)),
        NULL);
  fio_pubsub_channel_free(c);
}
/* *****************************************************************************
History Request Implementation
***************************************************************************** */

/** IPC handler: Worker receives history reply from master */
FIO_SFUNC void fio___pubsub_worker_on_history_reply(fio_ipc_s *ipc) {
  if (ipc->len < sizeof(uint32_t[2])) {
    /* Empty reply or done signal - nothing to deliver */
    return;
  }

  FIO_LOG_DEBUG2("(%d) worker_on_history_reply: channel=%.*s",
                 fio_io_pid(),
                 (int)(fio_buf2u32u(ipc->data)),
                 ipc->data + sizeof(uint32_t[2]));

  fio_queue_push(((fio_pubsub_subscription_s *)ipc->udata)->queue,
                 fio___pubsub_subscription_ipc_deliver,
                 fio_pubsub_subscription_dup(ipc->udata),
                 fio_ipc_dup(ipc));
}

/** IPC handler: Worker receives history reply from master */
FIO_SFUNC void fio___pubsub_worker_on_history_reply_done(fio_ipc_s *ipc) {
  fio_pubsub_subscription_free2(ipc->udata);
}

/** Callback for master history replay - sends messages to requesting worker */
FIO_SFUNC void fio___pubsub_history_replay_ipc_cb(fio_pubsub_msg_s *msg,
                                                  void *udata) {
  fio_ipc_s *ipc = (fio_ipc_s *)udata;
  uint32_t header[2] = {(uint32_t)msg->channel.len, (uint32_t)msg->message.len};

  static const char nul_byte[1] = {0};

  /* Send history message to requesting worker (not done yet)
   * Metadata (timestamp, id, filter) goes in IPC header */
  fio_ipc_reply(
      ipc,
      .timestamp = msg->timestamp,
      .id = msg->id,
      .flags = (uint16_t)msg->filter,
      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)&header, sizeof(header)),
                           FIO_BUF_INFO2(msg->channel.buf, msg->channel.len),
                           FIO_BUF_INFO2((char *)nul_byte, 1),
                           FIO_BUF_INFO2(msg->message.buf, msg->message.len),
                           FIO_BUF_INFO2((char *)nul_byte, 1)),
      .done = 0);
}

FIO_SFUNC void fio___pubsub_history_replay_ipc_done_cb(void *udata) {
  fio_ipc_reply(udata, .done = 1);
}

/** IPC handler: Master receives history request from worker */
FIO_SFUNC void fio___pubsub_master_on_history_request(fio_ipc_s *ipc) {
  if (ipc->len < FIO___PUBSUB_HISTORY_REQUEST_HEADER_LEN) {
    FIO_LOG_ERROR("(%d) master_on_history_request: invalid message size %u",
                  fio_io_pid(),
                  ipc->len);
    /* Send empty done reply */
    fio_ipc_reply(ipc, .done = 1);
    return;
  }

  fio___pubsub_history_request_s *req =
      (fio___pubsub_history_request_s *)ipc->data;
  /* Filter is carried in IPC header flags */
  int16_t filter = (int16_t)ipc->flags;

  FIO_LOG_DEBUG2("(%d) Got history request: channel=%.*s since=%llu",
                 fio_io_pid(),
                 (int)req->channel_len,
                 req->channel,
                 (unsigned long long)req->replay_since);

  fio_buf_info_s channel = FIO_BUF_INFO2(req->channel, req->channel_len);

  /* Find best history manager and replay */
  fio_pubsub_history_s *best = NULL;
  uint64_t best_oldest = UINT64_MAX;

  FIO_IMAP_EACH(fio___pubsub_history_managers,
                &FIO___PUBSUB_POSTOFFICE.history_managers,
                i) {
    fio_pubsub_history_s *mgr =
        FIO___PUBSUB_POSTOFFICE.history_managers.ary[i].manager;
    uint64_t mgr_oldest = mgr->oldest(mgr, channel, filter);
    if (mgr_oldest < best_oldest) {
      best_oldest = mgr_oldest;
      best = mgr;
    }
    if (best_oldest <= req->replay_since)
      break;
  }

  /* Replay from selected manager, sending each message via IPC reply */
  if (best)
    best->replay(best,
                 channel,
                 filter,
                 req->replay_since,
                 fio___pubsub_history_replay_ipc_cb,
                 fio___pubsub_history_replay_ipc_done_cb,
                 ipc);
  else
    fio_ipc_reply(ipc, .done = 1);
}

/** Worker requests history from master via IPC */
FIO_SFUNC void fio___pubsub_request_history(fio_pubsub_subscription_s *sub) {
  if (!sub || !sub->channel || !sub->replay_since)
    return;

  FIO_LOG_DEBUG2("(%d) request_history: channel=%.*s since=%llu",
                 fio_io_pid(),
                 (int)sub->channel->name_len,
                 sub->channel->name,
                 (unsigned long long)sub->replay_since);

  /* Build history request header on stack (filter goes in IPC flags) */
  fio___pubsub_history_request_s header = {
      .replay_since = sub->replay_since,
      .channel_len = sub->channel->name_len,
  };

  /* Send request to master - filter carried in IPC flags */
  fio_ipc_call(.call = fio___pubsub_master_on_history_request,
               .on_reply = fio___pubsub_worker_on_history_reply,
               .on_done = fio___pubsub_worker_on_history_reply_done,
               .udata = fio_pubsub_subscription_dup(sub),
               .flags = (uint16_t)sub->channel->filter,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)&header,
                                 FIO___PUBSUB_HISTORY_REQUEST_HEADER_LEN),
                   FIO_BUF_INFO2(sub->channel->name, sub->channel->name_len)));
}

/* *****************************************************************************
History Manager - Stubs
***************************************************************************** */

FIO_SFUNC int fio___pubsub_history_stub_push(
    const struct fio_pubsub_history_s *hist,
    fio_pubsub_msg_s *msg) {
  (void)hist;
  (void)msg;
  return 0;
}

FIO_SFUNC int fio___pubsub_history_stub_replay(
    const struct fio_pubsub_history_s *hist,
    fio_buf_info_s channel,
    int16_t filter,
    uint64_t since,
    void (*on_message)(fio_pubsub_msg_s *msg, void *udata),
    void (*on_done)(void *udata),
    void *udata) {
  (void)hist;
  (void)channel;
  (void)filter;
  (void)since;
  (void)on_message;
  (void)on_done;
  (void)udata;
  if (on_done)
    on_done(udata);
  return -1;
}

FIO_SFUNC uint64_t
fio___pubsub_history_stub_oldest(const struct fio_pubsub_history_s *hist,
                                 fio_buf_info_s channel,
                                 int16_t filter) {
  (void)hist;
  (void)channel;
  (void)filter;
  return UINT64_MAX;
}

FIO_SFUNC void fio___pubsub_history_stub_detach(
    const struct fio_pubsub_history_s *hist) {
  (void)hist;
}

/* *****************************************************************************
History Manager - Attach/Detach
***************************************************************************** */

FIO_SFUNC void fio___pubsub_history_managers_sort(void) {
  fio___pubsub_history_managers_s *m =
      &FIO___PUBSUB_POSTOFFICE.history_managers;
  if (!m->w)
    return;
  fio___pubsub_history_entry_sort(m->ary, m->w);
  fio___pubsub_history_managers_rehash(m);
}

FIO_SFUNC void fio___pubsub_history_attach_task(void *manager_,
                                                void *priority_) {
  fio_pubsub_history_s *manager = (fio_pubsub_history_s *)manager_;
  uint8_t priority = (uint8_t)(uintptr_t)priority_;

  fio___pubsub_history_entry_s entry = {.manager = manager,
                                        .priority = priority};
  fio___pubsub_history_entry_s *existing = fio___pubsub_history_managers_get(
      &FIO___PUBSUB_POSTOFFICE.history_managers,
      entry);
  if (existing) {
    existing->priority = priority;
    fio___pubsub_history_managers_sort();
  } else {
    if (fio___pubsub_history_managers_set(
            &FIO___PUBSUB_POSTOFFICE.history_managers,
            entry,
            1))
      fio___pubsub_history_managers_sort();
  }
}

SFUNC int fio_pubsub_history_attach(const fio_pubsub_history_s *manager,
                                    uint8_t priority) {
  if (!manager)
    return -1;
  fio_pubsub_history_s *updated = (fio_pubsub_history_s *)manager;
  if (!updated->push)
    updated->push = fio___pubsub_history_stub_push;
  if (!updated->replay)
    updated->replay = fio___pubsub_history_stub_replay;
  if (!updated->oldest)
    updated->oldest = fio___pubsub_history_stub_oldest;
  if (!updated->detached)
    updated->detached = fio___pubsub_history_stub_detach;

  if (fio_io_is_running())
    fio_io_defer(fio___pubsub_history_attach_task,
                 updated,
                 (void *)(uintptr_t)priority);
  else
    fio___pubsub_history_attach_task(updated, (void *)(uintptr_t)priority);
  return 0;
}

FIO_SFUNC void fio___pubsub_history_detach_task(void *manager_, void *ignr_) {
  fio_pubsub_history_s *manager = (fio_pubsub_history_s *)manager_;
  fio___pubsub_history_entry_s entry = {.manager = manager, .priority = 0};
  fio___pubsub_history_managers_remove(
      &FIO___PUBSUB_POSTOFFICE.history_managers,
      entry);
  /* removes the "hole", optimizing loops */
  fio___pubsub_history_managers_rehash(
      &FIO___PUBSUB_POSTOFFICE.history_managers);
  manager->detached(manager);
  (void)ignr_;
}

SFUNC void fio_pubsub_history_detach(const fio_pubsub_history_s *manager) {
  if (!manager)
    return;
  if (fio_io_is_running())
    fio_io_defer(fio___pubsub_history_detach_task, (void *)manager, NULL);
  else
    fio___pubsub_history_detach_task((void *)manager, NULL);
}

/* *****************************************************************************
Memory / Cache History Manager
***************************************************************************** */

/** Cleanup callback - called when history manager is detached */
FIO_SFUNC void fio___pubsub_history_cache_detached(
    const struct fio_pubsub_history_s *hist) {
  fio___pubsub_history_cache_destroy(&FIO___PUBSUB_POSTOFFICE.cache);
  (void)hist;
}
/**
 * Store a message in history.
 *
 * Returns 0 on success, -1 on error.
 * Called when a message is published (master only).
 */
FIO_SFUNC int fio___pubsub_history_cache_push(
    const struct fio_pubsub_history_s *hist,
    fio_pubsub_msg_s *msg) {
  fio___pubsub_history_channel_s *cache =
      fio___pubsub_history_cache_channel_new(&FIO___PUBSUB_POSTOFFICE.cache,
                                             msg->channel,
                                             msg->filter);
  if (!cache)
    return -1;
  fio_ipc_s *ipc = fio___pubsub_msg2ipc(msg);
  fio_ipc_detach(ipc);
  if (fio___pubsub_history_channel_push(cache, ipc))
    return 0; /* exists */
  FIO___PUBSUB_POSTOFFICE.cache.size +=
      fio___pubsub_history_cache_ipc_size(ipc);
  for (; FIO___PUBSUB_POSTOFFICE.cache.size >
         FIO___PUBSUB_POSTOFFICE.cache.limit;) {
    /* TODO pop and size update until size is under limit */
    FIO_MAP_EACH(fio___pubsub_history_map,
                 &FIO___PUBSUB_POSTOFFICE.cache.map,
                 pos) {
      if (FIO___PUBSUB_POSTOFFICE.cache.size <=
          FIO___PUBSUB_POSTOFFICE.cache.limit)
        goto done;
      FIO___PUBSUB_POSTOFFICE.cache.size -=
          fio___pubsub_history_channel_pop(&pos.node->value);
    }
    FIO_MAP_EACH(fio___pubsub_history_map,
                 &FIO___PUBSUB_POSTOFFICE.cache.map,
                 pos) {
      fio___pubsub_history_channel_rehash(&pos.node->value);
    }
  }
done:
  if (msg->timestamp >
      (uint64_t)(fio_io_last_tick() + FIO_PUBSUB_FUTURE_LIMIT_MS)) {
    /* TODO: add timer for publishing the message using fio_pubsub_publish */
  }
  return 0;
  (void)hist;
}

/**
 * Replay messages since timestamp by calling callback for each.
 *
 * Returns 0 if replay was handled, -1 if this manager cannot replay.
 */
FIO_SFUNC int fio___pubsub_history_cache_replay(
    const struct fio_pubsub_history_s *hist,
    fio_buf_info_s channel,
    int16_t filter,
    uint64_t since,
    void (*on_message)(fio_pubsub_msg_s *msg, void *udata),
    void (*on_done)(void *udata),
    void *udata) {
  (void)hist;
  fio___pubsub_history_channel_s *cache =
      fio___pubsub_history_cache_channel(&FIO___PUBSUB_POSTOFFICE.cache,
                                         channel,
                                         filter);
  if (!cache || !cache->count)
    goto done;
  size_t index = 0;
  uint64_t now = fio_io_last_tick();
  if (since > cache->ary[cache->w - 1]->timestamp)
    goto done;
  if (since <= cache->ary[index]->timestamp)
    goto loop;
  /* find best position in array (mini binary search) */
  for (size_t step = cache->w; (step >>= 1);) {
    if (!cache->ary[index]) {
      --index;
      continue;
    }
    if (since > cache->ary[index]->timestamp)
      index += step;
    else if (since < cache->ary[index]->timestamp)
      index -= step;
    else
      goto loop;
  }
  index -= (since < cache->ary[index]->timestamp);

loop:
  for (; index < cache->w && cache->ary[index]->timestamp <= now; ++index) {
    fio_pubsub_msg_s msg = fio___pubsub_ipc2msg(cache->ary[index]);
    on_message(&msg, udata);
  }
done:
  if (on_done)
    on_done(udata);
  return 0;
}

/**
 * Get oldest available timestamp for a channel.
 *
 * Returns UINT64_MAX if no history available.
 */
FIO_SFUNC uint64_t
fio___pubsub_history_cache_oldest(const struct fio_pubsub_history_s *hist,
                                  fio_buf_info_s channel,
                                  int16_t filter) {
  fio___pubsub_history_channel_s *cache =
      fio___pubsub_history_cache_channel(&FIO___PUBSUB_POSTOFFICE.cache,
                                         channel,
                                         filter);
  if (!cache)
    return UINT64_MAX;
  if (!cache->count)
    return UINT64_MAX;
  return cache->ary[0]->timestamp;
  (void)hist;
}

SFUNC const fio_pubsub_history_s *fio_pubsub_history_cache(size_t size_limit) {
  if (!size_limit && !FIO___PUBSUB_POSTOFFICE.cache.limit) {
    size_t mul = 1024 * 1024;
    int64_t tmp;
    char *env = getenv("WEBSITE_MEMORY_LIMIT_MB");
    if (!env) {
      mul = 1024;
      env = getenv("WEBSITE_MEMORY_LIMIT_KB");
    }
    if (!env) {
      mul = 1;
      env = getenv("WEBSITE_MEMORY_LIMIT");
    }
    if (env && (tmp = fio_atol(&env)) > 0)
      size_limit = mul * tmp;
    if (!size_limit)
      size_limit = FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT;
    FIO___PUBSUB_POSTOFFICE.cache.limit = size_limit;
  }
  FIO___PUBSUB_POSTOFFICE.cache_manager = (fio_pubsub_history_s){
      .detached = fio___pubsub_history_cache_detached,
      .push = fio___pubsub_history_cache_push,
      .replay = fio___pubsub_history_cache_replay,
      .oldest = fio___pubsub_history_cache_oldest,
  };
  return &FIO___PUBSUB_POSTOFFICE.cache_manager;
}

/* *****************************************************************************
Done.
***************************************************************************** */

#undef FIO___RECURSIVE_INCLUDE
#endif /* FIO_EXTERN_COMPLETE */

/* *****************************************************************************
Pub/Sub2 - Cleanup
***************************************************************************** */

#endif /* FIO_PUBSUB */
#undef FIO_PUBSUB
