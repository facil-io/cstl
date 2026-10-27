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

#ifndef FIO_DATA_READ_BLOCK
/**
 * Block size used when the temporary file can't be memory mapped (block reads
 * for `fio_data_read_until`, smallest read buffer).
 */
#define FIO_DATA_READ_BLOCK (1UL << 14)
#endif

/*
 * Define `FIO_DATA_NO_MMAP` to never memory map temporary files (always use
 * the heap read buffer fallback). Systems without `sys/mman.h` always do.
 */
#if !defined(FIO_DATA_NO_MMAP) && (FIO_OS_POSIX || __has_include("sys/mman.h"))
#include <sys/mman.h>
#define FIO___DATA_MMAP 1
#else
#define FIO___DATA_MMAP 0
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
  /** RAM data (`fio_bstr`); once spilled, the file mapping or a read buffer. */
  char *buf;
  /** Total data length (bytes actually stored). */
  size_t len;
  /** Reading position. */
  size_t pos;
  /** RAM limit before spilling to a temporary file (0 = default). */
  size_t limit;
  /** Once spilled: bytes mapped (if `map == 1`) or read buffer capacity. */
  size_t capa;
  /** Heap read buffer window: file offset of the bytes held in `buf`. */
  size_t woff;
  /** Heap read buffer window: file bytes held in `buf` (0 = none). */
  size_t wlen;
  /** Temporary file descriptor (-1 while the data is in RAM). */
  int fd;
  /** Sticky error flag (set when a write fails; cleared by destroy). */
  uint8_t err;
  /** Once spilled: 1 if `buf` maps the file, 2 if mapping failed, else 0. */
  uint8_t map;
} fio_data_s;

/** Initializes a `fio_data_s` using the default RAM limit. */
#define FIO_DATA_INIT                                                          \
  {                                                                            \
    .buf = NULL, .len = 0, .pos = 0, .limit = 0, .capa = 0, .woff = 0,         \
    .wlen = 0, .fd = -1, .err = 0, .map = 0                                    \
  }

/* *****************************************************************************
Data Store - API
***************************************************************************** */

/** Initializes `d`, setting the RAM limit (0 = `FIO_DATA_RAM_LIMIT`). */
FIO_IFUNC void fio_data_init(fio_data_s *d, size_t ram_limit);

/**
 * Releases all resources (closing any temporary file) and clears the error
 * state. `d` stays usable (and keeps its RAM limit).
 */
SFUNC void fio_data_destroy(fio_data_s *d);

/** Returns the data length (bytes actually stored). */
FIO_IFUNC size_t fio_data_len(fio_data_s *d);

/** Returns the reading position. */
FIO_IFUNC size_t fio_data_pos(fio_data_s *d);

/** Returns the temporary file's descriptor, or -1 if the data is in RAM. */
FIO_IFUNC int fio_data_fd(fio_data_s *d);

/**
 * Returns the RAM data (a `fio_bstr`), or NULL once spilled to a temporary
 * file (when `fio_data_fd(d) != -1`) or before any data was stored. The
 * inverse of `fio_data_fd`.
 *
 * Borrowed: any later `fio_data_*` call MAY invalidate it. To keep a
 * reference, use `fio_bstr_copy` (zero-copy; later writes copy the buffer
 * before changing it) and release it with `fio_bstr_free`.
 */
FIO_IFUNC char *fio_data_bstr(fio_data_s *d);

/**
 * Returns 1 if the store is in an error state (a write failed), 0 otherwise.
 *
 * Once set, the state is sticky: all writes fail (return -1) without writing
 * until `fio_data_destroy` is called. The data that was stored before the
 * failure (`fio_data_len` bytes) remains readable.
 */
FIO_IFUNC int fio_data_is_error(fio_data_s *d);

/**
 * Sets the reading position and returns the new position.
 *
 * Negative values count back from the end: `-n` is `len - n`, so `-1` is the
 * last byte (as with `lseek(fd, -1, SEEK_END)`); values before the start clamp
 * to 0. Any position at or past the end is EOF (i.e., `fio_data_len(d)` or
 * `SSIZE_MAX`).
 */
