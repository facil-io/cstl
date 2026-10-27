/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MULTIPART          /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          MIME Multipart Parser Module




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MULTIPART) && !defined(H___FIO_MULTIPART___H)
#define H___FIO_MULTIPART___H

/* *****************************************************************************
MIME Multipart Parser - Overview

A non-allocating, callback-based MIME multipart (multipart/form-data) parser.

Every part is reported once, complete, after its delimiter line was validated:
- a regular form field (no filename) calls `on_string`;
- a file (a filename attribute, even empty) calls `on_file`.

Values point into the parsed data (no copies) and are borrowed only during the
callback. A non-zero callback return value aborts parsing and is returned
unchanged in `err`.

The parser state (`fio_multipart_s`) holds a copy of the boundary and how far
the pending part was already searched, so data can be fed block by block
without searching the same bytes twice. Only one (incomplete) part needs to be
kept in memory: the bytes from `data + consumed`.

Multipart Format:
    --boundary\r\n
    Content-Disposition: form-data; name="field1"\r\n
    \r\n
    value1\r\n
    --boundary\r\n
    Content-Disposition: form-data; name="file1"; filename="test.txt"\r\n
    Content-Type: text/plain\r\n
    \r\n
    file content here\r\n
    --boundary--\r\n

Usage (all data available):
    static const fio_multipart_parser_callbacks_s callbacks = {
        .on_string = my_on_string,
        .on_file = my_on_file,
    };
    fio_multipart_s parser;
    if (fio_multipart_init(&parser, boundary))
      return -1; // invalid boundary
    fio_multipart_result_s r =
        fio_multipart_parse(&parser, &callbacks, udata, data, len, 1);
    if (r.err)
      return r.err; // malformed, truncated or a callback's error

Usage (block by block): keep `data + r.consumed ... data + len`, append the
next block and call again with `eof == 0` (`!r.consumed && !r.err` means more
data is required). Call with `eof == 1` once the input ends.
***************************************************************************** */

#ifndef FIO_MULTIPART_MAX_HEADER_SIZE
/** Maximum length of a part's headers (excluding the CRLFCRLF), also limiting
 * the spaces/tabs after a closing delimiter's two hyphens (which do not count
 * toward this limit). */
#define FIO_MULTIPART_MAX_HEADER_SIZE 8192
#endif

#ifndef FIO_MULTIPART_MAX_BOUNDARY
/** Maximum boundary length (RFC 2046 allows up to 70 characters). */
#define FIO_MULTIPART_MAX_BOUNDARY 250
#endif

/** Malformed or truncated input (callback errors are returned unchanged). */
#define FIO_MULTIPART_ERROR_GENERIC (-1)

/* *****************************************************************************
MIME Multipart Parser Types
***************************************************************************** */

/**
 * The MIME multipart parser callbacks (both optional: a NULL callback is
 * replaced by a no-op that skips its parts, which are still validated).
 *
 * All arguments are borrowed only during the callback. Returning non-zero
 * aborts parsing; the value is returned unchanged in `err`.
 */
typedef struct {
  /** A complete regular form field. A missing Content-Type has a NULL buf. */
  int (*on_string)(void *udata,
                   fio_buf_info_s key,
                   fio_buf_info_s value,
                   fio_buf_info_s content_type);
  /**
   * A complete file part (a `filename` attribute, possibly empty). `headers`
   * holds the raw part headers (without the terminating CRLFCRLF).
   */
  int (*on_file)(void *udata,
                 fio_buf_info_s key,
                 fio_buf_info_s filename,
                 fio_buf_info_s content_type,
                 fio_buf_info_s headers,
                 fio_buf_info_s data);
} fio_multipart_parser_callbacks_s;

/** The parser state, set by `fio_multipart_init`. All fields are private. */
typedef struct {
  /** Bytes of the pending part already searched. */
  size_t peek;
  /** Offset of the pending part's body (0 until its headers are complete). */
  size_t body;
  /** Parser state flags. */
  uint16_t flags;
  /** Length of `marker` ("\r\n--" + boundary). */
  uint16_t marker_len;
  /** The delimiter: "\r\n--" + boundary. */
  char marker[FIO_MULTIPART_MAX_BOUNDARY + 4];
} fio_multipart_s;

/** The MIME multipart parse result type. */
typedef struct {
  /** Bytes consumed (complete parts). Zero without an error: more data. */
  size_t consumed;
  /** 0, FIO_MULTIPART_ERROR_GENERIC or an unchanged callback error. */
  int err;
} fio_multipart_result_s;

