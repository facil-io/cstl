/* Live, in-process WebSocket read/write integration test. Only the listener
 * socket discovery uses an internal type (no public fd getter exists).
 * Run with: make stress/websocket-roundtrip.
 * A reactor timer bounds stalls; failures stop the reactor immediately.
 */
#define FIO_IO
#define FIO_HTTP
#include "../tests/test-helpers.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define WS_RT_LARGE (FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT + 257U)
#define WS_RT_TIMEOUT_MS 5000

static unsigned char ws_rt_large[WS_RT_LARGE];
static const char ws_rt_small[] = "\xffws-small\0binary";
static struct {
  unsigned server_open, client_open, server_messages, client_messages;
  unsigned failures, timed_out, active;
} ws_rt;

static void ws_rt_fail(const char *where, const char *why, size_t got,
                       size_t want) {
  fprintf(stderr, "websocket-roundtrip: %s: %s (got %zu, expected %zu)\n",
          where, why, got, want);
  __atomic_fetch_add(&ws_rt.failures, 1U, __ATOMIC_RELAXED);
  fio_io_stop();
}

static int ws_rt_check(fio_buf_info_s msg, uint8_t is_text,
                       const void *expected, size_t len, const char *where) {
  if (msg.len != len || (len && memcmp(msg.buf, expected, len))) {
    size_t first = 0;
    while (first < len && first < msg.len &&
           ((const unsigned char *)expected)[first] ==
               ((const unsigned char *)msg.buf)[first]) ++first;
    fprintf(stderr, "websocket-roundtrip: %s: first differing byte %zu (wire decoded %02x expected %02x)\n",
            where, first,
            first < msg.len ? (unsigned)(unsigned char)msg.buf[first] : 0U,
            first < len ? (unsigned)((const unsigned char *)expected)[first] : 0U);
    ws_rt_fail(where, "payload mismatch", msg.len, len);
    return -1;
  }
  if (is_text) {
    ws_rt_fail(where, "binary message expected", is_text, 0);
    return -1;
  }
  return 0;
}

static void ws_rt_server_http(fio_http_s *h) {
  if (!fio_http_websocket_requested(h)) {
    ws_rt_fail("server", "missing WebSocket upgrade", 0, 1);
    fio_http_status_set(h, 400);
    fio_http_finish(h);
    return;
  }
  fio_http_upgrade_websocket(h);
}

static void ws_rt_server_open(fio_http_s *h) {
  ++ws_rt.server_open;
  if (fio_http_websocket_write(h, ws_rt_small, sizeof(ws_rt_small), 0) ||
      fio_http_websocket_write(h, ws_rt_large, sizeof(ws_rt_large), 0)) {
    ws_rt_fail("server on_open", "write failed", 1, 0);
    return;
  }
  fio_http_write(h, .buf = ws_rt_small, .len = sizeof(ws_rt_small), .copy = 1);
  fio_http_write(h, .buf = ws_rt_large, .len = sizeof(ws_rt_large), .copy = 1);
}

static void ws_rt_client_open(fio_http_s *h) {
  (void)h;
  ++ws_rt.client_open;
}

/* Acknowledgements enforce order, avoiding races between separate writers. */
static void ws_rt_server_message(fio_http_s *h, fio_buf_info_s msg,
                                 uint8_t is_text) {
  unsigned phase = ws_rt.server_messages++;
  const void *expected = (phase == 0 || phase == 2) ?
      (const void *)ws_rt_small : (const void *)ws_rt_large;
  size_t len = (phase == 0 || phase == 2) ? sizeof(ws_rt_small) : sizeof(ws_rt_large);
  if (phase >= 6) {
    ws_rt_fail("server receive", "unexpected message", phase, 6);
    return;
  }
  if (ws_rt_check(msg, is_text, expected, len, "server receive"))
    return;
  char ack = (char)('0' + phase);
  if (fio_http_websocket_write(h, &ack, 1, 0))
    ws_rt_fail("server acknowledgement", "write failed", 1, 0);
}

