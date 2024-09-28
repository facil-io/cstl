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

#ifndef FIO_JSON_MAX_DEPTH
/** Maximum allowed JSON nesting level. Values above 64K might fail. */
#define FIO_JSON_MAX_DEPTH 512
#endif

#ifndef FIO_JSON_USE_FIO_ATON
#define FIO_JSON_USE_FIO_ATON 0
#endif

/** The JSON parser settings. */
typedef struct {
  /** NULL object was detected. Returns new object as `void *`. */
  void *(*on_null)(void);
  /** TRUE object was detected. Returns new object as `void *`. */
  void *(*on_true)(void);
  /** FALSE object was detected. Returns new object as `void *`. */
  void *(*on_false)(void);
  /** Number was detected (long long). Returns new object as `void *`. */
  void *(*on_number)(int64_t i);
  /** Float was detected (double).Returns new object as `void *`.  */
  void *(*on_float)(double f);
  /** (escaped) String was detected. Returns a new String as `void *`. */
  void *(*on_string)(const void *start, size_t len);
  /** (unescaped) String was detected. Returns a new String as `void *`. */
  void *(*on_string_simple)(const void *start, size_t len);
  /** Dictionary was detected. Returns ctx to hash map or NULL on error. */
  void *(*on_map)(void *ctx, void *at);
  /** Array was detected. Returns ctx to array or NULL on error. */
  void *(*on_array)(void *ctx, void *at);
  /** Array was detected. Returns non-zero on error. */
  int (*map_push)(void *ctx, void *key, void *value);
  /** Array was detected. Returns non-zero on error. */
  int (*array_push)(void *ctx, void *value);
  /** Called when an array object (`ctx`) appears done. */
  int (*array_finished)(void *ctx);
  /** Called when a map object (`ctx`) appears done. */
  int (*map_finished)(void *ctx);
  /** Called when context is expected to be an array (i.e., fio_json_update). */
  int (*is_array)(void *ctx);
  /** Called when context is expected to be a map (i.e., fio_json_update). */
  int (*is_map)(void *ctx);
  /** Called for the `key` element in case of error or NULL value. */
  void (*free_unused_object)(void *ctx);
  /** the JSON parsing encountered an error - what to do with ctx? */
  void *(*on_error)(void *ctx);
} fio_json_parser_callbacks_s;

/** The JSON return type. */
typedef struct {
  void *ctx;
  size_t stop_pos;
  int err;
} fio_json_result_s;

/**
 * The facil.io JSON parser is a non-strict parser, with support for trailing
 * commas in collections, new-lines in strings, extended escape characters and
 * octal, hex and binary numbers.
 *
 * The parser allows for streaming data and decouples the parsing process from
 * the resulting data-structure by calling static callbacks for JSON related
 * events.
 *
 * Returns the number of bytes consumed before parsing stopped (due to either
 * error or end of data). Stops as close as possible to the end of the buffer or
 * once an object parsing was completed.
 */
SFUNC fio_json_result_s fio_json_parse(fio_json_parser_callbacks_s *settings,
                                       const char *json_string,
                                       const size_t len);

