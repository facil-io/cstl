/*
Copyright: Boaz Segev, 2016-2022
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
*/
#include "http-handle.h"

#define FIO_TIME
#define FIO_SOCK
#define FIO_FILES
#include "fio-stl.h"

/* *****************************************************************************
Helper types
***************************************************************************** */
#if DEBUG
/* use a dedicated allocator when debugging, in order to detect leaks. */
#define FIO_MEMORY_NAME http_mem
#include "fio-stl.h"
#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#define FIO_MEM_REALLOC(p, ol, nl, cl) http_mem_realloc2((p), (nl), (cl))
#define FIO_MEM_FREE(ptr, size)        http_mem_free((ptr))
#define FIO_MEM_REALLOC_IS_SAFE        http_mem_realloc_is_safe()
#endif /* DEBUG */

#define FIO_STR_SMALL sstr
#include "fio-stl.h"
#define FIO_STR_NAME lstr
#include "fio-stl.h"
#define FIO_ARRAY_NAME                sary
#define FIO_ARRAY_TYPE                sstr_s
#define FIO_ARRAY_TYPE_CMP(a, b)      sstr_is_eq(&(a), &(b))
#define FIO_ARRAY_TYPE_COPY(a, b)     sstr_init_copy2(&(a), &(b))
#define FIO_ARRAY_TYPE_DESTROY(s)     sstr_destroy(&(s))
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#include "fio-stl.h"

FIO_IFUNC sary_s sary_tmp(sstr_s v) {
  sary_s a = FIO_ARRAY_INIT;
  sstr_s v_tmp = FIO_STR_INIT;
  sary_push(&a, v_tmp); /* avoid string data copy */
  sary2ptr(&a)[0] = v;
  return a;
}

#define FIO_MAP_NAME            smap
#define FIO_MAP_TYPE            lstr_s
#define FIO_MAP_TYPE_COPY(a, b) lstr_init_copy2(&(a), &(b))
#define FIO_MAP_TYPE_DESTROY(o) lstr_destroy(&(o))
#define FIO_MAP_KEY             sstr_s
#define FIO_MAP_KEY_CMP(a, b)   sstr_is_eq(&(a), &(b))
#define FIO_MAP_KEY_COPY(a, b)  sstr_init_copy2(&(a), &(b))
#define FIO_MAP_KEY_DESTROY(s)  sstr_destroy(&(s))
#include "fio-stl.h"

FIO_IFUNC lstr_s *smap_set2(smap_s *map,
                            fio_str_info_s key,
                            fio_str_info_s val,
                            uint8_t overwrite) {
  sstr_s k;
  lstr_s v;
  sstr_init_const(&k, key.buf, key.len);
  lstr_init_const(&v, val.buf, val.len);
  const uint64_t h = fio_risky_hash(key.buf, key.len, (uint64_t)(uintptr_t)map);
  return smap_set_ptr(map, h, k, v, NULL, overwrite);
}

FIO_IFUNC fio_str_info_s smap_get2(smap_s *map, fio_str_info_s key) {
  sstr_s k;
  sstr_init_const(&k, key.buf, key.len);
  const uint64_t h = fio_risky_hash(key.buf, key.len, (uint64_t)(uintptr_t)map);
  return lstr_info(smap_get_ptr(map, h, k));
}

#define FIO_MAP_NAME hmap
#define FIO_MAP_TYPE sary_s
#define FIO_MAP_TYPE_COPY(a, b)                                                \
  do {                                                                         \
    (a) = (sary_s)FIO_ARRAY_INIT;                                              \
    sary_push(&(a), sary_get((&b), 0));                                        \
  } while (0)
#define FIO_MAP_TYPE_DESTROY(o) sary_destroy(&(o))
#define FIO_MAP_KEY             sstr_s
#define FIO_MAP_KEY_CMP(a, b)   sstr_is_eq(&(a), &(b))
#define FIO_MAP_KEY_COPY(a, b)  sstr_init_copy2(&(a), &(b))
#define FIO_MAP_KEY_DESTROY(o)  sstr_destroy(&(o))
#include "fio-stl.h"

FIO_IFUNC sstr_s *hmap_set2(hmap_s *map,
                            fio_str_info_s key,
                            fio_str_info_s val,
                            uint8_t add) {
  sstr_s *r = NULL;
  sary_s *o = NULL;
  sstr_s k = {0};
  sstr_s v = {0};
  sary_s va = {0};
  if (!key.buf || !key.len || !map)
    return r;
  sstr_init_const(&k, key.buf, key.len);
  sstr_init_const(&v, val.buf, val.len);
  const uint64_t h = fio_risky_hash(key.buf, key.len, (uint64_t)(uintptr_t)map);
  if (!val.buf || !val.len)
    goto remove_key;
  va = sary_tmp(v);
  o = hmap_get_ptr(map, h, k);
  if (!o) {
    o = hmap_set_ptr(map, h, k, va, NULL, 1);
    add = 0;
  }
  sary_destroy(&va);
  if (FIO_UNLIKELY(!o)) {
    FIO_LOG_ERROR("Couldn't add value to header: %.*s:%.*s",
                  (int)key.len,
                  key.buf,
                  (int)val.len,
                  val.buf);
    return r;
  }
  if (add) {
    sary_push(o, v);
  }
  r = sary2ptr(o) + (sary_count(o) - 1);
  return r;

remove_key:
  hmap_remove(map, h, k, NULL);
  return r;
}

