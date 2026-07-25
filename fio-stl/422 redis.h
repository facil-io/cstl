/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_REDIS              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          Redis Pub/Sub Engine Module




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_REDIS) && !defined(FIO___RECURSIVE_INCLUDE) &&                 \
    !defined(H___FIO_REDIS___H)
#define H___FIO_REDIS___H
#undef FIO_REDIS

/* *****************************************************************************
Redis Engine - Overview

This module provides a Redis engine that can be used either as:
1. A standalone database client (for GET/SET/INCR/etc. commands)
2. A pub/sub engine when attached to facil.io's pub/sub system

Features:
- RESP3 protocol (REQUIRED: Redis >= 6.0; HELLO 3 handshake, no RESP2
  fallback - handshake failure is a hard engine error)
- Command queue with callbacks for arbitrary Redis commands
- Authentication support (folded into the HELLO 3 handshake)
- Automatic reconnection on connection loss
- Ping/pong keepalive
- Optional pub/sub integration with SUBSCRIBE/PSUBSCRIBE/PUBLISH

Single-Connection RESP3 Architecture:
=====================================
After HELLO 3, SUBSCRIBE no longer commandeers the connection, so commands
and push frames multiplex on ONE connection:
- Outgoing: a lock-step command FIFO (one command in flight) plus
  fire-and-forget SUBSCRIBE-family writes (their confirmations arrive as
  RESP3 push frames, never command replies, so they cannot desynchronize
  the FIFO).
- Incoming: per-frame routing. RESP3 push (`>`) frames (pub/sub deliveries,
  subscribe confirmations) go to the push handler; all other top-level
  frames are command replies matched to the FIFO head.

Multi-Process Architecture:
===========================
Only the MASTER process connects to the Redis server. Worker processes
communicate with the master via IPC (Inter-Process Communication).

  Master:   [Redis connection] → Redis Server
  Worker 1: [IPC connection] → Master → Redis
  Worker 2: [IPC connection] → Master → Redis

This ensures:
- No duplicate Redis connections from workers
- Correct pub/sub semantics (subscribe/unsubscribe only on master)
- Workers transparently forward commands and publishes via IPC

Thread Safety Model:
====================
The Redis engine is thread-safe. All internal state modifications are delegated
to the IO queue using fio_io_defer(), ensuring single-threaded execution of
state changes. This prevents race conditions without requiring locks.

Public API thread safety:
- fio_redis_new():   Thread-safe (defers connection to IO thread)
- fio_redis_dup():   Thread-safe (uses atomic reference counting)
- fio_redis_free():  Thread-safe (releases caller's ref; simply decrements ref)
- fio_redis_send():  Thread-safe (defers command queuing to IO thread)

Internal operations that run on the IO thread:
- Command queue management (add, remove, send)
- Connection state changes (connect, disconnect, reconnect)
- Protocol callbacks (on_attach, on_data, on_close, on_timeout)
- Pub/Sub engine callbacks (subscribe, publish, etc.)

Reference Counting Ownership Model:
====================================
The Redis engine uses reference counting (via FIO_REF) for memory management.
There are THREE completely independent concerns:

1. Reference counting — pure memory lifetime:
   - fio_redis_new():   Creates engine with ref=1 (caller owns this reference)
   - fio_redis_dup():   Increments ref, returns engine
   - fio_redis_free():  One line only: fio___redis_free(r). Nothing else.
   When ref hits 0: FIO_REF_DESTROY → fio___redis_destroy fires.

2. Pub/sub membership — system owns a ref TRANSFERRED at attach:
   - fio_pubsub_engine_attach(engine): the CALLER moves their reference into
     the system. The caller either relinquished their only ref (won't use it
     after), or called fio_redis_dup first to keep a copy. Either way: NO
     extra dup inside the redis module for attach. The ref is transferred.
   - fio___redis_detached callback (fired by fio_pubsub_engine_detach):
     the system releases its ref: fio___redis_free(r). That's it.
   - subscribe/psubscribe: only send SUBSCRIBE/PSUBSCRIBE commands to Redis.
     They do NOT touch ref counts. The ref was already transferred at attach.

3. IO ↔ Engine link — peers, no ownership:
   - fio_io_connect(..., .udata = r): raw pointer, no ownership, no dup.
   - fio___redis_destroy (called when ref hits 0): breaks the link both ways:
     nulls IO's udata, closes IO, nulls conn->io.
   - on_close callback: checks udata. If NULL (engine destroyed): do nothing.
     If non-NULL: null conn->io, schedule reconnect. No fio___redis_free.

Internal reference management:
- Deferred tasks (connect, callbacks) dup before scheduling, free at end.
- Connection on_close callbacks do NOT free (connection holds no ref).
- The on_detached callback releases the pub/sub system's ref.

Typical usage patterns:
  // Pattern A: with pub/sub (caller keeps a dup, transfers original to system)
  fio_pubsub_engine_s *redis = fio_redis_new(...);  // ref=1
  fio_redis_dup(redis);                              // ref=2, caller keeps this
  fio_pubsub_engine_attach(redis);                   // system takes original
ref
  // ... use pub/sub ...
  fio_pubsub_engine_detach(redis);  // system releases its ref → ref=1
  fio_redis_free(redis);            // caller releases dup → ref=0 → destroy

  // Pattern B: never attached (no system ref)
  fio_pubsub_engine_s *redis = fio_redis_new(...);  // ref=1
  fio_redis_free(redis);                             // ref=0 → destroy

Usage 1 - Database Only:
    // Create engine - only the publishing connection is established
    fio_pubsub_engine_s *redis = fio_redis_new(
        .url = "redis://localhost:6379",
        .auth = "password",  // optional
        .ping_interval = 30  // seconds, optional
    );

    // Send database commands
    FIOBJ cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("GET", 3));
    fiobj_array_push(cmd, fiobj_str_new_cstr("mykey", 5));
    fio_redis_send(redis, cmd, my_callback, my_udata);
    fiobj_free(cmd);

    // Cleanup - releases caller's ref
    fio_redis_free(redis);

Usage 2 - With Pub/Sub:
    fio_pubsub_engine_s *redis = fio_redis_new(.url = "localhost:6379");
    fio_redis_dup(redis);              // keep a local ref
    fio_pubsub_engine_attach(redis);   // system takes the original ref

    // ... use pub/sub ...

    // detach releases the system's ref; free releases the caller's dup.
    fio_pubsub_engine_detach(redis);
    fio_redis_free(redis);

Note: When used as a sub-engine for clustering, do NOT attach to pub/sub.
The cluster engine will manage the Redis engine directly.

***************************************************************************** */

/* *****************************************************************************
Redis Engine Settings
***************************************************************************** */

#ifndef FIO_REDIS_READ_BUFFER
/** Size of the read buffer for the Redis connection.
 * NOTE: must fit fio_redis_connection_s.buf_pos (uint32_t). */
#define FIO_REDIS_READ_BUFFER 65536
#endif

/** The read-buffer position type must be able to hold FIO_REDIS_READ_BUFFER. */
#define FIO___REDIS_BUF_POS_T uint32_t
FIO_ASSERT_STATIC(FIO_REDIS_READ_BUFFER <= UINT32_MAX,
                  "FIO_REDIS_READ_BUFFER exceeds the buf_pos type (uint32_t)");

#ifndef FIO_REDIS_MAX_BATCH
/**
 * Maximum number of complete messages processed per `on_data` event.
 *
 * When the cap is reached with more data buffered, processing continues in a
 * deferred task. This keeps any single event-loop callback small - downstream
 * work per message is limited to scheduling (defer / publish), never I/O.
 */
#define FIO_REDIS_MAX_BATCH 128
#endif

/* *****************************************************************************
Redis Engine Types
***************************************************************************** */

/** Arguments for creating a Redis engine */
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
  /** Redis server's password, if any (folded into the HELLO 3 handshake) */
  const char *auth;
  /** Length of auth string (0 = auto-detect with strlen) */
  size_t auth_len;
  /** Ping interval in seconds (0 = default 300 seconds) */
  uint8_t ping_interval;
  /**
   * Cumulative payload budget per top-level Redis message, in bytes.
   *
   * Budget = Σ(all string payload bytes) + 32 × (count of ALL objects -
   * String, Array, Map, Bool, Number, etc.). Checked BEFORE allocating or
   * appending; a breach logs an error and disconnects.
   *
   * 0 = default (16MB). Protects against hostile / corrupt servers declaring
   * huge `$<len>` allocations or oversized replies.
   */
  size_t payload_limit;
} fio_redis_args_s;

/** Redis connection state, observed from the IO thread. */
typedef enum {
  /** The engine stopped after a failed HELLO handshake (or NULL was queried). */
  FIO_REDIS_STATE_ERROR,
  /** No socket is attached, or the RESP3 HELLO handshake is still pending. */
  FIO_REDIS_STATE_CONNECTING,
  /** A socket is attached and its RESP3 HELLO handshake completed. */
  FIO_REDIS_STATE_CONNECTED,
} fio_redis_state_e;

/**
 * Creates a Redis pub/sub engine with reference count = 1.
 *
 * The engine is active only after the IO reactor starts running.
 *
 * The caller owns the returned reference and must call fio_redis_free()
 * when done. Attaching to pub/sub does NOT transfer ownership.
 *
 * Returns a pointer to the engine or NULL on error.
 */
SFUNC fio_pubsub_engine_s *fio_redis_new(fio_redis_args_s args);

/** Creates a Redis pub/sub engine (named arguments helper macro). */
#define fio_redis_new(...) fio_redis_new((fio_redis_args_s){__VA_ARGS__})

/**
 * Increments the reference count and returns the engine.
 *
 * Use this when you need to share the engine across multiple owners.
 * Each call to fio_redis_dup() must be balanced with fio_redis_free().
 */
SFUNC fio_pubsub_engine_s *fio_redis_dup(fio_pubsub_engine_s *engine);

/**
 * Releases the caller's reference to the engine.
 *
 * This function simply decrements the reference count. If the ref reaches 0
 * (no other refs held), fio___redis_destroy() fires immediately:
 * sets running=0, closes connections, drains the command queue, frees memory.
 *
 * The engine stays alive as long as any other ref is held (e.g. the pub/sub
 * system's ref taken via subscribe/psubscribe). The pub/sub system releases
 * its ref via the on_detached callback when fio_pubsub_engine_detach() fires.
 *
 * The caller does NOT need to call fio_pubsub_engine_detach() before freeing.
 * Calling fio_pubsub_engine_detach() before fio_redis_free() is also safe
 * (detach releases the system ref; free releases the caller ref).
 *
 * Safe to call with NULL (no-op).
 */
SFUNC void fio_redis_free(fio_pubsub_engine_s *engine);

/**
 * Returns the current Redis connection state.
 *
 * This function must be called from the IO thread. Commands can be sent in
 * every state: they queue until the RESP3 HELLO handshake completes.
 */
SFUNC fio_redis_state_e
fio_redis_state(fio_pubsub_engine_s const *engine);

/**
 * Sends a Redis command through the engine's connection.
 *
 * The response will be sent back using the optional callback. `udata` is passed
 * along untouched.
 *
 * The `command` should be a FIOBJ array containing the command and arguments.
 *
 * Note: NEVER call Pub/Sub commands (SUBSCRIBE, PSUBSCRIBE, UNSUBSCRIBE,
 * PUNSUBSCRIBE) using this function, as it will violate the Redis connection's
 * protocol.
 *
 * Returns 0 on success, -1 on error.
 */
SFUNC int fio_redis_send(fio_pubsub_engine_s *engine,
                         FIOBJ command,
                         void (*callback)(fio_pubsub_engine_s *e,
                                          FIOBJ reply,
                                          void *udata),
                         void *udata);

/* *****************************************************************************




Redis Engine Implementation




***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
/* we recursively include here */
#define FIO___RECURSIVE_INCLUDE 1

/* *****************************************************************************
Internal Types
***************************************************************************** */

/** Default cumulative payload budget per top-level message (16MB). */
#define FIO___REDIS_DEFAULT_PAYLOAD_LIMIT (16UL << 20)

/** Per-object budget overhead (every RESP object costs 32 bytes of budget). */
#define FIO___REDIS_PAYLOAD_OBJECT_COST 32

/**
 * Sentinel string context: returned by on_start_string when the payload
 * budget is exceeded. Non-NULL (so the parser enters the streaming path),
 * but the first on_string_write fails (-1), aborting the parse BEFORE any
 * large allocation happens. The sticky limit_exceeded flag is set at the
 * same time, so the connection is closed after the parse call regardless.
 */
#define FIO___REDIS_PS_SENTINEL ((void *)(uintptr_t)1)

/**
 * Marker object for zero-copy push-frame capture: used as the container
 * context and element value while a push frame is being captured (never a
 * valid FIOBJ - must never be freed or dereferenced).
 */
#define FIO___REDIS_CAPTURE_MARK ((void *)(uintptr_t)2)

/** Push frame modes (classified from element 0). */
#define FIO___REDIS_PUSH_UNDECIDED 0
#define FIO___REDIS_PUSH_MESSAGE   1
#define FIO___REDIS_PUSH_PMESSAGE  2
#define FIO___REDIS_PUSH_OTHER     3

