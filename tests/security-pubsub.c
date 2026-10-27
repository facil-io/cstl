/* *****************************************************************************
Test: Pub/Sub security regressions (420 pubsub.h)

Deterministic, single process, reactor-less: tasks are drained manually from
the IO queue. Checks use FIO_ASSERT (active under NDEBUG).

- PUB-01: removing one subscriber preserves routing to the others; removing
          the last subscriber removes the channel.
- PUB-02: forked-worker cleanup does not call master-only `detached` on
          engines owned by the master; the builtin engines are constant
          (accessors never re-write them).
- PUB-03: malformed publish layouts (lengths / NUL terminators) are rejected
          before delivery / history.
- PUB-04: per-channel eviction holes never reach `oldest` / `replay`; replay
          starts at the first message with `timestamp >= since`, in time order.
- PUB-06: explicit history cache limits are applied.
- PUB-07: history requests with an out-of-bounds channel length are refused;
          an asynchronous replay owns a reference to its request IPC.
- Cache:  size accounting saturates; entries above the limit (or a total that
          would wrap) are rejected without touching the cache.
- Engine: callbacks are typed by channel kind; notify-master IPC payloads are
          bounds checked and the pattern byte is normalized.
***************************************************************************** */
/* lenient allocator (no accounting, size-agnostic frees: tests shrink lengths
 * of IPC messages before releasing them) */
#include <stdlib.h>
#include <string.h>
static void *test_realloc(void *old, size_t old_size, size_t size, size_t cp) {
  (void)old_size;
  void *p = malloc(size);
  if (!p)
    return NULL;
  if (old) {
    memcpy(p, old, cp < size ? cp : size);
    free(old);
  }
  return p;
}
#define FIO_MEMORY_DISABLE           1
#define FIO_MEM_REALLOC_(p, o, n, c) test_realloc((p), (o), (n), (c))
#define FIO_MEM_FREE_(p, n)          free((p))
#define FIO_MEM_REALLOC_IS_SAFE_     0
#define FIO_MEM_ALIGNMENT_SIZE_      16
#define FIO_MEM_REALLOC_ALIGNED_(p, o, n, c, a)                                \
  ((p) = test_realloc_aligned((p), (n), (c), (a)))
#if defined(_WIN32)
#define FIO_MEM_FREE_ALIGNED_(p, n) _aligned_free((p))
#else
#define FIO_MEM_FREE_ALIGNED_(p, n) free((p))
#endif
static void *test_realloc_aligned(void *old, size_t size, size_t cp, size_t a) {
  void *p = NULL;
#if defined(_WIN32)
  p = _aligned_malloc(size, a);
  if (!p)
    return NULL;
#else
  if (posix_memalign(&p, (a < sizeof(void *) ? sizeof(void *) : a), size))
    return NULL;
#endif
  if (old) {
    memcpy(p, old, cp < size ? cp : size);
#if defined(_WIN32)
    _aligned_free(old);
#else
    free(old);
#endif
  }
  return p;
}
/* global hooks too: typed index maps (e.g., the engine set) use them */
#define FIO_MEM_REALLOC(p, o, n, c) test_realloc((p), (o), (n), (c))
#define FIO_MEM_FREE(p, n)          free((p))
#define FIO_MEM_REALLOC_IS_SAFE     0
#define FIO_MEM_ALIGNMENT_SIZE      16
#define FIO_MEM_REALLOC_ALIGNED(p, o, n, c, a)                                 \
  ((p) = test_realloc_aligned((p), (n), (c), (a)))
#if defined(_WIN32)
#define FIO_MEM_FREE_ALIGNED(p, n) _aligned_free((p))
#else
#define FIO_MEM_FREE_ALIGNED(p, n) free((p))
#endif
#define FIO_MEM_ALLOC_SIZE(n)      ((size_t)(n))

#define FIO_PUBSUB
#include "test-helpers.h"

#define PS_CHECK(cond, ...) FIO_ASSERT((cond), __VA_ARGS__)

static void drain_io_queue(void) {
  for (int i = 0; i < 8; ++i)
    fio_queue_perform_all(fio_io_queue());
}

static uint64_t ps_id;

/* authors a well formed publish IPC (as fio___pubsub_engine_ipc_publish) */
static fio_ipc_s *mk_pub(const char *ch, const char *msg, uint64_t ts) {
  uint32_t h[2] = {(uint32_t)strlen(ch), (uint32_t)strlen(msg)};
  fio_ipc_s *ipc =
      fio_ipc_new(.call = fio___pubsub_engine_ipc_publish_deliver,
                  .timestamp = ts,
                  .id = ++ps_id,
                  .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)h, sizeof(h)),
                                       FIO_BUF_INFO2((char *)ch, h[0]),
                                       FIO_BUF_INFO2((char *)"\0", 1),
                                       FIO_BUF_INFO2((char *)msg, h[1]),
                                       FIO_BUF_INFO2((char *)"\0", 1)));
  PS_CHECK(ipc, "fio_ipc_new failed");
  return ipc;
}

