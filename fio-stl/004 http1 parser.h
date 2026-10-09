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

/**
 * Marks the current message as having no body, regardless of any
 * `content-length` / `transfer-encoding` headers (i.e., a response to `HEAD`).
 *
 * Call from a parser callback before the headers end (i.e., `on_status`). The
 * mark is cleared once the message completes. Responses with a 1xx, 204 or 304
 * status are marked automatically (RFC 9112 §6.3).
 */
FIO_IFUNC void fio_http1_parser_skip_body(fio_http1_parser_s *p);

/**
 * Returns non-zero if the current message has no body.
 *
 * Known once the first line was parsed (before any header callback): requests
 * using GET / HEAD / OPTIONS, responses with a 1xx / 204 / 304 status, and
 * messages marked by `fio_http1_parser_skip_body`. Also true after a
 * `content-length: 0` header.
 *
 * Callbacks should test this before reserving body space - i.e., the
 * `content-length` of a HEAD response describes a body that never arrives.
 */
FIO_IFUNC size_t fio_http1_parser_skips_body(fio_http1_parser_s *p);

/** A return value for `fio_http1_expected` when chunked data is expected. */
#define FIO_HTTP1_EXPECTED_CHUNKED ((size_t)(-2))

/** `fio_http1_expected` value when body isn't allowed (GET/HEAD/OPTIONS). */
#define FIO___HTTP1_BODY_NOT_ALLOWED ((size_t)(-1))

/** Internal parser flag: an accepted `Expect: 100-continue` header was seen. */
#define FIO___HTTP1_FLAG_EXPECT ((size_t)1)
/** Internal parser flag: the message has no body (ignore body framing). */
#define FIO___HTTP1_FLAG_NO_BODY ((size_t)2)
/** Internal parser flag: an HTTP/1.1+ request still requires a Host header. */
#define FIO___HTTP1_FLAG_HOST_REQUIRED ((size_t)4)
/** Internal parser flag: a request Host header was already received. */
#define FIO___HTTP1_FLAG_HOST_SEEN ((size_t)8)
/** Internal parser flag: the message is a request (not a response). */
#define FIO___HTTP1_FLAG_REQUEST ((size_t)16)
/** Internal parser flag: Transfer-Encoding seen without a final `chunked`. */
#define FIO___HTTP1_FLAG_TE_UNFRAMED ((size_t)32)

/**
 * Returns non-zero if `version` is exactly `HTTP/1.0`.
 *
 * HTTP/1.0 messages are not persistent without a `keep-alive` connection
 * option and never use chunked transfer coding (RFC 9112 §9.3, §6.1). Every
 * other version string (HTTP/0.9 is unsupported) gets HTTP/1.1 semantics.
 */
FIO_IFUNC int fio_http1_version_is_legacy(fio_buf_info_s version);

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
  /* per-message state bits (`FIO___HTTP1_FLAG_*`). State is never encoded in
   * function pointer identity: identical stage functions may be folded by the
   * linker (i.e., MSVC / lld `/OPT:ICF`), making such comparisons unreliable.
   */
  size_t flags;
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
  if (p->expected == FIO___HTTP1_BODY_NOT_ALLOWED)
    return 0;
  return p->expected;
}

/** Marks the current message as having no body (i.e., `HEAD` response). */
FIO_IFUNC void fio_http1_parser_skip_body(fio_http1_parser_s *p) {
  p->flags |= FIO___HTTP1_FLAG_NO_BODY;
}

/** Returns non-zero if the current message has no body. */
FIO_IFUNC size_t fio_http1_parser_skips_body(fio_http1_parser_s *p) {
  return (p->flags & FIO___HTTP1_FLAG_NO_BODY) |
         (size_t)(p->expected == FIO___HTTP1_BODY_NOT_ALLOWED);
}

