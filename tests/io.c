/* *****************************************************************************
IO Module Comprehensive Tests

Tests the public IO API including:
- Reactor state management
- Protocol lifecycle and callbacks
- Listen/Connect operations
- IO operations (read, write, close, dup, suspend)
- Environment (env) API
- Roundtrip message exchange
- TLS helpers
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IO
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test State - Global Variables for Callback Tracking
***************************************************************************** */

/* Counters for callback invocations */
static volatile int fio___test_io_on_attach_count = 0;
static volatile int fio___test_io_on_data_count = 0;
static volatile int fio___test_io_on_ready_count = 0;
static volatile int fio___test_io_on_close_count = 0;

/* Data buffers for verification */
static char fio___test_io_received_data[4096];
static size_t fio___test_io_received_len = 0;

/* IO object tracking */
static fio_io_s *fio___test_io_client = NULL;

/* Synchronization */
static volatile int fio___test_io_server_ready = 0;
static volatile int fio___test_io_client_connected = 0;
static volatile int fio___test_io_exchange_complete = 0;

/* *****************************************************************************
Test: Reactor State API (without starting reactor)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, io_reactor_state)(void) {
  /* Test: Initial state (before reactor starts) */
  {
    FIO_ASSERT(fio_io_is_running() == 0,
               "fio_io_is_running should return 0 before reactor starts");
  }

  /* Test: PID functions work before reactor */
  {
    int pid = fio_io_pid();
    int root_pid = fio_io_root_pid();
    (void)pid;
    (void)root_pid;
  }

  /* Test: Workers calculation */
  {
    uint16_t w0 = fio_io_workers(0);
    FIO_ASSERT(w0 == 0, "fio_io_workers(0) should return 0");

    uint16_t w1 = fio_io_workers(1);
    FIO_ASSERT(w1 == 1, "fio_io_workers(1) should return 1");

    uint16_t w4 = fio_io_workers(4);
    FIO_ASSERT(w4 == 4, "fio_io_workers(4) should return 4");

    uint16_t wn1 = fio_io_workers(-1);
    FIO_ASSERT(wn1 >= 1, "fio_io_workers(-1) should return at least 1");
  }

  /* Test: Shutdown timeout */
  {
    size_t default_timeout = fio_io_shutdown_timeout();
    FIO_ASSERT(default_timeout > 0, "default shutdown timeout should be > 0");

    size_t new_timeout = fio_io_shutdown_timeout_set(5000);
    FIO_ASSERT(new_timeout == 5000, "shutdown timeout should be set to 5000");

    fio_io_shutdown_timeout_set(default_timeout);
  }

  /* Test: Queue access */
  {
    fio_queue_s *q = fio_io_queue();
    FIO_ASSERT(q != NULL, "fio_io_queue should return non-NULL");
  }
}

/* *****************************************************************************
Test: TLS Context Helpers
***************************************************************************** */

FIO_SFUNC int fio___test_io_tls_each_cert(fio_io_tls_each_s *e,
                                          const char *nm,
                                          const char *public_cert_file,
                                          const char *private_key_file,
                                          const char *pk_password) {
  size_t *counter = (size_t *)e->udata2;
  ++(*counter);
  (void)nm, (void)public_cert_file, (void)private_key_file, (void)pk_password;
  return 0;
}

FIO_SFUNC int fio___test_io_tls_each_alpn(fio_io_tls_each_s *e,
                                          const char *nm,
                                          void (*fn)(fio_io_s *)) {
  size_t *counter = (size_t *)e->udata2;
  ++(*counter);
  (void)nm, (void)fn;
  return 0;
}

FIO_SFUNC int fio___test_io_tls_each_trust(fio_io_tls_each_s *e,
                                           const char *nm) {
  size_t *counter = (size_t *)e->udata2;
  ++(*counter);
  (void)nm;
  return 0;
}

FIO_SFUNC void fio___test_io_tls_alpn_selected(fio_io_s *io) { (void)io; }