/* *****************************************************************************
JSON Parsing - Implementation - Helpers and Callbacks


Note: static Callacks must be implemented in the C file that uses the parser

Note: a Helper API is provided for the parsing implementation.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

// typedef struct {
//   struct {
//     uintptr_t start;
//     uintptr_t end;
//   } instructions[16];
//   uint32_t count;
// } fio___json_cb_queue_s;

typedef struct {
  fio_json_parser_callbacks_s cb;
  void *ctx;
  void *key;
  const char *pos;
  const char *end;
  uint32_t depth;
  uint32_t error;
} fio___json_state_s;

FIO_SFUNC void *fio___json_consume(fio___json_state_s *s);

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
  return (s->error = -1);
}
FIO_IFUNC int fio___json_consume_comma(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "comma");
  fio___json_consume_whitespace(s);
  if (*s->pos != ',')
    return -1;
  fio___json_consume_whitespace(s);
  return 0;
}
FIO_IFUNC int fio___json_consume_colon(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "colon");
  fio___json_consume_whitespace(s);
  if (*s->pos != ':')
    return -1;
  fio___json_consume_whitespace(s);
  return 0;
}

FIO_SFUNC void *fio___json_consume_infinit(fio___json_state_s *s,
                                           _Bool negative) {
  FIO_JSON___PRINT_STEP(s, "infinity");
  const uint64_t inf64 = fio_buf2u64u("infinity");
  const uint16_t inf16 = fio_buf2u16u("nf");
  uint64_t tst64;
  uint16_t tst16;
  if (s->pos + 3 > s->end)
    goto buffer_error;
  tst16 = (fio_buf2u16u(s->pos + 1) | (uint16_t)0x2020U);
  if (tst16 == inf16) {
    if (s->pos + 7 < s->end) {
      tst64 = (fio_buf2u64u(s->pos) | (uint64_t)0x2020202020202020ULL);
      s->pos += (tst64 == inf64) * 5;
    }
    s->pos += 3;
    return s->cb.on_float(negative ? (INFINITY * -1) : INFINITY);
  }
buffer_error:
  s->error = 1;
  return NULL;
}

FIO_SFUNC void *fio___json_consume_number(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "number");
#if FIO_JSON_USE_FIO_ATON
  fio_aton_s aton = fio_aton((char **)&s->pos);
  return aton.is_float ? s->cb.on_float(aton.f) : s->cb.on_number(aton.i);
#else
  const char *tst = s->pos;
  uint64_t i;
  double f;
  _Bool negative = (tst[0] == '-') | (tst[0] == '+');
  _Bool hex = 0;
  _Bool binary = 0;
  long ilimit = 19 + negative;
  tst += negative;
  if (tst + 1 > s->end)
    goto buffer_overflow;
  if ((tst[0] | 0x20) == 'i')
    goto is_inifinity;
  tst += (tst[0] == '0' && tst + 2 < s->end);
  if ((tst[0] | 32) == 'x') {
    hex = 1;
    while ((tst < s->end) & ((tst[0] >= '0' & tst[0] <= '9') |
                             ((tst[0] | 32) >= 'a' & (tst[0] | 32) <= 'f')))
      ++tst;
  } else if ((tst[0] | 32) == 'b') {
    binary = 1;
    ilimit = 66 + negative;
    while (((tst < s->end) & (tst[0] >= '0') & (tst[0] <= '1')))
      ++tst;
  } else {
    while (((tst < s->end) & (tst[0] >= '0') & (tst[0] <= '9')))
      ++tst;
  }
  if (tst > (s->pos + ilimit) ||
      ((tst < s->end) &&
       (tst[0] == '.' || (tst[0] | 32) == 'e' || (tst[0] | 32) == 'p')))
    goto is_float;
  tst = s->pos;
  s->pos += negative;
  errno = 0;
  i = (hex              ? fio_atol16u((char **)&s->pos)
       : binary         ? fio_atol_bin((char **)&s->pos)
       : *s->pos == '0' ? fio_atol8u((char **)&s->pos)
                        : fio_atol10u((char **)&s->pos));
  if (errno == E2BIG || (((uint64_t)(1 ^ hex ^ binary) << 63) & i))
    goto is_float_from_error;
  // s->error = (errno == E2BIG);
  return s->cb.on_number(fio_u2i_limit(i, negative));
is_float_from_error:
  s->pos = tst;
  errno = 0;

is_float:
  f = fio_atof((char **)&s->pos);
  s->error = (errno == E2BIG);
  return s->cb.on_float(f);

buffer_overflow:
  s->error = 1;
  return NULL;
is_inifinity:
  ++s->pos;
  return fio___json_consume_infinit(s, negative);
#endif /* FIO_JSON_USE_FIO_ATON */
}

FIO_SFUNC void *fio___json_consume_string(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "string");
  void *(*cb)(const void *start, size_t len) = s->cb.on_string_simple;
  const char *start = ++s->pos;
  for (; s->pos < s->end; ++s->pos) {
    if (*s->pos == '"')
      return cb(start, (s->pos++) - start);
    if (*s->pos == '\\')
      cb = s->cb.on_string;
    s->pos += (*s->pos == '\\');
  }
  s->error = 1;
  return NULL;
}

FIO_SFUNC void *fio___json_consume_map(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "map");
  void *old = s->ctx;
  void *old_key = s->key;
  void *map = s->cb.on_map(s, s->key);
  s->ctx = map;
  s->key = NULL;
  if (++s->depth == FIO_JSON_MAX_DEPTH)
    goto too_deep;
  for (;;) {
    ++s->pos;
    s->key = fio___json_consume(s);
    if (s->error || !s->key)
      break;
    if (*s->pos != ':')
      break;
    ++s->pos;
    if (fio___json_consume_whitespace(s))
      break;
    void *value = fio___json_consume(s);
    s->error |= s->cb.map_push(s->ctx, s->key, value);
    s->key = NULL;
    if (s->error || fio___json_consume_whitespace(s))
      break;
    if (*s->pos != ',')
      break;
  }
  if (s->key) {
    s->error = 1;
    s->cb.free_unused_object(s->key);
  } else if (*s->pos != '}' || s->error) {
    s->error = 1;
  } else {
    ++s->pos;
  }
  --s->depth;
  s->ctx = old;
  s->key = old_key;
  s->error |= s->cb.map_finished(map);
  return map;
