/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_STR                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                      Binary Safe String Core Helpers



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_STR) && !defined(H___FIO_STR___H)
#define H___FIO_STR___H
/* *****************************************************************************
String Authorship Helpers (`fio_string_write` functions)
***************************************************************************** */

/**
 * A reallocation callback type for buffers in a `fio_str_info_s`.
 *
 * The callback MUST allocate at least `len + 1` bytes, setting the new capacity
 * in `dest->capa`.
 * */
typedef int (*fio_string_realloc_fn)(fio_str_info_s *dest, size_t len);
/**
 * Writes data to the end of the string in the `fio_string_s` struct,
 * returning an updated `fio_string_s` struct.
 *
 * The returned string is NUL terminated if edited.
 *
 * * `dest` an `fio_string_s` struct containing the destination string.
 *
 * * `reallocate` is a callback that attempts to reallocate more memory (i.e.,
 * using `realloc`) and returns an updated `fio_string_s` struct containing the
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
 *      int fio_string_realloc_system(fio_str_info_s *dest, size_t len_no_nul) {
 *       const size_t new_capa = fio_string_capa4len(len_pre_nul);
 *       void *tmp = realloc(dest.buf, new_capa);
 *       if (!tmp)
 *         return -1;
 *       dest.capa = new_capa;
 *       dest.buf = (char *)tmp;
 *       return 0;
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
                               const void *restrict src,
                               size_t len);

/**
 * Similar to `fio_string_write`, only replacing/inserting a sub-string in a
 * specific location.
 *
 * Negative `start_pos` values are calculated backwards, `-1` == end of String.
 *
 * When `overwrite_len` is zero, the function will insert the data at
 * `start_pos`, pushing existing data until after the inserted data.
 *
 * If `overwrite_len` is non-zero, than `overwrite_len` bytes will be
 * overwritten (or deleted).
 *
 * If `len == 0` than `src` will be ignored and the data marked for replacement
 * will be erased.
 */
SFUNC int fio_string_replace(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             intptr_t start_pos,
                             size_t overwrite_len,
                             const void *src,
                             size_t len);