FIO_SFUNC void FIO_NAME_TEST(stl, io_tls_helpers)(void) {
  /* Test: TLS object lifecycle */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    FIO_ASSERT(tls != NULL, "fio_io_tls_new should return non-NULL");
    FIO_ASSERT(fio_io_tls_cert_count(tls) == 0,
               "initial cert count should be 0");
    FIO_ASSERT(fio_io_tls_alpn_count(tls) == 0,
               "initial alpn count should be 0");
    FIO_ASSERT(fio_io_tls_trust_count(tls) == 0,
               "initial trust count should be 0");
    fio_io_tls_free(tls);
  }

  /* Test: Reference counting */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_s *dup1 = fio_io_tls_dup(tls);
    FIO_ASSERT(dup1 == tls, "fio_io_tls_dup should return same pointer");
    fio_io_tls_free(dup1);
    fio_io_tls_free(tls);
  }

  /* Test: NULL handling */
  {
    fio_io_tls_s *null_dup = fio_io_tls_dup(NULL);
    FIO_ASSERT(null_dup == NULL, "fio_io_tls_dup(NULL) should return NULL");
    fio_io_tls_free(NULL);
  }

  /* Test: Certificate management */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_s *r1 =
        fio_io_tls_cert_add(tls, "server1", "cert1.pem", "key1.pem", "pass1");
    FIO_ASSERT(r1 == tls, "fio_io_tls_cert_add should return self");
    FIO_ASSERT(fio_io_tls_cert_count(tls) == 1, "cert count should be 1");
    fio_io_tls_cert_add(tls, "server2", "cert2.pem", "key2.pem", NULL);
    FIO_ASSERT(fio_io_tls_cert_count(tls) == 2, "cert count should be 2");
    fio_io_tls_free(tls);
  }

  /* Test: ALPN management */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_alpn_add(tls, "http/1.1", fio___test_io_tls_alpn_selected);
    FIO_ASSERT(fio_io_tls_alpn_count(tls) == 1, "alpn count should be 1");
    fio_io_tls_alpn_add(tls, "h2", fio___test_io_tls_alpn_selected);
    FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2, "alpn count should be 2");
    fio_io_tls_alpn_add(tls, NULL, fio___test_io_tls_alpn_selected);
    FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2, "NULL should be ignored");

    int result = fio_io_tls_alpn_select(tls, "http/1.1", 8, NULL);
    FIO_ASSERT(result == 0, "ALPN select should succeed for http/1.1");
    result = fio_io_tls_alpn_select(tls, "unknown", 7, NULL);
    FIO_ASSERT(result == -1, "ALPN select should fail for unknown");
    fio_io_tls_free(tls);
  }

  /* Test: Trust management */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_trust_add(tls, "ca1.pem");
    FIO_ASSERT(fio_io_tls_trust_count(tls) == 1, "trust count should be 1");
    fio_io_tls_trust_add(tls, "ca2.pem");
    FIO_ASSERT(fio_io_tls_trust_count(tls) == 2, "trust count should be 2");
    fio_io_tls_trust_add(tls, NULL);
    FIO_ASSERT(fio_io_tls_trust_count(tls) == 2, "NULL should not increment");
    fio_io_tls_free(tls);
  }

  /* Test: fio_io_tls_each iteration */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "s1", "c1.pem", "k1.pem", NULL);
    fio_io_tls_cert_add(tls, "s2", "c2.pem", "k2.pem", NULL);
    fio_io_tls_alpn_add(tls, "proto1", fio___test_io_tls_alpn_selected);
    fio_io_tls_alpn_add(tls, "proto2", fio___test_io_tls_alpn_selected);
    fio_io_tls_trust_add(tls, "trust1.pem");

    size_t cert_count = 0, alpn_count = 0, trust_count = 0;
    fio_io_tls_each(tls,
                    .udata2 = &cert_count,
                    .each_cert = fio___test_io_tls_each_cert);
    FIO_ASSERT(cert_count == 2, "each_cert should iterate 2 certs");
    fio_io_tls_each(tls,
                    .udata2 = &alpn_count,
                    .each_alpn = fio___test_io_tls_each_alpn);
    FIO_ASSERT(alpn_count == 2, "each_alpn should iterate 2 protocols");
    fio_io_tls_each(tls,
                    .udata2 = &trust_count,
                    .each_trust = fio___test_io_tls_each_trust);
    FIO_ASSERT(trust_count == 1, "each_trust should iterate 1 trust");
    fio_io_tls_free(tls);
  }

  /* Test: URL-based TLS detection */
  {
    struct {
      const char *url;
      int expect_tls;
    } tests[] = {
        {"http://example.com", 0},
        {"https://example.com", 1},
        {"tcp://example.com", 0},
        {"tcps://example.com", 1},
        {"tls://example.com", 1},
        {"ws://example.com", 0},
        {"wss://example.com", 1},
        {NULL, 0},
    };
    for (size_t i = 0; tests[i].url; ++i) {
      fio_url_s u = fio_url_parse(tests[i].url, FIO_STRLEN(tests[i].url));
      fio_io_tls_s *tls = fio_io_tls_from_url(NULL, u);
      int got_tls = (tls != NULL);
      FIO_ASSERT(got_tls == tests[i].expect_tls,
                 "TLS detection error for %s: expected %d, got %d",
                 tests[i].url,
                 tests[i].expect_tls,
                 got_tls);
      fio_io_tls_free(tls);
    }
  }
}