too_deep:
  s->ctx = old;
  s->key = old_key;
  s->error = 1;
  --s->depth;
  return map;
}
FIO_SFUNC void *fio___json_consume_array(fio___json_state_s *s) {
  FIO_JSON___PRINT_STEP(s, "array");
  void *old = s->ctx;
  void *array = s->ctx = s->cb.on_array(s, s->key);
  if (++s->depth == FIO_JSON_MAX_DEPTH)
    goto too_deep;
  for (;;) {
    ++s->pos;
    if (fio___json_consume_whitespace(s))
      break;
    if (*s->pos == ']')
      break;
    void *value = fio___json_consume(s);
    if (value)
      s->error |= s->cb.array_push(s->ctx, value);
    if (s->error || fio___json_consume_comma(s))
      break;
    if (*s->pos != ',')
      break;
  }
  if (*s->pos != ']' || s->error) {
    s->error = 1;
  } else {
    ++s->pos;
  }
  s->ctx = old;
  --s->depth;
  s->error |= s->cb.array_finished(array);
  return array;
too_deep:
  s->ctx = old;
  s->error = 1;
  --s->depth;
  return array;
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
  return s->cb.on_null();
on_error:
  s->error = 1;
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
  return s->cb.on_true();
on_error:
  s->error = 1;
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
  return s->cb.on_false();
on_error:
  s->error = 1;
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
  return s->cb.on_float(NAN);
on_error:
  s->error = 1;
  return NULL;
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
    s->error = 1;
    return -1;
  }
  if ((len > 3 && s->pos[0] == '/' && s->pos[1] == '*')) {
    const char *tmp = s->pos;
    while (tmp < s->end &&
           (tmp = (const char *)FIO_MEMCHR(s->pos, '/', s->end - tmp))) {
      s->pos = ++tmp;
      if (tmp[-2] == '*')
        return 0;
    }
    s->error = 1;
    return -1;
  }
  s->error = 1;
  return -1;
}

void *fio___json_consume(fio___json_state_s *s) {
  for (;;) {
    FIO_JSON___PRINT_STEP(s, "consumption type test");
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
    case 'E': return fio___json_consume_number(s);
    case 'i': /* fall through */
    case 'I': return fio___json_consume_infinit(s, 0);
    case '"': return fio___json_consume_string(s);
    case '{': return fio___json_consume_map(s);
    case '}': return NULL; /* don't progress, just stop. */
    case '[': return fio___json_consume_array(s);
    case ']': return NULL; /* don't progress, just stop. */
    case 'T':              /* fall through */
    case 't': return fio___json_consume_true(s);
    case 'F': /* fall through */
    case 'f': return fio___json_consume_false(s);
    case 'N': /* fall through */
    case 'n':
      return (((s->pos[1] | 32) == 'u') ? fio___json_consume_null
                                        : fio___json_consume_nan)(s);
    case '#':
    case '/':
      if (fio___json_consume_comment(s))
        goto set_error;
      continue;
    }
  set_error:
    s->error = 1;
    return NULL;
  }
}

static int fio___json_callback_noop(void *ctx) {
  return 0;
  (void)ctx;
}
static void *fio___json_callback_noop2(void *ctx) { return ctx; }

FIO_SFUNC int fio___json_callbacks_validate(fio_json_parser_callbacks_s *cb) {
  if (!cb)
    goto is_invalid;

  if (!cb->on_string)
    cb->on_string = cb->on_string_simple;
  if (!cb->on_string_simple)
    cb->on_string_simple = cb->on_string;

  if (!cb->on_null)
    goto is_invalid;
  if (!cb->on_true)
    goto is_invalid;
  if (!cb->on_false)
    goto is_invalid;
  if (!cb->on_number)
    goto is_invalid;
  if (!cb->on_float)
    goto is_invalid;
  if (!cb->on_string)
    goto is_invalid;
  if (!cb->on_map)
    goto is_invalid;
  if (!cb->on_array)
    goto is_invalid;
  if (!cb->map_push)
    goto is_invalid;
  if (!cb->array_push)
    goto is_invalid;
  if (!cb->free_unused_object)
    goto is_invalid;
  if (!cb->array_finished)
    cb->array_finished = fio___json_callback_noop;
  if (!cb->map_finished)
    cb->map_finished = fio___json_callback_noop;
  if (!cb->is_array)
    cb->is_array = fio___json_callback_noop;
  if (!cb->is_map)
    cb->is_map = fio___json_callback_noop;
  if (!cb->on_error)
    cb->on_error = fio___json_callback_noop2;
  return 0;
is_invalid:
  return -1;
}
/**
 * Returns the number of bytes consumed. Stops as close as possible to the end
 * of the buffer or once an object parsing was completed.
 */
