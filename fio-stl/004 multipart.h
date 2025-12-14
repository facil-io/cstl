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

This is a non-allocating, streaming callback-based MIME multipart parser
implementing the multipart/form-data format as used in HTTP file uploads.

The parser uses callbacks to handle form fields and file uploads:
- Regular form fields (no filename) trigger on_field callback
- File uploads trigger on_file_start, on_file_data, on_file_end callbacks

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

Usage:
    static const fio_multipart_parser_callbacks_s callbacks = {
        .on_field = my_on_field,
        .on_file_start = my_on_file_start,
        .on_file_data = my_on_file_data,
        .on_file_end = my_on_file_end,
    };

    fio_multipart_result_s result = fio_multipart_parse(
        &callbacks,
        my_context,
        FIO_BUF_INFO2(boundary_str, boundary_len),
        data,
        data_len);

    if (result.err == 0) {
        // Success - all data parsed
    } else if (result.err == -2) {
        // Need more data - call again with more data
    } else {
        // Error
    }

***************************************************************************** */

/* *****************************************************************************
MIME Multipart Parser Types
***************************************************************************** */

/** The MIME multipart parser callbacks. */
typedef struct {
  /**
   * Called for each regular form field (no filename).
   * Returns user-defined context (can be NULL).
   */
  void *(*on_field)(void *udata,
                    fio_buf_info_s name,
                    fio_buf_info_s value,
                    fio_buf_info_s content_type);

  /**
   * Called when a file upload starts.
   * Returns context for this file (passed to on_file_data/on_file_end).
   */
  void *(*on_file_start)(void *udata,
                         fio_buf_info_s name,
                         fio_buf_info_s filename,
                         fio_buf_info_s content_type);

  /**
   * Called with file data chunk.
   * May be called multiple times per file for streaming.
   * Returns non-zero to abort parsing.
   */
  int (*on_file_data)(void *udata, void *file_ctx, fio_buf_info_s data);

  /**
   * Called when file upload ends.
   */
  void (*on_file_end)(void *udata, void *file_ctx);

  /**
   * Called on parse error (optional).
   */
  void (*on_error)(void *udata);

} fio_multipart_parser_callbacks_s;

/** The MIME multipart parse result type. */
typedef struct {
  /** Number of bytes consumed from the input buffer. */
  size_t consumed;
  /** Number of form fields parsed. */
  size_t field_count;
  /** Number of files parsed. */
  size_t file_count;
  /** Error code: 0 = success, -1 = error, -2 = need more data. */
  int err;
} fio_multipart_result_s;

/* *****************************************************************************
MIME Multipart Parser API
***************************************************************************** */

/**
 * Parse MIME multipart data.
 *
 * `callbacks` contains the callback functions (should be static const).
 * `udata` is user data passed to all callbacks.
 * `boundary` is the multipart boundary string (without leading "--").
 * `data` is the data to parse.
 * `len` is the length of the data.
 *
 * Returns a result struct containing:
 * - `consumed`: Number of bytes consumed from the buffer
 * - `field_count`: Number of form fields parsed
 * - `file_count`: Number of files parsed
 * - `err`: 0 = success, -1 = error, -2 = need more data
 *
 * For streaming, call again with remaining data appended to unconsumed data.
 */
SFUNC fio_multipart_result_s
fio_multipart_parse(const fio_multipart_parser_callbacks_s *callbacks,
                    void *udata,
                    fio_buf_info_s boundary,
                    const char *data,
                    size_t len);

/* *****************************************************************************
MIME Multipart Parser - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Internal Helper: No-op callbacks
***************************************************************************** */

FIO_IFUNC void *fio___multipart_noop_field(void *udata,
                                           fio_buf_info_s name,
                                           fio_buf_info_s value,
                                           fio_buf_info_s content_type) {
  (void)udata;
  (void)name;
  (void)value;
  (void)content_type;
  return NULL;
}

FIO_IFUNC void *fio___multipart_noop_file_start(void *udata,
                                                fio_buf_info_s name,
                                                fio_buf_info_s filename,
                                                fio_buf_info_s content_type) {
  (void)udata;
  (void)name;
  (void)filename;
  (void)content_type;
  return NULL;
}

FIO_IFUNC int fio___multipart_noop_file_data(void *udata,
                                             void *file_ctx,
                                             fio_buf_info_s data) {
  (void)udata;
  (void)file_ctx;
  (void)data;
  return 0;
}