SFUNC size_t fio_data_seek(fio_data_s *d, ssize_t pos);

/**
 * Reads up to `len` bytes from the reading position; returns nothing at EOF.
 *
 * Use `(size_t)-1` to read all the remaining data. Once spilled, data is read
 * through a memory mapping of the temporary file; if mapping isn't available,
 * through a heap buffer of (at least) the requested size, which remembers the
 * file range it holds: overlapping reads only read the bytes not yet held.
 *
 * If that buffer can't be allocated, or the file returns fewer bytes than
 * requested (a short read: an I/O error or a file truncated by someone else;
 * `EINTR` is retried), an error is logged and an empty result is returned
 * without moving the reading position and keeping `errno` (`EIO` for a short
 * read without an error). Tell it apart from EOF by
 * `fio_data_pos(d) < fio_data_len(d)`. Truncated data is never returned. The
 * stored data stays intact (this is not a store error state).
 *
 * The returned buffer is borrowed: any later `fio_data_*` call on the same
 * store MAY invalidate it.
 */
SFUNC fio_str_info_s fio_data_read(fio_data_s *d, size_t len);

/**
 * Reads until (and including) `token`, `limit` bytes or EOF.
 *
 * `limit` is ignored if zero or larger than the remaining data. Fails like
 * `fio_data_read` (empty result, position unchanged, errno kept) if the heap
 * read buffer can't be allocated or a block read is short.
 *
 * The returned buffer is borrowed: any later `fio_data_*` call on the same
 * store MAY invalidate it.
 */
SFUNC fio_str_info_s fio_data_read_until(fio_data_s *d,
                                         char token,
                                         size_t limit);

/**
 * Prepares for `len` more bytes: reserves RAM, or moves the data to a
 * temporary file early if the expected size crosses the RAM limit.
 *
 * This is only a hint: if RAM can't be reserved or no temporary file can be
 * created, the data stays intact and no error state is set. A later write
 * retries the allocation or spill and reports failure if it can't proceed.
 */
SFUNC void fio_data_expect(fio_data_s *d, size_t len);

/**
 * Appends `len` bytes from `data`, spilling to a temporary file once the RAM
 * limit would be exceeded.
 *
 * Returns 0 on success or -1 on error. RAM allocation failure keeps the
 * previous data intact and enters the sticky error state. Failed file writes
 * are retried while they make progress; after two consecutive attempts that
 * store nothing, the store enters a (sticky) error state: only the data
 * actually stored is kept (`fio_data_len`) and all later writes fail (see
 * `fio_data_is_error`). RAM never grows past the RAM limit.
 */
SFUNC int fio_data_write(fio_data_s *d, const void *data, size_t len);

/**
 * Saves all the data to `filename`, atomically replacing any existing file.
 *
 * Uses `fio_filename_overwrite` for RAM data and `fio_fd_save` for spilled
 * data (a kernel side copy of the temporary file where available):
 * the data is written to a new sibling file in `filename`'s folder, flushed
 * (`fsync`) and renamed over `filename` (new files use mode `0666`, minus the
 * `umask`; a leading `"~/"` is the home folder). On error, any existing
 * `filename` is left untouched, no new file is left behind and `errno` is
 * kept. A short read of the temporary file fails the save (never truncates).
 * One `fsync` (the file, not the folder): a crash right after saving may leave
 * the previous version.
 *
 * Returns 0 on success or -1 on error (refuses with `EIO` while
 * `fio_data_is_error`). The store's data, reading position and previously
 * returned buffers are unchanged.
 */
SFUNC int fio_data_save(fio_data_s *d, const char *filename);

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
FIO_IFUNC char *fio_data_bstr(fio_data_s *d) {
  return (d->fd == -1) ? d->buf : NULL;
}
FIO_IFUNC int fio_data_is_error(fio_data_s *d) { return !!d->err; }

/* *****************************************************************************
Data Store - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/** The largest RAM buffer (kept below the `fio_bstr` size limit). */
#define FIO___DATA_RAM_MAX ((size_t)0x7FFFFFF0UL)

