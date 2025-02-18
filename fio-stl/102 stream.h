/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_STREAM             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




      A packet based data stream for storing / buffering endless data.



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_STREAM) && !defined(H___FIO_STREAM___H)
#define H___FIO_STREAM___H
#include <sys/stat.h>

#ifndef FIO_STREAM_COPY_PER_PACKET
/** Break apart large memory blocks into smaller pieces. by default 96Kb */
#define FIO_STREAM_COPY_PER_PACKET 98304
#endif

#ifndef FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN
/** If the data added is less than said bytes, copy is preferred (locality). */
#define FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN 116
#ifdef DEBUG
#undef FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN
#define FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN 8
#endif
#endif

/* *****************************************************************************
Stream API - types, constructor / destructor
***************************************************************************** */

typedef struct fio_stream_packet_s fio_stream_packet_s;

typedef struct {
  /* do not directly access! */
  fio_stream_packet_s *next;
  fio_stream_packet_s **pos;
  size_t consumed;
  size_t length;
} fio_stream_s;

/* at this point publish (declare only) the public API */

#ifndef FIO_STREAM_INIT
/* Initialization macro. */
#define FIO_STREAM_INIT(s)                                                     \
  { .next = NULL, .pos = &(s).next }
#endif

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY

/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC fio_stream_s *fio_stream_new(void);

/* Frees any internal data AND the object's container! */
FIO_IFUNC int fio_stream_free(fio_stream_s *stream);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** Destroys the object, re-initializing its container. */
SFUNC void fio_stream_destroy(fio_stream_s *stream);

/* *****************************************************************************
Stream API - packing data into packets and adding it to the stream
***************************************************************************** */

/** Packs data into a fio_stream_packet_s container. */
SFUNC fio_stream_packet_s *fio_stream_pack_data(void *buf,
                                                size_t len,
                                                size_t offset,
                                                uint8_t copy_buffer,
                                                void (*dealloc_func)(void *));

/** Packs a file descriptor into a fio_stream_packet_s container. */
SFUNC fio_stream_packet_s *fio_stream_pack_fd(int fd,
                                              size_t len,
                                              size_t offset,
                                              uint8_t keep_open);

/** Adds a packet to the stream. This isn't thread safe.*/
SFUNC void fio_stream_add(fio_stream_s *stream, fio_stream_packet_s *packet);

/** Destroys the fio_stream_packet_s - call this ONLY if unused. */
SFUNC void fio_stream_pack_free(fio_stream_packet_s *packet);

/* *****************************************************************************
Stream API - Consuming the stream
***************************************************************************** */

/**
 * Reads data from the stream (if any), leaving it in the stream.
 *
 * `buf` MUST point to a buffer with - at least - `len` bytes. This is required
 * in case the packed data is fragmented or references a file and needs to be
 * copied to an available buffer.
 *
 * On error, or if the stream is empty, `buf` will be set to NULL and `len` will
 * be set to zero.
 *
 * Otherwise, `buf` may retain the same value or it may point directly to a
 * memory address within the stream's buffer (the original value may be lost)
 * and `len` will be updated to the largest possible value for valid data that
 * can be read from `buf`.
 *
 * Note: this isn't thread safe.
 */
SFUNC void fio_stream_read(fio_stream_s *stream, char **buf, size_t *len);

/**
 * Advances the Stream, so the first `len` bytes are marked as consumed.
 *
 * Note: this isn't thread safe.
 */
SFUNC void fio_stream_advance(fio_stream_s *stream, size_t len);

/**
 * Returns true if there's any data in the stream.
 *
 * Note: this isn't truly thread safe.
 */
FIO_IFUNC uint8_t fio_stream_any(fio_stream_s *stream);

/**
 * Returns the number of bytes waiting in the stream.
 *
 * Note: this isn't truly thread safe.
 */
FIO_IFUNC size_t fio_stream_length(fio_stream_s *stream);

/* *****************************************************************************








                          Stream Implementation








***************************************************************************** */

/* *****************************************************************************
Stream Implementation - inlined static functions
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
FIO_LEAK_COUNTER_DEF(fio_stream)
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC fio_stream_s *fio_stream_new(void) {
  fio_stream_s *s = (fio_stream_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*s), 0);
  if (s) {
    *s = (fio_stream_s)FIO_STREAM_INIT(s[0]);
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio_stream);
  return s;
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC int fio_stream_free(fio_stream_s *s) {
  fio_stream_destroy(s);
  FIO_MEM_FREE_(s, sizeof(*s));
  FIO_LEAK_COUNTER_ON_FREE(fio_stream);
  return 0;
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Returns true if there's any data in the stream */
FIO_IFUNC uint8_t fio_stream_any(fio_stream_s *s) { return s && s->next; }

