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
- Command queue with callbacks for arbitrary Redis commands
- Authentication support
- Automatic reconnection on connection loss
- Ping/pong keepalive
- Optional pub/sub integration with SUBSCRIBE/PSUBSCRIBE/PUBLISH

Multi-Process Architecture:
===========================
Only the MASTER process connects to the Redis server. Worker processes
communicate with the master via IPC (Inter-Process Communication).

  Master:   [Redis pub_conn] + [Redis sub_conn] → Redis Server
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
- fio_redis_free():  Thread-safe (defers cleanup to IO thread)
- fio_redis_send():  Thread-safe (defers command queuing to IO thread)

Internal operations that run on the IO thread:
- Command queue management (add, remove, send)
- Connection state changes (connect, disconnect, reconnect)
- Protocol callbacks (on_attach, on_data, on_close, on_timeout)
- Pub/sub engine callbacks (subscribe, publish, etc.)

Reference Counting Ownership Model:
====================================
The Redis engine uses reference counting for memory management:

- fio_redis_new():   Creates engine with ref=1 (caller's reference)
- fio_redis_dup():   Increments ref, returns engine
- fio_redis_free():  Decrements ref, destroys when ref reaches 0

Important: fio_pubsub_engine_attach/detach do NOT affect reference counts.
The caller is responsible for calling fio_redis_free() when done.

Internal reference management:
- Deferred tasks (connect, callbacks) increment ref before scheduling
  and decrement after the task completes
- Connection on_close callbacks decrement ref (balancing connect's ref)
- The on_detached callback does NOT free memory; it only marks the
  engine as detached

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

    // Cleanup
    fio_redis_free(redis);

Usage 2 - With Pub/Sub:
    fio_pubsub_engine_s *redis = fio_redis_new(
        .url = "localhost:6379"
    );

    // Explicitly attach to pub/sub system - this starts the subscription
    // connection and enables SUBSCRIBE/PSUBSCRIBE/PUBLISH functionality
    fio_pubsub_engine_attach(redis);

    // ... use pub/sub ...

    // Explicitly detach before destroying if attached
    fio_pubsub_engine_detach(redis);
    fio_redis_free(redis);

Note: When used as a sub-engine for clustering, do NOT attach to pub/sub.
The cluster engine will manage the Redis engine directly.

***************************************************************************** */

/* *****************************************************************************
Redis Engine Settings
***************************************************************************** */

#ifndef FIO_REDIS_READ_BUFFER
/** Size of the read buffer for Redis connections */
#define FIO_REDIS_READ_BUFFER 32768
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
  /** Redis server's password, if any (for AUTH command) */
  const char *auth;
  /** Length of auth string (0 = auto-detect with strlen) */
  size_t auth_len;
  /** Ping interval in seconds (0 = default 300 seconds) */
  uint8_t ping_interval;
} fio_redis_args_s;

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
 * Decrements the reference count. When count reaches 0, destroys the engine.
 *
 * This function:
 * - Decrements the reference count
 * - If ref reaches 0:
 *   - Closes all connections
 *   - Frees all queued commands
 *   - Frees the engine memory
 *
 * IMPORTANT: If the engine was attached to pub/sub via
 * fio_pubsub_engine_attach(), you MUST call fio_pubsub_engine_detach() before
 * calling fio_redis_free().
 *
 * Safe to call with NULL (no-op).
 */
SFUNC void fio_redis_free(fio_pubsub_engine_s *engine);

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
  uint16_t buf_pos; /* Position in read buffer */
  uint8_t is_sub;   /* Is this the subscription connection? */
} fio_redis_connection_s;

/**
 * Redis engine structure.
 *
 * Reference counting ownership model (via FIO_REF):
 * - fio_redis_new():  ref = 1 (caller owns this reference)
 * - fio_redis_dup():  ref += 1 (returns engine)
 * - fio_redis_free(): ref -= 1, if ref==0 calls fio___redis_destroy()
 *
 * Internal reference management:
 * - fio_io_defer(fio___redis_connect, ...): ref += 1 before, ref -= 1 after
 * - fio_io_defer(fio___redis_perform_callback, ...): ref += 1 before, ref -= 1
 * after
 * - on_close callbacks: ref -= 1 (balances the connect ref)
 *
 * Pub/Sub integration (NO ref changes):
 * - fio_pubsub_engine_attach(): does NOT increment ref
 * - fio_pubsub_engine_detach(): does NOT decrement ref
 * - on_detached callback: only marks engine as detached, does NOT free
 */