static void publish_ipc(fio_ipc_s *ipc) {
  fio___pubsub_engine_ipc_publish_deliver(ipc);
  fio_ipc_free(ipc);
  drain_io_queue();
}

static uint64_t now_ms(void) { return (uint64_t)fio_io_last_tick_time(); }

static size_t channel_count(void) {
  return fio___pubsub_channel_map_count(&FIO___PUBSUB_POSTOFFICE.channels);
}

/* *****************************************************************************
PUB-01 - one subscriber's departure must not strand the others
***************************************************************************** */
static size_t ps_pub01_a, ps_pub01_b;
static void pub01_on_a(fio_pubsub_msg_s *msg) {
  (void)msg;
  ++ps_pub01_a;
}
static void pub01_on_b(fio_pubsub_msg_s *msg) {
  (void)msg;
  ++ps_pub01_b;
}
static void test_pub01(void) {
  fprintf(stderr, "* PUB-01 surviving subscriber remains routed\n");
  const size_t before = channel_count();
  uintptr_t a = 0, b = 0;
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1((char *)"pub01"),
                       .on_message = pub01_on_a,
                       .subscription_handle_ptr = &a);
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1((char *)"pub01"),
                       .on_message = pub01_on_b,
                       .subscription_handle_ptr = &b);
  drain_io_queue();
  PS_CHECK(a && b && channel_count() == before + 1,
           "two subscriptions share a routed channel");
  publish_ipc(mk_pub("pub01", "first", now_ms()));
  PS_CHECK(ps_pub01_a == 1 && ps_pub01_b == 1,
           "both subscribers receive the first message");
  PS_CHECK(!fio_pubsub_unsubscribe(.subscription_handle_ptr = &a),
           "unsubscribe first handle");
  drain_io_queue();
  PS_CHECK(!a && channel_count() == before + 1,
           "the first departure must not remove the channel");
  publish_ipc(mk_pub("pub01", "second", now_ms()));
  PS_CHECK(ps_pub01_a == 1 && ps_pub01_b == 2,
           "survivor receives subsequent messages");
  PS_CHECK(!fio_pubsub_unsubscribe(.subscription_handle_ptr = &b),
           "unsubscribe last handle");
  drain_io_queue();
  PS_CHECK(!b && channel_count() == before,
           "last departure removes the channel");
}

/* *****************************************************************************
PUB-02 - worker cleanup and constant builtin engines
***************************************************************************** */
static size_t ps_eng_detached;
static fio_pubsub_engine_s ps_engine;
static void eng_detached(const fio_pubsub_engine_s *e) {
  (void)e;
  ++ps_eng_detached;
}
static void eng_publish(const fio_pubsub_engine_s *e,
                        const fio_pubsub_msg_s *m) {
  (void)e, (void)m;
}

static void test_pub02(void) {
  fprintf(stderr, "* PUB-02 builtin engines / worker engine cleanup\n");
  /* builtins are constant-initialized and the accessors never write them */
  const fio_pubsub_engine_s *ipc = fio_pubsub_engine_ipc();
  const fio_pubsub_engine_s *cl = fio_pubsub_engine_cluster();
  PS_CHECK(ipc == &FIO___PUBSUB_POSTOFFICE.local_engine &&
               cl == &FIO___PUBSUB_POSTOFFICE.cluster_engine && ipc != cl,
           "builtin engines are distinct static objects");
  PS_CHECK(ipc->publish == fio___pubsub_engine_ipc_publish &&
               cl->publish == fio___pubsub_engine_cluster_publish,
           "builtin publish callbacks");
  PS_CHECK(ipc->detached && ipc->subscribe && ipc->psubscribe &&
               ipc->unsubscribe && ipc->punsubscribe && cl->detached &&
               cl->subscribe && cl->psubscribe && cl->unsubscribe &&
               cl->punsubscribe,
           "builtin engines fully initialized");
  FIO___PUBSUB_POSTOFFICE.local_engine.publish = eng_publish;
  FIO___PUBSUB_POSTOFFICE.cluster_engine.publish = eng_publish;
  PS_CHECK(fio_pubsub_engine_ipc()->publish == eng_publish &&
               fio_pubsub_engine_cluster()->publish == eng_publish,
           "accessors never re-write the builtin engines");
  FIO___PUBSUB_POSTOFFICE.local_engine.publish =
      fio___pubsub_engine_ipc_publish;
  FIO___PUBSUB_POSTOFFICE.cluster_engine.publish =
      fio___pubsub_engine_cluster_publish;

  ps_engine =
      (fio_pubsub_engine_s){.detached = eng_detached, .publish = eng_publish};
  fio_pubsub_engine_attach(&ps_engine);
  PS_CHECK(FIO___PUBSUB_POSTOFFICE.engines.count == 1, "engine attached");
  PS_CHECK(ps_engine.subscribe && ps_engine.psubscribe &&
               ps_engine.unsubscribe && ps_engine.punsubscribe &&
               ps_engine.publish == eng_publish &&
               ps_engine.detached == eng_detached,
           "engine defaults filled per field");

  /* forked worker cleanup: master-only `detached` is not called */
  fio_pubsub_engine_default_set(&ps_engine);
  ps_eng_detached = 0;
  fio___pubsub_cleanup(1);
  PS_CHECK(ps_eng_detached == 0, "child cleanup must not call detached");
  PS_CHECK(!FIO___PUBSUB_POSTOFFICE.engines.count,
           "child drops its engine collection");
  PS_CHECK(fio_pubsub_engine_default() == &ps_engine,
           "child keeps a publishable default (owned by master)");
  fio_pubsub_engine_detach(&ps_engine); /* no longer registered */
  PS_CHECK(ps_eng_detached == 0, "unregistered engine: detach is silent");
  PS_CHECK(fio_pubsub_engine_default() == fio_pubsub_engine_ipc(),
           "detach resets the default engine");

  /* the owning (master) cleanup does notify the engine */
  fio_pubsub_engine_attach(&ps_engine);
  fio___pubsub_cleanup(0);
  PS_CHECK(ps_eng_detached == 1, "master cleanup calls detached once");
  PS_CHECK(!FIO___PUBSUB_POSTOFFICE.engines.count, "engines released");
}

