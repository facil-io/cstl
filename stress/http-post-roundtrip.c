/* Live, in-process HTTP/1.1 POST roundtrip (client -> server -> client).
 *
 * Each case POSTs a deterministic payload with our client to our server.
 * Sizes straddle FIO_HTTP_BODY_RAM_LIMIT so both body storage backends are
 * exercised (RAM buffer and temporary file), on the client (request body
 * send), the server (request body receive) and the client again (response
 * body receive, the server echoes the payload).
 *
 * Every received body is verified with:
 * - sequential chunked reads (odd chunk size, position tracking, EOF);
 * - rewinds (absolute, negative-from-end, clamped seeks, re-reads);
 * - full memory dumps (`fio_http_body_read(h, SIZE_MAX)`), from the start
 *   and from a mid-body position (remainder dump).
 *
 * Run with: make stress/http-post-roundtrip
 * A reactor timer bounds stalls. Only listener port discovery uses an
 * internal type (no public bound-port getter exists).
 */
#define FIO_IO
#define FIO_HTTP
#include "../tests/test-helpers.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define POST_RT_TIMEOUT_MS 10000
#define POST_RT_CHUNK      4093 /* odd: never aligned with storage sizes */
#define POST_RT_MAX        ((FIO_HTTP_BODY_RAM_LIMIT << 1) + 12345)

static char post_rt_payload[POST_RT_MAX];

static struct {
  size_t len;
  const char *name;
  unsigned server_calls, client_calls, client_status;
} post_rt_cases[] = {
    {1500, "small (RAM)"},
    {FIO_HTTP_BODY_RAM_LIMIT, "at RAM limit (RAM)"},
    {FIO_HTTP_BODY_RAM_LIMIT + 1, "RAM limit + 1 (temp file)"},
    {POST_RT_MAX, "large (temp file)"},
};
#define POST_RT_CASES (sizeof(post_rt_cases) / sizeof(post_rt_cases[0]))

static struct {
  unsigned failures, timed_out, active, done;
} post_rt;

#define POST_RT_FAIL(fmt, ...)                                                 \
  do {                                                                         \
    fprintf(stderr, "http-post-roundtrip: FAIL " fmt "\n", __VA_ARGS__);      \
    ++post_rt.failures;                                                        \
  } while (0)

/** Verifies a read result against the payload at `offset`. */
static int post_rt_expect(const char *ctx,
                          const char *step,
                          fio_str_info_s r,
                          size_t offset,
                          size_t want_len) {
  if (r.len != want_len) {
    POST_RT_FAIL("%s: %s: read length %zu, expected %zu (offset %zu)",
                 ctx,
                 step,
                 r.len,
                 want_len,
                 offset);
    return -1;
  }
  if (want_len && (!r.buf || memcmp(r.buf, post_rt_payload + offset, r.len))) {
    size_t i = 0;
    while (r.buf && i < r.len && r.buf[i] == post_rt_payload[offset + i])
      ++i;
    POST_RT_FAIL("%s: %s: content mismatch at body offset %zu",
                 ctx,
                 step,
                 offset + i);
    return -1;
  }
  return 0;
}

static int post_rt_expect_pos(const char *ctx,
                              const char *step,
                              fio_http_s *h,
                              size_t want) {
  size_t pos = fio_http_body_pos(h);
  if (pos == want)
    return 0;
  POST_RT_FAIL("%s: %s: position %zu, expected %zu", ctx, step, pos, want);
  return -1;
}

