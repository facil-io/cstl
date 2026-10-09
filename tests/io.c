/* *****************************************************************************
Test - IO API / types / reactor integration (400-402)

Correctness coverage for fio_io reactor state, IO object lifecycle,
protocol callbacks, fd attachment, task/timer scheduling, env helpers,
protocol iteration, TLS context helpers, and listen/connect roundtrip.

Uses in-process socketpairs / loopback plus one bounded connection attempt to
a known external host on a bad port. No external processes or benchmarking.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IO
#include FIO_INCLUDE_FILE

/* *****************************************************************************
POSIX-only helpers
***************************************************************************** */
#if !FIO_OS_POSIX
static void test_io_skipped(void) {
  FIO_LOG_WARNING("SKIPPED");
  fprintf(stderr, "* IO integration tests skipped on non-POSIX platform\n");
}
#endif

/* *****************************************************************************
Static state for callback tracking
***************************************************************************** */
static volatile int fio___test_io_attach_count = 0;
static volatile int fio___test_io_data_count = 0;
static volatile int fio___test_io_ready_count = 0;
static volatile int fio___test_io_close_count = 0;
static volatile int fio___test_io_client_close_count = 0;
static volatile int fio___test_io_env_close_count = 0;
static volatile int fio___test_io_timer_count = 0;

static char fio___test_io_pair_received[64];
static size_t fio___test_io_pair_received_len = 0;

static char fio___test_io_listen_received[64];
static size_t fio___test_io_listen_received_len = 0;

/* *****************************************************************************
Unit tests - reactor state (no running reactor)
***************************************************************************** */
static void test_io_reactor_state(void) {
  FIO_ASSERT(fio_io_is_running() == 0,
             "fio_io_is_running should be 0 before reactor starts");
  FIO_ASSERT(fio_io_pid() > 0, "fio_io_pid should be positive");
  FIO_ASSERT(fio_io_root_pid() == fio_io_pid(),
             "root pid should equal pid before workers start");
  FIO_ASSERT(fio_io_is_master() == 1, "should be master before workers start");
  FIO_ASSERT(fio_io_is_worker() == 0, "should not be worker before start");

  FIO_ASSERT(fio_io_workers(0) == 0, "workers(0) should be 0");
  FIO_ASSERT(fio_io_workers(1) == 1, "workers(1) should be 1");
  FIO_ASSERT(fio_io_workers(4) == 4, "workers(4) should be 4");
  FIO_ASSERT(fio_io_workers(-1) >= 1, "workers(-1) should return at least 1");

  size_t original = fio_io_shutdown_timeout();
  FIO_ASSERT(original > 0, "default shutdown timeout should be > 0");
  FIO_ASSERT(fio_io_shutdown_timeout_set(1234) == 1234,
             "shutdown_timeout_set should return new value");
  FIO_ASSERT(fio_io_shutdown_timeout() == 1234,
             "shutdown_timeout should reflect set value");
  fio_io_shutdown_timeout_set(original);

  FIO_ASSERT(fio_io_queue() != NULL, "fio_io_queue should return non-NULL");
  fprintf(stderr, "* reactor state: OK\n");
}

static void test_io_noop_and_protocol_set_init(void) {
  fio_io_protocol_s pr = {0};
  FIO_ASSERT(fio_io_protocol_set(NULL, &pr) == &pr,
             "protocol_set(NULL, pr) should initialize and return pr");
  FIO_ASSERT(pr.on_attach != NULL,
             "protocol initialization should set default on_attach");
  FIO_ASSERT(pr.on_data != NULL,
             "protocol initialization should set default on_data");
  FIO_ASSERT(pr.timeout != 0,
             "protocol initialization should set default timeout");

  FIO_ASSERT(fio_io_protocol_set(NULL, NULL) == NULL,
             "protocol_set(NULL, NULL) should be a no-op");

  /* pre-reactor initialization yields a complete plaintext protocol; the
   * first real use with TLS (e.g., fio_io_listen + TLS) replaces the
   * plaintext default IO functions with the TLS defaults. */
  {
    fio_io_functions_s tls_fn = fio_io_tls_default_functions(NULL);
    fio_io_protocol_s tls_pr = {0};
    fio_io_protocol_set(NULL, &tls_pr);
    FIO_ASSERT(tls_pr.io_functions.start == fio_io_noop &&
                   tls_pr.io_functions.read == fio___io_func_default_read &&
                   tls_pr.io_functions.write == fio___io_func_default_write,
               "pre-initialized protocol should have plaintext IO functions");
    fio___io_protocol_init_test(&tls_pr, 1);
    FIO_ASSERT(tls_pr.io_functions.start == tls_fn.start &&
                   tls_pr.io_functions.read == tls_fn.read &&
                   tls_pr.io_functions.build_context == tls_fn.build_context,
               "pre-initialized protocol should get TLS IO defaults on first "
               "TLS use");
    fio___io_protocol_init_test(&tls_pr, 0); /* later uses don't re-init */
    FIO_ASSERT(tls_pr.io_functions.read == tls_fn.read,
               "IO functions are resolved once per protocol");

    fio_io_protocol_s plain_pr = {0};
    fio_io_protocol_set(NULL, &plain_pr);
    fio___io_protocol_init_test(&plain_pr, 0);
    FIO_ASSERT(plain_pr.io_functions.start == fio_io_noop &&
                   plain_pr.io_functions.read == fio___io_func_default_read,
               "plaintext use should keep plaintext IO defaults");
  }

  fio_io_noop(NULL);
  fprintf(stderr, "* fio_io_noop + protocol_set init: OK\n");
}

/* *****************************************************************************
Unit tests - deferred task scheduling
***************************************************************************** */
static volatile int fio___test_io_defer_count = 0;
static void *fio___test_io_defer_u1 = NULL;
static void *fio___test_io_defer_u2 = NULL;

static void fio___test_io_defer_task(void *u1, void *u2) {
  ++fio___test_io_defer_count;
  fio___test_io_defer_u1 = u1;
  fio___test_io_defer_u2 = u2;
}

static void test_io_defer(void) {
  fio___test_io_defer_count = 0;
  fio_io_defer(fio___test_io_defer_task, (void *)0xA1, (void *)0xB2);
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio___test_io_defer_count == 1, "deferred task should run once");
  FIO_ASSERT(fio___test_io_defer_u1 == (void *)0xA1, "defer udata1 mismatch");
  FIO_ASSERT(fio___test_io_defer_u2 == (void *)0xB2, "defer udata2 mismatch");

  fio___test_io_defer_count = 0;
  for (int i = 0; i < 5; ++i)
    fio_io_defer(fio___test_io_defer_task, NULL, NULL);
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio___test_io_defer_count == 5, "five deferred tasks should run");
  fprintf(stderr, "* fio_io_defer: OK\n");
}

/* *****************************************************************************
Unit tests - protocol iteration
***************************************************************************** */
static volatile int fio___test_io_each_count = 0;

static void fio___test_io_each_task(fio_io_s *io, void *udata) {
  ++fio___test_io_each_count;
  (void)io, (void)udata;
}

static void test_io_protocol_each(void) {
  fio___test_io_each_count = 0;
  FIO_ASSERT(fio_io_protocol_each(NULL, fio___test_io_each_task, NULL) == 0,
             "protocol_each(NULL) should return 0");

  fio_io_protocol_s pr = {0};
  FIO_ASSERT(fio_io_protocol_each(&pr, fio___test_io_each_task, NULL) == 0,
             "protocol_each(uninit) should return 0");
  fprintf(stderr, "* fio_io_protocol_each: OK\n");
}

/* *****************************************************************************
Unit tests - environment helpers (global env, io == NULL)
***************************************************************************** */
static void fio___test_io_env_on_close(void *udata) {
  ++fio___test_io_env_close_count;
  (void)udata;
}