/* *****************************************************************************
PUB-03
***************************************************************************** */
static void test_pub03(void) {
  fprintf(stderr, "* PUB-03 malformed publish layouts\n");
  fio_pubsub_history_cache(1 << 20);
  PS_CHECK(!fio_pubsub_history_attach(fio_pubsub_history_cache(0), 1),
           "attach cache");
  const fio_pubsub_history_s *hc = fio_pubsub_history_cache(0);
  fio_pubsub_msg_s m;

  fio_ipc_s *ok = mk_pub("pub03", "hello", now_ms());
  PS_CHECK(!fio___pubsub_ipc2msg_checked(&m, ok) && m.channel.len == 5 &&
               m.message.len == 5 && !memcmp(m.message.buf, "hello", 5),
           "valid exact-size layout accepted");
  const uint32_t full = ok->len;

  /* declared channel longer than the payload (report example) */
  fio_ipc_s *bad = mk_pub("pub03", "hello", now_ms());
  fio_u2buf32u(bad->data, 100);
  PS_CHECK(fio___pubsub_ipc2msg_checked(&m, bad), "long channel rejected");
  publish_ipc(bad); /* must not crash or reach history */
  PS_CHECK(hc->oldest(hc, FIO_BUF_INFO2((char *)"pub03", 5), 0) == UINT64_MAX,
           "malformed message must not reach history");

  /* lengths that would wrap 32-bit sums */
  bad = mk_pub("pub03", "hello", now_ms());
  fio_u2buf32u(bad->data, 0xFFFFFFFFU);
  fio_u2buf32u(bad->data + 4, 0xFFFFFFFFU);
  PS_CHECK(fio___pubsub_ipc2msg_checked(&m, bad), "wrapping lengths rejected");
  publish_ipc(bad);

  /* header-only and one byte short of the declared layout */
  bad = mk_pub("pub03", "hello", now_ms());
  bad->len = 8;
  PS_CHECK(fio___pubsub_ipc2msg_checked(&m, bad), "header only rejected");
  bad->len = full - 1;
  PS_CHECK(fio___pubsub_ipc2msg_checked(&m, bad), "truncated rejected");
  publish_ipc(bad);

  /* missing terminator */
  bad = mk_pub("pub03", "hello", now_ms());
  bad->data[8 + 5] = 'X';
  PS_CHECK(fio___pubsub_ipc2msg_checked(&m, bad), "unterminated rejected");
  publish_ipc(bad);
  PS_CHECK(hc->oldest(hc, FIO_BUF_INFO2((char *)"pub03", 5), 0) == UINT64_MAX,
           "no malformed message stored");

  /* public helper keeps its "empty buffers" contract */
  bad = mk_pub("pub03", "hello", now_ms());
  fio_u2buf32u(bad->data, 100);
  m = fio_pubsub_ipc2msg(bad);
  PS_CHECK(!m.channel.buf && !m.message.buf && m.timestamp == bad->timestamp,
           "fio_pubsub_ipc2msg returns empty buffers when malformed");
  fio_ipc_free(bad);

  publish_ipc(ok);
  PS_CHECK(hc->oldest(hc, FIO_BUF_INFO2((char *)"pub03", 5), 0) != UINT64_MAX,
           "valid message stored");
  fio_pubsub_history_detach(hc);
}