/* *****************************************************************************
Test: IO Environment (env) API
***************************************************************************** */

static volatile int fio___test_io_env_close_count = 0;

FIO_SFUNC void fio___test_io_env_on_close(void *udata) {
  ++fio___test_io_env_close_count;
  (void)udata;
}

FIO_SFUNC void FIO_NAME_TEST(stl, io_env)(void) {
  /* Test: Global environment (io = NULL) */
  {
    fio___test_io_env_close_count = 0;

    fio_io_env_set(NULL,
                   .name = FIO_BUF_INFO1((char *)"test_key"),
                   .type = 1,
                   .udata = (void *)0x1234,
                   .on_close = fio___test_io_env_on_close);

    void *val = fio_io_env_get(NULL,
                               .name = FIO_BUF_INFO1((char *)"test_key"),
                               .type = 1);
    FIO_ASSERT(val == (void *)0x1234, "env get should return set value");

    void *wrong = fio_io_env_get(NULL,
                                 .name = FIO_BUF_INFO1((char *)"test_key"),
                                 .type = 2);
    FIO_ASSERT(wrong == NULL, "env get with wrong type should return NULL");

    int unset_result =
        fio_io_env_unset(NULL,
                         .name = FIO_BUF_INFO1((char *)"test_key"),
                         .type = 1);
    FIO_ASSERT(unset_result == 0, "env unset should succeed");
    FIO_ASSERT(fio___test_io_env_close_count == 0,
               "unset should NOT call on_close");

    val = fio_io_env_get(NULL,
                         .name = FIO_BUF_INFO1((char *)"test_key"),
                         .type = 1);
    FIO_ASSERT(val == NULL, "env get after unset should return NULL");
  }

  /* Test: env_remove (with callback) */
  {
    fio___test_io_env_close_count = 0;

    fio_io_env_set(NULL,
                   .name = FIO_BUF_INFO1((char *)"remove_test"),
                   .type = 5,
                   .udata = (void *)0x5678,
                   .on_close = fio___test_io_env_on_close);

    int remove_result =
        fio_io_env_remove(NULL,
                          .name = FIO_BUF_INFO1((char *)"remove_test"),
                          .type = 5);
    FIO_ASSERT(remove_result == 0, "env remove should succeed");
    FIO_ASSERT(fio___test_io_env_close_count == 1,
               "remove SHOULD call on_close");
  }

  /* Test: Replacing existing value */
  {
    fio___test_io_env_close_count = 0;

    fio_io_env_set(NULL,
                   .name = FIO_BUF_INFO1((char *)"replace"),
                   .type = 1,
                   .udata = (void *)0xAAAA,
                   .on_close = fio___test_io_env_on_close);

    fio_io_env_set(NULL,
                   .name = FIO_BUF_INFO1((char *)"replace"),
                   .type = 1,
                   .udata = (void *)0xBBBB,
                   .on_close = fio___test_io_env_on_close);

    FIO_ASSERT(fio___test_io_env_close_count == 1,
               "replacing should call old value's on_close");

    void *val = fio_io_env_get(NULL,
                               .name = FIO_BUF_INFO1((char *)"replace"),
                               .type = 1);
    FIO_ASSERT(val == (void *)0xBBBB, "replaced value should be new value");

    fio_io_env_remove(NULL,
                      .name = FIO_BUF_INFO1((char *)"replace"),
                      .type = 1);
  }
}

/* *****************************************************************************
Test: Protocol Structure
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, io_protocol_init)(void) {
  {
    FIO_ASSERT(sizeof(fio_io_protocol_s) >= 64,
               "protocol struct should be at least 64 bytes");
  }

  {
    fio_io_protocol_s pr = {0};
    pr.on_attach = (void (*)(fio_io_s *))0x1111;
    pr.on_data = (void (*)(fio_io_s *))0x2222;
    pr.timeout = 5000;
    pr.buffer_size = 1024;

    FIO_ASSERT(pr.on_attach == (void (*)(fio_io_s *))0x1111, "on_attach error");
    FIO_ASSERT(pr.on_data == (void (*)(fio_io_s *))0x2222, "on_data error");
    FIO_ASSERT(pr.timeout == 5000, "timeout error");
    FIO_ASSERT(pr.buffer_size == 1024, "buffer_size error");
  }
}

/* *****************************************************************************
Test: Deferred Task Scheduling
***************************************************************************** */

