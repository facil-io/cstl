/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ATOL               /* Development inclusion - ignore line */
#define FIO_JSON               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                JSON Parsing


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_JSON) && !defined(H___FIO_JSON___H)
#define H___FIO_JSON___H

#if !defined(FIO_JSON_MAX_DEPTH) || (FIO_JSON_MAX_DEPTH + 0) == 0
#undef FIO_JSON_MAX_DEPTH
/**
 * Maximum number of open JSON containers (root is level 1). Default 128.
 * MUST be less than 65536. This limit sizes per-parse state (one pointer
 * and one byte per level). Deeper input aborts with an error.
 */
#define FIO_JSON_MAX_DEPTH 128
#endif

FIO_ASSERT_STATIC(FIO_JSON_MAX_DEPTH < 65536, "FIO_JSON_MAX_DEPTH too big");

/** Malformed JSON and Generic Errors */
#define FIO_JSON_ERROR_GENERIC (-1)
/** Nesting level too deep error */
#define FIO_JSON_ERROR_NESTING (-2)

/**
 * Value callback result: `obj` is the produced object (NULL is a valid value),
 * `err` is zero on success or any non-zero value on failure.
 */
typedef struct {
  void *obj;
  int err;
} fio_json_obj_result_s;

/**
 * Container callback result: `obj` is the opened container, `old` is a value
 * the callback replaced and hands to the parser for conflict resolution (or
 * NULL), `err` is zero on success or any non-zero value on failure.
 */
typedef struct {
  void *obj;
  void *old;
  int err;
} fio_json_push_result_s;

/**
 * The JSON parser callbacks.
 *
 * Every fallible callback returns `err == 0` on success. Any non-zero `err` is
 * opaque and propagates unchanged; on error, `obj` / `old` are ignored.
 *
 * **Ownership**: value callbacks return owned objects. A callback that receives
 * an owned value (`map_push`, `array_push`) takes it, on failure too.
 * Containers placed in a parent are borrowed; a root (`parent == NULL`) is
 * owned. Keys are borrowed only during the callback. A native NULL `obj` /
 * `old` is "nothing" and never triggers conflict resolution.
 */
typedef struct FIO_ALIGN(64) {
  /** JSON `null`: returns a new object. */
  fio_json_obj_result_s (*on_null)(void *udata);
  /** JSON `true`: returns a new object. */
  fio_json_obj_result_s (*on_true)(void *udata);
  /** JSON `false`: returns a new object. */
  fio_json_obj_result_s (*on_false)(void *udata);
  /** JSON integer: returns a new object. */
  fio_json_obj_result_s (*on_number)(void *udata, int64_t i);
  /** JSON float: returns a new object. */
  fio_json_obj_result_s (*on_float)(void *udata, double f);
  /** JSON string with escapes (raw, still escaped): returns a new object. */
  fio_json_obj_result_s (*on_string)(void *udata, fio_buf_info_s str);
  /** JSON string without escapes: returns a new object. */
  fio_json_obj_result_s (*on_string_simple)(void *udata, fio_buf_info_s str);
  /**
   * Opens a map at `parent[key]` (`key.buf == NULL` appends to an array
   * parent; `parent == NULL` creates an owned root). Reuse an existing map to
   * merge. A replaced value is freed by the callback or returned in `old`.
   */
  fio_json_push_result_s (*on_map)(void *udata,
                                   void *parent,
                                   fio_buf_info_s key);
  /** Like `on_map`, opening an array (reuse an existing array to append). */
  fio_json_push_result_s (*on_array)(void *udata,
                                     void *parent,
                                     fio_buf_info_s key);
  /**
   * Stores `value` (owned, taken even on error) at `map[key]`. A replaced
   * value is freed by the callback or returned in `obj` (the object to keep
   * if possible) for conflict resolution.
   */
  fio_json_obj_result_s (*map_push)(void *udata,
                                    void *map,
                                    fio_buf_info_s key,
                                    void *value);
  /** Appends `value` (owned, taken even on error). Returns non-zero on error.
   */
  int (*array_push)(void *udata, void *array, void *value);
  /** Returns non-zero if `obj` is an array (optional). */
  int (*is_array)(void *udata, void *obj);
  /** Returns non-zero if `obj` is a map (optional). */
  int (*is_map)(void *udata, void *obj);
  /** Frees an owned unused object (never a key); `obj` may be NULL. */
  void (*free_unused_object)(void *udata, void *obj);
} fio_json_parser_callbacks_s;