/* *****************************************************************************
MIME Multipart Parser API
***************************************************************************** */

/**
 * Initializes (or resets) a parser for `boundary` (without the leading "--"),
 * copying the boundary.
 *
 * Returns -1 if the boundary is empty or longer than
 * `FIO_MULTIPART_MAX_BOUNDARY` (the parser then fails every call).
 */
SFUNC int fio_multipart_init(fio_multipart_s *parser, fio_buf_info_s boundary);

/**
 * Parses MIME multipart data, reporting each complete part.
 *
 * `data` must start at the first byte not yet consumed (unconsumed bytes are
 * kept and new data appended). Bytes already searched aren't searched again.
 *
 * `eof` marks the end of the input: a closing delimiter may end with the data
 * and anything incomplete is an error (truncation). Without `eof`, a result
 * with `!consumed && !err` means more data is required.
 *
 * After the closing delimiter, the rest of the data (the epilogue) is
 * consumed and ignored. After an error, every call fails.
 *
 * Not reentrant per `parser`: callbacks must not use the same `parser`.
 */
SFUNC fio_multipart_result_s
fio_multipart_parse(fio_multipart_s *parser,
                    const fio_multipart_parser_callbacks_s *callbacks,
                    void *udata,
                    const char *data,
                    size_t len,
                    int eof);

/* *****************************************************************************
MIME Multipart Parser - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Internal Helper: Find substring in buffer
***************************************************************************** */

FIO_SFUNC const char *fio___multipart_find(const char *haystack,
                                           size_t haystack_len,
                                           const char *needle,
                                           size_t needle_len) {
  if (!needle_len || needle_len > haystack_len)
    return NULL;
  const char *end = haystack + haystack_len - needle_len + 1;
  for (const char *p = haystack; p < end; ++p) {
    p = (const char *)FIO_MEMCHR(p, needle[0], (size_t)(end - p));
    if (!p)
      return NULL;
    if (!FIO_MEMCMP(p, needle, needle_len))
      return p;
  }
  return NULL;
}

/* *****************************************************************************
Internal Helper: Skip whitespace
***************************************************************************** */

FIO_IFUNC const char *fio___multipart_skip_ws(const char *p, const char *end) {
  while (p < end && (*p == ' ' || *p == '\t'))
    ++p;
  return p;
}

/* *****************************************************************************
Internal Helper: Skip optional whitespace and CRLF
***************************************************************************** */

FIO_IFUNC const char *fio___multipart_skip_lwsp(const char *p,
                                                const char *end) {
  while (p < end) {
    if (*p == ' ' || *p == '\t') {
      ++p;
      continue;
    }
    /* Check for folded header (CRLF followed by space/tab) */
    if (p + 2 < end && p[0] == '\r' && p[1] == '\n' &&
        (p[2] == ' ' || p[2] == '\t')) {
      p += 3;
      continue;
    }
    break;
  }
  return p;
}

/* *****************************************************************************
Internal Helper: Case-insensitive prefix match
***************************************************************************** */

FIO_SFUNC int fio___multipart_prefix_icase(const char *str,
                                           size_t str_len,
                                           const char *prefix,
                                           size_t prefix_len) {
  if (str_len < prefix_len)
    return 0;
  for (size_t i = 0; i < prefix_len; ++i) {
    char c1 = str[i];
    char c2 = prefix[i];
    /* Convert to lowercase */
    c1 |= (char)((uint8_t)(c1 >= 'A' && c1 <= 'Z') << 5);
    c2 |= (char)((uint8_t)(c2 >= 'A' && c2 <= 'Z') << 5);
    if (c1 != c2)
      return 0;
  }
  return 1;
}

/* *****************************************************************************
Internal Helper: Extract quoted or unquoted value from header parameter
***************************************************************************** */

