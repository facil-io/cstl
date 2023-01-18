/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MODULE_NAME module /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                HTTP/1.1 Parser




Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP1_PARSER) && !defined(H___FIO_HTTP1_PARSER___H)
#define H___FIO_HTTP1_PARSER___H
/* *****************************************************************************
The HTTP/1.1 provides static functions only, always as part or implementation.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
HTTP/1.x Parser API
***************************************************************************** */

/** The HTTP/1.1 parser type */
typedef struct http1_parser_s http1_parser_s;
/** Initialization value for the parser */
#define HTTP1_PARSER_INIT ((http1_parser_s){0})

/**
 * Parses HTTP/1.x data, calling any callbacks.
 *
 * Returns bytes consumed or `HTTP1_PARSER_ERROR` (`(size_t)-1`) on error.
 */
static size_t http1_parse(http1_parser_s *p, fio_buf_info_s buf, void *udata);

/** The error return value for http1_parse. */
#define HTTP1_PARSER_ERROR ((size_t)-1)

/* *****************************************************************************
HTTP/1.x callbacks (to be implemented by parser user)
***************************************************************************** */

/** called when either a request or a response was received. */
static int http1_on_complete(void *udata);
/** called when a request method is parsed. */
static int http1_on_method(fio_buf_info_s method, void *udata);
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int http1_on_status(size_t istatus, fio_buf_info_s status, void *udata);
/** called when a request URL is parsed. */
static int http1_on_url(fio_buf_info_s path, void *udata);
/** called when a the HTTP/1.x version is parsed. */
static int http1_on_version(fio_buf_info_s version, void *udata);
/** called when a header is parsed. */
static int http1_on_header(fio_buf_info_s name,
                           fio_buf_info_s value,
                           void *udata);
/** called when the special content-length header is parsed. */
static int http1_on_header_content_length(fio_buf_info_s name,
                                          fio_buf_info_s value,
                                          size_t content_length,
                                          void *udata);
/** called when `Expect` arrives and may require a 100 continue response. */
static int http1_on_expect(fio_buf_info_s expected, void *udata);
/** called when a body chunk is parsed. */
static int http1_on_body_chunk(fio_buf_info_s chunk, void *udata);

/* *****************************************************************************
Implementation Stage Helpers
***************************************************************************** */

/* parsing stage 0 - read first line (proxy?). */
static int http1___start(http1_parser_s *p, fio_buf_info_s *buf, void *udata);
/* parsing stage 1 - read headers. */
static int http1___read_header(http1_parser_s *p,
                               fio_buf_info_s *buf,
                               void *udata);
/* parsing stage 2 - read body. */
static int http1___read_body(http1_parser_s *p,
                             fio_buf_info_s *buf,
                             void *udata);
/* parsing stage 2 - read chunked body. */
static int http1___read_body_chunked(http1_parser_s *p,
                                     fio_buf_info_s *buf,
                                     void *udata);
/* parsing stage 1 - read headers. */
static int http1___read_trailer(http1_parser_s *p,
                                fio_buf_info_s *buf,
                                void *udata);
/* completed parsing. */
static int http1___finish(http1_parser_s *p, fio_buf_info_s *buf, void *udata);

/* *****************************************************************************
Main Parsing Loop
***************************************************************************** */

/** The HTTP/1.1 parser type implementation */
struct http1_parser_s {
  int (*fn)(http1_parser_s *, fio_buf_info_s *, void *);
  size_t expected;
};

static size_t http1_parse(http1_parser_s *p, fio_buf_info_s buf, void *udata) {
  int i = 0;
  char *buf_start = buf.buf;
  if (!p->fn)
    p->fn = http1___start;
  while (buf.len && !(i = p->fn(p, &buf, udata)))
    ;
  if (i < 0)
    return HTTP1_PARSER_ERROR;
  return buf.buf - buf_start;
}
#define HTTP1___EXPECTED_CHUNKED ((size_t)(-1))