/* *****************************************************************************
PUB-04 / PUB-06
***************************************************************************** */
static uint64_t ps_replay[16];
static size_t ps_replay_n;
static size_t ps_replay_done;
static void replay_cb(fio_pubsub_msg_s *m, void *u) {
  (void)u;
  if (ps_replay_n < 16)
    ps_replay[ps_replay_n] = m->timestamp;
  ++ps_replay_n;
}
static void replay_done(void *u) {
  (void)u;
  ++ps_replay_done;
}
static void replay(const char *ch, uint64_t since) {
  const fio_pubsub_history_s *hc = fio_pubsub_history_cache(0);
  ps_replay_n = ps_replay_done = 0;
  hc->replay(hc,
             FIO_BUF_INFO2((char *)ch, strlen(ch)),
             0,
             since,
             replay_cb,
             replay_done,
             NULL);
  PS_CHECK(ps_replay_done == 1, "replay must call on_done once");
}
/* replay of `ch` starts at `oldest` and is strictly time ordered */
static void replay_check_order(const char *ch) {
  const fio_pubsub_history_s *hc = fio_pubsub_history_cache(0);
  const uint64_t o = hc->oldest(hc, FIO_BUF_INFO2((char *)ch, strlen(ch)), 0);
  replay(ch, 0);
  PS_CHECK(ps_replay_n ? (ps_replay[0] == o) : (o == UINT64_MAX),
           "replay starts at oldest (%s)",
           ch);
  for (size_t i = 1; i < ps_replay_n && i < 16; ++i)
    PS_CHECK(ps_replay[i] > ps_replay[i - 1], "replay ordered (%s)", ch);
}

static void test_pub04_06(void) {
  fprintf(stderr, "* PUB-04 / PUB-06 history eviction and limits\n");
  /* PUB-06: explicit limits are applied, zero keeps the current limit */
  fio_pubsub_history_cache(12345);
  PS_CHECK(FIO___PUBSUB_POSTOFFICE.cache.limit == 12345, "limit applied");
  fio_pubsub_history_cache(0);
  PS_CHECK(FIO___PUBSUB_POSTOFFICE.cache.limit == 12345, "zero keeps limit");

  const fio_pubsub_history_s *hc = fio_pubsub_history_cache(1 << 20);
  PS_CHECK(!fio_pubsub_history_attach(hc, 1), "attach cache");
  PS_CHECK(!FIO___PUBSUB_POSTOFFICE.cache.size, "clean cache");
  const uint64_t t0 = now_ms() - 10000;
  const char *chs[2] = {"pub04-a", "pub04-b"};
  for (int i = 0; i < 8; ++i)
    for (int c = 0; c < 2; ++c)
      publish_ipc(mk_pub(chs[c], "m", t0 + (uint64_t)(i + 1) * 10));

  /* lower bound replay (old search returned an extra older message) */
  replay("pub04-a", t0 + 55);
  PS_CHECK(ps_replay_n == 3 && ps_replay[0] == t0 + 60 &&
               ps_replay[2] == t0 + 80,
           "replay since=55 -> 60,70,80 (got %zu)",
           ps_replay_n);
  replay("pub04-a", t0 + 81);
  PS_CHECK(ps_replay_n == 0, "replay after newest is empty");
  replay("pub04-a", t0);
  PS_CHECK(ps_replay_n == 8, "replay from start is complete");

  /* per-channel eviction leaves holes (removed slots) in each channel;
   * ordered readers compact first; shrink the limit to force eviction */
  const size_t one = FIO___PUBSUB_POSTOFFICE.cache.size / 16;
  PS_CHECK(one * 16 == FIO___PUBSUB_POSTOFFICE.cache.size, "uniform entries");
  fio_pubsub_history_cache(one * 12 + one / 2);
  publish_ipc(mk_pub("pub04-c", "m", t0 + 100));
  PS_CHECK(FIO___PUBSUB_POSTOFFICE.cache.size == one * 12,
           "eviction honors the limit (five evicted)");
  for (int c = 0; c < 2; ++c) {
    fio___pubsub_history_channel_s *arr =
        fio___pubsub_history_cache_channel(&FIO___PUBSUB_POSTOFFICE.cache,
                                           FIO_BUF_INFO2((char *)chs[c], 7),
                                           0);
    PS_CHECK(arr && arr->count && arr->count < 8 && arr->count != arr->w,
             "precondition: evicted channel is sparse (%s)",
             chs[c]);
    const size_t total = arr->count;
    const uint64_t first =
        hc->oldest(hc, FIO_BUF_INFO2((char *)chs[c], 7), 0);
    PS_CHECK(first != UINT64_MAX && first > t0 + 10 && arr->count == arr->w,
             "oldest on a sparse channel skips the holes (compacted)");
    replay_check_order(chs[c]);
    PS_CHECK(ps_replay_n == total, "replay returns every retained message");
    /* since in a gap / exact / before first / after latest */
    replay(chs[c], first - 5);
    PS_CHECK(ps_replay_n == total && ps_replay[0] == first,
             "replay before first returns all");
    replay(chs[c], first + 5); /* between two timestamps */
    PS_CHECK(ps_replay_n == total - 1 && ps_replay[0] == first + 10,
             "replay in a gap starts at the next message");
    replay(chs[c], first + 10); /* exact timestamp is inclusive */
    PS_CHECK(ps_replay_n == total - 1 && ps_replay[0] == first + 10,
             "replay exact timestamp inclusive");
    replay(chs[c], UINT64_MAX - 1);
    PS_CHECK(ps_replay_n == 0, "replay after latest is empty");
  }
  /* repeated eviction rounds: limit holds after every push, order kept */
  {
    fio_pubsub_history_cache(1 << 20); /* no eviction while measuring */
    const size_t before = FIO___PUBSUB_POSTOFFICE.cache.size;
    publish_ipc(mk_pub("pub04-x", "m", t0 + 190)); /* measure one entry */
    const size_t entry = FIO___PUBSUB_POSTOFFICE.cache.size - before;
    PS_CHECK(entry && entry <= one, "entry size measured");
    fio_pubsub_history_cache(entry * 6);
    for (int i = 0; i < 40; ++i) {
      publish_ipc(mk_pub(chs[i & 1], "m", t0 + 200 + (uint64_t)i * 10));
      PS_CHECK(FIO___PUBSUB_POSTOFFICE.cache.size <=
                   FIO___PUBSUB_POSTOFFICE.cache.limit,
               "limit holds on round %d",
               i);
    }
    for (int c = 0; c < 2; ++c)
      replay_check_order(chs[c]);
  }
  fio_pubsub_history_detach(hc);
  fio_pubsub_history_cache(FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT);
}