/** The JSON parsing result. */
typedef struct {
  /** The root object (caller owned, partial on error). */
  void *ctx;
  /** Byte offset where parsing stopped. */
  size_t stop_pos;
  /** 0 on success, FIO_JSON_ERROR_* or an unchanged callback error. */
  int err;
} fio_json_result_s;

/**
 * The facil.io JSON parser is a non-strict parser, with support for trailing
 * commas in collections, new-lines in strings, extended escape characters,
 * comments and octal, hex and binary numbers.
 *
 * Parses one JSON value from `json_string` and stops once it is complete.
 *
 * `root` is NULL (the parser creates the root through `on_map` / `on_array`
 * with `parent == NULL`, or returns a scalar root) or an existing map / array
 * to update in place. An existing root requires a JSON root of the same type
 * (checked with `is_map` / `is_array`); otherwise nothing changes and the
 * error is FIO_JSON_ERROR_GENERIC.
 *
 * The caller owns `result.ctx`: on error it is the partial root (or `root`).
 *
 * Buffer requirement (guard-byte contract): number / quote-less key scanning
 * may read the byte at `json_string[len]` while deciding a token ended. The
 * buffer MUST remain readable through a non-numeric guard byte - pass a
 * NUL-terminated string (`fio_bstr` qualifies) or append a guard byte.
 */
SFUNC fio_json_result_s fio_json_parse(const fio_json_parser_callbacks_s *cb,
                                       void *udata,
                                       void *root,
                                       const char *json_string,
                                       const size_t len);

/**
 * Opens a map via `cb->on_map` and resolves a returned `old` value using the
 * default conflict rules. Returns the opened map (borrowed, owned at the root).
 * Requires on_map, on_array, map_push, array_push, free_unused_object; missing
 * is_map / is_array callbacks are treated as always false. Conflict descent
 * is bounded by FIO_JSON_MAX_DEPTH, returning FIO_JSON_ERROR_NESTING.
 */
SFUNC fio_json_obj_result_s
fio_json_resolve_on_map(const fio_json_parser_callbacks_s *cb,
                        void *udata,
                        void *parent,
                        fio_buf_info_s key);
/** Opens an array via `cb->on_array`, resolving `old` like
 * `fio_json_resolve_on_map`. Same requirements. */
SFUNC fio_json_obj_result_s
fio_json_resolve_on_array(const fio_json_parser_callbacks_s *cb,
                          void *udata,
                          void *parent,
                          fio_buf_info_s key);
/** Stores an owned `value` (taken even on error) via `cb->map_push`, resolving
 * a returned replaced value. Returns 0 or an unchanged callback error. Same
 * requirements. */
SFUNC int fio_json_resolve_map_push(const fio_json_parser_callbacks_s *cb,
                                    void *udata,
                                    void *map,
                                    fio_buf_info_s key,
                                    void *value);

/**
 * Returns 0 if every required callback is set, otherwise -1. Never writes.
 *
 * Required: on_null, on_true, on_false, on_number, on_float, on_string or
 * on_string_simple, on_map, on_array, map_push, array_push and
 * free_unused_object.
 */
SFUNC int fio_json_callbacks_validate(const fio_json_parser_callbacks_s *cb);

/* *****************************************************************************
JSON Parsing - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

typedef struct {
  fio_json_parser_callbacks_s cb; /* private copy with defaults filled */
  void *containers[FIO_JSON_MAX_DEPTH];
  uint8_t is_array[FIO_JSON_MAX_DEPTH];
  fio_buf_info_s key;
  void *udata;
  void *root;
  const char *pos;
  const char *end;
  uint32_t depth;
  int32_t error;
  fio_str_info_s key_buffer;
  char key_storage[1025]; /* 1024 decoded bytes plus the string guard byte. */
} fio___json_state_s;

#if 0 /* Used for Debugging */
#define FIO_JSON___PRINT_STEP(s, step_name)                                    \
  FIO_LOG_DEBUG2("JSON " step_name " starting at: %.*s",                       \
                 (int)((s->end - s->pos) > 16 ? 16 : (s->end - s->pos)),       \
                 s->pos)
