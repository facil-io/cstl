/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_RESP3              /* Development inclusion - ignore line */
#define FIO_ATOL               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                RESP 3 Parser Module




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_RESP3) && !defined(FIO___RECURSIVE_INCLUDE) &&                 \
    !defined(H___FIO_RESP3___H)
#define H___FIO_RESP3___H

/* *****************************************************************************
RESP3 Parser - Overview

This is a non-allocating, streaming callback-based RESP3 parser implementing
the full RESP3 specification.

The parser uses a context-stack pattern similar to the JSON parser, where:
- Primitive callbacks return the created object as `void*`
- Container callbacks receive parent context and return new context
- Push callbacks add children to containers
- The parser manages the context stack internally

RESP3 Types Supported:
- Simple types: +, -, :, _, ,, #, (
- Blob types: $, !, =
- Aggregate types: *, %, ~, >, |
- Streaming: $?, *?, %?, ~?

Usage:
    static const fio_resp3_callbacks_s callbacks = {
        .on_null = my_on_null,
        .on_number = my_on_number,
        .on_string = my_on_string,
        .on_array = my_on_array,
        .array_push = my_array_push,
        // ... other callbacks ...
    };

    fio_resp3_parser_s parser = {.udata = my_context};
    fio_resp3_result_s result = fio_resp3_parse(&parser, &callbacks, buf, len);
    if (result.err) { handle_error(); }
    // result.obj is the parsed top-level object
    // result.consumed indicates bytes consumed

***************************************************************************** */

/* *****************************************************************************
RESP3 Parser Settings
***************************************************************************** */

/** The maximum number of nested layers in object responses (2...32,768) */
#ifndef FIO_RESP3_MAX_NESTING
#define FIO_RESP3_MAX_NESTING 32
#endif

/**
 * Fixed-length blob strings (`$<len>`, `!<len>`, `=<len>`) larger than this
 * threshold are streamed incrementally when the callback table provides the
 * streaming string callbacks (on_start_string / on_string_write /
 * on_string_done): the parser starts the string as soon as its header is
 * parsed and feeds data in chunks as it arrives (`result.consumed` advances
 * per chunk), instead of waiting for the whole blob to be contiguous in the
 * read window. This allows blobs larger than any consumer read buffer.
 *
 * Blobs at or below the threshold keep the wait-for-contiguity behavior
 * (single zero-copy write, even across split reads). Callback tables without
 * streaming callbacks are unaffected (always wait-for-contiguity).
 */
#ifndef FIO_RESP3_STREAM_THRESHOLD
#define FIO_RESP3_STREAM_THRESHOLD 4096
#endif

/* *****************************************************************************
RESP3 Type Constants
***************************************************************************** */

/* Single Line Types */
/** Simple String: `+<string>\r\n` */
#define FIO_RESP3_SIMPLE_STR '+'
/** Simple Error: `-<string>\r\n` */
#define FIO_RESP3_SIMPLE_ERR '-'
/** Number: `:<number>\r\n` */
#define FIO_RESP3_NUMBER ':'
/** Null: `_\r\n` */
#define FIO_RESP3_NULL '_'
/** Double: `,<floating-point-number>\r\n` */
#define FIO_RESP3_DOUBLE ','
/** Boolean: `#t\r\n` or `#f\r\n` */
#define FIO_RESP3_BOOL '#'
/** Big Number: `(<big number>\r\n` */
#define FIO_RESP3_BIGNUM '('

/* Blob Types */
/** Blob String: `$<length>\r\n<bytes>\r\n` */
#define FIO_RESP3_BLOB_STR '$'
/** Blob Error: `!<length>\r\n<bytes>\r\n` */
#define FIO_RESP3_BLOB_ERR '!'
/** Verbatim String: `=<length>\r\n<type:><bytes>\r\n` */
#define FIO_RESP3_VERBATIM '='

/* Aggregate Types */
/** Array: `*<count>\r\n...elements...` */
#define FIO_RESP3_ARRAY '*'
/** Map: `%<count>\r\n...key-value pairs...` */
#define FIO_RESP3_MAP '%'
/** Set: `~<count>\r\n...elements...` */
#define FIO_RESP3_SET '~'
/** Push: `><count>\r\n...elements...` */
#define FIO_RESP3_PUSH '>'
/** Attribute: `|<count>\r\n...key-value pairs...` */
#define FIO_RESP3_ATTR '|'

/* Streaming */
/** Streamed string chunk: `;` */
#define FIO_RESP3_STREAM_CHUNK ';'
/** Streamed aggregate end: `.` */
#define FIO_RESP3_STREAM_END '.'

/* *****************************************************************************
RESP3 Parser Types
***************************************************************************** */

/** Parser frame for tracking nested structures */
typedef struct {
  /** Context for this container (returned by on_array/on_map/etc) */
  void *ctx;
  /** For maps: the pending key waiting for its value */
  void *key;
  /** Expected remaining elements */
  int64_t remaining;
  /** Type of this frame */
  uint8_t type;
  /** Is this a streaming type? */
  uint8_t streaming;
  /** For maps: are we expecting a key (0) or value (1)? */
  uint8_t expecting_value;
  /** Set treated as map: need to duplicate values as key+value */
  uint8_t set_as_map;
} fio_resp3_frame_s;

/** RESP3 parser state */
typedef struct {
  /** User data passed to all callbacks */
  void *udata;
  /** Current nesting depth */
  uint32_t depth;
  /** Protocol error flag */
  uint8_t error;
  /** Streaming string in progress flag */
  uint8_t streaming_string;
  /** Streaming string type (FIO_RESP3_BLOB_STR, FIO_RESP3_BLOB_ERR, etc.) */
  uint8_t streaming_string_type;
  /** Streamed blob or chunk: trailing CRLF bytes pending (0...2) */
  uint8_t streaming_blob_crlf;
  /** Non-zero for `$?`: complete each chunk without completing the string. */
  uint8_t streaming_chunked;
  /** Context for streaming string (from on_start_string) */
  void *streaming_string_ctx;
  /**
   * Data bytes remaining in the current fixed-length blob or `$?` chunk.
   * Zero when inactive or awaiting the next chunk header.
   */
  int64_t streaming_remaining;
  /** Stack for nested structures */
  fio_resp3_frame_s stack[FIO_RESP3_MAX_NESTING];
} fio_resp3_parser_s;