/* *****************************************************************************
History cache: duplicates, per-channel order, size accounting
***************************************************************************** */
static fio_ipc_s *mk_pub_id(const char *ch, uint64_t ts, uint64_t id) {
  fio_ipc_s *ipc = mk_pub(ch, "m", ts);
  ipc->id = id;
  return ipc;
}
static size_t hist_channels(void) {
  return (size_t)fio___pubsub_history_map_count(
      &FIO___PUBSUB_POSTOFFICE.cache.map);
}
static uint64_t hist_oldest(const char *ch) {
  const fio_pubsub_history_s *hc = fio_pubsub_history_cache(0);
  return hc->oldest(hc, FIO_BUF_INFO2((char *)ch, strlen(ch)), 0);
}

static void test_history_order(void) {
  fprintf(stderr, "* history cache: duplicates, order, size accounting\n");
  const fio_pubsub_history_s *hc = fio_pubsub_history_cache(1 << 20);
  PS_CHECK(!fio_pubsub_history_attach(hc, 1), "attach cache");
  PS_CHECK(!FIO___PUBSUB_POSTOFFICE.cache.size && !hist_channels(),
           "clean cache");
  const uint64_t t = now_ms() - 100000;

  /* duplicates (comparator: len + id + timestamp) */
  publish_ipc(mk_pub_id("ord-d", t + 1, 9001));
  const size_t entry = FIO___PUBSUB_POSTOFFICE.cache.size;
  PS_CHECK(entry, "entry measured");
  fio_ipc_s *same = mk_pub_id("ord-d", t + 1, 9001); /* distinct object */
  fio_ipc_s *again = fio_ipc_dup(same);
  publish_ipc(same);
  publish_ipc(again); /* the very same object twice */
  PS_CHECK(FIO___PUBSUB_POSTOFFICE.cache.size == entry,
           "colliding messages stored (and counted) once");
  publish_ipc(mk_pub_id("ord-e", t + 1, 9001)); /* same key, other channel */
  PS_CHECK(FIO___PUBSUB_POSTOFFICE.cache.size == entry * 2 &&
               hist_oldest("ord-d") == t + 1 && hist_oldest("ord-e") == t + 1,
           "same key in another channel is a different message");

  /* out of order timestamps: storage order 300, 100, 200 */
  publish_ipc(mk_pub_id("ord-o", t + 300, 9300));
  publish_ipc(mk_pub_id("ord-o", t + 100, 9100));
  publish_ipc(mk_pub_id("ord-o", t + 200, 9200));
  PS_CHECK(FIO___PUBSUB_POSTOFFICE.cache.size == entry * 5, "precondition");
  PS_CHECK(hist_oldest("ord-o") == t + 100, "oldest by time, not storage");
  replay("ord-o", 0);
  PS_CHECK(ps_replay_n == 3 && ps_replay[0] == t + 100 &&
               ps_replay[1] == t + 200 && ps_replay[2] == t + 300,
           "out of order insertion is replayed in time order");
  replay("ord-o", t + 150);
  PS_CHECK(ps_replay_n == 2 && ps_replay[0] == t + 200,
           "replay lower bound on an out of order channel");

  /* size accounting never wraps (synthetic totals / lengths, real payloads) */
  {
    const size_t ovh = fio___pubsub_history_cache_bytes(0);
    PS_CHECK(ovh == sizeof(fio_ipc_s) + 8, "overhead");
    PS_CHECK(fio___pubsub_history_cache_bytes(SIZE_MAX) == SIZE_MAX &&
                 fio___pubsub_history_cache_bytes(SIZE_MAX - ovh + 1) ==
                     SIZE_MAX,
             "bytes saturate");
    PS_CHECK(!fio___pubsub_history_cache_fits(0, SIZE_MAX, SIZE_MAX),
             "saturated size rejected");
    PS_CHECK(!fio___pubsub_history_cache_fits(SIZE_MAX - 10, SIZE_MAX, 11) &&
                 fio___pubsub_history_cache_fits(SIZE_MAX - 10, SIZE_MAX, 10),
             "total never wraps");
    PS_CHECK(!fio___pubsub_history_cache_fits(0, 100, 101) &&
                 fio___pubsub_history_cache_fits(0, 100, 100),
             "limit respected");
    fio___pubsub_history_cache_s *c = &FIO___PUBSUB_POSTOFFICE.cache;
    const size_t chans = hist_channels(), bytes = c->size, limit = c->limit;
    /* an entry above the limit is rejected: it no longer wipes the cache */
    fio_pubsub_history_cache(entry - 1);
    publish_ipc(mk_pub_id("ord-big", t + 700, 9700));
    PS_CHECK(c->size == bytes && hist_channels() == chans &&
                 hist_oldest("ord-big") == UINT64_MAX &&
                 hist_oldest("ord-o") == t + 100,
             "entry above the limit rejected, cache intact");
    /* a total close to SIZE_MAX: the push is refused, never wrapped */
    fio_pubsub_history_cache(SIZE_MAX);
    c->size = SIZE_MAX - (entry - 1);
    publish_ipc(mk_pub_id("ord-wrap", t + 701, 9701));
    PS_CHECK(c->size == SIZE_MAX - (entry - 1) && hist_channels() == chans &&
                 hist_oldest("ord-wrap") == UINT64_MAX,
             "wrapping push rejected");
    c->size = bytes;
    fio_pubsub_history_cache(limit);
  }
  fio_pubsub_history_detach(hc);
  PS_CHECK(!FIO___PUBSUB_POSTOFFICE.cache.size && !hist_channels(),
           "detach releases everything");
  fio_pubsub_history_cache(FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT);
}