#else
#define FIO_JSON___PRINT_STEP(s, step_name)
#endif

FIO_IFUNC int fio___json_consume_whitespace(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "white space");
  while (s->pos < s->end) {
    if (!(((uint8_t)*s->pos == 0x09U) | ((uint8_t)*s->pos == 0x0AU) |
          ((uint8_t)*s->pos == 0x0DU) | ((uint8_t)*s->pos == 0x20U)))
      return 0;
    ++s->pos;
  }
  return (s->error = FIO_JSON_ERROR_GENERIC);
}
FIO_IFUNC int fio___json_consume_comma(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "comma");
  if (fio___json_consume_whitespace(s) || *s->pos != ',')
    return -1;
  return 0;
}
FIO_IFUNC int fio___json_consume_colon(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "colon");
  if (fio___json_consume_whitespace(s) || *s->pos != ':')
    return -1;
  return 0;
}

FIO_SFUNC int fio___json_consume_comment(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "comment");
  const size_t len = (size_t)(s->end - s->pos);
  if (*s->pos == '#' || (len > 2 && s->pos[0] == '/' && s->pos[1] == '/')) {
    /* EOL style comment, C style or Bash/Ruby style*/
    const char *tmp = (const char *)FIO_MEMCHR(s->pos, '\n', len);
    if (tmp) {
      s->pos = tmp;
      return 0;
    }
    s->error = FIO_JSON_ERROR_GENERIC;
    return -1;
  }
  if ((len > 3 && s->pos[0] == '/' && s->pos[1] == '*')) {
    const char *tmp = s->pos + 2; /* skip past the opening slash-star */
    while (tmp < s->end &&
           (tmp = (const char *)FIO_MEMCHR(tmp, '/', s->end - tmp))) {
      s->pos = ++tmp;
      if (tmp[-2] == '*')
        return 0;
    }
    s->error = FIO_JSON_ERROR_GENERIC;
    return -1;
  }
  s->error = FIO_JSON_ERROR_GENERIC;
  return -1;
}

/* Unwraps a value callback result, recording a callback error. */
FIO_IFUNC void *fio___json_take(fio___json_state_s *s,
                                fio_json_obj_result_s r) {
  if (r.err) {
    s->error = r.err;
    return NULL;
  }
  return r.obj;
}

FIO_SFUNC void *fio___json_consume_infinit(fio___json_state_s *s,
                                           _Bool negative) {
  FIO_JSON___PRINT_STEP(s, "infinity");
  const uint64_t inf64 = fio_buf2u64u("infinity");
  const uint16_t inf16 = fio_buf2u16u("nf");
  uint64_t tst64;
  uint16_t tst16;
  if ((size_t)(s->end - s->pos) < 3)
    goto buffer_error;
  tst16 = (fio_buf2u16u(s->pos + 1) | (uint16_t)0x2020U);
  if (tst16 == inf16) {
    if ((size_t)(s->end - s->pos) >= 8) {
      tst64 = (fio_buf2u64u(s->pos) | (uint64_t)0x2020202020202020ULL);
      s->pos += (tst64 == inf64) * 5;
    }
    s->pos += 3;
    return fio___json_take(
        s,
        s->cb.on_float(s->udata, negative ? (INFINITY * -1) : INFINITY));
  }
buffer_error:
  s->error = FIO_JSON_ERROR_GENERIC;
  return NULL;
}

FIO_SFUNC void *fio___json_consume_number(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "number");
  fio_aton_s aton = fio_aton((char **)&s->pos);
  if (aton.err) {
    s->error = FIO_JSON_ERROR_GENERIC;
    return NULL;
  }
  return fio___json_take(s,
                         aton.is_float ? s->cb.on_float(s->udata, aton.f)
                                       : s->cb.on_number(s->udata, aton.i));
}