static void ws_rt_client_message(fio_http_s *h, fio_buf_info_s msg,
                                 uint8_t is_text) {
  unsigned phase = ws_rt.client_messages++;
  if (phase < 4) {
    const void *expected = (phase == 0 || phase == 2) ?
        (const void *)ws_rt_small : (const void *)ws_rt_large;
    size_t len = (phase == 0 || phase == 2) ? sizeof(ws_rt_small) : sizeof(ws_rt_large);
    if (ws_rt_check(msg, is_text, expected, len, "client receive"))
      return;
    if (phase == 3 && fio_http_websocket_write(h, ws_rt_small,
                                                sizeof(ws_rt_small), 0))
      ws_rt_fail("client small", "write failed", 1, 0);
    return;
  }
  if (phase >= 10) {
    ws_rt_fail("client", "unexpected message", phase, 10);
    return;
  }
  char ack = (char)('0' + phase - 4);
  if (ws_rt_check(msg, is_text, &ack, 1, "client acknowledgement"))
    return;
  if (phase == 4) {
    if (fio_http_websocket_write(h, ws_rt_large, sizeof(ws_rt_large), 0))
      ws_rt_fail("client large websocket_write", "write failed", 1, 0);
  } else if (phase == 5) {
    fio_http_write(h, .buf = ws_rt_small, .len = sizeof(ws_rt_small), .copy = 1);
  } else if (phase == 6) {
    /* The large generic-write branch must frame and mask client bytes. */
    fio_http_write(h, .buf = ws_rt_large, .len = sizeof(ws_rt_large), .copy = 1);
  } else if (phase == 7 || phase == 8) {
    /* Exercise both fd ownership modes and an offset into the file. */
    FILE *file = tmpfile();
    if (!file) {
      ws_rt_fail("client fd", "tmpfile failed", 0, 1);
      return;
    }
    int fd = dup(fileno(file));
    static const char prefix[] = "skip this prefix";
    if (fd < 0 || fwrite(prefix, 1, sizeof(prefix), file) != sizeof(prefix) ||
        fwrite(ws_rt_large, 1, sizeof(ws_rt_large), file) !=
                      sizeof(ws_rt_large) || fflush(file)) {
      if (fd >= 0) close(fd);
      fclose(file);
      ws_rt_fail("client fd", "file preparation failed", 0, 1);
      return;
    }
    fclose(file);
    fio_http_write(h, .fd = fd, .offset = sizeof(prefix),
                   .len = sizeof(ws_rt_large), .copy = (phase == 8));
    /* HTTP generic writes always close the supplied descriptor, even when
     * copy=1. Its payload has already been copied into the masked frame. */
    if (fcntl(fd, F_GETFD) != -1 || errno != EBADF)
      ws_rt_fail("client fd", "descriptor not closed", fd, 0);
  } else {
    /* A short fd read must never enqueue a partial frame, and must release
     * the descriptor before returning. */
    FILE *file = tmpfile();
    if (!file) {
      ws_rt_fail("client short fd", "tmpfile failed", 0, 1);
      return;
    }
    int fd = dup(fileno(file));
    fclose(file);
    if (fd < 0) {
      ws_rt_fail("client short fd", "dup failed", 0, 1);
      return;
    }
    fio_http_write(h, .fd = fd, .len = sizeof(ws_rt_large));
    if (fcntl(fd, F_GETFD) != -1 || errno != EBADF)
      ws_rt_fail("client short fd", "descriptor not closed", fd, 0);
    else
      fio_io_stop();
  }
}

static void ws_rt_client_http(fio_http_s *h) {
  fprintf(stderr, "ws client HTTP status %zu\n", (size_t)fio_http_status(h));
  ws_rt_fail("client HTTP", "upgrade rejected", fio_http_status(h), 101);
}
static void ws_rt_closed(fio_http_s *h) {
  (void)h;
  /* Expected only after all acknowledgements. A protocol-error close
   * before then is evidence, not a successful end to the roundtrip. */
  if (ws_rt.client_messages < 10 &&
      !__atomic_load_n(&ws_rt.failures, __ATOMIC_RELAXED))
    ws_rt_fail("WebSocket closed", "before all responses", ws_rt.client_messages, 10);
}
static int ws_rt_timeout(void *a, void *b) {
  (void)a; (void)b;
  if (!__atomic_load_n(&ws_rt.active, __ATOMIC_RELAXED) ||
      __atomic_load_n(&ws_rt.failures, __ATOMIC_RELAXED)) return -1;
  ws_rt.timed_out = 1;
  fio_io_stop();
  return -1;
}

int main(void) {
  for (size_t i = 0; i < sizeof(ws_rt_large); ++i)
    ws_rt_large[i] = (unsigned char)((i * 73U + (i >> 5) * 19U) & 255U);
  fio_http_listener_s *listener = fio_http_listen("tcp://127.0.0.1:0",
      .on_http = ws_rt_server_http,
      .on_authenticate_websocket = FIO_HTTP_AUTHENTICATE_ALLOW,
      .on_open = ws_rt_server_open,
      .on_message = ws_rt_server_message, .on_close = ws_rt_closed, .compress_ws = 0);
  if (!listener) {
    fprintf(stderr, "websocket-roundtrip: listener failed\n");
    return 1;
  }
  /* Public listener API does not expose the actual bound port for :0.
   * Pick it from the listener socket as in tests/http.c. */
  fio___io_listen_s *internal_listener = (fio___io_listen_s *)listener;
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  if (getsockname(internal_listener->fd, (struct sockaddr *)&addr, &addr_len)) {
    fprintf(stderr, "websocket-roundtrip: getsockname failed\n");
    fio_io_listen_stop((fio_io_listener_s *)listener);
    return 1;
  }
  char url[96];
  snprintf(url, sizeof(url), "ws://127.0.0.1:%u/roundtrip", ntohs(addr.sin_port));
  fio_io_s *client = fio_http_websocket_connect(url, NULL,
      .on_http = ws_rt_client_http, .on_open = ws_rt_client_open, .on_close = ws_rt_closed, .on_message = ws_rt_client_message,
      .compress_ws = 0);
  if (!client) {
    fprintf(stderr, "websocket-roundtrip: client connect failed\n");
    fio_io_listen_stop((fio_io_listener_s *)listener);
    return 1;
  }
  fio_io_run_every(.fn = ws_rt_timeout, .every = WS_RT_TIMEOUT_MS,
                   .repetitions = 1);
  __atomic_store_n(&ws_rt.active, 1U, __ATOMIC_RELAXED);
  fio_io_start(0);
  __atomic_store_n(&ws_rt.active, 0U, __ATOMIC_RELAXED);
  fio_io_listen_stop((fio_io_listener_s *)listener);
  fprintf(stderr, "websocket-roundtrip: opens %u/%u, messages server=%u client=%u, failures=%u, timeout=%u\n",
          ws_rt.server_open, ws_rt.client_open, ws_rt.server_messages,
          ws_rt.client_messages, ws_rt.failures, ws_rt.timed_out);
  return __atomic_load_n(&ws_rt.failures, __ATOMIC_RELAXED) ||
         ws_rt.timed_out || ws_rt.server_open != 1 ||
         ws_rt.client_open != 1 || ws_rt.server_messages != 6 ||
         ws_rt.client_messages != 10;
}
