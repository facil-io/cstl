/* *****************************************************************************
Redis Security Regression Tests (report 2026-10-28 007, stateless subset)

Deterministic, reactor-free Redis ownership and trust-boundary checks.
A counting allocator verifies live blocks. Checks use FIO_ASSERT.
***************************************************************************** */
/* This TU checks leaks with its own counting allocator. */
#undef DEBUG
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- counting allocator (all modules; single include pass) ---- */
static size_t sr_live;    /* live tracked blocks */

static void sr_die(const char *msg) {
  fprintf(stderr, "security-redis allocator: %s\n", msg);
  abort();
}

static void *sr_realloc(void *old, size_t size, size_t copy) {
  void *p = malloc(size ? size : 1);
  if (!p)
    return NULL;
  if (old) {
    memcpy(p, old, copy < size ? copy : size);
    free(old);
  } else {
    ++sr_live;
  }
  return p;
}

static void sr_free(void *p) {
  if (!p)
    return;
  if (!sr_live)
    sr_die("free without a live allocation");
  --sr_live;
  free(p);
}

/* Aligned blocks: over-allocate, keep the base pointer before the block. */
static void *sr_realloc_aligned(void *old, size_t size, size_t copy, size_t a) {
  if (a < sizeof(void *))
    a = sizeof(void *);
  char *base = (char *)malloc(size + a + sizeof(void *));
  if (!base)
    return NULL;
  uintptr_t u = (uintptr_t)(base + sizeof(void *));
  u = (u + (a - 1)) & ~(uintptr_t)(a - 1);
  void *p = (void *)u;
  ((void **)p)[-1] = base;
  if (old) {
    memcpy(p, old, copy < size ? copy : size);
    free(((void **)old)[-1]);
  } else {
    ++sr_live;
  }
  return p;
}

static void sr_free_aligned(void *p) {
  if (!p)
    return;
  if (!sr_live)
    sr_die("aligned free without a live allocation");
  --sr_live;
  free(((void **)p)[-1]);
}

#ifndef FIO_MEMORY_DISABLE
#define FIO_MEMORY_DISABLE 1
#endif
#define FIO_STRLEN strlen
#define FIO_MEM_REALLOC(p, o, n, c) sr_realloc((p), (n), (c))
#define FIO_MEM_FREE(p, n)          sr_free((p))
#define FIO_MEM_REALLOC_IS_SAFE     0
#define FIO_MEM_ALIGNMENT_SIZE      16
#define FIO_MEM_REALLOC_ALIGNED(p, o, n, c, a)                                 \
  ((p) = sr_realloc_aligned((p), (n), (c), (a)))
#define FIO_MEM_FREE_ALIGNED(p, n)   sr_free_aligned((p))
#define FIO_MEM_ALLOC_SIZE(n)        ((size_t)(n))
#define FIO_MEM_REALLOC_(p, o, n, c) sr_realloc((p), (n), (c))
#define FIO_MEM_FREE_(p, n)          sr_free((p))
#define FIO_MEM_REALLOC_IS_SAFE_     0
#define FIO_MEM_ALIGNMENT_SIZE_      16
#define FIO_MEM_REALLOC_ALIGNED_(p, o, n, c, a)                                \
  ((p) = sr_realloc_aligned((p), (n), (c), (a)))
#define FIO_MEM_FREE_ALIGNED_(p, n) sr_free_aligned((p))

#define FIO_LOG
#define FIO_CORE
#define FIO_SOCK
#define FIO_TIME
#define FIO_THREADS
#define FIO_FIOBJ
#define FIO_PUBSUB
#define FIO_RESP3
#define FIO_REDIS
#include "fio-stl/include.h"

/* *****************************************************************************
Helpers
***************************************************************************** */

static void sr_drain(void) { fio_queue_perform_all(fio_io_queue()); }

/** Reply recorder (one per command). */
typedef struct {
  size_t calls;
  size_t invalid;
  FIOBJ last; /* dup of the last valid reply (owned by the test) */
  fio_pubsub_engine_s *engine;
} sr_rec_s;

static void sr_rec_cb(fio_pubsub_engine_s *e, FIOBJ reply, void *udata) {
  sr_rec_s *rec = (sr_rec_s *)udata;
  ++rec->calls;
  rec->engine = e;
  if (reply == FIOBJ_INVALID) {
    ++rec->invalid;
    return;
  }
  fiobj_free(rec->last);
  rec->last = fiobj_dup(reply);
}