FIO_IFUNC fio_str_info_s hmap_get2(hmap_s *map, fio_str_info_s key, int index) {
  fio_str_info_s r = {0};
  sstr_s k;
  sstr_init_const(&k, key.buf, key.len);
  const uint64_t h = fio_risky_hash(key.buf, key.len, (uint64_t)(uintptr_t)map);
  sary_s *a = hmap_get_ptr(map, h, k);
  if (!a)
    return r;
  const uint32_t count = sary_count(a);
  if (index < 0)
    index += count;
  if ((uint32_t)index >= count)
    return r;
  r = sstr_info(sary2ptr(a) + index);
  return r;
}

/* *****************************************************************************
The HTTP handle type
***************************************************************************** */
struct http_s {
  void *udata;
  http_controller_s *controller;
  void *cdata;
  int64_t received_at;
  size_t status;
  size_t state;
  sstr_s method;
  sstr_s path;
  sstr_s query;
  sstr_s version;
  hmap_s headers[2];
  smap_s cookies[2];
  struct {
    char *buf;
    size_t len;
    size_t pos;
    size_t capa;
    int fd;
  } body;
};

#define HTTP_STATE_FINISHED       1
#define HTTP_STATE_STREAMING      2
#define HTTP_STATE_COOKIES_PARSED 4

#define HTTP_HDR_REQUEST(h)  (h->headers + 0)
#define HTTP_HDR_RESPONSE(h) (h->headers + 1)

#ifndef HTTP_BODY_RAM_LIMIT
#define HTTP_BODY_RAM_LIMIT (1 << 16)
#endif

#if FIO_HTTP_EXACT_LOGGING
FIO_IFUNC int64_t http_get_timestump(void) { return fio_time_milli(); }
#else
int64_t fio_last_tick(void);
FIO_IFUNC uint64_t http_get_timestump(void) { return (int64_t)fio_last_tick(); }
#endif

void http_destroy(http_s *h) {
  if (!h)
    return;
  if (h->controller)
    h->controller->on_unlinked(h, h->cdata);
  sstr_destroy(&h->method);
  sstr_destroy(&h->path);
  sstr_destroy(&h->query);
  sstr_destroy(&h->version);
  hmap_destroy(h->headers);
  hmap_destroy(h->headers + 1);
  smap_destroy(h->cookies);
  smap_destroy(h->cookies + 1);
  FIO_MEM_FREE(h->body.buf, h->body.capa);
  if (h->body.fd != -1)
    close(h->body.fd);
  *h = (http_s){.received_at = http_get_timestump(), .body.fd = -1};
}

#define FIO_REF_NAME http
#define FIO_REF_INIT(h)                                                        \
  h = (http_s) { .received_at = http_get_timestump(), .body.fd = -1 }
#define FIO_REF_DESTROY(h) http_destroy(&(h))
#include "fio-stl.h"

/** Create a new http_s handle. */
http_s *http_new(void) { return http_new2(); }

/** Reduces an http_s handle's reference count or frees it. */
void http_free(http_s *h) { http_free2(h); }

/** Increases an http_s handle's reference count. */
http_s *http_dup(http_s *h) { return http_dup2(h); }

/* *****************************************************************************
Linking to a controller
***************************************************************************** */

FIO_SFUNC void mock_c_on_unlinked(http_s *h, void *cdata) {
  (void)h;
  (void)cdata;
}
FIO_SFUNC int mock_c_start_response(http_s *h, int status, int streaming) {
  return -1;
  (void)h;
  (void)status;
  (void)streaming;
}
FIO_SFUNC void mock_c_send_headers(http_s *h) { (void)h; }
FIO_SFUNC void mock_c_write_body(http_s *h, http_write_args_s args) {
  if (args.data) {
    if (args.dealloc)
      args.dealloc((void *)args.data);
  } else if (args.fd != -1) {
    close(args.fd);
  }
  (void)h;
}

FIO_SFUNC void http_controller_validate(http_controller_s *c) {
  if (!c->on_unlinked)
    c->on_unlinked = mock_c_on_unlinked;
  if (!c->start_response)
    c->start_response = mock_c_start_response;
  if (!c->start_request)
    c->start_request = mock_c_start_response;
  if (!c->send_headers)
    c->send_headers = mock_c_send_headers;
  if (!c->write_body)
    c->write_body = mock_c_write_body;
}

/** Gets the HTTP Controller associated with the HTTP handle. */
http_controller_s *http_controller_get(http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return h->controller;
}

/** Returns the `void *` pointer returned by the HTTP Controller `on_link`. */
void *http_controller_data(http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return h->cdata;
}

/** Sets the HTTP Controller, calling the `on_link` callback as required. */
void http_controller_set(http_s *h, http_controller_s *c, void *cdata) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  if (h->controller)
    h->controller->on_unlinked(h, h->cdata);
  h->controller = c;
  h->cdata = cdata;
  if (!c)
    return;
  if (!fio_atomic_or(&c->private_flags, 1))
    http_controller_validate(c);
  return;
}

/* *****************************************************************************
Short String Property Set / Get
***************************************************************************** */

