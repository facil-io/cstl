/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                                JSON Parsing









***************************************************************************** */
#if defined(FIO_JSON) && !defined(H___FIO_JSON_H)
#define H___FIO_JSON_H

#ifndef JSON_MAX_DEPTH
/** Maximum allowed JSON nesting level. Values above 64K might fail. */
#define JSON_MAX_DEPTH 512
#endif

/** The JSON parser type. Memory must be initialized to 0 before first uses. */
typedef struct {
  /** level of nesting. */
  uint32_t depth;
  /** expectation bit flag: 0=key, 1=colon, 2=value, 4=comma/closure . */
  uint8_t expect;
  /** nesting bit flags - dictionary bit = 0, array bit = 1. */
  uint8_t nesting[(JSON_MAX_DEPTH + 7) >> 3];
} fio_json_parser_s;

#define FIO_JSON_INIT                                                          \
  { .depth = 0 }

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
SFUNC size_t fio_json_parse(fio_json_parser_s *parser,
                            const char *buffer,
                            const size_t len);

/* *****************************************************************************
JSON Parsing - Implementation - Helpers and Callbacks


Note: static Callacks must be implemented in the C file that uses the parser

Note: a Helper API is provided for the parsing implementation.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/** common FIO_JSON callback function properties */
#define FIO_JSON_CB static inline __attribute__((unused))

/* *****************************************************************************
JSON Parsing - Helpers API
***************************************************************************** */

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_in_array(fio_json_parser_s *parser);

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_in_object(fio_json_parser_s *parser);

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_key(fio_json_parser_s *parser);

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_value(fio_json_parser_s *parser);

/* *****************************************************************************
JSON Parsing - Implementation - Callbacks
***************************************************************************** */

/** a NULL object was detected */
FIO_JSON_CB void fio_json_on_null(fio_json_parser_s *p);
/** a TRUE object was detected */
static inline void fio_json_on_true(fio_json_parser_s *p);
/** a FALSE object was detected */
FIO_JSON_CB void fio_json_on_false(fio_json_parser_s *p);
/** a Number was detected (long long). */
FIO_JSON_CB void fio_json_on_number(fio_json_parser_s *p, long long i);
/** a Float was detected (double). */
FIO_JSON_CB void fio_json_on_float(fio_json_parser_s *p, double f);
/** a String was detected (int / float). update `pos` to point at ending */
FIO_JSON_CB void fio_json_on_string(fio_json_parser_s *p,
                                    const void *start,
                                    size_t len);
/** a dictionary object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_object(fio_json_parser_s *p);
/** a dictionary object closure detected */
FIO_JSON_CB void fio_json_on_end_object(fio_json_parser_s *p);
/** an array object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_array(fio_json_parser_s *p);
/** an array closure was detected */
FIO_JSON_CB void fio_json_on_end_array(fio_json_parser_s *p);
/** the JSON parsing is complete */
FIO_JSON_CB void fio_json_on_json(fio_json_parser_s *p);
/** the JSON parsing encountered an error */
FIO_JSON_CB void fio_json_on_error(fio_json_parser_s *p);

/* *****************************************************************************
JSON Parsing - Implementation - Helpers and Parsing


Note: static Callacks must be implemented in the C file that uses the parser
***************************************************************************** */

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_in_array(fio_json_parser_s *p) {
  return p->depth && fio_bitmap_get(p->nesting, p->depth);
}

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_in_object(fio_json_parser_s *p) {
  return p->depth && !fio_bitmap_get(p->nesting, p->depth);
}

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_key(fio_json_parser_s *p) {
  return fio_json_parser_is_in_object(p) && !p->expect;
}

/** Tests the state of the JSON parser. Returns 1 for true and 0 for false. */
FIO_JSON_CB uint8_t fio_json_parser_is_value(fio_json_parser_s *p) {
  return !fio_json_parser_is_key(p);
}

FIO_IFUNC const char *fio___json_skip_comments(const char *buffer,
                                               const char *stop) {
  if (*buffer == '#' ||
      ((stop - buffer) > 2 && buffer[0] == '/' && buffer[1] == '/')) {
    /* EOL style comment, C style or Bash/Ruby style*/
    buffer = (const char *)memchr(buffer + 1, '\n', stop - (buffer + 1));
    return buffer;
  }
  if (((stop - buffer) > 3 && buffer[0] == '/' && buffer[1] == '*')) {
    while ((buffer = (const char *)memchr(buffer, '/', stop - buffer)) &&
           buffer && ++buffer && buffer[-2] != '*')
      ;
    return buffer;
  }
  return NULL;
}

FIO_IFUNC const char *fio___json_consume_string(fio_json_parser_s *p,
                                                const char *buffer,
                                                const char *stop) {
  const char *start = ++buffer;
  for (;;) {
    buffer = (const char *)memchr(buffer, '\"', stop - buffer);
    if (!buffer)
      return NULL;
    size_t escaped = 1;
    while (buffer[0 - escaped] == '\\')
      ++escaped;
    if (escaped & 1)
      break;
    ++buffer;
  }
  fio_json_on_string(p, start, buffer - start);
  return buffer + 1;
}