static void sr_rec_clear(sr_rec_s *rec) {
  fiobj_free(rec->last);
  *rec = (sr_rec_s){0};
}

/** Publish recorder for push-capture tests. */
static struct {
  size_t count;
  char ch[64];
  size_t ch_len;
  char *msg;
  size_t msg_len;
} sr_pub;

static void sr_pub_hook(void *udata, fio_buf_info_s ch, fio_buf_info_s msg) {
  (void)udata;
  ++sr_pub.count;
  sr_pub.ch_len = ch.len < sizeof(sr_pub.ch) ? ch.len : sizeof(sr_pub.ch);
  memcpy(sr_pub.ch, ch.buf, sr_pub.ch_len);
  free(sr_pub.msg); /* untracked test storage */
  sr_pub.msg = (char *)malloc(msg.len + 1);
  FIO_ASSERT(sr_pub.msg, "test storage");
  memcpy(sr_pub.msg, msg.buf, msg.len);
  sr_pub.msg_len = msg.len;
}

static void sr_pub_reset(void) {
  free(sr_pub.msg);
  memset(&sr_pub, 0, sizeof(sr_pub));
}

/** Reactor-free engine (real FIO_REF allocation, unregistered, no io). */
static fio_redis_engine_s *sr_engine_new(void) {
  fio_redis_engine_s *r = fio___redis_new(0);
  FIO_ASSERT_ALLOC(r);
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_engine);
  *r = (fio_redis_engine_s){
      .cmd_queue = FIO_LIST_INIT(r->cmd_queue),
      .ping_interval = 30,
      .state = FIO_REDIS_STATE_CONNECTING,
  };
  r->conn.ps.payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT;
  r->conn.parser.udata = &r->conn.ps;
  r->conn.ps.publish_hook = sr_pub_hook;
  return r;
}

/** Queues a lock-step command with a recorder; marks it in flight. */
static fio_redis_cmd_s *sr_queue(fio_redis_engine_s *r,
                                 const char *bytes,
                                 sr_rec_s *rec) {
  fio_redis_cmd_s *c = fio___redis_queue_cmd_bytes(r,
                                                   bytes,
                                                   strlen(bytes),
                                                   rec ? sr_rec_cb : NULL,
                                                   rec,
                                                   0);
  FIO_ASSERT_ALLOC(c);
  return c;
}

/** Feeds bytes through the real parse loop. Returns -1 if parsing stopped
 * (parse/allocation failure or the engine stopped), 0 otherwise. */
static int sr_feed(fio_redis_engine_s *r, const char *data, size_t len) {
  FIO_ASSERT(len <= FIO_REDIS_READ_BUFFER - r->conn.buf_pos, "feed overflow");
  memcpy(r->buf + r->conn.buf_pos, data, len);
  r->conn.buf_pos += (FIO___REDIS_BUF_POS_T)len;
  fio___redis_parse_buffered(r, &r->conn, r->buf, NULL);
  return (r->conn.ps.alloc_failed || r->conn.ps.limit_exceeded || r->state == FIO_REDIS_STATE_ERROR)
             ? -1
             : 0;
}

static size_t sr_queue_len(fio_redis_engine_s *r) {
  size_t n = 0;
  FIO_LIST_EACH(fio_redis_cmd_s, node, &r->cmd_queue, pos) { ++n; }
  return n;
}

/** Authors (never sends) an IPC message with a header and a payload. */
static fio_ipc_s *sr_ipc(void (*call)(fio_ipc_s *),
                         void (*on_done)(fio_ipc_s *),
                         void *udata,
                         const void *a,
                         size_t alen,
                         const void *b,
                         size_t blen) {
  fio_ipc_s *ipc;
  if (blen)
    ipc = fio_ipc_new(.call = call,
                      .on_done = on_done,
                      .udata = udata,
                      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)a, alen),
                                           FIO_BUF_INFO2((char *)b, blen)));
  else
    ipc = fio_ipc_new(.call = call,
                      .on_done = on_done,
                      .udata = udata,
                      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)a, alen)));
  FIO_ASSERT_ALLOC(ipc);
  return ipc;
}

/** Stand-in worker engine pointer echoed through the raw IPC header. */
static fio_pubsub_engine_s sr_worker_engine;