#define HTTP___MAKE_GET_SET(property)                                          \
  fio_str_info_s http_##property##_get(http_s *h) {                            \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    return sstr_info(&h->property);                                            \
  }                                                                            \
                                                                               \
  fio_str_info_s http_##property##_set(http_s *h, fio_str_info_s value) {      \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    sstr_destroy(&h->property);                                                \
    return sstr_init_copy(&h->property, value.buf, value.len);                 \
  }

HTTP___MAKE_GET_SET(method)
HTTP___MAKE_GET_SET(path)
HTTP___MAKE_GET_SET(query)
HTTP___MAKE_GET_SET(version)
#undef HTTP___MAKE_GET_SET

/* *****************************************************************************
Header iteration Task
***************************************************************************** */

typedef struct {
  http_s *h;
  int (*callback)(http_s *, fio_str_info_s, fio_str_info_s, void *);
  void *udata;
} http___h_each_data_s;

FIO_SFUNC int http___h_each_task_wrapper(hmap_each_s *e) {
  http___h_each_data_s *data = e->udata;
  fio_str_info_s k = sstr_info(&e->key);
  FIO_ARRAY_EACH(sary, &e->value, pos) {
    if (data->callback(data->h, k, sstr_info(pos), data->udata) == -1)
      return -1;
  }
  return 0;
}

/* *****************************************************************************
Header data management
***************************************************************************** */

/**
 * Gets the header information associated with the HTTP handle.
 *
 * Since more than a single value may be associated with a header name, the
 * index may be used to collect subsequent values.
 *
 * An empty value is returned if no header value is available (or index is
 * exceeded).
 */
fio_str_info_s http_request_header_get(http_s *h,
                                       fio_str_info_s name,
                                       size_t index) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return hmap_get2(HTTP_HDR_REQUEST(h), name, index);
}

/** Sets the header information associated with the HTTP handle. */
fio_str_info_s http_request_header_set(http_s *h,
                                       fio_str_info_s name,
                                       fio_str_info_s value) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return sstr_info(hmap_set2(HTTP_HDR_REQUEST(h), name, value, 0));
}

/** Adds to the header information associated with the HTTP handle. */
fio_str_info_s http_request_header_add(http_s *h,
                                       fio_str_info_s name,
                                       fio_str_info_s value) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return sstr_info(hmap_set2(HTTP_HDR_REQUEST(h), name, value, 1));
}

/** Iterates through all headers. A non-zero return will stop iteration. */
size_t http_request_header_each(http_s *h,
                                int (*callback)(http_s *,
                                                fio_str_info_s name,
                                                fio_str_info_s value,
                                                void *udata),
                                void *udata) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!callback)
    return hmap_count(HTTP_HDR_REQUEST(h));
  http___h_each_data_s d = {.h = h, .callback = callback, .udata = udata};
  return hmap_each(HTTP_HDR_REQUEST(h), http___h_each_task_wrapper, &d, 0);
}

/**
 * Gets the header information associated with the HTTP handle.
 *
 * Since more than a single value may be associated with a header name, the
 * index may be used to collect subsequent values.
 *
 * An empty value is returned if no header value is available (or index is
 * exceeded).
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
fio_str_info_s http_response_header_get(http_s *h,
                                        fio_str_info_s name,
                                        size_t index) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return hmap_get2(HTTP_HDR_RESPONSE(h), name, index);
}

/**
 * Sets the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
fio_str_info_s http_response_header_set(http_s *h,
                                        fio_str_info_s name,
                                        fio_str_info_s value) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return sstr_info(hmap_set2(HTTP_HDR_RESPONSE(h), name, value, 0));
}

/**
 * Adds to the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
fio_str_info_s http_response_header_add(http_s *h,
                                        fio_str_info_s name,
                                        fio_str_info_s value) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return sstr_info(hmap_set2(HTTP_HDR_RESPONSE(h), name, value, 1));
}

/** Iterates through all headers. A non-zero return will stop iteration. */
size_t http_response_header_each(
    http_s *h,
    int (*callback)(http_s *, fio_str_info_s, fio_str_info_s, void *),
    void *udata) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!callback)
    return hmap_count(HTTP_HDR_RESPONSE(h));
  http___h_each_data_s d = {.h = h, .callback = callback, .udata = udata};
  return hmap_each(HTTP_HDR_RESPONSE(h), http___h_each_task_wrapper, &d, 0);
}

/* *****************************************************************************
Body / Payload handling
***************************************************************************** */

/** Gets the body (payload) length associated with the HTTP handle. */
size_t http_body_length(http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return h->body.len;
}

/** Adjusts the body's reading position. Negative values start at the end. */
size_t http_body_seek(http_s *h, ssize_t pos) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  if (pos < 0)
    pos += h->body.len;
  if (pos < 0)
    pos = 0;
  h->body.pos = pos;
  if (h->body.fd != -1)
    lseek(h->body.fd, (off_t)pos, SEEK_SET);
  return pos;
}