FIO_IFUNC void fio___multipart_noop_file_end(void *udata, void *file_ctx) {
  (void)udata;
  (void)file_ctx;
}

FIO_IFUNC void fio___multipart_noop_error(void *udata) { (void)udata; }

/* *****************************************************************************
Internal: Validated callbacks wrapper
***************************************************************************** */

typedef struct {
  void *(*on_field)(void *udata,
                    fio_buf_info_s name,
                    fio_buf_info_s value,
                    fio_buf_info_s content_type);
  void *(*on_file_start)(void *udata,
                         fio_buf_info_s name,
                         fio_buf_info_s filename,
                         fio_buf_info_s content_type);
  int (*on_file_data)(void *udata, void *file_ctx, fio_buf_info_s data);
  void (*on_file_end)(void *udata, void *file_ctx);
  void (*on_error)(void *udata);
} fio___multipart_cb_s;

FIO_SFUNC fio___multipart_cb_s
fio___multipart_callbacks_validate(const fio_multipart_parser_callbacks_s *cb) {
  fio___multipart_cb_s r;
  static const fio_multipart_parser_callbacks_s empty_cb = {0};
  if (!cb)
    cb = &empty_cb;
  r.on_field = cb->on_field ? cb->on_field : fio___multipart_noop_field;
  r.on_file_start =
      cb->on_file_start ? cb->on_file_start : fio___multipart_noop_file_start;
  r.on_file_data =
      cb->on_file_data ? cb->on_file_data : fio___multipart_noop_file_data;
  r.on_file_end =
      cb->on_file_end ? cb->on_file_end : fio___multipart_noop_file_end;
  r.on_error = cb->on_error ? cb->on_error : fio___multipart_noop_error;
  return r;
}

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
    if (c1 >= 'A' && c1 <= 'Z')
      c1 += 32;
    if (c2 >= 'A' && c2 <= 'Z')
      c2 += 32;
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

    /* Move to next line */
    p = line_end + 1;
  }

  return result;
}

/* *****************************************************************************
MIME Multipart Main Parse Function
***************************************************************************** */

