/* *****************************************************************************
Test: HTTP/1.x parser - fragmented input (no network)

Every message is parsed once as a whole (the reference), then re-parsed with
the stream cut at: every single position, every pair of positions, and every
fixed fragment size (1 byte at a time up to the whole message).

Fragments are fed the way the HTTP/1 connection (`434 http1.h`) feeds them:
unconsumed bytes are kept, the next fragment is appended, and the parser is
called until it consumes nothing. Each run must produce a callback transcript
identical to the reference (body bytes are merged, so chunk boundaries may
differ), consume all input and leave the parser empty.

The reference transcript itself is checked against a hand-written
expectation, so a parser that is consistently wrong also fails.
***************************************************************************** */
#define FIO_HTTP1_PARSER
#include "test-helpers.h"

/* *****************************************************************************
Transcript recorder (the parser's udata)
***************************************************************************** */

#define FRAG_TRANSCRIPT_CAPA 4096
#define FRAG_MSG_CAPA        1024

typedef struct {
  char buf[FRAG_TRANSCRIPT_CAPA];
  size_t len;
  int in_body; /* last token was body data (merge consecutive chunks) */
  int overflow;
} frag_transcript_s;

static void frag_put(frag_transcript_s *t, const char *data, size_t len) {
  if (t->len + len > FRAG_TRANSCRIPT_CAPA) {
    t->overflow = 1;
    return;
  }
  FIO_MEMCPY(t->buf + t->len, data, len);
  t->len += len;
}
static void frag_token(frag_transcript_s *t,
                       const char *tag,
                       fio_buf_info_s a,
                       const char *sep,
                       fio_buf_info_s b) {
  t->in_body = 0;
  frag_put(t, tag, strlen(tag));
  frag_put(t, a.buf, a.len);
  if (sep) {
    frag_put(t, sep, strlen(sep));
    frag_put(t, b.buf, b.len);
  }
  frag_put(t, "\n", 1);
}
#define FRAG_NONE FIO_BUF_INFO2(NULL, 0)

static void fio_http1_on_complete(void *udata) {
  frag_token((frag_transcript_s *)udata, "C", FRAG_NONE, NULL, FRAG_NONE);
}
static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  frag_token((frag_transcript_s *)udata, "M:", method, NULL, FRAG_NONE);
  return 0;
}
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  char tmp[32];
  int l = snprintf(tmp, sizeof(tmp), "%zu", istatus);
  frag_token((frag_transcript_s *)udata,
             "S:",
             FIO_BUF_INFO2(tmp, (size_t)l),
             " ",
             status);
  return 0;
}
static int fio_http1_on_url(fio_buf_info_s url, void *udata) {
  frag_token((frag_transcript_s *)udata, "U:", url, NULL, FRAG_NONE);
  return 0;
}
static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  frag_token((frag_transcript_s *)udata, "V:", version, NULL, FRAG_NONE);
  return 0;
}
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  frag_token((frag_transcript_s *)udata, "H:", name, "=", value);
  return 0;
}
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  char tmp[32];
  int l = snprintf(tmp, sizeof(tmp), "%zu", content_length);
  frag_token((frag_transcript_s *)udata,
             "L:",
             FIO_BUF_INFO2(tmp, (size_t)l),
             NULL,
             FRAG_NONE);
  (void)name, (void)value;
  return 0;
}
static int fio_http1_on_expect(void *udata) {
  frag_token((frag_transcript_s *)udata, "E", FRAG_NONE, NULL, FRAG_NONE);
  return 0;
}
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  frag_transcript_s *t = (frag_transcript_s *)udata;
  if (!chunk.len)
    return 0;
  if (t->in_body)
    --t->len; /* drop the "\n" terminator, append to the open body token */
  else
    frag_put(t, "B:", 2);
  frag_put(t, chunk.buf, chunk.len);
  frag_put(t, "\n", 1);
  t->in_body = 1;
  return 0;
}

/* *****************************************************************************
Fragmented feeding (mirrors `fio___http1_process_data`)
***************************************************************************** */

typedef struct {
  frag_transcript_s t;
  int error;
  size_t leftover;
  int parser_empty;
} frag_result_s;