/** Reads up to `length` of data from the body, returns nothing on EOF. */
fio_str_info_s http_body_read(http_s *h, size_t length) {
  fio_str_info_s r = {0};
  if (!h || h->body.pos >= h->body.len)
    return r;
  const size_t limit = h->body.len - h->body.pos;
  if (length > limit)
    length = limit;
  if (!length)
    return r;
  if (h->body.fd != -1)
    goto body_is_a_file;
  r.buf = h->body.buf + h->body.pos;
  r.len = length;
  h->body.pos += length;
  return r;

body_is_a_file:
  if (h->body.capa <= length) {
    size_t __attribute__((unused)) old_capa = h->body.capa;
    h->body.capa = (length + (!(length & 15)) + 15) & (~(size_t)15);
    void *tmp = FIO_MEM_REALLOC(h->body.buf, old_capa, h->body.capa, 0);
    FIO_ASSERT_ALLOC(tmp);
    h->body.buf = tmp;
    (void)old_capa;
  }
  lseek(h->body.fd, (off_t)h->body.pos, SEEK_SET);
  ssize_t actual = read(h->body.fd, h->body.buf, length);
  if (actual <= 0)
    return r;
  h->body.pos += actual;
  h->body.buf[actual] = 0;
  r.buf = h->body.buf;
  r.len = actual;
  return r;
}

/** Reads from the body until finding `token` or the end of the body. */
fio_str_info_s http_body_read_until(http_s *h, char token, size_t limit) {
  fio_str_info_s r = {0};
  if (!h || h->body.pos >= h->body.len)
    return r;
  const size_t limit_map[2] = {limit, h->body.len - h->body.pos};
  limit = limit_map[(limit_map[1] < limit_map[0]) | (!limit_map[0])];
  if (h->body.fd != -1)
    goto body_is_a_file;
  /* for mem buffer */
  char *found = memchr(h->body.buf + h->body.pos, token, limit);
  if (found) {
    r.buf = h->body.buf;
    r.len = (found - (h->body.buf + h->body.pos));
    h->body.pos = found - h->body.buf;
    ++h->body.pos;
    return r;
  }
  r.buf = h->body.buf + h->body.pos;
  r.len = limit;
  h->body.pos = h->body.len;
  return r;

body_is_a_file:
  /* TODO: for file */

  return r;
}

/** Allocates a body (payload) of (at least) the `expected_length`. */
void http_body_expect(http_s *h, size_t expect) {
  if ((!h | !expect) || h->body.buf || h->body.fd != -1)
    return;
  h->body.capa = (expect + (!(expect & 15)) + 15) & (~(size_t)15);
  if (expect <= HTTP_BODY_RAM_LIMIT) {
    h->body.buf = FIO_MEM_REALLOC(NULL, 0, h->body.capa, 0);
    FIO_ASSERT_ALLOC(h->body.buf);
    return;
  }
  h->body.fd = fio_filename_tmp();
  FIO_ASSERT(h->body.fd != -1, "couldn't open tmpfile for HTTP body payload");
}

/** Writes `data` to the body (payload) associated with the HTTP handle. */
void http_body_write(http_s *h, const void *data, size_t len) {
  if (!h || !len)
    return;
  if (h->body.fd != -1)
    goto is_file;
  {
    const size_t required =
        ((h->body.len + len) + (!((h->body.len + len) & 15)) + 15) &
        (~(size_t)15);
    if (required > HTTP_BODY_RAM_LIMIT)
      goto move2file;
    if (h->body.capa <= required) {
      void *tmp =
          FIO_MEM_REALLOC(h->body.buf, h->body.capa, required, h->body.len);
      FIO_ASSERT_ALLOC(tmp);
      h->body.buf = tmp;
      h->body.capa = required;
    }
  }
  memcpy(h->body.buf + h->body.len, data, len);
  h->body.len += len;
  return;

is_file:
  do {
    /* write chunks up to (1<<19)-1 long as `write` might break on big data */
    ssize_t written = write(h->body.fd,
                            data,
                            (len & ((1UL << 18) - 1)) +
                                (((1UL << 18) * (len >= ((1UL << 18))))));
    if (written < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        continue;
      FIO_LOG_ERROR("http_body_write encountered unexpected error %d: %s",
                    errno,
                    strerror(errno));
      return;
    }
    FIO_ASSERT(written > 0, "tmpfile IO error");
    h->body.len += written;
    len -= written;
  } while (len);
  return;

move2file:
  h->body.fd = fio_filename_tmp();
  FIO_ASSERT(h->body.fd != -1, "couldn't open tmpfile for HTTP body payload");
  h->body.capa = h->body.len;
  h->body.len = 0;
  http_body_write(h, h->body.buf, h->body.capa);
  http_body_write(h, data, len);
  FIO_MEM_FREE(h->body.buf, h->body.capa);
  h->body.buf = NULL;
  h->body.capa = 0;
  return;
}

/* *****************************************************************************
Cookies
***************************************************************************** */

FIO_IFUNC void http_cookie___parse_cookie(http_s *h, sstr_s *c) {
  /* loop and read Cookie: name=value; name2=value2; name3=value3 */
  fio_str_info_s s = sstr_info(c);
  while (s.len) {
    fio_str_info_s k = {0}, v = {0};
    /* remove white-space */
    while ((s.buf[0] == ' ' || s.buf[0] == '\t') && s.len) {
      ++s.buf;
      --s.len;
    }
    if (!s.len)
      return;
    char *div = memchr(s.buf, '=', s.len);
    char *end = memchr(s.buf, ';', s.len);
    if (!end)
      end = s.buf + s.len;
    v.buf = s.buf;
    if (div) {
      /* cookie name may be an empty string */
      k.buf = s.buf;
      k.len = div - s.buf;
      v.buf = div + 1;
    }
    v.len = end - v.buf;
    s.len = (s.buf + s.len) - end;
    s.buf = end;
    /* skip the ';' if exists (if len is not zero, !!s.len == 1). */
    s.buf += !!s.len;
    s.len -= !!s.len;
    smap_set2(h->cookies, k, v, 0);
  }
}