/* completed parsing. */
static int http1___finish(http1_parser_s *p, fio_buf_info_s *buf, void *udata) {
  (void)buf;
  *p = (http1_parser_s){0};
  http1_on_complete(udata);
  return 1;
}

/* *****************************************************************************
Reading the first line
***************************************************************************** */

/* parsing stage 0 - read first line (TODO: proxy protocol support?). */
static int http1___start(http1_parser_s *p, fio_buf_info_s *buf, void *udata) {
  /* find line start/end and test */
  char *start = buf->buf;
  char *tmp;
  while ((start[0] == ' ' || start[0] == '\r' || start[0] == '\n') &&
         start < buf->buf + buf->len) /* skip white space */
    ++start;
  if (start == buf->buf + buf->len) {
    buf->buf = start;
    p->fn = http1___finish;
    return 0;
  }
  char *eol = FIO_MEMCHR(start, '\n', buf->len);
  if (!eol)
    return 1;
  if (start + 13 > eol) /* test for minimal data GET HTTP/1 or ### HTTP/1 */
    return -1;

  /* prep next stage */
  buf->len -= (eol - buf->buf) + 1;
  buf->buf = eol + 1;
  p->fn = http1___read_header;
  eol -= eol[-1] == '\r';

  /* parse first line */
  if (start[0] > ('0' - 1) && start[0] < ('9' + 1))
    goto parse_response_line;
  /* request: method path version */
  if (!(tmp = FIO_MEMCHR(start, ' ', eol - start)))
    return -1;
  if (http1_on_method(FIO_BUF_INFO2(start, tmp - start), udata))
    return -1;
  start = tmp + 1;
  if (!(tmp = FIO_MEMCHR(start, ' ', eol - start)))
    return -1;
  if (http1_on_url(FIO_BUF_INFO2(start, tmp - start), udata))
    return -1;
  start = tmp + 1;
  if (start >= eol)
    return -1;
  if (http1_on_version(FIO_BUF_INFO2(start, (eol - start)), udata))
    return -1;
  return 0;

parse_response_line:
  /* response: version code text */
  if (!(tmp = FIO_MEMCHR(start, ' ', eol - start)))
    return -1;
  if (http1_on_version(FIO_BUF_INFO2(start, (tmp - start)), udata))
    return -1;
  start = tmp + 1;
  if (!(tmp = FIO_MEMCHR(start, ' ', eol - start)))
    return -1;
  if (http1_on_status(fio_atol10(&start),
                      FIO_BUF_INFO2((tmp + 1), eol - tmp),
                      udata))
    return -1;
  return 0;
}

/* *****************************************************************************
Reading Headers
***************************************************************************** */

/* returns either a lower case (ASCI) or the original char. */
static uint8_t http1_tolower(uint8_t c) {
  if ((c - ((uint8_t)'A' - 1U)) < ((uint8_t)'Z' - (uint8_t)'A'))
    c |= 32;
  return c;
}

/* seeks to the ':' divisor while testing and converting to downcase. */
static char *http1___seek_header_div(char *p) {
  static const _Bool forbidden_name_chars[256] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for (;;) {
    if (forbidden_name_chars[((uint8_t)(*p))])
      return p;
    *p = (char)http1_tolower((uint8_t)(*p));
    ++p;
  }
}

