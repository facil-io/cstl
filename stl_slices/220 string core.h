/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_STR                     /*Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                      Binary Safe String Core Helpers




***************************************************************************** */
#if defined(FIO_STR) && !defined(H__FIO_STR__H)
#define H__FIO_STR__H

/* *****************************************************************************
String Authorship Helpers (`fio_string_write` functions)
***************************************************************************** */

typedef void (*fio_string_realloc_fn)(fio_str_info_s *, size_t);
/**
 * Writes data to the end of the string in the `fio_string_s` struct,
 * returning an updated `fio_string_s` struct.
 *
 * The returned string is NUL terminated if edited.
 *
 * * `dest` an `fio_string_s` struct containing the destination string.
 *
 * * `reallocate` is a callback that attempts to reallocate more memory (i.e.,
 * using realloc) and returns an updated `fio_string_s` struct containing the
 *   updated capacity and buffer pointer (as well as the original length).
 *
 *   On failure the original `fio_string_s` should be returned. if
 * `reallocate` is NULL or fails, the data copied will be truncated.
 *
 * * `src` is the data to be written to the end of `dest`.
 *
 * * `len` is the length of the data to be written to the end of `dest`.
 *
 * Note: this function performs only minimal checks and assumes that `dest` is
 *       fully valid - i.e., that `dest.capa >= dest.len`, that `dest.buf` is
 *       valid, etc'.
 *
 * An example for a `reallocate` callback using the system's `realloc` function:
 *
 *      fio_str_info_s fio_string_realloc_system(fio_str_info_s dest,
 *                                                size_t new_capa) {
 *       void *tmp = realloc(dest.buf, new_capa);
 *       if (!tmp)
 *         return dest;
 *       dest.capa = new_capa;
 *       dest.buf = (char *)tmp;
 *       return dest;
 *     }
 *
 * An example for using the function:
 *
 *     void example(void) {
 *       char buf[32];
 *       fio_str_info_s str = FIO_STR_INFO3(buf, 0, 32);
 *       fio_string_write(&str, NULL, "The answer is: 0x", 17);
 *       str.len += fio_ltoa(str.buf + str.len, 42, 16);
 *       fio_string_write(&str, NULL, "!\n", 2);
 *       printf("%s", str.buf);
 *     }
 */
FIO_SFUNC int fio_string_write(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               const void *src,
                               size_t len);
/* Writes a signed number `i` to the String */
SFUNC int fio_string_write_i(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             int64_t i);
/* Writes an unsigned number `i` to the String */
SFUNC int fio_string_write_u(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             uint64_t i);
/* Writes a hex representation of `i` to the String */
SFUNC int fio_string_write_hex(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i);
/* Writes a binary representation of `i` to the String */
SFUNC int fio_string_write_bin(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i);

/**
 * Similar to fio_string_write, only replacing a sub-string or inserting a
 * string in a specific location.
 */
SFUNC int fio_string_insert(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            intptr_t start_pos,
                            size_t overwrite_len,
                            const void *src,
                            size_t len);

/** Argument type used by fio_string_write2. */
typedef struct {
  size_t klass;
  union {
    fio_str_info_s str;
    double f;
    int64_t i;
    uint64_t u;
  } info;
} fio_string_write_s;

/** Similar to fio_string_write, only using printf semantics. */
SFUNC int fio_string_printf(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            const char *format,
                            ...);

/** Similar to fio_string_write, only using vprintf semantics. */
SFUNC int fio_string_vprintf(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             const char *format,
                             va_list argv);

/**
 * Writes a group of objects (strings, numbers, etc') to `dest`.
 *
 * `dest` and `reallocate` are similar to `fio_string_write`.
 *
 * `src` is an array of `fio_string_write_s` structs, ending with a struct
 * that's all set to 0.
 *
 * Use the `fio_string_write2` macro for ease, i.e.:
 *
 *    fio_str_info_s str = {0};
 *    fio_string_write2(&str, my_reallocate,
 *                        FIO_STRING_WRITE_STR1("The answer is: "),
 *                        FIO_STRING_WRITE_NUM(42),
 *                        FIO_STRING_WRITE_STR2("(0x", 3),
 *                        FIO_STRING_WRITE_HEX(42),
 *                        FIO_STRING_WRITE_STR2(")", 1));
 *
 * Note: this function might end up allocating more memory than absolutely
 * required as it favors fast performance over memory savings. It performs only
 * a single allocation (if any) and computes numeral string length only when
 * writing the numbers to the string.
 */
SFUNC int fio_string_write2(fio_str_info_s *restrict dest,
                            fio_string_realloc_fn reallocate,
                            const fio_string_write_s srcs[]);

/* Helper macro for fio_string_write2 */
#define fio_string_write2(dest, reallocate, ...)                               \
  fio_string_write2((dest),                                                    \
                    (reallocate),                                              \
                    (fio_string_write_s[]){__VA_ARGS__, {0}})

/** A macro to add a String to `fio_string_write2`. */
#define FIO_STRING_WRITE_STR1(str_)                                            \
  ((fio_string_write_s){.klass = 1,                                            \
                        .info.str = {.len = strlen((str_)), .buf = (str_)}})

/** A macro to add a String with known length to `fio_string_write2`. */
#define FIO_STRING_WRITE_STR2(str_, len_)                                      \
  ((fio_string_write_s){.klass = 1, .info.str = {.len = (len_), .buf = (str_)}})

/** A macro to add a signed number to `fio_string_write2`. */
#define FIO_STRING_WRITE_NUM(num)                                              \
  ((fio_string_write_s){.klass = 2, .info.i = (int64_t)(num)})

/** A macro to add an unsigned number to `fio_string_write2`. */
#define FIO_STRING_WRITE_UNUM(num)                                             \
  ((fio_string_write_s){.klass = 3, .info.u = (uint64_t)(num)})

/** A macro to add a hex representation to `fio_string_write2`. */
#define FIO_STRING_WRITE_HEX(num)                                              \
  ((fio_string_write_s){.klass = 4, .info.u = (uint64_t)(num)})

/** A macro to add a binary representation to `fio_string_write2`. */
#define FIO_STRING_WRITE_BIN(num)                                              \
  ((fio_string_write_s){.klass = 5, .info.u = (uint64_t)(num)})

/** A macro to add a float (double) to `fio_string_write2`. */
#define FIO_STRING_WRITE_FLOAT(num)                                            \
  ((fio_string_write_s){.klass = 6, .info.f = (double)(num)})

/* *****************************************************************************
String C / JSON escaping
***************************************************************************** */

/**
 * Writes data at the end of the String, escaping the data using JSON semantics.
 *
 * The JSON semantic are common to many programming languages, promising a UTF-8
 * String while making it easy to read and copy the string during debugging.
 */
SFUNC int fio_string_write_escape(fio_str_info_s *restrict dest,
                                  fio_string_realloc_fn reallocate,
                                  const void *src,
                                  size_t len);