/** In-progress string capture (one at a time, per parser design). */
typedef struct {
  /** Declared string length. */
  size_t declared;
  /** Bytes received so far. */
  size_t received;
  /** Read-buffer pointer (zero-copy, complete in a single write). */
  const uint8_t *ptr;
  /** Exact temp assembly buffer (NULL when zero-copy). */
  uint8_t *temp;
} fio___redis_capture_string_s;

/** Completed channel slot (zero-copy read-buffer ptr or owned temp). */
typedef struct {
  const uint8_t *ptr;
  size_t len;
} fio___redis_capture_slot_s;

/** Test hook for observing publish calls (channel, message buf_infos). */
typedef void (*fio___redis_publish_hook_f)(void *udata,
                                           fio_buf_info_s channel,
                                           fio_buf_info_s message);

/**
 * Per-connection payload budget accounting (parser.udata points to this).
 *
 * One cumulative budget per top-level RESP message:
 *   total = Σ(all string payload bytes) + 32 × (all objects)
 * Checked BEFORE allocating or appending; breach is sticky (limit_exceeded)
 * and closes the connection. Reset when a top-level object completes and on
 * connection reset.
 */
typedef struct {
  /** Cumulative charge for the current top-level message. */
  size_t msg_total;
  /** The budget (copied from the engine; never changes after creation). */
  size_t payload_limit;
  /** Sticky flag: budget breached -> close connection after the parse call. */
  uint8_t limit_exceeded;
  /** Non-zero if the in-progress string's bytes were fully charged at start
   * (known `$<len>`); zero for `$?` chunked strings (charged per chunk). */
  uint8_t str_precharged;
  /** Non-zero when the current top-level frame is a RESP3 push (`>`) frame.
   * Set by on_push (top-level only), read + reset by the parse loop. */
  uint8_t is_push;
  /** Zero-copy push-frame capture state (active only inside push frames). */
  uint8_t capture;
  /** Push frame mode: FIO___REDIS_PUSH_{UNDECIDED,MESSAGE,PMESSAGE,OTHER}. */
  uint8_t push_mode;
  /** cap_channel points to an owned temp buffer (freeze/temp assembly). */
  uint8_t cap_channel_owned;
  /** Completed element count of the current push frame. */
  uint32_t elem_index;
  /** In-progress string capture. */
  fio___redis_capture_string_s cur;
  /** Completed channel slot. */
  fio___redis_capture_slot_s cap_channel;
  /** Test-only publish hook (NULL in production -> real fio_pubsub_publish). */
  fio___redis_publish_hook_f publish_hook;
  void *publish_hook_udata;
} fio___redis_parse_state_s;

/** Command queue node */
typedef struct fio_redis_cmd_s {
  FIO_LIST_NODE node;
  void (*callback)(fio_pubsub_engine_s *e, FIOBJ reply, void *udata);
  void *udata;
  size_t cmd_len;
  uint8_t cmd[];
} fio_redis_cmd_s;

/** Internal connection state */
typedef struct fio_redis_connection_s {
  fio_io_s *io;
  fio_resp3_parser_s parser;
  fio___redis_parse_state_s ps;  /* payload budget accounting (parser.udata) */
  FIO___REDIS_BUF_POS_T buf_pos; /* Position in read buffer */
} fio_redis_connection_s;

/**
 * Redis engine structure.
 *
 * Reference counting ownership model (via FIO_REF):
 * - fio_redis_new():  ref = 1 (caller owns this reference)
 * - fio_redis_dup():  ref += 1 (returns engine)
 * - fio_redis_free(): ref -= 1. If ref==0, fio___redis_destroy() fires.
 *
 * Internal reference management:
 * - fio_io_defer(fio___redis_connect, ...): ref += 1 before, ref -= 1 after
 * - fio_io_defer(fio___redis_perform_callback, ...): ref += 1 before, ref -= 1
 *   after
 * - on_close callbacks: NO ref change (connection holds no ownership ref)
 *
 * Pub/Sub integration:
 * - fio_pubsub_engine_attach(): CALLER transfers their ref to the system
 * - subscribe/psubscribe: only send wire commands, do NOT touch ref counts
 * - on_detached callback: releases the system's ref (fio___redis_free)
 *
 * IO ↔ Engine: peers, no ownership in either direction.
 */
typedef struct fio_redis_engine_s {
  fio_pubsub_engine_s engine;  /* Must be first for casting */
  fio_redis_connection_s conn; /* THE connection (RESP3 multiplexes all) */
  char *address;
  char *port;
  char *hello_cmd; /* Pre-formatted HELLO 3 [AUTH] command */
  size_t hello_cmd_len;
  FIO_LIST_HEAD cmd_queue; /* Command queue - accessed only from IO thread */
  size_t payload_limit;    /* Cumulative per-message payload budget */
  uint8_t *last_channel;   /* Last published channel (dedup, owned bytes) */
  size_t last_channel_len; /* Last published channel length */
  uint8_t ping_interval;
  volatile uint8_t pub_sent; /* Flag: command sent, awaiting reply */
  volatile uint8_t running;  /* Flag: engine is active (for reconnection) */
  volatile uint8_t attached; /* Flag: attached to pub/sub system */
  uint8_t buf[FIO_REDIS_READ_BUFFER]; /* Read buffer for the connection */
} fio_redis_engine_s;

/* Forward declarations */
FIO_SFUNC void fio___redis_destroy(fio_redis_engine_s *r);
FIO_SFUNC void fio___redis_on_fork(void *engine_);

/* Reference counting using FIO_REF (with FIO_REF_CONSTRUCTOR_ONLY) - generates:
 * - fio___redis_new(flex_size) - allocate with ref=1
 * - fio___redis_dup(ptr) - increment ref, return ptr
 * - fio___redis_free(ptr) - decrement ref, call destroy when 0
 * - fio___redis_references(ptr) - get current ref count
 */
#define FIO_REF_NAME             fio___redis
#define FIO_REF_TYPE             fio_redis_engine_s
#define FIO_REF_DESTROY(r)       fio___redis_destroy(&(r))
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_FLEX_TYPE        char
#include FIO_INCLUDE_FILE

FIO_LEAK_COUNTER_DEF(fio___redis_engine)
FIO_LEAK_COUNTER_DEF(fio___redis_cmd)

/* *****************************************************************************
RESP3 Callbacks for FIOBJ Building
***************************************************************************** */

/**
 * Charges `bytes` against the connection's payload budget.
 *
 * Returns non-zero on breach (and sets the sticky limit_exceeded flag).
 * Overflow-safe: msg_total <= payload_limit is an invariant, because bytes
 * are only added when they fit the remaining budget.
 *
 * No-op (always 0) when udata is NULL - the IPC reply re-parse path uses a
 * NULL udata; those bytes came from the master over trusted local IPC and
 * were already budget-checked when the master parsed them from Redis.
 */
FIO_SFUNC int fio___redis_ps_charge(void *udata, size_t bytes) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  if (!ps || ps->limit_exceeded)
    return ps ? (int)ps->limit_exceeded : 0;
  if (bytes > ps->payload_limit - ps->msg_total) {
    ps->limit_exceeded = 1;
    FIO_LOG_ERROR("(redis) payload limit exceeded (%zu + %zu > %zu)",
                  ps->msg_total,
                  bytes,
                  ps->payload_limit);
    return 1;
  }
  ps->msg_total += bytes;
  return 0;
}

/** Charges the per-object overhead (32) + `len` string payload bytes. */
FIO_SFUNC int fio___redis_ps_charge_str(void *udata, size_t len) {
  return fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST) ||
         fio___redis_ps_charge(udata, len);
}

/* *****************************************************************************
Zero-Copy Push-Frame Capture

RESP3 push frames have a closed, trivial vocabulary: ["message", channel,
payload], ["pmessage", pattern, channel, payload], subscribe confirmations,
pong, and (opt-in) keyspace notifications. None of them need FIOBJ.

While a top-level push frame is parsed (ps.capture), string data is captured
from read-buffer pointers (zero-copy): small strings are always contiguous
(the parser waits for contiguity below FIO_RESP3_STREAM_THRESHOLD), and any
string that completes in a single write is used in place. A string split
across parse calls is assembled into ONE exact temp buffer (budget-checked
at start), which is freed immediately after the synchronous publish returns.
No read-buffer pointer outlives its parse window: fio___redis_capture_freeze
moves an open channel slot into owned storage before read-buffer compaction,
and fio_pubsub_publish is synchronous (the IPC engine copies what it
retains).
***************************************************************************** */

/**
 * Begins capture of a fixed-length push-frame string.
 * Budget check BEFORE any allocation; oversize returns the error sentinel.
 * `$?` chunked strings are unsupported in push frames (Redis never emits
 * chunked encoding) - treated as a protocol violation: sticky breach.
 */
FIO_SFUNC void *fio___redis_capture_start_string(fio___redis_parse_state_s *ps,
                                                 size_t len,
                                                 uint8_t type) {
  (void)type;
  if (len == (size_t)-1) {
    FIO_LOG_ERROR("(redis) unsupported chunked ($?) string in push frame");
    ps->limit_exceeded = 1;
    return FIO___REDIS_PS_SENTINEL;
  }
  if (fio___redis_ps_charge_str(ps, len))
    return FIO___REDIS_PS_SENTINEL;
  ps->cur = (fio___redis_capture_string_s){.declared = len};
  return &ps->cur;
}

/**
 * Captures string data. A string completed in a single write is captured
 * from the read buffer (zero-copy); otherwise ONE exact temp buffer is
 * allocated at the first partial write (size was budget-checked at start)
 * and chunks are appended.
 */
FIO_SFUNC int fio___redis_capture_write(fio___redis_parse_state_s *ps,
                                        void *ctx,
                                        const void *data,
                                        size_t len) {
  if (ctx == FIO___REDIS_PS_SENTINEL)
    return -1;
  fio___redis_capture_string_s *cur = &ps->cur;
  if (!len)
    return 0;
  if (cur->received + len > cur->declared)
    return -1; /* overflow - parser accounting error */
  if (!cur->ptr && !cur->temp) {
    if (len == cur->declared) {
      /* Zero-copy: the string completed in a single write. The pointer is
       * safe until on_string_done (same parse call): the parser can only
       * suspend between here and done at pos == end (CRLF pending), where
       * the consumer has no unconsumed tail to compact. */
      cur->ptr = (const uint8_t *)data;
    } else {
      cur->temp = (uint8_t *)
          FIO_MEM_REALLOC(NULL, 0, cur->declared ? cur->declared : 1, 0);
      if (!cur->temp)
        return -1;
      FIO_MEMCPY(cur->temp, data, len);
    }
  } else if (cur->temp) {
    FIO_MEMCPY(cur->temp + cur->received, data, len);
  } else {
    return -1; /* zero-copy slot already complete - impossible extra write */
  }
  cur->received += len;
  return 0;
}

/** Publishes the captured channel + payload (message / deduped pmessage). */
FIO_SFUNC void fio___redis_capture_publish(fio___redis_parse_state_s *ps,
                                           const uint8_t *payload,
                                           size_t payload_len) {
  fio_redis_connection_s *conn =
      FIO_PTR_FROM_FIELD(fio_redis_connection_s, ps, ps);
  fio_redis_engine_s *r = FIO_PTR_FROM_FIELD(fio_redis_engine_s, conn, conn);
  const uint8_t *ch = ps->cap_channel.ptr;
  size_t ch_len = ps->cap_channel.len;
  int dedup_hit = (ch_len == r->last_channel_len &&
                   (!ch_len || !FIO_MEMCMP(r->last_channel, ch, ch_len)));

  if (ps->push_mode == FIO___REDIS_PUSH_MESSAGE) {
    if (ps->publish_hook) {
      ps->publish_hook(ps->publish_hook_udata,
                       FIO_BUF_INFO2((char *)ch, ch_len),
                       FIO_BUF_INFO2((char *)payload, payload_len));
    } else {
      fio_pubsub_publish(.channel = FIO_BUF_INFO2((char *)ch, ch_len),
                         .message = FIO_BUF_INFO2((char *)payload, payload_len),
                         .engine = fio_pubsub_engine_ipc());
    }
    /* last_channel: owned byte buffer, copy on change only */
    if (!dedup_hit) {
      uint8_t *nb = (uint8_t *)FIO_MEM_REALLOC(r->last_channel,
                                               r->last_channel_len,
                                               ch_len ? ch_len : 1,
                                               0);
      if (nb) {
        r->last_channel = nb;
        FIO_MEMCPY(nb, ch, ch_len);
        r->last_channel_len = ch_len;
      }
    }
  } else if (ps->push_mode == FIO___REDIS_PUSH_PMESSAGE && !dedup_hit) {
    if (ps->publish_hook) {
      ps->publish_hook(ps->publish_hook_udata,
                       FIO_BUF_INFO2((char *)ch, ch_len),
                       FIO_BUF_INFO2((char *)payload, payload_len));
    } else {
      fio_pubsub_publish(.channel = FIO_BUF_INFO2((char *)ch, ch_len),
                         .message = FIO_BUF_INFO2((char *)payload, payload_len),
                         .engine = fio_pubsub_engine_ipc());
    }
  }
}