/* extract header name and value from a line and pass info to handler */
static inline int http1___read_header_line(
    http1_parser_s *p,
    fio_buf_info_s *buf,
    void *udata,
    int (*handler)(http1_parser_s *, fio_buf_info_s, fio_buf_info_s, void *)) {
  char *start = buf->buf;
  char *eol = FIO_MEMCHR(start, '\n', buf->len);
  char *div;
  fio_buf_info_s name, value;
  if (!eol)
    return 1;

  buf->len -= (eol - buf->buf) + 1;
  buf->buf = eol + 1;
  eol -= (eol[-1] == '\r');
  if (eol == start)
    goto headers_finished;

  div = http1___seek_header_div(start);
  if (div[0] != ':')
    return -1;
  name = FIO_BUF_INFO2(start, (div - start));
  do {
    ++div;
  } while (*div == ' ' || *div == '\t');

  if (div != eol)
    while (eol[-1] == ' ' || eol[-1] == '\t')
      --eol;
  value = FIO_BUF_INFO2((div == eol) ? NULL : div, (eol - div));
  return handler(p, name, value, udata);

headers_finished:
  p->fn = (!p->expected)         ? http1___finish
          : (!(p->expected + 1)) ? http1___read_body_chunked
                                 : http1___read_body;
  return 0;
}

/* handle main header. */
static inline int http1___read_on_header(http1_parser_s *p,
                                         fio_buf_info_s name,
                                         fio_buf_info_s value,
                                         void *udata) {
  /* test for special headers */
  switch (name.len) {
  case 6: /* test for "expect" */
    if (fio_buf2u32_local(name.buf) == fio_buf2u32_local("expe") &&
        fio_buf2u32_local(name.buf + 2) ==
            fio_buf2u32_local("pect")) { /* Expect */
      return 0 - http1_on_expect(value, udata);
    }
    break;
  case 14: /* test for "content-length" */
    if (fio_buf2u64_local(name.buf) == fio_buf2u64_local("content-") &&
        fio_buf2u64_local(name.buf + 6) == fio_buf2u64_local("t-length")) {
      char *tmp = value.buf;
      uint64_t clen = fio_atol10u(&tmp);
      if (tmp != value.buf + value.len)
        return -1;
      if (p->expected)
        return 0 - (p->expected != clen);
      p->expected = clen;
      return 0 -
             (http1_on_header_content_length(name, value, clen, udata) == -1);
    }
    break;
  case 17: /* test for "transfer-encoding" (chunked?) */
    if (value.len >= 7 && (name.buf[16] == 'g') &&
        ((fio_buf2u64_local(name.buf) == fio_buf2u64_local("transfer")) &
         (fio_buf2u64_local(name.buf + 8) == fio_buf2u64_local("-encodin")))) {
      char *c_start = value.buf + value.len - 7;
      if ((fio_buf2u32_local(c_start) | 0x20202020UL) ==
              fio_buf2u32_local("chun") &&
          (fio_buf2u32_local(c_start + 3) | 0x20202020UL) ==
              fio_buf2u32_local("nked")) {
        if (p->expected && p->expected != HTTP1___EXPECTED_CHUNKED)
          return -1;
        p->expected = HTTP1___EXPECTED_CHUNKED;
        /* endpoint does not need to know if the body was chunked or not */
        if (value.len == 7)
          return 0;
        while (
            (c_start[-1] == ' ' || c_start[-1] == ',' || c_start[-1] == '\t') &&
            c_start > value.buf)
          --c_start;
        if (c_start == value.buf)
          return 0;
        value.len = c_start - value.buf;
      }
    }
    break;
  }
  /* perform callback */
  return http1_on_header(name, value, udata);
}

/* handle trailers (chunked encoding only). */
static inline int http1___read_on_trailer(http1_parser_s *p,
                                          fio_buf_info_s name,
                                          fio_buf_info_s value,
                                          void *udata) {
  (void)p;
  fio_buf_info_s forbidden[] = {
      FIO_BUF_INFO1("authorization"),
      FIO_BUF_INFO1("cache-control"),
      FIO_BUF_INFO1("content-encoding"),
      FIO_BUF_INFO1("content-length"),
      FIO_BUF_INFO1("content-range"),
      FIO_BUF_INFO1("content-type"),
      FIO_BUF_INFO1("expect"),
      FIO_BUF_INFO1("host"),
      FIO_BUF_INFO1("max-forwards"),
      FIO_BUF_INFO1("set-cookie"),
      FIO_BUF_INFO1("te"),
      FIO_BUF_INFO1("trailer"),
      FIO_BUF_INFO1("transfer-encoding"),
      FIO_BUF_INFO2(NULL, 0),
  }; /* known forbidden headers in trailer */
  for (size_t i = 0; forbidden[i].buf; ++i) {
    if (FIO_BUF_INFO_IS_EQ(name, forbidden[i]))
      return -1;
  }
  return http1_on_header(name, value, udata);
}