FIO_IFUNC void *fio___json_consume_string_any(fio___json_state_s *s,
                                              const char quote) {
  fio_json_obj_result_s (*cb)(void *udata, fio_buf_info_s str) =
      s->cb.on_string_simple;
  const char *start = ++s->pos;
  const uint64_t qv = 0x0101010101010101ULL * (uint64_t)(uint8_t)quote;
  for (;;) {
    if (s->pos >= s->end)
      break;
    /* SWAR fast scan: skip 8 bytes with no quote and no backslash */
    while ((size_t)(s->end - s->pos) >= 8) {
      uint64_t v, x;
      FIO_MEMCPY(&v, s->pos, 8);
      x = v ^ qv; /* quote */
      uint64_t dirty = (x - 0x0101010101010101ULL) & ~x & 0x8080808080808080ULL;
      x = v ^ 0x5C5C5C5C5C5C5C5CULL; /* '\\' */
      dirty |= (x - 0x0101010101010101ULL) & ~x & 0x8080808080808080ULL;
      if (dirty)
        break;
      s->pos += 8;
    }
    if (s->pos >= s->end)
      break;
    if (*s->pos == quote) {
      fio_buf_info_s str =
          FIO_BUF_INFO2((char *)start, (size_t)(s->pos - start));
      ++s->pos;
      return fio___json_take(s, cb(s->udata, str));
    }
    if (*s->pos++ == '\\') {
      cb = s->cb.on_string;
      if (s->pos == s->end)
        break;
      ++s->pos; /* skip the escaped byte */
    }
  }
  s->error = FIO_JSON_ERROR_GENERIC;
  return NULL;
}

FIO_SFUNC void *fio___json_consume_string(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "double quote string");
  return fio___json_consume_string_any(s, '"');
}

FIO_SFUNC void *fio___json_consume_string_single_quote(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "single quote string");
  return fio___json_consume_string_any(s, '\'');
}

FIO_IFUNC fio_buf_info_s fio___json_key_from_slice(fio___json_state_s *s,
                                                   const char *start,
                                                   size_t len,
                                                   int escaped) {
  if (escaped) {
    /* Retain a heap fallback when it fits; allocate-copy doesn't free it. */
    if (s->key_buffer.buf != s->key_storage && len >= s->key_buffer.capa) {
      FIO_STRING_FREE2(s->key_buffer);
      s->key_buffer = FIO_STR_INFO3(s->key_storage, 0, sizeof(s->key_storage));
    }
    s->key_buffer.len = 0;
    if (fio_string_write_unescape(&s->key_buffer,
                                  FIO_STRING_ALLOC_COPY,
                                  start,
                                  len)) {
      s->error = FIO_JSON_ERROR_GENERIC;
      return (fio_buf_info_s){0};
    }
    start = s->key_buffer.buf;
    len = s->key_buffer.len;
  }
  return (fio_buf_info_s){.buf = (char *)start, .len = len};
}

FIO_SFUNC fio_buf_info_s fio___json_consume_object_key(fio___json_state_s *s) {
  // return fio___json_consume(s);
  /* Ruby map maker
  m = []; m[255] = 0; 256.times {|i| m[i] = 0 };
  "0123456789_$\\".bytes.each {|b| m[b] = 1 };
  ('z'.ord - 'a'.ord + 1).times {|i| m['a'.ord + i] = 1; m['A'.ord + i] = 1; };
  puts "static const uint8_t valid_key[256] = { #{m.join(', ')} };"; nil
  */
  static const uint8_t valid_key[256] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  if (s->pos >= s->end)
    goto set_error;
  if (*s->pos == '"' || *s->pos == '\'') {
    const char quote = *s->pos;
    const char *start = ++s->pos;
    int escaped = 0;
    for (; s->pos < s->end; ++s->pos) {
      if (*s->pos == quote) {
        size_t len = (size_t)(s->pos++ - start);
        return fio___json_key_from_slice(s, start, len, escaped);
      }
      if (*s->pos == '\\') {
        escaped = 1;
        if (++s->pos == s->end)
          goto set_error;
      }
    }
    goto set_error;
  }
  if (*s->pos == ',' || *s->pos == '}')
    return (fio_buf_info_s){0};
  FIO_JSON___PRINT_STEP(s, "consumption of quote-less map key");
  int escaped = 0;
  const char *start = s->pos;
  for (; s->pos < s->end;) {
    size_t add = 1;
    const size_t left = (size_t)(s->end - s->pos);
    if (!valid_key[(*(uint8_t *)s->pos)] &&
        (add = fio_utf8_char_len_bounded(s->pos, left)) < 2)
      return fio___json_key_from_slice(s,
                                       start,
                                       (size_t)(s->pos - start),
                                       escaped);
    if (*s->pos == '\\') {
      escaped = 1;
      if (left == 1) {
        ++s->pos;
        goto set_error;
      }
      ++add;
    }
    s->pos += add;
  }
set_error:
  s->error = FIO_JSON_ERROR_GENERIC;
  return (fio_buf_info_s){0};
}