typedef struct fio_redis_engine_s {
  fio_pubsub_engine_s engine;      /* Must be first for casting */
  fio_redis_connection_s pub_conn; /* Publishing connection */
  fio_redis_connection_s sub_conn; /* Subscription connection */
  char *address;
  char *port;
  char *auth_cmd; /* Pre-formatted AUTH command */
  size_t auth_cmd_len;
  FIOBJ last_channel;      /* Last received channel (dedup) */
  FIO_LIST_HEAD cmd_queue; /* Command queue - accessed only from IO thread */
  uint8_t ping_interval;
  volatile uint8_t pub_sent;  /* Flag: command sent, awaiting reply */
  volatile uint8_t running;   /* Flag: engine is active (for reconnection) */
  volatile uint8_t attached;  /* Flag: attached to pub/sub system */
  volatile uint8_t detaching; /* Flag: detach in progress, don't re-attach */
  uint8_t
      buf[FIO_REDIS_READ_BUFFER * 2]; /* Read buffers for both connections */
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

FIO_SFUNC void *fio___redis_on_null(void *udata) {
  (void)udata;
  return (void *)fiobj_null();
}

FIO_SFUNC void *fio___redis_on_bool(void *udata, int is_true) {
  (void)udata;
  return (void *)(is_true ? fiobj_true() : fiobj_false());
}

FIO_SFUNC void *fio___redis_on_number(void *udata, int64_t num) {
  (void)udata;
  return (void *)fiobj_num_new((intptr_t)num);
}

FIO_SFUNC void *fio___redis_on_double(void *udata, double num) {
  (void)udata;
  return (void *)fiobj_float_new(num);
}

FIO_SFUNC void *fio___redis_on_bignum(void *udata,
                                      const void *data,
                                      size_t len) {
  (void)udata;
  /* Store big numbers as strings */
  return (void *)fiobj_str_new_cstr((const char *)data, len);
}

FIO_SFUNC void *fio___redis_on_string(void *udata,
                                      const void *data,
                                      size_t len,
                                      uint8_t type) {
  (void)udata;
  (void)type;
  return (void *)fiobj_str_new_cstr((const char *)data, len);
}

/**
 * Called when a blob string starts.
 * Pre-allocate FIOBJ string buffer if length is known.
 */
FIO_SFUNC void *fio___redis_on_start_string(void *udata,
                                            size_t len,
                                            uint8_t type) {
  (void)udata;
  (void)type;
  /* Pre-allocate if length known, otherwise create empty string */
  if (len != (size_t)-1 && len > 0)
    return (void *)fiobj_str_new_buf(len);
  return (void *)fiobj_str_new();
}

/**
 * Called with partial string data - append to FIOBJ string.
 */
FIO_SFUNC int fio___redis_on_string_write(void *udata,
                                          void *ctx,
                                          const void *data,
                                          size_t len) {
  (void)udata;
  fiobj_str_write((FIOBJ)ctx, (const char *)data, len);
  return 0;
}

/**
 * Called when string is complete - return the FIOBJ string.
 */
FIO_SFUNC void *fio___redis_on_string_done(void *udata,
                                           void *ctx,
                                           uint8_t type) {
  (void)udata;
  (void)type;
  return ctx; /* Return the completed FIOBJ string */
}

FIO_SFUNC void *fio___redis_on_error(void *udata,
                                     const void *data,
                                     size_t len,
                                     uint8_t type) {
  (void)udata;
  (void)type;
  /* Store errors as strings - caller can check context */
  FIOBJ err = fiobj_str_new_cstr((const char *)data, len);
  FIO_LOG_WARNING("(redis) error response: %.*s", (int)len, (const char *)data);
  return (void *)err;
}

FIO_SFUNC void *fio___redis_on_array(void *udata,
                                     void *parent_ctx,
                                     int64_t len) {
  (void)udata;
  (void)parent_ctx;
  (void)len;
  return (void *)fiobj_array_new();
}

FIO_SFUNC void *fio___redis_on_map(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  (void)len;
  return (void *)fiobj_hash_new();
}

FIO_SFUNC void *fio___redis_on_push(void *udata,
                                    void *parent_ctx,
                                    int64_t len) {
  /* Push messages are treated as arrays */
  return fio___redis_on_array(udata, parent_ctx, len);
}

FIO_SFUNC int fio___redis_array_push(void *udata, void *ctx, void *value) {
  (void)udata;
  fiobj_array_push((FIOBJ)ctx, (FIOBJ)value);
  return 0;
}

FIO_SFUNC int fio___redis_map_push(void *udata,
                                   void *ctx,
                                   void *key,
                                   void *value) {
  (void)udata;
  fiobj_hash_set((FIOBJ)ctx, (FIOBJ)key, (FIOBJ)value, NULL);
  if (key != value) /* in a set, both key and value are same and owned by set */
    fiobj_free((FIOBJ)key);
  return 0;
}

FIO_SFUNC void fio___redis_free_unused(void *udata, void *obj) {
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
    .push_push = fio___redis_array_push,
    .map_push = fio___redis_map_push,
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

/** Writes a FIOBJ to RESP format into a string.
 * Uses RESP3 type prefixes to preserve types through serialization roundtrips:
 * - NULL  → $-1\r\n (RESP2 null bulk string)
 * - TRUE  → #t\r\n (RESP3 boolean)
 * - FALSE → #f\r\n (RESP3 boolean)
 * - NUMBER → :<number>\r\n (RESP integer)
 * - FLOAT  → ,<float>\r\n (RESP3 double)
 * - HASH   → %<count>\r\n (RESP3 map)
 * - ARRAY  → *<count>\r\n (RESP2 array)
 * - STRING → $<len>\r\n<data>\r\n (RESP2 bulk string)
 */
FIO_SFUNC void fio___redis_fiobj2resp(FIOBJ dest, FIOBJ obj) {
  fio_str_info_s s;
  switch (FIOBJ_TYPE(obj)) {
  case FIOBJ_T_NULL: fiobj_str_write(dest, "$-1\r\n", 5); break;
  case FIOBJ_T_TRUE: fiobj_str_write(dest, "#t\r\n", 4); break;
  case FIOBJ_T_FALSE: fiobj_str_write(dest, "#f\r\n", 4); break;
  case FIOBJ_T_NUMBER:
    fiobj_str_write(dest, ":", 1);
    fiobj_str_write_i(dest, fiobj2i(obj));
    fiobj_str_write(dest, "\r\n", 2);
    break;
  case FIOBJ_T_FLOAT: {
    char tmp[64];
    size_t tmp_len = fio_ftoa(tmp, fiobj2f(obj), 10);
    fiobj_str_write(dest, ",", 1);
    fiobj_str_write(dest, tmp, tmp_len);
    fiobj_str_write(dest, "\r\n", 2);
    break;
  }
  case FIOBJ_T_ARRAY:
    fiobj_str_write(dest, "*", 1);
    fiobj_str_write_i(dest, (int64_t)fiobj_array_count(obj));
    fiobj_str_write(dest, "\r\n", 2);
    for (size_t i = 0; i < fiobj_array_count(obj); ++i) {
      fio___redis_fiobj2resp(dest, fiobj_array_get(obj, (int32_t)i));
    }
    break;
  case FIOBJ_T_HASH:
    fiobj_str_write(dest, "%", 1);
    fiobj_str_write_i(dest, (int64_t)fiobj_hash_count(obj));
    fiobj_str_write(dest, "\r\n", 2);
    FIO_MAP_EACH(fiobj_hash, obj, pos) {
      fio___redis_fiobj2resp(dest, pos.key);
      fio___redis_fiobj2resp(dest, pos.value);
    }
    break;
  case FIOBJ_T_STRING:
  default:
    s = fiobj2cstr(obj);
    fiobj_str_write(dest, "$", 1);
    fiobj_str_write_i(dest, (int64_t)s.len);
    fiobj_str_write(dest, "\r\n", 2);
    fiobj_str_write(dest, s.buf, s.len);
    fiobj_str_write(dest, "\r\n", 2);
    break;
  }
}

/* *****************************************************************************
Reference Counting and Cleanup

Reference counting ownership model (via FIO_REF macros):
- fio_redis_new():  ref = 1 (caller's reference)
- fio_redis_dup():  ref += 1
- fio_redis_free(): ref -= 1, if ref==0 calls fio___redis_destroy()

Internal reference management:
- fio_io_defer(fio___redis_connect, ...): ref += 1 before, ref -= 1 after
- fio_io_defer(fio___redis_perform_callback, ...): ref += 1 before, ref -= 1
after
- on_close callbacks: ref -= 1 (balances the connect ref)

Pub/Sub integration (NO ref changes):
- fio_pubsub_engine_attach(): does NOT increment ref
- fio_pubsub_engine_detach(): does NOT decrement ref
- on_detached callback: only marks engine as detached, does NOT free
***************************************************************************** */

FIO_SFUNC void fio___redis_connection_reset(fio_redis_connection_s *conn) {
  conn->buf_pos = 0;
  /* Clean up any partially parsed streaming string */
  if (conn->parser.streaming_string && conn->parser.streaming_string_ctx) {
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
  conn->parser = (fio_resp3_parser_s){0};
  conn->io = NULL;
}

/**
 * Internal: Destroy callback for FIO_REF.
 * Called when reference count reaches 0.
 */
FIO_SFUNC void fio___redis_destroy(fio_redis_engine_s *r) {
  FIO_LOG_DEBUG("(redis) destroying engine for %s:%s", r->address, r->port);

  /* Remove fork callback to prevent use-after-free */
  fio_state_callback_remove(FIO_CALL_IN_CHILD, fio___redis_on_fork, r);

  /* Free remaining resources - invoke callbacks to release IPC references */
  fiobj_free(r->last_channel);
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

/**
 * Internal: Deferred task to close connections and release reference.
 * Runs on IO thread to ensure thread-safety.
 *
 * IMPORTANT: fio_io_close_now() triggers on_close callbacks synchronously,
 * which call fio___redis_free(). We must capture IO handles before closing
 * to avoid use-after-free if the engine is freed during the first close.
 */
FIO_SFUNC void fio___redis_free_task(void *engine_, void *ignr_) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine_;
  (void)ignr_;

  /* Capture IO handles before closing - on_close may free the engine */
  fio_io_s *pub_io = r->pub_conn.io;
  fio_io_s *sub_io = r->sub_conn.io;

  /* Close connections - this will trigger on_close callbacks which will
   * release the connection references and eventually free the engine */
  if (pub_io)
    fio_io_close_now(pub_io);
  if (sub_io)
    fio_io_close_now(sub_io);

  /* Release the reference held for this deferred task */
  fio___redis_free(r);
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
  if (!r->pub_sent && !FIO_LIST_IS_EMPTY(&r->cmd_queue) && r->pub_conn.io) {
    r->pub_sent = 1;
    fio_redis_cmd_s *cmd =
        FIO_PTR_FROM_FIELD(fio_redis_cmd_s, node, r->cmd_queue.next);
    fio_io_write(r->pub_conn.io, cmd->cmd, cmd->cmd_len);
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

/**
 * Handle message on subscription connection.
 * Called from IO thread via on_data callback - no lock needed.
 */
FIO_SFUNC void fio___redis_on_sub_message(fio_redis_engine_s *r, FIOBJ msg) {
  if (FIOBJ_TYPE(msg) != FIOBJ_T_ARRAY) {
    /* Likely a PONG or status response */
    fio_str_info_s s = fiobj2cstr(msg);
    if (s.len != 4 || s.buf[0] != 'P') {
      FIO_LOG_WARNING("(redis) unexpected subscription data: %.*s",
                      (int)s.len,
                      s.buf);
    }
    return;
  }

  size_t count = fiobj_array_count(msg);
  if (count < 3)
    return;

  fio_str_info_s type = fiobj2cstr(fiobj_array_get(msg, 0));

  if (type.len == 7 && !FIO_MEMCMP(type.buf, "message", 7)) {
    /* Regular message: ["message", channel, data] */
    FIOBJ channel = fiobj_array_get(msg, 1);
    FIOBJ data = fiobj_array_get(msg, 2);
    fiobj_free(r->last_channel);
    r->last_channel = fiobj_dup(channel);
    fio_str_info_s ch = fiobj2cstr(channel);
    fio_str_info_s m = fiobj2cstr(data);
    fio_pubsub_publish(.channel = FIO_BUF_INFO2(ch.buf, ch.len),
                       .message = FIO_BUF_INFO2(m.buf, m.len),
                       .engine = fio_pubsub_engine_ipc());
  } else if (type.len == 8 && !FIO_MEMCMP(type.buf, "pmessage", 8) &&
             count >= 4) {
    /* Pattern message: ["pmessage", pattern, channel, data] */
    FIOBJ channel = fiobj_array_get(msg, 2);
    FIOBJ data = fiobj_array_get(msg, 3);
    /* Deduplicate if same channel as last message */
    if (!fiobj_is_eq(r->last_channel, channel)) {
      fio_str_info_s ch = fiobj2cstr(channel);
      fio_str_info_s m = fiobj2cstr(data);
      fio_pubsub_publish(.channel = FIO_BUF_INFO2(ch.buf, ch.len),
                         .message = FIO_BUF_INFO2(m.buf, m.len),
                         .engine = fio_pubsub_engine_ipc());
    }
  }
  /* Ignore subscribe/unsubscribe confirmations */
}

/* *****************************************************************************
Protocol Callbacks
***************************************************************************** */

FIO_SFUNC void fio___redis_on_attach(fio_io_s *io);
FIO_SFUNC void fio___redis_on_data(fio_io_s *io);
FIO_SFUNC void fio___redis_on_close_pub(void *buffer, void *udata);
FIO_SFUNC void fio___redis_on_close_sub(void *buffer, void *udata);
FIO_SFUNC void fio___redis_on_timeout(fio_io_s *io);
FIO_SFUNC void fio___redis_connect(void *engine_, void *conn_);

/** Protocol for publishing connection */
static fio_io_protocol_s FIO___REDIS_PUB_PROTOCOL = {
    .on_attach = fio___redis_on_attach,
    .on_data = fio___redis_on_data,
    .on_close = fio___redis_on_close_pub,
    .on_timeout = fio___redis_on_timeout,
    .timeout = 300000, /* 5 minutes default */
};

/** Protocol for subscription connection */
static fio_io_protocol_s FIO___REDIS_SUB_PROTOCOL = {
    .on_attach = fio___redis_on_attach,
    .on_data = fio___redis_on_data,
    .on_close = fio___redis_on_close_sub,
    .on_timeout = fio___redis_on_timeout,
    .timeout = 300000,
};

/**
 * Called when connection is attached to the protocol (connection ready).
 * Runs on IO thread - no lock needed.
 */
FIO_SFUNC void fio___redis_on_attach(fio_io_s *io) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)fio_io_udata(io);
  if (!r)
    return;

  /* Determine which connection this is based on protocol */
  fio_io_protocol_s *pr = fio_io_protocol(io);
  int is_sub = (pr == &FIO___REDIS_SUB_PROTOCOL);
  fio_redis_connection_s *conn = is_sub ? &r->sub_conn : &r->pub_conn;

  /* Store the IO handle now that connection is ready */
  conn->io = io;
  conn->is_sub = (uint8_t)is_sub;

  /* Send AUTH if configured.
   * For pub connection: queue as a proper command so its reply is consumed
   * and doesn't desynchronize the command queue.
   * For sub connection: direct write is fine since non-array replies are
   * ignored in fio___redis_on_sub_message. */
  if (!is_sub && r->auth_cmd_len) {
    fio_redis_cmd_s *auth_cmd = (fio_redis_cmd_s *)
        FIO_MEM_REALLOC(NULL, 0, sizeof(*auth_cmd) + r->auth_cmd_len + 1, 0);
    if (auth_cmd) {
      FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);
      *auth_cmd = (fio_redis_cmd_s){.cmd_len = r->auth_cmd_len};
      FIO_MEMCPY(auth_cmd->cmd, r->auth_cmd, r->auth_cmd_len);
      FIO_LIST_PUSH(&r->cmd_queue, &auth_cmd->node);
    }
  }
  if (is_sub && r->auth_cmd_len) {
    fio_io_write(io, r->auth_cmd, r->auth_cmd_len);
  }

  if (is_sub) {
    FIO_LOG_DEBUG("(redis) subscription connection established to %s:%s",
                  r->address,
                  r->port);
    /* If attached to pub/sub and not detaching, re-attach to trigger
     * resubscription. Don't re-attach if detaching to avoid adding the
     * engine back to the pubsub engines list after detach. */
    if (r->attached && r->running && !r->detaching)
      fio_pubsub_engine_attach(&r->engine);
  } else {
    FIO_LOG_DEBUG("(redis) publishing connection established to %s:%s",
                  r->address,
                  r->port);
    /* Send any queued commands - no lock needed, we're on IO thread */
    r->pub_sent = 0;
    fio___redis_send_next_cmd(r);

    /* Start subscription connection only if attached, running, and reactor
     * is still running */
    if (r->attached && r->running && fio_io_is_running() && !r->sub_conn.io) {
      fio___redis_dup(r); /* Increment ref for deferred task */
      fio_io_defer(fio___redis_connect, r, &r->sub_conn);
    }
  }
}

FIO_SFUNC void fio___redis_on_data(fio_io_s *io) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)fio_io_udata(io);
  if (!r)
    return;

  fio_redis_connection_s *conn;
  uint8_t *buf;
  int is_sub;

  if (io == r->sub_conn.io) {
    conn = &r->sub_conn;
    buf = r->buf + FIO_REDIS_READ_BUFFER;
    is_sub = 1;
  } else {
    conn = &r->pub_conn;
    buf = r->buf;
    is_sub = 0;
  }

  /* Read data */
  size_t available = FIO_REDIS_READ_BUFFER - conn->buf_pos;
  size_t len = fio_io_read(io, buf + conn->buf_pos, available);
  if (!len)
    return;

  conn->buf_pos += (uint16_t)len;

  /* Parse RESP data - loop to process all complete messages in buffer */
  for (;;) {
    fio_resp3_result_s result = fio_resp3_parse(&conn->parser,
                                                &FIO___REDIS_RESP3_CALLBACKS,
                                                buf,
                                                conn->buf_pos);

    if (result.err) {
      FIO_LOG_ERROR("(redis) parser error - closing connection");
      fio_io_close_now(io);
      return;
    }

    /* Move unconsumed data to start of buffer */
    if (result.consumed && result.consumed < conn->buf_pos) {
      size_t remaining = conn->buf_pos - result.consumed;
      FIO_MEMMOVE(buf, buf + result.consumed, remaining);
      conn->buf_pos = (uint16_t)remaining;
    } else if (result.consumed) {
      conn->buf_pos = 0;
    }

    if (result.obj) {
      FIOBJ msg = (FIOBJ)result.obj;
      if (is_sub) {
        fio___redis_on_sub_message(r, msg);
      } else {
        fio___redis_on_pub_message(r, msg);
      }
      fiobj_free(msg);
      /* Reset parser for next message and continue processing */
      conn->parser = (fio_resp3_parser_s){0};
      continue;
    }

    /* No complete message available, exit loop */
    break;
  }
}