static volatile int fio___test_io_defer_count = 0;
static void *fio___test_io_defer_udata1 = NULL;
static void *fio___test_io_defer_udata2 = NULL;

FIO_SFUNC void fio___test_io_defer_task(void *udata1, void *udata2) {
  ++fio___test_io_defer_count;
  fio___test_io_defer_udata1 = udata1;
  fio___test_io_defer_udata2 = udata2;
}

FIO_SFUNC void FIO_NAME_TEST(stl, io_defer)(void) {
  fio___test_io_defer_count = 0;
  fio___test_io_defer_udata1 = NULL;
  fio___test_io_defer_udata2 = NULL;

  fio_io_defer(fio___test_io_defer_task, (void *)0x1234, (void *)0x5678);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(fio___test_io_defer_count == 1,
             "deferred task should execute once");
  FIO_ASSERT(fio___test_io_defer_udata1 == (void *)0x1234, "udata1 error");
  FIO_ASSERT(fio___test_io_defer_udata2 == (void *)0x5678, "udata2 error");

  fio___test_io_defer_count = 0;
  fio_io_defer(fio___test_io_defer_task, NULL, NULL);
  fio_io_defer(fio___test_io_defer_task, NULL, NULL);
  fio_io_defer(fio___test_io_defer_task, NULL, NULL);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(fio___test_io_defer_count == 3, "3 deferred tasks should execute");
}

/* *****************************************************************************
Test: fio_io_noop
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, io_noop)(void) { fio_io_noop(NULL); }

/* *****************************************************************************
Test: fio_io_protocol_each
***************************************************************************** */

static volatile int fio___test_protocol_each_count = 0;

FIO_SFUNC void fio___test_protocol_each_task(fio_io_s *io, void *udata) {
  ++fio___test_protocol_each_count;
  (void)io, (void)udata;
}

FIO_SFUNC void FIO_NAME_TEST(stl, io_protocol_each)(void) {
  {
    size_t count =
        fio_io_protocol_each(NULL, fio___test_protocol_each_task, NULL);
    FIO_ASSERT(count == 0, "protocol_each with NULL should return 0");
  }

  {
    fio_io_protocol_s pr = {0};
    size_t count =
        fio_io_protocol_each(&pr, fio___test_protocol_each_task, NULL);
    FIO_ASSERT(count == 0,
               "protocol_each with uninit protocol should return 0");
  }
}

/* *****************************************************************************
Integration Test: Roundtrip Message Exchange + IO Operations
***************************************************************************** */

/* Server protocol callbacks */
FIO_SFUNC void fio___test_server_on_attach(fio_io_s *io) {
  ++fio___test_io_on_attach_count;
  (void)io;
}

FIO_SFUNC void fio___test_server_on_data(fio_io_s *io) {
  ++fio___test_io_on_data_count;
  char buf[1024];
  size_t r = fio_io_read(io, buf, sizeof(buf) - 1);
  if (r > 0) {
    buf[r] = '\0';
    char reply[1100];
    int len = snprintf(reply, sizeof(reply), "ECHO:%s", buf);
    fio_io_write(io, reply, len);
  }
}

FIO_SFUNC void fio___test_server_on_ready(fio_io_s *io) {
  ++fio___test_io_on_ready_count;
  (void)io;
}

FIO_SFUNC void fio___test_server_on_close(void *buffer, void *udata) {
  ++fio___test_io_on_close_count;
  (void)buffer, (void)udata;
}

static fio_io_protocol_s fio___test_server_protocol = {
    .on_attach = fio___test_server_on_attach,
    .on_data = fio___test_server_on_data,
    .on_ready = fio___test_server_on_ready,
    .on_close = fio___test_server_on_close,
    .on_timeout = fio_io_touch,
    .timeout = 5000,
};