/**
 * Finalizes a captured string: classifies the type token (element 0),
 * assigns the channel/payload slots, and publishes when the payload
 * completes. `temp` (when non-NULL) is the exact assembly buffer of `len`
 * bytes; ownership is transferred to the channel slot or freed.
 */
FIO_SFUNC void fio___redis_capture_string_done(fio___redis_parse_state_s *ps,
                                               const uint8_t *ptr,
                                               size_t len,
                                               uint8_t *temp) {
  if (ps->elem_index == 0) {
    /* Type token: message / pmessage / anything else (confirmations, pong,
     * keyspace notifications, unknown push types - all skipped). */
    if (len == 7 && !FIO_MEMCMP(ptr, "message", 7))
      ps->push_mode = FIO___REDIS_PUSH_MESSAGE;
    else if (len == 8 && !FIO_MEMCMP(ptr, "pmessage", 8))
      ps->push_mode = FIO___REDIS_PUSH_PMESSAGE;
    else
      ps->push_mode = FIO___REDIS_PUSH_OTHER;
    if (temp)
      FIO_MEM_FREE(temp, len ? len : 1);
    return;
  }

  int is_channel =
      (ps->push_mode == FIO___REDIS_PUSH_MESSAGE && ps->elem_index == 1) ||
      (ps->push_mode == FIO___REDIS_PUSH_PMESSAGE && ps->elem_index == 2);
  int is_payload =
      (ps->push_mode == FIO___REDIS_PUSH_MESSAGE && ps->elem_index == 2) ||
      (ps->push_mode == FIO___REDIS_PUSH_PMESSAGE && ps->elem_index == 3);

  if (is_channel) {
    ps->cap_channel.ptr = temp ? temp : ptr;
    ps->cap_channel.len = len;
    ps->cap_channel_owned = (uint8_t)(temp != NULL);
    return; /* temp ownership moved to the channel slot */
  }
  if (is_payload) {
    fio___redis_capture_publish(ps, ptr, len);
    if (ps->cap_channel_owned && ps->cap_channel.ptr)
      FIO_MEM_FREE((void *)ps->cap_channel.ptr,
                   ps->cap_channel.len ? ps->cap_channel.len : 1);
    ps->cap_channel = (fio___redis_capture_slot_s){0};
    ps->cap_channel_owned = 0;
    if (temp)
      FIO_MEM_FREE(temp, len ? len : 1);
    return;
  }
  /* Pattern element / OTHER mode / malformed extra element: discard. */
  if (temp)
    FIO_MEM_FREE(temp, len ? len : 1);
}

/**
 * Moves an open zero-copy channel slot into owned storage. MUST be called
 * before read-buffer compaction while a push frame is incomplete, so no
 * read-buffer pointer outlives its parse window. The bytes were already
 * budget-charged at string start.
 */
FIO_SFUNC void fio___redis_capture_freeze(fio___redis_parse_state_s *ps) {
  if (!ps->capture || !ps->cap_channel.ptr || ps->cap_channel_owned)
    return;
  size_t len = ps->cap_channel.len;
  uint8_t *t = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len ? len : 1, 0);
  if (!t) {
    ps->push_mode = FIO___REDIS_PUSH_OTHER; /* OOM - drop the frame */
    return;
  }
  FIO_MEMCPY(t, ps->cap_channel.ptr, len);
  ps->cap_channel.ptr = t;
  ps->cap_channel_owned = 1;
}

/** Resets all capture state, freeing any owned assembly buffers. */
FIO_SFUNC void fio___redis_capture_reset(fio___redis_parse_state_s *ps) {
  if (ps->cur.temp)
    FIO_MEM_FREE(ps->cur.temp, ps->cur.declared ? ps->cur.declared : 1);
  ps->cur = (fio___redis_capture_string_s){0};
  if (ps->cap_channel_owned && ps->cap_channel.ptr)
    FIO_MEM_FREE((void *)ps->cap_channel.ptr,
                 ps->cap_channel.len ? ps->cap_channel.len : 1);
  ps->cap_channel = (fio___redis_capture_slot_s){0};
  ps->cap_channel_owned = 0;
  ps->capture = 0;
  ps->push_mode = FIO___REDIS_PUSH_UNDECIDED;
  ps->elem_index = 0;
}

FIO_SFUNC void *fio___redis_on_null(void *udata) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  return (void *)fiobj_null();
}

FIO_SFUNC void *fio___redis_on_bool(void *udata, int is_true) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  return (void *)(is_true ? fiobj_true() : fiobj_false());
}

FIO_SFUNC void *fio___redis_on_number(void *udata, int64_t num) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  return (void *)fiobj_num_new((intptr_t)num);
}

FIO_SFUNC void *fio___redis_on_double(void *udata, double num) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  return (void *)fiobj_float_new(num);
}

FIO_SFUNC void *fio___redis_on_bignum(void *udata,
                                      const void *data,
                                      size_t len) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  /* Store big numbers as strings */
  fio___redis_ps_charge_str(udata, len);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  return (void *)fiobj_str_new_cstr((const char *)data, len);
}

FIO_SFUNC void *fio___redis_on_string(void *udata,
                                      const void *data,
                                      size_t len,
                                      uint8_t type) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  (void)type;
  fio___redis_ps_charge_str(udata, len);
  if (ps && ps->capture) {
    /* Simple string in a push frame: capture inline (always contiguous) */
    fio___redis_capture_string_done(ps, (const uint8_t *)data, len, NULL);
    return FIO___REDIS_CAPTURE_MARK;
  }
  return (void *)fiobj_str_new_cstr((const char *)data, len);
}

/**
 * Called when a blob string starts.
 *
 * Budget check BEFORE allocating: known `$<len>` lengths are charged in
 * full (32 + len) so an oversize declaration returns the error sentinel
 * instead of allocating. Allocation is EXACT after a passed check (no caps).
 * `$?` chunked strings charge 32 now and each chunk's bytes on write.
 */
FIO_SFUNC void *fio___redis_on_start_string(void *udata,
                                            size_t len,
                                            uint8_t type) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  if (ps && ps->capture)
    return fio___redis_capture_start_string(ps, len, type);
  (void)type;
  if (len == (size_t)-1) {
    /* Chunked string ($?): length unknown - charge overhead, bytes per chunk */
    if (fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST))
      return FIO___REDIS_PS_SENTINEL;
    if (ps)
      ps->str_precharged = 0;
    return (void *)fiobj_str_new();
  }
  /* Known length: charge overhead + ALL bytes before allocating (exact) */
  if (fio___redis_ps_charge_str(udata, len))
    return FIO___REDIS_PS_SENTINEL;
  if (ps)
    ps->str_precharged = 1;
  if (len > 0)
    return (void *)fiobj_str_new_buf(len);
  return (void *)fiobj_str_new();
}

/**
 * Called with partial string data - append to FIOBJ string.
 *
 * Fails (-1) immediately for sentinel contexts (budget breach at start),
 * aborting the parse. `$?` chunks are budget-checked per chunk BEFORE
 * appending; known-length strings were fully charged at start.
 */
FIO_SFUNC int fio___redis_on_string_write(void *udata,
                                          void *ctx,
                                          const void *data,
                                          size_t len) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  if (ps && ps->capture)
    return fio___redis_capture_write(ps, ctx, data, len);
  if (ctx == FIO___REDIS_PS_SENTINEL)
    return -1;
  if (ps && !ps->str_precharged && fio___redis_ps_charge(udata, len))
    return -1;
  fiobj_str_write((FIOBJ)ctx, (const char *)data, len);
  return 0;
}

/**
 * Called when string is complete - return the FIOBJ string.
 */
FIO_SFUNC void *fio___redis_on_string_done(void *udata,
                                           void *ctx,
                                           uint8_t type) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  (void)type;
  /* Sentinel is unreachable for fixed-length strings (the first write
   * fails first); a `$?` end marker may legally follow a sentinel start. */
  if (ctx == FIO___REDIS_PS_SENTINEL)
    return (void *)fiobj_null();
  if (ps && ps->capture) {
    fio___redis_capture_string_s *cur = &ps->cur;
    fio___redis_capture_string_done(ps,
                                    cur->temp ? cur->temp : cur->ptr,
                                    cur->received,
                                    cur->temp);
    ps->cur = (fio___redis_capture_string_s){0};
    return FIO___REDIS_CAPTURE_MARK;
  }
  return ctx; /* Return the completed FIOBJ string */
}

FIO_SFUNC void *fio___redis_on_error(void *udata,
                                     const void *data,
                                     size_t len,
                                     uint8_t type) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  (void)type;
  /* Store errors as strings - caller can check context */
  fio___redis_ps_charge_str(udata, len);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  FIOBJ err = fiobj_str_new_cstr((const char *)data, len);
  FIO_LOG_WARNING("(redis) error response: %.*s", (int)len, (const char *)data);
  return (void *)err;
}

FIO_SFUNC void *fio___redis_on_array(void *udata,
                                     void *parent_ctx,
                                     int64_t len) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  (void)parent_ctx;
  (void)len;
  fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  return (void *)fiobj_array_new();
}

FIO_SFUNC void *fio___redis_on_map(void *udata, void *parent_ctx, int64_t len) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  (void)parent_ctx;
  (void)len;
  fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST);
  if (ps && ps->capture)
    return FIO___REDIS_CAPTURE_MARK;
  return (void *)fiobj_hash_new();
}

FIO_SFUNC void *fio___redis_on_push(void *udata,
                                    void *parent_ctx,
                                    int64_t len) {
  /* Push frames are out-of-band server messages (pub/sub deliveries,
   * subscribe confirmations, keyspace notifications), never command
   * replies. Top-level push frames enter zero-copy capture: no FIOBJ is
   * built, string data is captured from read-buffer pointers. */
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  (void)len;
  if (ps && !parent_ctx) {
    ps->is_push = 1;
    fio___redis_ps_charge(udata, FIO___REDIS_PAYLOAD_OBJECT_COST);
    ps->capture = 1;
    ps->push_mode = FIO___REDIS_PUSH_UNDECIDED;
    ps->elem_index = 0;
    return FIO___REDIS_CAPTURE_MARK;
  }
  /* Nested push (malformed) or non-budgeted context: build as an array */
  return fio___redis_on_array(udata, parent_ctx, len);
}

FIO_SFUNC int fio___redis_array_push(void *udata, void *ctx, void *value) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  if (ps && ps->capture)
    return 0; /* capture mode: elements are captured / discarded */
  fiobj_array_push((FIOBJ)ctx, (FIOBJ)value);
  return 0;
}

FIO_SFUNC int fio___redis_map_push(void *udata,
                                   void *ctx,
                                   void *key,
                                   void *value) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  if (ps && ps->capture)
    return 0; /* capture mode: elements are captured / discarded */
  fiobj_hash_set((FIOBJ)ctx, (FIOBJ)key, (FIOBJ)value, NULL);
  if (key != value) /* in a set, both key and value are same and owned by set */
    fiobj_free((FIOBJ)key);
  return 0;
}

FIO_SFUNC int fio___redis_push_push(void *udata, void *ctx, void *value) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  if (ps && ps->capture) {
    ++ps->elem_index; /* one more push-frame element completed */
    return 0;
  }
  return fio___redis_array_push(udata, ctx, value);
}

FIO_SFUNC void *fio___redis_push_done(void *udata, void *ctx) {
  fio___redis_parse_state_s *ps = (fio___redis_parse_state_s *)udata;
  if (ps && ps->capture) {
    /* Captured push frame complete: publish already happened at payload
     * completion; the parse loop resets the capture state. Return a valid
     * (freeable) object so the parse loop observes the frame boundary. */
    return (void *)fiobj_null();
  }
  return ctx;
}

FIO_SFUNC void fio___redis_free_unused(void *udata, void *obj) {
  if (obj == FIO___REDIS_CAPTURE_MARK)
    return;
  (void)udata;
  fiobj_free((FIOBJ)obj);
}

FIO_SFUNC void *fio___redis_on_error_protocol(void *udata) {
  (void)udata;
  FIO_LOG_ERROR("(redis) RESP protocol error");
  return NULL;
}

/** RESP3 callbacks for building FIOBJ objects */
static const fio_resp3_callbacks_s FIO___REDIS_RESP3_CALLBACKS = {
    .on_null = fio___redis_on_null,
    .on_bool = fio___redis_on_bool,
    .on_number = fio___redis_on_number,
    .on_double = fio___redis_on_double,
    .on_bignum = fio___redis_on_bignum,
    .on_string = fio___redis_on_string,
    .on_error = fio___redis_on_error,
    .on_array = fio___redis_on_array,
    .on_map = fio___redis_on_map,
    .on_push = fio___redis_on_push,
    .array_push = fio___redis_array_push,
    .push_push = fio___redis_push_push,
    .map_push = fio___redis_map_push,
    .push_done = fio___redis_push_done,
    .free_unused = fio___redis_free_unused,
    .on_error_protocol = fio___redis_on_error_protocol,
    /* Streaming string callbacks for efficient large string handling */
    .on_start_string = fio___redis_on_start_string,
    .on_string_write = fio___redis_on_string_write,
    .on_string_done = fio___redis_on_string_done,
};

