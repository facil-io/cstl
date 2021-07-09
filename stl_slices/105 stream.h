/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_STREAM                  /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




      A packet based data stream for storing / buffering endless data.




***************************************************************************** */
#if defined(FIO_STREAM) && !defined(H___FIO_STREAM___H)
#define H___FIO_STREAM___H

#if !FIO_HAVE_UNIX_TOOLS
#if _MSC_VER
#pragma message("POSIX is required for the fio_stream API, or issues may occure.")
#else
#warning "POSIX behavior is expected by the fio_stream API."
#endif
#endif
#include <sys/stat.h>

#ifndef FIO_STREAM_COPY_PER_PACKET
/** Break apart large memory blocks into smaller pieces. by default 96Kb */
#define FIO_STREAM_COPY_PER_PACKET 98304
#endif

/* *****************************************************************************
Stream API - types, constructor / destructor
***************************************************************************** */

typedef struct fio_stream_packet_s fio_stream_packet_s;

typedef struct {
  /* do not directly acecss! */
  fio_stream_packet_s *next;
  fio_stream_packet_s **pos;
  uint32_t consumed;
  uint32_t packets;
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

/** Destroys the object, reinitializing its container. */
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
 * memory address wiithin the stream's buffer (the original value may be lost)
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
 * Note: this isn't truely thread safe.
 */
FIO_IFUNC uint8_t fio_stream_any(fio_stream_s *stream);

/**
 * Returns the number of packets waiting in the stream.
 *
 * Note: this isn't truely thread safe.
 */
FIO_IFUNC uint32_t fio_stream_packets(fio_stream_s *stream);

/* *****************************************************************************








                          Stream Implementation








***************************************************************************** */

/* *****************************************************************************
Stream Implementation - inlined static functions
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC fio_stream_s *fio_stream_new(void) {
  fio_stream_s *s = (fio_stream_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*s), 0);
  if (s) {
    *s = (fio_stream_s)FIO_STREAM_INIT(s[0]);
  }
  return s;
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC int fio_stream_free(fio_stream_s *s) {
  fio_stream_destroy(s);
  FIO_MEM_FREE_(s, sizeof(*s));
  return 0;
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/**
 * Returns true if there's any data iin the stream.
 *
 * Note: this isn't thread safe.
 */
FIO_IFUNC uint8_t fio_stream_any(fio_stream_s *s) { return s && !!s->next; }

/**
 * Returns the number of packets waiting in the stream.
 *
 * Note: this isn't truely thread safe.
 */
FIO_IFUNC uint32_t fio_stream_packets(fio_stream_s *s) { return s->packets; }

/* *****************************************************************************
Stream Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

FIO_IFUNC void fio_stream_packet_free_all(fio_stream_packet_s *p);
/* Frees any internal data AND the object's container! */
SFUNC void fio_stream_destroy(fio_stream_s *s) {
  if (!s)
    return;
  fio_stream_packet_free_all(s->next);
  *s = (fio_stream_s)FIO_STREAM_INIT(s[0]);
  return;
}

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

typedef struct fio_stream_packet_embd_s {
  uint32_t type;
  char buf[];
} fio_stream_packet_embd_s;
#define FIO_STREAM___EMBD_BIT_OFFSET 4

typedef struct fio_stream_packet_extrn_s {
  uint32_t type;
  uint32_t length;
  char *buf;
  uintptr_t offset;
  void (*dealloc)(void *buf);
} fio_stream_packet_extrn_s;

/** User-space socket buffer data */
typedef struct {
  uint32_t type;
  uint32_t length;
  int32_t offset;
  int fd;
} fio_stream_packet_fd_s;

FIO_SFUNC void fio_stream_packet_free(fio_stream_packet_s *p) {
  if (!p)
    return;
  union {
    fio_stream_packet_embd_s *em;
    fio_stream_packet_extrn_s *ext;
    fio_stream_packet_fd_s *f;
  } const u = {.em = (fio_stream_packet_embd_s *)(p + 1)};
  switch ((fio_stream_packet_type_e)(u.em->type & 3)) {
  case FIO_PACKET_TYPE_EMBEDDED:
    FIO_MEM_FREE_(
        p,
        sizeof(*p) + sizeof(*u.em) +
            (sizeof(char) * (u.em->type >> FIO_STREAM___EMBD_BIT_OFFSET)));
    break;
  case FIO_PACKET_TYPE_EXTERNAL:
    if (u.ext->dealloc)
      u.ext->dealloc(u.ext->buf);
    FIO_MEM_FREE_(p, sizeof(*p) + sizeof(*u.ext));
    break;
  case FIO_PACKET_TYPE_FILE:
    close(u.f->fd);
    /* fallthrough */
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

/** Packs data into a fio_stream_packet_s container. */
SFUNC fio_stream_packet_s *fio_stream_pack_data(void *buf,
                                                size_t len,
                                                size_t offset,
                                                uint8_t copy_buffer,
                                                void (*dealloc_func)(void *)) {
  fio_stream_packet_s *p = NULL;
  if (!len || !buf || (len & ((~(0UL)) << (32 - FIO_STREAM___EMBD_BIT_OFFSET))))
    goto error;
  if (copy_buffer || len <= 14) {
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
      tmp->next = p;
      em = (fio_stream_packet_embd_s *)(tmp + 1);
      em->type = (uint32_t)FIO_PACKET_TYPE_EMBEDDED |
                 (uint32_t)(slice << FIO_STREAM___EMBD_BIT_OFFSET);
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
  if (fd < 0)
    goto no_file;

  if (!len) {
    /* review file total length and auto-calculate */
    struct stat st;
    if (fstat(fd, &st))
      goto error;
    if (st.st_size <= 0 || offset >= (size_t)st.st_size ||
        (uint64_t)st.st_size >= ((uint64_t)1UL << 32))
      goto error;
    len = (size_t)st.st_size - offset;
  }

  p = (fio_stream_packet_s *)
      FIO_MEM_REALLOC_(NULL, 0, sizeof(*p) + sizeof(*f), 0);
  if (!p)
    goto error;
  p->next = NULL;
  f = (fio_stream_packet_fd_s *)(p + 1);
  *f = (fio_stream_packet_fd_s){
      .type =
          (keep_open ? FIO_PACKET_TYPE_FILE : FIO_PACKET_TYPE_FILE_NO_CLOSE),
      .length = (uint32_t)len,
      .offset = (int32_t)offset,
      .fd = fd,
  };
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
  uint32_t packets = 1;
  if (!s || !p)
    goto error;
  while (last->next) {
    last = last->next;
    ++packets;
  }
  if (!s->pos)
    s->pos = &s->next;
  *s->pos = p;
  s->pos = &last->next;
  s->packets += packets;
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

FIO_IFUNC size_t fio___stream_p2len(fio_stream_packet_s *p) {
  size_t len = 0;
  if (!p)
    return len;
  union {
    fio_stream_packet_embd_s *em;
    fio_stream_packet_extrn_s *ext;
    fio_stream_packet_fd_s *f;
  } const u = {.em = (fio_stream_packet_embd_s *)(p + 1)};

  switch ((fio_stream_packet_type_e)(u.em->type & 3)) {
  case FIO_PACKET_TYPE_EMBEDDED:
    len = u.em->type >> FIO_STREAM___EMBD_BIT_OFFSET;
    return len;
  case FIO_PACKET_TYPE_EXTERNAL:
    len = u.ext->length;
    return len;
  case FIO_PACKET_TYPE_FILE: /* fallthrough */
  case FIO_PACKET_TYPE_FILE_NO_CLOSE:
    len = u.f->length;
    return len;
  }
  return len;
}

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

  switch ((fio_stream_packet_type_e)(u.em->type & 3)) {
  case FIO_PACKET_TYPE_EMBEDDED:
    if (!buf[0] || !len[0] ||
        (!must_copy &&
         (!p->next ||
          (u.em->type >> FIO_STREAM___EMBD_BIT_OFFSET) >= len[0] + offset))) {
      buf[0] = u.em->buf + offset;
      len[0] = (size_t)(u.em->type >> FIO_STREAM___EMBD_BIT_OFFSET) - offset;
      return;
    }
    written = (u.em->type >> FIO_STREAM___EMBD_BIT_OFFSET) - offset;
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
  case FIO_PACKET_TYPE_FILE: /* fallthrough */
  case FIO_PACKET_TYPE_FILE_NO_CLOSE:
    if (!buf[0] || !len[0]) {
      buf[0] = NULL;
      len[0] = 0;
      return;
    }
    {
      uint8_t possible_eol_surprise = 0;
      written = u.f->length - offset;
      if (written > len[0])
        written = len[0];
      if (written) {
        ssize_t act;
      retry_on_signal:
        act = fio_pread(u.f->fd,
                        buf[0] + buf_offset,
                        written,
                        u.f->offset + offset);
        if (act <= 0) {
          /* no more data in the file? */
          FIO_LOG_DEBUG("file read error for %d: %s", u.f->fd, strerror(errno));
          if (errno == EINTR)
            goto retry_on_signal;
          u.f->length = offset;
        } else if ((size_t)act != written) {
          /* a surprising EOF? */
          written = act;
          possible_eol_surprise = 1;
        }
        len[0] -= written;
      }
      if (!possible_eol_surprise && len[0]) {
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
  len += s->consumed;
  while (len) {
    size_t p_len = fio___stream_p2len(s->next);
    if (len >= p_len) {
      fio_stream_packet_s *p = s->next;
      s->next = p->next;
      --s->packets;
      fio_stream_packet_free(p);
      len -= p_len;
      if (!s->next) {
        s->pos = &s->next;
        s->consumed = 0;
        s->packets = 0;
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
Stream Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, stream)(void) {
  char *const str =
      (char *)"My Hello World string should be long enough so it can be used "
              "for testing the stream functionality in the facil.io stream "
              "module. The stream moduule takes strings and failes and places "
              "them (by reference / copy) into a linked list of objects. When "
              "data is requested from the stream, the stream will either copy "
              "the data to a pre-allocated buffer or it may update the link to "
              "it points to its own internal buffer (avoiding a copy when "
              "possible).";
  fio_stream_s s = FIO_STREAM_INIT(s);
  char mem[4000];
  char *buf = mem;
  size_t len = 4000;
  fprintf(stderr, "* Testing fio_stream for streaming buffer storage.\n");
  fio_stream_add(&s, fio_stream_pack_data(str, 11, 3, 1, NULL));
  FIO_ASSERT(fio_stream_any(&s),
             "stream is empty after `fio_stream_add` (data, copy)");
  for (int i = 0; i < 3; ++i) {
    /* test that read operrations are immutable */
    buf = mem;
    len = 4000;
    fio_stream_read(&s, &buf, &len);
    FIO_ASSERT(len == 11,
               "fio_stream_read didn't read all data from stream? (%zu)",
               len);
    FIO_ASSERT(!memcmp(str + 3, buf, len),
               "fio_stream_read data error? (%.*s)",
               (int)len,
               buf);
    FIO_ASSERT_DEBUG(
        buf != mem,
        "fio_stream_read should have been performed with zero-copy");
  }
  fio_stream_advance(&s, len);
  FIO_ASSERT(
      !fio_stream_any(&s),
      "after advance, at this point, the stream should have been consumed.");
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(
      !buf && !len,
      "reading from an empty stream should set buf and len to NULL and zero.");
  fio_stream_destroy(&s);
  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty.");

  fio_stream_add(&s, fio_stream_pack_data(str, 11, 0, 1, NULL));
  fio_stream_add(&s, fio_stream_pack_data(str, 49, 11, 0, NULL));
  fio_stream_add(&s, fio_stream_pack_data(str, 20, 60, 0, NULL));

  FIO_ASSERT(fio_stream_any(&s), "stream with data shouldn't be empty.");
  FIO_ASSERT(fio_stream_packets(&s) == 3, "packet counut error.");

  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);

  FIO_ASSERT(len == 80,
             "fio_stream_read didn't read all data from stream(2)? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str, buf, len),
             "fio_stream_read data error? (%.*s)",
             (int)len,
             buf);

  buf = mem;
  len = 8;
  fio_stream_read(&s, &buf, &len);

  FIO_ASSERT(len < 80,
             "fio_stream_read didn't perform a partial read? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str, buf, len),
             "fio_stream_read partial read data error? (%.*s)",
             (int)len,
             buf);

  fio_stream_advance(&s, 20);
  FIO_ASSERT(fio_stream_packets(&s) == 2, "packet counut error (2).");
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 60,
             "fio_stream_read didn't read all data from stream(3)? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str + 20, buf, len),
             "fio_stream_read data error? (%.*s)",
             (int)len,
             buf);

  fio_stream_add(&s, fio_stream_pack_fd(open(__FILE__, O_RDONLY), 20, 0, 0));
  FIO_ASSERT(fio_stream_packets(&s) == 3, "packet counut error (3).");
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 80,
             "fio_stream_read didn't read all data from stream(4)? (%zu)",
             len);
  FIO_ASSERT(!memcmp("/* *****************", buf + 60, 20),
             "fio_stream_read file read data error?\n%.*s",
             (int)len,
             buf);
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 80,
             "fio_stream_read didn't (re)read all data from stream(5)? (%zu)",
             len);
  FIO_ASSERT(!memcmp("/* *****************", buf + 60, 20),
             "fio_stream_read file (re)read data error? (%.*s)",
             (int)len,
             buf);

  fio_stream_destroy(&s);
  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty.");
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_STREAM___EMBD_BIT_OFFSET
#endif /* FIO_STREAM */
#undef FIO_STREAM