/** Internal helper for connection close handling */
FIO_SFUNC void fio___redis_on_close_internal(fio_redis_engine_s *r,
                                             fio_redis_connection_s *conn,
                                             int is_sub) {
  fio___redis_connection_reset(conn);

  if (r->running && fio_io_is_running()) {
    FIO_LOG_WARNING("(redis) %s connection lost, reconnecting...",
                    is_sub ? "subscription" : "publishing");
    /* Increment ref for deferred reconnect task */
    fio___redis_dup(r);
    fio_io_defer(fio___redis_connect, r, conn);
  }
  /* Release the reference held by this connection */
  fio___redis_free(r);
}

FIO_SFUNC void fio___redis_on_close_pub(void *buffer, void *udata) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)udata;
  (void)buffer;
  if (!r)
    return;
  fio___redis_on_close_internal(r, &r->pub_conn, 0);
}

FIO_SFUNC void fio___redis_on_close_sub(void *buffer, void *udata) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)udata;
  (void)buffer;
  if (!r)
    return;
  fio___redis_on_close_internal(r, &r->sub_conn, 1);
}

/**
 * Handle connection timeout.
 * Runs on IO thread - no lock needed for cmd_queue access.
 */
FIO_SFUNC void fio___redis_on_timeout(fio_io_s *io) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)fio_io_udata(io);
  if (!r)
    return;

  if (io == r->sub_conn.io) {
    /* Subscription connection - just send PING */
    fio_io_write(io, (char *)"*1\r\n$4\r\nPING\r\n", 14);
  } else {
    /* Publishing connection - check for stuck commands */
    if (!FIO_LIST_IS_EMPTY(&r->cmd_queue)) {
      FIO_LOG_WARNING("(redis) server unresponsive, disconnecting");
      fio_io_close_now(io);
      return;
    }
    /* Queue a PING command */
    fio_redis_cmd_s *cmd =
        (fio_redis_cmd_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*cmd) + 15, 0);
    if (cmd) {
      FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);
      *cmd = (fio_redis_cmd_s){.cmd_len = 14};
      FIO_MEMCPY(cmd->cmd, "*1\r\n$4\r\nPING\r\n", 15);
      fio___redis_attach_cmd(r, cmd);
    }
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

  int is_sub = (conn == &r->sub_conn);
  conn->is_sub = is_sub;
  fio_io_protocol_s *protocol =
      is_sub ? &FIO___REDIS_SUB_PROTOCOL : &FIO___REDIS_PUB_PROTOCOL;

  /* Build URL */
  char url[512];
  size_t url_len = 0;
  url_len += (size_t)snprintf(url + url_len,
                              sizeof(url) - url_len,
                              "tcp://%s:%s",
                              r->address,
                              r->port);

  FIO_LOG_DEBUG("(redis) connecting to %s:%s (%s)",
                r->address,
                r->port,
                is_sub ? "subscription" : "publishing");

  /* Start async connection - on_attach will be called when ready */
  conn->io =
      fio_io_connect(url, .protocol = protocol, .udata = r, .timeout = 30000);
  if (!conn->io) {
    FIO_LOG_ERROR("(redis) failed to initiate connection to %s:%s",
                  r->address,
                  r->port);
    /* Retry after delay using proper timer callback signature */
    fio_io_run_every(.fn = fio___redis_connect_timer,
                     .udata1 = r,
                     .udata2 = conn,
                     .every = 1000,
                     .repetitions = 1);
    return;
  }
  /* Connection initiated - the ref added before connect is now held by the
   * connection. It will be released in on_close when the connection closes. */
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

  /* Serialize reply to RESP format */
  FIOBJ resp = fiobj_str_new_buf(256);
  if (reply == FIOBJ_INVALID) {
    /* No reply / error - send null */
    fiobj_str_write(resp, "$-1\r\n", 5);
  } else {
    fio___redis_fiobj2resp(resp, reply);
  }
  fio_str_info_s s = fiobj2cstr(resp);

  /* Send reply back to worker via IPC.
   * Echo the 16-byte header from the original request so the worker
   * can recover callback + engine pointers from the reply data. */
  fio_ipc_reply(ipc,
                .data = FIO_IPC_DATA(
                    FIO_BUF_INFO2(ipc->data, FIO___REDIS_IPC_SEND_HEADER),
                    FIO_BUF_INFO2(s.buf, s.len)),
                .done = 1);
  fiobj_free(resp);
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
                      FIO_BUF_INFO2("$-1\r\n", 5)),
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
                      FIO_BUF_INFO2("$-1\r\n", 5)),
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
 * IMPORTANT: This callback does NOT free memory. It only marks the engine
 * as detached. Memory is freed by fio_redis_free() when ref reaches 0.
 */