/** Writes an escaped data into the string after un-escaping the data. */
SFUNC int fio_string_write_unescape(fio_str_info_s *dest,
                                    fio_string_realloc_fn reallocate,
                                    const void *src,
                                    size_t len);

/* *****************************************************************************
String Base64 support
***************************************************************************** */

/** Writes data to String using Base64 encoding. */
SFUNC int fio_string_write_base64enc(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *data,
                                     size_t data_len,
                                     uint8_t url_encoded);

/** Writes decoded base64 data to String. */
SFUNC int fio_string_write_base64dec(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *encoded,
                                     size_t encoded_len);

/* *****************************************************************************
Memory Helpers (for Authorship)
***************************************************************************** */

/** Default reallocation callback implementation */
#define FIO_STRING_REALLOC fio_string_default_reallocate
/** Default reallocation callback for memory that mustn't be freed. */
#define FIO_STRING_ALLOC_COPY fio_string_default_copy_and_reallocate
/** Frees memory that was allocated with the default callbacks. */
#define FIO_STRING_FREE fio_string_default_free
/** Frees memory that was allocated with the default callbacks. */
#define FIO_STRING_FREE2 fio_string_default_free2
/** Does nothing. */
#define FIO_STRING_FREE_NOOP fio_string_default_free_noop

/** default reallocation callback implementation */
SFUNC void fio_string_default_reallocate(fio_str_info_s *dest, size_t new_capa);
/** default reallocation callback for memory that mustn't be freed. */
SFUNC void fio_string_default_copy_and_reallocate(fio_str_info_s *dest,
                                                  size_t new_capa);
/** frees memory that was allocated with the default callbacks. */
SFUNC void fio_string_default_free(void *);
/** frees memory that was allocated with the default callbacks. */
SFUNC void fio_string_default_free2(fio_str_info_s str);
/** does nothing. */
SFUNC void fio_string_default_free_noop(fio_str_info_s str);

/* *****************************************************************************
UTF-8 Support
***************************************************************************** */

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t fio_string_utf8_valid(fio_str_info_s str);

/** Returns the String's length in UTF-8 characters. */
SFUNC size_t fio_string_utf8_len(fio_str_info_s str);

/**
 * Takes a UTF-8 character selection information (UTF-8 position and length)
 * and updates the same variables so they reference the raw byte slice
 * information.
 *
 * If the String isn't UTF-8 valid up to the requested selection, than `pos`
 * will be updated to `-1` otherwise values are always positive.
 *
 * The returned `len` value may be shorter than the original if there wasn't
 * enough data left to accommodate the requested length. When a `len` value of
 * `0` is returned, this means that `pos` marks the end of the String.
 *
 * Returns -1 on error and 0 on success.
 */
SFUNC int fio_string_utf8_select(fio_str_info_s str,
                                 intptr_t *pos,
                                 size_t *len);

/* *****************************************************************************
Sorting / Comparison Helpers
***************************************************************************** */

/**
 * Compares two `fio_buf_info_s`, returning 1 if data in a is bigger than b.
 *
 * Note: returns 0 if data in b is bigger than or equal(!).
 */
SFUNC int fio_string_is_greater_buf(fio_buf_info_s a, fio_buf_info_s b);

/**
 * Compares two strings, returning 1 if string a is bigger than string b.
 *
 * Note: returns 0 if string b is bigger than string a or if strings are equal.
 */
FIO_IFUNC int fio_string_is_greater(fio_str_info_s a, fio_str_info_s b);

/* *****************************************************************************


                             String Implementation

                           IMPLEMENTATION - INLINED


***************************************************************************** */

/* *****************************************************************************
String Authorship Helpers - (inlined) implementation
***************************************************************************** */

/* performs `reallocate` if necessary, `capa` rounded up to 16 byte units. */
FIO_IFUNC size_t fio_string___write_validate_len(
    fio_str_info_s *dest,
    void (*reallocate)(fio_str_info_s *, size_t new_capa),
    size_t len) {
  if (reallocate && (dest->capa < dest->len + len + 1)) {
    const size_t new_len = dest->len + len;
    const size_t new_capa =
        (new_len + 15LL + (!(new_len & 15ULL))) & (~((size_t)15ULL));
    reallocate(dest, new_capa);
  }
  if (FIO_UNLIKELY(dest->capa < dest->len + len + 1)) {
    len = dest->capa - (dest->len + 1);
  }
  return len;
}

/* fio_string_write */
FIO_SFUNC int fio_string_write(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               const void *src,
                               size_t len) {
  int r = 0;
  len = fio_string___write_validate_len(dest, reallocate, len);
  if (FIO_UNLIKELY(dest->capa < dest->len + 2)) {
    r = -1;
    return r;
  }
  if (FIO_LIKELY(len && src))
    FIO_MEMCPY(dest->buf + dest->len, src, len);
  dest->len += len;
  dest->buf[dest->len] = 0;
  return r;
}

/**
 * Compares two strings, returning 1 if string a is bigger than string b.
 *
 * Note: returns 0 if string b is bigger than string a or if strings are equal.
 */
FIO_IFUNC int fio_string_is_greater(fio_str_info_s a, fio_str_info_s b) {
  return fio_string_is_greater_buf(FIO_STR2BUF_INFO(a), FIO_STR2BUF_INFO(b));
}

/* *****************************************************************************
Extern-ed functions
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
Allocation Helpers
***************************************************************************** */
SFUNC void fio_string_default_reallocate(fio_str_info_s *dest,
                                         size_t new_capa) {
  void *tmp = FIO_MEM_REALLOC_(dest->buf, dest->capa, new_capa, dest->len);
  if (!tmp)
    return;
  dest->capa = new_capa;
  dest->buf = (char *)tmp;
}

SFUNC void fio_string_default_copy_and_reallocate(fio_str_info_s *dest,
                                                  size_t new_capa) {
  void *tmp = FIO_MEM_REALLOC_(NULL, 0, new_capa, 0);
  if (!tmp)
    return;
  dest->capa = new_capa;
  dest->buf = (char *)tmp;
  if (dest->len)
    FIO_MEMCPY(tmp, dest->buf, dest->len);
}

SFUNC void fio_string_default_free(void *ptr) { FIO_MEM_FREE_(ptr, 0); }
SFUNC void fio_string_default_free2(fio_str_info_s str) {
  FIO_MEM_FREE_(str.buf, str.capa);
}

SFUNC void fio_string_default_free_noop(fio_str_info_s str) { (void)str; }

/* *****************************************************************************
Numeral Support
***************************************************************************** */

/* fio_string_write_i */
SFUNC int fio_string_write_i(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             int64_t i) {
  int r = -1;
  char buf[32];
  size_t inv = i < 0;
  if (inv) {
    i = 0 - i;
  }
  size_t len = 0;
  while (i) {
    uint64_t nxt = (uint64_t)i / 10;
    buf[len++] = '0' + ((uint64_t)i - (nxt * 10));
    i = (int64_t)nxt;
  }
  len += inv;
  if (fio_string___write_validate_len(dest, reallocate, len) != len)
    return r;
  r = 0;
  while (len) {
    dest->buf[dest->len++] = buf[--len];
  }
  dest->buf[dest->len] = '-';
  dest->len += inv;
  dest->buf[dest->len] = 0;
  return r;
}

