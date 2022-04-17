/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_STR_CORE                /*Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                      Binary Safe String Core Helpers




***************************************************************************** */
#if (defined(FIO_STR_SMALL) || defined(FIO_STR_NAME) ||                        \
     defined(FIO_STR_CORE)) &&                                                 \
    !defined(H__FIO_STR_CORE__H)
#define H__FIO_STR_CORE__H
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
FIO_SFUNC int fio_string_insert(fio_str_info_s *dest,
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
 */
FIO_SFUNC int fio_string_write2(fio_str_info_s *restrict dest,
                                void (*reallocate)(fio_str_info_s *,
                                                   size_t new_capa),
                                const fio_string_write_s srcs[]);

/* Helper macro for fio_string_write2 */
#define fio_string_write2(dest, reallocate, ...)                               \
  fio_string_write2((dest),                                                    \
                    (reallocate),                                              \
                    (fio_string_write_s[]){__VA_ARGS__, {0}})

#define FIO_STRING_WRITE_STR1(str_)                                            \
  ((fio_string_write_s){.klass = 1,                                            \
                        .info.str = {.len = strlen((str_)), .buf = (str_)}})
#define FIO_STRING_WRITE_STR2(str_, len_)                                      \
  ((fio_string_write_s){.klass = 1, .info.str = {.len = (len_), .buf = (str_)}})
#define FIO_STRING_WRITE_NUM(num)                                              \
  ((fio_string_write_s){.klass = 2, .info.i = (int64_t)(num)})
#define FIO_STRING_WRITE_UNUM(num)                                             \
  ((fio_string_write_s){.klass = 3, .info.u = (uint64_t)(num)})
#define FIO_STRING_WRITE_HEX(num)                                              \
  ((fio_string_write_s){.klass = 4, .info.u = (uint64_t)(num)})
#define FIO_STRING_WRITE_BIN(num)                                              \
  ((fio_string_write_s){.klass = 5, .info.u = (uint64_t)(num)})
#define FIO_STRING_WRITE_FLOAT(num)                                            \
  ((fio_string_write_s){.klass = 6, .info.f = (double)(num)})

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
FIO_SFUNC void fio_string_default_reallocate(fio_str_info_s *dest,
                                             size_t new_capa);
/** default reallocation callback for memory that mustn't be freed. */
FIO_SFUNC void fio_string_default_copy_and_reallocate(fio_str_info_s *dest,
                                                      size_t new_capa);
/** frees memory that was allocated with the default callbacks. */
FIO_SFUNC void fio_string_default_free(void *);
/** frees memory that was allocated with the default callbacks. */
FIO_SFUNC void fio_string_default_free2(fio_str_info_s str);
/** does nothing. */
FIO_SFUNC void fio_string_default_free_noop(fio_str_info_s str);

/* *****************************************************************************


                             String Implementation

                           IMPLEMENTATION - INLINED


***************************************************************************** */

/* *****************************************************************************
String Authorship Helpers - (inlined) implementation
***************************************************************************** */
FIO_SFUNC void fio_string_default_reallocate(fio_str_info_s *dest,
                                             size_t new_capa) {
  void *tmp = FIO_MEM_REALLOC_(dest->buf, dest->capa, new_capa, dest->len);
  if (!tmp)
    return;
  dest->capa = new_capa;
  dest->buf = (char *)tmp;
}

FIO_SFUNC void fio_string_default_copy_and_reallocate(fio_str_info_s *dest,
                                                      size_t new_capa) {
  void *tmp = FIO_MEM_REALLOC_(NULL, 0, new_capa, 0);
  if (!tmp)
    return;
  dest->capa = new_capa;
  dest->buf = (char *)tmp;
  if (dest->len)
    FIO_MEMCPY(tmp, dest->buf, dest->len);
}

FIO_SFUNC void fio_string_default_free(void *ptr) { FIO_MEM_FREE_(ptr, 0); }
FIO_SFUNC void fio_string_default_free2(fio_str_info_s str) {
  FIO_MEM_FREE_(str.buf, str.capa);
}

FIO_SFUNC void fio_string_default_free_noop(fio_str_info_s str) { (void)str; }

/* fio_string_write */
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
  static const char fio___i2c_map[] = "0123456789ABCDEF";
  int r = -1;
  char buf[16];
  size_t len = 0;
  while (i) {
    buf[len++] = fio___i2c_map[(i & 15)];
    i >>= 4;
    buf[len++] = fio___i2c_map[(i & 15)];
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

/* fio_string_insert */
FIO_SFUNC int fio_string_insert(fio_str_info_s *dest,
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

/* the fio_string_write2 is a printf alternative. */
FIO_SFUNC int fio_string_write2 FIO_NOOP(fio_str_info_s *restrict dest,
                                         void (*reallocate)(fio_str_info_s *,
                                                            size_t new_capa),
                                         const fio_string_write_s srcs[]) {
  int r = 0;
  const fio_string_write_s *pos = srcs;
  size_t len = 0;

  while (pos->klass) {
    switch (pos->klass) {
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
                    FIO_STRING_WRITE_STR2("I think ", 8),
                    FIO_STRING_WRITE_NUM(42),
                    FIO_STRING_WRITE_STR1(" is the best answer"));
  FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                 !memcmp(buf.buf, "I think 42 is t", 16),
             "fio_string_write2 failed to truncate!");
  memset(mem, 0, 16);
  buf = FIO_STR_INFO3(mem, 0, 16);
  fio_string_write2(&buf,
                    NULL,
                    FIO_STRING_WRITE_STR2("I think ", 8),
                    FIO_STRING_WRITE_HEX(42),
                    FIO_STRING_WRITE_STR1(" is the best answer"));
  FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                 !memcmp(buf.buf, "I think 2A is t", 16),
             "fio_string_write2 failed to truncate (hex)!");
  memset(mem, 0, 16);
  buf = FIO_STR_INFO3(mem, 0, 16);
  fio_string_write2(&buf,
                    NULL,
                    FIO_STRING_WRITE_STR2("I Think ", 8),
                    FIO_STRING_WRITE_FLOAT(42.42),
                    FIO_STRING_WRITE_STR1(" is the best answer"));
  FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                 !memcmp(buf.buf, "I Think 42.42 i", 16),
             "fio_string_write2 failed to truncate (float)!");
  buf = FIO_STR_INFO3(mem, 0, 16);
  fio_string_write2(&buf,
                    NULL,
                    FIO_STRING_WRITE_STR2("I think ", 8),
                    FIO_STRING_WRITE_BIN(-1LL),
                    FIO_STRING_WRITE_STR1(" is the best answer"));
  FIO_ASSERT(mem == buf.buf && buf.len == 8 && !memcmp(buf.buf, "I think ", 8),
             "fio_string_write2 failed to truncate (bin)!");
}

#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
String Core Cleanup
***************************************************************************** */
#undef FIO_STR_CORE
#endif /* H__FIO_STR_CORE__H */