FIO_SFUNC void fio___redis_detached(const fio_pubsub_engine_s *eng) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)eng;
  FIO_LOG_DEBUG("(redis) engine detached from pub/sub");
  r->attached = 0;
  r->detaching = 0; /* Detach complete */
  /* Close subscription connection since we're no longer attached */
  if (r->sub_conn.io) {
    fio_io_close_now(r->sub_conn.io);
  }
  /* NOTE: Do NOT call fio___redis_free() here. The "attached" state is not a
   * reference-counted ownership. The caller's fio_redis_free() handles all
   * reference counting via fio___redis_dup/fio___redis_free pairs. */
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

  /* Mark as attached and start subscription connection if needed */
  if (!r->attached) {
    r->attached = 1;
    if (r->running && !r->sub_conn.io) {
      fio___redis_dup(r);
      fio_io_defer(fio___redis_connect, r, &r->sub_conn);
    }
  }

  if (!r->sub_conn.io)
    return;

  /* Build SUBSCRIBE command */
  FIOBJ cmd = fiobj_str_new_buf(32 + channel.len);
  fiobj_str_write(cmd, "*2\r\n$9\r\nSUBSCRIBE\r\n$", 20);
  fiobj_str_write_i(cmd, (int64_t)channel.len);
  fiobj_str_write(cmd, "\r\n", 2);
  fiobj_str_write(cmd, channel.buf, channel.len);
  fiobj_str_write(cmd, "\r\n", 2);

  fio_str_info_s s = fiobj2cstr(cmd);
  fio_io_write(r->sub_conn.io, s.buf, s.len);
  fiobj_free(cmd);
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

  /* Mark as attached and start subscription connection if needed */
  if (!r->attached) {
    r->attached = 1;
    if (r->running && !r->sub_conn.io) {
      fio___redis_dup(r);
      fio_io_defer(fio___redis_connect, r, &r->sub_conn);
    }
  }

  if (!r->sub_conn.io)
    return;

  /* Build PSUBSCRIBE command */
  FIOBJ cmd = fiobj_str_new_buf(32 + channel.len);
  fiobj_str_write(cmd, "*2\r\n$10\r\nPSUBSCRIBE\r\n$", 22);
  fiobj_str_write_i(cmd, (int64_t)channel.len);
  fiobj_str_write(cmd, "\r\n", 2);
  fiobj_str_write(cmd, channel.buf, channel.len);
  fiobj_str_write(cmd, "\r\n", 2);

  fio_str_info_s s = fiobj2cstr(cmd);
  fio_io_write(r->sub_conn.io, s.buf, s.len);
  fiobj_free(cmd);
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

  /* Skip if not attached or no connection */
  if (!r->attached || !r->sub_conn.io)
    return;

  /* Build UNSUBSCRIBE command */
  FIOBJ cmd = fiobj_str_new_buf(32 + channel.len);
  fiobj_str_write(cmd, "*2\r\n$11\r\nUNSUBSCRIBE\r\n$", 23);
  fiobj_str_write_i(cmd, (int64_t)channel.len);
  fiobj_str_write(cmd, "\r\n", 2);
  fiobj_str_write(cmd, channel.buf, channel.len);
  fiobj_str_write(cmd, "\r\n", 2);

  fio_str_info_s s = fiobj2cstr(cmd);
  fio_io_write(r->sub_conn.io, s.buf, s.len);
  fiobj_free(cmd);
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

  /* Skip if not attached or no connection */
  if (!r->attached || !r->sub_conn.io)
    return;

  /* Build PUNSUBSCRIBE command */
  FIOBJ cmd = fiobj_str_new_buf(32 + channel.len);
  fiobj_str_write(cmd, "*2\r\n$12\r\nPUNSUBSCRIBE\r\n$", 24);
  fiobj_str_write_i(cmd, (int64_t)channel.len);
  fiobj_str_write(cmd, "\r\n", 2);
  fiobj_str_write(cmd, channel.buf, channel.len);
  fiobj_str_write(cmd, "\r\n", 2);

  fio_str_info_s s = fiobj2cstr(cmd);
  fio_io_write(r->sub_conn.io, s.buf, s.len);
  fiobj_free(cmd);
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
 */