/* *****************************************************************************
PUB-07 - history request bounds
***************************************************************************** */
static size_t ps_h_push, ps_h_oldest, ps_h_detached, ps_req_done;
static int ro_push(const fio_pubsub_history_s *h, fio_pubsub_msg_s *m) {
  (void)h, (void)m;
  ++ps_h_push;
  return 0;
}
static uint64_t full_oldest(const fio_pubsub_history_s *h,
                            fio_buf_info_s ch,
                            int16_t f) {
  (void)h, (void)ch, (void)f;
  ++ps_h_oldest;
  return UINT64_MAX;
}
static void full_detached(const fio_pubsub_history_s *h) {
  (void)h;
  ++ps_h_detached;
}
static int full_replay(const fio_pubsub_history_s *h,
                       fio_buf_info_s ch,
                       int16_t f,
                       uint64_t s,
                       void (*om)(fio_pubsub_msg_s *, void *),
                       void (*od)(void *),
                       void *u) {
  (void)h, (void)ch, (void)f, (void)s, (void)om;
  od(u);
  return 0;
}
/* missing callbacks are filled with stubs by `fio_pubsub_history_attach` */
static fio_pubsub_history_s ps_push_manager = {.push = ro_push};
static fio_pubsub_history_s ps_full_manager = {
    .detached = full_detached,
    .oldest = full_oldest,
    .replay = full_replay,
};
static void on_req_done(fio_ipc_s *ipc) {
  (void)ipc;
  ++ps_req_done;
}

static void test_pub07(void) {
  fprintf(stderr, "* PUB-07 history request bounds\n");
  PS_CHECK(!fio_pubsub_history_attach(&ps_push_manager, 10), "push attach");
  PS_CHECK(!fio_pubsub_history_attach(&ps_full_manager, 5), "full attach");
  ps_h_push = ps_h_oldest = ps_h_detached = 0;
  publish_ipc(mk_pub("pub09", "m", now_ms()));
  PS_CHECK(ps_h_push == 1, "push-only manager receives push");

  /* PUB-07: channel_len beyond the payload */
  fio___pubsub_history_request_s hdr = {.replay_since = 1, .channel_len = 5};
  fio_ipc_s *req =
      fio_ipc_new(.call = fio___pubsub_master_on_history_request,
                  .on_done = on_req_done,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)&hdr,
                                    FIO___PUBSUB_HISTORY_REQUEST_HEADER_LEN),
                      FIO_BUF_INFO2((char *)"pub09", 5)));
  PS_CHECK(req, "request");
  ps_req_done = 0;
  fio___pubsub_master_on_history_request(req); /* valid: managers queried */
  drain_io_queue();
  PS_CHECK(ps_h_oldest == 1 && ps_req_done == 1,
           "valid request queries the managers and completes");
  ((fio___pubsub_history_request_s *)req->data)->channel_len = 0x7FFFFFFFU;
  ps_req_done = 0;
  fio___pubsub_master_on_history_request(req);
  drain_io_queue();
  PS_CHECK(ps_h_oldest == 1, "oversized channel_len never reaches managers");
  PS_CHECK(ps_req_done == 1, "oversized request still completes");
  ((fio___pubsub_history_request_s *)req->data)->channel_len = 6; /* +1 */
  fio___pubsub_master_on_history_request(req);
  drain_io_queue();
  PS_CHECK(ps_h_oldest == 1, "one byte past the payload refused");
  /* shorter than the fixed request header */
  req->len = (uint32_t)(FIO___PUBSUB_HISTORY_REQUEST_HEADER_LEN - 1);
  ps_req_done = 0;
  fio___pubsub_master_on_history_request(req);
  drain_io_queue();
  PS_CHECK(ps_h_oldest == 1 && ps_req_done == 1,
           "truncated request refused and completed");
  fio_ipc_free(req);

  fio_pubsub_history_detach(&ps_push_manager);
  fio_pubsub_history_detach(&ps_full_manager);
  PS_CHECK(ps_h_detached == 1,
           "detached exactly once (got %zu)",
           ps_h_detached);
}