static void test_io_env_global(void) {
  fio___test_io_env_close_count = 0;

  fio_io_env_set(NULL,
                 .name = FIO_BUF_INFO1((char *)"gkey"),
                 .type = 1,
                 .udata = (void *)0x1234,
                 .on_close = fio___test_io_env_on_close);

  FIO_ASSERT(fio_io_env_get(NULL,
                            .name = FIO_BUF_INFO1((char *)"gkey"),
                            .type = 1) == (void *)0x1234,
             "global env get should return set value");
  FIO_ASSERT(fio_io_env_get(NULL,
                            .name = FIO_BUF_INFO1((char *)"gkey"),
                            .type = 2) == NULL,
             "global env get with wrong type should return NULL");

  FIO_ASSERT(fio_io_env_unset(NULL,
                              .name = FIO_BUF_INFO1((char *)"gkey"),
                              .type = 1) == 0,
             "global env unset should succeed");
  FIO_ASSERT(fio___test_io_env_close_count == 0,
             "unset should not call on_close");
  FIO_ASSERT(fio_io_env_get(NULL,
                            .name = FIO_BUF_INFO1((char *)"gkey"),
                            .type = 1) == NULL,
             "global env get after unset should return NULL");

  fio_io_env_set(NULL,
                 .name = FIO_BUF_INFO1((char *)"rkey"),
                 .type = 3,
                 .udata = (void *)0x5678,
                 .on_close = fio___test_io_env_on_close);
  FIO_ASSERT(fio_io_env_remove(NULL,
                               .name = FIO_BUF_INFO1((char *)"rkey"),
                               .type = 3) == 0,
             "global env remove should succeed");
  FIO_ASSERT(fio___test_io_env_close_count == 1, "remove should call on_close");

  /* replace existing value */
  fio___test_io_env_close_count = 0;
  fio_io_env_set(NULL,
                 .name = FIO_BUF_INFO1((char *)"rep"),
                 .type = 1,
                 .udata = (void *)0xAAAA,
                 .on_close = fio___test_io_env_on_close);
  fio_io_env_set(NULL,
                 .name = FIO_BUF_INFO1((char *)"rep"),
                 .type = 1,
                 .udata = (void *)0xBBBB,
                 .on_close = fio___test_io_env_on_close);
  FIO_ASSERT(fio___test_io_env_close_count == 1,
             "replacing a value should call old on_close");
  FIO_ASSERT(fio_io_env_get(NULL,
                            .name = FIO_BUF_INFO1((char *)"rep"),
                            .type = 1) == (void *)0xBBBB,
             "replaced value should be visible");
  fio_io_env_remove(NULL, .name = FIO_BUF_INFO1((char *)"rep"), .type = 1);

  fprintf(stderr, "* global env set/get/unset/remove: OK\n");
}

/* *****************************************************************************
Unit tests - TLS context helpers
***************************************************************************** */
static int fio___test_io_tls_each_cert(fio_io_tls_each_s *e,
                                       const char *nm,
                                       const char *public_cert_file,
                                       const char *private_key_file,
                                       const char *pk_password) {
  size_t *c = (size_t *)e->udata2;
  ++(*c);
  (void)nm, (void)public_cert_file, (void)private_key_file, (void)pk_password;
  return 0;
}

static int fio___test_io_tls_each_alpn(fio_io_tls_each_s *e,
                                       const char *nm,
                                       void (*fn)(fio_io_s *)) {
  size_t *c = (size_t *)e->udata2;
  ++(*c);
  (void)nm, (void)fn;
  return 0;
}

static int fio___test_io_tls_each_trust(fio_io_tls_each_s *e, const char *nm) {
  size_t *c = (size_t *)e->udata2;
  ++(*c);
  (void)nm;
  return 0;
}

typedef struct {
  size_t count;
  const char *name;
} fio___test_io_tls_trust_state_s;

static int fio___test_io_tls_capture_trust(fio_io_tls_each_s *e,
                                           const char *nm) {
  fio___test_io_tls_trust_state_s *state =
      (fio___test_io_tls_trust_state_s *)e->udata2;
  ++state->count;
  state->name = nm;
  return 0;
}

static void fio___test_io_tls_alpn_cb(fio_io_s *io) { ++(*(volatile int *)io); }

static void test_io_tls_helpers(void) {
  /* lifecycle and counts */
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls, "fio_io_tls_new should return non-NULL");
  FIO_ASSERT(fio_io_tls_cert_count(tls) == 0, "initial cert count should be 0");
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 0, "initial alpn count should be 0");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 0,
             "initial trust count should be 0");

  /* reference counting */
  FIO_ASSERT(fio_io_tls_dup(tls) == tls, "dup should return same pointer");
  fio_io_tls_free(tls);
  fio_io_tls_free(tls);
  FIO_ASSERT(fio_io_tls_dup(NULL) == NULL, "dup(NULL) should return NULL");
  fio_io_tls_free(NULL);

  /* certificate management */
  tls = fio_io_tls_new();
  FIO_ASSERT(fio_io_tls_cert_add(tls, "s1", "cert.pem", "key.pem", "pass") ==
                 tls,
             "cert_add should return self");
  FIO_ASSERT(fio_io_tls_cert_count(tls) == 1, "cert count should be 1");
  fio_io_tls_cert_add(tls, "s2", "c2.pem", "k2.pem", NULL);
  FIO_ASSERT(fio_io_tls_cert_count(tls) == 2, "cert count should be 2");

  /* ALPN management */
  FIO_ASSERT(fio_io_tls_alpn_add(tls, "h2", fio___test_io_tls_alpn_cb) == tls,
             "alpn_add should return self");
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 1, "alpn count should be 1");
  fio_io_tls_alpn_add(tls, "http/1.1", fio___test_io_tls_alpn_cb);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2, "alpn count should be 2");
  fio_io_tls_alpn_add(tls, NULL, fio___test_io_tls_alpn_cb);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2,
             "NULL alpn name should be ignored");

  volatile int alpn_counter = 0;
  FIO_ASSERT(fio_io_tls_alpn_select(tls, "h2", 2, (fio_io_s *)&alpn_counter) ==
                 0,
             "alpn_select should succeed for h2");
  FIO_ASSERT(alpn_counter == 1, "alpn callback should have fired");
  FIO_ASSERT(fio_io_tls_alpn_select(tls, "missing", 7, NULL) == -1,
             "alpn_select should fail for unknown protocol");

  /* trust management */
  fio_io_tls_trust_add(tls, "ca.pem");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 1, "trust count should be 1");
  fio_io_tls_trust_add(tls, NULL);
  /* NULL adds system trust but does not increment the explicit list */
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 1,
             "NULL trust_add should not increment explicit count");

  /* iteration */
  size_t cert_c = 0, alpn_c = 0, trust_c = 0;
  fio_io_tls_each(tls,
                  .udata2 = &cert_c,
                  .each_cert = fio___test_io_tls_each_cert);
  fio_io_tls_each(tls,
                  .udata2 = &alpn_c,
                  .each_alpn = fio___test_io_tls_each_alpn);
  fio_io_tls_each(tls,
                  .udata2 = &trust_c,
                  .each_trust = fio___test_io_tls_each_trust);
  FIO_ASSERT(cert_c == 2, "each_cert should iterate 2 certs");
  FIO_ASSERT(alpn_c == 2, "each_alpn should iterate 2 protocols");
  FIO_ASSERT(trust_c == 2,
             "each_trust should iterate 1 explicit trust + 1 system trust");

  fio_io_tls_free(tls);

  /* URL-based TLS detection */
  const struct {
    const char *url;
    int expect_tls;
    uintptr_t expect_trust_count;
    size_t expect_trust_each;
    const char *expect_trust_name;
  } url_tests[] = {
      {"http://example.com", 0},
      {"https://example.com", 1},
      {"tcp://example.com", 0},
      {"tcps://example.com", 1},
      {"tls://example.com", 1},
      {"ws://example.com", 0},
      {"wss://example.com", 1},
      {"udp://example.com", 0},
      {"udps://example.com", 1},
      {"http://example.com?trust=ca.pem", 1, 1, 1, "ca.pem"},
      {"http://example.com?trust=sys", 1, 0, 1, NULL},
      {"http://example.com?trust=system", 1, 0, 1, NULL},
      {NULL, 0},
  };
  for (size_t i = 0; url_tests[i].url; ++i) {
    fio_url_s u = fio_url_parse(url_tests[i].url, FIO_STRLEN(url_tests[i].url));
    fio_io_tls_s *t = fio_io_tls_from_url(NULL, u);
    int got = (t != NULL);
    FIO_ASSERT(got == url_tests[i].expect_tls,
               "TLS detection error for %s: expected %d, got %d",
               url_tests[i].url,
               url_tests[i].expect_tls,
               got);
    FIO_ASSERT(fio_io_tls_trust_count(t) == url_tests[i].expect_trust_count,
               "TLS trust count error for %s: expected %zu, got %zu",
               url_tests[i].url,
               (size_t)url_tests[i].expect_trust_count,
               (size_t)fio_io_tls_trust_count(t));
    fio___test_io_tls_trust_state_s trust_state = {0};
    if (t)
      fio_io_tls_each(t,
                      .udata2 = &trust_state,
                      .each_trust = fio___test_io_tls_capture_trust);
    FIO_ASSERT(trust_state.count == url_tests[i].expect_trust_each,
               "TLS trust iteration error for %s: expected %zu, got %zu",
               url_tests[i].url,
               url_tests[i].expect_trust_each,
               trust_state.count);
    FIO_ASSERT(
        (!trust_state.name && !url_tests[i].expect_trust_name) ||
            (trust_state.name && url_tests[i].expect_trust_name &&
             !strcmp(trust_state.name, url_tests[i].expect_trust_name)),
        "TLS trust source error for %s: expected %s, got %s",
        url_tests[i].url,
        url_tests[i].expect_trust_name ? url_tests[i].expect_trust_name
                                       : "system",
        trust_state.name ? trust_state.name : "system");
    fio_io_tls_free(t);
  }

  fprintf(stderr, "* TLS context helpers: OK\n");
}