/* parsing stage 1 - read headers. */
static int http1___read_header(http1_parser_s *p,
                               fio_buf_info_s *buf,
                               void *udata) {
  return http1___read_header_line(p, buf, udata, http1___read_on_header);
}

/* parsing stage 1 - read headers. */
static int http1___read_trailer(http1_parser_s *p,
                                fio_buf_info_s *buf,
                                void *udata) {
  return http1___read_header_line(p, buf, udata, http1___read_on_trailer);
}

/* *****************************************************************************
Reading the Body
***************************************************************************** */

/* parsing stage 2 - read body - known content length. */
static int http1___read_body(http1_parser_s *p,
                             fio_buf_info_s *buf,
                             void *udata) {
  if (buf->len >= p->expected) {
    buf->len = p->expected;
    if (http1_on_body_chunk(*buf, udata))
      return -1;
    buf->buf += buf->len;
    p->fn = http1___finish;
    return 0;
  }
  if (http1_on_body_chunk(*buf, udata))
    return -1;
  buf->buf += buf->len;
  return 1;
}

/* *****************************************************************************
Reading the Body (chunked)
***************************************************************************** */

/* parsing stage 2 - read chunked body - read chunk data. */
static int http1___read_body_chunked_read(http1_parser_s *p,
                                          fio_buf_info_s *buf,
                                          void *udata) {
  if (buf->len >= p->expected) {
    if (http1_on_body_chunk(FIO_BUF_INFO2(buf->buf, p->expected), udata))
      return -1;
    buf->buf += p->expected;
    buf->len -= p->expected;
    p->fn = http1___read_body_chunked;
    return 0;
  }
  if (http1_on_body_chunk(buf[0], udata))
    return -1;
  p->expected -= buf->len;
  buf->buf += buf->len;
  return 1;
}

/* parsing stage 2 - read chunked body - read next chunk length. */
static int http1___read_body_chunked(http1_parser_s *p,
                                     fio_buf_info_s *buf,
                                     void *udata) {
  (void)udata;
  if (buf->len < 3)
    return 1;
  buf->len -= buf->buf[0] == '\r';
  buf->buf += buf->buf[0] == '\r';
  buf->len -= buf->buf[0] == '\n';
  buf->buf += buf->buf[0] == '\n';
  char *eol = buf->buf;
  size_t expected = fio_atol16u(&eol); /* may read overflow, tests after */
  if (eol == buf->buf)
    return -1;
  eol += (eol[0] == '\r');
  if (eol >= buf->buf + buf->len)
    return 1; /* read overflowed */
  if (eol[0] != '\n')
    return -1;
  ++eol;
  p->expected = expected;
  if (p->expected) {
    /* further data expected */
    buf->len -= eol - buf->buf;
    buf->buf = eol;
    p->fn = http1___read_body_chunked_read;
    return 0;
  }
  if ((eol + 1 < buf->buf + buf->len) && (eol[0] == '\r' || eol[0] == '\n')) {
    /* no trailers, finish now. */
    eol += (eol[0] == '\r');
    ++eol;
    buf->len -= eol - buf->buf;
    buf->buf = eol;
    p->fn = http1___finish;
    return 0;
  }
  /* possible trailers */
  buf->len -= eol - buf->buf;
  buf->buf = eol;
  p->fn = http1___read_trailer;
  return 0;
}

/* *****************************************************************************
Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MODULE_NAME)(void) {
  /*
   * TODO: test HTTP parser here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_HTTP1_PARSER */
#undef FIO_HTTP1_PARSER