/* *****************************************************************************
Asynchronous history replay: the request outlives its IPC handler
***************************************************************************** */
static void (*ps_async_om)(fio_pubsub_msg_s *, void *);
static void (*ps_async_od)(void *);
static void *ps_async_u;
static size_t ps_async_replies;
static uint64_t full_oldest_zero(const fio_pubsub_history_s *h,
                                 fio_buf_info_s ch,
                                 int16_t f) {
  (void)h, (void)ch, (void)f;
  return 0;
}
/* documented: completes later (e.g., deferred), calling `on_done` once */
static int async_replay(const fio_pubsub_history_s *h,
                        fio_buf_info_s ch,
                        int16_t f,
                        uint64_t s,
                        void (*om)(fio_pubsub_msg_s *, void *),
                        void (*od)(void *),
                        void *u) {
  (void)h, (void)ch, (void)f, (void)s;
  ps_async_om = om;
  ps_async_od = od;
  ps_async_u = u;
  return 0;
}
static fio_pubsub_history_s ps_async_manager = {
    .oldest = full_oldest_zero,
    .replay = async_replay,
};
static void on_async_reply(fio_ipc_s *ipc) {
  (void)ipc;
  ++ps_async_replies;
}
static void test_async_replay(void) {
  fprintf(stderr, "* asynchronous history replay owns its request\n");
  drain_io_queue();
  PS_CHECK(!fio_pubsub_history_attach(&ps_async_manager, 1), "attach");
  drain_io_queue();
  fio___pubsub_history_request_s hdr = {.replay_since = 1, .channel_len = 5};
  fio_ipc_s *req =
      fio_ipc_new(.call = fio___pubsub_master_on_history_request,
                  .on_reply = on_async_reply,
                  .on_done = on_req_done,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)&hdr,
                                    FIO___PUBSUB_HISTORY_REQUEST_HEADER_LEN),
                      FIO_BUF_INFO2((char *)"async", 5)));
  PS_CHECK(req, "request");
  ps_req_done = ps_async_replies = 0;
  ps_async_u = NULL;
  fio___pubsub_master_on_history_request(req);
  fio_ipc_free(req); /* as the IPC task does once the handler returns */
  drain_io_queue();  /* (`fio_ipc_free` defers the release) */
  PS_CHECK(ps_async_u && !ps_req_done, "replay pending after the handler");
  fio_pubsub_msg_s msg = {.channel = FIO_BUF_INFO2((char *)"async", 5),
                          .message = FIO_BUF_INFO2((char *)"late", 4),
                          .timestamp = 2};
  ps_async_om(&msg, ps_async_u); /* the request must still be valid */
  ps_async_od(ps_async_u);       /* terminal: reply `done`, release */
  drain_io_queue();
  PS_CHECK(ps_async_replies == 1 && ps_req_done == 1,
           "late message and done delivered (replies %zu, done %zu)",
           ps_async_replies,
           ps_req_done);
  fio_pubsub_history_detach(&ps_async_manager);
  drain_io_queue();
}

