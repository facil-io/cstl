/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_STR                     /*Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                      Binary Safe String Core Helpers




***************************************************************************** */
#if defined(FIO_STR) && !defined(H__FIO_STR__H)
#define H__FIO_STR__H

/* *****************************************************************************
String Authorship Helpers
***************************************************************************** */

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
FIO_IFUNC int fio_string_write(fio_str_info_s *dest,
                               void (*reallocate)(fio_str_info_s *,
                                                  size_t new_capa),
                               const void *src,
                               size_t len);
/* Writes a signed number `i` to the String */
FIO_IFUNC int fio_string_write_i(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 int64_t i);
/* Writes an unsigned number `i` to the String */
FIO_IFUNC int fio_string_write_u(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 uint64_t i);
/* Writes a hex representation of `i` to the String */
FIO_IFUNC int fio_string_write_hex(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
                                   uint64_t i);
/* Writes a binary representation of `i` to the String */
FIO_IFUNC int fio_string_write_bin(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
                                   uint64_t i);

/**
 * Similar to fio_string_write, only replacing a sub-string or inserting a
 * string in a specific location.
 */
SFUNC int fio_string_insert(fio_str_info_s *dest,
                            void (*reallocate)(fio_str_info_s *,
                                               size_t new_capa),
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
FIO_IFUNC int fio_string_printf(fio_str_info_s *dest,
                                void (*reallocate)(fio_str_info_s *,
                                                   size_t new_capa),
                                const char *format,
                                ...);

/** Similar to fio_string_write, only using vprintf semantics. */
FIO_IFUNC int fio_string_vprintf(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
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
                            void (*reallocate)(fio_str_info_s *,
                                               size_t new_capa),
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
 * Compares two strings, returning 1 if string a is bigger than string b.
 *
 * Note: returns 0 if string b is bigger than string a or if strings are equal.
 */
SFUNC int fio_string_is_bigger(fio_str_info_s a, fio_str_info_s b);

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
FIO_IFUNC int fio_string_write(fio_str_info_s *dest,
                               void (*reallocate)(fio_str_info_s *,
                                                  size_t new_capa),
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

/* fio_string_write_i */
FIO_IFUNC int fio_string_write_i(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
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
FIO_IFUNC int fio_string_write_u(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
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
FIO_IFUNC int fio_string_write_hex(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
                                   uint64_t i) {
  static const char fio___i2c16_map[] = "0123456789ABCDEF";
  int r = -1;
  char buf[16];
  size_t len = 0;
  while (i) {
    buf[len++] = fio___i2c16_map[(i & 15)];
    i >>= 4;
    buf[len++] = fio___i2c16_map[(i & 15)];
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
FIO_IFUNC int fio_string_write_bin(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
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

/* Similar to fio_string_write, only using vprintf semantics. */
FIO_IFUNC int __attribute__((format(FIO___PRINTF_STYLE, 3, 0)))
fio_string_vprintf(fio_str_info_s *dest,
                   void (*reallocate)(fio_str_info_s *, size_t new_capa),
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
FIO_IFUNC int __attribute__((format(FIO___PRINTF_STYLE, 3, 4)))
fio_string_printf(fio_str_info_s *dest,
                  void (*reallocate)(fio_str_info_s *, size_t new_capa),
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
Insert / Write2
***************************************************************************** */

/* fio_string_insert */
SFUNC int fio_string_insert(fio_str_info_s *dest,
                            void (*reallocate)(fio_str_info_s *,
                                               size_t new_capa),
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
fio_string_is_bigger
***************************************************************************** */

/**
 * Compares two strings, returning 1 if string a is bigger than string b.
 *
 * Note: returns 0 if string b is bigger than string a or if strings are equal.
 */
SFUNC int fio_string_is_bigger(fio_str_info_s a, fio_str_info_s b) {
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
Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, string_core_helpers)(void) {
  fprintf(stderr, "* Testing Core String Helpers (fio_string_write).\n");
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
  FIO_ASSERT(mem == buf.buf && buf.len == 8 && !memcmp(buf.buf, "I think ", 8),
             "fio_string_write2 failed to truncate (bin)!");

  {
    /* Testing UTF-8 */
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
  /* Comparison testing */
  FIO_ASSERT(fio_string_is_bigger(FIO_STR_INFO1((char *)"A"),
                                  FIO_STR_INFO1((char *)"")),
             "fio_string_is_bigger failed for A vs __");
  FIO_ASSERT(fio_string_is_bigger(FIO_STR_INFO1((char *)"hello world"),
                                  FIO_STR_INFO1((char *)"hello worl")),
             "fio_string_is_bigger failed for hello worl(d)");
  FIO_ASSERT(fio_string_is_bigger(FIO_STR_INFO1((char *)"01234567"),
                                  FIO_STR_INFO1((char *)"012345664")),
             "fio_string_is_bigger failed for 01234567");
  FIO_ASSERT(!fio_string_is_bigger(FIO_STR_INFO1((char *)""),
                                   FIO_STR_INFO1((char *)"A")),
             "fio_string_is_bigger failed for A inv");
  FIO_ASSERT(!fio_string_is_bigger(FIO_STR_INFO1((char *)"hello worl"),
                                   FIO_STR_INFO1((char *)"hello world")),
             "fio_string_is_bigger failed for hello worl(d) inv");
  FIO_ASSERT(!fio_string_is_bigger(FIO_STR_INFO1((char *)"012345664"),
                                   FIO_STR_INFO1((char *)"01234567")),
             "fio_string_is_bigger failed for 01234567 inv");
  FIO_ASSERT(!fio_string_is_bigger(FIO_STR_INFO1((char *)"Hzzzzzzzzzz"),
                                   FIO_STR_INFO1((char *)"hello world")),
             "fio_string_is_bigger failed for Hello world");
  /* speed testing comparison */
#if !defined(DEBUG) || defined(NODEBUG)
  {
    char str_a[] =
        "This is not a very long string but it should be bigger than the other "
        "one that has one character missing at the end, okay??";
    char str_b[] =
        "This is not a very long string but it should be bigger than the other "
        "one that has one character missing at the end, okay?";
    fio_str_info_s sa = FIO_STR_INFO1(str_a);
    fio_str_info_s sb = FIO_STR_INFO1(str_b);
    clock_t start = clock();
    for (size_t i = 0; i < (1ULL << 17); ++i) {
      FIO_COMPILER_GUARD;
      int r = fio_string_is_bigger(sa, sb);
      FIO_ASSERT(r > 0, "fio_string_is_bigger error?!");
    }
    clock_t end = clock();
    fprintf(stderr,
            "\t* fio_string_is_bigger test cycles:   %zu\n",
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