/* *****************************************************************************
RESP Formatting Helpers
***************************************************************************** */

/**
 * RESP serialization uses RESP3 type prefixes to preserve types through
 * serialization roundtrips:
 * - NULL  → $-1\r\n (RESP2 null bulk string)
 * - TRUE  → #t\r\n (RESP3 boolean)
 * - FALSE → #f\r\n (RESP3 boolean)
 * - NUMBER → :<number>\r\n (RESP integer)
 * - FLOAT  → ,<float>\r\n (RESP3 double)
 * - HASH   → %<count>\r\n (RESP3 map)
 * - ARRAY  → *<count>\r\n (RESP2 array)
 * - STRING → $<len>\r\n<data>\r\n (RESP2 bulk string)
 *
 * Serialization is a two-pass, allocation-free (per call-site) design:
 * fio___redis_fiobj2resp_len() computes the exact size with a depth guard,
 * then fio___redis_fiobj2resp_write() writes directly into the destination
 * buffer with a raw cursor. This lets hot paths serialize straight into
 * their final allocation with zero temporary FIOBJ strings and zero copies.
 */

/** Serialization depth limit (matches the RESP3 parser's nesting limit). */
#define FIO___REDIS_RESP_MAX_DEPTH FIO_RESP3_MAX_NESTING

/** Returns the decimal digit count for `n` (including the sign). */
FIO_SFUNC size_t fio___redis_digits_i64(int64_t n) {
  size_t d = (n < 0) ? 2 : 1;
  uint64_t u = (n < 0) ? ((uint64_t)0 - (uint64_t)n) : (uint64_t)n;
  while (u > 9) {
    u /= 10;
    ++d;
  }
  return d;
}

/**
 * Returns the exact byte count required to serialize `obj` to RESP,
 * or 0 on error (nesting deeper than FIO___REDIS_RESP_MAX_DEPTH, or size
 * overflow). A valid serialization is never 0 bytes.
 */
FIO_SFUNC size_t fio___redis_fiobj2resp_len(FIOBJ obj, uint32_t depth) {
  if (depth > FIO___REDIS_RESP_MAX_DEPTH)
    return 0;
  switch (FIOBJ_TYPE(obj)) {
  case FIOBJ_T_NULL: return 5;  /* $-1\r\n */
  case FIOBJ_T_TRUE:            /* fallthrough */
  case FIOBJ_T_FALSE: return 4; /* #t\r\n / #f\r\n */
  case FIOBJ_T_NUMBER:          /* :<i>\r\n */
    return 3 + fio___redis_digits_i64(fiobj2i(obj));
  case FIOBJ_T_FLOAT: { /* ,<f>\r\n */
    char tmp[64];
    return 3 + fio_ftoa(tmp, fiobj2f(obj), 10);
  }
  case FIOBJ_T_ARRAY: {
    size_t count = fiobj_array_count(obj);
    size_t len = 3 + fio___redis_digits_i64((int64_t)count);
    for (size_t i = 0; i < count; ++i) {
      size_t sub = fio___redis_fiobj2resp_len(fiobj_array_get(obj, (int32_t)i),
                                              depth + 1);
      if (!sub || (len += sub) < sub)
        return 0; /* error or overflow */
    }
    return len;
  }
  case FIOBJ_T_HASH: {
    size_t len = 3 + fio___redis_digits_i64((int64_t)fiobj_hash_count(obj));
    int err = 0;
    FIO_MAP_EACH(fiobj_hash, obj, pos) {
      if (err)
        continue;
      size_t klen = fio___redis_fiobj2resp_len(pos.key, depth + 1);
      size_t vlen = fio___redis_fiobj2resp_len(pos.value, depth + 1);
      if (!klen || !vlen || (len += klen) < klen || (len += vlen) < vlen)
        err = 1; /* error or overflow */
    }
    return err ? 0 : len;
  }
  case FIOBJ_T_STRING:
  default: { /* $<len>\r\n<data>\r\n */
    fio_str_info_s s = fiobj2cstr(obj);
    return s.len + fio___redis_digits_i64((int64_t)s.len) + 5;
  }
  }
}

/**
 * Serializes `obj` to RESP at `pos`, returning the advanced cursor.
 *
 * The destination MUST have at least fio___redis_fiobj2resp_len(obj, depth)
 * bytes - no bounds checks are performed. Call the length function first
 * (it also validates depth).
 */
FIO_SFUNC uint8_t *fio___redis_fiobj2resp_write(uint8_t *pos,
                                                FIOBJ obj,
                                                uint32_t depth) {
  switch (FIOBJ_TYPE(obj)) {
  case FIOBJ_T_NULL: FIO_MEMCPY(pos, "$-1\r\n", 5); return pos + 5;
  case FIOBJ_T_TRUE: FIO_MEMCPY(pos, "#t\r\n", 4); return pos + 4;
  case FIOBJ_T_FALSE: FIO_MEMCPY(pos, "#f\r\n", 4); return pos + 4;
  case FIOBJ_T_NUMBER:
    *pos++ = ':';
    pos += fio_ltoa((char *)pos, fiobj2i(obj), 10);
    *pos++ = '\r';
    *pos++ = '\n';
    return pos;
  case FIOBJ_T_FLOAT:
    *pos++ = ',';
    pos += fio_ftoa((char *)pos, fiobj2f(obj), 10);
    *pos++ = '\r';
    *pos++ = '\n';
    return pos;
  case FIOBJ_T_ARRAY: {
    size_t count = fiobj_array_count(obj);
    *pos++ = '*';
    pos += fio_ltoa((char *)pos, (int64_t)count, 10);
    *pos++ = '\r';
    *pos++ = '\n';
    for (size_t i = 0; i < count; ++i)
      pos = fio___redis_fiobj2resp_write(pos,
                                         fiobj_array_get(obj, (int32_t)i),
                                         depth + 1);
    return pos;
  }
  case FIOBJ_T_HASH:
    *pos++ = '%';
    pos += fio_ltoa((char *)pos, (int64_t)fiobj_hash_count(obj), 10);
    *pos++ = '\r';
    *pos++ = '\n';
    FIO_MAP_EACH(fiobj_hash, obj, pos_) {
      pos = fio___redis_fiobj2resp_write(pos, pos_.key, depth + 1);
      pos = fio___redis_fiobj2resp_write(pos, pos_.value, depth + 1);
    }
    return pos;
  case FIOBJ_T_STRING:
  default: {
    fio_str_info_s s = fiobj2cstr(obj);
    *pos++ = '$';
    pos += fio_ltoa((char *)pos, (int64_t)s.len, 10);
    *pos++ = '\r';
    *pos++ = '\n';
    FIO_MEMCPY(pos, s.buf, s.len);
    pos += s.len;
    *pos++ = '\r';
    *pos++ = '\n';
    return pos;
  }
  }
}

/** Writes a FIOBJ to RESP format into a FIOBJ string (cold path helper). */
FIO_SFUNC void fio___redis_fiobj2resp(FIOBJ dest, FIOBJ obj) {
  size_t len = fio___redis_fiobj2resp_len(obj, 0);
  if (!len)
    return;
  uint8_t *tmp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len, 0);
  if (!tmp)
    return;
  fio___redis_fiobj2resp_write(tmp, obj, 0);
  fiobj_str_write(dest, (const char *)tmp, len);
  FIO_MEM_FREE(tmp, len);
}

/* *****************************************************************************
Subscribe-Family Command Builder
***************************************************************************** */

/** Worst-case overhead for a subscribe-family command (verb, digits, CRLFs). */
#define FIO___REDIS_SUB_CMD_OVERHEAD 64

/**
 * Writes a subscribe-family command:
 *   *2\r\n$<verb_len>\r\n<verb>\r\n$<channel_len>\r\n<channel>\r\n
 *
 * Returns the number of bytes written (at most
 * FIO___REDIS_SUB_CMD_OVERHEAD + channel.len).
 *
 * Raw text building - the command structure is fixed and known,
 * no dynamic FIOBJ types are needed.
 */
FIO_SFUNC size_t fio___redis_write_sub_cmd(uint8_t *dest,
                                           const char *verb,
                                           size_t verb_len,
                                           fio_buf_info_s channel) {
  uint8_t *pos = dest;
  FIO_MEMCPY(pos, "*2\r\n$", 5);
  pos += 5;
  pos += fio_ltoa((char *)pos, (int64_t)verb_len, 10);
  *pos++ = '\r';
  *pos++ = '\n';
  FIO_MEMCPY(pos, verb, verb_len);
  pos += verb_len;
  FIO_MEMCPY(pos, "\r\n$", 3);
  pos += 3;
  pos += fio_ltoa((char *)pos, (int64_t)channel.len, 10);
  *pos++ = '\r';
  *pos++ = '\n';
  FIO_MEMCPY(pos, channel.buf, channel.len);
  pos += channel.len;
  *pos++ = '\r';
  *pos++ = '\n';
  return (size_t)(pos - dest);
}

/**
 * Sends a subscribe-family command on the subscription connection.
 * Uses a stack buffer for typical channel names, heap for huge ones.
 * MUST be called from the IO thread only.
 */
FIO_SFUNC void fio___redis_sub_send(fio_redis_engine_s *r,
                                    const char *verb,
                                    size_t verb_len,
                                    fio_buf_info_s channel) {
  if (!r->conn.io)
    return;
  uint8_t stack_buf[256];
  uint8_t *buf = stack_buf;
  size_t need = FIO___REDIS_SUB_CMD_OVERHEAD + channel.len;
  if (need > sizeof(stack_buf)) {
    buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, need, 0);
    if (!buf)
      return;
  }
  size_t len = fio___redis_write_sub_cmd(buf, verb, verb_len, channel);
  fio_io_write(r->conn.io, buf, len);
  if (buf != stack_buf)
    FIO_MEM_FREE(buf, need);
}

/* *****************************************************************************
HELLO 3 Handshake Command Builder
***************************************************************************** */

/**
 * Returns the exact byte count of the HELLO 3 handshake command.
 *
 * No auth:  *3\r\n$5\r\nHELLO\r\n$1\r\n3\r\n                     (22 bytes)
 * Auth:     *5\r\n$5\r\nHELLO\r\n$1\r\n3\r\n$4\r\nAUTH\r\n$7\r\ndefault\r\n
 *           $<digits>\r\n<password>\r\n
 */
FIO_SFUNC size_t fio___redis_hello_cmd_len(size_t auth_len) {
  if (!auth_len)
    return 22;
  return 50 + fio___redis_digits_i64((int64_t)auth_len) + auth_len;
}

/**
 * Writes the HELLO 3 handshake command (auth folds in when auth_len > 0).
 * Returns the number of bytes written (== fio___redis_hello_cmd_len).
 * Raw text building - fixed, known structure, no dynamic FIOBJ types.
 */
FIO_SFUNC size_t fio___redis_write_hello_cmd(uint8_t *dest,
                                             const char *auth,
                                             size_t auth_len) {
  uint8_t *pos = dest;
  if (!auth_len) {
    FIO_MEMCPY(pos, "*3\r\n$5\r\nHELLO\r\n$1\r\n3\r\n", 22);
    return 22;
  }
  FIO_MEMCPY(pos,
             "*5\r\n$5\r\nHELLO\r\n$1\r\n3\r\n$4\r\nAUTH\r\n$7\r\ndefault\r\n$",
             46);
  pos += 46;
  pos += fio_ltoa((char *)pos, (int64_t)auth_len, 10);
  *pos++ = '\r';
  *pos++ = '\n';
  FIO_MEMCPY(pos, auth, auth_len);
  pos += auth_len;
  *pos++ = '\r';
  *pos++ = '\n';
  return (size_t)(pos - dest);
}

/* *****************************************************************************
Reference Counting and Cleanup

Reference counting ownership model (via FIO_REF macros):
- fio_redis_new():  ref = 1 (caller's reference)
- fio_redis_dup():  ref += 1
- fio_redis_free(): ref -= 1. If ref==0, fio___redis_destroy() fires.

Internal reference management:
- fio_io_defer(fio___redis_connect, ...): ref += 1 before, ref -= 1 after
- fio_io_defer(fio___redis_perform_callback, ...): ref += 1 before, ref -= 1
  after
- on_close callbacks: NO ref change (connection holds no ownership ref)

Pub/Sub integration:
- fio_pubsub_engine_attach(): CALLER transfers their ref to the system
- subscribe/psubscribe: only send wire commands, do NOT touch ref counts
- on_detached callback: releases the system's ref (fio___redis_free)

IO ↔ Engine: peers, no ownership in either direction.
- fio_io_connect(.udata = r): raw pointer, no dup
- fio___redis_destroy: nulls IO udata, closes IO, nulls conn->io
- on_close: if udata NULL → engine destroyed, do nothing; else reconnect

fio___redis_destroy (called when last ref drops):
- Sets running=0 (stops command queue from accepting new commands)
- Closes the connection (NULL udata first to prevent on_close dup)
- Removes fork callback
- Drains command queue (invokes callbacks with FIOBJ_INVALID)
- Frees last_channel
***************************************************************************** */