/* fio_string_write_u */
SFUNC int fio_string_write_u(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             uint64_t i) {
  int r = -1;
  char buf[32];
  size_t len = 0;
  while (i) {
    uint64_t nxt = i / 10;
    buf[len++] = '0' + (i - (nxt * 10));
    i = nxt;
  }
  if (fio_string___write_validate_len(dest, reallocate, len) != len)
    return r;
  r = 0;
  while (len) {
    dest->buf[dest->len++] = buf[--len];
  }
  dest->buf[dest->len] = 0;
  return r;
}

/* fio_string_write_hex */
SFUNC int fio_string_write_hex(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i) {
  int r = -1;
  char buf[16];
  size_t len = 0;
  while (i) {
    buf[len++] = fio_i2c(i & 15);
    i >>= 4;
    buf[len++] = fio_i2c(i & 15);
    i >>= 4;
  }
  if (fio_string___write_validate_len(dest, reallocate, len) != len)
    return r;
  r = 0;
  while (len) {
    dest->buf[dest->len++] = buf[--len];
  }
  dest->buf[dest->len] = 0;
  return r;
}

/* fio_string_write_hex */
SFUNC int fio_string_write_bin(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i) {
  int r = -1;
  char buf[64];
  size_t len = 0;
  while (i) {
    buf[len++] = '0' + (i & 1);
    i >>= 1;
    buf[len++] = '0' + (i & 1);
    i >>= 1;
  }
  if (fio_string___write_validate_len(dest, reallocate, len) != len)
    return r;
  r = 0;
  while (len) {
    dest->buf[dest->len++] = buf[--len];
  }
  dest->buf[dest->len] = 0;
  return r;

  return r;
}

/* *****************************************************************************
`printf` Style Support
***************************************************************************** */

/* Similar to fio_string_write, only using vprintf semantics. */
SFUNC int __attribute__((format(FIO___PRINTF_STYLE, 3, 0)))
fio_string_vprintf(fio_str_info_s *dest,
                   fio_string_realloc_fn reallocate,
                   const char *format,
                   va_list argv) {
  int r = 0;
  va_list argv_cpy;
  va_copy(argv_cpy, argv);
  int len = vsnprintf(NULL, 0, format, argv_cpy);
  va_end(argv_cpy);
  if (len <= 0)
    return -1;
  r = len;
  len = fio_string___write_validate_len(dest, reallocate, len);
  r = -1 + (r == len);
  if (FIO_UNLIKELY(dest->capa < dest->len + 2))
    return -1;
  vsnprintf(dest->buf + dest->len, len + 1, format, argv);
  dest->len += len;
  dest->buf[dest->len] = 0;
  return r;
}

/** Similar to fio_string_write, only using printf semantics. */
SFUNC int __attribute__((format(FIO___PRINTF_STYLE, 3, 4)))
fio_string_printf(fio_str_info_s *dest,
                  fio_string_realloc_fn reallocate,
                  const char *format,
                  ...) {
  int r = 0;
  va_list argv;
  va_start(argv, format);
  r = fio_string_vprintf(dest, reallocate, format, argv);
  va_end(argv);
  return r;
}

/* *****************************************************************************
UTF-8 Support
***************************************************************************** */
/**
 * Maps the first 5 bits in a byte (0b11111xxx) to a UTF-8 codepoint length.
 *
 * Codepoint length 0 == error.
 *
 * The first valid length can be any value between 1 to 4.
 *
 * A continuation byte (second, third or forth) valid length marked as 5.
 *
 * To map was populated using the following Ruby script:
 *
 *      map = []; 32.times { map << 0 }; (0..0b1111).each {|i| map[i] = 1} ;
 *      (0b10000..0b10111).each {|i| map[i] = 5} ;
 *      (0b11000..0b11011).each {|i| map[i] = 2} ;
 *      (0b11100..0b11101).each {|i| map[i] = 3} ;
 *      map[0b11110] = 4; map;
 */
static __attribute__((unused)) uint8_t fio__string_utf8_map[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    5, 5, 5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 3, 3, 4, 0};

/**
 * Advances the `ptr` by one utf-8 character, placing the value of the UTF-8
 * character into the i32 variable (which must be a signed integer with 32bits
 * or more). On error, `i32` will be equal to `-1` and `ptr` will not step
 * forwards.
 *
 * The `end` value provides overflow protection.
 */
#define FIO_STR_UTF8_CODE_POINT(ptr, end, i32)                                 \
  do {                                                                         \
    switch (fio__string_utf8_map[((uint8_t *)(ptr))[0] >> 3]) {                \
    case 1:                                                                    \
      (i32) = ((uint8_t *)(ptr))[0];                                           \
      ++(ptr);                                                                 \
      break;                                                                   \
    case 2:                                                                    \
      if (((ptr) + 2 > (end)) ||                                               \
          fio__string_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5) {             \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) =                                                                  \
          ((((uint8_t *)(ptr))[0] & 31) << 6) | (((uint8_t *)(ptr))[1] & 63);  \
      (ptr) += 2;                                                              \
      break;                                                                   \
    case 3:                                                                    \
      if (((ptr) + 3 > (end)) ||                                               \
          fio__string_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5 ||             \
          fio__string_utf8_map[((uint8_t *)(ptr))[2] >> 3] != 5) {             \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) = ((((uint8_t *)(ptr))[0] & 15) << 12) |                           \
              ((((uint8_t *)(ptr))[1] & 63) << 6) |                            \
              (((uint8_t *)(ptr))[2] & 63);                                    \
      (ptr) += 3;                                                              \
      break;                                                                   \
    case 4:                                                                    \
      if (((ptr) + 4 > (end)) ||                                               \
          fio__string_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5 ||             \
          fio__string_utf8_map[((uint8_t *)(ptr))[2] >> 3] != 5 ||             \
          fio__string_utf8_map[((uint8_t *)(ptr))[3] >> 3] != 5) {             \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) = ((((uint8_t *)(ptr))[0] & 7) << 18) |                            \
              ((((uint8_t *)(ptr))[1] & 63) << 12) |                           \
              ((((uint8_t *)(ptr))[2] & 63) << 6) |                            \
              (((uint8_t *)(ptr))[3] & 63);                                    \
      (ptr) += 4;                                                              \
      break;                                                                   \
    default: (i32) = -1; break;                                                \
    }                                                                          \
  } while (0);

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t fio_string_utf8_valid(fio_str_info_s str) {
  if (!str.len)
    return 1;
  char *const end = str.buf + str.len;
  int32_t c = 0;
  do {
    FIO_STR_UTF8_CODE_POINT(str.buf, end, c);
  } while (c > 0 && str.buf < end);
  return str.buf == end && c >= 0;
}