FIO_SFUNC void http_cookie___collect(http_s *h) {
  sary_s *header = NULL;
  {
    sstr_s k = {0};
    sstr_init_const(&k, "cookie", 6);
    uint64_t hash =
        fio_risky_hash("cookie", 6, (uint64_t)(uintptr_t)(h->headers));
    header = hmap_get_ptr(h->headers, hash, k);
  }
  if (!header)
    return;
  FIO_ARRAY_EACH(sary, header, pos) { http_cookie___parse_cookie(h, pos); }
  return;
}

void http_cookie_set___(void); /* sublime text marker */
/** Sets a response cookie. Returns -1 on error and 0 on success. */
int http_cookie_set FIO_NOOP(http_s *h, http_cookie_args_s cookie) {
  FIO_ASSERT_DEBUG(h, "Can't set cookie for NULL HTTP handler!");
  if ((!h | ((cookie.name_len + cookie.value_len) >= HTTP_MAX_HEADER_LENGTH)) ||
      (h->state & (HTTP_STATE_FINISHED | HTTP_STATE_STREAMING)))
    return -1;
  /* promises that some warnings print only once. */
  static unsigned int warn_illegal = 0;
  unsigned int need2warn = 0;

  /* valid / invalid characters in cookies, create with Ruby using:
      a = []
      256.times {|i| a[i] = 1;}
      ('a'.ord..'z'.ord).each {|i| a[i] = 0;}
      ('A'.ord..'Z'.ord).each {|i| a[i] = 0;}
      ('0'.ord..'9'.ord).each {|i| a[i] = 0;}
      "!#$%&'*+-.^_`|~".bytes.each {|i| a[i] = 0;}
      p a; nil
      "!#$%&'()*+-./:<=>?@[]^_`{|}~".bytes.each {|i| a[i] = 0;} # for values
      p a; nil
  */
  static const char invalid_cookie_name_char[256] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  static const char invalid_cookie_value_char[256] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  /* write name and value while auto-correcting encoding issues */
  size_t len = 0;
  lstr_s c = FIO_STR_INIT;
  fio_str_info_s t =
      lstr_reserve(&c,
                   cookie.name_len + cookie.value_len + cookie.domain_len +
                       cookie.path_len + 128);

#define copy_cookie_ch(ch_var)                                                 \
  if (invalid_cookie_##ch_var##_char[(uint8_t)cookie.ch_var[tmp]]) {           \
    need2warn |= 1;                                                            \
    t.buf[len++] = '%';                                                        \
    t.buf[len++] = fio_i2c(((uint8_t)cookie.ch_var[tmp] >> 4) & 0x0F);         \
    t.buf[len++] = fio_i2c((uint8_t)cookie.ch_var[tmp] & 0x0F);                \
  } else {                                                                     \
    t.buf[len++] = cookie.ch_var[tmp];                                         \
  }                                                                            \
  tmp += 1;                                                                    \
  if (t.capa <= len + 3) {                                                     \
    t = lstr_reserve(&c, t.capa + 32);                                         \
  }

  if (cookie.name) {
    size_t tmp = 0;
    if (cookie.name_len) {
      while (tmp < cookie.name_len) {
        copy_cookie_ch(name);
      }
    } else {
      while (cookie.name[tmp]) {
        copy_cookie_ch(name);
      }
    }
    if (need2warn && !warn_illegal) {
      warn_illegal |= 1;
      FIO_LOG_WARNING("illegal char 0x%.2x in cookie name (in %s)\n"
                      "         automatic %% encoding applied",
                      cookie.name[tmp],
                      cookie.name);
    }
  }
  t.buf[len++] = '=';
  if (cookie.value) {
    size_t tmp = 0;
    if (cookie.value_len) {
      while (tmp < cookie.value_len) {
        copy_cookie_ch(value);
      }
    } else {
      while (cookie.value[tmp]) {
        copy_cookie_ch(value);
      }
    }
    if (need2warn && !warn_illegal) {
      warn_illegal |= 1;
      FIO_LOG_WARNING("illegal char 0x%.2x in cookie value (in %s)\n"
                      "         automatic %% encoding applied",
                      cookie.value[tmp],
                      cookie.value);
    }
  } else
    cookie.max_age = -1;
#undef copy_cookie_ch

  /* server cookie data */
  t.buf[len++] = ';';
  t.buf[len++] = ' ';
  t = lstr_resize(&c, len);

  if (cookie.max_age) {
    lstr_reserve(&c, t.len + 40);
    lstr_write(&c, "Max-Age=", 8);
    lstr_write_i(&c, cookie.max_age);
    t = lstr_write(&c, "; ", 2);
  }

  if (cookie.domain && cookie.domain_len) {
    lstr_reserve(&c, t.len + 7 + 1 + cookie.domain_len);
    lstr_write(&c, "domain=", 7);
    lstr_write(&c, cookie.domain, cookie.domain_len);
    lstr_write(&c, ";", 1);
  }
  if (cookie.path && cookie.path_len) {
    lstr_reserve(&c, t.len + 5 + 1 + cookie.path_len);
    lstr_write(&c, "path=", 5);
    lstr_write(&c, cookie.path, cookie.path_len);
    t = lstr_write(&c, ";", 1);
  }
  if (cookie.http_only) {
    lstr_write(&c, "HttpOnly;", 9);
  }
  if (cookie.secure) {
    lstr_write(&c, "secure;", 7);
  }
  switch (cookie.same_site) {
  case HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT: /* fall through */
  default: break;
  case HTTP_COOKIE_SAME_SITE_NONE: lstr_write(&c, "SameSite=None;", 14); break;
  case HTTP_COOKIE_SAME_SITE_LAX: lstr_write(&c, "SameSite=Lax;", 13); break;
  case HTTP_COOKIE_SAME_SITE_STRICT:
    lstr_write(&c, "SameSite=Strict;", 16);
    break;
  }
  lstr_s *ptr = smap_set2(h->cookies + 1,
                          FIO_STR_INFO2((char *)cookie.name, cookie.name_len),
                          lstr_info(&c),
                          1);
  if (!ptr)
    goto ptr_error;
  *ptr = c;
  /* set the "read" cookie store data */
  smap_set2(h->cookies,
            FIO_STR_INFO2((char *)cookie.name, cookie.name_len),
            FIO_STR_INFO2((char *)cookie.value, cookie.value_len),
            1);
  return 0;

ptr_error:
  lstr_destroy(&c);
  return -1;
}

