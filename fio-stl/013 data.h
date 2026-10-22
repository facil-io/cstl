/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_DATA               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




              Data Store - RAM Buffer that Spills to a Temporary File



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_DATA) && !defined(H___FIO_DATA___H)
#define H___FIO_DATA___H

/* *****************************************************************************
Data Store - Settings
***************************************************************************** */

#ifndef FIO_DATA_RAM_LIMIT
/** Default number of bytes kept in RAM before spilling to a temporary file. */
#define FIO_DATA_RAM_LIMIT (1UL << 17)
#endif

/* *****************************************************************************
Data Store - Type
***************************************************************************** */

/**
 * A seekable data store that keeps data in RAM until it grows beyond `limit`,
 * at which point it moves to a temporary file.
 *
 * Initialize with `FIO_DATA_INIT` (or `fio_data_init`) and release with
 * `fio_data_destroy`. Fields are read-only for users; use the API.
 */
typedef struct {
  /** RAM data (`fio_bstr`), or the last read once spilled to `fd`. */
  char *buf;
  /** Total data length. */
  size_t len;
  /** Reading position. */
  size_t pos;
  /** RAM limit before spilling to a temporary file (0 = default). */
  size_t limit;
  /** Temporary file descriptor (-1 while the data is in RAM). */
  int fd;
} fio_data_s;

/** Initializes a `fio_data_s` using the default RAM limit. */
#define FIO_DATA_INIT                                                          \
  { .buf = NULL, .len = 0, .pos = 0, .limit = 0, .fd = -1 }

/* *****************************************************************************
Data Store - API
***************************************************************************** */

/** Initializes `d`, setting the RAM limit (0 = `FIO_DATA_RAM_LIMIT`). */
FIO_IFUNC void fio_data_init(fio_data_s *d, size_t ram_limit);

/** Releases all resources (closing any temporary file). `d` stays usable. */
SFUNC void fio_data_destroy(fio_data_s *d);

/** Returns the data length. */
FIO_IFUNC size_t fio_data_len(fio_data_s *d);

/** Returns the reading position. */
FIO_IFUNC size_t fio_data_pos(fio_data_s *d);

/** Returns the temporary file's descriptor, or -1 if the data is in RAM. */
FIO_IFUNC int fio_data_fd(fio_data_s *d);

/**
 * Sets the reading position and returns the new position.
 *
 * Negative values count from the end. `SSIZE_MAX` keeps the current position.
 * The position is clamped to the data length.
 */
SFUNC size_t fio_data_seek(fio_data_s *d, ssize_t pos);

/**
 * Reads up to `len` bytes from the reading position; returns nothing at EOF.
 *
 * The returned buffer is valid until the next read, write or destroy.
 */
SFUNC fio_str_info_s fio_data_read(fio_data_s *d, size_t len);

/**
 * Reads until (and including) `token`, `limit` bytes or EOF.
 *
 * `limit` is ignored if zero or larger than the remaining data. The returned
 * buffer is valid until the next read, write or destroy.
 */
SFUNC fio_str_info_s fio_data_read_until(fio_data_s *d,
                                         char token,
                                         size_t limit);

/** Prepares for `len` more bytes (may spill to a temporary file early). */
SFUNC void fio_data_expect(fio_data_s *d, size_t len);

/**
 * Appends `len` bytes from `data`, spilling to a temporary file once the RAM
 * limit is exceeded.
 *
 * Returns 0 on success or -1 on error (data may be partially written).
 */
SFUNC int fio_data_write(fio_data_s *d, const void *data, size_t len);

/* *****************************************************************************
Data Store - Inline Implementation
***************************************************************************** */

FIO_IFUNC void fio_data_init(fio_data_s *d, size_t ram_limit) {
  *d = (fio_data_s)FIO_DATA_INIT;
  d->limit = ram_limit;
}
FIO_IFUNC size_t fio_data_len(fio_data_s *d) { return d->len; }
FIO_IFUNC size_t fio_data_pos(fio_data_s *d) { return d->pos; }
FIO_IFUNC int fio_data_fd(fio_data_s *d) { return d->fd; }