/* *****************************************************************************
Unit tests - default io_functions table
***************************************************************************** */
static void test_io_default_functions(void) {
  fio_io_functions_s f = {0};
  fio_io_functions_s d = fio_io_tls_default_functions(&f);
  FIO_ASSERT(d.build_context != NULL,
             "default functions should set build_context");
  FIO_ASSERT(d.read != NULL, "default functions should set read");
  FIO_ASSERT(d.write != NULL, "default functions should set write");
  FIO_ASSERT(d.flush != NULL, "default functions should set flush");
  FIO_ASSERT(d.finish != NULL, "default functions should set finish");
  FIO_ASSERT(d.cleanup != NULL, "default functions should set cleanup");

  fio_io_functions_s d2 = fio_io_tls_default_functions(NULL);
  FIO_ASSERT(d2.read == d.read, "repeated default query should be consistent");
  fprintf(stderr, "* default io_functions: OK\n");
}

/* *****************************************************************************
Unit test - invalid-host connection failure

An address-resolution failure occurs before an fio_io_s is created. The
internal connecting protocol routes its own on_close to the application's
on_failed callback, but the application protocol is never attached. Therefore
on_failed is the only application callback and fio_io_free is not called.

The failure teardown is deferred to the IO queue (IO-thread affinity), so
on_failed does NOT fire before fio_io_connect returns - this test drains
the queue to perform it synchronously.
***************************************************************************** */
static fio_io_protocol_s fio___test_io_invalid_host_protocol;
static volatile int fio___test_io_invalid_host_failed_count = 0;
static volatile int fio___test_io_invalid_host_unexpected_count = 0;
static volatile int fio___test_io_invalid_host_close_count = 0;

static void fio___test_io_invalid_host_unexpected(fio_io_s *io) {
  ++fio___test_io_invalid_host_unexpected_count;
  (void)io;
}

static void fio___test_io_invalid_host_on_close(void *buffer, void *udata) {
  ++fio___test_io_invalid_host_close_count;
  (void)buffer, (void)udata;
}

static void fio___test_io_invalid_host_on_failed(fio_io_protocol_s *pr,
                                                  void *udata) {
  ++fio___test_io_invalid_host_failed_count;
  FIO_ASSERT(pr == &fio___test_io_invalid_host_protocol,
             "invalid-host on_failed protocol mismatch");
  FIO_ASSERT(udata == (void *)0xBAD,
             "invalid-host on_failed udata mismatch");
}

static void test_io_connect_invalid_host(void) {
  fio___test_io_invalid_host_failed_count = 0;
  fio___test_io_invalid_host_unexpected_count = 0;
  fio___test_io_invalid_host_close_count = 0;

  fio___test_io_invalid_host_protocol = (fio_io_protocol_s){
      .on_attach = fio___test_io_invalid_host_unexpected,
      .on_data = fio___test_io_invalid_host_unexpected,
      .on_ready = fio___test_io_invalid_host_unexpected,
      .on_close = fio___test_io_invalid_host_on_close,
      .on_timeout = fio___test_io_invalid_host_unexpected,
      .timeout = 100,
  };

  fio_io_s *io = fio_io_connect("tcp://fio-io-test.invalid:19876",
                                .protocol =
                                    &fio___test_io_invalid_host_protocol,
                                .on_failed =
                                    fio___test_io_invalid_host_on_failed,
                                .udata = (void *)0xBAD,
                                .timeout = 100);

  FIO_ASSERT(!io, "invalid-host fio_io_connect should return NULL");
  /* Failure teardown is deferred to the IO queue. Drain it so the deferred
   * fio___connecting_on_close runs on_failed and releases the connecting
   * state before the assertions below. */
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio___test_io_invalid_host_failed_count == 1,
             "invalid-host on_failed should run exactly once (got %d)",
             fio___test_io_invalid_host_failed_count);
  FIO_ASSERT(!fio___test_io_invalid_host_unexpected_count,
             "invalid-host application IO callback should not run (got %d)",
             fio___test_io_invalid_host_unexpected_count);
  FIO_ASSERT(!fio___test_io_invalid_host_close_count,
             "invalid-host application on_close should not run (got %d)",
             fio___test_io_invalid_host_close_count);
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio___io),
             "invalid-host failure should not allocate an IO object");
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio___io_connecting_s),
             "invalid-host connecting state should be cleaned up");

  fprintf(stderr,
          "* invalid-host connect (on_failed=1, application callbacks=0, "
          "IO allocations=0): OK\n");
}

/* *****************************************************************************
Integration tests - protocol callbacks and IO object lifecycle

All integration tests run inside a single reactor instance (workers=0).
A timer-driven test runner advances through a sequence of checks and stops
the reactor when finished.
***************************************************************************** */

/* Protocol used for the attached socketpair end */
static fio_io_protocol_s fio___test_io_pair_protocol;

/* Protocol used for the listen/connect roundtrip */
static fio_io_protocol_s fio___test_io_server_protocol;
static fio_io_protocol_s fio___test_io_client_protocol;
static fio_io_protocol_s fio___test_io_late_protocol;

static fio_socket_i fio___test_io_pair_local = FIO_SOCKET_INVALID;
static fio_io_s *fio___test_io_pair_io = NULL;
static fio_io_listener_s *fio___test_io_listener = NULL;

static volatile int fio___test_io_runner_step = 0;
static volatile int fio___test_io_pair_done = 0;
static volatile int fio___test_io_listen_done = 0;
static volatile int fio___test_io_timer_done = 0;
static volatile int fio___test_io_timeout_fired = 0;
static volatile int fio___test_io_lifecycle_settle_count = 0;
static volatile int fio___test_io_late_started = 0;
static volatile int fio___test_io_late_synchronous = 0;
static volatile int fio___test_io_late_failed_count = 0;
static volatile int fio___test_io_late_unexpected_count = 0;
static volatile int fio___test_io_late_close_count = 0;

/* ---- pair protocol callbacks ---- */
static void fio___test_io_pair_on_attach(fio_io_s *io) {
  ++fio___test_io_attach_count;
  FIO_ASSERT(fio_io_is_open(io) == 1, "attached IO should be open");
  FIO_ASSERT(fio_io_fd(io) != FIO_SOCKET_INVALID,
             "attached IO should have valid fd");
  FIO_ASSERT(fio_io_protocol(io) == &fio___test_io_pair_protocol,
             "protocol pointer should match");
  FIO_ASSERT(fio_io_buffer(io) != NULL, "buffer should be non-NULL");
  FIO_ASSERT(fio_io_buffer_len(io) >= fio___test_io_pair_protocol.buffer_size,
             "buffer length should be at least protocol buffer_size");
}

static void fio___test_io_pair_on_data(fio_io_s *io) {
  ++fio___test_io_data_count;
  char buf[256];
  size_t r = fio_io_read(io, buf, sizeof(buf) - 1);
  if (r > 0 && r < sizeof(fio___test_io_pair_received)) {
    FIO_MEMCPY(fio___test_io_pair_received, buf, r);
    fio___test_io_pair_received_len = r;
  }
}

static void fio___test_io_pair_on_ready(fio_io_s *io) {
  ++fio___test_io_ready_count;
  (void)io;
}

static void fio___test_io_pair_on_close(void *buffer, void *udata) {
  ++fio___test_io_close_count;
  (void)buffer, (void)udata;
}

/* ---- server protocol for listen/connect ---- */
static void fio___test_io_server_on_attach(fio_io_s *io) {
  ++fio___test_io_attach_count;
  FIO_ASSERT(fio_io_protocol(io) == &fio___test_io_server_protocol,
             "server protocol mismatch");
}