/**
 * The RESP3 parser callbacks (designed to be static const).
 *
 * All callbacks receive `udata` from the parser state as their first argument.
 */
typedef struct {
  /* ===== Primitive Callbacks - return the created object ===== */

  /** Called when NULL (`_`) is received. Returns new object. */
  void *(*on_null)(void *udata);

  /** Called when Boolean (`#t` or `#f`) is received. Returns new object. */
  void *(*on_bool)(void *udata, int is_true);

  /** Called when a Number (`:`) is parsed. Returns new object. */
  void *(*on_number)(void *udata, int64_t num);

  /** Called when a Double (`,`) is parsed. Returns new object. */
  void *(*on_double)(void *udata, double num);

  /** Called when a Big Number (`(`) is parsed. Returns new object. */
  void *(*on_bignum)(void *udata, const void *data, size_t len);

  /**
   * Called when a complete String is received.
   * `type` is FIO_RESP3_SIMPLE_STR, FIO_RESP3_BLOB_STR, or FIO_RESP3_VERBATIM.
   * Returns new object.
   */
  void *(*on_string)(void *udata, const void *data, size_t len, uint8_t type);

  /**
   * Called when an error message is received (simple `-` or blob `!`).
   * `type` is FIO_RESP3_SIMPLE_ERR or FIO_RESP3_BLOB_ERR.
   * Returns new object.
   */
  void *(*on_error)(void *udata, const void *data, size_t len, uint8_t type);

  /* ===== Container Callbacks - receive parent ctx, return new ctx ===== */

  /** Called when an Array starts. Returns new array context. */
  void *(*on_array)(void *udata, void *parent_ctx, int64_t len);

  /** Called when a Map starts. Returns new map context. */
  void *(*on_map)(void *udata, void *parent_ctx, int64_t len);

  /** Called when a Set starts. Returns new set context. */
  void *(*on_set)(void *udata, void *parent_ctx, int64_t len);

  /** Called when a Push message starts. Returns new push context. */
  void *(*on_push)(void *udata, void *parent_ctx, int64_t len);

  /** Called when an Attribute starts. Returns new attribute context. */
  void *(*on_attr)(void *udata, void *parent_ctx, int64_t len);

  /* ===== Push Callbacks - add child to container ===== */

  /** Add value to array. Returns non-zero on error. */
  int (*array_push)(void *udata, void *ctx, void *value);

  /** Add key-value pair to map. Returns non-zero on error. */
  int (*map_push)(void *udata, void *ctx, void *key, void *value);

  /** Add value to set. Returns non-zero on error. */
  int (*set_push)(void *udata, void *ctx, void *value);

  /** Add value to push message. Returns non-zero on error. */
  int (*push_push)(void *udata, void *ctx, void *value);

  /** Add key-value pair to attribute. Returns non-zero on error. */
  int (*attr_push)(void *udata, void *ctx, void *key, void *value);

  /* ===== Done Callbacks (optional) - finalize container ===== */

  /** Called when array is complete. Returns final object. */
  void *(*array_done)(void *udata, void *ctx);

  /** Called when map is complete. Returns final object. */
  void *(*map_done)(void *udata, void *ctx);

  /** Called when set is complete. Returns final object. */
  void *(*set_done)(void *udata, void *ctx);

  /** Called when push is complete. Returns final object. */
  void *(*push_done)(void *udata, void *ctx);

  /** Called when attribute is complete. Returns final object. */
  void *(*attr_done)(void *udata, void *ctx);

  /* ===== Error Handling ===== */

  /** Free an unused object (e.g., orphaned key on error). */
  void (*free_unused)(void *udata, void *obj);

  /** Called on protocol error. */
  void *(*on_error_protocol)(void *udata);

  /* ===== Streaming String Callbacks (optional) ===== */

  /**
   * Called when a blob string starts (before data arrives).
   * `len` is the declared length of the string ((size_t)-1 for streaming).
   * `type` is FIO_RESP3_BLOB_STR, FIO_RESP3_BLOB_ERR, or FIO_RESP3_VERBATIM.
   * Returns a context for the string being built (e.g., a string buffer).
   * If NULL is returned, falls back to buffering and calling on_string when
   * complete.
   */
  void *(*on_start_string)(void *udata, size_t len, uint8_t type);

  /**
   * Called with partial string data (may be called multiple times).
   * `ctx` is the context returned by on_start_string.
   * Returns 0 on success, non-zero to abort parsing.
   */
  int (*on_string_write)(void *udata, void *ctx, const void *data, size_t len);

  /**
   * Called when the string is complete.
   * `ctx` is the context returned by on_start_string.
   * Returns the final string object to be used as a value.
   */
  void *(*on_string_done)(void *udata, void *ctx, uint8_t type);

} fio_resp3_callbacks_s;

/** The RESP3 parse result type. */
typedef struct {
  /** The parsed top-level object (or NULL on error/incomplete) */
  void *obj;
  /** Number of bytes consumed from the buffer */
  size_t consumed;
  /** Non-zero if an error occurred */
  int err;
} fio_resp3_result_s;

/* *****************************************************************************
RESP3 Parser API
***************************************************************************** */

/**
 * Parse RESP3 data from buffer.
 *
 * `parser` is the parser state. Initialize with `{.udata = my_data}` for first
 *          call. For continuation after partial parse, pass the same parser.
 * `callbacks` contains the callback functions (should be static const).
 * `buf` is the data to parse.
 * `len` is the length of the data.
 *
 * Returns a result struct containing:
 * - `obj`: The parsed top-level object (NULL if incomplete or error)
 * - `consumed`: Number of bytes consumed from the buffer
 * - `err`: Non-zero if a protocol error occurred
 *
 * For partial data, the parser state is preserved. Call again with remaining
 * data appended to unconsumed data.
 */
SFUNC fio_resp3_result_s fio_resp3_parse(fio_resp3_parser_s *parser,
                                         const fio_resp3_callbacks_s *callbacks,
                                         const void *buf,
                                         size_t len);