/**
 * Runs the real worker-side fio___redis_ipc_send_on_done on a reply message
 * using the raw [callback (8) | engine (8) | RESP] header. Returns a dup of
 * the delivered reply (FIOBJ_INVALID if the reply was rejected) and stores
 * the callback count in `calls`.
 */
static FIOBJ sr_on_done_reply(const char *data, size_t len, size_t *calls) {
  sr_rec_s rec = {0};
  void (*cb)(fio_pubsub_engine_s *, FIOBJ, void *) = sr_rec_cb;
  fio_pubsub_engine_s *eng = &sr_worker_engine;
  char hdr[FIO___REDIS_IPC_SEND_HEADER];
  FIO_ASSERT(sizeof(cb) == 8 && sizeof(eng) == 8, "raw IPC header layout");
  memcpy(hdr, &cb, 8);
  memcpy(hdr + 8, &eng, 8);
  fio_ipc_s *ipc =
      sr_ipc(NULL, fio___redis_ipc_send_on_done, &rec, hdr, 16, data, len);
  fio___redis_ipc_send_on_done(ipc);
  fio_ipc_free(ipc); /* deferred to the IO queue */
  sr_drain();        /* release it now: callers count live blocks */
  FIO_ASSERT(rec.calls <= 1, "worker callback ran more than once");
  FIO_ASSERT(!rec.calls || rec.engine == eng, "engine pointer not echoed");
  *calls = rec.calls;
  FIOBJ o = rec.last;
  rec.last = FIOBJ_INVALID;
  sr_rec_clear(&rec);
  return o;
}

/** Decodes `data` through the worker reply path: the expected reply. */
/* *****************************************************************************
Push capture: attribute containers inside push frames do not leak (extra fix)
***************************************************************************** */

static void test_push_attribute_no_leak(void) {
  fprintf(stderr, "* push-frame attribute containers are released...\n");
  static const char frame[] = ">3\r\n$7\r\nmessage\r\n"
                              "|1\r\n+k\r\n*2\r\n:1\r\n%1\r\n+a\r\n+b\r\n"
                              "$2\r\nch\r\n"
                              "|1\r\n~1\r\n+s\r\n*1\r\n+v\r\n"
                              "$2\r\nhi\r\n";
  const size_t live0 = sr_live;
  sr_pub_reset();
  fio_redis_engine_s *r = sr_engine_new();
  FIO_ASSERT(!sr_feed(r, frame, sizeof(frame) - 1), "push must parse");
  FIO_ASSERT(sr_pub.count == 1 && sr_pub.ch_len == 2 &&
                 !memcmp(sr_pub.ch, "ch", 2) && sr_pub.msg_len == 2 &&
                 !memcmp(sr_pub.msg, "hi", 2),
             "attributed push must publish ch/hi (count %zu)",
             sr_pub.count);
  FIO_ASSERT(!r->conn.buf_pos && !r->conn.parser.depth, "frame consumed");
  fio___redis_free(r);
  sr_drain();
  FIO_ASSERT(sr_live == live0, "attribute push leak (%zu)", sr_live - live0);
  sr_pub_reset();
}

/* *****************************************************************************
R-06: a pending zero-copy capture survives read-buffer compaction
***************************************************************************** */

static void test_r06_capture_freeze(void) {
  fprintf(stderr, "* R-06 zero-copy capture frozen before compaction...\n");
  enum { PAYLOAD = 6000 };
  char *frame = (char *)malloc(PAYLOAD + 128);
  FIO_ASSERT(frame, "test storage");
  int hdr = snprintf(frame,
                     128,
                     ">3\r\n$7\r\nmessage\r\n$2\r\nch\r\n$%d\r\n",
                     (int)PAYLOAD);
  FIO_ASSERT(hdr > 0 && hdr < 128, "header");
  memset(frame + hdr, 'p', PAYLOAD);
  const size_t live0 = sr_live;
  sr_pub_reset();
  fio_redis_engine_s *r = sr_engine_new();
  /* Everything except the payload's CRLF, then clobber the free buffer. */
  FIO_ASSERT(!sr_feed(r, frame, (size_t)hdr + PAYLOAD), "partial push");
  FIO_ASSERT(!sr_pub.count, "not yet complete");
  memset(r->buf + r->conn.buf_pos,
         'X',
         FIO_REDIS_READ_BUFFER - r->conn.buf_pos);
  FIO_ASSERT(!sr_feed(r, "\r\n", 2), "push completes");
  FIO_ASSERT(sr_pub.count == 1 && sr_pub.msg_len == PAYLOAD,
             "push must publish once (count %zu, len %zu)",
             sr_pub.count,
             sr_pub.msg_len);
  for (size_t i = 0; i < PAYLOAD; ++i)
    FIO_ASSERT(sr_pub.msg[i] == 'p', "payload corrupted at %zu", i);
  fio___redis_free(r);
  sr_drain();
  FIO_ASSERT(sr_live == live0, "capture leak (%zu)", sr_live - live0);
  sr_pub_reset();
  free(frame);
}