FIO_SFUNC fio_buf_info_s fio___multipart_extract_param(const char *header,
                                                       size_t header_len,
                                                       const char *param_name,
                                                       size_t param_name_len) {
  fio_buf_info_s result = FIO_BUF_INFO0;
  const char *end = header + header_len;
  const char *p = header;

  /* Search for parameter name followed by '=' */
  while (p < end) {
    /* Find the parameter name */
    const char *found =
        fio___multipart_find(p, (size_t)(end - p), param_name, param_name_len);
    if (!found)
      return result;

    /* Check if it's at start or preceded by ; or whitespace */
    if (found > header) {
      char prev = found[-1];
      if (prev != ';' && prev != ' ' && prev != '\t') {
        p = found + 1;
        continue;
      }
    }

    /* Skip parameter name */
    p = found + param_name_len;
    p = fio___multipart_skip_ws(p, end);

    /* Expect '=' */
    if (p >= end || *p != '=') {
      continue;
    }
    ++p;
    p = fio___multipart_skip_ws(p, end);

    if (p >= end)
      return result;

    /* Extract value - quoted or unquoted */
    if (*p == '"') {
      /* Quoted value */
      ++p;
      const char *value_start = p;
      while (p < end && *p != '"') {
        if (*p == '\\' && p + 1 < end) {
          p += 2; /* Skip escaped character */
        } else {
          ++p;
        }
      }
      result.buf = (char *)value_start;
      result.len = (size_t)(p - value_start);
      return result;
    } else {
      /* Unquoted value - ends at ; or whitespace or end */
      const char *value_start = p;
      while (p < end && *p != ';' && *p != ' ' && *p != '\t' && *p != '\r' &&
             *p != '\n') {
        ++p;
      }
      result.buf = (char *)value_start;
      result.len = (size_t)(p - value_start);
      return result;
    }
  }

  return result;
}

/* *****************************************************************************
Internal Helper: Parse Content-Disposition header
***************************************************************************** */

typedef struct {
  fio_buf_info_s name;
  fio_buf_info_s filename;
} fio___multipart_disposition_s;

FIO_SFUNC fio___multipart_disposition_s
fio___multipart_parse_disposition(const char *value, size_t value_len) {
  fio___multipart_disposition_s result = {{0}, {0}};

  result.name = fio___multipart_extract_param(value, value_len, "name", 4);
  result.filename =
      fio___multipart_extract_param(value, value_len, "filename", 8);

  return result;
}

/* *****************************************************************************
Internal Helper: Find header value in part headers
***************************************************************************** */

FIO_SFUNC fio_buf_info_s fio___multipart_find_header(const char *headers,
                                                     size_t headers_len,
                                                     const char *header_name,
                                                     size_t header_name_len) {
  fio_buf_info_s result = FIO_BUF_INFO0;
  const char *p = headers;
  const char *end = headers + headers_len;

  while (p < end) {
    /* Find end of current line */
    const char *line_end = (const char *)FIO_MEMCHR(p, '\n', (size_t)(end - p));
    if (!line_end)
      line_end = end;

    /* Calculate line length (excluding \r\n) */
    size_t line_len = (size_t)(line_end - p);
    if (line_len > 0 && p[line_len - 1] == '\r')
      --line_len;

    /* Check if this line starts with the header name */
    if (fio___multipart_prefix_icase(p,
                                     line_len,
                                     header_name,
                                     header_name_len)) {
      const char *value_start = p + header_name_len;
      /* Skip optional whitespace after colon */
      if (value_start < p + line_len && *value_start == ':') {
        ++value_start;
        value_start = fio___multipart_skip_ws(value_start, p + line_len);
        result.buf = (char *)value_start;
        result.len = (size_t)((p + line_len) - value_start);
        return result;
      }
    }

    /* Move to next line (never past `end`) */
    if (line_end == end)
      break;
    p = line_end + 1;
  }

  return result;
}

/* *****************************************************************************
Internal Helper: Classify the delimiter line after a boundary
***************************************************************************** */

/**
 * Classifies the bytes at `pos` (right after a delimiter) without side effects.
 *
 * Grammar (RFC 2046): delimiter CRLF | delimiter "--" [padding] [CRLF epilogue]
 *
 * Returns 1 if another part follows, 0 for a closing delimiter line ending in
 * CRLF (the epilogue is ignored), 2 for a closing delimiter (and padding)
 * reaching the end of the data, -2 if more data is needed or -1 on malformed
 * input. On 1, 0 or 2, `*pnext` points past the consumed delimiter line.
 */
FIO_SFUNC int fio___multipart_delimiter_line(const char *pos,
                                             const char *end,
                                             const char **pnext) {
  if ((size_t)(end - pos) < 2)
    return -2;
  if (pos[0] == '\r' && pos[1] == '\n') {
    *pnext = pos + 2;
    return 1;
  }
  if (pos[0] != '-' || pos[1] != '-')
    return -1;
  const char *padding = pos + 2;
  pos = fio___multipart_skip_ws(padding, end);
  if ((size_t)(pos - padding) > (size_t)FIO_MULTIPART_MAX_HEADER_SIZE)
    return -1;
  *pnext = pos;
  if (pos == end) /* the line may continue (padding / CRLF) */
    return 2;
  if (*pos != '\r')
    return -1;
  if (pos + 1 == end)
    return -2;
  if (pos[1] != '\n')
    return -1;
  *pnext = pos + 2;
  return 0;
}