/* *****************************************************************************
JSON Conflict Resolution Helpers (default rules)
***************************************************************************** */

#define FIO___JSON_EMPTY_KEY ((fio_buf_info_s){.buf = (char *)"", .len = 0})

FIO_IFUNC int fio___json_is_map(const fio_json_parser_callbacks_s *cb,
                                void *udata,
                                void *obj) {
  return cb->is_map && cb->is_map(udata, obj);
}
FIO_IFUNC int fio___json_is_array(const fio_json_parser_callbacks_s *cb,
                                  void *udata,
                                  void *obj) {
  return cb->is_array && cb->is_array(udata, obj);
}
FIO_IFUNC void fio___json_free(const fio_json_parser_callbacks_s *cb,
                               void *udata,
                               void *obj) {
  if (obj)
    cb->free_unused_object(udata, obj);
}

SFUNC int fio_json_resolve_map_push(const fio_json_parser_callbacks_s *cb,
                                    void *udata,
                                    void *map,
                                    fio_buf_info_s key,
                                    void *value) {
  size_t depth = 0;
resolve_next:
  if (depth++ == FIO_JSON_MAX_DEPTH) {
    fio___json_free(cb, udata, value);
    return FIO_JSON_ERROR_NESTING;
  }
  fio_json_obj_result_s r = cb->map_push(udata, map, key, value);
  if (r.err)
    return r.err;
  void *old = r.obj;
  if (!old)
    return 0;
  const int old_is_map = fio___json_is_map(cb, udata, old);
  if (old_is_map || fio___json_is_array(cb, udata, old)) {
    /* keep the existing container: restore it, then nest the incoming value */
    r = cb->map_push(udata, map, key, old);
    if (r.err)
      return r.err;
    if (!r.obj)
      return 0;
    if (old_is_map) {
      map = old;
      key = FIO___JSON_EMPTY_KEY;
      value = r.obj;
      goto resolve_next;
    }
    return cb->array_push(udata, old, r.obj);
  }
  /* scalar conflict: replace the incoming value with [old, incoming] */
  fio_json_push_result_s a = cb->on_array(udata, map, key);
  if (a.err) {
    fio___json_free(cb, udata, old);
    return a.err;
  }
  if (!a.obj) {
    fio___json_free(cb, udata, old);
    fio___json_free(cb, udata, a.old);
    return FIO_JSON_ERROR_GENERIC;
  }
  int err = cb->array_push(udata, a.obj, old);
  if (err) {
    fio___json_free(cb, udata, a.old);
    return err;
  }
  return a.old ? cb->array_push(udata, a.obj, a.old) : 0;
}

SFUNC fio_json_obj_result_s
fio_json_resolve_on_map(const fio_json_parser_callbacks_s *cb,
                        void *udata,
                        void *parent,
                        fio_buf_info_s key) {
  fio_json_obj_result_s r = {0};
  fio_json_push_result_s m = cb->on_map(udata, parent, key);
  if (m.err) {
    r.err = m.err;
    return r;
  }
  if (!m.obj) {
    fio___json_free(cb, udata, m.old);
    r.err = FIO_JSON_ERROR_GENERIC;
    return r;
  }
  if (m.old) /* the replaced value moves to the new map's "" key */
    r.err = fio_json_resolve_map_push(cb,
                                      udata,
                                      m.obj,
                                      FIO___JSON_EMPTY_KEY,
                                      m.old);
  if (!r.err)
    r.obj = m.obj;
  return r;
}

