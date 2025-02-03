/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP1_PARSER       /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                HTTP/1.1 Parser




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP1_PARSER) && !defined(H___FIO_HTTP1_PARSER___H) &&         \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
/* *****************************************************************************
The HTTP/1.1 provides static functions only, always as part or implementation.
***************************************************************************** */
#define H___FIO_HTTP1_PARSER___H

/* *****************************************************************************
HTTP/1.x Parser API
***************************************************************************** */

/** The HTTP/1.1 parser type */
typedef struct fio_http1_parser_s fio_http1_parser_s;
/** Initialization value for the parser */
#define FIO_HTTP1_PARSER_INIT ((fio_http1_parser_s){0})

/**
 * Parses HTTP/1.x data, calling any callbacks.
 *
 * Returns bytes consumed or `FIO_HTTP1_PARSER_ERROR` (`(size_t)-1`) on error.
 */
FIO_SFUNC size_t fio_http1_parse(fio_http1_parser_s *p,
                                 fio_buf_info_s buf,
                                 void *udata);

/** Returns true if the parser is waiting to parse a new request/response .*/
FIO_IFUNC size_t fio_http1_parser_is_empty(fio_http1_parser_s *p);

/** Returns true if the parser is waiting for header data .*/
FIO_IFUNC size_t fio_http1_parser_is_on_header(fio_http1_parser_s *p);

/** Returns true if the parser is on body data .*/
FIO_IFUNC size_t fio_http1_parser_is_on_body(fio_http1_parser_s *p);

/** The error return value for fio_http1_parse. */
#define FIO_HTTP1_PARSER_ERROR ((size_t)-1)

/** Returns the number of bytes of payload still expected to be received. */
FIO_IFUNC size_t fio_http1_expected(fio_http1_parser_s *p);

/** A return value for `fio_http1_expected` when chunked data is expected. */
#define FIO_HTTP1_EXPECTED_CHUNKED ((size_t)(-1))

/* *****************************************************************************
HTTP/1.x callbacks (to be implemented by parser user)
***************************************************************************** */

/** called when either a request or a response was received. */
static void fio_http1_on_complete(void *udata);
/** called when a request method is parsed. */
static int fio_http1_on_method(fio_buf_info_s method, void *udata);
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata);
/** called when a request URL is parsed. */
static int fio_http1_on_url(fio_buf_info_s path, void *udata);
/** called when a the HTTP/1.x version is parsed. */
static int fio_http1_on_version(fio_buf_info_s version, void *udata);
/** called when a header is parsed. */
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata);
/** called when the special content-length header is parsed. */
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata);
/** called when `Expect` arrives and may require a 100 continue response. */
static int fio_http1_on_expect(void *udata);
/** called when a body chunk is parsed. */
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata);

/* *****************************************************************************
Implementation Stage Helpers
***************************************************************************** */

/* parsing stage 0 - read first line (proxy?). */
static int fio_http1___start(fio_http1_parser_s *p,
                             fio_buf_info_s *buf,
                             void *udata);
/* parsing stage 1 - read headers. */
static int fio_http1___read_header(fio_http1_parser_s *p,
                                   fio_buf_info_s *buf,
                                   void *udata);
/* parsing stage 2 - read body. */
static int fio_http1___read_body(fio_http1_parser_s *p,
                                 fio_buf_info_s *buf,
                                 void *udata);
/* parsing stage 2 - read chunked body. */
static int fio_http1___read_body_chunked(fio_http1_parser_s *p,
                                         fio_buf_info_s *buf,
                                         void *udata);
/* parsing stage 1 - read headers. */
static int fio_http1___read_trailer(fio_http1_parser_s *p,
                                    fio_buf_info_s *buf,
                                    void *udata);
/* completed parsing. */
static int fio_http1___finish(fio_http1_parser_s *p,
                              fio_buf_info_s *buf,
                              void *udata);

/* *****************************************************************************
HTTP Parser Type
***************************************************************************** */

/** The HTTP/1.1 parser type implementation */
struct fio_http1_parser_s {
  int (*fn)(fio_http1_parser_s *, fio_buf_info_s *, void *);
  size_t expected;
};

/** Returns true if the parser is waiting to parse a new request/response .*/
FIO_IFUNC size_t fio_http1_parser_is_empty(fio_http1_parser_s *p) {
  return !p->fn || p->fn == fio_http1___start;
}

/** Returns true if the parser is waiting for header data .*/
FIO_IFUNC size_t fio_http1_parser_is_on_header(fio_http1_parser_s *p) {
  return p->fn == fio_http1___read_header || p->fn == fio_http1___read_trailer;
}