FIO_IFUNC const char *fio___json_consume_number(fio_json_parser_s *p,
                                                const char *buffer,
                                                const char *stop) {

  const char *const was = buffer;
  errno = 0; /* testo for E2BIG on number parsing */
  long long i = fio_atol((char **)&buffer);

  if (buffer < stop &&
      ((*buffer) == '.' || (*buffer | 32) == 'e' || (*buffer | 32) == 'x' ||
       (*buffer | 32) == 'p' || (*buffer | 32) == 'i' || errno)) {
    buffer = was;
    double f = fio_atof((char **)&buffer);
    fio_json_on_float(p, f);
  } else {
    fio_json_on_number(p, i);
  }
  return buffer;
}

FIO_IFUNC const char *fio___json_identify(fio_json_parser_s *p,
                                          const char *buffer,
                                          const char *stop) {
  /* Use `break` to change separator requirement status.
   * Use `continue` to keep separator requirement the same.
   */
  switch (*buffer) {
  case 0x09: /* fallthrough */
  case 0x0A: /* fallthrough */
  case 0x0D: /* fallthrough */
  case 0x20:
    /* consume whitespace */
    ++buffer;
    if (!((uintptr_t)buffer & 7)) {
      while (buffer + 8 < stop) {
        const uint64_t w1 = 0x0101010101010101 * 0x09;
        const uint64_t w2 = 0x0101010101010101 * 0x0A;
        const uint64_t w3 = 0x0101010101010101 * 0x0D;
        const uint64_t w4 = 0x0101010101010101 * 0x20;
        const uint64_t t1 = ~(w1 ^ (*(uint64_t *)(buffer)));
        const uint64_t t2 = ~(w2 ^ (*(uint64_t *)(buffer)));
        const uint64_t t3 = ~(w3 ^ (*(uint64_t *)(buffer)));
        const uint64_t t4 = ~(w4 ^ (*(uint64_t *)(buffer)));
        const uint64_t b1 =
            (((t1 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t1 & 0x8080808080808080ULL));
        const uint64_t b2 =
            (((t2 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t2 & 0x8080808080808080ULL));
        const uint64_t b3 =
            (((t3 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t3 & 0x8080808080808080ULL));
        const uint64_t b4 =
            (((t4 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t4 & 0x8080808080808080ULL));
        if ((b1 | b2 | b3 | b4) != 0x8080808080808080ULL)
          break;
        buffer += 8;
      }
    }
    return buffer;
  case ',': /* comma separator */
    if (!p->depth || !(p->expect & 4))
      goto unexpected_separator;
    ++buffer;
    p->expect = (fio_bitmap_get(p->nesting, p->depth) << 1);
    return buffer;
  case ':': /* colon separator */
    if (!p->depth || !(p->expect & 1))
      goto unexpected_separator;
    ++buffer;
    p->expect = 2;
    return buffer;
    /*
     *
     * JSON Strings
     *
     */
  case '"':
    if (p->depth && (p->expect & ((uint8_t)5)))
      goto missing_separator;
    buffer = fio___json_consume_string(p, buffer, stop);
    if (!buffer)
      goto unterminated_string;
    break;
    /*
     *
     * JSON Objects
     *
     */
  case '{':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    p->expect = 0;
    if (p->depth == JSON_MAX_DEPTH)
      goto too_deep;
    ++p->depth;
    fio_bitmap_unset(p->nesting, p->depth);
    fio_json_on_start_object(p);
    return buffer + 1;
  case '}':
    if (fio_bitmap_get(p->nesting, p->depth) || !p->depth || (p->expect & 3))
      goto object_closure_unexpected;
    fio_bitmap_unset(p->nesting, p->depth);
    p->expect = 4; /* expect comma */
    --p->depth;
    fio_json_on_end_object(p);
    return buffer + 1;
    /*
     *
     * JSON Arrays
     *
     */
  case '[':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    fio_json_on_start_array(p);
    p->expect = 2;
    if (p->depth == JSON_MAX_DEPTH)
      goto too_deep;
    ++p->depth;
    fio_bitmap_set(p->nesting, p->depth);
    return buffer + 1;
  case ']':
    if (!fio_bitmap_get(p->nesting, p->depth) || !p->depth)
      goto array_closure_unexpected;
    fio_bitmap_unset(p->nesting, p->depth);
    p->expect = 4; /* expect comma */
    --p->depth;
    fio_json_on_end_array(p);
    return buffer + 1;
    /*
     *
     * JSON Primitives (true / false / null (NaN))
     *
     */
  case 'N': /* NaN or null? - fallthrough */
  case 'n':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    if (buffer + 4 > stop || buffer[1] != 'u' || buffer[2] != 'l' ||
        buffer[3] != 'l') {
      if (buffer + 3 > stop || (buffer[1] | 32) != 'a' ||
          (buffer[2] | 32) != 'n')
        return NULL;
      char *nan_str = (char *)"NaN";
      fio_json_on_float(p, fio_atof(&nan_str));
      buffer += 3;
      break;
    }
    fio_json_on_null(p);
    buffer += 4;
    break;
  case 't': /* true */
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    if (buffer + 4 > stop || buffer[1] != 'r' || buffer[2] != 'u' ||
        buffer[3] != 'e')
      return NULL;
    fio_json_on_true(p);
    buffer += 4;
    break;
  case 'f': /* false */
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    if (buffer + 5 > stop || buffer[1] != 'a' || buffer[2] != 'l' ||
        buffer[3] != 's' || buffer[4] != 'e')
      return NULL;
    fio_json_on_false(p);
    buffer += 5;
    break;
    /*
     *
     * JSON Numbers (Integers / Floats)
     *
     */
  case '+': /* fallthrough */
  case '-': /* fallthrough */
  case '0': /* fallthrough */
  case '1': /* fallthrough */
  case '2': /* fallthrough */
  case '3': /* fallthrough */
  case '4': /* fallthrough */
  case '5': /* fallthrough */
  case '6': /* fallthrough */
  case '7': /* fallthrough */
  case '8': /* fallthrough */
  case '9': /* fallthrough */
  case 'x': /* fallthrough */
  case '.': /* fallthrough */
  case 'e': /* fallthrough */
  case 'E': /* fallthrough */
  case 'i': /* fallthrough */
  case 'I':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    buffer = fio___json_consume_number(p, buffer, stop);
    if (!buffer)
      goto bad_number_format;
    break;
    /*
     *
     * Comments
     *
     */
  case '#': /* fallthrough */
  case '/': /* fallthrough */
    return fio___json_skip_comments(buffer, stop);
    /*
     *
     * Unrecognized Data Handling
     *
     */
  default:
    FIO_LOG_DEBUG("unrecognized JSON identifier at:\n%.*s",
                  ((stop - buffer > 48) ? (int)48 : ((int)(stop - buffer))),
                  buffer);
    return NULL;
  }
  /* p->expect should be either 0 (key) or 2 (value) */
  p->expect = (p->expect << 1) + ((p->expect ^ 2) >> 1);
  return buffer;

missing_separator:
  FIO_LOG_DEBUG("missing JSON separator '%c' at (%d):\n%.*s",
                (p->expect == 2 ? ':' : ','),
                p->expect,
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))),
                buffer);
  fio_json_on_error(p);
  return NULL;
unexpected_separator:
  FIO_LOG_DEBUG("unexpected JSON separator at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))),
                buffer);
  fio_json_on_error(p);
  return NULL;
unterminated_string:
  FIO_LOG_DEBUG("unterminated JSON string at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))),
                buffer);
  fio_json_on_error(p);
  return NULL;
bad_number_format:
  FIO_LOG_DEBUG("bad JSON numeral format at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))),
                buffer);
  fio_json_on_error(p);
  return NULL;
array_closure_unexpected:
  FIO_LOG_DEBUG("JSON array closure unexpected at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))),
                buffer);
  fio_json_on_error(p);
  return NULL;
object_closure_unexpected:
  FIO_LOG_DEBUG("JSON object closure unexpected at (%d):\n%.*s",
                p->expect,
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))),
                buffer);
  fio_json_on_error(p);
  return NULL;