/* *****************************************************************************
RESP3 Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Internal Helper: No-op callbacks
***************************************************************************** */

FIO_IFUNC void *fio___resp3_noop_obj(void *udata) {
  return (void *)(uintptr_t)1; /* Return non-NULL sentinel */
  (void)udata;
}

FIO_IFUNC void *fio___resp3_noop_bool(void *udata, int v) {
  return (void *)(uintptr_t)1;
  (void)udata;
  (void)v;
}

FIO_IFUNC void *fio___resp3_noop_i64(void *udata, int64_t v) {
  return (void *)(uintptr_t)1;
  (void)udata;
  (void)v;
}

FIO_IFUNC void *fio___resp3_noop_dbl(void *udata, double v) {
  return (void *)(uintptr_t)1;
  (void)udata;
  (void)v;
}

FIO_IFUNC void *fio___resp3_noop_data(void *udata,
                                      const void *d,
                                      size_t l,
                                      uint8_t t) {
  return (void *)(uintptr_t)1;
  (void)udata;
  (void)d;
  (void)l;
  (void)t;
}

FIO_IFUNC void *fio___resp3_noop_bignum(void *udata, const void *d, size_t l) {
  return (void *)(uintptr_t)1;
  (void)udata;
  (void)d;
  (void)l;
}

FIO_IFUNC void *fio___resp3_noop_container(void *udata,
                                           void *parent,
                                           int64_t len) {
  return (void *)(uintptr_t)1;
  (void)udata;
  (void)parent;
  (void)len;
}

FIO_IFUNC int fio___resp3_noop_push(void *udata, void *ctx, void *value) {
  return 0;
  (void)udata;
  (void)ctx;
  (void)value;
}

FIO_IFUNC int fio___resp3_noop_push_kv(void *udata,
                                       void *ctx,
                                       void *key,
                                       void *value) {
  return 0;
  (void)udata;
  (void)ctx;
  (void)key;
  (void)value;
}

FIO_IFUNC void *fio___resp3_noop_done(void *udata, void *ctx) {
  return ctx;
  (void)udata;
}

FIO_IFUNC void fio___resp3_noop_free(void *udata, void *obj) {
  (void)udata;
  (void)obj;
}

FIO_IFUNC void *fio___resp3_noop_error(void *udata) {
  return NULL;
  (void)udata;
}

/* *****************************************************************************
Internal: Validated callbacks wrapper
***************************************************************************** */

typedef struct {
  void *(*on_null)(void *udata);
  void *(*on_bool)(void *udata, int is_true);
  void *(*on_number)(void *udata, int64_t num);
  void *(*on_double)(void *udata, double num);
  void *(*on_bignum)(void *udata, const void *data, size_t len);
  void *(*on_string)(void *udata, const void *data, size_t len, uint8_t type);
  void *(*on_error)(void *udata, const void *data, size_t len, uint8_t type);
  void *(*on_array)(void *udata, void *parent_ctx, int64_t len);
  void *(*on_map)(void *udata, void *parent_ctx, int64_t len);
  void *(*on_set)(void *udata, void *parent_ctx, int64_t len);
  void *(*on_push)(void *udata, void *parent_ctx, int64_t len);
  void *(*on_attr)(void *udata, void *parent_ctx, int64_t len);
  int (*array_push)(void *udata, void *ctx, void *value);
  int (*map_push)(void *udata, void *ctx, void *key, void *value);
  int (*set_push)(void *udata, void *ctx, void *value);
  int (*push_push)(void *udata, void *ctx, void *value);
  int (*attr_push)(void *udata, void *ctx, void *key, void *value);
  void *(*array_done)(void *udata, void *ctx);
  void *(*map_done)(void *udata, void *ctx);
  void *(*set_done)(void *udata, void *ctx);
  void *(*push_done)(void *udata, void *ctx);
  void *(*attr_done)(void *udata, void *ctx);
  void (*free_unused)(void *udata, void *obj);
  void *(*on_error_protocol)(void *udata);
  /* Streaming string callbacks (NULL if not provided) */
  void *(*on_start_string)(void *udata, size_t len, uint8_t type);
  int (*on_string_write)(void *udata, void *ctx, const void *data, size_t len);
  void *(*on_string_done)(void *udata, void *ctx, uint8_t type);
  /** Flag: treat sets as maps (when set callbacks are missing) */
  uint8_t set_as_map;
} fio___resp3_cb_s;

FIO_SFUNC fio___resp3_cb_s
fio___resp3_callbacks_validate(const fio_resp3_callbacks_s *cb) {
  fio___resp3_cb_s r;
  static const fio_resp3_callbacks_s empty_cb = {0};
  if (!cb) {
    cb = &empty_cb;
  }
  r.on_null = cb->on_null ? cb->on_null : fio___resp3_noop_obj;
  r.on_bool = cb->on_bool ? cb->on_bool : fio___resp3_noop_bool;
  r.on_number = cb->on_number ? cb->on_number : fio___resp3_noop_i64;
  r.on_double = cb->on_double ? cb->on_double : fio___resp3_noop_dbl;
  r.on_bignum = cb->on_bignum ? cb->on_bignum : fio___resp3_noop_bignum;
  r.on_string = cb->on_string ? cb->on_string : fio___resp3_noop_data;
  r.on_error = cb->on_error ? cb->on_error : fio___resp3_noop_data;
  r.on_array = cb->on_array ? cb->on_array : fio___resp3_noop_container;
  r.on_map = cb->on_map ? cb->on_map : fio___resp3_noop_container;
  r.on_set = cb->on_set ? cb->on_set : fio___resp3_noop_container;
  r.on_push = cb->on_push ? cb->on_push : fio___resp3_noop_container;
  r.on_attr = cb->on_attr ? cb->on_attr : fio___resp3_noop_container;
  r.array_push = cb->array_push ? cb->array_push : fio___resp3_noop_push;
  r.map_push = cb->map_push ? cb->map_push : fio___resp3_noop_push_kv;
  r.set_push = cb->set_push ? cb->set_push : fio___resp3_noop_push;
  r.push_push = cb->push_push ? cb->push_push : fio___resp3_noop_push;
  r.attr_push = cb->attr_push ? cb->attr_push : fio___resp3_noop_push_kv;
  r.array_done = cb->array_done ? cb->array_done : fio___resp3_noop_done;
  r.map_done = cb->map_done ? cb->map_done : fio___resp3_noop_done;
  r.set_done = cb->set_done ? cb->set_done : fio___resp3_noop_done;
  r.push_done = cb->push_done ? cb->push_done : fio___resp3_noop_done;
  r.attr_done = cb->attr_done ? cb->attr_done : fio___resp3_noop_done;
  r.free_unused = cb->free_unused ? cb->free_unused : fio___resp3_noop_free;
  r.on_error_protocol =
      cb->on_error_protocol ? cb->on_error_protocol : fio___resp3_noop_error;
  /* Streaming string callbacks - keep NULL if not provided (no fallback) */
  r.on_start_string = cb->on_start_string;
  r.on_string_write = cb->on_string_write;
  r.on_string_done = cb->on_string_done;
  /* Treat sets as maps when set callbacks are missing but map callbacks exist
   */
  r.set_as_map = (!cb->on_set && !cb->set_push && !cb->set_done) &&
                 (cb->on_map || cb->map_push || cb->map_done);
  return r;
}