FIO_SFUNC void fio___redis_connection_reset(fio_redis_connection_s *conn) {
  conn->buf_pos = 0;
  if (conn->ps.capture) {
    /* Zero-copy push capture in progress: contexts are capture markers /
     * the in-progress capture struct, never FIOBJ objects. */
    fio___redis_capture_reset(&conn->ps);
  } else {
    /* Clean up any partially parsed streaming string (never the budget
     * sentinel, which is not a FIOBJ) */
    if (conn->parser.streaming_string && conn->parser.streaming_string_ctx &&
        conn->parser.streaming_string_ctx != FIO___REDIS_PS_SENTINEL) {
      fiobj_free((FIOBJ)conn->parser.streaming_string_ctx);
    }
    /* Clean up any partially built containers in the parser stack */
    while (conn->parser.depth > 0) {
      fio_resp3_frame_s *f = &conn->parser.stack[conn->parser.depth - 1];
      if (f->key)
        fiobj_free((FIOBJ)f->key);
      if (f->ctx)
        fiobj_free((FIOBJ)f->ctx);
      --conn->parser.depth;
    }
  }
  /* Reset the payload budget for the next message (payload_limit persists) */
  conn->ps.msg_total = 0;
  conn->ps.limit_exceeded = 0;
  conn->ps.str_precharged = 0;
  conn->ps.is_push = 0;
  conn->parser = (fio_resp3_parser_s){.udata = &conn->ps};
  conn->io = NULL;
}

/**
 * Internal: Destroy callback for FIO_REF.
 * Called when reference count reaches 0.
 *
 * This is the ONLY place that sets running=0, closes connections, removes
 * the fork callback, and drains the command queue.
 *
 * Guard: NULL the io pointer BEFORE calling fio_io_close_now to prevent
 * double-close re-entrancy (on_close calls fio___redis_free which would
 * re-enter destroy if io is still set).
 */
FIO_SFUNC void fio___redis_destroy(fio_redis_engine_s *r) {
  FIO_LOG_DEBUG("(redis) destroying engine for %s:%s", r->address, r->port);

  /* Stop reconnection attempts - must be set before closing connections
   * so that on_close callbacks do not schedule a reconnect. */
  r->running = 0;

  /* Close connections safely.
   * 1. NULL the io field first so on_close_internal sees conn->io == NULL
   *    and skips the reconnect path.
   * 2. Set udata to NULL on the io handle so on_close returns early
   *    without calling fio___redis_free (which would underflow the ref count
   *    since we are already in destroy with ref == 0).
   * 3. Then close. */
  fio_io_s *io = r->conn.io;
  r->conn.io = NULL;
  if (io) {
    fio_io_udata_set(io, NULL);
    fio_io_close_now(io);
  }

  /* Remove fork callback to prevent use-after-free */
  fio_state_callback_remove(FIO_CALL_IN_CHILD, fio___redis_on_fork, r);

  /* Free remaining resources - invoke callbacks to release IPC references */
  if (r->last_channel)
    FIO_MEM_FREE(r->last_channel, r->last_channel_len);
  while (!FIO_LIST_IS_EMPTY(&r->cmd_queue)) {
    fio_redis_cmd_s *cmd;
    FIO_LIST_POP(fio_redis_cmd_s, node, cmd, &r->cmd_queue);
    if (cmd->callback)
      cmd->callback((fio_pubsub_engine_s *)r, FIOBJ_INVALID, cmd->udata);
    FIO_MEM_FREE(cmd, sizeof(*cmd) + cmd->cmd_len);
    FIO_LEAK_COUNTER_ON_FREE(fio___redis_cmd);
  }
  FIO_LEAK_COUNTER_ON_FREE(fio___redis_engine);
  /* Note: FIO_REF handles the actual memory deallocation */
}

/* *****************************************************************************
Command Queue Management

NOTE: All command queue operations MUST run on the IO thread to prevent race
conditions. Use fio___redis_attach_cmd_task() via fio_io_defer() for thread-safe
command queuing from any thread.
***************************************************************************** */

/**
 * Internal: Send the next command in the queue if ready.
 * MUST be called from the IO thread only.
 */
FIO_SFUNC void fio___redis_send_next_cmd(fio_redis_engine_s *r) {
  if (!r->pub_sent && !FIO_LIST_IS_EMPTY(&r->cmd_queue) && r->conn.io) {
    r->pub_sent = 1;
    fio_redis_cmd_s *cmd =
        FIO_PTR_FROM_FIELD(fio_redis_cmd_s, node, r->cmd_queue.next);
    fio_io_write(r->conn.io, cmd->cmd, cmd->cmd_len);
  }
}

/**
 * Internal: Deferred task to attach a command to the queue.
 * Runs on the IO thread to ensure thread-safety.
 */
FIO_SFUNC void fio___redis_attach_cmd_task(void *engine_, void *cmd_) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine_;
  fio_redis_cmd_s *cmd = (fio_redis_cmd_s *)cmd_;

  /* Check if engine is still running */
  if (!r->running) {
    /* Engine is shutting down, invoke callback with NULL reply */
    if (cmd->callback)
      cmd->callback((fio_pubsub_engine_s *)r, FIOBJ_INVALID, cmd->udata);
    FIO_MEM_FREE(cmd, sizeof(*cmd) + cmd->cmd_len);
    FIO_LEAK_COUNTER_ON_FREE(fio___redis_cmd);
    fio___redis_free(r);
    return;
  }

  /* Add command to queue and try to send */
  FIO_LIST_PUSH(&r->cmd_queue, &cmd->node);
  fio___redis_send_next_cmd(r);
  fio___redis_free(r); /* Release the reference held for this deferred task */
}

/**
 * Internal: Queue a command for execution.
 * Thread-safe - defers the actual queue manipulation to the IO thread.
 */
FIO_SFUNC void fio___redis_attach_cmd(fio_redis_engine_s *r,
                                      fio_redis_cmd_s *cmd) {
  fio___redis_dup(r); /* Hold reference for deferred task */
  fio_io_defer(fio___redis_attach_cmd_task, r, cmd);
}

/* *****************************************************************************
Callback Task for Command Replies
***************************************************************************** */

FIO_SFUNC void fio___redis_perform_callback(void *engine_, void *cmd_) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine_;
  fio_redis_cmd_s *cmd = (fio_redis_cmd_s *)cmd_;
  FIOBJ reply = (FIOBJ)cmd->node.next;
  if (cmd->callback)
    cmd->callback((fio_pubsub_engine_s *)engine_, reply, cmd->udata);
  fiobj_free(reply);
  FIO_MEM_FREE(cmd, sizeof(*cmd) + cmd->cmd_len);
  FIO_LEAK_COUNTER_ON_FREE(fio___redis_cmd);
  fio___redis_free(r); /* Release the reference held for this deferred task */
}

/* *****************************************************************************
Message Handlers

NOTE: These handlers are called from the IO thread via protocol callbacks
(on_data), so they don't need locks for accessing engine state.
***************************************************************************** */

/**
 * Handle reply on publishing connection.
 * Called from IO thread via on_data callback - no lock needed.
 */
FIO_SFUNC void fio___redis_on_pub_message(fio_redis_engine_s *r, FIOBJ msg) {
  /* Dequeue the command that this reply is for */
  FIO_LIST_NODE *node = r->cmd_queue.next;
  if (node != &r->cmd_queue) {
    FIO_LIST_REMOVE(node);
  } else {
    node = NULL;
  }
  r->pub_sent = 0;
  fio___redis_send_next_cmd(r);

  if (!node) {
    FIO_LOG_WARNING("(redis) received reply with no pending command");
    return;
  }

  fio_redis_cmd_s *cmd = FIO_PTR_FROM_FIELD(fio_redis_cmd_s, node, node);
  cmd->node.next = (FIO_LIST_NODE *)fiobj_dup(msg);
  fio___redis_dup(r); /* Hold reference for deferred callback */
  fio_io_defer(fio___redis_perform_callback, r, cmd);
}

/* *****************************************************************************
Protocol Callbacks
***************************************************************************** */

FIO_SFUNC void fio___redis_on_attach(fio_io_s *io);
FIO_SFUNC void fio___redis_on_data(fio_io_s *io);
FIO_SFUNC void fio___redis_on_close(void *buffer, void *udata);
FIO_SFUNC void fio___redis_on_timeout(fio_io_s *io);
FIO_SFUNC void fio___redis_connect(void *engine_, void *conn_);
FIO_SFUNC void fio___redis_subscribe(const fio_pubsub_engine_s *eng,
                                     fio_buf_info_s channel,
                                     int16_t filter);
FIO_SFUNC void fio___redis_psubscribe(const fio_pubsub_engine_s *eng,
                                      fio_buf_info_s channel,
                                      int16_t filter);

/** Protocol for the (single) Redis connection */
static fio_io_protocol_s FIO___REDIS_PROTOCOL = {
    .on_attach = fio___redis_on_attach,
    .on_data = fio___redis_on_data,
    .on_close = fio___redis_on_close,
    .on_timeout = fio___redis_on_timeout,
    .timeout = 300000, /* 5 minutes default */
};

/**
 * HELLO 3 handshake reply callback.
 *
 * A successful HELLO returns a map (hash) reply. Anything else (an `-ERR`
 * for servers without RESP3, i.e. Redis < 6.0, or an auth failure) is a
 * HARD engine error: no RESP2 fallback. The engine stops and the connection
 * is closed; queued commands are failed at engine destroy.
 */
FIO_SFUNC void fio___redis_on_hello_reply(fio_pubsub_engine_s *e,
                                          FIOBJ reply,
                                          void *udata) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)e;
  (void)udata;
  if (reply == FIOBJ_INVALID || FIOBJ_TYPE(reply) != FIOBJ_T_HASH) {
    FIO_LOG_ERROR(
        "(redis) HELLO 3 handshake failed (RESP3 requires Redis >= 6.0; "
        "check auth) - engine disabled");
    r->running = 0;
    fio_io_s *io = r->conn.io;
    if (io)
      fio_io_close_now(io); /* on_close will not reconnect (running == 0) */
    return;
  }
  FIO_LOG_DEBUG("(redis) HELLO 3 handshake complete (RESP3)");
}

/**
 * Returns a snapshot of the Redis connection state.
 *
 * This function must be called from the IO thread: it observes the command
 * queue to distinguish a TCP connection awaiting `HELLO 3` from a RESP3-ready
 * connection. Commands may be sent regardless of the returned state.
 */
SFUNC fio_redis_state_e
fio_redis_state(fio_pubsub_engine_s const *engine) {
  if (!engine)
    return FIO_REDIS_STATE_ERROR;
  fio_redis_engine_s const *r = (fio_redis_engine_s const *)engine;
  if (!r->running)
    return FIO_REDIS_STATE_ERROR;
  if (!r->conn.io)
    return FIO_REDIS_STATE_CONNECTING;
  FIO_LIST_NODE const *node = r->cmd_queue.next;
  if (node != &r->cmd_queue) {
    fio_redis_cmd_s const *cmd =
        FIO_PTR_FROM_FIELD(fio_redis_cmd_s, node, node);
    if (cmd->callback == fio___redis_on_hello_reply)
      return FIO_REDIS_STATE_CONNECTING;
  }
  return FIO_REDIS_STATE_CONNECTED;
}

/** Capacity of the static resubscribe batch buffer. */
#define FIO___REDIS_RESUBSCRIBE_BUF_CAPA (128 * 1024)

/* A single static slot suffices: fio_io_write copies the data into the
 * outgoing IO buffer, so the slot may be reused as soon as a write call
 * returns (round robin of 1 concurrent allocation). */
FIO_STATIC_ALLOC_DEF(fio___redis_resubscribe_buf,
                     uint8_t,
                     FIO___REDIS_RESUBSCRIBE_BUF_CAPA,
                     1)

/** Sink receiving ready-to-send resubscribe wire buffers. */
typedef struct {
  /** Opaque user data, passed back to `on_write`. */
  void *udata;
  /**
   * Called once per flushed batch (and per oversized solo command).
   * `buf` may be reused / freed as soon as the callback returns.
   */
  void (*on_write)(void *udata, uint8_t *buf, size_t len);
} fio___redis_resubscribe_sink_s;

/**
 * Batches SUBSCRIBE/PSUBSCRIBE commands for all known channels (channels
 * first, then patterns) into the static resubscribe buffer, calling
 * `sink.on_write` whenever the buffer is full and once more at the end.
 *
 * Channel names that don't fit in half the buffer require a dynamic
 * allocation and are sent as a separate SUBSCRIBE/PSUBSCRIBE message.
 */