/* *****************************************************************************
R-01: a failed HELLO stops parsing; nothing behind it is matched or sent
***************************************************************************** */

static void test_r01_hello_failure(void) {
  fprintf(stderr, "* R-01 failed HELLO consumed synchronously...\n");
  const size_t live0 = sr_live;
  sr_rec_s set = {0};
  fio_redis_engine_s *r = sr_engine_new();
  sr_queue(r, "*3\r\n$3\r\nSET\r\n$1\r\nk\r\n$1\r\nv\r\n", &set);
  FIO_ASSERT(fio___redis_queue_cmd_bytes(r,
                                         "*2\r\n$5\r\nHELLO\r\n$1\r\n3\r\n",
                                         22,
                                         fio___redis_on_hello_reply,
                                         NULL,
                                         1),
             "HELLO queued");
  r->pub_sent = 1; /* HELLO written, awaiting its reply */
  static const char wire[] = "-ERR unknown command 'HELLO'\r\n+OK\r\n";
  int stop = sr_feed(r, wire, sizeof(wire) - 1);
  FIO_ASSERT(stop == -1, "a failed HELLO must stop the parse loop");
  FIO_ASSERT(r->state == FIO_REDIS_STATE_ERROR && !r->pub_sent, "engine stopped, nothing in flight");
  FIO_ASSERT(!set.calls && sr_queue_len(r) == 1,
             "SET must stay unsent and unmatched");
  FIO_ASSERT(r->conn.buf_pos == sizeof(wire) - 1,
             "no reply after the failed HELLO may be consumed");
  /* A reply with nothing in flight is ignored (never pops unsent SET). */
  r->conn.buf_pos = 0;
  r->state = FIO_REDIS_STATE_CONNECTING;
  FIO_ASSERT(!sr_feed(r, "+OK\r\n", 5), "unsolicited reply ignored");
  FIO_ASSERT(!set.calls && sr_queue_len(r) == 1, "unsolicited reply no pop");
  fio___redis_free(r);
  sr_drain();
  FIO_ASSERT(set.calls == 1 && set.invalid == 1, "SET failed exactly once");
  FIO_ASSERT(sr_live == live0, "leak (%zu)", sr_live - live0);
}

/* *****************************************************************************
R-07: a written command is failed on connection loss, never replayed
***************************************************************************** */

static void test_r07_no_replay(void) {
  fprintf(stderr, "* R-07 in-flight command failed on loss, unsent kept...\n");
  const size_t live0 = sr_live;
  sr_rec_s a = {0}, b = {0};
  fio_redis_engine_s *r = sr_engine_new();
  sr_queue(r, "*2\r\n$4\r\nINCR\r\n$1\r\nk\r\n", &a);
  sr_queue(r, "*2\r\n$4\r\nINCR\r\n$1\r\nj\r\n", &b);
  r->pub_sent = 1;
  r->state = FIO_REDIS_STATE_CONNECTED;
  /* Established-connection close path; the reactor is not running, so no
   * retry timer is scheduled. */
  fio___redis_on_close_internal(r, &r->conn);
  FIO_ASSERT(fio_redis_state(&r->engine) == FIO_REDIS_STATE_CONNECTING,
             "connection loss must reset the connection-state snapshot");
  sr_drain(); /* the failure callback may run inline or deferred */
  FIO_ASSERT(a.calls == 1 && a.invalid == 1, "written command failed once");
  FIO_ASSERT(!b.calls && sr_queue_len(r) == 1 && !r->pub_sent,
             "unsent command stays queued");
  fio___redis_free(r);
  sr_drain();
  FIO_ASSERT(a.calls == 1, "never replayed / failed twice");
  FIO_ASSERT(b.calls == 1 && b.invalid == 1, "unsent failed at stop");
  FIO_ASSERT(sr_live == live0, "leak (%zu)", sr_live - live0);
}