SFUNC fio_json_obj_result_s
fio_json_resolve_on_array(const fio_json_parser_callbacks_s *cb,
                          void *udata,
                          void *parent,
                          fio_buf_info_s key) {
  size_t depth = 0;
resolve_next:
  if (depth++ == FIO_JSON_MAX_DEPTH)
    return (fio_json_obj_result_s){.err = FIO_JSON_ERROR_NESTING};
  fio_json_obj_result_s r = {0};
  fio_json_push_result_s a = cb->on_array(udata, parent, key);
  if (a.err) {
    r.err = a.err;
    return r;
  }
  if (!a.obj) {
    fio___json_free(cb, udata, a.old);
    r.err = FIO_JSON_ERROR_GENERIC;
    return r;
  }
  if (!a.old) {
    r.obj = a.obj;
    return r;
  }
  if (key.buf && fio___json_is_map(cb, udata, a.old)) {
    /* keep the existing map: restore it and open the array at its "" key */
    r = cb->map_push(udata, parent, key, a.old);
    if (r.err)
      return r;
    fio___json_free(cb, udata, r.obj); /* the displaced new array */
    parent = a.old;
    key = FIO___JSON_EMPTY_KEY;
    goto resolve_next;
  }
  r.err = cb->array_push(udata, a.obj, a.old);
  if (!r.err)
    r.obj = a.obj;
  return r;
}

FIO_SFUNC void *fio___json_consume_null(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "null");
  const uint32_t wrd = fio_buf2u32u("null");
  uint32_t data;
  if (s->end - s->pos < 4)
    goto on_error;
  data = fio_buf2u32u(s->pos) | (uint32_t)0x20202020;
  if (data != wrd)
    goto on_error;
  s->pos += 4;
  return fio___json_take(s, s->cb.on_null(s->udata));
on_error:
  s->error = FIO_JSON_ERROR_GENERIC;
  return NULL;
}
FIO_SFUNC void *fio___json_consume_true(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "true");
  const uint32_t wrd = fio_buf2u32u("true");
  uint32_t data;
  if (s->end - s->pos < 4)
    goto on_error;
  data = fio_buf2u32u(s->pos) | (uint32_t)0x20202020;
  if (data != wrd)
    goto on_error;
  s->pos += 4;
  return fio___json_take(s, s->cb.on_true(s->udata));
on_error:
  s->error = FIO_JSON_ERROR_GENERIC;
  return NULL;
}
FIO_SFUNC void *fio___json_consume_false(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "false");
  const uint32_t wrd = fio_buf2u32u("alse");
  uint32_t data;
  if (s->end - s->pos < 5)
    goto on_error;
  data = fio_buf2u32u(s->pos + 1) | (uint32_t)0x20202020;
  if (data != wrd)
    goto on_error;
  s->pos += 5;
  return fio___json_take(s, s->cb.on_false(s->udata));
on_error:
  s->error = FIO_JSON_ERROR_GENERIC;
  return NULL;
}
FIO_SFUNC void *fio___json_consume_nan(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "nan");
  const uint16_t wrd = fio_buf2u16u("an");
  uint16_t data;
  if (s->end - s->pos < 3)
    goto on_error;
  data = fio_buf2u16u(s->pos + 1) | (uint16_t)0x2020;
  if (data != wrd)
    goto on_error;
  s->pos += 3;
  return fio___json_take(s, s->cb.on_float(s->udata, NAN));
on_error:
  s->error = FIO_JSON_ERROR_GENERIC;
  return NULL;
}