FIO_SFUNC void fio___redis_on_fork(void *engine_) {
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine_;
  r->engine.publish = fio___redis_publish_worker;
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

  size_t auth_len = args.auth_len;
  if (args.auth && !auth_len)
    auth_len = strlen(args.auth);

  /* Calculate AUTH command size if needed */
  size_t auth_cmd_len = 0;
  if (auth_len) {
    /* *2\r\n$4\r\nAUTH\r\n$<len>\r\n<password>\r\n */
    auth_cmd_len = 18 + 20 + auth_len; /* generous estimate */
  }

  /* Allocate engine using FIO_REF (includes ref count, starts at 1).
   * Note: buf[FIO_REDIS_READ_BUFFER * 2] is already inside the struct,
   * so we only need flex space for the strings. */
  size_t flex_size = host_len + 1 + port_len + 1 + auth_cmd_len;
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
      .ping_interval = args.ping_interval,
      .running = 1,
  };

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

  /* Build AUTH command if needed */
  if (auth_len) {
    r->auth_cmd = str_ptr;
    size_t pos = 0;
    FIO_MEMCPY(r->auth_cmd + pos, "*2\r\n$4\r\nAUTH\r\n$", 15);
    pos += 15;
    pos += (size_t)fio_ltoa(r->auth_cmd + pos, (int64_t)auth_len, 10);
    r->auth_cmd[pos++] = '\r';
    r->auth_cmd[pos++] = '\n';
    FIO_MEMCPY(r->auth_cmd + pos, args.auth, auth_len);
    pos += auth_len;
    r->auth_cmd[pos++] = '\r';
    r->auth_cmd[pos++] = '\n';
    r->auth_cmd[pos] = 0;
    r->auth_cmd_len = pos;
  }

  /* Set timeout based on ping interval */
  uint32_t timeout_ms = (uint32_t)args.ping_interval * 1000;
  if (timeout_ms > FIO_IO_TIMEOUT_MAX)
    timeout_ms = FIO_IO_TIMEOUT_MAX;
  FIO___REDIS_PUB_PROTOCOL.timeout = timeout_ms;
  FIO___REDIS_SUB_PROTOCOL.timeout = timeout_ms;

  /* Start connection - increment ref for deferred task.
   * The deferred connect will check fio_io_is_worker() and skip on workers. */
  fio___redis_dup(r);
  fio_io_defer(fio___redis_connect, r, &r->pub_conn);

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
 * Decrements the reference count. When count reaches 0, destroys the engine.
 * Thread-safe - defers connection closure to the IO thread.
 */