/** Returns a cookie value (either received of newly set), if any. */
fio_str_info_s http_cookie_get(http_s *h, const char *name, size_t len) {
  if (!fio_atomic_or(&h->state, HTTP_STATE_COOKIES_PARSED))
    http_cookie___collect(h);
  return smap_get2(h->cookies,
                   (fio_str_info_s){.buf = (char *)name, .len = len});
}

/** Iterates through all cookies. A non-zero return will stop iteration. */
size_t http_cookie_each(http_s *h,
                        int (*callback)(http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value,
                                        void *udata),
                        void *udata) {
  size_t i = 0;
  FIO_MAP_EACH(smap, h->cookies, pos) {
    ++i;
    if (callback(h,
                 sstr_info(&pos->obj.key),
                 lstr_info(&pos->obj.value),
                 udata))
      return i;
  }
  return i;
}

/**
 * Iterates through all response set cookies.
 *
 * A non-zero return will stop iteration.
 */
size_t http_set_cookie_each(http_s *h,
                            int (*callback)(http_s *,
                                            fio_str_info_s name,
                                            fio_str_info_s value,
                                            void *udata),
                            void *udata) {
  size_t i = 0;
  smap_s *set_cookies = h->cookies + 1;
  fio_str_info_s header_name = FIO_STR_INFO2("set-cookie", 10);
  FIO_MAP_EACH(smap, set_cookies, pos) {
    ++i;
    if (callback(h, header_name, lstr_info(&pos->obj.value), udata))
      return i;
  }
  return i;
}

/* *****************************************************************************
Responding to an HTTP event.
***************************************************************************** */

/** Returns true if the HTTP handle's response was sent. */
int http_is_finished(http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return h->state & HTTP_STATE_FINISHED;
}

/** Returns true if the HTTP handle's response is streaming. */
int http_is_streaming(http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (HTTP_STATE_STREAMING == (h->state & HTTP_STATE_STREAMING));
}

/** Gets the status associated with the HTTP handle (response). */
size_t http_status_get(http_s *h) { return h->status; }

/** Sets the status associated with the HTTP handle (response). */
size_t http_status_set(http_s *h, size_t status) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (h->status = status);
}

void http_write___(void); /* sublime text marker */
/**
 * Writes `data` to the response body associated with the HTTP handle after
 * sending all headers (no further headers may be sent).
 */
void http_write FIO_NOOP(http_s *h, http_write_args_s args) {
  if (!h || (http_is_finished(h) | (!h->controller)))
    goto handle_error;
  const http_controller_s *const c = h->controller;
  hmap_s *const hdrs = h->headers + (!!h->status);
  if (!(h->state & HTTP_STATE_STREAMING)) {
    h->state |= HTTP_STATE_STREAMING;
    /* test if streaming / single body response */
    if (args.finish) {
      /* validate / set Content-Length (not streaming) */
      sstr_s k = {0};
      sstr_s v = {0};
      sary_s va = {0};
      sstr_init_const(&k, "content-length", 14);
      sstr_write_i(&v, args.len);
      va = sary_tmp(v);
      const uint64_t hash =
          fio_risky_hash("content-length", 14, (uint64_t)(uintptr_t)(hdrs));
      hmap_remove(hdrs, hash, k, NULL);
      hmap_set_ptr(hdrs, hash, k, va, NULL, 1);
      sary_destroy(&va);
      sstr_destroy(&v);
    }
    /* start a response, unless status == 0 (which starts a request). */
    (&c->start_response)[h->status == 0](h, h->status, !args.finish);
    c->send_headers(h);
  }
  c->write_body(h, args);
  h->state |= HTTP_STATE_FINISHED * (!!args.finish);
  return;

handle_error:
  if (args.fd)
    close(args.fd);
  if (args.dealloc && args.data)
    args.dealloc((void *)args.data);
}