static void fio___test_io_server_on_data(fio_io_s *io) {
  ++fio___test_io_data_count;
  char buf[256];
  size_t r = fio_io_read(io, buf, sizeof(buf) - 1);
  if (r > 0) {
    char reply[320];
    int len = snprintf(reply, sizeof(reply), "ECHO:%.*s", (int)r, buf);
    fio_io_write(io, reply, (size_t)len);
  }
}

static void fio___test_io_server_on_ready(fio_io_s *io) {
  ++fio___test_io_ready_count;
  (void)io;
}

static void fio___test_io_server_on_close(void *buffer, void *udata) {
  ++fio___test_io_close_count;
  (void)buffer, (void)udata;
}

/* ---- client protocol for listen/connect ---- */
static void fio___test_io_client_on_attach(fio_io_s *io) {
  FIO_ASSERT(fio_io_protocol(io) == &fio___test_io_client_protocol,
             "client protocol mismatch");
  FIO_ASSERT(fio_io_udata(io) == (void *)0xBEEF,
             "client udata should be preserved");

  void *old = fio_io_udata_set(io, (void *)0xCAFE);
  FIO_ASSERT(old == (void *)0xBEEF, "udata_set should return old value");
  FIO_ASSERT(fio_io_udata(io) == (void *)0xCAFE, "udata should update");

  /* send test message */
  fio_io_write(io, "hello", 5);
}

static void fio___test_io_client_on_data(fio_io_s *io) {
  char buf[256];
  size_t r = fio_io_read(io, buf, sizeof(buf) - 1);
  if (r > 0 && r < sizeof(fio___test_io_listen_received)) {
    FIO_MEMCPY(fio___test_io_listen_received, buf, r);
    fio___test_io_listen_received_len = r;
    fio___test_io_listen_done = 1;
    fio_io_close(io);
  }
  (void)io;
}

static void fio___test_io_client_on_close(void *buffer, void *udata) {
  ++fio___test_io_client_close_count;
  FIO_ASSERT(fio___test_io_client_close_count == 1,
             "client on_close called more than once");
  (void)buffer, (void)udata;
}

static void fio___test_io_client_on_failed(fio_io_protocol_s *pr, void *ud) {
  (void)pr, (void)ud;
  FIO_ASSERT(0, "client connection failed");
}

/* ---- late connection-failure protocol callbacks ---- */
static void fio___test_io_late_unexpected(fio_io_s *io) {
  ++fio___test_io_late_unexpected_count;
  (void)io;
}

static void fio___test_io_late_on_close(void *buffer, void *udata) {
  ++fio___test_io_late_close_count;
  (void)buffer, (void)udata;
}

static void fio___test_io_late_on_failed(fio_io_protocol_s *pr, void *udata) {
  ++fio___test_io_late_failed_count;
  if (!fio___test_io_late_started)
    fio___test_io_late_synchronous = 1;
  FIO_ASSERT(fio___test_io_late_failed_count == 1,
             "late-failure on_failed called more than once");
  FIO_ASSERT(pr == &fio___test_io_late_protocol,
             "late-failure on_failed protocol mismatch");
  FIO_ASSERT(udata == (void *)0xD1E,
             "late-failure on_failed udata mismatch");
}

/* ---- timer callback ---- */
static int fio___test_io_timer_cb(void *u1, void *u2) {
  ++fio___test_io_timer_count;
  (void)u1, (void)u2;
  if (fio___test_io_timer_count >= 3) {
    fio___test_io_timer_done = 1;
    return -1; /* stop timer */
  }
  return 0;
}

/* ---- driver tasks ---- */
static void fio___test_io_attach_pair_task(void *u1, void *u2) {
  (void)u1, (void)u2;
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  FIO_ASSERT(!fio_sock_socketpair(fds),
             "socketpair failed: %s",
             strerror(errno));

  fio___test_io_pair_local = fds[1];
  fio___test_io_pair_io = fio_io_attach_fd(fds[0],
                                           &fio___test_io_pair_protocol,
                                           (void *)0xFACE,
                                           NULL);
  FIO_ASSERT(fio___test_io_pair_io, "attach_fd should return non-NULL");
  FIO_ASSERT(fio_io_fd(fio___test_io_pair_io) == fds[0],
             "fd on IO object should match attached fd");

  /* verify initial open/suspended state */
  FIO_ASSERT(fio_io_is_open(fio___test_io_pair_io) == 1,
             "freshly attached IO should be open");
  FIO_ASSERT(fio_io_is_suspended(fio___test_io_pair_io) == 0,
             "freshly attached IO should not be suspended");

  /* verify dup keeps the pointer usable */
  fio_io_s *dup = fio_io_dup(fio___test_io_pair_io);
  FIO_ASSERT(dup == fio___test_io_pair_io, "dup should return same pointer");
  fio_io_free(dup);

  /* write from local end; reactor should fire on_data on the attached end */
  FIO_ASSERT(fio_sock_write(fio___test_io_pair_local, "pair", 4) == 4,
             "socketpair write failed");
}

static int fio___test_io_check_pair_task(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (!fio___test_io_pair_io || fio___test_io_pair_done)
    return 0;

  if (fio___test_io_pair_received_len == 4 &&
      !FIO_MEMCMP(fio___test_io_pair_received, "pair", 4)) {
    fio___test_io_pair_done = 1;

    /* test suspend / unsuspend */
    fio_io_suspend(fio___test_io_pair_io);
    FIO_ASSERT(fio_io_is_suspended(fio___test_io_pair_io) == 1,
               "suspend should set suspended flag");
    fio_io_unsuspend(fio___test_io_pair_io);
    FIO_ASSERT(fio_io_is_suspended(fio___test_io_pair_io) == 0,
               "unsuspend should clear suspended flag");

    /* test touch and backlog */
    fio_io_touch(fio___test_io_pair_io);
    FIO_ASSERT(fio_io_backlog(fio___test_io_pair_io) == 0,
               "backlog should be 0 after read");

    /* test env on the IO object */
    fio___test_io_env_close_count = 0;
    fio_io_env_set(fio___test_io_pair_io,
                   .name = FIO_BUF_INFO1((char *)"iokey"),
                   .type = 1,
                   .udata = (void *)0x4321,
                   .on_close = fio___test_io_env_on_close);
    FIO_ASSERT(fio_io_env_get(fio___test_io_pair_io,
                              .name = FIO_BUF_INFO1((char *)"iokey"),
                              .type = 1) == (void *)0x4321,
               "IO env get should return set value");
    FIO_ASSERT(fio_io_env_unset(fio___test_io_pair_io,
                                .name = FIO_BUF_INFO1((char *)"iokey"),
                                .type = 1) == 0,
               "IO env unset should succeed");

    /* test protocol_each counts this IO */
    fio___test_io_each_count = 0;
    size_t n = fio_io_protocol_each(&fio___test_io_pair_protocol,
                                    fio___test_io_each_task,
                                    NULL);
    FIO_ASSERT(n >= 1, "protocol_each should find at least one IO");
    FIO_ASSERT(fio___test_io_each_count >= 1,
               "protocol_each task should have run");

    /* A graceful close changes the public predicate immediately, but must
     * preserve the internal OPEN bit until previously scheduled output drains. */
    fio_io_write(fio___test_io_pair_io, "drain", 5);
    fio_io_close(fio___test_io_pair_io);
    FIO_ASSERT(!fio_io_is_open(fio___test_io_pair_io),
               "IO marked for closure must not report open");
    FIO_ASSERT(fio___test_io_pair_io->flags & FIO___IO_FLAG_OPEN,
               "graceful close must retain OPEN while output drains");
  }
  return 0;
}

static void fio___test_io_start_listen_task(void *u1, void *u2) {
  (void)u1, (void)u2;
  fio___test_io_listener =
      fio_io_listen(.url = "tcp://127.0.0.1:19876",
                    .protocol = &fio___test_io_server_protocol,
                    .udata = (void *)0x9999,
                    .hide_from_log = 1);
  FIO_ASSERT(fio___test_io_listener, "listen should succeed");

  fio_buf_info_s url = fio_io_listener_url(fio___test_io_listener);
  FIO_ASSERT(url.len > 0, "listener URL should not be empty");
  FIO_ASSERT(fio_io_listener_protocol(fio___test_io_listener) ==
                 &fio___test_io_server_protocol,
             "listener protocol mismatch");
  FIO_ASSERT(fio_io_listener_udata(fio___test_io_listener) == (void *)0x9999,
             "listener udata mismatch");
  FIO_ASSERT(fio_io_listener_is_tls(fio___test_io_listener) == 0,
             "listener should not report TLS");
  (void)url;

  fio_io_s *io = fio_io_connect("tcp://127.0.0.1:19876",
                                .protocol = &fio___test_io_client_protocol,
                                .on_failed = fio___test_io_client_on_failed,
                                .udata = (void *)0xBEEF,
                                .timeout = 5000);
  FIO_ASSERT(io, "connect should succeed");
}