FIO_SFUNC void fio___redis_resubscribe_batch(fio___redis_resubscribe_sink_s sink) {
  uint8_t *buf = fio___redis_resubscribe_buf(0);
  const size_t half = FIO___REDIS_RESUBSCRIBE_BUF_CAPA / 2;
  size_t used = 0;
  for (int is_pattern = 0; is_pattern < 2; ++is_pattern) {
    fio___pubsub_channel_map_s *map =
        is_pattern ? &FIO___PUBSUB_POSTOFFICE.patterns
                   : &FIO___PUBSUB_POSTOFFICE.channels;
    const char *verb = is_pattern ? "PSUBSCRIBE" : "SUBSCRIBE";
    const size_t verb_len = is_pattern ? 10 : 9;
    FIO_MAP_EACH(fio___pubsub_channel_map, map, i) {
      fio_pubsub_channel_s *ch = i.node->key;
      size_t need = FIO___REDIS_SUB_CMD_OVERHEAD + ch->name_len;
      if (need > half) {
        /* oversized channel name: separate message, dynamic allocation */
        if (used) {
          sink.on_write(sink.udata, buf, used);
          used = 0;
        }
        uint8_t *solo = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, need, 0);
        if (solo) {
          size_t len = fio___redis_write_sub_cmd(solo,
                                                 verb,
                                                 verb_len,
                                                 FIO_BUF_INFO2(ch->name,
                                                               ch->name_len));
          sink.on_write(sink.udata, solo, len);
          FIO_MEM_FREE(solo, need);
        }
        continue;
      }
      if (used + need > FIO___REDIS_RESUBSCRIBE_BUF_CAPA) {
        sink.on_write(sink.udata, buf, used);
        used = 0;
      }
      used += fio___redis_write_sub_cmd(buf + used,
                                        verb,
                                        verb_len,
                                        FIO_BUF_INFO2(ch->name,
                                                      ch->name_len));
    }
  }
  if (used)
    sink.on_write(sink.udata, buf, used);
}

/** fio_io_write adapter for fio___redis_resubscribe_batch. */
FIO_SFUNC void fio___redis_resubscribe_io_write(void *io_,
                                                uint8_t *buf,
                                                size_t len) {
  fio_io_write((fio_io_s *)io_, buf, len);
}

/**
 * Resubscription on (re)connect: sends SUBSCRIBE/PSUBSCRIBE for all channels
 * the pub/sub system knows about, batched into the static resubscribe buffer
 * (flushed whenever it fills up).
 *
 * We do NOT call fio_pubsub_engine_attach here because that defers
 * fio___pubsub_attach_task, which could run after fio___pubsub_detach_task
 * (if detach is in progress), causing a use-after-free when attach_task
 * re-adds the freed engine to the pub/sub engines list.
 *
 * Instead, we directly iterate the pub/sub channel maps and send wire
 * commands. This is safe because we're on the IO thread and the channel
 * maps are only modified on the IO thread.
 *
 * MUST be called from the IO thread only, AFTER HELLO was written (RESP3
 * confirmations arrive as push frames on the same connection).
 */
FIO_SFUNC void fio___redis_resubscribe(fio_redis_engine_s *r) {
  if (!r->attached || !r->running || !r->conn.io)
    return;
  fio___redis_resubscribe_batch((fio___redis_resubscribe_sink_s){
      .udata = (void *)r->conn.io,
      .on_write = fio___redis_resubscribe_io_write,
  });
}

/**
 * Called when connection is attached to the protocol (connection ready).
 * Runs on IO thread - no lock needed.
 *
 * Queues HELLO 3 (auth folded in) as the first command. Its map reply is
 * consumed by the normal lock-step FIFO (see fio___redis_on_hello_reply).
 */
FIO_SFUNC void fio___redis_on_attach(fio_io_s *io) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)fio_io_udata(io);
  if (!r)
    return;

  fio_redis_connection_s *conn = &r->conn;
  conn->io = io;

  /* Queue HELLO 3 at the queue HEAD: it must be the first command on the
   * wire of every (re)connection - before any leftover commands and before
   * any SUBSCRIBE bytes (an early SUBSCRIBE would commandeer a RESP2 link
   * and its confirmations would desynchronize the reply FIFO). */
  fio_redis_cmd_s *hello = (fio_redis_cmd_s *)
      FIO_MEM_REALLOC(NULL, 0, sizeof(*hello) + r->hello_cmd_len + 1, 0);
  if (!hello) {
    FIO_LOG_ERROR("(redis) failed to allocate HELLO command");
    fio_io_close_now(io); /* reconnect will retry */
    return;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);
  *hello = (fio_redis_cmd_s){
      .callback = fio___redis_on_hello_reply,
      .cmd_len = r->hello_cmd_len,
  };
  FIO_MEMCPY(hello->cmd, r->hello_cmd, r->hello_cmd_len);
  hello->cmd[r->hello_cmd_len] = 0;
  /* Prepend to queue head (FIO_LIST has no unshift) */
  hello->node.next = r->cmd_queue.next;
  hello->node.prev = &r->cmd_queue;
  r->cmd_queue.next->prev = &hello->node;
  r->cmd_queue.next = &hello->node;

  FIO_LOG_DEBUG("(redis) connection established to %s:%s", r->address, r->port);

  /* Send any queued commands (HELLO is now at the head) - IO thread */
  r->pub_sent = 0;
  fio___redis_send_next_cmd(r);

  /* Resubscribe all known channels on this connection (reconnect case).
   * The batch is written AFTER the queued HELLO, so RESP3 confirmation
   * push frames never desynchronize the command FIFO. */
  fio___redis_resubscribe(r);
}

/**
 * Deferred continuation for fio___redis_parse_buffered (message cap reached).
 * Holds its own engine reference.
 */
FIO_SFUNC void fio___redis_on_data_continue(void *engine_, void *conn_);

/**
 * Parses all complete RESP messages currently in the connection's buffer.
 *
 * Uses a running offset and compacts the buffer at most once, at the end.
 * The parser is NOT reset between messages: after each complete object the
 * parser is already back at depth 0 with streaming flags cleared, so the
 * historical per-message `{0}` reset (~1KB memset per message) is redundant.
 * Partial/error state is still cleaned up by fio___redis_connection_reset
 * when the connection closes.
 *
 * Processes at most FIO_REDIS_MAX_BATCH messages per call; if more data
 * remains, the rest is deferred (fio___redis_on_data_continue) so no single
 * event-loop callback grows unbounded.
 *
 * MUST be called from the IO thread only.
 */
FIO_SFUNC void fio___redis_parse_buffered(fio_redis_engine_s *r,
                                          fio_redis_connection_s *conn,
                                          uint8_t *buf,
                                          fio_io_s *io) {
  size_t pos = 0;
  size_t count = 0;
  for (;;) {
    fio_resp3_result_s result = fio_resp3_parse(&conn->parser,
                                                &FIO___REDIS_RESP3_CALLBACKS,
                                                buf + pos,
                                                conn->buf_pos - pos);

    if (result.err || conn->ps.limit_exceeded) {
      FIO_LOG_ERROR("(redis) %s - closing connection",
                    conn->ps.limit_exceeded ? "payload limit exceeded"
                                            : "parser error");
      fio_io_close_now(io);
      return;
    }

    pos += result.consumed;

    if (!result.obj)
      break; /* no complete message available */

    /* Top-level message complete - reset per-message state */
    conn->ps.msg_total = 0;

    FIOBJ msg = (FIOBJ)result.obj;

    /* Per-frame routing: RESP3 push frames are out-of-band server messages
     * (pub/sub deliveries, subscribe confirmations); everything else is a
     * command reply matched to the lock-step FIFO queue. */
    uint8_t is_push = conn->ps.is_push;
    conn->ps.is_push = 0;
    if (is_push) {
      /* Zero-copy capture: publishing already happened at payload
       * completion (synchronous, before any compaction). Clean up. */
      fio___redis_capture_reset(&conn->ps);
    } else {
      fio___redis_on_pub_message(r, msg);
    }
    fiobj_free(msg);

    /* Cap work per event - defer the rest to keep the event loop snappy. */
    if (++count >= FIO_REDIS_MAX_BATCH && pos < conn->buf_pos) {
      fio___redis_capture_freeze(&conn->ps);
      conn->buf_pos -= (FIO___REDIS_BUF_POS_T)pos;
      FIO_MEMMOVE(buf, buf + pos, conn->buf_pos);
      fio___redis_dup(r); /* ref for deferred continuation task */
      fio_io_defer(fio___redis_on_data_continue, r, conn);
      return;
    }
  }

  /* Single compaction of unconsumed data to start of buffer */
  if (pos) {
    fio___redis_capture_freeze(&conn->ps);
    conn->buf_pos -= (FIO___REDIS_BUF_POS_T)pos;
    if (conn->buf_pos)
      FIO_MEMMOVE(buf, buf + pos, conn->buf_pos);
  }
}

FIO_SFUNC void fio___redis_on_data_continue(void *engine_, void *conn_) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine_;
  fio_redis_connection_s *conn = (fio_redis_connection_s *)conn_;
  fio_io_s *io = conn->io;
  if (!r->running || !io) {
    fio___redis_free(r);
    return;
  }
  fio___redis_parse_buffered(r, conn, r->buf, io);
  fio___redis_free(r);
}

FIO_SFUNC void fio___redis_on_data(fio_io_s *io) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)fio_io_udata(io);
  if (!r)
    return;

  fio_redis_connection_s *conn = &r->conn;
  uint8_t *buf = r->buf;

  /* Read data */
  size_t available = FIO_REDIS_READ_BUFFER - conn->buf_pos;
  size_t len = fio_io_read(io, buf + conn->buf_pos, available);
  if (!len)
    return;

  conn->buf_pos += (FIO___REDIS_BUF_POS_T)len;
  fio___redis_parse_buffered(r, conn, buf, io);
}

/** Internal helper for connection close handling.
 *
 * Called from on_close with the udata from the IO handle.
 * If r is NULL, the engine was already destroyed (fio___redis_destroy nulled
 * the udata before closing) — do nothing.
 *
 * The connection holds NO ownership ref. Reconnect tasks hold their own ref
 * (dup before defer, free at end of fio___redis_connect).
 */
FIO_SFUNC void fio___redis_on_close_internal(fio_redis_engine_s *r,
                                             fio_redis_connection_s *conn) {
  if (!r)
    return; /* engine already destroyed — udata was NULL */
  fio___redis_connection_reset(conn);

  /* Reconnect if IO reactor is still running */
  if (fio_io_is_running()) {
    FIO_LOG_WARNING("(redis) connection lost, reconnecting...");
    fio___redis_dup(r); /* ref for deferred reconnect task */
    fio_io_defer(fio___redis_connect, r, conn);
  }
  /* No fio___redis_free here — connection holds no ownership ref */
}

FIO_SFUNC void fio___redis_on_close(void *buffer, void *udata) {
  (void)buffer;
  fio___redis_on_close_internal((fio_redis_engine_s *)udata,
                                udata ? &((fio_redis_engine_s *)udata)->conn
                                      : NULL);
}

/**
 * Handle connection timeout.
 * Runs on IO thread - no lock needed for cmd_queue access.
 */
FIO_SFUNC void fio___redis_on_timeout(fio_io_s *io) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)fio_io_udata(io);
  if (!r)
    return;

  /* Check for stuck commands */
  if (!FIO_LIST_IS_EMPTY(&r->cmd_queue)) {
    FIO_LOG_WARNING("(redis) server unresponsive, disconnecting");
    fio_io_close_now(io);
    return;
  }
  /* Queue a PING command - already on the IO thread, queue directly.
   * The +PONG reply dequeues it under the normal lock-step FIFO. */
  fio_redis_cmd_s *cmd =
      (fio_redis_cmd_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*cmd) + 14, 0);
  if (cmd) {
    FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);
    *cmd = (fio_redis_cmd_s){.cmd_len = 14};
    FIO_MEMCPY(cmd->cmd, "*1\r\n$4\r\nPING\r\n", 14);
    FIO_LIST_PUSH(&r->cmd_queue, &cmd->node);
    fio___redis_send_next_cmd(r);
  }
}

/* *****************************************************************************
Connection Management
***************************************************************************** */

/** Timer callback wrapper for fio___redis_connect (returns int as required) */
FIO_SFUNC int fio___redis_connect_timer(void *engine_, void *conn_);

FIO_SFUNC void fio___redis_connect(void *engine_, void *conn_) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine_;
  fio_redis_connection_s *conn = (fio_redis_connection_s *)conn_;

  /* Workers must NOT connect to Redis - they use IPC to master.
   * Check !fio_io_is_master() rather than fio_io_is_worker() because in
   * single-process mode (fio_io_start(0)) both is_master and is_worker are
   * true, and we DO want to connect in that case. */
  if (!fio_io_is_master()) {
    fio___redis_free(r);
    return;
  }

  if (!r->running || conn->io) {
    fio___redis_free(r);
    return;
  }

  /* Build URL - bounded FIO_MEMCPY assembly, no truncation */
  char url[512];
  const size_t host_len = FIO_STRLEN(r->address);
  const size_t port_len = FIO_STRLEN(r->port);
  if (host_len + port_len + 8 > sizeof(url)) {
    FIO_LOG_ERROR("(redis) address too long: %s:%s", r->address, r->port);
    /* Same failure path as a failed connection attempt */
    fio_io_run_every(.fn = fio___redis_connect_timer,
                     .udata1 = r,
                     .udata2 = conn,
                     .every = 1000,
                     .repetitions = 1);
    return; /* ref ownership transferred to timer */
  }
  FIO_MEMCPY(url, "tcp://", 6);
  FIO_MEMCPY(url + 6, r->address, host_len);
  url[6 + host_len] = ':';
  FIO_MEMCPY(url + 7 + host_len, r->port, port_len);
  url[7 + host_len + port_len] = 0;

  FIO_LOG_DEBUG("(redis) connecting to %s:%s", r->address, r->port);

  /* Start async connection - on_attach will be called when ready */
  conn->io = fio_io_connect(url,
                            .protocol = &FIO___REDIS_PROTOCOL,
                            .udata = r,
                            .timeout = 30000);
  if (!conn->io) {
    FIO_LOG_ERROR("(redis) failed to initiate connection to %s:%s",
                  r->address,
                  r->port);
    /* Retry after delay — pass our ref to the timer (timer holds it) */
    fio_io_run_every(.fn = fio___redis_connect_timer,
                     .udata1 = r,
                     .udata2 = conn,
                     .every = 1000,
                     .repetitions = 1);
    return; /* ref ownership transferred to timer */
  }
  /* Connection initiated successfully.
   * The connection holds NO ownership ref — release the deferred task's ref. */
  fio___redis_free(r);
}