/** Returns true if the parser is on body data .*/
FIO_IFUNC size_t fio_http1_parser_is_on_body(fio_http1_parser_s *p) {
  return p->fn == fio_http1___read_body ||
         p->fn == fio_http1___read_body_chunked;
}

/** Returns the number of bytes of payload still expected to be received. */
FIO_IFUNC size_t fio_http1_expected(fio_http1_parser_s *p) {
  return p->expected;
}

/* *****************************************************************************
Main Parsing Loop
***************************************************************************** */

FIO_SFUNC size_t fio_http1_parse(fio_http1_parser_s *p,
                                 fio_buf_info_s buf,
                                 void *udata) {
  int i = 0;
  char *buf_start = buf.buf;
  if (!buf.len)
    return 0;
  if (!p->fn)
    p->fn = fio_http1___start;
  while (!(i = p->fn(p, &buf, udata)))
    ;
  if (i < 0)
    return FIO_HTTP1_PARSER_ERROR;
  return buf.buf - buf_start;
}

/* completed parsing. */
static int fio_http1___finish(fio_http1_parser_s *p,
                              fio_buf_info_s *buf,
                              void *udata) {
  (void)buf;
  *p = (fio_http1_parser_s){0};
  fio_http1_on_complete(udata);
  return 1;
}

/* *****************************************************************************
Reading the first line
***************************************************************************** */

/* parsing stage 0 - read first line (TODO: proxy protocol support?). */
static int fio_http1___start(fio_http1_parser_s *p,
                             fio_buf_info_s *buf,
                             void *udata) {
  /* find line start/end and test */
  fio_buf_info_s wrd[3];
  char *start = buf->buf;
  char *tmp;
  while ((start[0] == ' ' || start[0] == '\r' || start[0] == '\n') &&
         start < buf->buf + buf->len) /* skip white space */
    ++start;
  if (start == buf->buf + buf->len) {
    buf->buf = start;
    return 1;
  }
  char *eol = (char *)FIO_MEMCHR(start, '\n', buf->len);
  if (!eol)
    return 1;
  if (start + 13 > eol) /* test for minimal data GET HTTP/1 or ### HTTP/1 */
    return -1;

  /* prep next stage */
  buf->len -= (eol - buf->buf) + 1;
  buf->buf = eol + 1;
  eol -= eol[-1] == '\r';

  /* parse first line */
  /* request: method path version ; response: version code txt */
  if (!(tmp = (char *)FIO_MEMCHR(start, ' ', (size_t)(eol - start))))
    return -1;
  wrd[0] = FIO_BUF_INFO2(start, (size_t)(tmp - start));
  start = tmp + 1;
  if (!(tmp = (char *)FIO_MEMCHR(start, ' ', eol - start)))
    return -1;
  wrd[1] = FIO_BUF_INFO2(start, (size_t)(tmp - start));
  start = tmp + 1;
  if (start >= eol)
    return -1;
  wrd[2] = FIO_BUF_INFO2(start, (size_t)(eol - start));
  if (fio_c2i(wrd[1].buf[0]) < 10) /* test if path or code */
    goto parse_response_line;
  if (wrd[2].len > 14)
    wrd[2].len = 14;
  if (fio_http1_on_method(wrd[0], udata))
    return -1;
  if (fio_http1_on_url(wrd[1], udata))
    return -1;
  if (fio_http1_on_version(wrd[2], udata))
    return -1;
  return (p->fn = fio_http1___read_header)(p, buf, udata);

parse_response_line:
  if (wrd[0].len > 14)
    wrd[0].len = 14;
  if (fio_http1_on_version(wrd[0], udata))
    return -1;
  if (fio_http1_on_status(fio_atol10u(&wrd[1].buf), wrd[2], udata))
    return -1;
  return (p->fn = fio_http1___read_header)(p, buf, udata);
}

/* *****************************************************************************
Reading Headers
***************************************************************************** */

/* parsing stage 1 - read headers (after `expect` header). */
static int fio_http1___read_header_post_expect(fio_http1_parser_s *p,
                                               fio_buf_info_s *buf,
                                               void *udata);

