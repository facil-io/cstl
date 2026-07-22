/* *****************************************************************************
Test - IO API / types / reactor integration (400-402)

Correctness coverage for fio_io reactor state, IO object lifecycle,
protocol callbacks, fd attachment, task/timer scheduling, env helpers,
protocol iteration, TLS context helpers, and listen/connect roundtrip.

Uses in-process socketpairs / loopback only.  No external processes,
network calls, or benchmarking.
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

static fio_socket_i fio___test_io_pair_local = FIO_SOCKET_INVALID;
static fio_io_s *fio___test_io_pair_io = NULL;
static fio_io_s *fio___test_io_listen_io = NULL;
static fio_io_listener_s *fio___test_io_listener = NULL;

static volatile int fio___test_io_runner_step = 0;
static volatile int fio___test_io_pair_done = 0;
static volatile int fio___test_io_listen_done = 0;
static volatile int fio___test_io_timer_done = 0;
static volatile int fio___test_io_timeout_fired = 0;

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
  (void)buffer, (void)udata;
}

static void fio___test_io_client_on_failed(fio_io_protocol_s *pr, void *ud) {
  (void)pr, (void)ud;
  FIO_ASSERT(0, "client connection failed");
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

    /* test close schedules graceful close */
    fio_io_close(fio___test_io_pair_io);
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
  fio___test_io_listen_io = io;
}

static int fio___test_io_check_done_task(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (fio___test_io_pair_done && fio___test_io_listen_done &&
      fio___test_io_timer_done) {
    fio_io_stop();
    return -1;
  }
  return 0;
}

static int fio___test_io_timeout_cb(void *u1, void *u2) {
  (void)u1, (void)u2;
  fio___test_io_timeout_fired = 1;
  FIO_LOG_ERROR(
      "io integration timeout (pair_done=%d listen_done=%d timer_done=%d)",
      fio___test_io_pair_done,
      fio___test_io_listen_done,
      fio___test_io_timer_done);
  fio_io_stop();
  return -1;
}

static void fio___test_io_on_start(void *ignr_) {
  (void)ignr_;
  /* Schedule driver tasks and timers. */
  fio_io_defer(fio___test_io_attach_pair_task, NULL, NULL);
  fio_io_defer(fio___test_io_start_listen_task, NULL, NULL);

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
                   .every = 3000,
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
  fio___test_io_pair_received_len = 0;
  fio___test_io_listen_received_len = 0;
  fio___test_io_pair_local = FIO_SOCKET_INVALID;
  fio___test_io_pair_io = NULL;
  fio___test_io_listen_io = NULL;
  fio___test_io_listener = NULL;
  fio___test_io_runner_step = 0;
  fio___test_io_pair_done = 0;
  fio___test_io_listen_done = 0;
  fio___test_io_timer_done = 0;
  fio___test_io_timeout_fired = 0;
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

  /* Cleanup the local end of the socketpair if it is still open. */
  if (FIO_SOCK_FD_ISVALID(fio___test_io_pair_local)) {
    fio_sock_close(fio___test_io_pair_local);
    fio___test_io_pair_local = FIO_SOCKET_INVALID;
  }

  fprintf(stderr, "* IO reactor integration: OK\n");
#endif
}

/* *****************************************************************************
Main entry point
***************************************************************************** */
int main(void) {
  fprintf(stderr, "=== IO API / types / reactor tests ===\n");

  test_io_reactor_state();
  test_io_noop_and_protocol_set_init();
  test_io_defer();
  test_io_protocol_each();
  test_io_env_global();
  test_io_tls_helpers();
  test_io_default_functions();

  test_io_integration();

  fprintf(stderr, "=== IO tests passed ===\n");
  return 0;
}