/* *****************************************************************************
R-07: a loss callback may release the caller's last engine reference
***************************************************************************** */

static size_t sr_free_on_loss_calls;
static size_t sr_free_on_loss_invalid;

static void sr_free_on_loss_cb(fio_pubsub_engine_s *e,
                               FIOBJ reply,
                               void *udata) {
  (void)udata;
  ++sr_free_on_loss_calls;
  sr_free_on_loss_invalid += (reply == FIOBJ_INVALID);
  fio_redis_free(e); /* drops the caller-owned (sole) reference */
}

static void test_r07_loss_callback_frees_engine(void) {
  fprintf(stderr, "* R-07 loss callback may free the engine...\n");
  const size_t live0 = sr_live;
  sr_free_on_loss_calls = 0;
  sr_free_on_loss_invalid = 0;
  fio_redis_engine_s *r = sr_engine_new(); /* sole caller reference */
  FIO_ASSERT(fio___redis_queue_cmd_bytes(r,
                                         "*2\r\n$4\r\nINCR\r\n$1\r\nk\r\n",
                                         21,
                                         sr_free_on_loss_cb,
                                         NULL,
                                         0),
             "command queued");
  r->pub_sent = 1; /* written, awaiting its reply */
  /* Reactor stopped: no retry. `r` must not be accessed after this call. */
  fio___redis_on_connect_failed(NULL, r);
  r = NULL;
  sr_drain();
  FIO_ASSERT(sr_free_on_loss_calls == 1 && sr_free_on_loss_invalid == 1,
             "loss callback must run exactly once with FIOBJ_INVALID (%zu)",
             sr_free_on_loss_calls);
  FIO_ASSERT(sr_live == live0, "leak (%zu)", sr_live - live0);
}

/* *****************************************************************************
R-03: bounded IPC publish headers (raw engine-pointer header)
***************************************************************************** */

static void test_r03_ipc_publish_bounds(void) {
  fprintf(stderr, "* R-03 IPC publish bounds...\n");
  const size_t live0 = sr_live;
  FIO_ASSERT(sizeof(fio___redis_ipc_publish_header_s) == 16,
             "fixed IPC publish header");
  fio_redis_engine_s *r = sr_engine_new();

  /* Malformed headers (lengths beyond the payload) are dropped. */
  fio___redis_ipc_publish_header_s h = {.engine = r,
                                        .channel_len = UINT32_MAX,
                                        .message_len = 2};
  fio_ipc_s *ipc =
      sr_ipc(fio___redis_ipc_publish_on_master, NULL, NULL, &h, 16, "chhi", 4);
  fio___redis_ipc_publish_on_master(ipc);
  fio_ipc_free(ipc);
  h.channel_len = 3;
  h.message_len = UINT32_MAX - 2; /* sum wraps in 32 bits */
  ipc = sr_ipc(fio___redis_ipc_publish_on_master, NULL, NULL, &h, 16, "chhi", 4);
  fio___redis_ipc_publish_on_master(ipc);
  fio_ipc_free(ipc);
  ipc = sr_ipc(fio___redis_ipc_publish_on_master, NULL, NULL, &h, 15, NULL, 0);
  fio___redis_ipc_publish_on_master(ipc); /* short header */
  fio_ipc_free(ipc);
  sr_drain();
  FIO_ASSERT(!sr_queue_len(r), "malformed publish dropped");

  /* A valid publish queues an exactly sized PUBLISH. */
  h.channel_len = 2;
  h.message_len = 2;
  ipc = sr_ipc(fio___redis_ipc_publish_on_master, NULL, NULL, &h, 16, "chhi", 4);
  fio___redis_ipc_publish_on_master(ipc);
  fio_ipc_free(ipc);
  sr_drain();
  static const char expect[] = "*3\r\n$7\r\nPUBLISH\r\n$2\r\nch\r\n$2\r\nhi\r\n";
  FIO_ASSERT(sr_queue_len(r) == 1, "valid publish queued");
  fio_redis_cmd_s *c =
      FIO_PTR_FROM_FIELD(fio_redis_cmd_s, node, r->cmd_queue.next);
  FIO_ASSERT(c->cmd_len == sizeof(expect) - 1 &&
                 !memcmp(c->cmd, expect, c->cmd_len),
             "exact PUBLISH bytes");
  fio___redis_free(r);
  sr_drain();
  FIO_ASSERT(sr_live == live0, "leak (%zu)", sr_live - live0);
}