/** Returns non-zero if `version` is exactly `HTTP/1.0` (one 64 bit test). */
FIO_IFUNC int fio_http1_version_is_legacy(fio_buf_info_s v) {
  return v.len == 8 && fio_buf2u64u(v.buf) == fio_buf2u64u("HTTP/1.0");
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
  const uint32_t method_get = (fio_buf2u32u("GET ") | 0x20202020);
  const uint32_t method_head = (fio_buf2u32u("HEAD") | 0x20202020);
  const uint64_t method_options =
      (fio_buf2u64u("OPTIONS ") | (uint64_t)0x2020202020202020ULL);
  /* find line start/end and test */
  fio_buf_info_s wrd[3];
  char *start = buf->buf;
  char *tmp;
  while (start < buf->buf + buf->len &&
         (start[0] == ' ' || start[0] == '\r' || start[0] == '\n'))
    ++start; /* skip white space */
  if (start == buf->buf + buf->len) {
    buf->buf = start;
    return 1;
  }
  char *eol =
      (char *)FIO_MEMCHR(start, '\n', (size_t)((buf->buf + buf->len) - start));
  if (!eol)
    return 1;
  if (start + 13 > eol) /* test for minimal data GET HTTP/1 or ### HTTP/1 */
    return -1;
  /* test for `NUL` in data */
  if (FIO_MEMCHR(start, 0, (size_t)(eol - start)))
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
  /* GET / HEAD / OPTIONS requests have no body (known before callbacks) */
  if (((wrd[0].len == 3 || wrd[0].len == 4) &&
       ((fio_buf2u32u(wrd[0].buf) | 0x20202020) == method_get ||
        (fio_buf2u32u(wrd[0].buf) | 0x20202020) == method_head)) ||
      (wrd[0].len == 7 &&
       ((fio_buf2u64u(wrd[0].buf) | (uint64_t)0x2020202020202020ULL) ==
        method_options)))
    p->expected = FIO___HTTP1_BODY_NOT_ALLOWED;
  /* HTTP/1.1 requests require exactly one Host header (RFC 9112 §3.2) */
  p->flags |= FIO___HTTP1_FLAG_REQUEST;
  if (!fio_http1_version_is_legacy(wrd[2]))
    p->flags |= FIO___HTTP1_FLAG_HOST_REQUIRED;

  if (fio_http1_on_method(wrd[0], udata))
    return -1;
  if (fio_http1_on_url(wrd[1], udata))
    return -1;
  if (fio_http1_on_version(wrd[2], udata))
    return -1;

  /* switch to header reading mode */
  return (p->fn = fio_http1___read_header)(p, buf, udata);

parse_response_line:
  if (wrd[0].len > 14)
    wrd[0].len = 14;
  if (fio_http1_on_version(wrd[0], udata))
    return -1;
  {
    const size_t status = fio_atol10u(&wrd[1].buf);
    /* 1xx, 204 and 304 responses never have a body (RFC 9112 §6.3) */
    if ((status - 100) < 100 || status == 204 || status == 304)
      p->flags |= FIO___HTTP1_FLAG_NO_BODY;
    if (fio_http1_on_status(status, wrd[2], udata))
      return -1;
  }
  return (p->fn = fio_http1___read_header)(p, buf, udata);
}

/* *****************************************************************************
Reading Headers
***************************************************************************** */

/* Returns non-zero if a transfer-coding list names `chunked` (any position). */
static int fio_http1___te_has_chunked(fio_buf_info_s v) {
  while (v.len) {
    char *comma = (char *)FIO_MEMCHR(v.buf, ',', v.len);
    size_t len = comma ? (size_t)(comma - v.buf) : v.len;
    char *semi = (char *)FIO_MEMCHR(v.buf, ';', len);
    size_t start = 0, end = semi ? (size_t)(semi - v.buf) : len;
    while (start < end && (v.buf[start] == ' ' || v.buf[start] == '\t'))
      ++start;
    while (end > start && (v.buf[end - 1] == ' ' || v.buf[end - 1] == '\t'))
      --end;
    if (end - start == 7 &&
        (fio_buf2u32u(v.buf + start) | 0x20202020UL) == fio_buf2u32u("chun") &&
        (fio_buf2u32u(v.buf + start + 3) | 0x20202020UL) ==
            fio_buf2u32u("nked"))
      return 1;
    len += !!comma;
    v.buf += len;
    v.len -= len;
  }
  return 0;
}