/* *****************************************************************************
Data Store - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO_IFUNC size_t fio___data_limit(fio_data_s *d) {
  return d->limit ? d->limit : (size_t)FIO_DATA_RAM_LIMIT;
}

/** Moves RAM data to a new temporary file. Returns -1 on error. */
FIO_SFUNC int fio___data_move2fd(fio_data_s *d) {
  d->fd = fio_filename_tmp();
  if (d->fd == -1) {
    static int error_printed = 0;
    if (!error_printed) {
      error_printed = 1;
      FIO_LOG_ERROR("fio_data_s couldn't open temporary file! (%d) %s",
                    errno,
                    strerror(errno));
    }
    return -1;
  }
  fio_buf_info_s b = fio_bstr_buf(d->buf);
  if (!b.len)
    return 0;
  ssize_t written = fio_fd_write(d->fd, b.buf, b.len);
  if (written == (ssize_t)b.len)
    return 0;
  close(d->fd);
  FIO_LOG_ERROR("fio_data_s couldn't transfer data to temporary file "
                "(transferred %zd / %zu)",
                written,
                b.len);
  return (d->fd = -1);
}

SFUNC void fio_data_destroy(fio_data_s *d) {
  if (!d)
    return;
  fio_bstr_free(d->buf);
  if (d->fd != -1)
    close(d->fd);
  d->buf = NULL;
  d->len = d->pos = 0;
  d->fd = -1;
}

SFUNC size_t fio_data_seek(fio_data_s *d, ssize_t pos) {
  if (pos == SSIZE_MAX)
    pos = (ssize_t)d->pos;
  if (pos < 0) {
    pos += (ssize_t)d->len;
    if (pos < 0)
      pos = 0;
  }
  if ((size_t)pos >= d->len)
    pos = (ssize_t)d->len;
  d->pos = (size_t)pos;
  return d->pos;
}

SFUNC fio_str_info_s fio_data_read(fio_data_s *d, size_t len) {
  fio_str_info_s r = {0};
  if (d->pos >= d->len)
    return r;
  if (len > d->len - d->pos)
    len = d->len - d->pos;
  if (d->fd == -1) {
    r = FIO_STR_INFO2((d->buf + d->pos), len);
  } else {
    d->buf = fio_bstr_len_set(d->buf, 0);
    d->buf = fio_bstr_readfd(d->buf, d->fd, (intptr_t)d->pos, (intptr_t)len);
    r = fio_bstr_info(d->buf);
  }
  d->pos += r.len;
  return r;
}

SFUNC fio_str_info_s fio_data_read_until(fio_data_s *d,
                                         char token,
                                         size_t limit) {
  fio_str_info_s r = {0};
  if (d->pos >= d->len)
    return r;
  if (!limit || limit > d->len - d->pos)
    limit = d->len - d->pos;
  if (d->fd == -1) {
    r = FIO_STR_INFO2((d->buf + d->pos), limit);
    char *end = (char *)FIO_MEMCHR(r.buf, token, limit);
    if (end)
      r.len = (size_t)(end + 1 - r.buf);
  } else {
    d->buf = fio_bstr_len_set(d->buf, 0);
    d->buf = fio_bstr_getdelim_fd(d->buf, d->fd, (intptr_t)d->pos, token, limit);
    r = fio_bstr_info(d->buf);
  }
  d->pos += r.len;
  if (!r.len)
    r.buf = NULL;
  return r;
}

SFUNC void fio_data_expect(fio_data_s *d, size_t len) {
  if (d->fd != -1)
    return;
  if (len + d->len > fio___data_limit(d)) {
    fio___data_move2fd(d);
    return;
  }
  d->buf = fio_bstr_reserve(d->buf, len);
}

SFUNC int fio_data_write(fio_data_s *d, const void *data, size_t len) {
  if (!data || !len)
    return 0;
  if (d->fd == -1) {
    if (len + d->len <= fio___data_limit(d) || fio___data_move2fd(d)) {
      d->buf = fio_bstr_write(d->buf, data, len); /* RAM (or spill failed) */
      d->len += len;
      return 0;
    }
  }
  ssize_t written = fio_fd_write(d->fd, data, len);
  if (written > 0)
    d->len += (size_t)written;
  return 0 - (written != (ssize_t)len);
}

/* *****************************************************************************
Data Store - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_DATA */
#undef FIO_DATA