/* Parser states (bit flags) */
#define FIO___MULTIPART_S_PART  1 /* the first delimiter line was parsed */
#define FIO___MULTIPART_S_DONE  2 /* the closing delimiter line was parsed */
#define FIO___MULTIPART_S_ERROR 4 /* failed (every call fails) */

/**
 * Classifies the delimiter line at `pos` for the parser (see
 * `fio___multipart_delimiter_line`). At `eof`, a closing line may end with the
 * data and anything incomplete is malformed.
 *
 * Returns 1 if another part follows, 0 for the closing line, -2 if more data
 * is needed or -1 on malformed input. On 1 or 0, `*pnext` points past the
 * line.
 */
FIO_SFUNC int fio___multipart_line(const char *pos,
                                   const char *end,
                                   int eof,
                                   const char **pnext) {
  int line = fio___multipart_delimiter_line(pos, end, pnext);
  if (line == 2) { /* closing line (and padding) reaching the end of data */
    return eof ? 0 : -2;
  }
  if (line == -2 && eof)
    return -1;
  return line;
}

/** Reports a complete part: headers at `part` (`body - 4` bytes long), the
 * body from `part + body` to `body_end`. Returns the callback's value. */
FIO_SFUNC int fio___multipart_report(const fio_multipart_parser_callbacks_s *cb,
                                     void *udata,
                                     const char *part,
                                     size_t body,
                                     const char *body_end) {
  size_t headers_len = body - 4;
  fio_buf_info_s disposition_value =
      fio___multipart_find_header(part, headers_len, "Content-Disposition", 19);
  fio___multipart_disposition_s disposition =
      fio___multipart_parse_disposition(disposition_value.buf,
                                        disposition_value.len);
  fio_buf_info_s content_type =
      fio___multipart_find_header(part, headers_len, "Content-Type", 12);
  fio_buf_info_s value =
      FIO_BUF_INFO2((char *)part + body, (size_t)(body_end - (part + body)));
  if (disposition.filename.buf)
    return cb->on_file(udata,
                       disposition.name,
                       disposition.filename,
                       content_type,
                       FIO_BUF_INFO2((char *)part, headers_len),
                       value);
  return cb->on_string(udata, disposition.name, value, content_type);
}

/** Default `on_string`: skips the field. */
FIO_SFUNC int fio___multipart_on_string_noop(void *udata,
                                             fio_buf_info_s key,
                                             fio_buf_info_s value,
                                             fio_buf_info_s content_type) {
  (void)udata, (void)key, (void)value, (void)content_type;
  return 0;
}

/** Default `on_file`: skips the file. */
FIO_SFUNC int fio___multipart_on_file_noop(void *udata,
                                           fio_buf_info_s key,
                                           fio_buf_info_s filename,
                                           fio_buf_info_s content_type,
                                           fio_buf_info_s headers,
                                           fio_buf_info_s data) {
  (void)udata, (void)key, (void)filename, (void)content_type, (void)headers,
      (void)data;
  return 0;
}

/* *****************************************************************************
MIME Multipart Parser - API
***************************************************************************** */

SFUNC int fio_multipart_init(fio_multipart_s *p, fio_buf_info_s boundary) {
  if (!p)
    return -1;
  p->peek = 0;
  p->body = 0;
  p->flags = 0;
  p->marker_len = 0;
  if (!boundary.buf || !boundary.len ||
      boundary.len > (size_t)FIO_MULTIPART_MAX_BOUNDARY) {
    p->flags = FIO___MULTIPART_S_ERROR;
    return -1;
  }
  p->marker[0] = '\r';
  p->marker[1] = '\n';
  p->marker[2] = '-';
  p->marker[3] = '-';
  FIO_MEMCPY(p->marker + 4, boundary.buf, boundary.len);
  p->marker_len = (uint16_t)(boundary.len + 4);
  return 0;
}