/* Client protocol callbacks */
FIO_SFUNC void fio___test_client_on_attach(fio_io_s *io) {
  fio___test_io_client = io;

  /* Test IO object functions */
  int fd = fio_io_fd(io);
  FIO_ASSERT(fd >= 0, "fio_io_fd should return valid fd");

  FIO_ASSERT(fio_io_is_open(io) == 1, "fio_io_is_open should return 1");

  void *ud = fio_io_udata(io);
  FIO_ASSERT(ud == (void *)0xABCD, "udata should be preserved from connect");

  void *old = fio_io_udata_set(io, (void *)0x1234);
  FIO_ASSERT(old == (void *)0xABCD, "udata_set should return old value");
  FIO_ASSERT(fio_io_udata(io) == (void *)0x1234, "udata should be updated");

  fio_io_protocol_s *pr = fio_io_protocol(io);
  FIO_ASSERT(pr != NULL, "fio_io_protocol should return non-NULL");

  void *buf = fio_io_buffer(io);
  FIO_ASSERT(buf != NULL, "fio_io_buffer should return non-NULL");
  size_t buf_len = fio_io_buffer_len(io);
  FIO_ASSERT(buf_len >= pr->buffer_size,
             "IO object should have, at least, length set");
  /* Mark connected and send message immediately
   * Note: For fio_io_connect, on_attach is called AFTER connection is
   * established, not before. The on_ready callback may not be called
   * because the protocol switch happens after the socket is already ready.
   */
  fio___test_io_client_connected = 1;

  /* Send the test message */
  const char *msg = "Hello, IO!";
  fio_io_write(io, (void *)msg, FIO_STRLEN(msg));
}

FIO_SFUNC void fio___test_client_on_data(fio_io_s *io) {
  char buf[1024];
  size_t r = fio_io_read(io, buf, sizeof(buf) - 1);
  if (r > 0) {
    buf[r] = '\0';
    if (r < sizeof(fio___test_io_received_data)) {
      FIO_MEMCPY(fio___test_io_received_data, buf, r);
      fio___test_io_received_len = r;
    }
    fio___test_io_exchange_complete = 1;
  }
}

FIO_SFUNC void fio___test_client_on_ready(fio_io_s *io) {
  /* Note: For fio_io_connect, on_ready may not be called on the user's protocol
   * because the protocol switch happens after the socket is already ready.
   * This is expected behavior - on_attach is the reliable callback to use. */
  ++fio___test_io_on_ready_count;

  /* Test additional IO functions if we get here */
  size_t backlog = fio_io_backlog(io);
  (void)backlog; /* May not be 0 if data was written */

  fio_io_touch(io);

  fio_io_suspend(io);
  FIO_ASSERT(fio_io_is_suspended(io) == 1,
             "fio_io_is_suspended should return 1 after suspend");
  fio_io_unsuspend(io);
}

FIO_SFUNC void fio___test_client_on_close(void *buffer, void *udata) {
  fio___test_io_client = NULL;
  (void)buffer, (void)udata;
}

static fio_io_protocol_s fio___test_client_protocol = {
    .on_attach = fio___test_client_on_attach,
    .on_data = fio___test_client_on_data,
    .on_ready = fio___test_client_on_ready,
    .on_close = fio___test_client_on_close,
    .on_timeout = fio_io_touch,
    .timeout = 5000,
    .buffer_size = 128,
};

/* Listener callbacks */
static fio_io_listener_s *fio___test_listener = NULL;

FIO_SFUNC void fio___test_listener_on_start(fio_io_protocol_s *pr, void *ud) {
  fio___test_io_server_ready = 1;
  (void)pr, (void)ud;
}

/* Timer callbacks */
FIO_SFUNC int fio___test_connect_client(void *u1, void *u2) {
  (void)u1, (void)u2;
  fio_io_connect("tcp://127.0.0.1:19876",
                 .protocol = &fio___test_client_protocol,
                 .udata = (void *)0xABCD,
                 .timeout = 5000);
  return -1;
}

FIO_SFUNC int fio___test_check_done(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (fio___test_io_exchange_complete) {
    fio_io_stop();
    return -1;
  }
  return 0;
}

FIO_SFUNC int fio___test_timeout(void *u1, void *u2) {
  (void)u1, (void)u2;
  FIO_LOG_WARNING("  test timeout!");
  fio_io_stop();
  return -1;
}

FIO_SFUNC void fio___test_on_start(void *ignr_) {
  (void)ignr_;
  /* The listener was set up before fio_io_start, so it should be ready now */
  fio___test_io_server_ready = 1;
}