SFUNC void fio_redis_free(fio_pubsub_engine_s *engine) {
  if (!engine)
    return;
  fio_redis_engine_s *r = (fio_redis_engine_s *)engine;

  /* Mark as not running to prevent reconnection attempts and re-attach.
   * This is safe to do from any thread as it's a simple flag. */
  r->running = 0;

  /* Mark as detaching to prevent re-attach in on_attach callback */
  r->detaching = 1;

  /* Always try to detach from pubsub to ensure engine is removed from the
   * engines list. This prevents use-after-free during exit cleanup where
   * pubsub cleanup may try to call detached() on an already-freed engine.
   * Note: fio_pubsub_engine_detach is safe to call even if not attached -
   * it will simply not find the engine in the list and not call detached().
   *
   * We hold an extra reference during detach because detach may call
   * fio___redis_detached which calls fio___redis_free. Without the extra
   * reference, the engine could be freed during detach. */
  fio___redis_dup(r);
  fio_pubsub_engine_detach(engine);

  /* Defer connection closure to IO thread for thread-safety.
   * We already have one extra reference from above, use it for the task. */
  fio_io_defer(fio___redis_free_task, r, NULL);

  /* Release caller's reference */
  fio___redis_free(r);
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

  /* Convert command to RESP format */
  FIOBJ resp = fiobj_str_new_buf(256);
  fio___redis_fiobj2resp(resp, command);
  fio_str_info_s s = fiobj2cstr(resp);

  /* Worker path: forward via IPC to master.
   * Use !fio_io_is_master() so single-process mode takes the master path.
   *
   * IPC data layout: [callback(8) | engine(8) | RESP command bytes]
   * ipc->udata carries the user's udata directly (no heap allocation).
   * Master echoes the 16-byte header in its reply so the worker's on_done
   * handler can recover callback + engine pointers from the reply data. */
  if (!fio_io_is_master()) {
    fio_ipc_call(.call = fio___redis_ipc_send_on_master,
                 .on_done = fio___redis_ipc_send_on_done,
                 .udata = udata,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)&callback, sizeof(callback)),
                     FIO_BUF_INFO2((char *)&r, sizeof(r)),
                     FIO_BUF_INFO2(s.buf, s.len)));
    fiobj_free(resp);
    return 0;
  }

  /* Master path: queue command directly */
  fio_redis_cmd_s *cmd =
      (fio_redis_cmd_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*cmd) + s.len + 1, 0);
  if (!cmd) {
    fiobj_free(resp);
    return -1;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_cmd);

  *cmd = (fio_redis_cmd_s){
      .callback = callback,
      .udata = udata,
      .cmd_len = s.len,
  };
  FIO_MEMCPY(cmd->cmd, s.buf, s.len + 1);
  fiobj_free(resp);

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