/** Runs all body read checks. Returns the number of failures found. */
static unsigned post_rt_check_body(fio_http_s *h, const char *ctx, size_t len) {
  unsigned before = post_rt.failures;
  int want_fd = len > FIO_HTTP_BODY_RAM_LIMIT;

  if (fio_http_body_length(h) != len) {
    POST_RT_FAIL("%s: body length %zu, expected %zu",
                 ctx,
                 fio_http_body_length(h),
                 len);
    return post_rt.failures - before; /* nothing else is meaningful */
  }
  if ((fio_http_body_fd(h) != -1) != want_fd)
    POST_RT_FAIL("%s: storage is %s, expected %s",
                 ctx,
                 fio_http_body_fd(h) == -1 ? "RAM" : "temp file",
                 want_fd ? "temp file" : "RAM");

  /* 1. Sequential chunked reads */
  {
    size_t off = 0;
    fio_http_body_seek(h, 0);
    for (;;) {
      size_t want = len - off < POST_RT_CHUNK ? len - off : POST_RT_CHUNK;
      fio_str_info_s r = fio_http_body_read(h, POST_RT_CHUNK);
      if (post_rt_expect(ctx, "sequential read", r, off, want))
        break;
      off += r.len;
      if (post_rt_expect_pos(ctx, "sequential read", h, off) || !r.len)
        break;
    }
    if (off != len)
      POST_RT_FAIL("%s: sequential reads covered %zu of %zu", ctx, off, len);
  }

  /* 2. Rewinds */
  {
    size_t mid = len / 2;
    fio_http_body_seek(h, 0);
    post_rt_expect(ctx, "rewind to 0", fio_http_body_read(h, 17), 0, 17);
    post_rt_expect_pos(ctx, "rewind to 0", h, 17);
    fio_http_body_seek(h, (ssize_t)mid);
    post_rt_expect(ctx, "seek to middle", fio_http_body_read(h, 33), mid, 33);
    fio_http_body_seek(h, -25);
    post_rt_expect(ctx,
                   "negative seek",
                   fio_http_body_read(h, 25),
                   len - 25,
                   25);
    post_rt_expect(ctx, "read at EOF", fio_http_body_read(h, 10), len, 0);
    if (fio_http_body_seek(h, -(ssize_t)(len + 10)) != 0)
      POST_RT_FAIL("%s: negative seek past start must clamp to 0", ctx);
    if (fio_http_body_seek(h, (ssize_t)(len + 10)) != len)
      POST_RT_FAIL("%s: seek past end must clamp to length", ctx);
    if (fio_http_body_seek(h, SSIZE_MAX) != len)
      POST_RT_FAIL("%s: seek(SSIZE_MAX) must report the position", ctx);
    /* rewind after a partial read, then re-read the same bytes */
    fio_http_body_seek(h, (ssize_t)mid);
    fio_http_body_read(h, 100);
    fio_http_body_seek(h, (ssize_t)mid);
    post_rt_expect(ctx, "re-read after rewind", fio_http_body_read(h, 100),
                   mid, 100);
  }

  /* 3. Full memory dumps */
  {
    size_t third = len / 3;
    fio_http_body_seek(h, 0);
    post_rt_expect(ctx,
                   "full dump from start",
                   fio_http_body_read(h, (size_t)-1),
                   0,
                   len);
    post_rt_expect_pos(ctx, "full dump from start", h, len);
    fio_http_body_seek(h, (ssize_t)third);
    post_rt_expect(ctx,
                   "remainder dump from len/3",
                   fio_http_body_read(h, (size_t)-1),
                   third,
                   len - third);
    post_rt_expect_pos(ctx, "remainder dump from len/3", h, len);
  }
  return post_rt.failures - before;
}

static size_t post_rt_case_from_path(fio_http_s *h) {
  fio_str_info_s path = fio_http_path(h);
  if (path.len != 7 || memcmp(path.buf, "/post/", 6) ||
      (size_t)(path.buf[6] - '0') >= POST_RT_CASES)
    return (size_t)-1;
  return (size_t)(path.buf[6] - '0');
}

static void post_rt_server_on_http(fio_http_s *h) {
  size_t i = post_rt_case_from_path(h);
  if (i == (size_t)-1 || !fio_http_method_is_post(h)) {
    POST_RT_FAIL("server: unexpected request %s %s",
                 fio_http_method(h).buf,
                 fio_http_path(h).buf);
    fio_http_status_set(h, 400);
    fio_http_finish(h);
    return;
  }
  ++post_rt_cases[i].server_calls;
  char ctx[96];
  snprintf(ctx, sizeof(ctx), "server request [%s]", post_rt_cases[i].name);
  if (post_rt_check_body(h, ctx, post_rt_cases[i].len)) {
    fio_http_status_set(h, 500);
    fio_http_finish(h);
    return;
  }
  /* echo the expected payload (static memory, no copy needed) */
  fio_http_write(h,
                 .buf = post_rt_payload,
                 .len = post_rt_cases[i].len,
                 .finish = 1);
}