static void fio___test_io_start_late_failure_task(void *u1, void *u2) {
  (void)u1, (void)u2;
  /* Reserve an ephemeral loopback port, then close its listener before
   * connecting. The resulting refused connection exercises the asynchronous
   * failure path without relying on DNS or an external network endpoint. */
  fio_socket_i listener =
      fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER | FIO_SOCK_TCP);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(listener),
             "failed to reserve loopback port for late failure test");
  struct sockaddr_in address = {0};
  socklen_t address_len = sizeof(address);
  FIO_ASSERT(!getsockname(listener,
                           (struct sockaddr *)&address,
                           &address_len),
             "getsockname failed while reserving loopback port");
  fio_sock_close(listener);

  char url[64];
  snprintf(url,
           sizeof(url),
           "tcp://127.0.0.1:%u",
           ntohs(address.sin_port));
  fio_io_s *io = fio_io_connect(url,
                                .protocol = &fio___test_io_late_protocol,
                                .on_failed = fio___test_io_late_on_failed,
                                .udata = (void *)0xD1E,
                                .timeout = 1500);
  if (!io) {
    fio___test_io_late_synchronous = 1;
    return;
  }
  fio___test_io_late_started = 1;
}

static int fio___test_io_check_done_task(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (fio___test_io_pair_done && fio___test_io_listen_done &&
      fio___test_io_timer_done && fio___test_io_client_close_count == 1 &&
      fio___test_io_late_failed_count == 1 &&
      (fio___test_io_late_started || fio___test_io_late_synchronous)) {
    /* Keep polling briefly after the expected lifecycle callbacks. This
     * catches duplicate close/free tasks that arrive on a later poll cycle. */
    if (++fio___test_io_lifecycle_settle_count >= 5) {
      fio_io_stop();
      return -1;
    }
  }
  return 0;
}

static int fio___test_io_timeout_cb(void *u1, void *u2) {
  (void)u1, (void)u2;
  fio___test_io_timeout_fired = 1;
  FIO_LOG_ERROR(
      "io integration timeout (pair_done=%d listen_done=%d timer_done=%d "
      "client_on_close=%d late_started=%d late_synchronous=%d "
      "late_on_failed=%d late_callbacks=%d late_on_close=%d)",
      fio___test_io_pair_done,
      fio___test_io_listen_done,
      fio___test_io_timer_done,
      fio___test_io_client_close_count,
      fio___test_io_late_started,
      fio___test_io_late_synchronous,
      fio___test_io_late_failed_count,
      fio___test_io_late_unexpected_count,
      fio___test_io_late_close_count);
  fio_io_stop();
  return -1;
}

static void fio___test_io_on_start(void *ignr_) {
  (void)ignr_;
  /* Schedule driver tasks and timers. */
  fio_io_defer(fio___test_io_attach_pair_task, NULL, NULL);
  fio_io_defer(fio___test_io_start_listen_task, NULL, NULL);
  fio_io_defer(fio___test_io_start_late_failure_task, NULL, NULL);

  fio_io_run_every(.fn = fio___test_io_check_pair_task,
                   .every = 20,
                   .repetitions = -1);
  fio_io_run_every(.fn = fio___test_io_check_done_task,
                   .every = 20,
                   .repetitions = -1);
  fio_io_run_every(.fn = fio___test_io_timer_cb,
                   .every = 10,
                   .repetitions = -1);
  fio_io_run_every(.fn = fio___test_io_timeout_cb,
                   .every = 7000,
                   .repetitions = 1);
}

/* *****************************************************************************
Integration test runner
***************************************************************************** */
static void test_io_integration(void) {
#if !FIO_OS_POSIX
  test_io_skipped();
  return;
#else
  /* reset state */
  fio___test_io_attach_count = 0;
  fio___test_io_data_count = 0;
  fio___test_io_ready_count = 0;
  fio___test_io_close_count = 0;
  fio___test_io_client_close_count = 0;
  fio___test_io_pair_received_len = 0;
  fio___test_io_listen_received_len = 0;
  fio___test_io_pair_local = FIO_SOCKET_INVALID;
  fio___test_io_pair_io = NULL;
  fio___test_io_listener = NULL;
  fio___test_io_runner_step = 0;
  fio___test_io_pair_done = 0;
  fio___test_io_listen_done = 0;
  fio___test_io_timer_done = 0;
  fio___test_io_timeout_fired = 0;
  fio___test_io_lifecycle_settle_count = 0;
  fio___test_io_late_started = 0;
  fio___test_io_late_synchronous = 0;
  fio___test_io_late_failed_count = 0;
  fio___test_io_late_unexpected_count = 0;
  fio___test_io_late_close_count = 0;
  fio___test_io_timer_count = 0;

  /* initialize protocols */
  fio___test_io_pair_protocol = (fio_io_protocol_s){
      .on_attach = fio___test_io_pair_on_attach,
      .on_data = fio___test_io_pair_on_data,
      .on_ready = fio___test_io_pair_on_ready,
      .on_close = fio___test_io_pair_on_close,
      .on_timeout = fio_io_touch,
      .timeout = 5000,
      .buffer_size = 128,
  };
  fio___test_io_server_protocol = (fio_io_protocol_s){
      .on_attach = fio___test_io_server_on_attach,
      .on_data = fio___test_io_server_on_data,
      .on_ready = fio___test_io_server_on_ready,
      .on_close = fio___test_io_server_on_close,
      .on_timeout = fio_io_touch,
      .timeout = 5000,
  };
  fio___test_io_client_protocol = (fio_io_protocol_s){
      .on_attach = fio___test_io_client_on_attach,
      .on_data = fio___test_io_client_on_data,
      .on_close = fio___test_io_client_on_close,
      .on_timeout = fio_io_touch,
      .timeout = 5000,
      .buffer_size = 128,
  };
  fio___test_io_late_protocol = (fio_io_protocol_s){
      .on_attach = fio___test_io_late_unexpected,
      .on_data = fio___test_io_late_unexpected,
      .on_ready = fio___test_io_late_unexpected,
      .on_close = fio___test_io_late_on_close,
      .on_timeout = fio___test_io_late_unexpected,
      .timeout = 1500,
  };

  fio_state_callback_add(FIO_CALL_ON_START, fio___test_io_on_start, NULL);
  fio_io_start(0);
  fio_state_callback_remove(FIO_CALL_ON_START, fio___test_io_on_start, NULL);

  FIO_ASSERT(!fio___test_io_timeout_fired, "integration timed out");
  FIO_ASSERT(fio___test_io_pair_done, "socketpair roundtrip did not complete");
  FIO_ASSERT(fio___test_io_listen_done,
             "listen/connect roundtrip did not complete");
  FIO_ASSERT(fio___test_io_timer_done, "timer did not fire enough times");
  FIO_ASSERT(fio___test_io_attach_count >= 2,
             "on_attach should have fired for both accepted and pair IOs");
  FIO_ASSERT(fio___test_io_data_count >= 2,
             "on_data should have fired for both roundtrips");
  FIO_ASSERT(fio___test_io_listen_received_len == 10,
             "listen roundtrip received wrong length (%zu)",
             fio___test_io_listen_received_len);
  FIO_ASSERT(!FIO_MEMCMP(fio___test_io_listen_received, "ECHO:hello", 10),
             "listen roundtrip received wrong data");
  FIO_ASSERT(fio___test_io_client_close_count == 1,
             "client on_close should run exactly once (got %d)",
             fio___test_io_client_close_count);
  FIO_ASSERT(fio___test_io_late_started ||
                 fio___test_io_late_synchronous,
             "late-failure connection produced no observable outcome");
  FIO_ASSERT(fio___test_io_late_failed_count == 1,
             "late-failure on_failed should run exactly once (got %d)",
             fio___test_io_late_failed_count);
  FIO_ASSERT(!fio___test_io_late_unexpected_count,
             "late-failure application IO callback should not run (got %d)",
             fio___test_io_late_unexpected_count);
  FIO_ASSERT(!fio___test_io_late_close_count,
             "late-failure application on_close should not run (got %d)",
             fio___test_io_late_close_count);
  if (fio___test_io_late_synchronous)
    FIO_ASSERT(!fio___test_io_late_started,
               "late-failure cannot be both synchronous and reactor-started");
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio___io),
             "IO objects should be fully released");
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio___io_connecting_s),
             "late-failure connecting state should be cleaned up");

  FIO_ASSERT(fio___test_io_close_count >= 1,
             "graceful pair close must deliver on_close");
  char drained[8] = {0};
  FIO_ASSERT(
      fio_sock_read(fio___test_io_pair_local, drained, sizeof(drained)) == 5 &&
          !FIO_MEMCMP(drained, "drain", 5),
      "graceful close must drain previously scheduled output");
  FIO_ASSERT(
      fio_sock_read(fio___test_io_pair_local, drained, sizeof(drained)) == 0,
      "pair peer must observe EOF after drained output");

  /* Cleanup the local end of the socketpair if it is still open. */
  if (FIO_SOCK_FD_ISVALID(fio___test_io_pair_local)) {
    fio_sock_close(fio___test_io_pair_local);
    fio___test_io_pair_local = FIO_SOCKET_INVALID;
  }

  fprintf(stderr,
          "* IO reactor integration (client on_close=1, IO allocations=0): "
          "OK\n");
  if (fio___test_io_late_synchronous)
    fprintf(stderr,
            "* WARNING: late connect ended synchronously; reactor lifecycle "
            "coverage skipped (on_failed=1, IO allocations=0)\n");
  else
    fprintf(stderr,
            "* late connect failure (on_failed=1, application callbacks=0, "
            "IO allocations=0): OK\n");