/** Timer callback wrapper - calls fio___redis_connect and returns 0 to stop */
FIO_SFUNC int fio___redis_connect_timer(void *engine_, void *conn_) {
  fio___redis_connect(engine_, conn_);
  return 0; /* Don't repeat - fio___redis_connect handles its own retry */
}

/* *****************************************************************************
IPC Handlers for Multi-Process Communication

Workers forward publish and send operations to the master via IPC.
The master executes them on the real Redis connection and replies back.

IPC data layouts:
- PUBLISH from worker:
    [fio___redis_ipc_publish_header_s][channel_bytes][message_bytes]

- fio_redis_send from worker:
    [callback_ptr (8)][engine_ptr (8)][RESP command bytes]
    User's udata stored in ipc->udata directly (no heap allocation).

- Reply from master to worker (fio_redis_send):
    [callback_ptr (8)][engine_ptr (8)][RESP reply bytes]
    The 16-byte header is echoed from the original request so the worker
    can recover the callback and engine pointers.
***************************************************************************** */

/** IPC data header for PUBLISH forwarding from worker to master */
typedef struct {
  fio_redis_engine_s *engine;
  uint32_t channel_len;
  uint32_t message_len;
} fio___redis_ipc_publish_header_s;

/**
 * Internal: Build a RESP PUBLISH command for the given channel and message.
 * Returns a newly allocated fio_redis_cmd_s or NULL on failure.
 */
FIO_SFUNC fio_redis_cmd_s *fio___redis_publish_cmd_new(fio_buf_info_s channel,
                                                       fio_buf_info_s message) {
  size_t cmd_size = sizeof(fio_redis_cmd_s) + 64 + channel.len + message.len;
  fio_redis_cmd_s *cmd =
      (fio_redis_cmd_s *)FIO_MEM_REALLOC(NULL, 0, cmd_size, 0);
  if (!cmd)
    return NULL;
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);

  *cmd = (fio_redis_cmd_s){0};

  char *buf = (char *)cmd->cmd;
  size_t pos = 0;

  /* *3\r\n$7\r\nPUBLISH\r\n$<ch_len>\r\n<ch>\r\n$<msg_len>\r\n<msg>\r\n */
  FIO_MEMCPY(buf + pos, "*3\r\n$7\r\nPUBLISH\r\n$", 18);
  pos += 18;
  pos += (size_t)fio_ltoa(buf + pos, (int64_t)channel.len, 10);
  buf[pos++] = '\r';
  buf[pos++] = '\n';
  FIO_MEMCPY(buf + pos, channel.buf, channel.len);
  pos += channel.len;
  buf[pos++] = '\r';
  buf[pos++] = '\n';
  buf[pos++] = '$';
  pos += (size_t)fio_ltoa(buf + pos, (int64_t)message.len, 10);
  buf[pos++] = '\r';
  buf[pos++] = '\n';
  FIO_MEMCPY(buf + pos, message.buf, message.len);
  pos += message.len;
  buf[pos++] = '\r';
  buf[pos++] = '\n';
  buf[pos] = 0;

  cmd->cmd_len = pos;
  return cmd;
}

/**
 * Master-side IPC handler: receives PUBLISH from a worker.
 * Extracts channel + message and queues a PUBLISH command on the Redis
 * pub connection.
 */
FIO_SFUNC void fio___redis_ipc_publish_on_master(fio_ipc_s *ipc) {
  if (ipc->len < sizeof(fio___redis_ipc_publish_header_s))
    return;

  fio___redis_ipc_publish_header_s header;
  FIO_MEMCPY(&header, ipc->data, sizeof(header));

  /* Validate lengths */
  size_t expected = sizeof(header) + header.channel_len + header.message_len;
  if (ipc->len < expected)
    return;

  char *payload = ipc->data + sizeof(header);
  fio_buf_info_s channel = FIO_BUF_INFO2(payload, header.channel_len);
  fio_buf_info_s message =
      FIO_BUF_INFO2(payload + header.channel_len, header.message_len);

  fio_redis_engine_s *r = header.engine;
  if (!r || !r->running)
    return;

  fio_redis_cmd_s *cmd = fio___redis_publish_cmd_new(channel, message);
  if (!cmd)
    return;
  fio___redis_attach_cmd(r, cmd);
}

/** Size of the IPC data header for fio_redis_send: callback + engine ptrs */
#define FIO___REDIS_IPC_SEND_HEADER 16

/**
 * Master-side callback: Redis replied to a command forwarded from a worker.
 * Serializes the FIOBJ reply to RESP and sends it back via IPC reply.
 * Echoes the 16-byte header (callback + engine) so the worker can recover them.
 */
FIO_SFUNC void fio___redis_ipc_send_redis_reply(fio_pubsub_engine_s *e,
                                                FIOBJ reply,
                                                void *udata) {
  fio_ipc_s *ipc = (fio_ipc_s *)udata;
  (void)e;

  /* Serialize reply to RESP format (stack buffer, heap for large replies) */
  uint8_t stack_buf[512];
  uint8_t *buf = stack_buf;
  size_t resp_len = 0;
  if (reply != FIOBJ_INVALID)
    resp_len = fio___redis_fiobj2resp_len(reply, 0);
  if (resp_len) {
    if (resp_len > sizeof(stack_buf)) {
      buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, resp_len, 0);
      if (!buf) {
        buf = stack_buf;
        resp_len = 0;
      }
    }
    if (resp_len)
      fio___redis_fiobj2resp_write(buf, reply, 0);
  }
  if (!resp_len) {
    /* No reply / error / unserializable / OOM - send null */
    resp_len = 5;
    FIO_MEMCPY(buf, "$-1\r\n", 5);
  }

  /* Send reply back to worker via IPC.
   * Echo the 16-byte header from the original request so the worker
   * can recover callback + engine pointers from the reply data. */
  fio_ipc_reply(ipc,
                .data = FIO_IPC_DATA(
                    FIO_BUF_INFO2(ipc->data, FIO___REDIS_IPC_SEND_HEADER),
                    FIO_BUF_INFO2((char *)buf, resp_len)),
                .done = 1);
  if (buf != stack_buf)
    FIO_MEM_FREE(buf, resp_len);
  fio_ipc_free(ipc);
}

/**
 * Master-side IPC handler: receives fio_redis_send() from a worker.
 * IPC data layout: [callback (8) | engine (8) | RESP command bytes]
 * Extracts the engine pointer, queues the RESP command on the Redis
 * pub connection, and arranges to reply via IPC when Redis responds.
 */
FIO_SFUNC void fio___redis_ipc_send_on_master(fio_ipc_s *ipc) {
  if (ipc->len < FIO___REDIS_IPC_SEND_HEADER)
    return;

  /* Extract engine pointer (skip callback, that's for the worker) */
  fio_redis_engine_s *r;
  FIO_MEMCPY(&r, ipc->data + 8, sizeof(r));

  if (!r || !r->running) {
    /* Engine gone - reply with null so worker callback fires.
     * Echo the 16-byte header so the worker can recover callback + engine. */
    fio_ipc_reply(ipc,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2(ipc->data, FIO___REDIS_IPC_SEND_HEADER),
                      FIO_BUF_INFO2((char *)"$-1\r\n", 5)),
                  .done = 1);
    return;
  }

  /* Extract RESP command bytes (after the 16-byte header) */
  char *resp_data = ipc->data + FIO___REDIS_IPC_SEND_HEADER;
  size_t resp_len = ipc->len - FIO___REDIS_IPC_SEND_HEADER;

  /* Allocate command node for the master's queue */
  fio_redis_cmd_s *cmd = (fio_redis_cmd_s *)
      FIO_MEM_REALLOC(NULL, 0, sizeof(*cmd) + resp_len + 1, 0);
  if (!cmd) {
    fio_ipc_reply(ipc,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2(ipc->data, FIO___REDIS_IPC_SEND_HEADER),
                      FIO_BUF_INFO2((char *)"$-1\r\n", 5)),
                  .done = 1);
    return;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);

  /* Dup the IPC message so we can reply later when Redis responds.
   * The dup preserves ipc->data which still has the 16-byte header. */
  fio_ipc_s *ipc_ref = fio_ipc_dup(ipc);

  *cmd = (fio_redis_cmd_s){
      .callback = fio___redis_ipc_send_redis_reply,
      .udata = ipc_ref,
      .cmd_len = resp_len,
  };
  FIO_MEMCPY(cmd->cmd, resp_data, resp_len);
  cmd->cmd[resp_len] = 0;

  /* Queue command on master's Redis connection */
  fio___redis_attach_cmd(r, cmd);
}

/**
 * Worker-side IPC done handler: receives Redis reply from master.
 * IPC data layout: [callback (8) | engine (8) | RESP reply bytes]
 * Extracts callback + engine from the data prefix, parses the RESP reply,
 * and invokes the original user callback.
 *
 * This is the on_done handler (not on_reply) because the master sends
 * a single reply with .done = 1, which dispatches directly to on_done.
 */
FIO_SFUNC void fio___redis_ipc_send_on_done(fio_ipc_s *ipc) {
  if (ipc->len < FIO___REDIS_IPC_SEND_HEADER)
    return;

  /* Extract callback and engine from the reply data prefix */
  void (*callback)(fio_pubsub_engine_s *, FIOBJ, void *);
  fio_pubsub_engine_s *engine;
  FIO_MEMCPY(&callback, ipc->data, sizeof(callback));
  FIO_MEMCPY(&engine, ipc->data + 8, sizeof(engine));

  /* Parse RESP reply from data after the 16-byte header */
  FIOBJ reply = FIOBJ_INVALID;
  size_t resp_off = FIO___REDIS_IPC_SEND_HEADER;
  if (ipc->len > resp_off) {
    fio_resp3_parser_s parser = {0};
    fio_resp3_result_s result = fio_resp3_parse(&parser,
                                                &FIO___REDIS_RESP3_CALLBACKS,
                                                ipc->data + resp_off,
                                                ipc->len - resp_off);
    if (result.obj)
      reply = (FIOBJ)result.obj;
  }

  /* Invoke the original callback. ipc->udata is the user's udata. */
  if (callback)
    callback(engine, reply, ipc->udata);
  fiobj_free(reply);
}

/* *****************************************************************************
Pub/Sub Engine Callbacks

NOTE: All pub/sub engine callbacks are called from the IO thread by the pub/sub
system, so they don't need locks for accessing engine state.

EXECUTION CONTEXT:
- subscribe, psubscribe, unsubscribe, punsubscribe: MASTER ONLY
- publish: ANY process (worker or master)
- detached: MASTER ONLY
***************************************************************************** */

/**
 * Called when engine is detached from pub/sub.
 * Runs on IO thread - no lock needed.
 *
 * Releases the system's reference that was taken in fio___redis_subscribe /
 * fio___redis_psubscribe when r->attached was first set to 1.
 *
 * Guard: if r->attached is 0, no system ref was taken — return immediately
 * to avoid underflowing the reference count.
 *
 * NOTE: We do NOT close the connection here. It is shared with the command
 * queue and will close naturally (via IO shutdown or normal close path).
 * This avoids the re-entrancy hazard of closing it here and having on_close
 * fire on an engine that may already be in fio___redis_destroy.
 * Server-side subscriptions are dropped with fire-and-forget UNSUBSCRIBE /
 * PUNSUBSCRIBE (RESP3 confirmations arrive as push frames, ignored).
 */
FIO_SFUNC void fio___redis_detached(const fio_pubsub_engine_s *eng) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  FIO_LOG_DEBUG("(redis) engine detached from pub/sub");
  /* Guard: only release if the attach-ref was actually taken. */
  if (!r->attached)
    return;
  r->attached = 0;

  /* Drop all server-side subscriptions (fire-and-forget; RESP3
   * confirmations arrive as push frames and are ignored). The connection
   * itself stays open - it is shared with the command queue. */
  if (r->conn.io) {
    fio_io_write(r->conn.io,
                 (char *)"*1\r\n$11\r\nUNSUBSCRIBE\r\n"
                         "*1\r\n$12\r\nPUNSUBSCRIBE\r\n",
                 45);
  }

  /* Release the system's reference (transferred at fio_pubsub_engine_attach).
   * The caller moved their ref to the system; this releases it. */
  fio___redis_free(r);
}