/* handle headers before calling callback. */
static inline int fio_http1___on_header(fio_http1_parser_s *p,
                                        fio_buf_info_s name,
                                        fio_buf_info_s value,
                                        void *udata) {
  /* a response without a body (HEAD / 1xx / 204 / 304) ignores body framing:
   * `content-length` is informational, other framing headers are forwarded. */
  const size_t no_body = (p->flags & FIO___HTTP1_FLAG_NO_BODY);
  /* test for special headers */
  switch (name.len) {
  case 4: /* test for "host" (requests: exactly one, RFC 9112 §3.2) */
    if ((p->flags & FIO___HTTP1_FLAG_REQUEST) &&
        fio_buf2u32u(name.buf) == fio_buf2u32u("host")) {
      if ((p->flags & FIO___HTTP1_FLAG_HOST_SEEN))
        return -1;
      p->flags |= FIO___HTTP1_FLAG_HOST_SEEN;
      p->flags &= ~FIO___HTTP1_FLAG_HOST_REQUIRED;
    }
    break;
  case 6: /* test for "expect" */
    if (!no_body && value.len == 12 &&
        fio_buf2u32u(name.buf) == fio_buf2u32u("expe") &&
        fio_buf2u32u(name.buf + 2) == fio_buf2u32u("pect")) {
      /* Expect value validation */
      if (fio_buf2u64u(value.buf) == fio_buf2u64u("100-cont") &&
          fio_buf2u32u(value.buf + 8) == fio_buf2u32u("inue")) {
        p->flags |= FIO___HTTP1_FLAG_EXPECT;
        return 0;
      }
      return -1;
    }
    break;
  case 14: /* test for "content-length" */
    if (fio_buf2u64u(name.buf) == fio_buf2u64u("content-") &&
        fio_buf2u64u(name.buf + 6) == fio_buf2u64u("t-length")) {
      if (!value.len)
        return -1;
      char *tmp = value.buf;
      errno = 0; /* reset errno before parsing */
      uint64_t clen = fio_stol10u(&tmp, value.buf + value.len);
      /* Reject if: parsing failed or trailing junk (tmp didn't reach end),
       * overflow occurred, value doesn't fit size_t (32-bit builds), or
       * value collides with sentinel values */
      if ((unsigned)(tmp != value.buf + value.len) | (errno == E2BIG) |
          ((uint64_t)(size_t)clen != clen) |
          (clen == FIO___HTTP1_BODY_NOT_ALLOWED) |
          (clen == FIO_HTTP1_EXPECTED_CHUNKED))
        return -1;
      if (no_body) /* report the value, never read a body */
        return 0 - (fio_http1_on_header_content_length(name,
                                                       value,
                                                       (size_t)clen,
                                                       udata) == -1);
      if (!clen) /* no length? */
        clen = FIO___HTTP1_BODY_NOT_ALLOWED;
      /* Prevent CL.TE / TE.CL by validating header's payload changes nothing */
      if (p->expected)
        return 0 - (p->expected != clen); /* causes parser to fail and stop */
      p->expected = clen;
      if (clen == FIO___HTTP1_BODY_NOT_ALLOWED)
        return 0;
      /* fio_http1_on_header_content_length tests if body length is too large */
      return 0 -
             (fio_http1_on_header_content_length(name, value, clen, udata) ==
              -1);
    }
    break;
  case 17: /* test for "transfer-encoding" (chunked?) */
    if (!no_body && (name.buf[16] == 'g') &&
        !((fio_buf2u64u(name.buf) ^ fio_buf2u64u("transfer")) |
          (fio_buf2u64u(name.buf + 8) ^ fio_buf2u64u("-encodin")))) {
      /* `chunked` MUST be the final coding, applied once (RFC 9112 §6.3,
       * §7). Field lines form a single list (RFC 9110 §5.3): a coding after
       * `chunked`, a repeated `chunked`, Content-Length (TE.CL) or a bodyless
       * method (GET / HEAD / OPTIONS) all leave `p->expected` set - reject.
       * A list that doesn't end with `chunked` can't be framed and is
       * rejected once the headers end, rather than misreading the body as
       * the next message (request smuggling). */
      if (p->expected)
        return -1;
      char *c_start = value.buf + value.len - (value.len >= 7 ? 7 : 0);
      if (value.len < 7 ||
          (fio_buf2u32u(c_start) | 0x20202020UL) != fio_buf2u32u("chun") ||
          (fio_buf2u32u(c_start + 3) | 0x20202020UL) !=
              fio_buf2u32u("nked") ||
          (value.len > 7 && c_start[-1] != ' ' && c_start[-1] != ',' &&
           c_start[-1] != '\t')) {
        if (fio_http1___te_has_chunked(value))
          return -1; /* `chunked` isn't the final coding */
        p->flags |= FIO___HTTP1_FLAG_TE_UNFRAMED;
        break; /* forward the codings, a later field line may end the list */
      }
      p->expected = FIO_HTTP1_EXPECTED_CHUNKED;
      p->flags &= ~FIO___HTTP1_FLAG_TE_UNFRAMED;
      /* endpoint does not need to know if the body was chunked or not */
      while (c_start > value.buf &&
             (c_start[-1] == ' ' || c_start[-1] == ',' || c_start[-1] == '\t'))
        --c_start;
      if (c_start == value.buf)
        return 0;
      value.len = c_start - value.buf;
      if (fio_http1___te_has_chunked(value))
        return -1;
      /* remaining codings (i.e., `gzip`) are forwarded for the endpoint to
       * decode (a 501 response is only a SHOULD, RFC 9112 §6.1). */
    }
    break;
  }
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

/** The subset of the forbidden chars that allows UTF-8 headers */
static const _Bool FIO___HTTP_FORBIDDEN_NAME_CHARS[256] = {
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* seeks to the ':' divisor while testing and converting to downcase. */
static char *fio_http1___seek_header_div(char *p) {
  FIO_ASSERT(FIO___HTTP_FORBIDDEN_NAME_CHARS[' '] &&
                 FIO___HTTP_FORBIDDEN_NAME_CHARS['\t'],
             "missing forbidden HTTP Header Name characters");
  for (;;) {
    if (FIO_UNLIKELY(FIO___HTTP_FORBIDDEN_NAME_CHARS[((uint8_t)(*p))]))
      return p;
    *p = fio_ct_tolower(*p);
    ++p;
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
    eol -= (eol != start && eol[-1] == '\r');
    if (FIO_UNLIKELY(eol == start))
      goto headers_finished;

    div = fio_http1___seek_header_div(start);
    if (div[0] != ':' || div == start)
      return -1;
    name = FIO_BUF_INFO2(start, (size_t)(div - start));
    do {
      ++div;
    } while (*div == ' ' || *div == '\t');

    if (div != eol)
      while (eol[-1] == ' ' || eol[-1] == '\t')
        --eol;
    value = FIO_BUF_INFO2((div == eol) ? NULL : div, (size_t)(eol - div));

    if (FIO_MEMCHR(value.buf, 0, value.len))
      return -1;
    int r = handler(p, name, value, udata);
    if (FIO_UNLIKELY(r))
      return r;
  }

headers_finished:
  if ((p->flags &
       (FIO___HTTP1_FLAG_HOST_REQUIRED | FIO___HTTP1_FLAG_TE_UNFRAMED)))
    return -1; /* no Host for HTTP/1.1 (RFC 9112 §3.2) or no final chunked */
  if ((p->flags & FIO___HTTP1_FLAG_NO_BODY))
    p->expected = 0; /* body framing ignored (i.e., skip_body set late) */
  if ((p->flags & FIO___HTTP1_FLAG_EXPECT)) {
    /* consume the flag (chunked trailers also finish through this path) */
    p->flags &= ~FIO___HTTP1_FLAG_EXPECT;
    /* `100 Continue` only matters when a body may follow (RFC 9110 §10.1.1) */
    if (p->expected && p->expected != FIO___HTTP1_BODY_NOT_ALLOWED &&
        fio_http1_on_expect(udata))
      goto expect_failed;
  }
  p->fn = (!p->expected || p->expected == FIO___HTTP1_BODY_NOT_ALLOWED)
              ? fio_http1___finish
          : (!(p->expected - FIO_HTTP1_EXPECTED_CHUNKED))
              ? fio_http1___read_body_chunked
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

  if (!FIO_MEMCHR(buf->buf, '\n', buf->len)) /* prevent read overflow */
    return (buf->len < 10) ? 1 : -1;

  char *eol = buf->buf;
  errno = 0;
  /* strict, bounded hex: no 0x prefix, no separators, junk rejected below */
  uint64_t expected64 = fio_stol16u(&eol, buf->buf + buf->len);
  if (eol == buf->buf || errno == E2BIG ||
      expected64 > 0x0FFFFFFF) /* cap expected */
    return -1;
  size_t expected = (size_t)expected64;
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