/* *****************************************************************************
HTTP Status as String.
***************************************************************************** */

/** Returns a human readable string related to the HTTP status number. */
fio_str_info_s http_status2str(size_t status) {
  fio_str_info_s r = {0};
#define HTTP_RETURN_STATUS(str)                                                \
  do {                                                                         \
    r.len = strlen(str);                                                       \
    r.buf = str;                                                               \
    return r;                                                                  \
  } while (0);
  switch (status) {
  // clang-format off
  case 100: HTTP_RETURN_STATUS("Continue");
  case 101: HTTP_RETURN_STATUS("Switching Protocols");
  case 102: HTTP_RETURN_STATUS("Processing");
  case 103: HTTP_RETURN_STATUS("Early Hints");
  case 110: HTTP_RETURN_STATUS("Response is Stale"); /* caching code*/
  case 111: HTTP_RETURN_STATUS("Re-validation Failed"); /* caching code*/
  case 112: HTTP_RETURN_STATUS("Disconnected Operation"); /* caching code*/
  case 113: HTTP_RETURN_STATUS("Heuristic Expiration"); /* caching code*/
  case 199: HTTP_RETURN_STATUS("Miscellaneous Warning"); /* caching code*/
  case 200: HTTP_RETURN_STATUS("OK");
  case 201: HTTP_RETURN_STATUS("Created");
  case 202: HTTP_RETURN_STATUS("Accepted");
  case 203: HTTP_RETURN_STATUS("Non-Authoritative Information");
  case 204: HTTP_RETURN_STATUS("No Content");
  case 205: HTTP_RETURN_STATUS("Reset Content");
  case 206: HTTP_RETURN_STATUS("Partial Content");
  case 207: HTTP_RETURN_STATUS("Multi-Status");
  case 208: HTTP_RETURN_STATUS("Already Reported");
  case 214: HTTP_RETURN_STATUS("Transformation Applied"); /* caching code*/
  case 218: HTTP_RETURN_STATUS("This is fine (Apache Web Server)"); /* unofficial */
  case 226: HTTP_RETURN_STATUS("IM Used");
  case 299: HTTP_RETURN_STATUS("Miscellaneous Persistent Warning"); /* caching code*/
  case 300: HTTP_RETURN_STATUS("Multiple Choices");
  case 301: HTTP_RETURN_STATUS("Moved Permanently");
  case 302: HTTP_RETURN_STATUS("Found");
  case 303: HTTP_RETURN_STATUS("See Other");
  case 304: HTTP_RETURN_STATUS("Not Modified");
  case 305: HTTP_RETURN_STATUS("Use Proxy");
  case 307: HTTP_RETURN_STATUS("Temporary Redirect");
  case 308: HTTP_RETURN_STATUS("Permanent Redirect");
  case 400: HTTP_RETURN_STATUS("Bad Request");
  case 401: HTTP_RETURN_STATUS("Unauthorized");
  case 402: HTTP_RETURN_STATUS("Payment Required");
  case 403: HTTP_RETURN_STATUS("Forbidden");
  case 404: HTTP_RETURN_STATUS("Not Found");
  case 405: HTTP_RETURN_STATUS("Method Not Allowed");
  case 406: HTTP_RETURN_STATUS("Not Acceptable");
  case 407: HTTP_RETURN_STATUS("Proxy Authentication Required");
  case 408: HTTP_RETURN_STATUS("Request Timeout");
  case 409: HTTP_RETURN_STATUS("Conflict");
  case 410: HTTP_RETURN_STATUS("Gone");
  case 411: HTTP_RETURN_STATUS("Length Required");
  case 412: HTTP_RETURN_STATUS("Precondition Failed");
  case 413: HTTP_RETURN_STATUS("Content Too Large");
  case 414: HTTP_RETURN_STATUS("URI Too Long");
  case 415: HTTP_RETURN_STATUS("Unsupported Media Type");
  case 416: HTTP_RETURN_STATUS("Range Not Satisfiable");
  case 417: HTTP_RETURN_STATUS("Expectation Failed");
  case 419: HTTP_RETURN_STATUS("Page Expired (Laravel Framework)"); /* unofficial */
  case 420: HTTP_RETURN_STATUS("Enhance Your Calm (Twitter) - Method Failure (Spring Framework)"); /* unofficial */
  case 421: HTTP_RETURN_STATUS("Misdirected Request");
  case 422: HTTP_RETURN_STATUS("Unprocessable Content");
  case 423: HTTP_RETURN_STATUS("Locked");
  case 424: HTTP_RETURN_STATUS("Failed Dependency");
  case 425: HTTP_RETURN_STATUS("Too Early");
  case 426: HTTP_RETURN_STATUS("Upgrade Required");
  case 427: HTTP_RETURN_STATUS("Unassigned");
  case 428: HTTP_RETURN_STATUS("Precondition Required");
  case 429: HTTP_RETURN_STATUS("Too Many Requests");
  case 430: HTTP_RETURN_STATUS("Request Header Fields Too Large (Shopify)"); /* unofficial */
  case 431: HTTP_RETURN_STATUS("Request Header Fields Too Large");
  case 444: HTTP_RETURN_STATUS("No Response"); /* nginx code */
  case 450: HTTP_RETURN_STATUS("Blocked by Windows Parental Controls (Microsoft)"); /* unofficial */
  case 451: HTTP_RETURN_STATUS("Unavailable For Legal Reasons");
  case 494: HTTP_RETURN_STATUS("Request header too large"); /* nginx code */
  case 495: HTTP_RETURN_STATUS("SSL Certificate Error"); /* nginx code */
  case 496: HTTP_RETURN_STATUS("SSL Certificate Required"); /* nginx code */
  case 497: HTTP_RETURN_STATUS("HTTP Request Sent to HTTPS Port"); /* nginx code */
  case 498: HTTP_RETURN_STATUS("Invalid Token (Esri)"); /* unofficial */
  case 499: HTTP_RETURN_STATUS("Client Closed Request"); /* nginx code */
  case 500: HTTP_RETURN_STATUS("Internal Server Error");
  case 501: HTTP_RETURN_STATUS("Not Implemented");
  case 502: HTTP_RETURN_STATUS("Bad Gateway");
  case 503: HTTP_RETURN_STATUS("Service Unavailable");
  case 504: HTTP_RETURN_STATUS("Gateway Timeout");
  case 505: HTTP_RETURN_STATUS("HTTP Version Not Supported");
  case 506: HTTP_RETURN_STATUS("Variant Also Negotiates");
  case 507: HTTP_RETURN_STATUS("Insufficient Storage");
  case 508: HTTP_RETURN_STATUS("Loop Detected");
  case 509: HTTP_RETURN_STATUS("Bandwidth Limit Exceeded (Apache Web Server/cPanel)"); /* unofficial */
  case 510: HTTP_RETURN_STATUS("Not Extended");
  case 511: HTTP_RETURN_STATUS("Network Authentication Required");
  case 529: HTTP_RETURN_STATUS("Site is overloaded (Qualys)"); /* unofficial */
  case 530: HTTP_RETURN_STATUS("Site is frozen (Pantheon web)"); /* unofficial */
  case 598: HTTP_RETURN_STATUS("Network read timeout error"); /* unofficial */
    // clang-format on
  }
  HTTP_RETURN_STATUS("Unknown");
#undef HTTP_RETURN_STATUS
}