static void post_rt_client_on_http(fio_http_s *h) {
  size_t i = (size_t)(uintptr_t)fio_http_udata(h);
  if (i >= POST_RT_CASES) {
    POST_RT_FAIL("client: unknown case index %zu", i);
    fio_io_stop();
    return;
  }
  ++post_rt_cases[i].client_calls;
  post_rt_cases[i].client_status = (unsigned)fio_http_status(h);
  if (fio_http_status(h) == 200) {
    char ctx[96];
    snprintf(ctx, sizeof(ctx), "client response [%s]", post_rt_cases[i].name);
    post_rt_check_body(h, ctx, post_rt_cases[i].len);
  } else
    POST_RT_FAIL("client [%s]: status %zu, expected 200",
                 post_rt_cases[i].name,
                 (size_t)fio_http_status(h));
  if (++post_rt.done == POST_RT_CASES)
    fio_io_stop();
}

static int post_rt_timeout(void *a, void *b) {
  (void)a, (void)b;
  if (!post_rt.active)
    return -1;
  post_rt.timed_out = 1;
  fio_io_stop();
  return -1;
}

int main(void) {
  for (size_t i = 0; i < POST_RT_MAX; ++i) /* position dependent bytes */
    post_rt_payload[i] = (char)((i * 131U + (i >> 8) * 7U + (i >> 16)) & 255U);

  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0", .on_http = post_rt_server_on_http);
  if (!l) {
    fprintf(stderr, "http-post-roundtrip: listener failed\n");
    return 1;
  }
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  if (getsockname(((fio___io_listen_s *)l)->fd,
                  (struct sockaddr *)&addr,
                  &addr_len)) {
    fprintf(stderr, "http-post-roundtrip: getsockname failed\n");
    fio_io_listen_stop((fio_io_listener_s *)l);
    return 1;
  }

  for (size_t i = 0; i < POST_RT_CASES; ++i) {
    char url[96];
    snprintf(url,
             sizeof(url),
             "http://127.0.0.1:%u/post/%zu",
             (unsigned)ntohs(addr.sin_port),
             i);
    fio_http_s *h = fio_http_new();
    fio_http_method_set(h, FIO_STR_INFO2((char *)"POST", 4));
    fio_http_udata_set(h, (void *)(uintptr_t)i);
    fio_http_body_write(h, post_rt_payload, post_rt_cases[i].len);
    /* client side storage must match too (exercises both send paths) */
    char ctx[96];
    snprintf(ctx, sizeof(ctx), "client request [%s]", post_rt_cases[i].name);
    post_rt_check_body(h, ctx, post_rt_cases[i].len);
    if (!fio_http_connect(url,
                          h,
                          .on_http = post_rt_client_on_http,
                          .udata = (void *)(uintptr_t)i)) {
      fprintf(stderr, "http-post-roundtrip: connect failed (%s)\n", url);
      fio_io_listen_stop((fio_io_listener_s *)l);
      return 1;
    }
  }

  fio_io_run_every(.fn = post_rt_timeout,
                   .every = POST_RT_TIMEOUT_MS,
                   .repetitions = 1);
  post_rt.active = 1;
  fio_io_start(0);
  post_rt.active = 0;
  fio_io_listen_stop((fio_io_listener_s *)l);

  int bad = post_rt.failures || post_rt.timed_out;
  for (size_t i = 0; i < POST_RT_CASES; ++i) {
    fprintf(stderr,
            "http-post-roundtrip: %-26s %7zu bytes: server=%u client=%u "
            "status=%u\n",
            post_rt_cases[i].name,
            post_rt_cases[i].len,
            post_rt_cases[i].server_calls,
            post_rt_cases[i].client_calls,
            post_rt_cases[i].client_status);
    bad |= post_rt_cases[i].server_calls != 1 ||
           post_rt_cases[i].client_calls != 1 ||
           post_rt_cases[i].client_status != 200;
  }
  fprintf(stderr,
          "http-post-roundtrip: failures=%u timeout=%u -> %s\n",
          post_rt.failures,
          post_rt.timed_out,
          bad ? "FAILED" : "PASSED");
  return bad;
}
