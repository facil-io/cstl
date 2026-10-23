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
    static void *my_on_pair(void *udata, fio_buf_info_s name, fio_buf_info_s
value) {
        // Process name=value pair
        // name and value are decoded and valid only during this callback
        return udata;
    }

    static const fio_url_encoded_parser_callbacks_s callbacks = {
        .on_pair = my_on_pair,
    };

    fio_url_encoded_result_s result = fio_url_encoded_parse(&callbacks,
                                                            my_context,
                                                            data,
                                                            len);
    if (result.err) { handle_error(); }
    // result.consumed indicates bytes consumed
    // result.count indicates number of pairs found

***************************************************************************** */

/* *****************************************************************************
URL-Encoded Parser Types
***************************************************************************** */

/**
 * The URL-encoded parser callbacks.
 *
 * Callbacks receive `udata` as their first argument.
 */
typedef struct {
  /**
   * Called for each name=value pair found.
   *
   * `name` and `value` are decoded (percent escapes and '+' to spaces).
   * Buffers are borrowed, valid only during this callback, and may contain NUL
   * bytes. Copy any data needed after the callback returns; do not free it.
   *
   * Returns the (possibly updated) udata, or NULL to stop parsing.
   */
  void *(*on_pair)(void *udata, fio_buf_info_s name, fio_buf_info_s value);

  /**
   * Called if decoding cannot allocate a temporary buffer (optional).
   *
   * Receives the current udata after temporary buffers are freed. The failed
   * pair is not reported. Not called when on_pair returns NULL.
   */
  void (*on_error)(void *udata);

} fio_url_encoded_parser_callbacks_s;

/** The URL-encoded parse result type. */
typedef struct {
  /** Number of bytes consumed from the buffer. */
  size_t consumed;
  /** Number of name=value pairs found. */
  size_t count;
  /** Non-zero if decoding failed or a callback returned NULL. */
  int err;
} fio_url_encoded_result_s;

/* *****************************************************************************
URL-Encoded Parser API
***************************************************************************** */

/**
 * Parse URL-encoded data from buffer.
 *
 * `callbacks` contains the callback functions (should be static const).
 * `udata` is user data passed to callbacks.
 * `data` is the URL-encoded data to parse.
 * `len` is the length of the data.
 *
 * Returns a result struct containing:
 * - `consumed`: Number of bytes consumed from the buffer
 * - `count`: Number of name=value pairs found
 * - `err`: Non-zero if decoding failed or a callback returned NULL
 *
 * Parsing rules:
 * - Pairs are separated by `&`
 * - Name and value are separated by `=`
 * - Empty value is valid: `name=` → value.len = 0
 * - Missing `=` means value is empty: `name` → name="name", value.len = 0
 * - Empty name with value: `=value` → name.len = 0, value="value"
 * - Empty pairs (`&&`) are skipped
 *
 * Names and values are decoded once using `fio_string_write_url_dec`, including
 * '+' to spaces. Invalid percent escapes remain literal. Callback buffers are
 * borrowed and valid only until the callback returns. The input is not
 * modified. Long fields may allocate temporary memory, which is freed after the
 * callback. On allocation failure, on_error is called and consumed excludes the
 * failed pair; count includes only pairs passed to on_pair.
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

/* *****************************************************************************
Internal Helper: No-op callbacks
***************************************************************************** */

FIO_IFUNC void *fio___url_encoded_noop_on_pair(void *udata,
                                               fio_buf_info_s name,
                                               fio_buf_info_s value) {
  /* Return non-NULL sentinel to continue parsing even if udata is NULL */
  return udata ? udata : (void *)(uintptr_t)1;
  (void)name;
  (void)value;
}

FIO_IFUNC void fio___url_encoded_noop_on_error(void *udata) { (void)udata; }

/* *****************************************************************************
Internal: Validated callbacks wrapper
***************************************************************************** */