/* *****************************************************************************
Testing the Handle.
***************************************************************************** */
#if defined(TEST)
#ifndef FIO_URL
#define FIO_URL
#include "fio-stl.h"
#endif
void http_test FIO_NOOP(void) {
  fprintf(stderr, "* Testing HTTP handle.\r\n");
  http_s *h = http_new();
  FIO_ASSERT_ALLOC(h);
  { /* test status */
    size_t old_status = http_status_get(h);
    http_status_set(h, ++old_status);
    FIO_ASSERT(http_status_get(h) == old_status, "status set round-trip error");
  }
  { /* test url type parameters */
    fio_url_s url =
        fio_url_parse("http://example.com/my/very/long/path/example/should/"
                      "always/allocate/memory?query=0",
                      82);
    /* path */
    http_path_set(h, FIO_BUF2STR_INFO(url.path));
    FIO_ASSERT(http_path_get(h).len == url.path.len &&
                   !memcmp(http_path_get(h).buf, url.path.buf, url.path.len),
               "path set round-trip error");
    FIO_ASSERT(http_path_get(h).buf != url.path.buf, "path copy error");
    /* query */
    http_query_set(h, FIO_BUF2STR_INFO(url.query));
    FIO_ASSERT(http_query_get(h).len == url.query.len &&
                   !memcmp(http_query_get(h).buf, url.query.buf, url.query.len),
               "query set round-trip error");
    FIO_ASSERT(http_query_get(h).buf != url.query.buf, "query copy error");
    /* host header */
    http_request_header_add(h,
                            FIO_STR_INFO2("host", 4),
                            FIO_BUF2STR_INFO(url.host));
    http_request_header_add(h,
                            FIO_STR_INFO2("host", 4),
                            FIO_BUF2STR_INFO(url.path));
    FIO_ASSERT(
        (http_request_header_get(h, FIO_STR_INFO2("host", 4), 0).len ==
             url.host.len &&
         !memcmp(http_request_header_get(h, FIO_STR_INFO2("host", 4), 0).buf,
                 url.host.buf,
                 url.host.len)),
        "host header set round-trip error");
    FIO_ASSERT(http_request_header_get(h, FIO_STR_INFO2("host", 4), 0).buf !=
                   url.host.buf,
               "host copy error");
    http_request_header_add(h,
                            FIO_STR_INFO2("host", 4),
                            FIO_BUF2STR_INFO(url.path));
    FIO_ASSERT(
        http_request_header_get(h, FIO_STR_INFO2("host", 4), 1).len ==
                url.path.len &&
            !memcmp(http_request_header_get(h, FIO_STR_INFO2("host", 4), 1).buf,
                    url.path.buf,
                    url.path.len),
        "host header[1] set round-trip error");
    FIO_ASSERT(
        http_request_header_get(h, FIO_STR_INFO2("host", 4), 0).len ==
                url.host.len &&
            !memcmp(http_request_header_get(h, FIO_STR_INFO2("host", 4), 0).buf,
                    url.host.buf,
                    url.host.len),
        "host header[0] data lost!");
  }
  http_free(h);
}
#endif