/* Returns the number of bytes waiting in the stream */
FIO_IFUNC size_t fio_stream_length(fio_stream_s *s) { return s->length; }

/* *****************************************************************************
Stream Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO_IFUNC void fio_stream_packet_free_all(fio_stream_packet_s *p);
/* Frees any internal data AND the object's container! */
SFUNC void fio_stream_destroy(fio_stream_s *s) {
  if (!s)
    return;
  fio_stream_packet_free_all(s->next);
  *s = (fio_stream_s)FIO_STREAM_INIT(s[0]);
  return;
}

FIO_LEAK_COUNTER_DEF(fio_stream_packet_s)

/* *****************************************************************************
Stream API - packing data into packets and adding it to the stream
***************************************************************************** */

struct fio_stream_packet_s {
  fio_stream_packet_s *next;
};

typedef enum {
  FIO_PACKET_TYPE_EMBEDDED = 0,
  FIO_PACKET_TYPE_EXTERNAL = 1,
  FIO_PACKET_TYPE_FILE = 2,
  FIO_PACKET_TYPE_FILE_NO_CLOSE = 3,
} fio_stream_packet_type_e;
#define FIO_STREAM___TYPE_BITS 2

typedef struct fio_stream_packet_embd_s {
  fio_stream_packet_type_e type;
  uint32_t length;
  char buf[];
} fio_stream_packet_embd_s;

typedef struct fio_stream_packet_extrn_s {
  fio_stream_packet_type_e type;
  size_t length;
  char *buf;
  uintptr_t offset;
  void (*dealloc)(void *buf);
} fio_stream_packet_extrn_s;

/** User-space socket buffer data */
typedef struct {
  fio_stream_packet_type_e type;
  size_t length;
  size_t offset;
  int fd;
} fio_stream_packet_fd_s;

FIO_SFUNC void fio_stream_packet_free(fio_stream_packet_s *p) {
  if (!p)
    return;
  FIO_LEAK_COUNTER_ON_FREE(fio_stream_packet_s);
  union {
    fio_stream_packet_embd_s *em;
    fio_stream_packet_extrn_s *ext;
    fio_stream_packet_fd_s *f;
  } const u = {.em = (fio_stream_packet_embd_s *)(p + 1)};
  switch (u.em->type) {
  case FIO_PACKET_TYPE_EMBEDDED:
    FIO_MEM_FREE_(p, sizeof(*p) + sizeof(*u.em) + u.em->length);
    break;
  case FIO_PACKET_TYPE_EXTERNAL:
    if (u.ext->dealloc)
      u.ext->dealloc(u.ext->buf);
    FIO_MEM_FREE_(p, sizeof(*p) + sizeof(*u.ext));
    break;
  case FIO_PACKET_TYPE_FILE: close(u.f->fd);
#ifdef DEBUG
    FIO_LOG_DEBUG2("fio_stream_packet_free closed file fd %d", u.f->fd);
#endif
    /* fall through */
  case FIO_PACKET_TYPE_FILE_NO_CLOSE:
    FIO_MEM_FREE_(p, sizeof(*p) + sizeof(*u.f));
    break;
  }
}

FIO_IFUNC void fio_stream_packet_free_all(fio_stream_packet_s *p) {
  while (p) {
    register fio_stream_packet_s *t = p;
    p = p->next;
    fio_stream_packet_free(t);
  }
}

FIO_IFUNC size_t fio___stream_p2len(fio_stream_packet_s *p) {
  size_t len = 0;
  if (!p)
    return len;
  union {
    fio_stream_packet_embd_s *em;
    fio_stream_packet_extrn_s *ext;
    fio_stream_packet_fd_s *f;
  } const u = {.em = (fio_stream_packet_embd_s *)(p + 1)};

  switch ((fio_stream_packet_type_e)(u.em->type &
                                     ((1UL << FIO_STREAM___TYPE_BITS) - 1))) {
  case FIO_PACKET_TYPE_EMBEDDED: return len = u.em->length; return len;
  case FIO_PACKET_TYPE_EXTERNAL: len = u.ext->length; return len;
  case FIO_PACKET_TYPE_FILE: /* fall through */
  case FIO_PACKET_TYPE_FILE_NO_CLOSE: len = u.f->length; return len;
  }
  return len;
}