#endif
}

/* Asserts inside a running reactor: stop the reactor first, otherwise the
 * at-exit cleanup keeps performing the (self-rescheduling) reactor task and a
 * failing test hangs instead of exiting. */
#define FIO___TEST_IO_REACTOR_ASSERT(cond, ...)                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fio_io_stop();                                                           \
      FIO_ASSERT(0, __VA_ARGS__);                                              \
    }                                                                          \
  } while (0)

/* *****************************************************************************
Regression - one protocol shared by a listener and fio_io_connect

`fio_io_connect` used to re-initialize the protocol unconditionally,
resetting `reserved.ios` / `reserved.protocols` while live IOs were linked.
This orphaned live IOs and double-linked the protocol into the reactor's
protocol list (cycles -> reactor livelock, or writes into freed IOs).
Assertions fire before the corruption can livelock the reactor.
***************************************************************************** */
#define FIO___TEST_IO_SHARED_MAX 8
static fio_io_protocol_s fio___test_io_shared_protocol;
static fio_io_s *fio___test_io_shared_ios[FIO___TEST_IO_SHARED_MAX];
static volatile int fio___test_io_shared_attached = 0;
static volatile int fio___test_io_shared_closed = 0;
static volatile int fio___test_io_shared_failed = 0;
static volatile int fio___test_io_shared_step = 0;
static volatile int fio___test_io_shared_done = 0;
static volatile int fio___test_io_shared_ticks = 0;
static char fio___test_io_shared_url[64];

static void fio___test_io_shared_on_attach(fio_io_s *io) {
  FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_attached < FIO___TEST_IO_SHARED_MAX,
             "shared protocol: too many attached IOs");
  /* hold a reference so the test may close the IO safely later */
  fio___test_io_shared_ios[fio___test_io_shared_attached++] = fio_io_dup(io);
}
static void fio___test_io_shared_on_data(fio_io_s *io) {
  char buf[64];
  while (fio_io_read(io, buf, sizeof(buf)))
    ;
}
static void fio___test_io_shared_on_close(void *buffer, void *udata) {
  ++fio___test_io_shared_closed;
  (void)buffer, (void)udata;
}
static void fio___test_io_shared_on_failed(fio_io_protocol_s *pr, void *ud) {
  ++fio___test_io_shared_failed;
  (void)pr, (void)ud;
}

static void fio___test_io_shared_count_task(fio_io_s *io, void *count) {
  ++*(size_t *)count;
  (void)io;
}
/* IOs reachable through the protocol's `reserved.ios` list. */
static size_t fio___test_io_shared_ios_count(void) {
  size_t count = 0;
  fio_io_protocol_each(&fio___test_io_shared_protocol,
                       fio___test_io_shared_count_task,
                       &count);
  return count;
}
/* Times the protocol is linked in the reactor's protocol list (bounded, as a
 * corrupted list may cycle). */
static size_t fio___test_io_shared_links(void) {
  size_t links = 0, guard = 0;
  FIO_LIST_EACH(fio_io_protocol_s,
                reserved.protocols,
                &FIO___IO.protocols,
                pr) {
    links += (pr == &fio___test_io_shared_protocol);
    if (++guard > 64)
      break;
  }
  return links;
}

static void fio___test_io_shared_connect(void) {
  fio_io_s *io = fio_io_connect(fio___test_io_shared_url,
                                .protocol = &fio___test_io_shared_protocol,
                                .on_failed = fio___test_io_shared_on_failed,
                                .timeout = 5000);
  FIO_ASSERT(io, "shared protocol: fio_io_connect failed");
}

static int fio___test_io_shared_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  FIO___TEST_IO_REACTOR_ASSERT(++fio___test_io_shared_ticks < 500,
             "shared protocol: test timed out at step %d (attached %d, "
             "closed %d, failed %d)",
             fio___test_io_shared_step,
             fio___test_io_shared_attached,
             fio___test_io_shared_closed,
             fio___test_io_shared_failed);
  FIO___TEST_IO_REACTOR_ASSERT(!fio___test_io_shared_failed,
             "shared protocol: a connection failed");
  switch (fio___test_io_shared_step) {
  case 0: /* first client: one accepted + one client IO share the protocol */
    fio___test_io_shared_connect();
    fio___test_io_shared_step = 1;
    return 0;
  case 1:
    if (fio___test_io_shared_attached < 2)
      return 0;
    FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_ios_count() == 2,
               "shared protocol: expected 2 IOs, found %zu",
               fio___test_io_shared_ios_count());
    /* second client while the protocol has live IOs */
    fio___test_io_shared_connect();
    FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_ios_count() == 2,
               "fio_io_connect orphaned the protocol's live IOs "
               "(expected 2, found %zu)",
               fio___test_io_shared_ios_count());
    FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_links() == 1,
               "fio_io_connect corrupted the reactor protocol list");
    fio___test_io_shared_step = 2;
    return 0;
  case 2:
    if (fio___test_io_shared_attached < 4)
      return 0;
    FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_ios_count() == 4,
               "shared protocol: expected 4 IOs, found %zu",
               fio___test_io_shared_ios_count());
    FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_links() == 1,
               "shared protocol linked %zu times in the reactor list",
               fio___test_io_shared_links());
    for (int i = 0; i < fio___test_io_shared_attached; ++i) {
      fio_io_close_now(fio___test_io_shared_ios[i]);
      fio_io_free(fio___test_io_shared_ios[i]);
      fio___test_io_shared_ios[i] = NULL;
    }
    fio___test_io_shared_step = 3;
    return 0;
  case 3:
    if (fio___test_io_shared_closed < 4)
      return 0;
    FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_ios_count() == 0,
               "shared protocol: closed IOs still listed (%zu)",
               fio___test_io_shared_ios_count());
    FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_shared_links() == 0,
               "empty shared protocol still in the reactor list");
    fio___test_io_shared_done = 1;
    fio_io_stop();
    return -1;
  }
  return 0;
}