typedef struct {
  void *(*on_pair)(void *udata, fio_buf_info_s name, fio_buf_info_s value);
  void (*on_error)(void *udata);
} fio___url_encoded_cb_s;

FIO_SFUNC fio___url_encoded_cb_s fio___url_encoded_callbacks_validate(
    const fio_url_encoded_parser_callbacks_s *cb) {
  fio___url_encoded_cb_s r;
  static const fio_url_encoded_parser_callbacks_s empty_cb = {0};
  if (!cb)
    cb = &empty_cb;
  r.on_pair = cb->on_pair ? cb->on_pair : fio___url_encoded_noop_on_pair;
  r.on_error = cb->on_error ? cb->on_error : fio___url_encoded_noop_on_error;
  return r;
}

/* *****************************************************************************
URL-Encoded Main Parse Function
***************************************************************************** */

SFUNC fio_url_encoded_result_s
fio_url_encoded_parse(const fio_url_encoded_parser_callbacks_s *callbacks,
                      void *udata,
                      const char *data,
                      size_t len) {
  fio_url_encoded_result_s result = {.consumed = 0, .count = 0, .err = 0};
  const char *pos = data;
  const char *end = data + len;

  fio___url_encoded_cb_s cb = fio___url_encoded_callbacks_validate(callbacks);

  while (pos < end) {
    /* Find the end of this pair (next '&' or end of data) */
    const char *pair_end = pos;
    while (pair_end < end && *pair_end != '&')
      ++pair_end;

    /* Skip empty pairs (e.g., "&&" or leading "&") */
    if (pair_end == pos) {
      ++pos;
      continue;
    }

    /* Find the '=' separator within this pair */
    const char *eq = pos;
    while (eq < pair_end && *eq != '=')
      ++eq;

    fio_buf_info_s name;
    fio_buf_info_s value;

    if (eq < pair_end) {
      /* Found '=' - split into name and value */
      name.buf = (char *)pos;
      name.len = (size_t)(eq - pos);
      value.buf = (char *)(eq + 1);
      value.len = (size_t)(pair_end - (eq + 1));
    } else {
      /* No '=' found - entire segment is the name, value is empty */
      name.buf = (char *)pos;
      name.len = (size_t)(pair_end - pos);
      value.buf = (char *)pair_end; /* Points to end, len = 0 */
      value.len = 0;
    }

    /* Decode before the callback, retaining ownership of both buffers. */
    FIO_STR_INFO_TMP_VAR(decoded_name, 1024);
    FIO_STR_INFO_TMP_VAR(decoded_value, 4096);
    int decode_err = fio_string_write_url_dec(&decoded_name,
                                              FIO_STRING_ALLOC_COPY,
                                              name.buf,
                                              name.len) ||
                     fio_string_write_url_dec(&decoded_value,
                                              FIO_STRING_ALLOC_COPY,
                                              value.buf,
                                              value.len);
    if (!decode_err) {
      udata = cb.on_pair(udata,
                         FIO_STR2BUF_INFO(decoded_name),
                         FIO_STR2BUF_INFO(decoded_value));
      ++result.count;
    }
    if (FIO_STR_INFO_TMP_IS_REALLOCATED(decoded_name))
      FIO_STRING_FREE2(decoded_name);
    if (FIO_STR_INFO_TMP_IS_REALLOCATED(decoded_value))
      FIO_STRING_FREE2(decoded_value);

    /* Clean up even when decoding fails or the callback stops parsing. */
    if (decode_err || !udata) {
      result.err = 1;
      result.consumed = (size_t)((decode_err ? pos : pair_end) - data);
      if (decode_err)
        cb.on_error(udata);
      return result;
    }

    /* Move past this pair */
    pos = pair_end;
    if (pos < end && *pos == '&')
      ++pos;
  }

  result.consumed = (size_t)(pos - data);
  return result;
}

/* *****************************************************************************
URL-Encoded Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_URL_ENCODED
#endif /* FIO_URL_ENCODED */
