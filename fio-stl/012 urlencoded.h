/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_URL_ENCODED        /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          URL-Encoded Parser Module




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_URL_ENCODED) && !defined(H___FIO_URL_ENCODED___H)
#define H___FIO_URL_ENCODED___H

/* Dependency: string decoding and temporary-buffer allocation helpers. */
#ifndef H___FIO_STR___H
#define FIO_STR
#define FIO___RECURSIVE_INCLUDE 99 /* preserve FIO_EXTERN linkage */
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#endif

/* *****************************************************************************
URL-Encoded Parser - Overview

This is a callback-based URL-encoded (application/x-www-form-urlencoded) parser.

The parser splits name=value pairs and decodes each name and value before
calling on_pair. Decoding uses temporary stack buffers with heap fallback for
long fields. The input is not modified.

URL-encoded format:
- Pairs separated by `&`
- Name and value separated by `=`
- Special characters are percent-encoded (%XX)

Usage:
    static int my_on_pair(void *udata, fio_buf_info_s name,
                          fio_buf_info_s value) {
        // name and value are decoded and valid only during this callback
        return 0; // non-zero stops parsing; returned unchanged in result.err
    }

    static const fio_url_encoded_parser_callbacks_s callbacks = {
        .on_pair = my_on_pair,
    };

    fio_url_encoded_result_s result = fio_url_encoded_parse(&callbacks,
                                                            my_context,
                                                            data,
                                                            len);
    if (result.err) { handle_error(result.err); }
    // result.consumed: bytes of pairs processed without error
    // result.count: number of pairs processed without error

***************************************************************************** */

/* *****************************************************************************
URL-Encoded Parser Types
***************************************************************************** */

/** The URL-encoded parser callbacks. */
typedef struct {
  /**
   * Called for each non-empty name=value pair.
   *
   * `name` and `value` are decoded (percent escapes and '+' to spaces).
   * Buffers are borrowed, valid only during this callback, and may contain NUL
   * bytes. Copy any data needed after the callback returns; do not free it.
   *
   * Returns 0 to continue. Any other value stops parsing and is returned
   * unchanged in `result.err`.
   */
  int (*on_pair)(void *udata, fio_buf_info_s name, fio_buf_info_s value);
} fio_url_encoded_parser_callbacks_s;

/** The URL-encoded parse result type. */
typedef struct {
  /** Bytes of input processed without error (excludes a failed pair). */
  size_t consumed;
  /** Number of non-empty pairs processed without error. */
  size_t count;
  /** 0 on success, -1 on allocation failure, or the `on_pair` error. */
  int err;
} fio_url_encoded_result_s;

/* *****************************************************************************
URL-Encoded Parser API
***************************************************************************** */

/**
 * Parses URL-encoded data, calling `on_pair` with each decoded name and value.
 *
 * `callbacks` is borrowed for the call. If `callbacks` or `on_pair` is NULL,
 * pairs are only counted: nothing is decoded and nothing is allocated.
 * `udata` is passed to `on_pair` unchanged. `data[0..len)` is not modified.
 * `data` may be NULL only when `len` is zero; empty input returns all zeros.
 *
 * Parsing rules:
 * - Pairs are separated by `&`; empty pairs (`&&`) are skipped
 * - The first `=` separates name and value
 * - `name=` and `name` both have an empty value; `=value` has an empty name
 *
 * Names and values are decoded once using `fio_string_write_url_dec`, including
 * legacy %uXXXX escapes and '+' to spaces. Invalid percent escapes remain
 * literal. Long fields may allocate temporary memory, freed after the callback.
 *
 * On error, parsing stops before the failed pair: `consumed` and `count` cover
 * only the pairs processed without error. `err` is -1 if a temporary buffer
 * could not be allocated, or the non-zero value returned by `on_pair`.
 */
SFUNC fio_url_encoded_result_s
fio_url_encoded_parse(const fio_url_encoded_parser_callbacks_s *callbacks,
                      void *udata,
                      const char *data,
                      size_t len);

/* *****************************************************************************
URL-Encoded Parser Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

SFUNC fio_url_encoded_result_s
fio_url_encoded_parse(const fio_url_encoded_parser_callbacks_s *callbacks,
                      void *udata,
                      const char *data,
                      size_t len) {
  fio_url_encoded_result_s result = {.consumed = 0, .count = 0, .err = 0};
  if (!len)
    return result;
  int (*on_pair)(void *, fio_buf_info_s, fio_buf_info_s) =
      callbacks ? callbacks->on_pair : NULL;
  const char *pos = data;
  const char *end = data + len;

  while (pos < end) {
    /* Find the end of this pair (next '&' or end of data) */
    const char *pair_end = pos;
    while (pair_end < end && *pair_end != '&')
      ++pair_end;

    if (pair_end != pos && on_pair) {
      /* The first '=' splits name and value; no '=' means an empty value. */
      const char *eq = pos;
      while (eq < pair_end && *eq != '=')
        ++eq;
      const char *val = eq + (eq < pair_end);

      /* Decode before the callback, retaining ownership of both buffers. */
      FIO_STR_INFO_TMP_VAR(decoded_name, 1024);
      FIO_STR_INFO_TMP_VAR(decoded_value, 4096);
      int err = -(fio_string_write_url_dec(&decoded_name,
                                           FIO_STRING_ALLOC_COPY,
                                           pos,
                                           (size_t)(eq - pos)) ||
                  fio_string_write_url_dec(&decoded_value,
                                           FIO_STRING_ALLOC_COPY,
                                           val,
                                           (size_t)(pair_end - val)));
      if (!err)
        err = on_pair(udata,
                      FIO_STR2BUF_INFO(decoded_name),
                      FIO_STR2BUF_INFO(decoded_value));
      if (FIO_STR_INFO_TMP_IS_REALLOCATED(decoded_name))
        FIO_STRING_FREE2(decoded_name);
      if (FIO_STR_INFO_TMP_IS_REALLOCATED(decoded_value))
        FIO_STRING_FREE2(decoded_value);
      if (err) {
        result.err = err;
        return result;
      }
    }
    result.count += (pair_end != pos);

    /* Move past this pair and its '&' (empty pairs are skipped). */
    pos = pair_end + (pair_end < end);
    result.consumed = (size_t)(pos - data);
  }
  return result;
}

/* *****************************************************************************
URL-Encoded Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_URL_ENCODED
#endif /* FIO_URL_ENCODED */