static void test_io_shared_protocol_listen_connect(void) {
  FIO_MEMSET(fio___test_io_shared_ios, 0, sizeof(fio___test_io_shared_ios));
  fio___test_io_shared_attached = 0;
  fio___test_io_shared_closed = 0;
  fio___test_io_shared_failed = 0;
  fio___test_io_shared_step = 0;
  fio___test_io_shared_done = 0;
  fio___test_io_shared_ticks = 0;
  fio___test_io_shared_protocol = (fio_io_protocol_s){
      .on_attach = fio___test_io_shared_on_attach,
      .on_data = fio___test_io_shared_on_data,
      .on_close = fio___test_io_shared_on_close,
      .on_timeout = fio_io_touch,
  };

  /* reserve an ephemeral loopback port for the listener */
  fio_socket_i reserve =
      fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER | FIO_SOCK_TCP);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(reserve), "failed to reserve loopback port");
  struct sockaddr_in address = {0};
  socklen_t address_len = sizeof(address);
  FIO_ASSERT(!getsockname(reserve, (struct sockaddr *)&address, &address_len),
             "getsockname failed while reserving loopback port");
  fio_sock_close(reserve);
  snprintf(fio___test_io_shared_url,
           sizeof(fio___test_io_shared_url),
           "tcp://127.0.0.1:%u",
           (unsigned)ntohs(address.sin_port));

  fio_io_listener_s *listener =
      fio_io_listen(.url = fio___test_io_shared_url,
                    .protocol = &fio___test_io_shared_protocol,
                    .hide_from_log = 1);
  FIO_ASSERT(listener, "shared protocol: listen failed");
  fio_io_run_every(.fn = fio___test_io_shared_driver,
                   .every = 10,
                   .repetitions = -1);
  fio_io_start(0);
  fio_io_listen_stop(listener);

  FIO_ASSERT(fio___test_io_shared_done,
             "shared protocol: test did not complete");
  FIO_ASSERT(fio___test_io_shared_closed == 4,
             "shared protocol: expected 4 on_close calls, got %d",
             fio___test_io_shared_closed);
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio___io),
             "shared protocol: IO objects should be fully released");
  fprintf(stderr, "* shared protocol listen + connect (list integrity): OK\n");
}
#undef FIO___TEST_IO_SHARED_MAX

/* *****************************************************************************
Regression - protocol_set(io, NULL) ("zombie" / disengaged IO) lifecycle

An IO with a non-default transport (e.g., TLS) keeps its IO functions when
set to NULL (a temporary protocol, freed on close). Plaintext IOs use the
shared mock protocol. The caller owns `udata` / buffer resources. Covers:
re-NULL (no extra allocation, no IO leak), revival (transport carries over,
no second `start`), same-protocol re-set (no IO leak) and the write tail.
***************************************************************************** */
typedef enum {
  FIO___TEST_IO_ZOMBIE_CLOSE,   /* real -> NULL -> close */
  FIO___TEST_IO_ZOMBIE_RENULL,  /* real -> NULL -> NULL -> close */
  FIO___TEST_IO_ZOMBIE_REVIVE,  /* real -> NULL -> real2 -> close */
  FIO___TEST_IO_ZOMBIE_WRITE,   /* real -> NULL, write tail -> close */
  FIO___TEST_IO_ZOMBIE_SAME,    /* real -> same real -> close */
  FIO___TEST_IO_ZOMBIE_PLAIN,   /* plaintext -> NULL (mock), write -> close */
  FIO___TEST_IO_ZOMBIE_CASES
} fio___test_io_zombie_case_e;

static const char *fio___test_io_zombie_names[] = {
    "real -> NULL -> close",
    "real -> NULL -> NULL -> close",
    "real -> NULL -> real2 -> close",
    "real -> NULL -> write tail -> close",
    "real -> same real -> close",
    "plaintext -> NULL -> write tail -> close",
};

static struct {
  fio___test_io_zombie_case_e tcase;
  int step;
  int ticks;
  int done;
  int start_calls;
  int custom_reads;
  int custom_writes;
  int attach_calls;
  int close_p1;
  int close_p2;
  int close_plain;
  fio_io_s *io;
  fio_io_protocol_s *zombie;
  fio_socket_i peer;
  char tail[16];
  size_t tail_len;
} fio___test_io_zombie;

static ssize_t fio___test_io_zombie_read(fio_socket_i fd,
                                         void *buf,
                                         size_t len,
                                         void *tls) {
  ++fio___test_io_zombie.custom_reads;
  return fio___io_func_default_read(fd, buf, len, tls);
}
static ssize_t fio___test_io_zombie_write(fio_socket_i fd,
                                          const void *buf,
                                          size_t len,
                                          void *tls) {
  ++fio___test_io_zombie.custom_writes;
  return fio___io_func_default_write(fd, buf, len, tls);
}
static void fio___test_io_zombie_start(fio_io_s *io) {
  ++fio___test_io_zombie.start_calls;
  (void)io;
}
static void fio___test_io_zombie_on_attach(fio_io_s *io) {
  ++fio___test_io_zombie.attach_calls;
  (void)io;
}
static void fio___test_io_zombie_close_p1(void *b, void *u) {
  ++fio___test_io_zombie.close_p1;
  (void)b, (void)u;
}
static void fio___test_io_zombie_close_p2(void *b, void *u) {
  ++fio___test_io_zombie.close_p2;
  (void)b, (void)u;
}
static void fio___test_io_zombie_close_plain(void *b, void *u) {
  ++fio___test_io_zombie.close_plain;
  (void)b, (void)u;
}

static fio_io_protocol_s fio___test_io_zombie_p1;
static fio_io_protocol_s fio___test_io_zombie_p2;
static fio_io_protocol_s fio___test_io_zombie_plain;

/* Reads the tail written to the peer (non-blocking, retried per tick). */
static int fio___test_io_zombie_read_tail(void) {
  ssize_t r = fio_sock_read(
      fio___test_io_zombie.peer,
      fio___test_io_zombie.tail + fio___test_io_zombie.tail_len,
      sizeof(fio___test_io_zombie.tail) - 1 - fio___test_io_zombie.tail_len);
  if (r > 0)
    fio___test_io_zombie.tail_len += (size_t)r;
  return fio___test_io_zombie.tail_len >= 4;
}

static int fio___test_io_zombie_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  const fio___test_io_zombie_case_e tc = fio___test_io_zombie.tcase;
  fio_io_s *io = fio___test_io_zombie.io;
  FIO___TEST_IO_REACTOR_ASSERT(++fio___test_io_zombie.ticks < 300,
             "zombie (%s): timed out at step %d",
             fio___test_io_zombie_names[tc],
             fio___test_io_zombie.step);
  switch (fio___test_io_zombie.step) {
  case 0: /* first switch */
    fio_io_protocol_set(io,
                        (tc == FIO___TEST_IO_ZOMBIE_SAME)
                            ? &fio___test_io_zombie_p1
                            : NULL);
    break;
  case 1: /* inspect the first switch, perform the second action */
    if (tc == FIO___TEST_IO_ZOMBIE_SAME) {
      FIO___TEST_IO_REACTOR_ASSERT(fio_io_protocol(io) == &fio___test_io_zombie_p1,
                 "same-protocol re-set should keep the protocol");
    } else if (tc == FIO___TEST_IO_ZOMBIE_PLAIN) {
      FIO___TEST_IO_REACTOR_ASSERT(fio_io_protocol(io) == &FIO___IO_MOCK_PROTOCOL,
                 "plaintext IO set to NULL should use the shared mock");
    } else {
      fio___test_io_zombie.zombie = fio_io_protocol(io);
      FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_zombie.zombie != &FIO___IO_MOCK_PROTOCOL &&
                     fio___test_io_zombie.zombie != &fio___test_io_zombie_p1,
                 "custom transport IO set to NULL should get its own "
                 "temporary protocol");
      FIO___TEST_IO_REACTOR_ASSERT(fio___test_io_zombie.zombie->io_functions.read ==
                         fio___test_io_zombie_read &&
                     fio___test_io_zombie.zombie->io_functions.write ==
                         fio___test_io_zombie_write,
                 "temporary protocol should keep the IO's transport");
    }
    if (tc == FIO___TEST_IO_ZOMBIE_RENULL)
      fio_io_protocol_set(io, NULL);
    else if (tc == FIO___TEST_IO_ZOMBIE_REVIVE)
      fio_io_protocol_set(io, &fio___test_io_zombie_p2);
    else if (tc == FIO___TEST_IO_ZOMBIE_WRITE ||
             tc == FIO___TEST_IO_ZOMBIE_PLAIN)
      fio_io_write(io, "tail", 4);
    break;
  case 2: /* inspect the second action, then close */
    if (tc == FIO___TEST_IO_ZOMBIE_RENULL)
      FIO___TEST_IO_REACTOR_ASSERT(fio_io_protocol(io) == fio___test_io_zombie.zombie,
                 "re-NULL should keep the same temporary protocol");
    if (tc == FIO___TEST_IO_ZOMBIE_REVIVE)
      FIO___TEST_IO_REACTOR_ASSERT(fio_io_protocol(io) == &fio___test_io_zombie_p2,
                 "revived IO should use the new protocol");
    if ((tc == FIO___TEST_IO_ZOMBIE_WRITE ||
         tc == FIO___TEST_IO_ZOMBIE_PLAIN) &&
        !fio___test_io_zombie_read_tail())
      return 0; /* tail not arrived yet - retry next tick */
    fio_io_close_now(io);
    fio___test_io_zombie.io = NULL;
    break;
  default: /* wait for destruction */
    if (FIO_LEAK_COUNTER_COUNT(fio___io) > 1) /* the wakeup IO remains */
      return 0;
    fio___test_io_zombie.done = 1;
    fio_io_stop();
    return -1;
  }
  ++fio___test_io_zombie.step;
  return 0;
}