/** Argument type used by fio_string_write2. */
typedef struct {
  size_t klass;
  union {
    struct {
      size_t len;
      const char *buf;
    } str;
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
  ((fio_string_write_s){                                                       \
      .klass = 1,                                                              \
      .info.str = {.len = FIO_STRLEN((str_)), .buf = (str_)}})

/** A macro to add a String with known length to `fio_string_write2`. */
#define FIO_STRING_WRITE_STR2(str_, len_)                                      \
  ((fio_string_write_s){.klass = 1, .info.str = {.len = (len_), .buf = (str_)}})

/** A macro to add a String with known length to `fio_string_write2`. */
#define FIO_STRING_WRITE_STR_INFO(str_)                                        \
  ((fio_string_write_s){.klass = 1,                                            \
                        .info.str = {.len = (str_).len, .buf = (str_).buf}})

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
String Numerals support
***************************************************************************** */

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

/* *****************************************************************************
String printf style support
***************************************************************************** */

/** Similar to fio_string_write, only using printf semantics. */
SFUNC FIO___PRINTF_STYLE(3, 0) int fio_string_printf(
    fio_str_info_s *dest,
    fio_string_realloc_fn reallocate,
    const char *format,
    ...);

/** Similar to fio_string_write, only using vprintf semantics. */
SFUNC FIO___PRINTF_STYLE(3, 0) int fio_string_vprintf(
    fio_str_info_s *dest,
    fio_string_realloc_fn reallocate,
    const char *format,
    va_list argv);

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
                                  const void *raw,
                                  size_t raw_len);

/** Writes an escaped data into the string after un-escaping the data. */
SFUNC int fio_string_write_unescape(fio_str_info_s *dest,
                                    fio_string_realloc_fn reallocate,
                                    const void *enscaped,
                                    size_t enscaped_len);

/* *****************************************************************************
String Base64 support
***************************************************************************** */

/** Writes data to String using base64 encoding. */
SFUNC int fio_string_write_base64enc(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *raw,
                                     size_t raw_len,
                                     uint8_t url_encoded);

/** Writes decoded base64 data to String. */
SFUNC int fio_string_write_base64dec(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *encoded,
                                     size_t encoded_len);

/* *****************************************************************************
String URL Encoding support
***************************************************************************** */

/** Writes data to String using URL encoding (a.k.a., percent encoding). */
SFUNC int fio_string_write_url_enc(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *raw,
                                   size_t raw_len);

/** Writes decoded URL data to String, decoding + to spaces. */
SFUNC int fio_string_write_url_dec(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *encoded,
                                   size_t encoded_len);

/** Writes decoded URL data to String, without decoding + to spaces. */
SFUNC int fio_string_write_path_dec(fio_str_info_s *dest,
                                    fio_string_realloc_fn reallocate,
                                    const void *encoded,
                                    size_t encoded_len);

/* *****************************************************************************
String HTML escaping support
***************************************************************************** */

/** Writes HTML escaped data to a String. */
SFUNC int fio_string_write_html_escape(fio_str_info_s *dest,
                                       fio_string_realloc_fn reallocate,
                                       const void *raw,
                                       size_t raw_len);

/** Writes HTML un-escaped data to a String - incomplete and minimal. */
SFUNC int fio_string_write_html_unescape(fio_str_info_s *dest,
                                         fio_string_realloc_fn reallocate,
                                         const void *enscaped,
                                         size_t enscaped_len);

/* *****************************************************************************
String File Reading support
***************************************************************************** */

/**
 * Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at`.
 *
 * If `limit` is 0 (or less than 0) data will be written until EOF.
 *
 * If `start_at` is negative, position will be calculated from the end of the
 * file where `-1 == EOF`.
 *
 * Note: this will fail unless used on actual files (not sockets, not pipes).
 * */
SFUNC int fio_string_readfd(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            int fd,
                            intptr_t start_at,
                            size_t limit);

/**
 * Opens the file `filename` and pastes it's contents (or a slice ot it) at
 * the end of the String. If `limit == 0`, than the data will be read until
 * EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned in the state's `data` field.
 */
SFUNC int fio_string_readfile(fio_str_info_s *dest,
                              fio_string_realloc_fn reallocate,
                              const char *filename,
                              intptr_t start_at,
                              size_t limit);

/**
 * Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at` and
 * ending either at the first occurrence of `delim` or at EOF.
 *
 * If `limit` is 0 (or less than 0) as much data as may be required will be
 * written.
 *
 * If `start_at` is negative, position will be calculated from the end of the
 * file where `-1 == EOF`.
 *
 * Note: this will fail unless used on actual seekable files (not sockets, not
 * pipes).
 * */
SFUNC int fio_string_getdelim_fd(fio_str_info_s *dest,
                                 fio_string_realloc_fn reallocate,
                                 int fd,
                                 intptr_t start_at,
                                 char delim,
                                 size_t limit);

/**
 * Opens the file `filename`, calls `fio_string_getdelim_fd` and closes the
 * file.
 */
SFUNC int fio_string_getdelim_file(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const char *filename,
                                   intptr_t start_at,
                                   char delim,
                                   size_t limit);

/* *****************************************************************************
Memory Helpers (for Authorship)
***************************************************************************** */

/* calculates a 16 bytes boundary aligned capacity for `new_len`. */
FIO_IFUNC size_t fio_string_capa4len(size_t new_len);

/** Default reallocation callback implementation */
#define FIO_STRING_REALLOC fio_string_default_reallocate
/** Default reallocation callback for memory that mustn't be freed. */
#define FIO_STRING_ALLOC_COPY fio_string_default_copy_and_reallocate
/** default allocator for the fio_keystr_s string data.. */
#define FIO_STRING_ALLOC_KEY fio_string_default_key_alloc
/** Frees memory that was allocated with the default callbacks. */
#define FIO_STRING_FREE fio_string_default_free
/** Frees memory that was allocated with the default callbacks. */
#define FIO_STRING_FREE2 fio_string_default_free2
/** Frees memory that was allocated for a key string. */
#define FIO_STRING_FREE_KEY fio_string_default_free_key
/** Does nothing. */
#define FIO_STRING_FREE_NOOP fio_string_default_free_noop
/** Does nothing. */
#define FIO_STRING_FREE_NOOP2 fio_string_default_free_noop2

/** default reallocation callback implementation. */
SFUNC int fio_string_default_reallocate(fio_str_info_s *dst, size_t len);
/** default reallocation callback for memory that mustn't be freed. */
SFUNC int fio_string_default_copy_and_reallocate(fio_str_info_s *dest,
                                                 size_t new_capa);
/** frees memory that was allocated with the default callbacks. */
SFUNC void fio_string_default_free(void *);
/** frees memory that was allocated with the default callbacks. */
SFUNC void fio_string_default_free2(fio_str_info_s str);
/** does nothing. */
SFUNC void fio_string_default_free_noop(void *);
/** does nothing. */
SFUNC void fio_string_default_free_noop2(fio_str_info_s str);

/** default allocator for the fio_keystr_s string data.. */
SFUNC void *fio_string_default_key_alloc(size_t len);
/** frees a fio_keystr_s memory that was allocated with the default callback. */
SFUNC void fio_string_default_free_key(void *, size_t);
/* *****************************************************************************
UTF-8 Support
***************************************************************************** */

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t fio_string_utf8_valid(fio_str_info_s str);

/** Returns the String's length in UTF-8 characters or 0 if invalid. */
SFUNC size_t fio_string_utf8_len(fio_str_info_s str);

/** Returns 0 if non-UTF-8 or returns 1-4 (UTF-8 if a valid char). */
SFUNC size_t fio_string_utf8_valid_code_point(const void *u8c, size_t buf_len);

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
Binary String Type - Embedded Strings optimized for mutability and locality
***************************************************************************** */

/* for internal use only */
typedef struct {
  uint32_t len;
  uint32_t capa;
  uint32_t ref;
} fio___bstr_meta_s;

/* for internal use only */
typedef struct {
  fio___bstr_meta_s meta;
  char *ptr;
} fio___bstr_const_s;

/** Reserves `len` for future `write` operations (used to minimize realloc). */
FIO_IFUNC char *fio_bstr_reserve(char *bstr, size_t len);

/** Copies a `fio_bstr` using "copy on write". */
FIO_IFUNC char *fio_bstr_copy(char *bstr);
/** Frees a binary string allocated by a `fio_bstr` function. Returns NULL.*/
FIO_IFUNC void fio_bstr_free(char *bstr);

/** Returns information about the fio_bstr. */
FIO_IFUNC fio_str_info_s fio_bstr_info(const char *bstr);
/** Returns information about the fio_bstr. */
FIO_IFUNC fio_buf_info_s fio_bstr_buf(const char *bstr);
/** Gets the length of the fio_bstr. `bstr` MUST NOT be NULL. */
FIO_IFUNC size_t fio_bstr_len(const char *bstr);
/** Sets the length of the fio_bstr. `bstr` MUST NOT be NULL. */
FIO_IFUNC char *fio_bstr_len_set(char *bstr, size_t len);

/** Compares to see if fio_bstr a is greater than fio_bstr b (for FIO_SORT). */
FIO_SFUNC int fio_bstr_is_greater(const char *a, const char *b);
/** Compares to see if fio_bstr a is equal to another String. */
FIO_SFUNC int fio_bstr_is_eq2info(const char *a_, fio_str_info_s b);
/** Compares to see if fio_bstr a is equal to another String. */
FIO_SFUNC int fio_bstr_is_eq2buf(const char *a_, fio_buf_info_s b);

/** Writes data to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write(char *bstr,
                               const void *restrict src,
                               size_t len);
/** Replaces data in a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_replace(char *bstr,
                                 intptr_t start_pos,
                                 size_t overwrite_len,
                                 const void *src,
                                 size_t len);
/** Writes data to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write2(char *bstr, const fio_string_write_s srcs[]);
/** Writes data to a fio_bstr, returning the address of the new fio_bstr. */
#define fio_bstr_write2(bstr, ...)                                             \
  fio_bstr_write2(bstr, (fio_string_write_s[]){__VA_ARGS__, {0}})

/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_i(char *bstr, int64_t num);
/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_u(char *bstr, uint64_t num);
/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_hex(char *bstr, uint64_t num);
/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_bin(char *bstr, uint64_t num);

/** Writes escaped data to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_escape(char *bstr, const void *src, size_t len);
/** Un-escapes and writes data to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_unescape(char *bstr,
                                        const void *src,
                                        size_t len);

/** Writes base64 encoded data to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_base64enc(char *bstr,
                                         const void *src,
                                         size_t len,
                                         uint8_t url_encoded);
/** Decodes base64 data and writes to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_base64dec(char *bstr,
                                         const void *src,
                                         size_t len);

/** Writes data to String using URL encoding (a.k.a., percent encoding). */
FIO_IFUNC char *fio_bstr_write_url_enc(char *bstr,
                                       const void *data,
                                       size_t len);
/** Writes decoded URL data to String. */
FIO_IFUNC char *fio_bstr_write_url_dec(char *bstr,
                                       const void *encoded,
                                       size_t len);

/** Writes HTML escaped data to a String. */
FIO_IFUNC char *fio_bstr_write_html_escape(char *bstr,
                                           const void *raw,
                                           size_t len);
/** Writes HTML un-escaped data to a String - incomplete and minimal. */
FIO_IFUNC char *fio_bstr_write_html_unescape(char *bstr,
                                             const void *escaped,
                                             size_t len);

/** Writes to the String from a regular file `fd`. */
FIO_IFUNC char *fio_bstr_readfd(char *bstr,
                                int fd,
                                intptr_t start_at,
                                intptr_t limit);
/** Writes to the String from a regular file named `filename`. */
FIO_IFUNC char *fio_bstr_readfile(char *bstr,
                                  const char *filename,
                                  intptr_t start_at,
                                  intptr_t limit);
/** Writes to the String from a regular file named `filename`. */
FIO_IFUNC char *fio_bstr_getdelim_file(char *bstr,
                                       const char *filename,
                                       intptr_t start_at,
                                       char delim,
                                       size_t limit);
/** Writes to the String from a regular file `fd`. */
FIO_IFUNC char *fio_bstr_getdelim_fd(char *bstr,
                                     int fd,
                                     intptr_t start_at,
                                     char delim,
                                     size_t limit);

/** Writes a `fio_bstr` in `printf` style. */
FIO_IFUNC FIO___PRINTF_STYLE(2, 0) char *fio_bstr_printf(char *bstr,
                                                         const char *format,
                                                         ...);

/** default reallocation callback implementation - mostly for internal use. */
SFUNC int fio_bstr_reallocate(fio_str_info_s *dest, size_t len);

/* *****************************************************************************
Key String Type - binary String container for Hash Maps and Arrays
***************************************************************************** */

/** a semi-opaque type used for the `fio_keystr` functions */
typedef struct fio_keystr_s fio_keystr_s;

/** returns the Key String. NOTE: Key Strings are NOT NUL TERMINATED! */
FIO_IFUNC fio_buf_info_s fio_keystr_buf(fio_keystr_s *str);
/** returns the Key String. NOTE: Key Strings are NOT NUL TERMINATED! */
FIO_IFUNC fio_str_info_s fio_keystr_info(fio_keystr_s *str);

/** Returns a TEMPORARY `fio_keystr_s` to be used as a key for a hash map. */
FIO_IFUNC fio_keystr_s fio_keystr(const char *buf, uint32_t len);
/** Returns a copy of `fio_keystr_s` - used internally by the hash map. */
FIO_SFUNC fio_keystr_s fio_keystr_copy(fio_str_info_s str,
                                       void *(*alloc_func)(size_t len));
/** Destroys a copy of `fio_keystr_s` - used internally by the hash map. */
FIO_SFUNC void fio_keystr_destroy(fio_keystr_s *key,
                                  void (*free_func)(void *, size_t));
/** Compares two Key Strings. */
FIO_IFUNC int fio_keystr_is_eq(fio_keystr_s a, fio_keystr_s b);
/** Compares a Key String to any String - used internally by the hash map. */
FIO_IFUNC int fio_keystr_is_eq2(fio_keystr_s a_, fio_str_info_s b);
/** Returns a good-enough `fio_keystr_s` risky hash. */
FIO_IFUNC uint64_t fio_keystr_hash(fio_keystr_s a);

#define FIO_KEYSTR_CONST ((size_t)-1LL)

/* *****************************************************************************


                             String Implementation

                           IMPLEMENTATION - INLINED


***************************************************************************** */

/* *****************************************************************************
String Authorship Helpers - (inlined) implementation
***************************************************************************** */

/* calculates a 16 bytes boundary aligned capacity for `new_len`. */
FIO_IFUNC size_t fio_string_capa4len(size_t new_len) {
  return sizeof(char) *
         ((new_len + 15LL + (!(new_len & 15ULL))) & (~((size_t)15ULL)));
}

/*
 * performs `reallocate` if necessary, `capa` rounded up to 16 byte units.
 * updates `len` if reallocation fails (or is unavailable).
 */
FIO_IFUNC int fio_string___write_validate_len(fio_str_info_s *restrict dest,
                                              fio_string_realloc_fn reallocate,
                                              size_t *restrict len) {
  size_t l = len[0];
  if ((dest->capa > dest->len + l))
    return 0;
  if (reallocate && l < (dest->capa >> 2) &&
      ((dest->capa >> 2) + (dest->capa) < 0x7FFFFFFFULL))
    l = (dest->capa >> 2);
  l += dest->len;
  if (l < 0x7FFFFFFFULL && reallocate && !reallocate(dest, l))
    return 0;
  if (dest->capa > dest->len + 1)
    len[0] = dest->capa - (dest->len + 1);
  else
    len[0] = 0;
  return -1;
}

/* fio_string_write */
FIO_SFUNC int fio_string_write(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               const void *restrict src,
                               size_t len) {
  int r = 0;
  if (!len)
    return r;
  r = fio_string___write_validate_len(dest, reallocate, &len);
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
Binary String Type - Embedded Strings
***************************************************************************** */
FIO___LEAK_COUNTER_DEF(fio_bstr_s)

#ifndef FIO___BSTR_META
#define FIO___BSTR_META(bstr)                                                  \
  FIO_PTR_MATH_SUB(fio___bstr_meta_s, bstr, sizeof(fio___bstr_meta_s))
#endif

/** Duplicates a `fio_bstr` using copy on write. */
FIO_IFUNC char *fio_bstr_copy(char *bstr) {
  if (!bstr)
    return bstr;
  fio___bstr_meta_s *meta = FIO___BSTR_META(bstr);
  if (fio_atomic_add(&meta->ref, 1) > ((uint32_t)1UL << 31))
    goto copy_anyway;
  return bstr;
copy_anyway:
  bstr = fio_bstr_write(NULL, bstr, meta->len);
  fio_bstr_free((char *)(meta + 1));
  return bstr;
}

/** Frees a binary string allocated by a `fio_bstr` function. */
FIO_IFUNC void fio_bstr_free(char *bstr) {
  if (!bstr)
    return;
  fio___bstr_meta_s *meta = FIO___BSTR_META(bstr);
  if (fio_atomic_sub(&meta->ref, 1))
    return;
  FIO_MEM_FREE_(meta, (meta->capa + sizeof(*meta)));
  FIO___LEAK_COUNTER_ON_FREE(fio_bstr_s);
}

/** internal helper - sets the length of the fio_bstr. */
FIO_IFUNC char *fio_bstr___len_set(char *bstr, size_t len) {
  if (!bstr)
    return bstr;
  if (len >= 0xFFFFFFFFULL)
    return bstr;
  bstr[(FIO___BSTR_META(bstr)->len = (uint32_t)len)] = 0;
  return bstr;
}

/** internal helper to make unique before mutation */
FIO_SFUNC char *fio_bstr___make_unique(char *bstr) {
  if (!bstr)
    return bstr;
  fio___bstr_meta_s *meta = FIO___BSTR_META(bstr);
  if (!meta->ref)
    return bstr;
  fio_str_info_s i = fio_bstr_info(bstr);
  i.capa = 0;
  if (i.len)
    fio_bstr_reallocate(&i, i.len);
  else
    i.buf = NULL;
  fio_bstr_free(bstr);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Reserves `len` for future `write` operations (used to minimize realloc). */
FIO_IFUNC char *fio_bstr_reserve(char *bstr, size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  if (i.len + len < i.capa)
    return bstr;
  fio_bstr_reallocate(&i, (i.len + len));
  return fio_bstr___len_set(i.buf, i.len);
}

/** Returns information about the fio_bstr. */
FIO_IFUNC fio_str_info_s fio_bstr_info(const char *bstr) {
  fio___bstr_meta_s mem[1] = {{0}};
  fio___bstr_meta_s *meta_map[2] = {FIO___BSTR_META(bstr), mem};
  fio___bstr_meta_s *meta = meta_map[!bstr];
  return FIO_STR_INFO3((char *)bstr, meta->len, meta->capa);
}

/** Returns information about the fio_bstr. */
FIO_IFUNC fio_buf_info_s fio_bstr_buf(const char *bstr) {
  fio___bstr_meta_s mem[1] = {{0}};
  fio___bstr_meta_s *meta_map[2] = {FIO___BSTR_META(bstr), mem};
  fio___bstr_meta_s *meta = meta_map[!bstr];
  return FIO_BUF_INFO2((char *)bstr, meta->len);
}

/** Gets the length of the fio_bstr. `bstr` MUST NOT be NULL. */
FIO_IFUNC size_t fio_bstr_len(const char *bstr) {
  if (!bstr)
    return 0;
  fio___bstr_meta_s *meta = FIO___BSTR_META(bstr);
  return meta->len;
}

/** Sets the length of the fio_bstr. `bstr` MUST NOT be NULL. */
FIO_IFUNC char *fio_bstr_len_set(char *bstr, size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  if (!bstr)
    return bstr;
  fio___bstr_meta_s *meta = FIO___BSTR_META(bstr);
  meta->len = len < ((size_t)(meta->capa)) ? (uint32_t)len : meta->len;
  return fio_bstr___len_set(bstr, len);
}

/** Writes data to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write(char *bstr,
                               const void *restrict src,
                               size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Replaces data in a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_replace(char *bstr,
                                 intptr_t start_pos,
                                 size_t overwrite_len,
                                 const void *src,
                                 size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_replace(&i,
                     fio_bstr_reallocate,
                     start_pos,
                     overwrite_len,
                     src,
                     len);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes data to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write2 FIO_NOOP(char *bstr,
                                         const fio_string_write_s srcs[]) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write2 FIO_NOOP(&i, fio_bstr_reallocate, srcs);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_i(char *bstr, int64_t num) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_i(&i, fio_bstr_reallocate, num);
  return fio_bstr___len_set(i.buf, i.len);
}
/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_u(char *bstr, uint64_t num) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_u(&i, fio_bstr_reallocate, num);
  return fio_bstr___len_set(i.buf, i.len);
}
/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_hex(char *bstr, uint64_t num) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_hex(&i, fio_bstr_reallocate, num);
  return fio_bstr___len_set(i.buf, i.len);
}
/** Writes number to a fio_bstr, returning the address of the new fio_bstr. */
FIO_IFUNC char *fio_bstr_write_bin(char *bstr, uint64_t num) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_bin(&i, fio_bstr_reallocate, num);
  return fio_bstr___len_set(i.buf, i.len);
}
/** Writes escaped data to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_escape(char *bstr, const void *src, size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_escape(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Un-escapes and writes data to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_unescape(char *bstr,
                                        const void *src,
                                        size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_unescape(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes base64 encoded data to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_base64enc(char *bstr,
                                         const void *src,
                                         size_t len,
                                         uint8_t url_encoded) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_base64enc(&i, fio_bstr_reallocate, src, len, url_encoded);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Decodes base64 data and writes to a fio_bstr, returning its new address. */
FIO_IFUNC char *fio_bstr_write_base64dec(char *bstr,
                                         const void *src,
                                         size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_base64dec(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes data to String using URL encoding (a.k.a., percent encoding). */
FIO_IFUNC char *fio_bstr_write_url_enc(char *bstr,
                                       const void *src,
                                       size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_url_enc(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes decoded URL data to String. */
FIO_IFUNC char *fio_bstr_write_url_dec(char *bstr,
                                       const void *src,
                                       size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_url_dec(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes HTML escaped data to a String. */
FIO_IFUNC char *fio_bstr_write_html_escape(char *bstr,
                                           const void *src,
                                           size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_html_escape(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}
/** Writes HTML un-escaped data to a String - incomplete and minimal. */
FIO_IFUNC char *fio_bstr_write_html_unescape(char *bstr,
                                             const void *src,
                                             size_t len) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_write_html_unescape(&i, fio_bstr_reallocate, src, len);
  return fio_bstr___len_set(i.buf, i.len);
}

FIO_IFUNC FIO___PRINTF_STYLE(2, 0) char *fio_bstr_printf(char *bstr,
                                                         const char *format,
                                                         ...) {
  bstr = fio_bstr___make_unique(bstr);
  va_list argv;
  va_start(argv, format);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_vprintf(&i, fio_bstr_reallocate, format, argv);
  va_end(argv);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes to the String from a regular file `fd`. */
FIO_IFUNC char *fio_bstr_readfd(char *bstr,
                                int fd,
                                intptr_t start_at,
                                intptr_t limit) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_readfd(&i, fio_bstr_reallocate, fd, start_at, limit);
  return fio_bstr___len_set(i.buf, i.len);
}
/** Writes to the String from a regular file named `filename`. */
FIO_IFUNC char *fio_bstr_readfile(char *bstr,
                                  const char *filename,
                                  intptr_t start_at,
                                  intptr_t limit) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_readfile(&i, fio_bstr_reallocate, filename, start_at, limit);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes to the String from a regular file named `filename`. */
FIO_IFUNC char *fio_bstr_getdelim_file(char *bstr,
                                       const char *filename,
                                       intptr_t start_at,
                                       char delim,
                                       size_t limit) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_getdelim_file(&i,
                           fio_bstr_reallocate,
                           filename,
                           start_at,
                           delim,
                           limit);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Writes to the String from a regular file `fd`. */
FIO_IFUNC char *fio_bstr_getdelim_fd(char *bstr,
                                     int fd,
                                     intptr_t start_at,
                                     char delim,
                                     size_t limit) {
  bstr = fio_bstr___make_unique(bstr);
  fio_str_info_s i = fio_bstr_info(bstr);
  fio_string_getdelim_fd(&i, fio_bstr_reallocate, fd, start_at, delim, limit);
  return fio_bstr___len_set(i.buf, i.len);
}

/** Compares to see if fio_bstr a is greater than fio_bstr b (for FIO_SORT). */
FIO_SFUNC int fio_bstr_is_greater(const char *a, const char *b) {
  return fio_string_is_greater_buf(fio_bstr_buf(a), fio_bstr_buf(b));
}

/** Compares to see if fio_bstr a is equal to another String. */
FIO_SFUNC int fio_bstr_is_eq2info(const char *a_, fio_str_info_s b) {
  fio_str_info_s a = fio_bstr_info(a_);
  return FIO_STR_INFO_IS_EQ(a, b);
}
/** Compares to see if fio_bstr a is equal to another String. */
FIO_SFUNC int fio_bstr_is_eq2buf(const char *a_, fio_buf_info_s b) {
  fio_buf_info_s a = fio_bstr_buf(a_);
  return FIO_BUF_INFO_IS_EQ(a, b);
}

/* *****************************************************************************
Key String Type - binary String container for Hash Maps and Arrays
***************************************************************************** */

FIO___LEAK_COUNTER_DEF(fio_keystr_s)

/* key string type implementation */
struct fio_keystr_s {
  uint8_t info;
  uint8_t embd[3];
  uint32_t len;
  const char *buf;
};

/** returns the Key String. NOTE: Key Strings are NOT NUL TERMINATED! */
FIO_IFUNC fio_buf_info_s fio_keystr_buf(fio_keystr_s *str) {
  fio_buf_info_s r;
  if ((str->info + 1) > 1) {
    r = (fio_buf_info_s){.len = str->info, .buf = (char *)str->embd};
    return r;
  }
  r = (fio_buf_info_s){.len = str->len, .buf = (char *)str->buf};
  return r;
}
/** returns the Key String. NOTE: Key Strings are NOT NUL TERMINATED! */
FIO_IFUNC fio_str_info_s fio_keystr_info(fio_keystr_s *str) {
  fio_str_info_s r;
  if ((str->info + 1) > 1) {
    r = (fio_str_info_s){.len = str->info, .buf = (char *)str->embd};
    return r;
  }
  r = (fio_str_info_s){.len = str->len, .buf = (char *)str->buf};
  return r;
}

/** Returns a TEMPORARY `fio_keystr_s` to be used as a key for a hash map. */
FIO_IFUNC fio_keystr_s fio_keystr(const char *buf, uint32_t len) {
  fio_keystr_s r = {0};
  if (len + 1 < sizeof(r)) { /* always embed small strings in container! */
    r.info = (uint8_t)len;
    FIO_MEMCPY(r.embd, buf, len);
    return r;
  }
  r.info = 0xFF;
  r.len = len;
  r.buf = buf;
  return r;
}

/** Returns a copy of `fio_keystr_s` - used internally by the hash map. */
FIO_SFUNC fio_keystr_s fio_keystr_copy(fio_str_info_s str,
                                       void *(*alloc_func)(size_t len)) {
  fio_keystr_s r = {0};
  if (!str.buf || !str.len)
    return r;
  if (str.len + 2 < sizeof(r)) {
    r.info = (uint8_t)str.len;
    FIO_MEMCPY(r.embd, str.buf, str.len);
    return r;
  }
  if (str.capa == FIO_KEYSTR_CONST) {
  no_mem2:
    r.info = 0xFF;
    r.len = str.len;
    r.buf = str.buf;
    return r;
  }
  char *buf;
  r.len = str.len;
  r.buf = buf = (char *)alloc_func(str.len + 1);
  if (!buf)
    goto no_mem;
  FIO___LEAK_COUNTER_ON_ALLOC(fio_keystr_s);
  FIO_MEMCPY(buf, str.buf, str.len);
  buf[str.len] = 0;
  return r;
no_mem:
  FIO_LOG_FATAL("fio_keystr_copy allocation failed - results undefined!!!");
  goto no_mem2;
}
/** Destroys a copy of `fio_keystr_s` - used internally by the hash map. */
FIO_SFUNC void fio_keystr_destroy(fio_keystr_s *key,
                                  void (*free_func)(void *, size_t)) {
  if (key->info || !key->buf)
    return;
  free_func((void *)key->buf, key->len);
  FIO___LEAK_COUNTER_ON_FREE(fio_keystr_s);
}

/** Compares two Key Strings. */
FIO_IFUNC int fio_keystr_is_eq(fio_keystr_s a_, fio_keystr_s b_) {
  fio_buf_info_s a = fio_keystr_buf(&a_);
  fio_buf_info_s b = fio_keystr_buf(&b_);
  return FIO_BUF_INFO_IS_EQ(a, b);
}

/** Compares a Key String to any String - used internally by the hash map. */
FIO_IFUNC int fio_keystr_is_eq2(fio_keystr_s a_, fio_str_info_s b) {
  fio_str_info_s a = fio_keystr_info(&a_);
  return FIO_STR_INFO_IS_EQ(a, b);
}

/** Returns a good-enough `fio_keystr_s` risky hash. */
FIO_IFUNC uint64_t fio_keystr_hash(fio_keystr_s a_) {
  fio_buf_info_s a = fio_keystr_buf(&a_);
  return fio_risky_hash(a.buf, a.len, (uint64_t)(uintptr_t)fio_string_write2);
}

/* *****************************************************************************
Extern-ed functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO___LEAK_COUNTER_DEF(fio_string_default_allocations)
FIO___LEAK_COUNTER_DEF(fio_string_default_key_allocations)
/* *****************************************************************************
Allocation Helpers
***************************************************************************** */
SFUNC int fio_string_default_reallocate(fio_str_info_s *dest, size_t len) {
  len = fio_string_capa4len(len);
  void *tmp = FIO_MEM_REALLOC_(dest->buf, dest->capa, len, dest->len);
  if (!tmp)
    return -1;
  if (!dest->buf)
    FIO___LEAK_COUNTER_ON_ALLOC(fio_string_default_allocations);
  dest->capa = len;
  dest->buf = (char *)tmp;
  return 0;
}

SFUNC int fio_string_default_copy_and_reallocate(fio_str_info_s *dest,
                                                 size_t len) {
  len = fio_string_capa4len(len);
  void *tmp = FIO_MEM_REALLOC_(NULL, 0, len, 0);
  if (!tmp)
    return -1;
  FIO___LEAK_COUNTER_ON_ALLOC(fio_string_default_allocations);
  dest->capa = len;
  dest->buf = (char *)tmp;
  if (dest->len)
    FIO_MEMCPY(tmp, dest->buf, dest->len);
  return 0;
}

SFUNC void *fio_string_default_key_alloc(size_t len) {
  return FIO_MEM_REALLOC_(NULL, 0, len, 0);
}

SFUNC void fio_string_default_free(void *ptr) {
  if (ptr) {
    FIO___LEAK_COUNTER_ON_FREE(fio_string_default_allocations);
    FIO_MEM_FREE_(ptr, 0);
  }
}
SFUNC void fio_string_default_free2(fio_str_info_s str) {
  if (str.buf) {
    FIO___LEAK_COUNTER_ON_FREE(fio_string_default_allocations);
    FIO_MEM_FREE_(str.buf, str.capa);
  }
}

/** frees a fio_keystr_s memory that was allocated with the default callback. */
SFUNC void fio_string_default_free_key(void *buf, size_t capa) {
  FIO_MEM_FREE_(buf, capa);
  (void)capa; /* if unused */
}

SFUNC void fio_string_default_free_noop(void *str) { (void)str; }
SFUNC void fio_string_default_free_noop2(fio_str_info_s str) { (void)str; }

/* *****************************************************************************
Numeral Support
***************************************************************************** */

/* fio_string_write_i */
SFUNC int fio_string_write_i(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             int64_t i) {
  int r = -1;
  size_t len = 0;
  len = fio_digits10(i);
  if (fio_string___write_validate_len(dest, reallocate, &len))
    return r; /* no writing of partial numbers. */
  r = 0;
  fio_ltoa10(dest->buf + dest->len, i, len);
  dest->len += len;
  return r;
}

/* fio_string_write_u */
SFUNC int fio_string_write_u(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             uint64_t i) {
  int r = -1;
  size_t len = fio_digits10u(i);
  if (fio_string___write_validate_len(dest, reallocate, &len))
    return r; /* no writing of partial numbers. */
  r = 0;
  fio_ltoa10u(dest->buf + dest->len, i, len);
  dest->len += len;
  return r;
}

/* fio_string_write_hex */
SFUNC int fio_string_write_hex(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i) {
  int r = 0;
  size_t len = fio_digits16u(i);
  if (fio_string___write_validate_len(dest, reallocate, &len))
    return (r = -1); /* no writing of partial numbers. */
  fio_ltoa16u(dest->buf + dest->len, i, len);
  dest->len += len;
  return r;
}

/* fio_string_write_bin */
SFUNC int fio_string_write_bin(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i) {
  int r = 0;
  size_t len = fio_digits_bin(i);
  if (fio_string___write_validate_len(dest, reallocate, &len))
    return (r = -1); /* no writing of partial numbers. */
  fio_ltoa_bin(dest->buf + dest->len, i, len);
  dest->len += len;
  return r;
}

/* *****************************************************************************
`printf` Style Support
***************************************************************************** */

/* Similar to fio_string_write, only using vprintf semantics. */
SFUNC int FIO___PRINTF_STYLE(3, 0)
    fio_string_vprintf(fio_str_info_s *dest,
                       fio_string_realloc_fn reallocate,
                       const char *format,
                       va_list argv) {
  int r = 0;
  va_list argv_cpy;
  va_copy(argv_cpy, argv);
  int len_i = vsnprintf(NULL, 0, format, argv_cpy);
  va_end(argv_cpy);
  if (len_i <= 0)
    return -1;
  size_t len = (size_t)len_i;
  r = fio_string___write_validate_len(dest, reallocate, &len);
  if (FIO_UNLIKELY(dest->capa < dest->len + 2))
    return -1;
  if (len)
    vsnprintf(dest->buf + dest->len, len + 1, format, argv);
  dest->len += len;
  dest->buf[dest->len] = 0;
  return r;
}

/** Similar to fio_string_write, only using printf semantics. */
SFUNC int FIO___PRINTF_STYLE(3, 4)
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

/** Returns 0 if non-UTF-8 or returns 1-4 (UTF-8 if a valid char). */
SFUNC size_t fio_string_utf8_valid_code_point(const void *c, size_t buf_len) {
  uint8_t l = fio__string_utf8_map[(*(uint8_t *)c) >> 3];
  if (buf_len < l)
    return 0;
  switch (l) {
  case 4:
    if (fio__string_utf8_map[(*((uint8_t *)c + 3)) >> 3] != 5)
      return 0; /* fall through */
  case 3:
    if (fio__string_utf8_map[(*((uint8_t *)c + 2)) >> 3] != 5)
      return 0; /* fall through */
  case 2:
    if (fio__string_utf8_map[(*((uint8_t *)c + 1)) >> 3] != 5)
      return 0; /* fall through */
  case 1: return l;
  }
  return 0;
}

/** Encodes `u` in UTF-8 format, writing it to `dest`. */
FIO_IFUNC size_t fio___string_utf8_code_point_len(size_t u) {
  if (u < 128) {
    return 1;
  } else if (u < 2048) {
    return 2;
  } else if (u < 65536) {
    return 3;
  } else {
    return 4;
  }
}

/** Encodes `u` in UTF-8 format, writing it to `dest`. */
FIO_IFUNC size_t fio___string_utf8_write(uint8_t *dest, size_t u) {
  size_t at = 0;
  if (u < 128) {
    dest[at++] = u;
  } else if (u < 2048) {
    dest[at++] = 192 | (u >> 6);
    dest[at++] = 128 | (u & 63);
  } else if (u < 65536) {
    dest[at++] = 224 | (u >> 12);
    dest[at++] = 128 | ((u >> 6) & 63);
    dest[at++] = 128 | (u & 63);
  } else {
    dest[at++] = 240 | ((u >> 18) & 7);
    dest[at++] = 128 | ((u >> 12) & 63);
    dest[at++] = 128 | ((u >> 6) & 63);
    dest[at++] = 128 | (u & 63);
  }
  return at;
}

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
  size_t len = a_len_is_bigger ? b.len : a.len; /* shared length */
  if (a.buf == b.buf)
    return a_len_is_bigger;
  uint64_t ua[4] FIO_ALIGN(16) = {0};
  uint64_t ub[4] FIO_ALIGN(16) = {0};
  uint64_t flag = 0;
  if (len < 32)
    goto mini_cmp;

  len -= 32;
  for (;;) {
    for (size_t i = 0; i < 4; ++i) {
      fio_memcpy8(ua + i, a.buf);
      fio_memcpy8(ub + i, b.buf);
      flag |= (ua[i] ^ ub[i]);
      a.buf += 8;
      b.buf += 8;
    }
    if (flag)
      goto review_diff;
    if (len > 31) {
      len -= 32;
      continue;
    }
    if (!len)
      return a_len_is_bigger;
    a.buf -= 32;
    b.buf -= 32;
    a.buf += len & 31;
    b.buf += len & 31;
    len = 0;
  }

review_diff:
  if (ua[2] != ub[2]) {
    ua[3] = ua[2];
    ub[3] = ub[2];
  }
  if (ua[1] != ub[1]) {
    ua[3] = ua[1];
    ub[3] = ub[1];
  }
  if (ua[0] != ub[0]) {
    ua[3] = ua[0];
    ub[3] = ub[0];
  }
review_diff8:
  ua[3] = fio_lton64(ua[3]); /* comparison requires network byte order */
  ub[3] = fio_lton64(ub[3]);
  return ua[3] > ub[3];

mini_cmp:
  if (len > 7) {
    len -= 8;
    for (;;) {
      fio_memcpy8(ua + 3, a.buf);
      fio_memcpy8(ub + 3, b.buf);
      if (ua[3] != ub[3])
        goto review_diff8;
      if (len > 7) {
        a.buf += 8;
        b.buf += 8;
        len -= 8;
        continue;
      }
      if (!len)
        return a_len_is_bigger;
      a.buf += len & 7;
      b.buf += len & 7;
      len = 0;
    }
  }
  while (len--) {
    if (a.buf[0] != b.buf[0])
      return a.buf[0] > b.buf[0];
    ++a.buf;
    ++b.buf;
  }
  return a_len_is_bigger;
}

/* *****************************************************************************
Insert / Write2
***************************************************************************** */

/* fio_string_replace */
SFUNC int fio_string_replace(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             intptr_t start_pos,
                             size_t overwrite_len,
                             const void *src,
                             size_t len) {
  int r = 0;
  if (start_pos < 0) {
    start_pos = dest->len + start_pos + 1;
    if (start_pos < 0)
      start_pos = 0;
  }
  if (dest->len < (size_t)start_pos + overwrite_len + 1) {
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
      r = -1; /* in case reallocate is NULL */
      if (!reallocate ||
          FIO_UNLIKELY(
              (r = reallocate(dest, fio_string_capa4len(dest->len + extra))))) {
        move_len -= (dest->len + extra + 1) - dest->capa;
        if (dest->capa < start_pos + len + 1) {
          move_len = 0;
          len = dest->capa - start_pos - 1;
        }
      }
    }
  }
  if (move_len)
    FIO_MEMMOVE(dest->buf + start_pos + len, dest->buf + move_start, move_len);
  if (len)
    FIO_MEMCPY(dest->buf + start_pos, src, len);
  dest->len = start_pos + len + move_len;
  dest->buf[dest->len] = 0;
  return r;
}

/* IDE marker */
void fio_string_write2____(void);
/* the fio_string_write2 is a printf alternative. */
SFUNC int fio_string_write2 FIO_NOOP(fio_str_info_s *restrict dest,
                                     fio_string_realloc_fn reallocate,
                                     const fio_string_write_s srcs[]) {
  int r = 0;
  const fio_string_write_s *pos = srcs;
  size_t len = 0;

  while (pos->klass) {
    switch (pos->klass) { /* use more memory rather then calculate twice. */
    case 2: /* number */ len += fio_digits10(pos->info.i); break;
    case 3: /* unsigned */ len += fio_digits10u(pos->info.u); break;
    case 4: /* hex */ len += fio_digits16u(pos->info.u); break;
    case 5: /* binary */ len += fio_digits_bin(pos->info.u); break;
    case 6: /* float */ len += 18; break;
    default: len += pos->info.str.len;
    }
    ++pos;
  }
  if (!len)
    return r;
  pos = srcs;
  if (fio_string___write_validate_len(dest, reallocate, &len))
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
  if (dest->buf + dest->len == (char *)src_)
    return (r = -1);
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
    /* skip if valid UTF-8 */
    {
      size_t utf_8_valid_len =
          fio_string_utf8_valid_code_point((void *)(src + i), len - i);
      if (utf_8_valid_len > 1) {
        i += utf_8_valid_len - 1;
        continue;
      }
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
  if ((dest->capa < dest->len + escaped_len + 1) &&
      (!reallocate ||
       reallocate(dest, fio_string_capa4len(dest->len + escaped_len + 4)))) {
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
    switch (fio_string_utf8_valid_code_point((void *)(src + i), len - i)) {
    case 4: writer[at++] = src[i++]; /* fall through */
    case 3: writer[at++] = src[i++]; /* fall through */
    case 2:
      writer[at++] = src[i++];
      writer[at++] = src[i];
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
  size_t reduced = len;
  if ((!len | !src_ | !dest))
    return r;
  if (dest->len + len >= dest->capa) { /* reserve only what we need */
    reduced = 0;
    const char *tmp = (const char *)src_;
    const char *stop = tmp + len - 1; /* avoid overflow for tmp[1] */
    for (;;) {
      tmp = (const char *)FIO_MEMCHR(tmp, '\\', (size_t)(stop - tmp));
      if (!tmp)
        break;
      size_t step = 1;
      step += ((tmp[1] == 'x') << 1); /* step == 3 */
      step += (tmp[1] == 'u');        /* UTF-8 output <= 3 */
      reduced += step;
      tmp += step + 1;
      if (tmp + 1 > stop)
        break;
    }
    FIO_ASSERT_DEBUG(reduced < len, "string unescape reduced too long");
    reduced = len - reduced;
    if (fio_string___write_validate_len(dest, reallocate, &reduced)) {
      r = -1;
      len = dest->capa - (dest->len + 1);
    }
  }
  const uint8_t *src = (const uint8_t *)src_;
  const uint8_t *end = src + len;
  uint8_t *writer = (uint8_t *)dest->buf + dest->len;
  while (src < end) {
    if (*src != '\\') {
      const uint8_t *escape_pos =
          (const uint8_t *)FIO_MEMCHR(src, '\\', end - src);
      if (!escape_pos)
        escape_pos = end;
      const size_t valid_len = escape_pos - src;
      if (writer + at != src && valid_len)
        FIO_MEMMOVE(writer + at, src, valid_len);
      at += valid_len;
      src = escape_pos;
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
          /* surrogate-pair (high/low code points) */
          u = (u & 0x03FF) << 10;
          u |= (((((fio_c2i(src[7]) << 4) | fio_c2i(src[8])) << 8) |
                 ((fio_c2i(src[9]) << 4) | fio_c2i(src[10]))) &
                0x03FF);
          u += 0x10000;
          src += 6;
        }
        at += fio___string_utf8_write(writer + at, u);
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
  FIO_ASSERT_DEBUG(r || (at < reduced + 1),
                   "string unescape reduced calculation error");
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
  size_t target_size = (groups + (mod != 0)) * 4;

  if (fio_string___write_validate_len(dest, reallocate, &target_size)) {
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
  static const uint8_t base64_valid[256] = {
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
  static const uint8_t base64_decodes[128] = {
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
  if (!dest || !encoded_ || !len)
    return r;
  const uint8_t *encoded = (const uint8_t *)encoded_;
  /* skip unknown data at end */
  while (len && !base64_valid[encoded[len - 1]]) {
    len--;
  }
  if (!len)
    return (r = -1);

  /* reserve memory space */
  {
    size_t required_len = (((len >> 2) * 3) + 3);
    if (fio_string___write_validate_len(dest, reallocate, &required_len)) {
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
String URL Encoding support
***************************************************************************** */

/** Writes data to String using URL encoding (a.k.a., percent encoding). */
SFUNC int fio_string_write_url_enc(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *data,
                                   size_t data_len) {
  static const uint8_t url_enc_map[256] = {
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0,
      2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
  int r = 0;
  /* reserve memory space */
  {
    size_t required_len = data_len;
    for (size_t i = 0; i < data_len; ++i) {
      required_len += url_enc_map[((uint8_t *)data)[i]];
    }
    if (fio_string___write_validate_len(dest, reallocate, &required_len)) {
      return (r = -1); /* no partial encoding. */
    };
  }
  for (size_t i = 0; i < data_len; ++i) {
    if (!url_enc_map[((uint8_t *)data)[i]]) {
      dest->buf[dest->len++] = ((uint8_t *)data)[i];
      continue;
    }
    dest->buf[dest->len++] = '%';
    dest->buf[dest->len++] = fio_i2c(((uint8_t *)data)[i] >> 4);
    dest->buf[dest->len++] = fio_i2c(((uint8_t *)data)[i] & 15);
  }
  dest->buf[dest->len] = 0;
  return r;
}

/** Writes decoded URL data to String. */
FIO_IFUNC int fio_string_write_url_dec_internal(
    fio_str_info_s *dest,
    fio_string_realloc_fn reallocate,
    const void *encoded,
    size_t encoded_len,
    _Bool plus_is_included) {
  int r = 0;
  if (!dest || !encoded || !encoded_len)
    return r;
  uint8_t *pr = (uint8_t *)encoded;
  uint8_t *last = pr;
  uint8_t *end = pr + encoded_len;
  if (dest->len + encoded_len >= dest->capa) { /* reserve only what we need */
    size_t act_len = 0;
    while (end > pr && (pr = (uint8_t *)FIO_MEMCHR(pr, '%', end - pr))) {
      act_len += pr - last;
      last = pr + 1;
      if (end - last > 1 && fio_c2i(last[0]) < 16 && fio_c2i(last[1]) < 16)
        last += 2;
      else if (end - last > 4 && (last[0] | 32) == 'u' &&
               fio_c2i(last[1]) < 16 && fio_c2i(last[2]) < 16 &&
               fio_c2i(last[3]) < 16 && fio_c2i(last[4]) < 16) {
        last += 5;
        act_len += 3; /* uXXXX length maxes out at 4 ... I think */
      }
      pr = last;
    }
    act_len += end - last;
    if (fio_string___write_validate_len(dest, reallocate, &act_len)) {
      return (r = -1); /* no partial decoding. */
    };
  }
  /* copy and un-encode data */
  pr = (uint8_t *)encoded;
  last = pr;
  end = pr + encoded_len;
  while (end > pr && (pr = (uint8_t *)FIO_MEMCHR(pr, '%', end - pr))) {
    const size_t slice_len = pr - last;
    if (slice_len) {
      FIO_MEMCPY(dest->buf + dest->len, last, slice_len);
      /* test for '+' in the slice that has no % characters */
      if (plus_is_included) {
        uint8_t *start_plus = (uint8_t *)dest->buf + dest->len;
        uint8_t *end_plus = start_plus + slice_len;
        while (
            start_plus && start_plus < end_plus &&
            (start_plus =
                 (uint8_t *)FIO_MEMCHR(start_plus, '+', end_plus - start_plus)))
          *(start_plus++) = ' ';
      }
    }
    dest->len += slice_len;
    last = pr + 1;
    if (end - last > 1 && fio_c2i(last[0]) < 16 && fio_c2i(last[1]) < 16) {
      dest->buf[dest->len++] = (fio_c2i(last[0]) << 4) | fio_c2i(last[1]);
      last += 2;
    } else if (end - last > 4 && (last[0] | 32) == 'u' &&
               fio_c2i(last[1]) < 16 && fio_c2i(last[2]) < 16 &&
               fio_c2i(last[3]) < 16 && fio_c2i(last[4]) < 16) {
      uint32_t u = (((fio_c2i(last[1]) << 4) | fio_c2i(last[2])) << 8) |
                   ((fio_c2i(last[3]) << 4) | fio_c2i(last[4]));
      if (end - last > 9 &&
          ((fio_c2i(last[1]) << 4) | fio_c2i(last[2])) == 0xD8U &&
          last[5] == '%' && last[6] == 'u' && fio_c2i(last[7]) < 16 &&
          fio_c2i(last[8]) < 16 && fio_c2i(last[9]) < 16 &&
          fio_c2i(last[10]) < 16) {
        /* surrogate-pair (high/low code points) */
        u = (u & 0x03FF) << 10;
        u |= (((((fio_c2i(last[7]) << 4) | fio_c2i(last[8])) << 8) |
               ((fio_c2i(last[9]) << 4) | fio_c2i(last[10]))) &
              0x03FF);
        u += 0x10000;
        last += 6;
      }
      dest->len += fio___string_utf8_write((uint8_t *)dest->buf + dest->len, u);
      last += 5;
    } else {
      dest->buf[dest->len++] = '%';
    }
    pr = last;
  }
  if (end > last) {
    const size_t slice_len = end - last;
    FIO_MEMCPY(dest->buf + dest->len, last, slice_len);
    /* test for '+' in the slice that has no % characters */
    if (plus_is_included) {
      uint8_t *start_plus = (uint8_t *)dest->buf + dest->len;
      uint8_t *end_plus = start_plus + slice_len;
      while (
          start_plus && start_plus < end_plus &&
          (start_plus =
               (uint8_t *)FIO_MEMCHR(start_plus, '+', end_plus - start_plus)))
        *(start_plus++) = ' ';
    }
    dest->len += slice_len;
  }
  dest->buf[dest->len] = 0;
  return r;
}

/** Writes decoded URL data to String. */
SFUNC int fio_string_write_url_dec(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *encoded,
                                   size_t encoded_len) {
  return fio_string_write_url_dec_internal(dest,
                                           reallocate,
                                           encoded,
                                           encoded_len,
                                           1);
}

/** Writes decoded URL data to String. */
SFUNC int fio_string_write_path_dec(fio_str_info_s *dest,
                                    fio_string_realloc_fn reallocate,
                                    const void *encoded,
                                    size_t encoded_len) {
  return fio_string_write_url_dec_internal(dest,
                                           reallocate,
                                           encoded,
                                           encoded_len,
                                           0);
}

/* *****************************************************************************
String HTML escaping support
***************************************************************************** */

/** Writes HTML escaped data to a String. */
SFUNC int fio_string_write_html_escape(fio_str_info_s *dest,
                                       fio_string_realloc_fn reallocate,
                                       const void *data,
                                       size_t data_len) {
  /* produced using the following Ruby script:
    a = (0..255).to_a.map {|i| "&#x#{i.to_s(16)};" }
    must_escape = ['&', '<', '>', '"', "'", '`', '!', '@', '$', '%',
                   '(', ')', '=', '+', '{', '}', '[', ']'] # space?
    (32..123).each {|i| a[i] = i.chr unless must_escape.include?(i.chr) }
    a['<'.ord] = "&lt;"
    a['>'.ord] = "&gt;"
    a['"'.ord] = "&qout;"
    a['&'.ord] = "&amp;"
    b = a.map {|s| s.length }
    puts "static uint8_t html_escape_len[] = {", b.to_s.slice(1..-2), "};"
  */
  static uint8_t html_escape_len[] = {
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 1, 6, 6, 1, 6, 6, 5, 6, 6, 6, 1, 6, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 6, 4, 1, 6, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 1, 6, 1, 1,
      6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};
  int r = 0;
  size_t start = 0;
  size_t pos = 0;
  if (!data_len || !data || !dest)
    return r;
  { /* reserve memory space */
    size_t required_len = data_len;
    for (size_t i = 0; i < data_len; ++i) {
      required_len += html_escape_len[((uint8_t *)data)[i]];
    }
    if (fio_string___write_validate_len(dest, reallocate, &required_len)) {
      return (r = -1); /* no partial encoding. */
    };
  }
  for (;;) { /* copy and encode data */
    while (pos < data_len && html_escape_len[((uint8_t *)data)[pos]] == 1)
      ++pos;
    /* don't escape valid UTF-8 */
    if (pos < data_len)
      switch (
          fio_string_utf8_valid_code_point((void *)(((uint8_t *)data) + pos),
                                           data_len - pos)) {
      case 4: ++pos; /* fall through */
      case 3: ++pos; /* fall through */
      case 2: pos += 2; continue;
      }
    /* copy valid segment before escaping */
    if (pos != start) {
      const size_t len = pos - start;
      FIO_MEMCPY(dest->buf + dest->len, (uint8_t *)data + start, len);
      dest->len += len;
      start = pos;
    }
    if (pos == data_len)
      break;
    /* escape data */
    dest->buf[dest->len++] = '&';
    switch (((uint8_t *)data)[pos]) {
    case '<':
      dest->buf[dest->len++] = 'l';
      dest->buf[dest->len++] = 't';
      break;
    case '>':
      dest->buf[dest->len++] = 'g';
      dest->buf[dest->len++] = 't';
      break;
    case '"':
      dest->buf[dest->len++] = 'q';
      dest->buf[dest->len++] = 'u';
      dest->buf[dest->len++] = 'o';
      dest->buf[dest->len++] = 't';
      break;
    case '&':
      dest->buf[dest->len++] = 'a';
      dest->buf[dest->len++] = 'm';
      dest->buf[dest->len++] = 'p';
      break;
    default:
      dest->buf[dest->len++] = '#';
      dest->buf[dest->len++] = 'x';
      dest->len += ((dest->buf[dest->len] =
                         fio_i2c(((uint8_t *)data)[pos] >> 4)) != '0');
      dest->buf[dest->len++] = fio_i2c(((uint8_t *)data)[pos] & 15);
    }
    dest->buf[dest->len++] = ';';
    ++pos;
    start = pos;
  }
  dest->buf[dest->len] = 0;
  return r;
}

/** Writes HTML un-escaped data to a String - incomplete and minimal. */
SFUNC int fio_string_write_html_unescape(fio_str_info_s *dest,
                                         fio_string_realloc_fn reallocate,
                                         const void *data,
                                         size_t data_len) {
  int r = 0;
  struct {
    uint64_t code;
    uint32_t clen;
    uint8_t r[4];
  } html_named_codes[] = {
#define FIO___STRING_HTML_CODE_POINT(named_code, result)                       \
  {.code = *(uint64_t *)(named_code "\0\0\0\0\0\0\0\0"),                       \
   .clen = (uint32_t)(sizeof(named_code) - 1),                                 \
   .r = result}
      FIO___STRING_HTML_CODE_POINT("lt", "<"),
      FIO___STRING_HTML_CODE_POINT("gt", ">"),
      FIO___STRING_HTML_CODE_POINT("amp", "&"),
      FIO___STRING_HTML_CODE_POINT("apos", "'"),
      FIO___STRING_HTML_CODE_POINT("quot", "\""),
      FIO___STRING_HTML_CODE_POINT("nbsp", "\xC2\xA0"),
      FIO___STRING_HTML_CODE_POINT("tab", "\t"),
      FIO___STRING_HTML_CODE_POINT("ge", "≥"),
      FIO___STRING_HTML_CODE_POINT("le", "≤"),
      FIO___STRING_HTML_CODE_POINT("ne", "≠"),
      FIO___STRING_HTML_CODE_POINT("copy", "©"),
      FIO___STRING_HTML_CODE_POINT("raquo", "»"),
      FIO___STRING_HTML_CODE_POINT("laquo", "«"),
      FIO___STRING_HTML_CODE_POINT("rdquo", "”"),
      FIO___STRING_HTML_CODE_POINT("ldquo", "“"),
      FIO___STRING_HTML_CODE_POINT("reg", "®"),
      FIO___STRING_HTML_CODE_POINT("asymp", "≈"),
      FIO___STRING_HTML_CODE_POINT("bdquo", "„"),
      FIO___STRING_HTML_CODE_POINT("bull", "•"),
      FIO___STRING_HTML_CODE_POINT("cent", "¢"),
      FIO___STRING_HTML_CODE_POINT("euro", "€"),
      FIO___STRING_HTML_CODE_POINT("dagger", "†"),
      FIO___STRING_HTML_CODE_POINT("deg", "°"),
      FIO___STRING_HTML_CODE_POINT("frac14", "¼"),
      FIO___STRING_HTML_CODE_POINT("frac12", "½"),
      FIO___STRING_HTML_CODE_POINT("frac34", "¾"),
      FIO___STRING_HTML_CODE_POINT("hellip", "…"),
      FIO___STRING_HTML_CODE_POINT("lsquo", "‘"),
      FIO___STRING_HTML_CODE_POINT("mdash", "—"),
      FIO___STRING_HTML_CODE_POINT("middot", "·"),
      FIO___STRING_HTML_CODE_POINT("ndash", "–"),
      FIO___STRING_HTML_CODE_POINT("para", "¶"),
      FIO___STRING_HTML_CODE_POINT("plusmn", "±"),
      FIO___STRING_HTML_CODE_POINT("pound", "£"),
      FIO___STRING_HTML_CODE_POINT("prime", "′"),
      FIO___STRING_HTML_CODE_POINT("rsquo", "’"),
      FIO___STRING_HTML_CODE_POINT("sbquo", "‚"),
      FIO___STRING_HTML_CODE_POINT("sect", "§"),
      FIO___STRING_HTML_CODE_POINT("trade", "™"),
      FIO___STRING_HTML_CODE_POINT("yen", "¥"),
  };
  if (!dest || !data || !data_len)
    return r;
  size_t reduced = data_len + dest->len;
  uint8_t *start = (uint8_t *)data;
  uint8_t *const end = start + data_len;
  if (dest->len + data_len >= dest->capa) { /* reserve only what we need */
    reduced = data_len;
    uint8_t *del = start;
    while (end > del && (del = (uint8_t *)FIO_MEMCHR(del, '&', end - del))) {
      uint8_t *tmp = del++;
      /* note that in some cases the `;` might be dropped (history) */
      if (del[0] == '#') {
        ++del;
        del += (del[0] == 'x');
        uint64_t num =
            (del[-1] == 'x' ? fio_atol16u : fio_atol10u)((char **)&del);
        if (del >= end || num > 65535) /* untrusted result */
          continue;
        del += (*del == ';');
        reduced -= (del - tmp);
        reduced += fio___string_utf8_code_point_len(num);
        continue;
      }
      union {
        uint64_t u64;
        uint8_t u8[8];
      } u;
      for (size_t i = 0;
           i < sizeof(html_named_codes) / sizeof(html_named_codes[0]);
           ++i) {
        u.u64 = 0;
        for (size_t p = 0; p < html_named_codes[i].clen; ++p)
          u.u8[p] = del[p] | 32;
        if (u.u64 != html_named_codes[i].code)
          continue;
        del += html_named_codes[i].clen;
        if (del > end)
          break;
        del += (del < end && del[0] == ';');
        reduced -= (del - tmp);
        for (size_t j = 0; html_named_codes[i].r[j]; ++j)
          ++reduced;
        break;
      }
    }
    if (fio_string___write_validate_len(dest, reallocate, &reduced)) {
      return (r = -1); /* no partial decoding. */
    }
    reduced += dest->len;
  }
  { /* copy and unescape data */
    uint8_t *del = start = (uint8_t *)data;
    while (end > (start = del) &&
           (del = (uint8_t *)FIO_MEMCHR(del, '&', end - del))) {
      if (start != del) {
        const size_t len = del - start;
        FIO_MEMCPY(dest->buf + dest->len, start, len);
        dest->len += len;
        start = del;
      }
      ++del;
      if (del == end)
        break;
      if (del[0] == '#') {
        ++del;
        if (del + 2 > end)
          break;
        del += (del[0] == 'x');
        uint64_t num =
            (del[-1] == 'x' ? fio_atol16u : fio_atol10u)((char **)&del);
        if (*del != ';' || num > 65535)
          goto untrusted_no_encode;
        dest->len +=
            fio___string_utf8_write((uint8_t *)dest->buf + dest->len, num);
        del += (del < end && del[0] == ';');
        continue;
      }
      /* note that in some cases the `;` might be dropped (history) */
      for (size_t i = 0;
           i < sizeof(html_named_codes) / sizeof(html_named_codes[0]);
           ++i) {
        union {
          uint64_t u64;
          uint8_t u8[8];
        } u = {0};
        for (size_t p = 0; p < html_named_codes[i].clen; ++p)
          u.u8[p] = del[p] | 32;
        if (u.u64 != html_named_codes[i].code)
          continue;
        del += html_named_codes[i].clen;
        del += (del < end && del[0] == ';');
        start = del;
        for (size_t j = 0; html_named_codes[i].r[j]; ++j) {
          dest->buf[dest->len++] = html_named_codes[i].r[j];
        }
        break;
      }
      if (start == del)
        continue;
    untrusted_no_encode: /* untrusted, don't decode */
      del += (del < end && del[0] == ';');
      FIO_MEMCPY(dest->buf + dest->len, start, del - start);
      dest->len += del - start;
    }
  }
  if (start < end) {
    const size_t len = end - start;
    FIO_MEMCPY(dest->buf + dest->len, start, len);
    dest->len += len;
  }
  dest->buf[dest->len] = 0;
  FIO_ASSERT_DEBUG(dest->len < reduced + 1,
                   "string HTML unescape reduced calculation error");
  return r;
}

/* *****************************************************************************
String File Reading support
***************************************************************************** */

FIO_IFUNC intptr_t fio___string_fd_normalise_offset(intptr_t i,
                                                    size_t file_len) {
  if (i < 0) {
    i += (intptr_t)file_len + 1;
    if (i < 0)
      i = 0;
  }
  return i;
}

/**
 * Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at`.
 *
 * If `limit` is 0 (or less than 0) data will be written until EOF.
 *
 * If `start_at` is negative, position will be calculated from the end of the
 * file where `-1 == EOF`.
 *
 * Note: this will fail unless used on actual files (not sockets, not pipes).
 * */
SFUNC int fio_string_readfd(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            int fd,
                            intptr_t start_at,
                            size_t limit) {
  int r = 0;
  if (!dest) {
    return r;
  }
  size_t file_len = fio_fd_size(fd);
  start_at = fio___string_fd_normalise_offset(start_at, file_len);
  if (!limit || file_len < (size_t)(limit + start_at)) {
    limit = (intptr_t)file_len - start_at;
  }
  if (!file_len || !limit || (size_t)start_at >= file_len) {
    return (r = -1);
  }
  r = fio_string___write_validate_len(dest, reallocate, &limit);
  size_t added = fio_fd_read(fd, dest->buf + dest->len, limit, (off_t)start_at);
  dest->len += added;
  dest->buf[dest->len] = 0;
  return r;
}

/**
 * Opens the file `filename` and pastes it's contents (or a slice ot it) at
 * the end of the String. If `limit == 0`, than the data will be read until
 * EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned in the state's `data` field.
 */
SFUNC int fio_string_readfile(fio_str_info_s *dest,
                              fio_string_realloc_fn reallocate,
                              const char *filename,
                              intptr_t start_at,
                              size_t limit) {
  int r = -1;
  int fd = fio_filename_open(filename, O_RDONLY);
  if (fd == -1)
    return r;
  r = fio_string_readfd(dest, reallocate, fd, start_at, limit);
  close(fd);
  return r;
}

/**
 * Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at` and
 * ending at the first occurrence of `token`.
 *
 * If `limit` is 0 (or less than 0) as much data as may be required will be
 * written.
 *
 * If `start_at` is negative, position will be calculated from the end of the
 * file where `-1 == EOF`.
 *
 * Note: this will fail unless used on actual seekable files (not sockets, not
 * pipes).
 * */
SFUNC int fio_string_getdelim_fd(fio_str_info_s *dest,
                                 fio_string_realloc_fn reallocate,
                                 int fd,
                                 intptr_t start_at,
                                 char delim,
                                 size_t limit) {
  int r = -1;
  if (!dest || fd == -1)
    return (r = 0);
  size_t file_len = fio_fd_size(fd);
  if (!file_len)
    return r;
  start_at = fio___string_fd_normalise_offset(start_at, file_len);
  if ((size_t)start_at >= file_len)
    return r;
  size_t index = fio_fd_find_next(fd, delim, (size_t)start_at);
  if (index == FIO_FD_FIND_EOF)
    return r;
  if (limit < 1 || limit > (index - start_at) + 1) {
    limit = (index - start_at) + 1;
  }

  r = fio_string___write_validate_len(dest, reallocate, &limit);
  size_t added = fio_fd_read(fd, dest->buf + dest->len, limit, (off_t)start_at);
  dest->len += added;
  dest->buf[dest->len] = 0;
  return r;
}

/**
 * Opens the file `filename`, calls `fio_string_getdelim_fd` and closes the
 * file.
 */
SFUNC int fio_string_getdelim_file(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const char *filename,
                                   intptr_t start_at,
                                   char delim,
                                   size_t limit) {
  int r = -1;
  int fd = fio_filename_open(filename, O_RDONLY);
  if (fd == -1)
    return r;
  r = fio_string_getdelim_fd(dest, reallocate, fd, start_at, delim, limit);
  close(fd);
  return r;
}

/* *****************************************************************************
Binary String Type - Embedded Strings
***************************************************************************** */
/** default reallocation callback implementation */
SFUNC int fio_bstr_reallocate(fio_str_info_s *dest, size_t len) {
  fio___bstr_meta_s *bstr_m = NULL;
  const size_t new_capa = fio_string_capa4len(len + sizeof(bstr_m[0]));
  if (!dest->capa)
    goto copy_the_string;
  bstr_m = (fio___bstr_meta_s *)FIO_MEM_REALLOC_(
      ((fio___bstr_meta_s *)dest->buf - 1),
      sizeof(bstr_m[0]) + dest->capa,
      new_capa,
      ((fio___bstr_meta_s *)dest->buf)[-1].len + sizeof(bstr_m[0]));
  if (!bstr_m)
    return -1;
update_metadata:
  dest->buf = (char *)(bstr_m + 1);
  bstr_m->capa = dest->capa = new_capa - sizeof(bstr_m[0]);
  return 0;

copy_the_string:
  bstr_m = (fio___bstr_meta_s *)FIO_MEM_REALLOC_(NULL, 0, new_capa, 0);
  if (!bstr_m)
    return -1;
  if (!FIO_MEM_REALLOC_IS_SAFE_)
    *bstr_m = (fio___bstr_meta_s){0};
  FIO___LEAK_COUNTER_ON_ALLOC(fio_bstr_s);
  if (dest->len) {
    FIO_MEMCPY((bstr_m + 1), dest->buf, dest->len + 1);
    bstr_m->len = dest->len;
  }
  goto update_metadata;
}

/* *****************************************************************************
String Core Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_STR
#endif /* H__FIO_STR__H */