SFUNC fio_json_result_s fio_json_parse(fio_json_parser_callbacks_s *callbacks,
                                       const char *start,
                                       const size_t len) {
  fio_json_result_s r = {.stop_pos = 0, .err = 0};
  fio___json_state_s state;
  if (fio___json_callbacks_validate(callbacks))
    goto missing_callback;

  state = (fio___json_state_s){
      .cb = callbacks[0],
      .pos = start,
      .end = start + len,
  };

  /* skip BOM, if exists */
  if (len >= 3 && state.pos[0] == (char)0xEF && state.pos[1] == (char)0xBB &&
      state.pos[2] == (char)0xBF) {
    state.pos += 3;
    if (len == 3)
      goto finish;
  }
  r.ctx = fio___json_consume(&state);
  r.err = state.error;
  r.stop_pos = state.pos - start;
  if (state.error)
    goto failed;
finish:
  return r;
failed:
  FIO_LOG_DEBUG(
      "JSON parsing failed after:\n%.*s",
      ((state.end - state.pos > 48) ? 48 : ((int)(state.end - state.pos))),
      state.pos);
  r.ctx = callbacks->on_error(r.ctx);
  return r;

missing_callback:
  FIO_LOG_ERROR("JSON parser missing a critical callback!");
  r.err = 1;
  return r;
}

/** Dictionary was detected. Returns ctx to hash map or NULL on error. */
FIO_SFUNC void *fio___json_parse_update_on_map(void *ctx, void *at) {
  void **ex_data = (void **)ctx;
  fio_json_parser_callbacks_s *cb = (fio_json_parser_callbacks_s *)ex_data[0];
  ctx = ex_data[1];
  fio___json_state_s *s = (fio___json_state_s *)ex_data[2];
  s->cb.on_map = cb->on_map;
  s->cb.on_array = cb->on_array;
  if (ctx && !s->cb.is_map(ctx))
    return NULL;
  else if (!ctx)
    ctx = cb->on_map(ctx, at);
  return ctx;
}
/** Array was detected. Returns ctx to array or NULL on error. */
FIO_SFUNC void *fio___json_parse_update_on_array(void *ctx, void *at) {
  void **ex_data = (void **)ctx;
  fio_json_parser_callbacks_s *cb = (fio_json_parser_callbacks_s *)ex_data[0];
  ctx = ex_data[1];
  fio___json_state_s *s = (fio___json_state_s *)ex_data[2];
  s->cb.on_map = cb->on_map;
  s->cb.on_array = cb->on_array;
  if (ctx && !s->cb.is_array(ctx))
    return NULL;
  else if (!ctx)
    ctx = cb->on_array(ctx, at);

  return ctx;
}

/**
 * Use only when `ctx` is an object and JSON data is wrapped in an object (of
 * the same type).
 *
 * i.e., update an array or hash map.
 */
SFUNC fio_json_result_s fio_json_parse_update(fio_json_parser_callbacks_s *s,
                                              void *ctx,
                                              const char *start,
                                              const size_t len) {
  fio_json_result_s r = {.stop_pos = 0, .err = 0};
  fio_json_parser_callbacks_s callbacks;
  callbacks.on_map = fio___json_parse_update_on_map;
  callbacks.on_array = fio___json_parse_update_on_array;
  fio___json_state_s state;
  void *ex_data[3] = {s, ctx, &state};

  if (!s->is_array)
    goto missing_callback;
  if (!s->is_map)
    goto missing_callback;
  if (fio___json_callbacks_validate(s))
    goto missing_callback;

  callbacks = *s;
  state = (fio___json_state_s){
      .cb = callbacks,
      .pos = start,
      .end = start + len,
  };
  state.ctx = (void *)ex_data;
  /* skip BOM, if exists */
  if (len >= 3 && state.pos[0] == (char)0xEF && state.pos[1] == (char)0xBB &&
      state.pos[2] == (char)0xBF) {
    state.pos += 3;
    if (len == 3)
      goto finish;
  }
  r.ctx = fio___json_consume(&state);
  r.err = state.error;
  r.stop_pos = state.pos - start;
  if (state.error)
    goto failed;
finish:
  return r;
failed:
  FIO_LOG_DEBUG(
      "JSON parsing failed after:\n%.*s",
      ((state.end - state.pos > 48) ? 48 : ((int)(state.end - state.pos))),
      state.pos);
  r.ctx = s->on_error(r.ctx);
  return r;
missing_callback:
  FIO_LOG_ERROR("JSON parser missing a critical callback!");
  r.err = 1;
  return r;
}
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_JSON
#endif /* FIO_JSON */