/** Packs data into a fio_stream_packet_s container. */
SFUNC fio_stream_packet_s *fio_stream_pack_data(void *buf,
                                                size_t len,
                                                size_t offset,
                                                uint8_t copy_buffer,
                                                void (*dealloc_func)(void *)) {
  fio_stream_packet_s *p = NULL;
  if (!len || !buf || (len & ((~(0UL)) << (32 - FIO_STREAM___TYPE_BITS))))
    goto error;
  if (copy_buffer || len < FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN) {
    while (len) {
      /* break apart large memory blocks into smaller pieces */
      const size_t slice =
          (len > FIO_STREAM_COPY_PER_PACKET) ? FIO_STREAM_COPY_PER_PACKET : len;
      fio_stream_packet_embd_s *em;
      fio_stream_packet_s *tmp = (fio_stream_packet_s *)FIO_MEM_REALLOC_(
          NULL,
          0,
          sizeof(*p) + sizeof(*em) + (sizeof(char) * slice),
          0);
      if (!tmp)
        goto error;
      FIO_LEAK_COUNTER_ON_ALLOC(fio_stream_packet_s);
      tmp->next = p;
      em = (fio_stream_packet_embd_s *)(tmp + 1);
      em->type = FIO_PACKET_TYPE_EMBEDDED;
      em->length = (uint32_t)slice;
      FIO_MEMCPY(em->buf, (char *)buf + offset + (len - slice), slice);
      p = tmp;
      len -= slice;
    }
    if (dealloc_func)
      dealloc_func(buf);
  } else {
    fio_stream_packet_extrn_s *ext;
    p = (fio_stream_packet_s *)
        FIO_MEM_REALLOC_(NULL, 0, sizeof(*p) + sizeof(*ext), 0);
    if (!p)
      goto error;
    FIO_LEAK_COUNTER_ON_ALLOC(fio_stream_packet_s);
    p->next = NULL;
    ext = (fio_stream_packet_extrn_s *)(p + 1);
    *ext = (fio_stream_packet_extrn_s){
        .type = FIO_PACKET_TYPE_EXTERNAL,
        .length = (uint32_t)len,
        .buf = (char *)buf,
        .offset = offset,
        .dealloc = dealloc_func,
    };
  }
  return p;

error:
  if (dealloc_func)
    dealloc_func(buf);
  fio_stream_packet_free_all(p);
  return p;
}

/** Packs a file descriptor into a fio_stream_packet_s container. */
SFUNC fio_stream_packet_s *fio_stream_pack_fd(int fd,
                                              size_t len,
                                              size_t offset,
                                              uint8_t keep_open) {
  fio_stream_packet_s *p = NULL;
  fio_stream_packet_fd_s *f;
  if ((unsigned)(fd + 1) < 2)
    goto no_file;

  if (!len) {
    /* review file total length and auto-calculate */
    len = fio_fd_size(fd);
    if (!len || offset >= len || len >= 0x7FFFFFFF)
      goto error;
    len -= offset;
  }

  p = (fio_stream_packet_s *)
      FIO_MEM_REALLOC_(NULL, 0, sizeof(*p) + sizeof(*f), 0);
  if (!p)
    goto error;
  FIO_LEAK_COUNTER_ON_ALLOC(fio_stream_packet_s);
  p->next = NULL;
  f = (fio_stream_packet_fd_s *)(p + 1);
  *f = (fio_stream_packet_fd_s){
      .type =
          (keep_open ? FIO_PACKET_TYPE_FILE_NO_CLOSE : FIO_PACKET_TYPE_FILE),
      .length = len,
      .offset = offset,
      .fd = fd,
  };
#ifdef DEBUG
  FIO_LOG_DEBUG2("fio_stream_pack_fd wrapping file fd %d", fd);
#endif
  return p;
error:
  if (!keep_open)
    close(fd);
no_file:
  return p;
}

/** Adds a packet to the stream. This isn't thread safe.*/
SFUNC void fio_stream_add(fio_stream_s *s, fio_stream_packet_s *p) {
  fio_stream_packet_s *last = p;
  size_t len = 0;

  if (!s || !p)
    goto error;
  len = fio___stream_p2len(p);

  while (last->next) {
    last = last->next;
    len += fio___stream_p2len(last);
  }
  if (!s->pos)
    s->pos = &s->next;
  *s->pos = p;
  s->pos = &last->next;
  s->length += len;
  return;
error:
  fio_stream_pack_free(p);
}

/** Destroys the fio_stream_packet_s - call this ONLY if unused. */
SFUNC void fio_stream_pack_free(fio_stream_packet_s *p) {
  fio_stream_packet_free_all(p);
}

/* *****************************************************************************
Stream API - Consuming the stream
***************************************************************************** */