/** Returns the String's length in UTF-8 characters. */
SFUNC size_t fio_string_utf8_len(fio_str_info_s str) {
  if (!str.len)
    return 0;
  char *end = str.buf + str.len;
  size_t utf8len = 0;
  int32_t c = 0;
  do {
    ++utf8len;
    FIO_STR_UTF8_CODE_POINT(str.buf, end, c);
  } while (c > 0 && str.buf < end);
  if (str.buf != end || c == -1) {
    /* invalid */
    return 0;
  }
  return utf8len;
}

/**
 * Takes a UTF-8 character selection information (UTF-8 position and length)
 * and updates the same variables so they reference the raw byte slice
 * information.
 *
 * If the String isn't UTF-8 valid up to the requested selection, than `pos`
 * will be updated to `-1` otherwise values are always positive.
 *
 * The returned `len` value may be shorter than the original if there wasn't
 * enough data left to accommodate the requested length. When a `len` value of
 * `0` is returned, this means that `pos` marks the end of the String.
 *
 * Returns -1 on error and 0 on success.
 */
SFUNC int fio_string_utf8_select(fio_str_info_s str,
                                 intptr_t *pos,
                                 size_t *len) {
  int32_t c = 0;
  char *p = str.buf;
  char *const end = str.buf + str.len;
  size_t start;

  if (!str.buf)
    goto error;
  if (!str.len || *pos == -1)
    goto at_end;

  if (*pos) {
    if ((*pos) > 0) {
      start = *pos;
      while (start && p < end && c >= 0) {
        FIO_STR_UTF8_CODE_POINT(p, end, c);
        --start;
      }
      if (c == -1)
        goto error;
      if (start || p >= end)
        goto at_end;
      *pos = p - str.buf;
    } else {
      /* walk backwards */
      p = str.buf + str.len - 1;
      c = 0;
      ++*pos;
      do {
        switch (fio__string_utf8_map[((uint8_t *)p)[0] >> 3]) {
        case 5: ++c; break;
        case 4:
          if (c != 3)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 3:
          if (c != 2)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 2:
          if (c != 1)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 1:
          if (c)
            goto error;
          ++(*pos);
          break;
        default: goto error;
        }
        --p;
      } while (p > str.buf && *pos);
      if (c)
        goto error;
      ++p; /* There's always an extra back-step */
      *pos = (p - str.buf);
    }
  }

  /* find end */
  start = *len;
  while (start && p < end && c >= 0) {
    FIO_STR_UTF8_CODE_POINT(p, end, c);
    --start;
  }
  if (c == -1 || p > end)
    goto error;
  *len = p - (str.buf + (*pos));
  return 0;

at_end:
  *pos = str.len;
  *len = 0;
  return 0;
error:
  *pos = -1;
  *len = 0;
  return -1;
}

/* *****************************************************************************
fio_string_is_greater
***************************************************************************** */

/**
 * Compares two `fio_buf_info_s`, returning 1 if data in a is bigger than b.
 *
 * Note: returns 0 if data in b is bigger than or equal(!).
 */
SFUNC int fio_string_is_greater_buf(fio_buf_info_s a, fio_buf_info_s b) {
  const size_t a_len_is_bigger = a.len > b.len;
  const size_t len = a_len_is_bigger ? b.len : a.len; /* shared length */
  if (a.buf == b.buf)
    return a_len_is_bigger;
  uint64_t ua;
  uint64_t ub;
  if (len > 7)
    for (size_t i = 0; i < len; i += 8) {
      ua = fio_buf2u64(a.buf);
      ub = fio_buf2u64(b.buf);
      if (ua != ub)
        return ua > ub;
      a.buf += 8;
      b.buf += 8;
    }
  if (len & 4) {
    ua = fio_buf2u32(a.buf);
    ub = fio_buf2u32(b.buf);
    if (ua != ub)
      return ua > ub;
    a.buf += 4;
    b.buf += 4;
  }
  ua = 0;
  ub = 0;
  switch ((len & 7)) { // clang-format off
  case 3: ua |= ((uint64_t)a.buf[2] << 40); ub |= ((uint64_t)b.buf[2] << 40); /* fall through */
  case 2: ua |= ((uint64_t)a.buf[1] << 48); ub |= ((uint64_t)b.buf[1] << 48); /* fall through */
  case 1: ua |= ((uint64_t)a.buf[0] << 56); ub |= ((uint64_t)b.buf[0] << 56); /* fall through */
  case 0: // clang-format on
    if (ua > ub)
      return 1;
  }
  return a_len_is_bigger & (ua == ub);
}

/* *****************************************************************************
Insert / Write2
***************************************************************************** */

/* fio_string_insert */
SFUNC int fio_string_insert(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            intptr_t start_pos,
                            size_t overwrite_len,
                            const void *src,
                            size_t len) {
  int r = 0;
  if (start_pos < 0) {
    start_pos = dest->len + start_pos;
    if (start_pos < 0)
      start_pos = 0;
  }
  if (dest->len < (size_t)start_pos + len + 1) {
    if ((size_t)start_pos < dest->len)
      dest->len = start_pos;
    return fio_string_write(dest, reallocate, src, len);
  }
  size_t move_start = start_pos + overwrite_len;
  size_t move_len = dest->len - (start_pos + overwrite_len);
  if (overwrite_len < len) {
    /* adjust for possible memory expansion */
    const size_t extra = len - overwrite_len;
    if (dest->capa < dest->len + extra + 1) {
      if (reallocate) {
        const size_t new_len = dest->len + extra;
        const size_t new_capa =
            (new_len + 15LL + (!(new_len & 15ULL))) & (~((size_t)15ULL));
        reallocate(dest, new_capa);
      }
      if (FIO_UNLIKELY(dest->capa < dest->len + extra + 1)) {
        move_len -= (dest->len + extra + 1) - dest->capa;
        r = -1;
      }
    }
  }
  memmove(dest->buf + start_pos + len, dest->buf + move_start, move_len);
  memcpy(dest->buf + start_pos, src, len);
  dest->len = start_pos + len + move_len;
  dest->buf[dest->len] = 0;
  return r;
}