SFUNC fio_multipart_result_s
fio_multipart_parse(fio_multipart_s *p,
                    const fio_multipart_parser_callbacks_s *callbacks,
                    void *udata,
                    const char *data,
                    size_t len,
                    int eof) {
  fio_multipart_result_s r = {.consumed = 0, .err = 0};
  if (!p || (p->flags & FIO___MULTIPART_S_ERROR) || !p->marker_len ||
      (!data && len)) {
    if (p)
      p->flags = FIO___MULTIPART_S_ERROR;
    r.err = FIO_MULTIPART_ERROR_GENERIC;
    return r;
  }
  if ((p->flags & FIO___MULTIPART_S_DONE)) { /* the epilogue is ignored */
    r.consumed = len;
    return r;
  }
  /* fill defaults once (a private copy), so callbacks are never tested */
  fio_multipart_parser_callbacks_s cb = {0};
  if (callbacks)
    cb = callbacks[0];
  if (!cb.on_string)
    cb.on_string = fio___multipart_on_string_noop;
  if (!cb.on_file)
    cb.on_file = fio___multipart_on_file_noop;
  if (!data) /* no data (len == 0): avoid NULL pointer arithmetic */
    data = (const char *)&cb;

  const char *pos = data; /* the pending part (or first delimiter) */
  const char *end = data + len;
  const char *next = NULL;
  const size_t mlen = p->marker_len;
  size_t peek = p->peek; /* offsets are relative to `pos` */
  size_t body = p->body;
  int line;
  p->peek = 0;
  p->body = 0;
  if (peek > len || body > len) /* contract violation (tail not kept) */
    peek = body = 0;

  if (!(p->flags & FIO___MULTIPART_S_PART)) {
    /* The first delimiter: [CRLF] "--" boundary, then its line */
    const char *d = data;
    const size_t dlen = mlen - 2;
    if (len && d[0] == '\r') {
      if (len < 2)
        goto more;
      if (d[1] != '\n')
        goto error;
      d += 2;
    }
    if ((size_t)(end - d) < dlen) { /* reject a mismatch early */
      if (FIO_MEMCMP(d, p->marker + 2, (size_t)(end - d)))
        goto error;
      goto more;
    }
    if (FIO_MEMCMP(d, p->marker + 2, dlen))
      goto error;
    line = fio___multipart_line(d + dlen, end, eof, &next);
    if (line == -2)
      goto more;
    if (line == -1)
      goto error;
    if (!line)
      goto done;
    p->flags |= FIO___MULTIPART_S_PART;
    pos = next;
  }

  for (;;) {
    const size_t avail = (size_t)(end - pos);
    if (!body) { /* find the end of the headers, within the size limit */
      const size_t limit = (size_t)FIO_MULTIPART_MAX_HEADER_SIZE + 4;
      const size_t span = (avail < limit) ? avail : limit;
      const size_t from = (peek > 3) ? (peek - 3) : 0;
      const char *headers_end =
          (from < span)
              ? fio___multipart_find(pos + from, span - from, "\r\n\r\n", 4)
              : NULL;
      if (!headers_end) {
        if (avail >= limit) /* part headers are too long */
          goto error;
        peek = avail;
        goto more;
      }
      body = (size_t)(headers_end + 4 - pos);
      peek = body;
      if (!fio___multipart_find_header(pos, body - 4, "Content-Disposition", 19)
               .buf)
        goto error;
    }
    /* find the delimiter ending the body, resuming the previous search */
    const size_t from = (peek >= body + mlen - 1) ? (peek - (mlen - 1)) : body;
    const char *delimiter =
        fio___multipart_find(pos + from, avail - from, p->marker, mlen);
    if (!delimiter) {
      peek = avail;
      goto more;
    }
    line = fio___multipart_line(delimiter + mlen, end, eof, &next);
    if (line == -2) { /* the delimiter is found again (no rescan) */
      peek = (size_t)(delimiter - pos);
      goto more;
    }
    if (line == -1) {
      pos = delimiter;
      goto error;
    }
    /* the part is complete and its delimiter line valid: report it */
    r.err = fio___multipart_report(&cb, udata, pos, body, delimiter);
    if (r.err) {
      p->flags = FIO___MULTIPART_S_ERROR;
      r.consumed = (size_t)(pos - data);
      return r;
    }
    if (!line)
      goto done;
    pos = next;
    peek = body = 0;
  }

more:
  if (eof)
    goto error;
  p->peek = peek;
  p->body = body;
  r.consumed = (size_t)(pos - data);
  return r;

done:
  p->flags = FIO___MULTIPART_S_PART | FIO___MULTIPART_S_DONE;
  r.consumed = len; /* the epilogue is ignored */
  return r;

error:
  p->flags = FIO___MULTIPART_S_ERROR;
  r.err = FIO_MULTIPART_ERROR_GENERIC;
  r.consumed = (size_t)(pos - data); /* where parsing failed */
  return r;
}

/* *****************************************************************************
MIME Multipart Parser - Cleanup
***************************************************************************** */
#undef FIO___MULTIPART_S_PART
#undef FIO___MULTIPART_S_DONE
#undef FIO___MULTIPART_S_ERROR
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MULTIPART
#endif /* FIO_MULTIPART */