too_deep:
  FIO_LOG_DEBUG("JSON object nesting too deep at:\n%.*s",
                p->expect,
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))),
                buffer);
  fio_json_on_error(p);
  return NULL;
}

/**
 * Returns the number of bytes consumed. Stops as close as possible to the end
 * of the buffer or once an object parsing was completed.
 */
SFUNC size_t fio_json_parse(fio_json_parser_s *p,
                            const char *buffer,
                            const size_t len) {
  const char *start = buffer;
  const char *stop = buffer + len;
  const char *last;
  /* skip BOM, if exists */
  if (len >= 3 && buffer[0] == (char)0xEF && buffer[1] == (char)0xBB &&
      buffer[2] == (char)0xBF) {
    buffer += 3;
    if (len == 3)
      goto finish;
  }
  /* loop until the first JSON data was read */
  do {
    last = buffer;
    buffer = fio___json_identify(p, buffer, stop);
    if (!buffer)
      goto failed;
  } while (!p->expect && buffer < stop);
  /* loop until the JSON object (nesting) is closed */
  while (p->depth && buffer < stop) {
    last = buffer;
    buffer = fio___json_identify(p, buffer, stop);
    if (!buffer)
      goto failed;
  }
  if (!p->depth) {
    p->expect = 0;
    fio_json_on_json(p);
  }
finish:
  return buffer - start;
failed:
  FIO_LOG_DEBUG("JSON parsing failed after:\n%.*s",
                ((stop - last > 48) ? 48 : ((int)(stop - last))),
                last);
  return last - start;
}

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_JSON
#endif /* FIO_JSON */