SFUNC fio_multipart_result_s
fio_multipart_parse(const fio_multipart_parser_callbacks_s *callbacks,
                    void *udata,
                    fio_buf_info_s boundary,
                    const char *data,
                    size_t len) {
  fio_multipart_result_s result = {.consumed = 0,
                                   .field_count = 0,
                                   .file_count = 0,
                                   .err = 0};

  if (!data || !len || !boundary.buf || !boundary.len) {
    result.err = -1;
    return result;
  }

  fio___multipart_cb_s cb = fio___multipart_callbacks_validate(callbacks);

  const char *pos = data;
  const char *end = data + len;

  /* Build boundary markers:
   * - delimiter: "\r\n--" + boundary
   * - first_delimiter: "--" + boundary (at start of data)
   * - close_delimiter: "\r\n--" + boundary + "--"
   */
  char delimiter_buf[256 + 6]; /* "--" + boundary + "\r\n" + "--" + NUL */
  char *first_delimiter = delimiter_buf;
  char *delimiter = delimiter_buf + 2;
  size_t first_delimiter_len = boundary.len + 2;
  size_t delimiter_len = boundary.len + 4;

  if (boundary.len > 250) {
    result.err = -1;
    cb.on_error(udata);
    return result;
  }

  /* Build first delimiter: "--" + boundary */
  first_delimiter[0] = '-';
  first_delimiter[1] = '-';
  FIO_MEMCPY(first_delimiter + 2, boundary.buf, boundary.len);

  /* Build delimiter: "\r\n--" + boundary */
  delimiter[0] = '\r';
  delimiter[1] = '\n';
  /* delimiter + 2 already points to "--" + boundary from first_delimiter */

  /* Check for initial boundary */
  if ((size_t)(end - pos) < first_delimiter_len) {
    result.err = -2; /* Need more data */
    return result;
  }

  /* First boundary may or may not have leading CRLF */
  if (pos[0] == '\r' && pos[1] == '\n') {
    pos += 2;
  }

  if ((size_t)(end - pos) < first_delimiter_len ||
      FIO_MEMCMP(pos, first_delimiter, first_delimiter_len) != 0) {
    result.err = -1;
    cb.on_error(udata);
    return result;
  }

  pos += first_delimiter_len;

  /* Check for immediate close (empty multipart) */
  if ((size_t)(end - pos) >= 2 && pos[0] == '-' && pos[1] == '-') {
    /* Empty multipart - just the closing boundary */
    pos += 2;
    /* Skip optional trailing CRLF */
    if ((size_t)(end - pos) >= 2 && pos[0] == '\r' && pos[1] == '\n')
      pos += 2;
    result.consumed = (size_t)(pos - data);
    return result;
  }

  /* Expect CRLF after boundary */
  if ((size_t)(end - pos) < 2) {
    result.err = -2;
    return result;
  }
  if (pos[0] != '\r' || pos[1] != '\n') {
    result.err = -1;
    cb.on_error(udata);
    return result;
  }
  pos += 2;

  /* Parse parts */
  while (pos < end) {
    /* Find end of headers (double CRLF) */
    const char *headers_end =
        fio___multipart_find(pos, (size_t)(end - pos), "\r\n\r\n", 4);
    if (!headers_end) {
      result.err = -2; /* Need more data */
      result.consumed = (size_t)((pos - 2 - delimiter_len) - data);
      if (result.consumed > len)
        result.consumed = 0;
      return result;
    }

    const char *headers_start = pos;
    size_t headers_len = (size_t)(headers_end - headers_start);
    const char *body_start = headers_end + 4;

    /* Parse Content-Disposition header */
    fio_buf_info_s disposition_value =
        fio___multipart_find_header(headers_start,
                                    headers_len,
                                    "Content-Disposition",
                                    19);

    if (!disposition_value.buf) {
      result.err = -1;
      cb.on_error(udata);
      return result;
    }

    fio___multipart_disposition_s disposition =
        fio___multipart_parse_disposition(disposition_value.buf,
                                          disposition_value.len);

    /* Parse Content-Type header (optional) */
    fio_buf_info_s content_type = fio___multipart_find_header(headers_start,
                                                              headers_len,
                                                              "Content-Type",
                                                              12);

    /* Find the next boundary to determine body end */
    const char *next_boundary = fio___multipart_find(body_start,
                                                     (size_t)(end - body_start),
                                                     delimiter,
                                                     delimiter_len);

    if (!next_boundary) {
      /* Check if we might have partial data */
      /* We need at least delimiter_len bytes after body to be sure */
      result.err = -2; /* Need more data */
      /* Rewind to before this part's headers */
      result.consumed = (size_t)((pos - 2 - delimiter_len) - data);
      if (result.consumed > len)
        result.consumed = 0;
      return result;
    }

    /* Body is from body_start to next_boundary */
    size_t body_len = (size_t)(next_boundary - body_start);

    /* Determine if this is a file or field */
    if (disposition.filename.buf && disposition.filename.len > 0) {
      /* File upload */
      void *file_ctx = cb.on_file_start(udata,
                                        disposition.name,
                                        disposition.filename,
                                        content_type);

      /* Send file data */
      fio_buf_info_s file_data = FIO_BUF_INFO2((char *)body_start, body_len);
      int abort = cb.on_file_data(udata, file_ctx, file_data);

      cb.on_file_end(udata, file_ctx);

      if (abort) {
        result.err = -1;
        result.consumed = (size_t)(next_boundary - data);
        return result;
      }

      ++result.file_count;
    } else {
      /* Regular form field */
      fio_buf_info_s value = FIO_BUF_INFO2((char *)body_start, body_len);
      cb.on_field(udata, disposition.name, value, content_type);
      ++result.field_count;
    }

    /* Move past the boundary */
    pos = next_boundary + delimiter_len;

    /* Check for closing boundary (--) or continuation (CRLF) */
    if ((size_t)(end - pos) < 2) {
      result.err = -2; /* Need more data */
      result.consumed = (size_t)(next_boundary - data);
      return result;
    }

    if (pos[0] == '-' && pos[1] == '-') {
      /* Closing boundary */
      pos += 2;
      /* Skip optional trailing CRLF */
      if ((size_t)(end - pos) >= 2 && pos[0] == '\r' && pos[1] == '\n')
        pos += 2;
      result.consumed = (size_t)(pos - data);
      return result;
    }

    if (pos[0] != '\r' || pos[1] != '\n') {
      result.err = -1;
      cb.on_error(udata);
      result.consumed = (size_t)(pos - data);
      return result;
    }

    pos += 2; /* Skip CRLF, continue to next part */
  }

  /* Reached end of data without finding closing boundary */
  result.err = -2;
  return result;
}

/* *****************************************************************************
MIME Multipart Parser - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MULTIPART
#endif /* FIO_MULTIPART */