FIO_SFUNC void *fio___json_consume(fio___json_state_s *s) {
  enum {
    ROOT_VALUE,
    MAP_KEY,
    ARRAY_VALUE,
    ARRAY_ITEM,
    MAP_VALUE,
    MAP_AFTER,
    ARRAY_AFTER
  } phase = ROOT_VALUE;
  void *root = s->root;
  void *value = NULL;
  for (;;) {
    void *parent = s->depth ? s->containers[s->depth - 1] : NULL;
    if (phase == MAP_KEY) {
      if (fio___json_consume_whitespace(s))
        goto unwind;
      if (*s->pos == '#' || *s->pos == '/') {
        if (fio___json_consume_comment(s))
          goto unwind;
        continue;
      }
      s->key = fio___json_consume_object_key(s);
      if (s->error)
        goto unwind;
      if (!s->key.buf) {
        if (*s->pos != '}') {
          s->error = FIO_JSON_ERROR_GENERIC;
          goto unwind;
        }
        ++s->pos;
        goto close_level;
      }
      if (fio___json_consume_colon(s)) {
        s->error = FIO_JSON_ERROR_GENERIC;
        goto unwind;
      }
      ++s->pos;
      if (fio___json_consume_whitespace(s))
        goto unwind;
      phase = MAP_VALUE;
    } else if (phase == ARRAY_VALUE) {
      ++s->pos;
      if (fio___json_consume_whitespace(s))
        goto unwind;
      if (*s->pos == ']') {
        ++s->pos;
        goto close_level;
      }
      phase = ARRAY_ITEM;
    } else if (phase == MAP_AFTER) {
      s->key = (fio_buf_info_s){0};
      if (fio___json_consume_whitespace(s))
        goto unwind;
      if (*s->pos == ',') {
        ++s->pos;
        phase = MAP_KEY;
        continue;
      }
      if (*s->pos != '}') {
        s->error = FIO_JSON_ERROR_GENERIC;
        goto unwind;
      }
      ++s->pos;
      goto close_level;
    } else if (phase == ARRAY_AFTER) {
      if (fio___json_consume_comma(s)) {
        if (s->error || *s->pos != ']') {
          s->error = FIO_JSON_ERROR_GENERIC;
          goto unwind;
        }
        ++s->pos;
        goto close_level;
      }
      phase = ARRAY_VALUE;
      continue;
    }
    FIO_JSON___PRINT_STEP(s, "consumption type test");
    if (s->pos < s->end)
      switch (*s->pos) {
      case 0x09: /* fall through */
      case 0x0A: /* fall through */
      case 0x0D: /* fall through */
      case 0x20:
        ++s->pos;
        if (fio___json_consume_whitespace(s))
          goto set_error;
        continue;
      case '+': /* fall through */
      case '-': /* fall through */
      case '0': /* fall through */
      case '1': /* fall through */
      case '2': /* fall through */
      case '3': /* fall through */
      case '4': /* fall through */
      case '5': /* fall through */
      case '6': /* fall through */
      case '7': /* fall through */
      case '8': /* fall through */
      case '9': /* fall through */
      case 'x': /* fall through */
      case '.': /* fall through */
      case 'e': /* fall through */
      case 'E': value = fio___json_consume_number(s); goto scalar;
      case 'i': /* fall through */
      case 'I': value = fio___json_consume_infinit(s, 0); goto scalar;
      case '"': value = fio___json_consume_string(s); goto scalar;
      case '\'': value = fio___json_consume_string_single_quote(s); goto scalar;
      case '{':
      case '[': {
        uint8_t array = (*s->pos == '[');
        FIO_JSON___PRINT_STEP(s, array ? "array" : "map");
        void *container;
        if (!parent && s->root) { /* update an existing root in place */
          if (!(array ? s->cb.is_array(s->udata, s->root)
                      : s->cb.is_map(s->udata, s->root))) {
            s->error = FIO_JSON_ERROR_GENERIC;
            goto unwind;
          }
          container = s->root;
        } else {
          fio_json_obj_result_s c =
              array
                  ? fio_json_resolve_on_array(&s->cb, s->udata, parent, s->key)
                  : fio_json_resolve_on_map(&s->cb, s->udata, parent, s->key);
          container = fio___json_take(s, c);
          if (!parent)
            root = container;
          if (s->error)
            goto unwind;
          if (!container) {
            s->error = FIO_JSON_ERROR_GENERIC;
            goto unwind;
          }
        }
        s->key = (fio_buf_info_s){0};
        /* The old parser opened the failing level, but never finished it. */
        if (s->depth == FIO_JSON_MAX_DEPTH) {
          s->error = FIO_JSON_ERROR_NESTING;
          goto unwind;
        }
        s->containers[s->depth] = container;
        s->is_array[s->depth++] = array;
        phase = array ? ARRAY_VALUE : MAP_KEY;
        if (!array)
          ++s->pos;
        continue;
      }
      case '}': /* fall through */
      case ']':
        if (phase == ARRAY_ITEM) { /* empty slot (e.g. `[1,]`): no value */
          phase = ARRAY_AFTER;
          continue;
        }
        value = NULL; /* missing map value (e.g. `{"a":}`) stores NULL */
        goto scalar;
      case 'T': /* fall through */
      case 't': value = fio___json_consume_true(s); goto scalar;
      case 'F': /* fall through */
      case 'f': value = fio___json_consume_false(s); goto scalar;
      case 'N': /* fall through */
      case 'n':
        value = ((s->end - s->pos > 3) && ((s->pos[1] | 32) == 'u')
                     ? fio___json_consume_null
                     : fio___json_consume_nan)(s);
        goto scalar;
      case '#':
      case '/':
        if (fio___json_consume_comment(s))
          goto set_error;
        continue;
      }
  set_error:
    s->error = FIO_JSON_ERROR_GENERIC;
    goto unwind;
  scalar:
    if (s->error)
      goto unwind;
    if (!parent) {
      if (!s->root)
        return value;
      /* a scalar can't update an existing root */
      fio___json_free(&s->cb, s->udata, value);
      s->error = FIO_JSON_ERROR_GENERIC;
      goto unwind;
    }
    if (phase == MAP_VALUE) {
      s->error =
          fio_json_resolve_map_push(&s->cb, s->udata, parent, s->key, value);
      phase = MAP_AFTER;
    } else {
      s->error = s->cb.array_push(s->udata, parent, value);
      phase = ARRAY_AFTER;
    }
    if (s->error)
      goto unwind;
    continue;
  close_level:
    --s->depth;
    if (!s->depth)
      return root;
    phase = s->is_array[s->depth - 1] ? ARRAY_AFTER : MAP_AFTER;
    continue;
  unwind:
    s->depth = 0;
    return root;
  }
}