/* `fio_data_s.map` values (spilled data). */
#define FIO___DATA_MAP_NONE   0 /* `buf` is NULL or a heap read buffer */
#define FIO___DATA_MAP_ON     1 /* `buf` maps the first `capa` file bytes */
#define FIO___DATA_MAP_FAILED 2 /* as NONE, mapping failed (until a write) */

#ifndef FIO___DATA_REALLOC
/** Read buffer (re)allocation (a testing hook: same as `FIO_MEM_REALLOC_`). */
#define FIO___DATA_REALLOC(ptr, old_size, new_size, copy_len)                  \
  FIO_MEM_REALLOC_((ptr), (old_size), (new_size), (copy_len))
#endif

#ifndef FIO___DATA_WRITE
/** Low level file write (a testing hook: same contract as `fio_fd_write`). */
#define FIO___DATA_WRITE(fd, buf, len) fio_fd_write((fd), (buf), (len))
#endif

/** The effective RAM limit (default when 0, clamped to the bstr limit). */
FIO_IFUNC size_t fio___data_limit(fio_data_s *d) {
  size_t r = d->limit ? d->limit : (size_t)FIO_DATA_RAM_LIMIT;
  return (r > FIO___DATA_RAM_MAX) ? FIO___DATA_RAM_MAX : r;
}

/**
 * Makes sure the RAM data (`d->buf`) can hold `total` bytes (+ NUL), keeping
 * its content.
 *
 * Grows geometrically, up to `max` bytes. `total` MUST NOT exceed
 * `FIO___DATA_RAM_MAX`. Returns -1 on allocation failure, keeping the
 * original buffer and any shared references intact.
 */
FIO_SFUNC int fio___data_reserve(fio_data_s *d, size_t total, size_t max) {
  fio_str_info_s i = fio_bstr_info(d->buf);
  if (d->buf && total < i.capa) /* (a shared buffer reports capa 1: COW) */
    return 0;
  size_t grow = i.capa + (i.capa >> 1);
  if (grow > total && grow <= max)
    total = grow;
  if (fio_bstr_reallocate(&i, total)) {
    errno = ENOMEM;
    return -1;
  }
  d->buf = fio_bstr_len_set(i.buf, i.len);
  return 0;
}

/** Enters the (sticky) error state. Always returns -1. */
FIO_SFUNC int fio___data_set_error(fio_data_s *d, const char *what) {
  d->err = 1;
  FIO_LOG_ERROR("fio_data_s %s (%d) %s - entering error state (stored %zu)",
                what,
                errno,
                strerror(errno),
                d->len);
  (void)what; /* logging may be compiled out */
  return -1;
}

/**
 * Writes `len` bytes to `fd` at offset `at`, returning the bytes written.
 *
 * Retries for as long as attempts make progress; gives up after two
 * consecutive attempts that write nothing.
 */
FIO_SFUNC size_t fio___data_write_at(int fd,
                                     const char *buf,
                                     size_t len,
                                     size_t at) {
  size_t total = 0;
  for (int fails = 0; total < len && fails < 2;) {
    ssize_t w = -1;
    if (lseek(fd, (off_t)(at + total), SEEK_SET) != (off_t)-1)
      w = FIO___DATA_WRITE(fd, buf + total, len - total);
    if (w <= 0) {
      ++fails;
      continue;
    }
    fails = 0;
    total += ((size_t)w > len - total) ? (len - total) : (size_t)w;
  }
  return total;
}

/** Releases the read buffer / mapping of spilled data (resets `map`). */
FIO_SFUNC void fio___data_release_view(fio_data_s *d) {
#if FIO___DATA_MMAP
  if (d->map == FIO___DATA_MAP_ON) {
    munmap((void *)d->buf, d->capa);
    d->buf = NULL;
    d->capa = 0;
  }
#endif
  if (d->buf)
    FIO_MEM_FREE_(d->buf, d->capa);
  d->buf = NULL;
  d->capa = 0;
  d->wlen = 0;
  d->map = FIO___DATA_MAP_NONE;
}