/**
 * Shared attach logic for subscribe / psubscribe: marks the engine as
 * attached and starts the connection if needed.
 */
FIO_SFUNC void fio___redis_ensure_attached(fio_redis_engine_s *r) {
  /* The system's ref was transferred at fio_pubsub_engine_attach time -
   * the caller moved their ref to the system. No dup here. */
  if (!r->attached) {
    r->attached = 1;
    if (r->running && !r->conn.io) {
      fio___redis_dup(r); /* ref for deferred connect task */
      fio_io_defer(fio___redis_connect, r, &r->conn);
    }
  }
}

/**
 * Subscribe to a channel.
 * Runs on MASTER ONLY - no lock needed.
 */
FIO_SFUNC void fio___redis_subscribe(const fio_pubsub_engine_s *eng,
                                     fio_buf_info_s channel,
                                     int16_t filter) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  (void)filter;
  fio___redis_ensure_attached(r);
  fio___redis_sub_send(r, "SUBSCRIBE", 9, channel);
}

/**
 * Subscribe to a pattern.
 * Runs on MASTER ONLY - no lock needed.
 */
FIO_SFUNC void fio___redis_psubscribe(const fio_pubsub_engine_s *eng,
                                      fio_buf_info_s channel,
                                      int16_t filter) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  (void)filter;
  fio___redis_ensure_attached(r);
  fio___redis_sub_send(r, "PSUBSCRIBE", 10, channel);
}

/**
 * Unsubscribe from a channel.
 * Runs on MASTER ONLY - no lock needed.
 */
FIO_SFUNC void fio___redis_unsubscribe(const fio_pubsub_engine_s *eng,
                                       fio_buf_info_s channel,
                                       int16_t filter) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  (void)filter;

  /* Skip if not attached (fio___redis_sub_send checks the connection) */
  if (!r->attached)
    return;
  fio___redis_sub_send(r, "UNSUBSCRIBE", 11, channel);
}

/**
 * Unsubscribe from a pattern.
 * Runs on MASTER ONLY - no lock needed.
 */
FIO_SFUNC void fio___redis_punsubscribe(const fio_pubsub_engine_s *eng,
                                        fio_buf_info_s channel,
                                        int16_t filter) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  (void)filter;

  /* Skip if not attached (fio___redis_sub_send checks the connection) */
  if (!r->attached)
    return;
  fio___redis_sub_send(r, "PUNSUBSCRIBE", 12, channel);
}

/**
 * Worker-side publish: forwards channel + message to master via IPC.
 * Installed as the engine's publish callback in forked worker processes.
 */
FIO_SFUNC void fio___redis_publish_worker(const fio_pubsub_engine_s *eng,
                                          const fio_pubsub_msg_s *msg) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  fio___redis_ipc_publish_header_s header = {
      .engine = r,
      .channel_len = (uint32_t)msg->channel.len,
      .message_len = (uint32_t)msg->message.len,
  };
  fio_ipc_call(.call = fio___redis_ipc_publish_on_master,
               .data =
                   FIO_IPC_DATA(FIO_BUF_INFO2((char *)&header, sizeof(header)),
                                msg->channel,
                                msg->message));
}

/**
 * Master-side publish: builds PUBLISH command and queues it on the Redis
 * pub connection.
 *
 * In single-process mode (fio_io_start(0)) this is also the active callback
 * since FIO_CALL_IN_CHILD never fires and the vtable is not swapped.
 */
FIO_SFUNC void fio___redis_publish(const fio_pubsub_engine_s *eng,
                                   const fio_pubsub_msg_s *msg) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  fio_redis_cmd_s *cmd =
      fio___redis_publish_cmd_new(msg->channel, msg->message);
  if (!cmd)
    return;
  fio___redis_attach_cmd(r, cmd);
}

/**
 * FIO_CALL_IN_CHILD callback: swap the publish vtable pointer to the
 * worker-specific IPC-forwarding implementation.
 *
 * This fires only in forked worker processes (never in single-process mode).
 *
 * We also reset the inherited leak counters here. The worker inherits the
 * master's allocation counters via fork, but the engine is owned and freed
 * by the master — the worker should not count it as a leak.
 */
FIO_SFUNC void fio___redis_on_fork(void *engine_) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine_;
  r->engine.publish = fio___redis_publish_worker;
  /* Reset inherited leak counters: the engine was allocated in the master
   * and will be freed there. The worker's counters would otherwise report
   * a false positive "1 leak" for each inherited engine. */
  FIO_LEAK_COUNTER_ON_FREE(fio___redis_engine);
  FIO_LEAK_COUNTER_ON_FREE(fio___redis);
}

/* *****************************************************************************
Public API
***************************************************************************** */

void fio_redis_new____(void); /* IDE marker */
/**
 * Creates a Redis pub/sub engine with reference count = 1.
 *
 * The caller owns the returned reference and must call fio_redis_free()
 * when done. Attaching to pub/sub does NOT transfer ownership.
 */
SFUNC fio_pubsub_engine_s *fio_redis_new FIO_NOOP(fio_redis_args_s args) {
  /* Default URL if not provided */
  static const char *default_host = "localhost";
  static const char *default_port = "6379";

  /* Parse URL to extract host and port */
  const char *host = default_host;
  size_t host_len = 9; /* strlen("localhost") */
  const char *port = default_port;
  size_t port_len = 4; /* strlen("6379") */

  if (args.url && args.url[0]) {
    fio_url_s u = fio_url_parse(args.url, strlen(args.url));
    if (u.host.buf && u.host.len) {
      host = u.host.buf;
      host_len = u.host.len;
    }
    if (u.port.buf && u.port.len) {
      port = u.port.buf;
      port_len = u.port.len;
    }
  }

  if (!args.ping_interval)
    args.ping_interval = 30;

  if (!args.payload_limit)
    args.payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT;

  size_t auth_len = args.auth_len;
  if (args.auth && !auth_len)
    auth_len = strlen(args.auth);

  /* Pre-compute the HELLO 3 handshake command (auth folds into HELLO).
   * Exact sizing (no estimates); auth failure is a hard engine error. */
  size_t hello_cmd_len = fio___redis_hello_cmd_len(auth_len);

  /* Allocate engine using FIO_REF (includes ref count, starts at 1).
   * Note: buf[FIO_REDIS_READ_BUFFER] is already inside the struct,
   * so we only need flex space for the strings. */
  size_t flex_size = host_len + 1 + port_len + 1 + hello_cmd_len;
  fio_redis_engine_s *r = fio___redis_new(flex_size);
  if (!r) {
    FIO_LOG_ERROR("(redis) failed to allocate engine");
    return NULL;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_engine);

  /* Initialize (ref = 1 is already set by fio___redis_new) */
  *r = (fio_redis_engine_s){
      .engine =
          {
              .detached = fio___redis_detached,
              .subscribe = fio___redis_subscribe,
              .psubscribe = fio___redis_psubscribe,
              .unsubscribe = fio___redis_unsubscribe,
              .punsubscribe = fio___redis_punsubscribe,
              .publish = fio___redis_publish,
          },
      .cmd_queue = FIO_LIST_INIT(r->cmd_queue),
      .payload_limit = args.payload_limit,
      .ping_interval = args.ping_interval,
      .running = 1,
  };

  /* Wire the per-connection payload budget accounting into the parser */
  r->conn.ps.payload_limit = args.payload_limit;
  r->conn.parser.udata = &r->conn.ps;

  /* Set up string pointers in the flex area after the struct */
  char *str_ptr = (char *)(r + 1);
  r->address = str_ptr;
  FIO_MEMCPY(r->address, host, host_len);
  r->address[host_len] = '\0';
  str_ptr += host_len + 1;

  r->port = str_ptr;
  FIO_MEMCPY(r->port, port, port_len);
  r->port[port_len] = '\0';
  str_ptr += port_len + 1;

  /* Build the HELLO 3 command (with AUTH folded in when configured) */
  r->hello_cmd = str_ptr;
  r->hello_cmd_len =
      fio___redis_write_hello_cmd((uint8_t *)r->hello_cmd, args.auth, auth_len);
  r->hello_cmd[r->hello_cmd_len] = 0;
  FIO_ASSERT(r->hello_cmd_len == hello_cmd_len,
             "(redis) HELLO command size mismatch");

  /* Set timeout based on ping interval */
  uint32_t timeout_ms = (uint32_t)args.ping_interval * 1000;
  if (timeout_ms > FIO_IO_TIMEOUT_MAX)
    timeout_ms = FIO_IO_TIMEOUT_MAX;
  FIO___REDIS_PROTOCOL.timeout = timeout_ms;

  /* Start connection - increment ref for deferred task.
   * The deferred connect will check fio_io_is_worker() and skip on workers. */
  fio___redis_dup(r);
  fio_io_defer(fio___redis_connect, r, &r->conn);

  /* Register fork callback to swap publish vtable in worker processes.
   * FIO_CALL_IN_CHILD fires only in forked workers, not in single-process
   * mode, so the master publish path remains active when appropriate. */
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___redis_on_fork, r);

  FIO_LOG_DEBUG("(redis) engine created for %s:%s (ref=1)",
                r->address,
                r->port);
  return &r->engine;
}

/**
 * Increments the reference count and returns the engine.
 */
SFUNC fio_pubsub_engine_s *fio_redis_dup(fio_pubsub_engine_s *engine) {
  if (!engine)
    return NULL;
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine;
  fio___redis_dup(r);
  return engine;
}

/**
 * Releases the caller's reference to the engine.
 * Thread-safe - simply decrements the reference count.
 *
 * If this was the last reference, fio___redis_destroy() fires immediately:
 * sets running=0, closes connections, drains the command queue, frees memory.
 *
 * The caller does NOT need to call fio_pubsub_engine_detach() before freeing.
 * If the engine is attached, the pub/sub system holds its own ref which is
 * released when fio___redis_detached() fires (via detach or shutdown).
 *
 * Safe to call with NULL (no-op).
 */
SFUNC void fio_redis_free(fio_pubsub_engine_s *engine) {
  if (!engine)
    return;
  fio___redis_free((fio_redis_engine_s *)engine);
}

/**
 * Sends a Redis command.
 * Thread-safe - command queuing is deferred to the IO thread.
 *
 * On MASTER: queues command directly on the Redis pub connection.
 * On WORKER: forwards command to master via IPC, receives reply back.
 */
SFUNC int fio_redis_send(fio_pubsub_engine_s *engine,
                         FIOBJ command,
                         void (*callback)(fio_pubsub_engine_s *e,
                                          FIOBJ reply,
                                          void *udata),
                         void *udata) {
  if (!engine || FIOBJ_TYPE(command) != FIOBJ_T_ARRAY)
    return -1;

  fio_redis_engine_s *r = (fio_redis_engine_s *)engine;

  /* Compute the exact RESP length once (validates nesting depth) */
  size_t resp_len = fio___redis_fiobj2resp_len(command, 0);
  if (!resp_len)
    return -1;

  /* Worker path: forward via IPC to master.
   * Use !fio_io_is_master() so single-process mode takes the master path.
   *
   * IPC data layout: [callback(8) | engine(8) | RESP command bytes]
   * ipc->udata carries the user's udata directly (no heap allocation).
   * Master echoes the 16-byte header in its reply so the worker's on_done
   * handler can recover callback + engine pointers from the reply data.
   *
   * The RESP bytes are serialized into a stack buffer (heap for large
   * commands) - fio_ipc_call copies the data synchronously. */
  if (!fio_io_is_master()) {
    uint8_t stack_buf[512];
    uint8_t *buf = stack_buf;
    if (resp_len > sizeof(stack_buf)) {
      buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, resp_len, 0);
      if (!buf)
        return -1;
    }
    fio___redis_fiobj2resp_write(buf, command, 0);
    fio_ipc_call(.call = fio___redis_ipc_send_on_master,
                 .on_done = fio___redis_ipc_send_on_done,
                 .udata = udata,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)&callback, sizeof(callback)),
                     FIO_BUF_INFO2((char *)&r, sizeof(r)),
                     FIO_BUF_INFO2((char *)buf, resp_len)));
    if (buf != stack_buf)
      FIO_MEM_FREE(buf, resp_len);
    return 0;
  }

  /* Master path: serialize directly into the queued command node.
   * Single allocation, single write pass, no temporary FIOBJ string. */
  fio_redis_cmd_s *cmd = (fio_redis_cmd_s *)
      FIO_MEM_REALLOC(NULL, 0, sizeof(*cmd) + resp_len + 1, 0);
  if (!cmd)
    return -1;
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);

  *cmd = (fio_redis_cmd_s){
      .callback = callback,
      .udata = udata,
      .cmd_len = resp_len,
  };
  *fio___redis_fiobj2resp_write(cmd->cmd, command, 0) = 0;

  /* Queue command - thread-safe via fio_io_defer */
  fio___redis_attach_cmd(r, cmd);
  return 0;
}

/* *****************************************************************************
Redis Module Cleanup
***************************************************************************** */
#undef FIO___RECURSIVE_INCLUDE
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_REDIS && !H___FIO_REDIS___H */