/** Feeds `msg` cut at the ascending positions in `cuts`. */
static void frag_feed(frag_result_s *r,
                      const char *msg,
                      size_t len,
                      const size_t *cuts,
                      size_t ncuts) {
  char buf[FRAG_MSG_CAPA]; /* parser may write (lower-cases header names) */
  size_t buf_len = 0, pos = 0;
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  FIO_MEMSET(r, 0, sizeof(*r));
  FIO_ASSERT(len <= FRAG_MSG_CAPA, "fragmented test message too long");
  for (size_t i = 0; i <= ncuts; ++i) {
    size_t end = (i < ncuts) ? cuts[i] : len;
    FIO_MEMCPY(buf + buf_len, msg + pos, end - pos);
    buf_len += end - pos;
    pos = end;
    for (;;) {
      size_t consumed =
          fio_http1_parse(&parser, FIO_BUF_INFO2(buf, buf_len), &r->t);
      if (consumed == FIO_HTTP1_PARSER_ERROR) {
        r->error = 1;
        return;
      }
      if (!consumed)
        break;
      FIO_ASSERT(consumed <= buf_len,
                 "parser consumed %zu of %zu bytes",
                 consumed,
                 buf_len);
      buf_len -= consumed;
      if (buf_len)
        FIO_MEMMOVE(buf, buf + consumed, buf_len);
    }
  }
  r->leftover = buf_len;
  r->parser_empty = !!fio_http1_parser_is_empty(&parser);
}

/** Prints a transcript with escaped control characters. */
static void frag_print(const char *label, const char *s, size_t len) {
  fprintf(stderr, "      %s (%zu bytes):\n        ", label, len);
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = (unsigned char)s[i];
    if (c == '\n')
      fprintf(stderr, "\n        ");
    else if (c < 32 || c > 126)
      fprintf(stderr, "\\x%02X", c);
    else
      fputc(c, stderr);
  }
  fputc('\n', stderr);
}

static void frag_describe_cuts(char *dest,
                               size_t capa,
                               const size_t *cuts,
                               size_t ncuts) {
  size_t l = 0;
  dest[0] = 0;
  for (size_t i = 0; i < ncuts && l + 24 < capa; ++i)
    l += (size_t)snprintf(dest + l, capa - l, "%s%zu", (i ? "," : ""), cuts[i]);
  if (ncuts && l + 24 >= capa)
    snprintf(dest + l, capa - l, ",... (%zu cuts)", ncuts);
}

static void frag_compare(const char *name,
                         const char *msg,
                         size_t len,
                         const frag_result_s *ref,
                         const size_t *cuts,
                         size_t ncuts) {
  frag_result_s r;
  frag_feed(&r, msg, len, cuts, ncuts);
  if (!r.error && !r.t.overflow && !r.leftover && r.parser_empty &&
      r.t.len == ref->t.len && !memcmp(r.t.buf, ref->t.buf, r.t.len))
    return;
  char where[160];
  frag_describe_cuts(where, sizeof(where), cuts, ncuts);
  fprintf(stderr,
          "    FAIL %s: cuts at [%s]: error=%d leftover=%zu parser_empty=%d\n",
          name,
          where,
          r.error,
          r.leftover,
          r.parser_empty);
  frag_print("expected", ref->t.buf, ref->t.len);
  frag_print("got", r.t.buf, r.t.len);
  FIO_ASSERT(0, "%s: fragmented parse differs from unfragmented parse", name);
}

/* *****************************************************************************
Test driver
***************************************************************************** */

static void frag_test(const char *name,
                      const char *msg,
                      size_t len,
                      const char *expected,
                      size_t expected_len) {
  frag_result_s ref;
  size_t cuts[FRAG_MSG_CAPA];
  size_t runs = 0;
  fprintf(stderr, "  * fragmented: %s (%zu bytes)\n", name, len);

  /* reference: unfragmented, checked against the hand-written expectation */
  frag_feed(&ref, msg, len, NULL, 0);
  if (ref.error || ref.t.overflow || ref.leftover || !ref.parser_empty ||
      ref.t.len != expected_len || memcmp(ref.t.buf, expected, expected_len)) {
    fprintf(stderr,
            "    FAIL %s (unfragmented): error=%d leftover=%zu "
            "parser_empty=%d\n",
            name,
            ref.error,
            ref.leftover,
            ref.parser_empty);
    frag_print("expected", expected, expected_len);
    frag_print("got", ref.t.buf, ref.t.len);
    FIO_ASSERT(0, "%s: unfragmented parse mismatch", name);
  }

  /* every single cut */
  for (size_t i = 1; i < len; ++i) {
    cuts[0] = i;
    frag_compare(name, msg, len, &ref, cuts, 1);
    ++runs;
  }
  /* every pair of cuts */
  for (size_t i = 1; i < len; ++i) {
    for (size_t j = i + 1; j < len; ++j) {
      cuts[0] = i;
      cuts[1] = j;
      frag_compare(name, msg, len, &ref, cuts, 2);
      ++runs;
    }
  }
  /* every fixed fragment size (1 == byte-by-byte) */
  for (size_t step = 1; step < len; ++step) {
    size_t n = 0;
    for (size_t at = step; at < len; at += step)
      cuts[n++] = at;
    frag_compare(name, msg, len, &ref, cuts, n);
    ++runs;
  }
  fprintf(stderr, "    %zu fragmented runs matched\n", runs);
}