/* *****************************************************************************
Typed engine callbacks, notify-master bounds and pattern byte normalization
***************************************************************************** */
static size_t ps_eng_cb[4]; /* sub, psub, unsub, punsub */
static void cnt_sub(const fio_pubsub_engine_s *e, fio_buf_info_s c, int16_t f) {
  (void)e, (void)c, (void)f;
  ++ps_eng_cb[0];
}
static void cnt_psub(const fio_pubsub_engine_s *e,
                     fio_buf_info_s c,
                     int16_t f) {
  (void)e, (void)c, (void)f;
  ++ps_eng_cb[1];
}
static void cnt_unsub(const fio_pubsub_engine_s *e,
                      fio_buf_info_s c,
                      int16_t f) {
  (void)e, (void)c, (void)f;
  ++ps_eng_cb[2];
}
static void cnt_punsub(const fio_pubsub_engine_s *e,
                       fio_buf_info_s c,
                       int16_t f) {
  (void)e, (void)c, (void)f;
  ++ps_eng_cb[3];
}
static fio_ipc_s *mk_notify(void (*call)(fio_ipc_s *),
                            const char *ch,
                            uint8_t pattern_byte) {
  uint32_t len = (uint32_t)strlen(ch);
  fio_ipc_s *ipc =
      fio_ipc_new(.call = call,
                  .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)&len, 4),
                                       FIO_BUF_INFO2((char *)&pattern_byte, 1),
                                       FIO_BUF_INFO2((char *)ch, len)));
  PS_CHECK(ipc, "notify ipc");
  return ipc;
}
static void *env_sub_get(const char *ch, uint8_t is_pattern) {
  return fio_io_env_get(NULL,
                        .type = fio___pubsub_channel_env_type(0, is_pattern),
                        .name = FIO_BUF_INFO2((char *)ch, strlen(ch)));
}
/* creates and releases a channel object: engine callbacks by channel kind */
static void typed_channel(const char *name, uint8_t is_pattern) {
  fio_pubsub_channel_s *ch = fio___pubsub_channel_new_for_map(
      FIO_STR_INFO3((char *)name,
                    strlen(name),
                    FIO___PUBSUB_CHANNEL_ENCODE_CAPA(0, is_pattern)));
  PS_CHECK(ch && ch->is_pattern == is_pattern, "channel object");
  fio_pubsub_channel_free(ch);
}
static void test_typed_callbacks(void) {
  fprintf(stderr, "* engine callbacks typed by kind, notify-master IPC\n");
  fio_pubsub_engine_s e = {.detached = eng_detached,
                           .subscribe = cnt_sub,
                           .psubscribe = cnt_psub,
                           .unsubscribe = cnt_unsub,
                           .punsubscribe = cnt_punsub,
                           .publish = eng_publish};
  drain_io_queue();
  fio_pubsub_engine_attach(&e);
  drain_io_queue();
  FIO_MEMSET(ps_eng_cb, 0, sizeof(ps_eng_cb));
  typed_channel("typed-ch", 0);
  PS_CHECK(ps_eng_cb[0] == 1 && ps_eng_cb[2] == 1 && !ps_eng_cb[1] &&
               !ps_eng_cb[3],
           "channel: subscribe / unsubscribe only");
  typed_channel("typed-*", 1);
  PS_CHECK(ps_eng_cb[1] == 1 && ps_eng_cb[3] == 1 && ps_eng_cb[0] == 1 &&
               ps_eng_cb[2] == 1,
           "pattern: psubscribe / punsubscribe only");

  /* notify-master: any nonzero pattern byte means "pattern" (0 / 1 only) */
  const size_t pats =
      fio___pubsub_channel_map_count(&FIO___PUBSUB_POSTOFFICE.patterns);
  fio_ipc_s *ipc =
      mk_notify(fio___pubsub_notify_master_of_subscription, "typed-n*", 2);
  fio___pubsub_notify_master_of_subscription(ipc);
  fio_ipc_free(ipc);
  drain_io_queue();
  PS_CHECK(fio___pubsub_channel_map_count(&FIO___PUBSUB_POSTOFFICE.patterns) ==
                   pats + 1 &&
               ps_eng_cb[1] == 2 && env_sub_get("typed-n*", 1),
           "pattern byte 2 creates a pattern subscription");
  ipc = mk_notify(fio___pubsub_notify_master_of_unsubscription, "typed-n*", 7);
  fio___pubsub_notify_master_of_unsubscription(ipc);
  fio_ipc_free(ipc);
  drain_io_queue();
  PS_CHECK(!env_sub_get("typed-n*", 1),
           "pattern byte 7 removes the pattern subscription");

  /* notify-master bounds: a name past the payload / a short header */
  {
    const size_t chans = channel_count();
    const size_t pats2 =
        fio___pubsub_channel_map_count(&FIO___PUBSUB_POSTOFFICE.patterns);
    const size_t subs = ps_eng_cb[0] + ps_eng_cb[1];
    fio_ipc_s *bad =
        mk_notify(fio___pubsub_notify_master_of_subscription, "bound", 0);
    fio_u2buf32u(bad->data, 100); /* declared name longer than the payload */
    fio___pubsub_notify_master_of_subscription(bad);
    fio___pubsub_notify_master_of_unsubscription(bad);
    drain_io_queue();
    PS_CHECK(channel_count() == chans && ps_eng_cb[0] + ps_eng_cb[1] == subs,
             "out of bounds name refused");
    fio_u2buf32u(bad->data, 5);
    bad->len = 4; /* shorter than [u32 len][u8 pattern] */
    fio___pubsub_notify_master_of_subscription(bad);
    fio___pubsub_notify_master_of_unsubscription(bad);
    drain_io_queue();
    PS_CHECK(channel_count() == chans &&
                 fio___pubsub_channel_map_count(
                     &FIO___PUBSUB_POSTOFFICE.patterns) == pats2 &&
                 ps_eng_cb[0] + ps_eng_cb[1] == subs && !env_sub_get("bound", 0),
             "short notify header refused");
    fio_ipc_free(bad);
  }
  ps_eng_detached = 0;
  fio_pubsub_engine_detach(&e);
  PS_CHECK(ps_eng_detached == 1, "typed engine detached once");
}

int main(void) {
  test_pub01();
  test_pub02();
  test_pub03();
  test_pub04_06();
  test_history_order();
  test_typed_callbacks();
  test_pub07();
  test_async_replay();
  drain_io_queue();
  fprintf(stderr, "* pub/sub security regressions passed.\n");
  return 0;
}