static void fio___test_io_zombie_run(fio___test_io_zombie_case_e tc) {
  FIO_MEMSET(&fio___test_io_zombie, 0, sizeof(fio___test_io_zombie));
  fio___test_io_zombie.tcase = tc;
  fio_socket_i fds[2];
  FIO_ASSERT(!fio_sock_socketpair(fds), "zombie: socketpair failed");
  fio_sock_set_non_block(fds[1]);
  fio___test_io_zombie.peer = fds[1];
  fio___test_io_zombie.io =
      fio_io_attach_fd(fds[0],
                       (tc == FIO___TEST_IO_ZOMBIE_PLAIN)
                           ? &fio___test_io_zombie_plain
                           : &fio___test_io_zombie_p1,
                       NULL,
                       NULL);
  FIO_ASSERT(fio___test_io_zombie.io, "zombie: attach failed");
  fio_io_run_every(.fn = fio___test_io_zombie_driver,
                   .every = 10,
                   .repetitions = -1);
  fio_io_start(0);
  fio_sock_close(fio___test_io_zombie.peer);

  const char *nm = fio___test_io_zombie_names[tc];
  FIO_ASSERT(fio___test_io_zombie.done, "zombie (%s): did not complete", nm);
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio___io),
             "zombie (%s): IO object leaked",
             nm);
  if (tc != FIO___TEST_IO_ZOMBIE_PLAIN) {
    FIO_ASSERT(fio___test_io_zombie.start_calls == 1,
               "zombie (%s): transport `start` should run once (ran %d)",
               nm,
               fio___test_io_zombie.start_calls);
    FIO_ASSERT(fio___test_io_zombie.attach_calls ==
                   1 + (tc == FIO___TEST_IO_ZOMBIE_REVIVE),
               "zombie (%s): unexpected on_attach count (%d)",
               nm,
               fio___test_io_zombie.attach_calls);
  }
  /* the user's on_close runs only for the protocol the IO closed with */
  FIO_ASSERT(fio___test_io_zombie.close_p1 ==
                 (tc == FIO___TEST_IO_ZOMBIE_SAME),
             "zombie (%s): unexpected protocol 1 on_close count (%d)",
             nm,
             fio___test_io_zombie.close_p1);
  FIO_ASSERT(fio___test_io_zombie.close_p2 ==
                 (tc == FIO___TEST_IO_ZOMBIE_REVIVE),
             "zombie (%s): unexpected protocol 2 on_close count (%d)",
             nm,
             fio___test_io_zombie.close_p2);
  FIO_ASSERT(!fio___test_io_zombie.close_plain,
             "zombie (%s): plaintext on_close should not run after NULL",
             nm);
  if (tc == FIO___TEST_IO_ZOMBIE_WRITE || tc == FIO___TEST_IO_ZOMBIE_PLAIN)
    FIO_ASSERT(fio___test_io_zombie.tail_len == 4 &&
                   !FIO_MEMCMP(fio___test_io_zombie.tail, "tail", 4),
               "zombie (%s): tail not delivered",
               nm);
  if (tc == FIO___TEST_IO_ZOMBIE_WRITE)
    FIO_ASSERT(fio___test_io_zombie.custom_writes > 0,
               "zombie (%s): tail should use the IO's transport",
               nm);
}

static void test_io_protocol_set_null_lifecycle(void) {
  fio___test_io_zombie_p1 = (fio_io_protocol_s){
      .on_attach = fio___test_io_zombie_on_attach,
      .on_close = fio___test_io_zombie_close_p1,
      .io_functions =
          {
              .start = fio___test_io_zombie_start,
              .read = fio___test_io_zombie_read,
              .write = fio___test_io_zombie_write,
          },
  };
  fio___test_io_zombie_p2 = fio___test_io_zombie_p1;
  fio___test_io_zombie_p2.on_close = fio___test_io_zombie_close_p2;
  fio___test_io_zombie_plain = (fio_io_protocol_s){
      .on_close = fio___test_io_zombie_close_plain,
  };
  for (int i = 0; i < FIO___TEST_IO_ZOMBIE_CASES; ++i)
    fio___test_io_zombie_run((fio___test_io_zombie_case_e)i);
  fprintf(stderr,
          "* protocol_set(io, NULL) lifecycle (%d cases, no leaks): OK\n",
          (int)FIO___TEST_IO_ZOMBIE_CASES);
}

static unsigned test_io_close_order_calls;
static void test_io_close_order_on_close(void *buf, void *udata) {
  (void)buf;
  (void)udata;
  ++test_io_close_order_calls;
}

static void test_io_close_write_order(void) {
  for (unsigned closing = 0; closing < 2; ++closing) {
    fio_socket_i fds[2];
    FIO_ASSERT(!fio_sock_socketpair(fds), "close ordering socketpair failed");
    fio_io_protocol_s protocol = {.on_close = test_io_close_order_on_close};
    fio_io_s *io = fio_io_attach_fd(fds[0], &protocol, NULL, NULL);
    FIO_ASSERT(io, "close ordering attach failed");
    fio_io_dup(io); /* observation reference after graceful close */
    fio_queue_perform_all(fio_io_queue());
    /* A ready task for earlier output may precede an accepted final write. */
    fio___io_poll_on_ready_schd(io);
    fio_io_write(io, "tail", 4);
    if (closing) {
      fio_io_close(io);
      FIO_ASSERT(!fio_io_is_open(io), "pending graceful close must report closed");
      fio_io_close(io); /* repeated close must be harmless */
    }
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(!!fio_io_is_open(io) == !closing,
               "dirty writes must not close an IO without a close request");
    char buf[8];
    ssize_t len = fio_sock_read(fds[1], buf, sizeof(buf));
    FIO_ASSERT(len == 4 && !FIO_MEMCMP(buf, "tail", 4),
               "earlier ready task must not discard final accepted write");
    fio_io_close(io); /* close dirty-only control, or repeat after draining */
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(!(io->flags & FIO___IO_FLAG_OPEN),
               "graceful close must finish after write tasks drain");
    fio_io_free(io);
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(fio_sock_read(fds[1], buf, sizeof(buf)) == 0 &&
                   test_io_close_order_calls == closing + 1,
               "close ordering must deliver EOF and one close callback");
    fio_sock_close(fds[1]);
  }
}

static void test_io_open_predicate(void) {
  fio_io_s io = {0};
  FIO_ASSERT(!fio_io_is_open(&io), "zeroed IO must not report open");
  io.flags = FIO___IO_FLAG_OPEN | FIO___IO_FLAG_SUSPENDED;
  FIO_ASSERT(fio_io_is_open(&io), "suspended open IO must report open");
  const uint32_t closing[] = {FIO___IO_FLAG_CLOSE,
                              FIO___IO_FLAG_CLOSE_REMOTE,
                              FIO___IO_FLAG_CLOSE_ERROR};
  for (size_t i = 0; i < sizeof(closing) / sizeof(closing[0]); ++i) {
    io.flags = FIO___IO_FLAG_OPEN | closing[i];
    FIO_ASSERT(!fio_io_is_open(&io),
               "IO with closing flag %u must not report open",
               closing[i]);
  }
}

/* *****************************************************************************
Main entry point
***************************************************************************** */
int main(void) {
  fprintf(stderr, "=== IO API / types / reactor tests ===\n");

  test_io_reactor_state();
  test_io_open_predicate();
  test_io_close_write_order();
  test_io_noop_and_protocol_set_init();
  test_io_defer();
  test_io_protocol_each();
  test_io_env_global();
  test_io_tls_helpers();
  test_io_default_functions();
  test_io_connect_invalid_host();

  test_io_shared_protocol_listen_connect();
  test_io_protocol_set_null_lifecycle();
  test_io_integration();

  fprintf(stderr, "=== IO tests passed ===\n");
  return 0;
}