/* IDE marker */
void fio_string_write2____(void);
/* the fio_string_write2 is a printf alternative. */
SFUNC int fio_string_write2 FIO_NOOP(fio_str_info_s *restrict dest,
                                     void (*reallocate)(fio_str_info_s *,
                                                        size_t new_capa),
                                     const fio_string_write_s srcs[]) {
  int r = 0;
  const fio_string_write_s *pos = srcs;
  size_t len = 0;

  while (pos->klass) {
    switch (pos->klass) { /* use more memory rather then calculate twice. */
    case 2: /* number */ len += 20; break;
    case 3: /* unsigned */ len += 20; break;
    case 4: /* hex */ len += 16; break;
    case 5: /* binary */ len += 64; break;
    case 6: /* float */ len += 18; break;
    default: len += pos->info.str.len;
    }
    ++pos;
  }
  pos = srcs;
  if (fio_string___write_validate_len(dest, reallocate, len) != len)
    goto truncate;
  while (pos->klass) {
    switch (pos->klass) {
    case 2: fio_string_write_i(dest, NULL, pos->info.i); break;   /* number */
    case 3: fio_string_write_u(dest, NULL, pos->info.u); break;   /* unsigned */
    case 4: fio_string_write_hex(dest, NULL, pos->info.u); break; /* hex */
    case 5: fio_string_write_bin(dest, NULL, pos->info.u); break; /* binary */
    case 6:                                                       /* float */
      dest->len += snprintf(dest->buf + dest->len, 19, "%.15g", pos->info.f);
      break;
    default:
      FIO_MEMCPY(&dest->buf[dest->len], pos->info.str.buf, pos->info.str.len);
      dest->len += pos->info.str.len;
    }
    ++pos;
  }
finish:
  dest->buf[dest->len] = 0;
  return r;
truncate:
  r = -1;
  while (pos->klass) {
    switch (pos->klass) {
    case 2:
      if (fio_string_write_i(dest, NULL, pos->info.i))
        goto finish;
      break; /* number */
    case 3:
      if (fio_string_write_u(dest, NULL, pos->info.u))
        goto finish;
      break; /* unsigned */
    case 4:
      if (fio_string_write_hex(dest, NULL, pos->info.u))
        goto finish;
      break; /* hex */
    case 5:
      if (fio_string_write_bin(dest, NULL, pos->info.u))
        goto finish;
      break; /* binary */
    case 6:  /* float */
      len = snprintf(dest->buf + dest->len, 19, "%.15g", pos->info.f);
      if (dest->capa < dest->len + len + 2)
        goto finish;
      dest->len += len;
      break;
    default:
      if (fio_string_write(dest, NULL, pos->info.str.buf, pos->info.str.len))
        goto finish;
    }
    ++pos;
  }
  goto finish;
}

/* *****************************************************************************
String C / JSON escaping
***************************************************************************** */

/**
 * Writes data at the end of the String, escaping the data using JSON semantics.
 *
 * The JSON semantic are common to many programming languages, promising a UTF-8
 * String while making it easy to read and copy the string during debugging.
 */