#define FRAG_TEST(name, msg, expected)                                         \
  frag_test(name, msg, sizeof(msg) - 1, expected, sizeof(expected) - 1)

int main(void) {
  fprintf(stderr, "Testing HTTP/1.x parser with fragmented input\n");

  FRAG_TEST("Content-Length request",
            "POST /upload/file.txt?a=1&b=two HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "X-Mixed-Case:   padded value  \r\n"
            "Content-Length: 25\r\n"
            "X-Empty:\r\n"
            "\r\n"
            "body\r\nwith CRLF\r\n\r\n& more",
            "M:POST\n"
            "U:/upload/file.txt?a=1&b=two\n"
            "V:HTTP/1.1\n"
            "H:host=example.com\n"
            "H:x-mixed-case=padded value\n"
            "L:25\n"
            "H:x-empty=\n"
            "B:body\r\nwith CRLF\r\n\r\n& more\n"
            "C\n");

  FRAG_TEST("chunked request with trailers",
            "POST /chunks HTTP/1.1\r\n"
            "Host: x\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\nHello\r\n"
            "1A\r\n0123456789\r\nabcdef\r\n012345\r\n"
            "0\r\n"
            "X-Trailer: done\r\n"
            "\r\n",
            "M:POST\n"
            "U:/chunks\n"
            "V:HTTP/1.1\n"
            "H:host=x\n"
            "B:Hello0123456789\r\nabcdef\r\n012345\n"
            "H:x-trailer=done\n"
            "C\n");

  FRAG_TEST("chunked request without trailers",
            "PUT /c HTTP/1.1\r\n"
            "Host: x\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "3\r\nabc\r\n"
            "0\r\n"
            "\r\n",
            "M:PUT\n"
            "U:/c\n"
            "V:HTTP/1.1\n"
            "H:host=x\n"
            "B:abc\n"
            "C\n");

  FRAG_TEST("LF-only line endings (Content-Length and chunked)",
            "POST /lf HTTP/1.1\n"
            "Host: x\n"
            "Content-Length: 3\n"
            "\n"
            "abc"
            "POST /lf2 HTTP/1.1\n"
            "Host: y\n"
            "Transfer-Encoding: chunked\n"
            "\n"
            "4\nwxyz\n"
            "0\n"
            "\n",
            "M:POST\n"
            "U:/lf\n"
            "V:HTTP/1.1\n"
            "H:host=x\n"
            "L:3\n"
            "B:abc\n"
            "C\n"
            "M:POST\n"
            "U:/lf2\n"
            "V:HTTP/1.1\n"
            "H:host=y\n"
            "B:wxyz\n"
            "C\n");

  FRAG_TEST("Expect: 100-continue request",
            "POST /expect HTTP/1.1\r\n"
            "Host: x\r\n"
            "Expect: 100-continue\r\n"
            "Content-Length: 4\r\n"
            "\r\n"
            "data",
            "M:POST\n"
            "U:/expect\n"
            "V:HTTP/1.1\n"
            "H:host=x\n"
            "L:4\n"
            "E\n"
            "B:data\n"
            "C\n");

  FRAG_TEST("pipelined requests (GET, Content-Length, chunked) after CRLF",
            "\r\n"
            "GET /one HTTP/1.1\r\n"
            "Host: a\r\n"
            "\r\n"
            "POST /two HTTP/1.1\r\n"
            "Host: b\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "12345"
            "POST /three HTTP/1.1\r\n"
            "Host: c\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "2\r\nxy\r\n"
            "0\r\n"
            "\r\n",
            "M:GET\n"
            "U:/one\n"
            "V:HTTP/1.1\n"
            "H:host=a\n"
            "C\n"
            "M:POST\n"
            "U:/two\n"
            "V:HTTP/1.1\n"
            "H:host=b\n"
            "L:5\n"
            "B:12345\n"
            "C\n"
            "M:POST\n"
            "U:/three\n"
            "V:HTTP/1.1\n"
            "H:host=c\n"
            "B:xy\n"
            "C\n");

  FRAG_TEST("response with Content-Length body",
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "hello world",
            "V:HTTP/1.1\n"
            "S:200 OK\n"
            "H:content-type=text/plain\n"
            "L:11\n"
            "B:hello world\n"
            "C\n");

  fprintf(stderr, "HTTP/1.x parser fragmented input tests passed\n");
  return 0;
}