/* *****************************************************************************
Internal Helper: Find newline
***************************************************************************** */

FIO_IFUNC const uint8_t *fio___resp3_find_eol(const uint8_t *pos,
                                              const uint8_t *end) {
  if (pos >= end)
    return NULL;
  const uint8_t *nl =
      (const uint8_t *)FIO_MEMCHR(pos, '\n', (size_t)(end - pos));
  return nl;
}

/* *****************************************************************************
Internal Helper: Parse integer from buffer
***************************************************************************** */

FIO_IFUNC int64_t fio___resp3_parse_int(const uint8_t **pos,
                                        const uint8_t *eol) {
  /* Validate the complete field, including a digit after an optional sign. */
  const uint8_t *digits = *pos;
  errno = 0;
  if (digits < eol && (*digits == '-' || *digits == '+'))
    ++digits;
  char *cursor = (char *)*pos;
  int64_t n = fio_stol10(&cursor, (const char *)eol);
  *pos = (const uint8_t *)cursor;
  if (!errno && (digits == eol || *pos != eol))
    errno = EINVAL;
  return n;
}

/* *****************************************************************************
Internal Helper: Parse double from buffer
***************************************************************************** */

FIO_IFUNC double fio___resp3_parse_double(const uint8_t *start,
                                          const uint8_t *eol) {
  const uint8_t *p = start;
  errno = 0;
  if (p < eol && (*p == '-' || *p == '+'))
    ++p;
  if ((size_t)(eol - p) == 3) {
    /* Exact specials; retain case-insensitive/-nan support for Redis < 7.2. */
    if (fio_ct_tolower(p[0]) == 'i' && fio_ct_tolower(p[1]) == 'n' &&
        fio_ct_tolower(p[2]) == 'f')
      return (*start == '-') ? -INFINITY : INFINITY;
    if (fio_ct_tolower(p[0]) == 'n' && fio_ct_tolower(p[1]) == 'a' &&
        fio_ct_tolower(p[2]) == 'n')
      return NAN;
  }
  const uint8_t *digits = p;
  while (p < eol && (uint8_t)(*p - '0') < 10)
    ++p;
  if (p == digits)
    goto invalid;
  if (p < eol && *p == '.') {
    digits = ++p;
    while (p < eol && (uint8_t)(*p - '0') < 10)
      ++p;
    if (p == digits)
      goto invalid;
  }
  if (p < eol && (*p == 'e' || *p == 'E')) {
    ++p;
    if (p < eol && (*p == '-' || *p == '+'))
      ++p;
    digits = p;
    while (p < eol && (uint8_t)(*p - '0') < 10)
      ++p;
    if (p == digits)
      goto invalid;
  }
  if (p != eol)
    goto invalid;
  /* The validated decimal is followed by CRLF, which bounds fio_atof. */
  char *cursor = (char *)start;
  double n = fio_atof(&cursor);
  if ((const uint8_t *)cursor != eol)
    goto invalid;
  return n;
invalid:
  errno = EINVAL;
  return 0;
}

/* *****************************************************************************
Internal Helper: Get parent context
***************************************************************************** */

FIO_IFUNC void *fio___resp3_parent_ctx(fio_resp3_parser_s *p) {
  if (p->depth == 0)
    return NULL;
  return p->stack[p->depth - 1].ctx;
}

/* *****************************************************************************
Internal Helper: Push value to current container
***************************************************************************** */

FIO_SFUNC int fio___resp3_push_value(fio_resp3_parser_s *p,
                                     fio___resp3_cb_s *cb,
                                     void *value) {
  if (p->depth == 0)
    return 0;

  fio_resp3_frame_s *f = &p->stack[p->depth - 1];

  switch (f->type) {
  case FIO_RESP3_ARRAY: return cb->array_push(p->udata, f->ctx, value);
  case FIO_RESP3_SET:
    /* When set_as_map is enabled, treat set elements as map key=value pairs */
    if (f->set_as_map)
      return cb->map_push(p->udata, f->ctx, value, value);
    return cb->set_push(p->udata, f->ctx, value);
  case FIO_RESP3_PUSH: return cb->push_push(p->udata, f->ctx, value);
  case FIO_RESP3_MAP:
  case FIO_RESP3_ATTR:
    if (!f->expecting_value) {
      f->key = value;
      f->expecting_value = 1;
      return 0;
    } else {
      void *key = f->key;
      f->key = NULL;
      f->expecting_value = 0;
      if (f->type == FIO_RESP3_MAP)
        return cb->map_push(p->udata, f->ctx, key, value);
      else
        return cb->attr_push(p->udata, f->ctx, key, value);
    }
  default: return 0;
  }
}

/* *****************************************************************************
Internal Helper: Complete container and pop from stack
***************************************************************************** */