SFUNC int fio_string_write_escape(fio_str_info_s *restrict dest,
                                  fio_string_realloc_fn reallocate,
                                  const void *src_,
                                  size_t len) {
  int r = 0;
  if ((!len | !src_ | !dest))
    return r;
  const uint8_t *src = (const uint8_t *)src_;
  size_t at = 0;
  uint8_t set_at = 1;
  size_t escaped_len = len;

  /* collect escaping requirements */
  for (size_t i = 0; i < len; ++i) {
    /* skip valid ascii */
    if ((src[i] > 34 && src[i] < 127 && src[i] != '\\') || src[i] == '!' ||
        src[i] == ' ')
      continue;
    /* skip valid UTF-8 */
    switch (fio__string_utf8_map[src[i] >> 3]) {
    case 4:
      if (fio__string_utf8_map[src[i + 3] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 3:
      if (fio__string_utf8_map[src[i + 2] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 2:
      if (fio__string_utf8_map[src[i + 1] >> 3] != 5) {
        break; /* from switch */
      }
      i += fio__string_utf8_map[src[i] >> 3] - 1;
      continue;
    }
    /* store first instance of character that needs escaping */
    /* constant time (non-branching) alternative to if(`set_at`) */
    at ^= ((set_at | (0 - set_at)) & (i ^ at));
    set_at = 0;

    /* count extra bytes */
    switch (src[i]) {
    case '\b': /* fall through */
    case '\f': /* fall through */
    case '\n': /* fall through */
    case '\r': /* fall through */
    case '\t': /* fall through */
    case '"':  /* fall through */
    case '\\': /* fall through */
    case '/': ++escaped_len; break;
    default:
      /* escaping all control characters and non-UTF-8 characters */
      escaped_len += 3 + ((src[i] < 127) << 1);
    }
  }
  /* reserve space and copy any valid "head" */
  /* the +4 adds room for the usual use case of a following "\", \"" */
  if (fio_string___write_validate_len(dest, reallocate, escaped_len + 4) <
      escaped_len) {
    r = -1;
    len = (dest->capa + len) - (dest->len + escaped_len + 2);
    if (dest->capa < len + 2)
      return r;
  }

  uint8_t *writer = (uint8_t *)dest->buf + dest->len;
  /* is escaping required? - simple memcpy if we don't need to escape */
  if (set_at) {
    FIO_MEMCPY(writer, src, len);
    dest->len += len;
    return r;
  }
  /* simple memcpy until first char that needs escaping */
  if (at >= 8) {
    FIO_MEMCPY(writer, src, at);
  } else {
    at = 0;
  }
  /* start escaping */
  for (size_t i = at; i < len; ++i) {
    /* skip valid ascii */
    if ((src[i] > 34 && src[i] < 127 && src[i] != '\\') || src[i] == '!' ||
        src[i] == ' ') {
      writer[at++] = src[i];
      continue;
    }
    /* skip valid UTF-8 */
    switch (fio__string_utf8_map[src[i] >> 3]) {
    case 4:
      if (fio__string_utf8_map[src[i + 3] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 3:
      if (fio__string_utf8_map[src[i + 2] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 2:
      if (fio__string_utf8_map[src[i + 1] >> 3] != 5) {
        break; /* from switch */
      }
      switch (fio__string_utf8_map[src[i] >> 3]) {
      case 4: writer[at++] = src[i++]; /* fall through */
      case 3: writer[at++] = src[i++]; /* fall through */
      case 2: writer[at++] = src[i++]; writer[at++] = src[i];
      }
      continue;
    }

    /* write escape sequence */
    writer[at++] = '\\';
    switch (src[i]) {
    case '\b': writer[at++] = 'b'; break;
    case '\f': writer[at++] = 'f'; break;
    case '\n': writer[at++] = 'n'; break;
    case '\r': writer[at++] = 'r'; break;
    case '\t': writer[at++] = 't'; break;
    case '"': writer[at++] = '"'; break;
    case '\\': writer[at++] = '\\'; break;
    case '/': writer[at++] = '/'; break;
    default:
      /* escaping all control characters and non-UTF-8 characters */
      if (src[i] < 127) {
        writer[at++] = 'u';
        writer[at++] = '0';
        writer[at++] = '0';
        writer[at++] = fio_i2c(src[i] >> 4);
        writer[at++] = fio_i2c(src[i] & 15);
      } else {
        /* non UTF-8 data... encode as hex */
        writer[at++] = 'x';
        writer[at++] = fio_i2c(src[i] >> 4);
        writer[at++] = fio_i2c(src[i] & 15);
      }
    }
  }
  dest->len += at;
  dest->buf[dest->len] = 0;
  return r;
}

/**
 * Writes an escaped data into the string after unescaping the data.
 */
SFUNC int fio_string_write_unescape(fio_str_info_s *dest,
                                    fio_string_realloc_fn reallocate,
                                    const void *src_,
                                    size_t len) {
  int r = 0;
  size_t at = 0;
  size_t reduced = 0;
  if ((!len | !src_ | !dest))
    return r;
  { /* calculate assumed `len` reduction (minimal reduction) */
    const char *tmp = (const char *)src_;
    const char *stop = tmp + len - 1; /* avoid overflow for tmp[1] */
    for (;;) {
      tmp = (const char *)memchr(tmp, '\\', (size_t)(stop - tmp));
      if (!tmp)
        break;
      size_t step = 1;
      step += ((tmp[1] == 'x') << 1); /* step == 3 */
      step += (tmp[1] == 'u');        /* UTF-8 output <= 3 */
      reduced += step;
      tmp += step;
      if (tmp + 1 > stop)
        break;
    }
    FIO_ASSERT_DEBUG(reduced < len, "string unescape reduced too long");
    reduced = len - reduced;
  }
  at = fio_string___write_validate_len(dest, reallocate, reduced);
  if (reduced != at) {
    r = -1;
    len = at;
  }
  at = 0;
  const uint8_t *src = (const uint8_t *)src_;
  const uint8_t *end = src + len;
  uint8_t *writer = (uint8_t *)dest->buf + dest->len;
  while (src < end) {
    if (*src != '\\') {
      const uint8_t *escape_pos = (const uint8_t *)memchr(src, '\\', end - src);
      if (!escape_pos)
        escape_pos = end;
      const size_t valid_len = escape_pos - src;
      if (valid_len) {
        memmove(writer + at, src, valid_len);
        at += valid_len;
        src = escape_pos;
      }
    }
    if (end - src == 1) {
      writer[at++] = *(src++);
    }
    if (src >= end)
      break;
    /* escaped data - src[0] == '\\' */
    ++src;
    switch (src[0]) {
    case 'b':
      writer[at++] = '\b';
      ++src;
      break; /* from switch */
    case 'f':
      writer[at++] = '\f';
      ++src;
      break; /* from switch */
    case 'n':
      writer[at++] = '\n';
      ++src;
      break; /* from switch */
    case 'r':
      writer[at++] = '\r';
      ++src;
      break; /* from switch */
    case 't':
      writer[at++] = '\t';
      ++src;
      break; /* from switch */
    case 'u': {
      /* test UTF-8 notation */
      if (fio_c2i(src[1]) < 16 && fio_c2i(src[2]) < 16 &&
          fio_c2i(src[3]) < 16 && fio_c2i(src[4]) < 16) {
        uint32_t u = (((fio_c2i(src[1]) << 4) | fio_c2i(src[2])) << 8) |
                     ((fio_c2i(src[3]) << 4) | fio_c2i(src[4]));
        if (((fio_c2i(src[1]) << 4) | fio_c2i(src[2])) == 0xD8U &&
            src[5] == '\\' && src[6] == 'u' && fio_c2i(src[7]) < 16 &&
            fio_c2i(src[8]) < 16 && fio_c2i(src[9]) < 16 &&
            fio_c2i(src[10]) < 16) {
          /* surrogate-pair */
          u = (u & 0x03FF) << 10;
          u |= (((((fio_c2i(src[7]) << 4) | fio_c2i(src[8])) << 8) |
                 ((fio_c2i(src[9]) << 4) | fio_c2i(src[10]))) &
                0x03FF);
          u += 0x10000;
          src += 6;
        }
        if (u < 128) {
          writer[at++] = u;
        } else if (u < 2048) {
          writer[at++] = 192 | (u >> 6);
          writer[at++] = 128 | (u & 63);
        } else if (u < 65536) {
          writer[at++] = 224 | (u >> 12);
          writer[at++] = 128 | ((u >> 6) & 63);
          writer[at++] = 128 | (u & 63);
        } else {
          writer[at++] = 240 | ((u >> 18) & 7);
          writer[at++] = 128 | ((u >> 12) & 63);
          writer[at++] = 128 | ((u >> 6) & 63);
          writer[at++] = 128 | (u & 63);
        }
        src += 5;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case 'x': { /* test for hex notation */
      if (fio_c2i(src[1]) < 16 && fio_c2i(src[2]) < 16) {
        writer[at++] = (fio_c2i(src[1]) << 4) | fio_c2i(src[2]);
        src += 3;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': { /* test for octal notation */
      if (src[0] >= '0' && src[0] <= '7' && src[1] >= '0' && src[1] <= '7') {
        writer[at++] = ((src[0] - '0') << 3) | (src[1] - '0');
        src += 2;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case '"':
    case '\\':
    case '/':
    /* fall through */
    default:
    invalid_escape:
      writer[at++] = *(src++);
    }
  }
  dest->len += at;
  dest->buf[dest->len] = 0;
  FIO_ASSERT_DEBUG(at < reduced, "string unescape reduced calculation error");
  return r;
}

/* *****************************************************************************
String Base64 support
***************************************************************************** */

/** Writes data to String using Base64 encoding. */
SFUNC int fio_string_write_base64enc(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *data,
                                     size_t len,
                                     uint8_t url_encoded) {
  int r = 0;
  if (!dest || !data || !len)
    return r;
  static const char *encmap[2] = {
      /* Regular, URL encoding*/
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=",
  };

  /* the base64 encoding array */
  const char *encoding = encmap[!!url_encoded];

  /* base64 length and padding information */
  size_t groups = len / 3;
  const size_t mod = len - (groups * 3);
  const size_t target_size = (groups + (mod != 0)) * 4;

  if (fio_string___write_validate_len(dest, reallocate, target_size) !=
      target_size) {
    return (r = -1); /* no partial encoding. */
  }
  char *writer = dest->buf + dest->len;
  const unsigned char *reader = (const unsigned char *)data;
  dest->len += target_size;
  /* write encoded data */
  while (groups) {
    --groups;
    const unsigned char tmp1 = *(reader++);
    const unsigned char tmp2 = *(reader++);
    const unsigned char tmp3 = *(reader++);

    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[(((tmp1 & 3) << 4) | ((tmp2 >> 4) & 15))];
    *(writer++) = encoding[((tmp2 & 15) << 2) | ((tmp3 >> 6) & 3)];
    *(writer++) = encoding[tmp3 & 63];
  }

  /* write padding / ending */
  switch (mod) {
  case 2: {
    const unsigned char tmp1 = *(reader++);
    const unsigned char tmp2 = *(reader++);

    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[((tmp1 & 3) << 4) | ((tmp2 >> 4) & 15)];
    *(writer++) = encoding[((tmp2 & 15) << 2)];
    *(writer++) = '=';
  } break;
  case 1: {
    const unsigned char tmp1 = *(reader++);

    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[(tmp1 & 3) << 4];
    *(writer++) = '=';
    *(writer++) = '=';
  } break;
  }
  dest->buf[dest->len] = 0;
  return r;
}

/** Writes decoded base64 data to String. */
SFUNC int fio_string_write_base64dec(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *encoded_,
                                     size_t len) {
  /* Base64 decoding array. Generation script (Ruby):
s = ["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",
     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_="]
valid = []; (0..255).each {|i| valid[i] = 0 };
decoder = []; (0..127).each {|i| decoder[i] = 0 };
s.each {|d| d.bytes.each_with_index { |b, i| decoder[b] = i; valid[b] = 1 } };
p valid; p decoder; nil
  */
  const static uint8_t base64_valid[256] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  const static uint8_t base64_decodes[128] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 0,  62, 0,  63,
      52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  64, 0,  0,
      0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63,
      0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0,  0,  0,  0,  0,
  };
  int r = 0;
  if (!dest || !encoded_)
    return (r = -1);
  const uint8_t *encoded = (const uint8_t *)encoded_;
  /* skip unknown data at end */
  while (len && !base64_valid[encoded[len - 1]]) {
    len--;
  }
  if (!len)
    return (r = -1);

  /* reserve memory space */
  {
    const uint32_t required_len = (((len >> 2) * 3) + 3);
    if (fio_string___write_validate_len(dest, reallocate, required_len) !=
        required_len) {
      return (r = -1); /* no partial decoding. */
    };
  }

  /* decoded and count actual length */
  size_t pos = 0;
  uint8_t b64wrd[4];
  const uint8_t *stop = encoded + len;
  uint8_t *writer = (uint8_t *)dest->buf + dest->len;
  for (;;) {
    if (base64_valid[encoded[0]])
      b64wrd[pos++] = base64_decodes[encoded[0]];
    else if (!isspace(encoded[0]))
      break;
    ++encoded;
    if (pos == 4) {
      writer[0] = (b64wrd[0] << 2) | (b64wrd[1] >> 4);
      writer[1] = (b64wrd[1] << 4) | (b64wrd[2] >> 2);
      writer[2] = (b64wrd[2] << 6) | b64wrd[3];
      pos = 0;
      writer += 3;
    }
    if (encoded == stop)
      break;
  }
  switch (pos) {
  case 1: b64wrd[1] = 0; /* fall through */
  case 2: b64wrd[2] = 0; /* fall through */
  case 3: b64wrd[3] = 0; /* fall through */
  case 4:
    writer[0] = (b64wrd[0] << 2) | (b64wrd[1] >> 4);
    writer[1] = (b64wrd[1] << 4) | (b64wrd[2] >> 2);
    writer[2] = (b64wrd[2] << 6) | b64wrd[3];
    writer += 3;
  }
  writer -= (encoded[-1] == '=') + (encoded[-2] == '=');
  if (writer < ((uint8_t *)dest->buf + dest->len))
    writer = ((uint8_t *)dest->buf + dest->len);
  dest->len = (size_t)(writer - (uint8_t *)dest->buf);
  dest->buf[dest->len] = 0;
  return r;
}

/* *****************************************************************************
Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, string_core_helpers)(void) {
  fprintf(stderr, "* Testing Core String API.\n");
  { /* test basic fio_string_write functions. */
    fprintf(stderr, "* Testing Core String writing functions.\n");
    char mem[16];
    fio_str_info_s buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_write(&buf, NULL, "Hello World", 11);
    FIO_ASSERT(mem == buf.buf && buf.len == 11 &&
                   !memcmp(buf.buf, "Hello World", 12),
               "fio_string_write failed!");
    fio_string_write(&buf, NULL, "Hello World", 11);
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "Hello WorldHell", 16),
               "fio_string_write failed to truncate!");
    fio_string_insert(&buf, NULL, 0, 5, "Hola", 4);
    FIO_ASSERT(mem == buf.buf && buf.len == 14 &&
                   !memcmp(buf.buf, "Hola WorldHell", 15),
               "fio_string_insert at index 0 failed!");
    fio_string_insert(&buf, NULL, 5, 9, "World", 5);
    FIO_ASSERT(mem == buf.buf && buf.len == 10 &&
                   !memcmp(buf.buf, "Hola World", 11),
               "fio_string_insert end overwrite failed!");
    fio_string_insert(&buf, NULL, 5, 0, "my beautiful", 12);
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "Hola my beautif", 16),
               "fio_string_insert failed to truncate!");
    buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_printf(&buf, NULL, "I think %d is the best answer", 42);
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I think 42 is t", 16),
               "fio_string_printf failed to truncate!");

    memset(mem, 0, 16);
    buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_write2(&buf,
                      NULL,
                      FIO_STRING_WRITE_STR2((char *)"I think ", 8),
                      FIO_STRING_WRITE_NUM(42),
                      FIO_STRING_WRITE_STR1((char *)" is the best answer"));
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I think 42 is t", 16),
               "fio_string_write2 failed to truncate!");
    memset(mem, 0, 16);
    buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_write2(&buf,
                      NULL,
                      FIO_STRING_WRITE_STR2((char *)"I think ", 8),
                      FIO_STRING_WRITE_HEX(42),
                      FIO_STRING_WRITE_STR1((char *)" is the best answer"));
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I think 2A is t", 16),
               "fio_string_write2 failed to truncate (hex)!");
    memset(mem, 0, 16);
    buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_write2(&buf,
                      NULL,
                      FIO_STRING_WRITE_STR2((char *)"I Think ", 8),
                      FIO_STRING_WRITE_FLOAT(42.42),
                      FIO_STRING_WRITE_STR1((char *)" is the best answer"));
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I Think 42.42 i", 16),
               "fio_string_write2 failed to truncate (float)!");
    buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_write2(&buf,
                      NULL,
                      FIO_STRING_WRITE_STR2((char *)"I think ", 8),
                      FIO_STRING_WRITE_BIN(-1LL),
                      FIO_STRING_WRITE_STR1((char *)" is the best answer"));
    FIO_ASSERT(mem == buf.buf && buf.len == 8 &&
                   !memcmp(buf.buf, "I think ", 8),
               "fio_string_write2 failed to truncate (bin)!");
  }
  { /* Testing UTF-8 */
    fprintf(stderr, "* Testing UTF-8 support.\n");
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    fio_str_info_s utf8 = FIO_STR_INFO1((char *)utf8_sample);
    intptr_t pos = -2;
    size_t len = 2;
    FIO_ASSERT(fio_string_utf8_select(utf8, &pos, &len) == 0,
               "`fio_string_utf8_select` returned error for negative pos on "
               "UTF-8 data! (%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(pos == (intptr_t)utf8.len - 4, /* 4 byte emoji */
               "`fio_string_utf8_select` error, negative position invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)pos);
    FIO_ASSERT(len == 4, /* last utf-8 char is 4 byte long */
               "`fio_string_utf8_select` error, truncated length invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)len);
    pos = 1;
    len = 20;
    FIO_ASSERT(fio_string_utf8_select(utf8, &pos, &len) == 0,
               "`fio_string_utf8_select` returned error on UTF-8 data! "
               "(%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(pos == 4,
               "`fio_string_utf8_select` error, position invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)pos);
    FIO_ASSERT(len == 10,
               "`fio_string_utf8_select` error, length invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)len);
    pos = 1;
    len = 3;
    FIO_ASSERT(fio_string_utf8_select(utf8, &pos, &len) == 0,
               "`fio_string_utf8_select` returned error on UTF-8 data "
               "(2)! (%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(len ==
                   10, /* 3 UTF-8 chars: 4 byte + 4 byte + 2 byte codes == 10 */
               "`fio_string_utf8_select` error, length invalid on UTF-8 data! "
               "(%zd)",
               (ssize_t)len);
  }
  { /* testing C / JSON style escaping */
    fprintf(stderr, "* Testing C / JSON style character (un)escaping.\n");
    char mem[2048];
    fio_str_info_s unescaped = FIO_STR_INFO3(mem, 0, 512);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 512, 0, 512);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 1024, 0, 1024);
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    FIO_ASSERT(
        !fio_string_write(&unescaped, NULL, utf8_sample, strlen(utf8_sample)),
        "Couldn't write UTF-8 example.");
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      FIO_ASSERT(!fio_string_write(&unescaped, NULL, &c, 1),
                 "write returned an error");
    }
    FIO_ASSERT(
        !fio_string_write_escape(&encoded, NULL, unescaped.buf, unescaped.len),
        "write escape returned an error");
    FIO_ASSERT(
        !fio_string_write_unescape(&decoded, NULL, encoded.buf, encoded.len),
        "write unescape returned an error");
    FIO_ASSERT(encoded.len, "JSON encoding failed");
    FIO_ASSERT(!memcmp(encoded.buf, utf8_sample, strlen(utf8_sample)),
               "valid UTF-8 data shouldn't be escaped:\n%.*s\n%s",
               (int)encoded.len,
               encoded.buf,
               decoded.buf);
    FIO_ASSERT(
        unescaped.len == decoded.len,
        "C escaping roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
        unescaped.len,
        decoded.len,
        decoded.len,
        encoded.len,
        decoded.buf);
    FIO_ASSERT(!memcmp(unescaped.buf, decoded.buf, unescaped.len),
               "C escaping round-trip failed:\n %s",
               decoded.buf);
  }
  { /* testing Base64 Support */
    fprintf(stderr, "* Testing Base64 encoding / decoding.\n");
    char mem[2048];
    fio_str_info_s original = FIO_STR_INFO3(mem, 0, 512);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 512, 0, 512);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 1024, 0, 512);
    fio_string_write(&original,
                     NULL,
                     "Hello World, this is the voice of peace:)",
                     41);
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      FIO_ASSERT(!fio_string_write(&original, NULL, &c, 1),
                 "write returned an error");
    }
    FIO_ASSERT(!fio_string_write_base64enc(&encoded,
                                           NULL,
                                           original.buf,
                                           original.len,
                                           1),
               "base64 write escape returned an error");
    FIO_ASSERT(
        !fio_string_write_base64dec(&decoded, NULL, encoded.buf, encoded.len),
        "base64 write unescape returned an error");

    FIO_ASSERT(encoded.len, "Base64 encoding failed");
    FIO_ASSERT(decoded.len < encoded.len,
               "Base64 decoding failed:\n%s",
               encoded.buf);
    FIO_ASSERT(original.len == decoded.len,
               "Base64 roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
               original.len,
               decoded.len,
               decoded.len,
               encoded.len,
               decoded.buf);
    FIO_ASSERT(!memcmp(original.buf, decoded.buf, original.len),
               "Base64 round-trip failed:\n %s",
               decoded.buf);
  }
  { /* Comparison testing */
    FIO_ASSERT(fio_string_is_greater(FIO_STR_INFO1((char *)"A"),
                                     FIO_STR_INFO1((char *)"")),
               "fio_string_is_greater failed for A vs __");
    FIO_ASSERT(fio_string_is_greater(FIO_STR_INFO1((char *)"hello world"),
                                     FIO_STR_INFO1((char *)"hello worl")),
               "fio_string_is_greater failed for hello worl(d)");
    FIO_ASSERT(fio_string_is_greater(FIO_STR_INFO1((char *)"01234567"),
                                     FIO_STR_INFO1((char *)"012345664")),
               "fio_string_is_greater failed for 01234567");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)""),
                                      FIO_STR_INFO1((char *)"A")),
               "fio_string_is_greater failed for A inv");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)"hello worl"),
                                      FIO_STR_INFO1((char *)"hello world")),
               "fio_string_is_greater failed for hello worl(d) inv");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)"012345664"),
                                      FIO_STR_INFO1((char *)"01234567")),
               "fio_string_is_greater failed for 01234567 inv");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)"Hzzzzzzzzzz"),
                                      FIO_STR_INFO1((char *)"hello world")),
               "fio_string_is_greater failed for Hello world");
  }