FIO_SFUNC void fio___stream_read_internal(fio_stream_packet_s *p,
                                          char **buf,
                                          size_t *len,
                                          size_t buf_offset,
                                          size_t offset,
                                          size_t must_copy) {
  if (!p || !len[0]) {
    len[0] = 0;
    return;
  }
  union {
    fio_stream_packet_embd_s *em;
    fio_stream_packet_extrn_s *ext;
    fio_stream_packet_fd_s *f;
  } const u = {.em = (fio_stream_packet_embd_s *)(p + 1)};
  size_t written = 0;

  switch (u.em->type) {
  case FIO_PACKET_TYPE_EMBEDDED:
    if (!buf[0] || !len[0] ||
        (!must_copy && (!p->next || u.em->length >= len[0] + offset))) {
      buf[0] = u.em->buf + offset;
      len[0] = (size_t)u.em->length - offset;
      return;
    }
    written = u.em->length - offset;
    if (written > len[0])
      written = len[0];
    if (written) {
      FIO_MEMCPY(buf[0] + buf_offset, u.em->buf + offset, written);
      len[0] -= written;
    }
    if (len[0]) {
      fio___stream_read_internal(p->next, buf, len, written + buf_offset, 0, 1);
    }
    len[0] += written;
    return;
  case FIO_PACKET_TYPE_EXTERNAL:
    if (!buf[0] || !len[0] ||
        (!must_copy && (!p->next || u.ext->length >= len[0] + offset))) {
      buf[0] = u.ext->buf + u.ext->offset + offset;
      len[0] = (size_t)(u.ext->length) - offset;
      return;
    }
    written = u.ext->length - offset;
    if (written > len[0])
      written = len[0];
    if (written) {
      FIO_MEMCPY(buf[0] + buf_offset,
                 u.ext->buf + u.ext->offset + offset,
                 written);
      len[0] -= written;
    }
    if (len[0]) {
      fio___stream_read_internal(p->next, buf, len, written + buf_offset, 0, 1);
    }
    len[0] += written;
    return;
    break;
  case FIO_PACKET_TYPE_FILE: /* fall through */
  case FIO_PACKET_TYPE_FILE_NO_CLOSE:
    if (!buf[0] || !len[0]) {
      len[0] = 0;
      return;
    }
    {
      uint8_t possible_eof_surprise = 0;
      written = u.f->length - offset; /* written here == to be read & written */
      if (written > len[0])
        written = len[0];
      if (written) {
        ssize_t act;
      retry_on_signal:
        act = fio_fd_read(u.f->fd,
                          buf[0] + buf_offset,
                          written,
                          u.f->offset + offset);
        if (act <= 0) {
          /* no more data in the file? */
          FIO_LOG_DEBUG("file read error for %d: %s", u.f->fd, strerror(errno));
          if (errno == EINTR)
            goto retry_on_signal;
          // u.f->length = offset; /* mark EOF */
        } else if ((size_t)act != written) {
          /* a surprising EOF? */
          written = act;
          possible_eof_surprise = 1;
          // u.f->length = offset + act; /* mark EOF? */
        }
        len[0] -= written;
      }
      if (!possible_eof_surprise && len[0]) {
        fio___stream_read_internal(p->next,
                                   buf,
                                   len,
                                   written + buf_offset,
                                   0,
                                   1);
      }
      len[0] += written;
    }
    return;
  }
}

/**
 * Reads data from the stream (if any), leaving it in the stream.
 *
 * `buf` MUST point to a buffer with - at least - `len` bytes. This is required
 * in case the packed data is fragmented or references a file and needs to be
 * copied to an available buffer.
 *
 * On error, or if the stream is empty, `buf` will be set to NULL and `len` will
 * be set to zero.
 *
 * Otherwise, `buf` may retain the same value or it may point directly to a
 * memory address wiithin the stream's buffer (the original value may be lost)
 * and `len` will be updated to the largest possible value for valid data that
 * can be read from `buf`.
 *
 * Note: this isn't thread safe.
 */
SFUNC void fio_stream_read(fio_stream_s *s, char **buf, size_t *len) {
  if (!s || !s->next)
    goto none;
  fio___stream_read_internal(s->next, buf, len, 0, s->consumed, 0);
  return;
none:
  *buf = NULL;
  *len = 0;
}

/**
 * Advances the Stream, so the first `len` bytes are marked as consumed.
 *
 * Note: this isn't thread safe.
 */
SFUNC void fio_stream_advance(fio_stream_s *s, size_t len) {
  if (!s || !s->next)
    return;
  s->length -= len;
  len += s->consumed;
  while (len) {
    size_t p_len = fio___stream_p2len(s->next);
    if (len >= p_len) {
      fio_stream_packet_s *p = s->next;
      s->next = p->next;
      fio_stream_packet_free(p);
      len -= p_len;
      if (!s->next) {
        s->pos = &s->next;
        s->consumed = 0;
        s->length = 0;
        return;
      }
    } else {
      s->consumed = len;
      return;
    }
  }
  s->consumed = len;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_STREAM___TYPE_BITS
#endif /* FIO_STREAM */
#undef FIO_STREAM