FIO_SFUNC void *fio___resp3_complete_frame(fio_resp3_parser_s *p,
                                           fio___resp3_cb_s *cb) {
  fio_resp3_frame_s *f = &p->stack[p->depth - 1];
  void *result = f->ctx;

  switch (f->type) {
  case FIO_RESP3_ARRAY: result = cb->array_done(p->udata, f->ctx); break;
  case FIO_RESP3_MAP: result = cb->map_done(p->udata, f->ctx); break;
  case FIO_RESP3_SET:
    /* When set_as_map is enabled, use map_done instead of set_done */
    if (f->set_as_map)
      result = cb->map_done(p->udata, f->ctx);
    else
      result = cb->set_done(p->udata, f->ctx);
    break;
  case FIO_RESP3_PUSH: result = cb->push_done(p->udata, f->ctx); break;
  case FIO_RESP3_ATTR: result = cb->attr_done(p->udata, f->ctx); break;
  default: break;
  }

  if (f->key) {
    cb->free_unused(p->udata, f->key);
    f->key = NULL;
  }

  --p->depth;
  return result;
}

/* *****************************************************************************
Internal Helper: Handle value completion and container unwinding
***************************************************************************** */

FIO_SFUNC void *fio___resp3_on_value(fio_resp3_parser_s *p,
                                     fio___resp3_cb_s *cb,
                                     void *value) {
  /* Push to parent if nested */
  if (p->depth > 0) {
    if (fio___resp3_push_value(p, cb, value)) {
      p->error = 1;
      return NULL;
    }
  }

  /* Unwind completed containers */
  while (p->depth > 0) {
    fio_resp3_frame_s *f = &p->stack[p->depth - 1];

    if (f->streaming)
      return NULL; /* Wait for end marker */

    if (f->remaining > 0)
      --f->remaining;

    if (f->remaining > 0)
      return NULL; /* More elements expected */

    /* Frame complete */
    uint8_t type = f->type;
    value = fio___resp3_complete_frame(p, cb);

    /* Attributes annotate the next value; neither advance nor finish the
     * parent when their metadata completes, even inside another attribute. */
    if (type == FIO_RESP3_ATTR)
      return NULL;

    /* Push completed container to parent */
    if (p->depth > 0) {
      if (fio___resp3_push_value(p, cb, value)) {
        p->error = 1;
        return NULL;
      }
    }
  }

  return value;
}

/* *****************************************************************************
Internal Helper: Reject nesting before a callback allocates an unowned context
***************************************************************************** */

FIO_IFUNC int fio___resp3_can_push_frame(fio_resp3_parser_s *p,
                                         fio___resp3_cb_s *cb) {
  if (p->depth < FIO_RESP3_MAX_NESTING)
    return 1;
  p->error = 1;
  cb->on_error_protocol(p->udata);
  return 0;
}

/* *****************************************************************************
Internal Helper: Push new frame
***************************************************************************** */

FIO_IFUNC int fio___resp3_push_frame(fio_resp3_parser_s *p,
                                     fio___resp3_cb_s *cb,
                                     uint8_t type,
                                     void *ctx,
                                     int64_t count,
                                     int streaming,
                                     int set_as_map) {
  if (p->depth >= FIO_RESP3_MAX_NESTING) {
    p->error = 1;
    cb->on_error_protocol(p->udata);
    return -1;
  }

  fio_resp3_frame_s *f = &p->stack[p->depth];
  f->type = type;
  f->ctx = ctx;
  f->key = NULL;
  f->remaining = count;
  f->streaming = (uint8_t)streaming;
  f->expecting_value = 0;
  f->set_as_map = (uint8_t)set_as_map;
  ++p->depth;
  return 0;
}

/* *****************************************************************************
Internal Helper: Start incremental streaming of a fixed-length blob
***************************************************************************** */

/**
 * Starts incremental streaming for a fixed-length blob string (`$`, `!`, `=`)
 * whose declared length exceeds FIO_RESP3_STREAM_THRESHOLD.
 *
 * Returns non-zero if streaming started (parser state was updated and the
 * caller must `continue` the parse loop), zero if the caller should fall
 * back to the buffered (wait-for-contiguity) path.
 *
 * Streaming is skipped when callbacks are absent, the blob is small, or
 * on_start_string declines the string. Chunked strings are handled separately.
 */
FIO_SFUNC int fio___resp3_stream_blob_start(fio_resp3_parser_s *p,
                                            fio___resp3_cb_s *cb,
                                            size_t blob_len,
                                            uint8_t type) {
  if (!cb->on_start_string || !cb->on_string_write || !cb->on_string_done ||
      blob_len <= FIO_RESP3_STREAM_THRESHOLD || p->streaming_string)
    return 0;
  void *ctx = cb->on_start_string(p->udata, blob_len, type);
  if (!ctx)
    return 0;
  p->streaming_string = 1;
  p->streaming_string_type = type;
  p->streaming_string_ctx = ctx;
  p->streaming_remaining = (int64_t)blob_len;
  p->streaming_blob_crlf = 2;
  return 1;
}

/* *****************************************************************************
RESP3 Main Parse Function
***************************************************************************** */