#if !defined(DEBUG) || defined(NODEBUG)
  { /* speed testing comparison */
    char str_a[] = "This is not a very long string but it should be bigger "
                   "than the other "
                   "one that has one character missing at the end, okay??";
    char str_b[] = "This is not a very long string but it should be bigger "
                   "than the other "
                   "one that has one character missing at the end, okay?";
    fio_str_info_s sa = FIO_STR_INFO1(str_a);
    fio_str_info_s sb = FIO_STR_INFO1(str_b);
    clock_t start = clock();
    for (size_t i = 0; i < (1ULL << 17); ++i) {
      FIO_COMPILER_GUARD;
      int r = fio_string_is_greater(sa, sb);
      FIO_ASSERT(r > 0, "fio_string_is_greater error?!");
    }
    clock_t end = clock();
    fprintf(stderr,
            "\t* fio_string_is_greater test cycles:   %zu\n",
            (size_t)(end - start));
    start = clock();
    for (size_t i = 0; i < (1ULL << 17); ++i) {
      FIO_COMPILER_GUARD;
      int r = memcmp(str_a, str_b, sa.len > sb.len ? sb.len : sa.len);
      if (!r)
        r = sa.len > sb.len;
      FIO_ASSERT(r > 0, "memcmp error?!");
    }
    end = clock();
    fprintf(stderr,
            "\t* memcmp libc test cycles:            %zu\n",
            (size_t)(end - start));
    start = clock();
    for (size_t i = 0; i < (1ULL << 17); ++i) {
      FIO_COMPILER_GUARD;
      int r = strcmp(str_a, str_b);
      FIO_ASSERT(r > 0, "strcmp error?!");
    }
    end = clock();
    fprintf(stderr,
            "\t* strcmp libc test cycles:            %zu\n",
            (size_t)(end - start));
  }
#endif /* DEBUG */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
String Core Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_STR
#endif /* H__FIO_STR__H */