/** Invalidates the mapping / read window before a write changes the file. */
FIO_IFUNC void fio___data_unmap(fio_data_s *d) {
  if (d->map == FIO___DATA_MAP_ON)
    fio___data_release_view(d);
  d->wlen = 0; /* (conservative: appends don't change buffered bytes) */
  d->map = FIO___DATA_MAP_NONE; /* a write gives mapping another chance */
}

/**
 * Returns a pointer to all of the data (RAM or a mapping of the temporary
 * file, mapped lazily), or NULL if the heap read buffer must be used.
 */
FIO_SFUNC char *fio___data_ptr(fio_data_s *d) {
  if (d->fd == -1)
    return d->buf;
#if FIO___DATA_MMAP
  if (d->map == FIO___DATA_MAP_NONE && d->len) {
    void *m = mmap(NULL, d->len, PROT_READ, MAP_SHARED, d->fd, 0);
    if (m == MAP_FAILED) {
      d->map = FIO___DATA_MAP_FAILED;
      FIO_LOG_DEBUG2("fio_data_s couldn't map %zu bytes (%d) %s - using heap",
                     d->len,
                     errno,
                     strerror(errno));
      return NULL;
    }
    fio___data_release_view(d); /* frees any heap read buffer */
    d->buf = (char *)m;
    d->capa = d->len;
    d->map = FIO___DATA_MAP_ON;
  }
  if (d->map == FIO___DATA_MAP_ON)
    return d->buf;
#endif
  return NULL;
}

/**
 * Makes the heap read buffer hold at least `want` bytes, keeping the first
 * `keep` bytes. Returns 0 on success.
 *
 * On allocation failure, logs an error and returns -1 (the read buffer stays
 * valid if `keep`). Never aborts: `want` may be controlled by a remote peer.
 */
FIO_SFUNC int fio___data_heap_reserve(fio_data_s *d, size_t want, size_t keep) {
  if (d->capa >= want)
    return 0;
  if (!keep && d->buf) { /* nothing to keep: avoid copying on realloc */
    FIO_MEM_FREE_(d->buf, d->capa);
    d->buf = NULL;
    d->capa = 0;
    d->wlen = 0;
  }
  void *tmp = FIO___DATA_REALLOC(d->buf, d->capa, want, keep);
  if (!tmp) {
    FIO_LOG_ERROR("fio_data_s couldn't allocate a %zu byte read buffer "
                  "(read failed, data intact)",
                  want);
    return -1;
  }
  d->buf = (char *)tmp;
  d->capa = want;
  return 0;
}

/**
 * Reads exactly `len` bytes at `at` from the temporary file to `dest`.
 *
 * `fio_fd_read` retries `EINTR`; a short read (EOF before `d->len` or a read
 * error) is logged and returns -1, keeping errno (`EIO` if the read didn't set
 * one). Data is never returned truncated.
 */
FIO_SFUNC int fio___data_fd_read(fio_data_s *d,
                                 char *dest,
                                 size_t len,
                                 size_t at) {
  errno = 0;
  size_t got = fio_fd_read(d->fd, dest, len, (off_t)at);
  if (got == len)
    return 0;
  if (!errno)
    errno = EIO;
  int e = errno;
  FIO_LOG_ERROR("fio_data_s short read from temporary file (%zu / %zu bytes "
                "at %zu) (%d) %s - read failed",
                (got > len ? (size_t)0 : got),
                len,
                at,
                e,
                strerror(e));
  errno = e;
  return -1;
}

/**
 * Loads the file bytes [at, at + len) to the heap read buffer, returning a
 * pointer to them (NULL on error, see `fio___data_heap_reserve` and
 * `fio___data_fd_read`). `at + len` MUST NOT exceed `d->len`.
 *
 * The buffer remembers the file range it holds (`woff`, `wlen`): bytes
 * already held are reused, never read again. If the range is held, a pointer
 * into the buffer is returned; otherwise any held bytes at `at` are moved to
 * the buffer's start and only the missing bytes are read. If the buffer must
 * grow, it grows to `grow` (`>= len`) bytes. On error the window stays valid.
 */