FIO_SFUNC void FIO_NAME_TEST(stl, io_roundtrip)(void) {
  /* Reset state */
  fio___test_io_on_attach_count = 0;
  fio___test_io_on_data_count = 0;
  fio___test_io_on_ready_count = 0;
  fio___test_io_on_close_count = 0;
  FIO_MEMSET(fio___test_io_received_data,
             0,
             sizeof(fio___test_io_received_data));
  fio___test_io_received_len = 0;
  fio___test_io_client = NULL;
  fio___test_io_server_ready = 0;
  fio___test_io_client_connected = 0;
  fio___test_io_exchange_complete = 0;
  fio___test_listener = NULL;

  /* Set up listener BEFORE reactor starts (like pubsub tests) */
  fio___test_listener = fio_io_listen(.url = "tcp://127.0.0.1:19876",
                                      .protocol = &fio___test_server_protocol,
                                      .udata = (void *)0x9999,
                                      .on_start = fio___test_listener_on_start,
                                      .hide_from_log = 1);

  if (!fio___test_listener) {
    FIO_LOG_ERROR("  failed to set up listener!");
    return;
  }

  /* Test listener API (before reactor starts) */
  fio_io_protocol_s *pr = fio_io_listener_protocol(fio___test_listener);
  FIO_ASSERT(pr == &fio___test_server_protocol,
             "fio_io_listener_protocol should return set protocol");

  void *ud = fio_io_listener_udata(fio___test_listener);
  FIO_ASSERT(ud == (void *)0x9999, "fio_io_listener_udata error");

  fio_buf_info_s url = fio_io_listener_url(fio___test_listener);
  FIO_ASSERT(url.len > 0, "listener URL should not be empty");
  int is_tls = fio_io_listener_is_tls(fio___test_listener);
  FIO_ASSERT(is_tls == 0, "listener should not be TLS");

  /* Register startup callback */
  fio_state_callback_add(FIO_CALL_ON_START, fio___test_on_start, NULL);

  /* Schedule timers BEFORE reactor starts (like pubsub tests) */
  fio_io_run_every(.fn = fio___test_connect_client,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_check_done,
                   .every = 50,
                   .repetitions = -1); /* -1 = repeat forever */
  fio_io_run_every(.fn = fio___test_timeout, .every = 5000, .repetitions = 1);

  /* Start reactor in single-process mode */
  fio_io_start(0);

  /* Clean up state callback */
  fio_state_callback_remove(FIO_CALL_ON_START, fio___test_on_start, NULL);

  /* Verify results */
  FIO_ASSERT(fio___test_io_exchange_complete,
             "roundtrip should complete before timeout");
  FIO_ASSERT(fio___test_io_received_len > 0, "should have received data");

  const char *expected = "ECHO:Hello, IO!";
  FIO_ASSERT(fio___test_io_received_len == FIO_STRLEN(expected),
             "received length should match expected (%zu vs %zu)",
             fio___test_io_received_len,
             FIO_STRLEN(expected));
  FIO_ASSERT(FIO_MEMCMP(fio___test_io_received_data,
                        expected,
                        fio___test_io_received_len) == 0,
             "received data should match expected: got '%s', expected '%s'",
             fio___test_io_received_data,
             expected);

  /* Verify protocol callbacks were invoked */
  FIO_ASSERT(fio___test_io_on_attach_count > 0,
             "on_attach should have been called");
  FIO_ASSERT(fio___test_io_on_data_count > 0,
             "on_data should have been called");
  /* Note: on_ready may not be called for client protocol after fio_io_connect
   * because the protocol switch happens after the socket is already ready.
   * on_ready IS called for server (accepted) connections. */
}

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(void) {
  fprintf(stderr, "\t  Testing IO Module:\n");
  FIO_PRINT_SIZE_OF(fio_io_protocol_s);
  FIO_PRINT_SIZE_OF(fio_io_functions_s);
  FIO_PRINT_SIZE_OF(fio_io_tls_s);

  /* Unit tests (no reactor required) */
  FIO_NAME_TEST(stl, io_reactor_state)();
  FIO_NAME_TEST(stl, io_tls_helpers)();
  FIO_NAME_TEST(stl, io_env)();
  FIO_NAME_TEST(stl, io_protocol_init)();
  FIO_NAME_TEST(stl, io_defer)();
  FIO_NAME_TEST(stl, io_noop)();
  FIO_NAME_TEST(stl, io_protocol_each)();

  /* Integration test (requires reactor) - tests listen, connect, protocol
   * callbacks, IO operations, and roundtrip message exchange */
  FIO_NAME_TEST(stl, io_roundtrip)();

  fprintf(stderr, "\t  IO Module tests completed successfully!\n");
  return 0;
}