/* handle headers before calling callback. */
static inline int fio_http1___on_header(fio_http1_parser_s *p,
                                        fio_buf_info_s name,
                                        fio_buf_info_s value,
                                        void *udata) {
  /* test for special headers */
  switch (name.len) {
  case 6: /* test for "expect" */
    if (value.len == 12 && fio_buf2u32u(name.buf) == fio_buf2u32u("expe") &&
        fio_buf2u32u(name.buf + 2) == fio_buf2u32u("pect") &&
        fio_buf2u64u(value.buf) == fio_buf2u64u("100-cont") &&
        fio_buf2u32u(value.buf + 8) == fio_buf2u32u("inue")) { /* Expect */
      p->fn = fio_http1___read_header_post_expect;
      return 0;
    }
    break;
  case 14: /* test for "content-length" */
    if (fio_buf2u64u(name.buf) == fio_buf2u64u("content-") &&
        fio_buf2u64u(name.buf + 6) == fio_buf2u64u("t-length")) {
      char *tmp = value.buf;
      uint64_t clen = fio_atol10u(&tmp);
      if (tmp != value.buf + value.len)
        return -1;
      if (p->expected)
        return 0 - (p->expected != clen);
      p->expected = clen;
      return 0 -
             (fio_http1_on_header_content_length(name, value, clen, udata) ==
              -1);
    }
    break;
  case 17: /* test for "transfer-encoding" (chunked?) */
    if (value.len >= 7 && (name.buf[16] == 'g') &&
        !((fio_buf2u64u(name.buf) ^ fio_buf2u64u("transfer")) |
          (fio_buf2u64u(name.buf + 8) ^ fio_buf2u64u("-encodin")))) {
      char *c_start = value.buf + value.len - 7;
      if ((fio_buf2u32u(c_start) | 0x20202020UL) == fio_buf2u32u("chun") &&
          (fio_buf2u32u(c_start + 3) | 0x20202020UL) == fio_buf2u32u("nked")) {
        if (p->expected && p->expected != FIO_HTTP1_EXPECTED_CHUNKED)
          return -1;
        p->expected = FIO_HTTP1_EXPECTED_CHUNKED;
        /* endpoint does not need to know if the body was chunked or not */
        if (value.len == 7)
          return 0;
        if (c_start[-1] != ' ' && c_start[-1] != ',' && c_start[-1] != '\t')
          return -1;
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
  return 0 - (fio_http1_on_header(name, value, udata) == -1);
}

/* handle trailers (chunked encoding only) before calling callback. */
static inline int fio_http1___on_trailer(fio_http1_parser_s *p,
                                         fio_buf_info_s name,
                                         fio_buf_info_s value,
                                         void *udata) {
  (void)p;
  fio_buf_info_s forbidden[] = {
      FIO_BUF_INFO1((char *)"authorization"),
      FIO_BUF_INFO1((char *)"cache-control"),
      FIO_BUF_INFO1((char *)"content-encoding"),
      FIO_BUF_INFO1((char *)"content-length"),
      FIO_BUF_INFO1((char *)"content-range"),
      FIO_BUF_INFO1((char *)"content-type"),
      FIO_BUF_INFO1((char *)"expect"),
      FIO_BUF_INFO1((char *)"host"),
      FIO_BUF_INFO1((char *)"max-forwards"),
      FIO_BUF_INFO1((char *)"set-cookie"),
      FIO_BUF_INFO1((char *)"te"),
      FIO_BUF_INFO1((char *)"trailer"),
      FIO_BUF_INFO1((char *)"transfer-encoding"),
      FIO_BUF_INFO2(NULL, 0),
  }; /* known forbidden headers in trailer */
  for (size_t i = 0; forbidden[i].buf; ++i) {
    if (FIO_BUF_INFO_IS_EQ(name, forbidden[i]))
      return -1;
  }
  return fio_http1_on_header(name, value, udata);
}

/* returns either a lower case (ASCI) or the original char. */
static uint8_t fio_http1_tolower(uint8_t c) {
  if ((c - ((uint8_t)'A' - 1U)) < ((uint8_t)'Z' - (uint8_t)'A'))
    c |= 32;
  return c;
}

/* seeks to the ':' divisor while testing and converting to downcase. */
static char *fio_http1___seek_header_div(char *p) {
  /* this is the subset of the forbidden chars that allows UTF-8 headers */
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
    *p = (char)fio_http1_tolower((uint8_t)(*p));
    ++p;
    if (FIO_UNLIKELY(forbidden_name_chars[((uint8_t)(*p))]))
      return p;
  }
}

/* extract header name and value from a line and pass info to handler */
static inline int fio_http1___read_header_line(
    fio_http1_parser_s *p,
    fio_buf_info_s *buf,
    void *udata,
    int (*handler)(fio_http1_parser_s *,
                   fio_buf_info_s,
                   fio_buf_info_s,
                   void *)) {
  for (;;) {
    char *start = buf->buf;
    char *eol = (char *)FIO_MEMCHR(start, '\n', buf->len);
    char *div;
    fio_buf_info_s name, value;
    if (!eol)
      return 1;

    buf->len -= (eol - buf->buf) + 1;
    buf->buf = eol + 1;
    eol -= (eol[-1] == '\r');
    if (FIO_UNLIKELY(eol == start))
      goto headers_finished;

    div = fio_http1___seek_header_div(start);
    if (div[0] != ':')
      return -1;
    name = FIO_BUF_INFO2(start, (size_t)(div - start));
    do {
      ++div;
    } while (*div == ' ' || *div == '\t');

    if (div != eol)
      while (eol[-1] == ' ' || eol[-1] == '\t')
        --eol;
    value = FIO_BUF_INFO2((div == eol) ? NULL : div, (size_t)(eol - div));
    int r = handler(p, name, value, udata);
    if (FIO_UNLIKELY(r))
      return r;
  }

headers_finished:
  if (p->fn == fio_http1___read_header_post_expect && p->expected &&
      fio_http1_on_expect(udata))
    goto expect_failed;
  p->fn = (!p->expected)         ? fio_http1___finish
          : (!(p->expected + 1)) ? fio_http1___read_body_chunked
                                 : fio_http1___read_body;
  return p->fn(p, buf, udata);

expect_failed:
  *p = (fio_http1_parser_s){0};
  return 1;
}

/* parsing stage 1 - read headers. */
static int fio_http1___read_header(fio_http1_parser_s *p,
                                   fio_buf_info_s *buf,
                                   void *udata) {
  return fio_http1___read_header_line(p, buf, udata, fio_http1___on_header);
}

/* parsing stage 1 - read headers (after `expect` header). */
static int fio_http1___read_header_post_expect(fio_http1_parser_s *p,
                                               fio_buf_info_s *buf,
                                               void *udata) {
  return fio_http1___read_header_line(p, buf, udata, fio_http1___on_header);
}

/* parsing stage 1 - read headers. */
static int fio_http1___read_trailer(fio_http1_parser_s *p,
                                    fio_buf_info_s *buf,
                                    void *udata) {
  return fio_http1___read_header_line(p, buf, udata, fio_http1___on_trailer);
}

/* *****************************************************************************
Reading the Body
***************************************************************************** */

/* parsing stage 2 - read body - known content length. */
static int fio_http1___read_body(fio_http1_parser_s *p,
                                 fio_buf_info_s *buf,
                                 void *udata) {
  if (!buf->len)
    return 1;
  if (buf->len >= p->expected) {
    buf->len = p->expected;
    if (fio_http1_on_body_chunk(*buf, udata))
      return -1;
    buf->buf += buf->len;
    return fio_http1___finish(p, buf, udata);
  }
  if (fio_http1_on_body_chunk(*buf, udata))
    return -1;
  buf->buf += buf->len;
  p->expected -= buf->len;
  buf->len = 0;
  return 1;
}

/* *****************************************************************************
Reading the Body (chunked)
***************************************************************************** */

/* parsing stage 2 - read chunked body - read chunk data. */
static int fio_http1___read_body_chunked_read(fio_http1_parser_s *p,
                                              fio_buf_info_s *buf,
                                              void *udata) {
  if (!buf->len)
    return 1;
  if (buf->len >= p->expected) {
    if (fio_http1_on_body_chunk(FIO_BUF_INFO2(buf->buf, p->expected), udata))
      return -1;
    buf->buf += p->expected;
    buf->len -= p->expected;
    p->fn = fio_http1___read_body_chunked;
    return 0;
  }
  if (fio_http1_on_body_chunk(buf[0], udata))
    return -1;
  p->expected -= buf->len;
  buf->buf += buf->len;
  return 1;
}

/* parsing stage 2 - read chunked body - read next chunk length. */
static int fio_http1___read_body_chunked(fio_http1_parser_s *p,
                                         fio_buf_info_s *buf,
                                         void *udata) {
  (void)udata;
  if (buf->len < 3)
    return 1;
  { /* remove possible extra EOL after chunk payload */
    size_t tmp = (buf->buf[0] == '\r');
    tmp += (buf->buf[tmp] == '\n');
    buf->len -= tmp;
    buf->buf += tmp;
  }

  // if (!FIO_MEMCHR(buf->buf, '\n', buf->len)) /* prevent read overflow? */
  //   return 1;

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
    return (p->fn = fio_http1___read_body_chunked_read)(p, buf, udata);
  }
  if ((eol + 1 < buf->buf + buf->len) && (eol[0] == '\r' || eol[0] == '\n')) {
    /* no trailers, finish now. */
    eol += (eol[0] == '\r');
    ++eol;
    buf->len -= eol - buf->buf;
    buf->buf = eol;
    return fio_http1___finish(p, buf, udata);
  }
  /* possible trailers */
  buf->len -= eol - buf->buf;
  buf->buf = eol;
  return (p->fn = fio_http1___read_trailer)(p, buf, udata);
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#undef FIO_HTTP1_PARSER
#endif /* FIO_HTTP1_PARSER && FIO_EXTERN_COMPLETE*/