FIO_SFUNC char *fio___data_heap_load(fio_data_s *d,
                                     size_t at,
                                     size_t len,
                                     size_t grow) {
  size_t keep = 0;
  if (at >= d->woff && at - d->woff < d->wlen) {
    size_t ofs = at - d->woff;
    keep = d->wlen - ofs;
    if (keep >= len)
      return d->buf + ofs;
    if (ofs)
      FIO_MEMMOVE(d->buf, d->buf + ofs, keep);
  }
  d->woff = at;
  d->wlen = keep;
  if (d->capa < len && fio___data_heap_reserve(d, grow, keep))
    return NULL;
  if (fio___data_fd_read(d, d->buf + keep, len - keep, at + keep))
    return NULL;
  d->wlen = len;
  return d->buf;
}

/**
 * Reads up to `len` bytes at `at` (`at < d->len`) to the heap read buffer.
 *
 * Returns an empty result if the buffer can't be allocated or on a short read.
 */
FIO_SFUNC fio_str_info_s fio___data_heap_read(fio_data_s *d,
                                              size_t at,
                                              size_t len) {
  fio_str_info_s r = {0};
  if (len > d->len - at)
    len = d->len - at;
  char *p = fio___data_heap_load(d, at, len, len);
  if (p)
    r = FIO_STR_INFO2(p, len);
  return r;
}

/**
 * Moves RAM data to a new temporary file.
 *
 * On success the RAM buffer is released. On failure the RAM data is kept
 * intact and -1 is returned (the error state is NOT set).
 */
FIO_SFUNC int fio___data_move2fd(fio_data_s *d) {
  fio_buf_info_s b = fio_bstr_buf(d->buf);
  int fd = fio_filename_tmp();
  if (fd == -1) /* retry once (zero progress) */
    fd = fio_filename_tmp();
  if (fd == -1)
    return -1;
  if (fio___data_write_at(fd, b.buf, b.len, 0) != b.len) {
    int e = errno;
    close(fd);
    errno = e;
    return -1;
  }
  fio_bstr_free(d->buf);
  d->buf = NULL;
  d->capa = d->wlen = 0;
  d->map = FIO___DATA_MAP_NONE;
  d->fd = fd;
  return 0;
}

/** Appends to the temporary file; sets the error state on error. */
FIO_SFUNC int fio___data_fd_write(fio_data_s *d, const char *data, size_t len) {
  if (len > ((size_t)SSIZE_MAX - d->len)) {
    errno = EFBIG;
    return fio___data_set_error(d, "length overflow");
  }
  fio___data_unmap(d);
  /* always append at `len` (reads may move the file offset). */
  size_t w = fio___data_write_at(d->fd, data, len, d->len);
  d->len += w;
  if (w == len)
    return 0;
  return fio___data_set_error(d, "couldn't write to temporary file");
}

SFUNC void fio_data_destroy(fio_data_s *d) {
  if (!d)
    return;
  if (d->fd == -1) {
    fio_bstr_free(d->buf);
    d->buf = NULL;
  } else {
    fio___data_release_view(d);
    close(d->fd);
  }
  d->buf = NULL;
  d->len = d->pos = d->capa = d->woff = d->wlen = 0;
  d->fd = -1;
  d->err = 0;
  d->map = FIO___DATA_MAP_NONE;
}

SFUNC size_t fio_data_seek(fio_data_s *d, ssize_t pos) {
  if (pos < 0) { /* -n == len - n (-1 is the last byte), clamped to 0 */
    size_t back = (size_t)(-(pos + 1)) + 1; /* no overflow for SSIZE_MIN */
    d->pos = (back >= d->len) ? 0 : (d->len - back);
    return d->pos;
  }
  d->pos = ((size_t)pos > d->len) ? d->len : (size_t)pos;
  return d->pos;
}