SFUNC fio_resp3_result_s fio_resp3_parse(fio_resp3_parser_s *parser,
                                         const fio_resp3_callbacks_s *callbacks,
                                         const void *buf,
                                         size_t len) {
  fio_resp3_result_s result = {.obj = NULL, .consumed = 0, .err = 0};
  const uint8_t *pos = (const uint8_t *)buf;
  const uint8_t *start = pos;
  const uint8_t *end = pos + len;
  void *obj = NULL;

  if (!parser) {
    FIO_LOG_ERROR("RESP3 parser: parser state is NULL");
    result.err = 1;
    return result;
  }

  fio___resp3_cb_s cb = fio___resp3_callbacks_validate(callbacks);

  if (parser->error) {
    result.err = 1;
    return result;
  }

  while (pos < end) {
    /* Fixed blob or chunk continuation: drain arbitrary bytes without an
     * EOL scan or buffering, even when one chunk exceeds the read window. */
    if (parser->streaming_remaining) {
      size_t avail = (size_t)(end - pos);
      size_t chunk = (avail < (size_t)parser->streaming_remaining)
                         ? avail
                         : (size_t)parser->streaming_remaining;
      if (cb.on_string_write(parser->udata,
                             parser->streaming_string_ctx,
                             pos,
                             chunk)) {
        parser->error = 1;
        result.err = 1;
        goto done;
      }
      pos += chunk;
      parser->streaming_remaining -= (int64_t)chunk;
      result.consumed = (size_t)(pos - start);
      if (parser->streaming_remaining)
        break; /* need more data */
    }
    if (parser->streaming_blob_crlf) {
      /* Validate trailing CRLF, preserving progress across split calls. */
      while (parser->streaming_blob_crlf && pos < end) {
        if ((parser->streaming_blob_crlf == 2 && *pos == '\r') ||
            (parser->streaming_blob_crlf == 1 && *pos == '\n')) {
          ++pos;
          --parser->streaming_blob_crlf;
        } else {
          goto protocol_error;
        }
      }
      result.consumed = (size_t)(pos - start);
      if (parser->streaming_blob_crlf)
        break; /* need more data */
      if (parser->streaming_chunked)
        continue; /* Chunk complete, but only ;0 completes the string. */
      /* Streamed fixed-length blob complete */
      obj = cb.on_string_done(parser->udata,
                              parser->streaming_string_ctx,
                              parser->streaming_string_type);
      parser->streaming_string = 0;
      parser->streaming_string_ctx = NULL;
      parser->streaming_string_type = 0;
      obj = fio___resp3_on_value(parser, &cb, obj);
      if (parser->error) {
        result.err = 1;
        goto done;
      }
      if (obj && parser->depth == 0) {
        result.obj = obj;
        goto done;
      }
      continue;
    }

    /* Between chunks, no other RESP token may replace the active context. */
    if (parser->streaming_string && *pos != FIO_RESP3_STREAM_CHUNK)
      goto protocol_error;

    const uint8_t *eol = fio___resp3_find_eol(pos, end);
    if (!eol)
      break; /* Need more data */
    if (eol - pos < 2 || eol[-1] != '\r')
      goto protocol_error;
    --eol; /* All field parsers use the exclusive end before CRLF. */
    uint8_t type = *pos++;

    switch (type) {
    /* ===== Line-oriented values ===== */
    case FIO_RESP3_SIMPLE_STR:
    case FIO_RESP3_SIMPLE_ERR:
      if (FIO_MEMCHR(pos, '\r', (size_t)(eol - pos)))
        goto protocol_error;
      obj = (type == FIO_RESP3_SIMPLE_STR)
                ? cb.on_string(parser->udata, pos, (size_t)(eol - pos), type)
                : cb.on_error(parser->udata, pos, (size_t)(eol - pos), type);
      goto line_value;
    case FIO_RESP3_NUMBER: {
      int64_t n = fio___resp3_parse_int(&pos, eol);
      if (FIO_UNLIKELY(errno))
        goto protocol_error;
      obj = cb.on_number(parser->udata, n);
      goto line_value;
    }
    case FIO_RESP3_NULL:
      if (pos != eol)
        goto protocol_error;
      obj = cb.on_null(parser->udata);
      goto line_value;
    case FIO_RESP3_DOUBLE: {
      double n = fio___resp3_parse_double(pos, eol);
      if (FIO_UNLIKELY(errno))
        goto protocol_error;
      obj = cb.on_double(parser->udata, n);
      goto line_value;
    }
    case FIO_RESP3_BOOL:
      if (eol - pos != 1 || (*pos != 't' && *pos != 'f'))
        goto protocol_error;
      obj = cb.on_bool(parser->udata, *pos == 't');
      goto line_value;
    case FIO_RESP3_BIGNUM: {
      const uint8_t *digits = pos;
      if (digits < eol && (*digits == '-' || *digits == '+'))
        ++digits;
      if (digits == eol)
        goto protocol_error;
      for (const uint8_t *d = digits; d < eol; ++d)
        if ((uint8_t)(*d - '0') > 9)
          goto protocol_error;
      obj = cb.on_bignum(parser->udata, pos, (size_t)(eol - pos));
      goto line_value;
    }
    line_value:
      pos = eol + 2;
      obj = fio___resp3_on_value(parser, &cb, obj);
      if (parser->error) {
        result.err = 1;
        goto done;
      }
      if (obj && parser->depth == 0) {
        result.obj = obj;
        goto done;
      }
      break;

    /* ===== Length-prefixed strings share size and framing validation ===== */
    case FIO_RESP3_BLOB_STR:
    case FIO_RESP3_BLOB_ERR:
    case FIO_RESP3_VERBATIM: {
      if (type == FIO_RESP3_BLOB_STR && eol - pos == 1 && *pos == '?') {
        /* Unknown-length strings require the entire callback trio. */
        if (!cb.on_start_string || !cb.on_string_write || !cb.on_string_done)
          goto protocol_error;
        void *ctx = cb.on_start_string(parser->udata, (size_t)-1, type);
        if (!ctx)
          goto protocol_error;
        parser->streaming_string = 1;
        parser->streaming_chunked = 1;
        parser->streaming_string_type = type;
        parser->streaming_string_ctx = ctx;
        pos = eol + 2;
        break;
      }
      int64_t declared = fio___resp3_parse_int(&pos, eol);
      if (FIO_UNLIKELY(errno))
        goto protocol_error;
      pos = eol + 2;
      if (declared < 0) {
        /* Only RESP2's null bulk-string marker is valid here. */
        if (type != FIO_RESP3_BLOB_STR || declared != -1)
          goto protocol_error;
        obj = cb.on_null(parser->udata);
        goto blob_value;
      }
      /* Check the wire length BEFORE narrowing or invoking callbacks. */
      if ((uint64_t)declared > (uint64_t)(SIZE_MAX - 2))
        goto protocol_error;
      size_t blob_len = (size_t)declared;
      if (type == FIO_RESP3_VERBATIM) {
        if (blob_len < 4)
          goto protocol_error;
        /* Retain only the small header until its format prefix is available;
         * no extra parser state or copy is needed across a split prefix. */
        if ((size_t)(end - pos) < 4) {
          pos = start + result.consumed;
          goto done;
        }
        if (pos[3] != ':' || pos[0] < '!' || pos[0] > '~' || pos[1] < '!' ||
            pos[1] > '~' || pos[2] < '!' || pos[2] > '~')
          goto protocol_error;
      }
      if (fio___resp3_stream_blob_start(parser, &cb, blob_len, type))
        continue;
      if ((size_t)(end - pos) < blob_len + 2) {
        pos = start + result.consumed;
        goto done;
      }
      if (pos[blob_len] != '\r' || pos[blob_len + 1] != '\n')
        goto protocol_error;

      if (cb.on_start_string && cb.on_string_write && cb.on_string_done) {
        void *ctx = cb.on_start_string(parser->udata, blob_len, type);
        if (ctx) {
          /* Register before writes: the caller can clean up even if a write
           * fails (the same ownership as an incremental string). */
          parser->streaming_string = 1;
          parser->streaming_string_ctx = ctx;
          parser->streaming_string_type = type;
          if (cb.on_string_write(parser->udata, ctx, pos, blob_len)) {
            parser->error = 1;
            result.err = 1;
            goto done;
          }
          obj = cb.on_string_done(parser->udata, ctx, type);
          parser->streaming_string = 0;
          parser->streaming_string_ctx = NULL;
          parser->streaming_string_type = 0;
          goto blob_consumed;
        }
      }
      obj = (type == FIO_RESP3_BLOB_ERR)
                ? cb.on_error(parser->udata, pos, blob_len, type)
                : cb.on_string(parser->udata, pos, blob_len, type);
    blob_consumed:
      pos += blob_len + 2;
    blob_value:
      obj = fio___resp3_on_value(parser, &cb, obj);
      if (parser->error) {
        result.err = 1;
        goto done;
      }
      if (obj && parser->depth == 0) {
        result.obj = obj;
        goto done;
      }
      break;
    }

    /* ===== Array: *<count>\r\n ===== */
    case FIO_RESP3_ARRAY: {
      int streaming = 0;
      int64_t count;

      if ((size_t)(eol - pos) == 1 && *pos == '?') {
        streaming = 1;
        count = -1;
      } else {
        count = fio___resp3_parse_int(&pos, eol);
        if (FIO_UNLIKELY(errno)) {
          parser->error = 1;
          result.err = 1;
          cb.on_error_protocol(parser->udata);
          goto done;
        }
      }
      pos = eol + 2;

      if (count < -1 && !streaming) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }
      if (count == -1 && !streaming) {
        obj = cb.on_null(parser->udata);
        obj = fio___resp3_on_value(parser, &cb, obj);
        if (parser->error) {
          result.err = 1;
          goto done;
        }
        if (obj && parser->depth == 0) {
          result.obj = obj;
          goto done;
        }
        break;
      }

      if (count != 0 && !fio___resp3_can_push_frame(parser, &cb)) {
        result.err = 1;
        goto done;
      }
      void *parent = fio___resp3_parent_ctx(parser);
      void *ctx = cb.on_array(parser->udata, parent, count);

      if (count == 0) {
        obj = cb.array_done(parser->udata, ctx);
        obj = fio___resp3_on_value(parser, &cb, obj);
        if (parser->error) {
          result.err = 1;
          goto done;
        }
        if (obj && parser->depth == 0) {
          result.obj = obj;
          goto done;
        }
        break;
      }

      if (fio___resp3_push_frame(parser,
                                 &cb,
                                 FIO_RESP3_ARRAY,
                                 ctx,
                                 count,
                                 streaming,
                                 0)) {
        result.err = 1;
        goto done;
      }
      break;
    }

    /* ===== Map: %<count>\r\n ===== */
    case FIO_RESP3_MAP: {
      int streaming = 0;
      int64_t count;

      if ((size_t)(eol - pos) == 1 && *pos == '?') {
        streaming = 1;
        count = -1;
      } else {
        count = fio___resp3_parse_int(&pos, eol);
        if (FIO_UNLIKELY(errno)) {
          parser->error = 1;
          result.err = 1;
          cb.on_error_protocol(parser->udata);
          goto done;
        }
      }
      pos = eol + 2;

      if (count < 0 && !streaming) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }
      /* Count is pairs: validate before invoking potentially allocating
       * callbacks, and before attempting to register a frame. */
      if (count > (INT64_MAX >> 1) ||
          (count != 0 && !fio___resp3_can_push_frame(parser, &cb))) {
        if (!parser->error) {
          parser->error = 1;
          cb.on_error_protocol(parser->udata);
        }
        result.err = 1;
        goto done;
      }

      void *parent = fio___resp3_parent_ctx(parser);
      void *ctx = cb.on_map(parser->udata, parent, count);

      if (count == 0) {
        obj = cb.map_done(parser->udata, ctx);
        obj = fio___resp3_on_value(parser, &cb, obj);
        if (parser->error) {
          result.err = 1;
          goto done;
        }
        if (obj && parser->depth == 0) {
          result.obj = obj;
          goto done;
        }
        break;
      }

      /* Maps: count is pairs, need count*2 elements. */
      int64_t elements = streaming ? -1 : count * 2;
      if (fio___resp3_push_frame(parser,
                                 &cb,
                                 FIO_RESP3_MAP,
                                 ctx,
                                 elements,
                                 streaming,
                                 0)) {
        result.err = 1;
        goto done;
      }
      break;
    }

    /* ===== Set: ~<count>\r\n ===== */
    case FIO_RESP3_SET: {
      int streaming = 0;
      int64_t count;

      if ((size_t)(eol - pos) == 1 && *pos == '?') {
        streaming = 1;
        count = -1;
      } else {
        count = fio___resp3_parse_int(&pos, eol);
        if (FIO_UNLIKELY(errno)) {
          parser->error = 1;
          result.err = 1;
          cb.on_error_protocol(parser->udata);
          goto done;
        }
      }
      pos = eol + 2;

      if (count < 0 && !streaming) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }
      if (count != 0 && !fio___resp3_can_push_frame(parser, &cb)) {
        result.err = 1;
        goto done;
      }

      void *parent = fio___resp3_parent_ctx(parser);
      /* When set_as_map is enabled, use on_map instead of on_set */
      void *ctx = cb.set_as_map ? cb.on_map(parser->udata, parent, count)
                                : cb.on_set(parser->udata, parent, count);

      if (count == 0) {
        /* When set_as_map is enabled, use map_done instead of set_done */
        obj = cb.set_as_map ? cb.map_done(parser->udata, ctx)
                            : cb.set_done(parser->udata, ctx);
        obj = fio___resp3_on_value(parser, &cb, obj);
        if (parser->error) {
          result.err = 1;
          goto done;
        }
        if (obj && parser->depth == 0) {
          result.obj = obj;
          goto done;
        }
        break;
      }

      if (fio___resp3_push_frame(parser,
                                 &cb,
                                 FIO_RESP3_SET,
                                 ctx,
                                 count,
                                 streaming,
                                 cb.set_as_map)) {
        result.err = 1;
        goto done;
      }
      break;
    }

    /* ===== Push: ><count>\r\n ===== */
    case FIO_RESP3_PUSH: {
      int64_t count = fio___resp3_parse_int(&pos, eol);
      if (FIO_UNLIKELY(errno)) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }
      pos = eol + 2;

      if (count < 0 ||
          (count != 0 && !fio___resp3_can_push_frame(parser, &cb))) {
        if (!parser->error) {
          parser->error = 1;
          cb.on_error_protocol(parser->udata);
        }
        result.err = 1;
        goto done;
      }

      void *parent = fio___resp3_parent_ctx(parser);
      void *ctx = cb.on_push(parser->udata, parent, count);

      if (count == 0) {
        obj = cb.push_done(parser->udata, ctx);
        obj = fio___resp3_on_value(parser, &cb, obj);
        if (parser->error) {
          result.err = 1;
          goto done;
        }
        if (obj && parser->depth == 0) {
          result.obj = obj;
          goto done;
        }
        break;
      }

      if (fio___resp3_push_frame(parser,
                                 &cb,
                                 FIO_RESP3_PUSH,
                                 ctx,
                                 count,
                                 0,
                                 0)) {
        result.err = 1;
        goto done;
      }
      break;
    }

    /* ===== Attribute: |<count>\r\n ===== */
    case FIO_RESP3_ATTR: {
      int64_t count = fio___resp3_parse_int(&pos, eol);
      if (FIO_UNLIKELY(errno)) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }
      pos = eol + 2;

      if (count < 0 || count > (INT64_MAX >> 1) ||
          (count != 0 && !fio___resp3_can_push_frame(parser, &cb))) {
        if (!parser->error) {
          parser->error = 1;
          cb.on_error_protocol(parser->udata);
        }
        result.err = 1;
        goto done;
      }

      void *parent = fio___resp3_parent_ctx(parser);
      void *ctx = cb.on_attr(parser->udata, parent, count);

      if (count == 0) {
        cb.attr_done(parser->udata, ctx);
        break;
      }

      /* Attributes: count is pairs, need count*2 elements. */
      if (fio___resp3_push_frame(parser,
                                 &cb,
                                 FIO_RESP3_ATTR,
                                 ctx,
                                 count * 2,
                                 0,
                                 0)) {
        result.err = 1;
        goto done;
      }
      break;
    }

    /* ===== Streamed string chunk: ;<length>\r\n<bytes>\r\n ===== */
    case FIO_RESP3_STREAM_CHUNK: {
      /* This is only valid when we're in a streaming string */
      if (!parser->streaming_string) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }

      int64_t chunk_len = fio___resp3_parse_int(&pos, eol);
      pos = eol + 2;

      if (FIO_UNLIKELY(errno)) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }

      /* Length 0 means end of streaming string */
      if (chunk_len == 0) {
        obj = cb.on_string_done(parser->udata,
                                parser->streaming_string_ctx,
                                parser->streaming_string_type);
        parser->streaming_string = 0;
        parser->streaming_chunked = 0;
        parser->streaming_string_ctx = NULL;
        parser->streaming_string_type = 0;
        obj = fio___resp3_on_value(parser, &cb, obj);
        if (parser->error) {
          result.err = 1;
          goto done;
        }
        if (obj && parser->depth == 0) {
          result.obj = obj;
          goto done;
        }
        break;
      }

      /* Negative chunk lengths are a protocol error (no negative lengths
       * exist in RESP3); rejecting them also prevents an infinite rewind. */
      if (chunk_len < 0) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }

      if ((uint64_t)chunk_len > (uint64_t)(SIZE_MAX - 2))
        goto protocol_error;
      /* Drain chunks with the same bounded incremental engine as fixed blobs.
       */
      parser->streaming_remaining = chunk_len;
      parser->streaming_blob_crlf = 2;
      result.consumed = (size_t)(pos - start);
      continue;
    }

    /* ===== Streamed aggregate end: .\r\n ===== */
    case FIO_RESP3_STREAM_END: {
      if (eol != pos || !parser->depth ||
          !parser->stack[parser->depth - 1].streaming ||
          ((parser->stack[parser->depth - 1].type == FIO_RESP3_MAP ||
            parser->stack[parser->depth - 1].type == FIO_RESP3_ATTR) &&
           parser->stack[parser->depth - 1].expecting_value)) {
        parser->error = 1;
        result.err = 1;
        cb.on_error_protocol(parser->udata);
        goto done;
      }
      pos = eol + 2;
      obj = fio___resp3_complete_frame(parser, &cb);
      obj = fio___resp3_on_value(parser, &cb, obj);
      if (parser->error) {
        result.err = 1;
        goto done;
      }
      if (obj && parser->depth == 0) {
        result.obj = obj;
        goto done;
      }
      break;
    }

    default:
      parser->error = 1;
      result.err = 1;
      cb.on_error_protocol(parser->udata);
      goto done;
    }

    /* Update consumed position after each complete element */
    result.consumed = (size_t)(pos - start);
  }

  goto done;
protocol_error:
  parser->error = 1;
  result.err = 1;
  cb.on_error_protocol(parser->udata);
done:
  result.consumed = (size_t)(pos - start);
  return result;
}

/* *****************************************************************************
RESP3 Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_RESP3
#endif /* FIO_RESP3 */