static int fio___json_callback_noop(void *udata, void *obj) {
  (void)udata;
  (void)obj;
  return 0;
}

SFUNC int fio_json_callbacks_validate(const fio_json_parser_callbacks_s *cb) {
  if (!cb || !cb->on_null || !cb->on_true || !cb->on_false || !cb->on_number ||
      !cb->on_float || !(cb->on_string || cb->on_string_simple) ||
      !cb->on_map || !cb->on_array || !cb->map_push || !cb->array_push ||
      !cb->free_unused_object)
    return -1;
  return 0;
}

SFUNC fio_json_result_s fio_json_parse(const fio_json_parser_callbacks_s *cb,
                                       void *udata,
                                       void *root,
                                       const char *start,
                                       const size_t len) {
  fio_json_result_s r = {.ctx = root, .stop_pos = 0, .err = 0};
  fio___json_state_s state;
  if (fio_json_callbacks_validate(cb))
    goto missing_callback;
  state = (fio___json_state_s){
      .cb = cb[0],
      .udata = udata,
      .root = root,
      .pos = start,
      .end = start + len,
  };
  /* fill defaults in the private copy */
  if (!state.cb.on_string)
    state.cb.on_string = state.cb.on_string_simple;
  if (!state.cb.on_string_simple)
    state.cb.on_string_simple = state.cb.on_string;
  if (!state.cb.is_array)
    state.cb.is_array = fio___json_callback_noop;
  if (!state.cb.is_map)
    state.cb.is_map = fio___json_callback_noop;
  state.key_buffer =
      FIO_STR_INFO3(state.key_storage, 0, sizeof(state.key_storage));

  /* skip BOM, if exists */
  if (len >= 3 && state.pos[0] == (char)0xEF && state.pos[1] == (char)0xBB &&
      state.pos[2] == (char)0xBF) {
    state.pos += 3;
    if (len == 3) {
      r.stop_pos = 3;
      goto finish;
    }
  }
  r.ctx = fio___json_consume(&state);
  r.err = state.error;
  r.stop_pos = state.pos - start;
  if (state.error && state.pos <= state.end)
    FIO_LOG_DEBUG(
        "JSON parsing failed after:\n%.*s",
        (int)((state.end - state.pos > 48) ? 48 : (state.end - state.pos)),
        state.pos);
finish:
  if (state.key_buffer.buf != state.key_storage)
    FIO_STRING_FREE2(state.key_buffer);
  return r;

missing_callback:
  FIO_LOG_ERROR("JSON parser missing a critical callback!");
  r.err = FIO_JSON_ERROR_GENERIC;
  return r;
}
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_JSON
#endif /* FIO_JSON */