/* *****************************************************************************
R-13: worker reply decode (real fio___redis_ipc_send_on_done)
***************************************************************************** */

static void sr_on_done_rejects(const char *name, const char *data, size_t len) {
  size_t calls = 0;
  FIOBJ o = sr_on_done_reply(data, len, &calls);
  FIO_ASSERT(calls == 1 && o == FIOBJ_INVALID,
             "%s: reply must decode to FIOBJ_INVALID",
             name);
}

static void test_r13_ipc_on_done(void) {
  fprintf(stderr, "* R-13 worker IPC reply decode...\n");
  const size_t live0 = sr_live;
  size_t calls = 0;
  FIOBJ o = sr_on_done_reply("$-1\r\n", 5, &calls);
  FIO_ASSERT(calls == 1 && o == fiobj_null(), "RESP2 null is a real null");
  o = sr_on_done_reply("_\r\n", 3, &calls);
  FIO_ASSERT(calls == 1 && o == fiobj_null(), "RESP3 null");
  o = sr_on_done_reply("*2\r\n:1\r\n$2\r\nab\r\n", 16, &calls);
  FIO_ASSERT(calls == 1 && o != FIOBJ_INVALID &&
                 FIOBJ_TYPE(o) == FIOBJ_T_ARRAY && fiobj_array_count(o) == 2,
             "complete array reply delivered");
  fiobj_free(o);
  sr_on_done_rejects("empty", NULL, 0);
  sr_on_done_rejects("trailing", "+OK\r\n+X\r\n", 9);
  sr_on_done_rejects("partial string", "$3\r\nab", 6);
  sr_on_done_rejects("partial array", "*3\r\n:1\r\n%1\r\n+k\r\n", 16);
  sr_on_done_rejects("garbage", "?x\r\n", 4);

  /* A reply shorter than the raw header never reaches a callback. */
  sr_rec_s rec = {0};
  fio_ipc_s *ipc = sr_ipc(NULL, fio___redis_ipc_send_on_done, &rec, "x", 1,
                          NULL, 0);
  fio___redis_ipc_send_on_done(ipc);
  fio_ipc_free(ipc);
  FIO_ASSERT(!rec.calls, "short reply must be ignored");
  sr_rec_clear(&rec);
  sr_drain();
  FIO_ASSERT(sr_live == live0, "leak (%zu)", sr_live - live0);
}

int main(void) {
  if (getenv("SR_LOSS_ONLY")) { /* isolate the loss-callback regression */
    test_r07_loss_callback_frees_engine();
    fprintf(stderr, "* security-redis (SR_LOSS_ONLY): passed\n");
    return 0;
  }
  /* Reject impossible auth extents before allocation or a source read. */
  FIO_ASSERT(!fio___redis_hello_cmd_len(SIZE_MAX), "HELLO length wrapped");
  FIO_ASSERT(!fio_redis_new(.auth = "x", .auth_len = SIZE_MAX),
             "oversized auth accepted");
  FIO_ASSERT(!fio_redis_new(.auth = NULL, .auth_len = 1),
             "NULL auth with nonzero length accepted");
  FIO_ASSERT(!fio___redis_publish_cmd_new(FIO_BUF_INFO2("x", SIZE_MAX),
                                          FIO_BUF_INFO2("y", 1)),
             "PUBLISH sizing wrapped");
  FIO_ASSERT(!fio___redis_publish_cmd_new(FIO_BUF_INFO2(NULL, 1),
                                          FIO_BUF_INFO2("y", 1)),
             "NULL PUBLISH channel accepted");
  test_push_attribute_no_leak();
  test_r06_capture_freeze();
  test_r01_hello_failure();
  test_r07_no_replay();
  test_r03_ipc_publish_bounds();
  test_r13_ipc_on_done();
  test_r07_loss_callback_frees_engine();
  fprintf(stderr, "* security-redis: all checks passed\n");
  return 0;
}