SFUNC fio_str_info_s fio_data_read(fio_data_s *d, size_t len) {
  fio_str_info_s r = {0};
  if (d->pos >= d->len || !len)
    return r;
  if (len > d->len - d->pos)
    len = d->len - d->pos;
  char *p = fio___data_ptr(d);
  if (p)
    r = FIO_STR_INFO2((p + d->pos), len);
  else
    r = fio___data_heap_read(d, d->pos, len);
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
  char *p = fio___data_ptr(d);
  if (p) { /* RAM or mapped: pointer math only */
    r = FIO_STR_INFO2((p + d->pos), limit);
    char *end = (char *)FIO_MEMCHR(r.buf, token, limit);
    if (end)
      r.len = (size_t)(end + 1 - r.buf);
  } else { /* heap: load blocks (reusing the window), scan new bytes once */
    size_t scanned = 0;
    for (;;) {
      size_t need = limit - scanned;
      if (need > (size_t)FIO_DATA_READ_BLOCK)
        need = (size_t)FIO_DATA_READ_BLOCK;
      need += scanned;
      if (d->pos >= d->woff && d->pos - d->woff < d->wlen &&
          d->woff + d->wlen - d->pos > scanned) /* scan held bytes first */
        need = d->woff + d->wlen - d->pos;
      if (need > limit)
        need = limit;
      size_t grow = d->capa + (d->capa >> 1);
      if (grow < need)
        grow = need;
      if (grow > limit)
        grow = limit;
      r.buf = fio___data_heap_load(d, d->pos, need, grow);
      if (!r.buf)
        return (fio_str_info_s){0}; /* `pos` unchanged, errno kept */
      size_t have = d->woff + d->wlen - d->pos; /* all held bytes */
      if (have > limit)
        have = limit;
      char *end = (char *)FIO_MEMCHR(r.buf + scanned, token, have - scanned);
      if (end) {
        r.len = (size_t)(end + 1 - r.buf);
        break;
      }
      scanned = r.len = have;
      if (r.len >= limit)
        break;
    }
  }
  d->pos += r.len;
  if (!r.len)
    r.buf = NULL;
  return r;
}

SFUNC void fio_data_expect(fio_data_s *d, size_t len) {
  if (d->fd != -1 || d->err)
    return;
  const size_t limit = fio___data_limit(d);
  if (d->len > limit || len > limit - d->len) {
    if (fio___data_move2fd(d))
      FIO_LOG_WARNING("fio_data_s couldn't spill to a temporary file (%d) %s",
                      errno,
                      strerror(errno));
    return;
  }
  fio___data_reserve(d, d->len + len, limit);
}

SFUNC int fio_data_write(fio_data_s *d, const void *data, size_t len) {
  if (d->err)
    return -1;
  if (!data || !len)
    return 0;
  if (d->fd == -1) {
    const size_t limit = fio___data_limit(d);
    if (d->len <= limit && len <= limit - d->len) { /* fits in RAM */
      if (fio___data_reserve(d, d->len + len, limit))
        return fio___data_set_error(d, "couldn't grow RAM storage");
      FIO_MEMCPY(d->buf + d->len, data, len);
      d->len += len;
      d->buf = fio_bstr_len_set(d->buf, d->len);
      return 0;
    }
    if (fio___data_move2fd(d)) /* RAM data stays intact */
      return fio___data_set_error(d, "couldn't spill to a temporary file");
  }
  return fio___data_fd_write(d, (const char *)data, len);
}

/* *****************************************************************************
Data Store - Saving to a File
***************************************************************************** */

SFUNC int fio_data_save(fio_data_s *d, const char *filename) {
  if (!d || !filename || !filename[0]) {
    errno = EINVAL;
    return -1;
  }
  if (d->err) {
    errno = EIO;
    return -1;
  }
  /* RAM: save the buffer; spilled: the kernel copies the temporary file (the
   * mapping / heap read buffer is left untouched). */
  if (d->fd == -1)
    return fio_filename_overwrite(filename, d->buf, d->len);
  return fio_fd_save(d->fd, filename, d->len);
}

/* *****************************************************************************
Data Store - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_DATA */
#undef FIO_DATA
