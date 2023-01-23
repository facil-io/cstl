/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___    /* Development inclusion - ignore line */
#define FIO_HTTP_HANDLE /* Development inclusion - ignore line */
#define FIO_STR         /* Development inclusion - ignore line */
#include "./include.h"  /* Development inclusion - ignore line */
#endif                  /* Development inclusion - ignore line */
/* *****************************************************************************




                      An HTTP connection Handle helper




Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP_HANDLE) && !defined(FIO_STL_KEEP__) &&                    \
    !defined(H___FIO_HTTP_HANDLE___H)
#define H___FIO_HTTP_HANDLE___H

/* *****************************************************************************
HTTP Handle Settings
***************************************************************************** */
#ifndef FIO_HTTP_EXACT_LOGGING
/**
 * By default, facil.io logs the HTTP request cycle using a fuzzy starting and
 * ending point for the time stamp.
 *
 * The fuzzy timestamp includes delays that aren't related to the HTTP request
 * and may ignore time passed due to timestamp caching.
 *
 * On the other hand, `FIO_HTTP_EXACT_LOGGING` collects exact time stamps to
 * measure the time it took to process the HTTP request (excluding time spent
 * reading / writing the data from the network).
 *
 * Due to the preference to err on the side of higher performance, fuzzy
 * time-stamping is the default.
 */
#define FIO_HTTP_EXACT_LOGGING 0
#endif

#ifndef FIO_HTTP_BODY_RAM_LIMIT
/**
 * The HTTP handle automatically switches between RAM storage and file storage
 * once the HTTP body (payload) reaches a certain size. This control this point
 * of transition
 */
#define FIO_HTTP_BODY_RAM_LIMIT (1 << 17)
#endif

/* *****************************************************************************
HTTP Handle Type
***************************************************************************** */

/** Named arguments for the http_write function. */
typedef struct fio_http_s fio_http_s;

/**
 * The HTTP Controller points to all the callbacks required by the HTTP Handler.
 *
 * This allows the HTTP Handler to be somewhat protocol agnostic.
 */
typedef struct fio_http_controller_s fio_http_controller_s;

/* *****************************************************************************
Constructor / Destructor
***************************************************************************** */

/** Create a new fio_http_s handle. */
SFUNC fio_http_s *fio_http_new(void);

/** Reduces an fio_http_s handle's reference count or frees it. */
SFUNC void fio_http_free(fio_http_s *);

/** Increases an fio_http_s handle's reference count. */
SFUNC fio_http_s *fio_http_dup(fio_http_s *);

/* *****************************************************************************
Opaque User and Controller Data
***************************************************************************** */

/** Gets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata_get(fio_http_s *);

/** Sets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata_set(fio_http_s *, void *);

/** Gets the HTTP Controller associated with the HTTP handle. */
FIO_IFUNC fio_http_controller_s *fio_http_controller_get(fio_http_s *h);

/** Gets the HTTP Controller associated with the HTTP handle. */
FIO_IFUNC fio_http_controller_s *fio_http_controller_set(
    fio_http_s *h,
    fio_http_controller_s *controller);

/** Returns the existing controller data (`void *` pointer). */
FIO_IFUNC void *fio_http_cdata_get(fio_http_s *h);

/** Sets a new controller data (`void *` pointer). */
FIO_IFUNC void *fio_http_cdata_set(fio_http_s *h, void *cdata);

/* *****************************************************************************
Data associated with the Request (usually set by the HTTP protocol)
***************************************************************************** */

/** Gets the method information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_method_get(fio_http_s *);

/** Sets the method information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s);

/** Gets the path information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_path_get(fio_http_s *);

/** Sets the path information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s);

/** Gets the query information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_query_get(fio_http_s *);

/** Sets the query information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s);

/** Gets the version information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_version_get(fio_http_s *);

/** Sets the version information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_version_set(fio_http_s *, fio_str_info_s);

/**
 * Gets the header information associated with the HTTP handle.
 *
 * Since more than a single value may be associated with a header name, the
 * index may be used to collect subsequent values.
 *
 * An empty value is returned if no header value is available (or index is
 * exceeded).
 */
SFUNC fio_str_info_s fio_http_request_header_get(fio_http_s *,
                                                 fio_str_info_s name,
                                                 size_t index);

/** Sets the header information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_request_header_set(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);

/** Sets the header information associated with the HTTP handle. */
SFUNC fio_str_info_s
fio_http_request_header_set_if_missing(fio_http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);

/** Adds to the header information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_request_header_add(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);

/**
 * Iterates through all request headers (except cookies!).
 *
 * A non-zero return will stop iteration.
 * */
SFUNC size_t fio_http_request_header_each(fio_http_s *,
                                          int (*callback)(fio_http_s *,
                                                          fio_str_info_s name,
                                                          fio_str_info_s value,
                                                          void *udata),
                                          void *udata);

/** Gets the body (payload) length associated with the HTTP handle. */
SFUNC size_t fio_http_body_length(fio_http_s *);

/** Adjusts the body's reading position. Negative values start at the end. */
SFUNC size_t fio_http_body_seek(fio_http_s *, ssize_t pos);

/** Reads up to `length` of data from the body, returns nothing on EOF. */
SFUNC fio_str_info_s fio_http_body_read(fio_http_s *, size_t length);

/**
 * Reads from the body until finding `token`, reaching `limit` or EOF.
 *
 * Note: `limit` is ignored if the
 */
SFUNC fio_str_info_s fio_http_body_read_until(fio_http_s *,
                                              char token,
                                              size_t limit);

/** Allocates a body (payload) of (at least) the `expected_length`. */
SFUNC void fio_http_body_expect(fio_http_s *, size_t expected_length);

/** Writes `data` to the body (payload) associated with the HTTP handle. */
SFUNC void fio_http_body_write(fio_http_s *, const void *data, size_t len);

/* *****************************************************************************
Cookies
***************************************************************************** */

/**
 * This is a helper for setting cookie data.
 *
 * This struct is used together with the `fio_http_cookie_set` macro. i.e.:
 *
 *       fio_http_set_cookie(h,
 *                      .name = "my_cookie",
 *                      .value = "data");
 *
 */
typedef struct {
  /** The cookie's name. */
  const char *name;
  /** The cookie's value (leave blank to delete cookie). */
  const char *value;
  /** The cookie's domain (optional). */
  const char *domain;
  /** The cookie's path (optional). */
  const char *path;
  /** The cookie name's size in bytes or a terminating NUL will be assumed.*/
  size_t name_len;
  /** The cookie value's size in bytes or a terminating NUL will be assumed.*/
  size_t value_len;
  /** The cookie domain's size in bytes or a terminating NUL will be assumed.*/
  size_t domain_len;
  /** The cookie path's size in bytes or a terminating NULL will be assumed.*/
  size_t path_len;
  /** Max Age (how long should the cookie persist), in seconds (0 == session).*/
  int max_age;
  /**
   * The SameSite settings.
   *
   * See: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
   */
  enum {
    /** allow the browser to dictate this property */
    HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT = 0,
    /** The browser sends the cookie with cross-site and same-site requests. */
    HTTP_COOKIE_SAME_SITE_NONE,
    /**
     * The cookie is withheld on cross-site sub-requests.
     *
     * The cookie is sent when a user navigates to the URL from an external
     * site.
     */
    HTTP_COOKIE_SAME_SITE_LAX,
    /** The browser sends the cookie only for same-site requests. */
    HTTP_COOKIE_SAME_SITE_STRICT,
  } same_site;
  /** Limit cookie to secure connections.*/
  unsigned secure : 1;
  /** Limit cookie to HTTP (intended to prevent JavaScript access/hijacking).*/
  unsigned http_only : 1;
} fio_http_cookie_args_s;

/**
 * Sets a response cookie.
 *
 * Returns -1 on error and 0 on success.
 *
 * Note: Long cookie names and long cookie values will be considered a security
 * violation and an error will be returned. Many browsers and proxies impose
 * limits on headers and cookies, cookies often limited to 4Kb in total for both
 * name and value.
 */
SFUNC int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s);

#ifndef __cplusplus
/** Named arguments helper. See fio_http_cookie_args_s for details. */
#define fio_http_cookie_set(http___handle, ...)                                \
  fio_http_cookie_set((http___handle), (fio_http_cookie_args_s){__VA_ARGS__})
#endif

/** Returns a cookie value (either received of newly set), if any. */
SFUNC fio_str_info_s fio_http_cookie_get(fio_http_s *,
                                         const char *name,
                                         size_t name_len);

/** Iterates through all cookies. A non-zero return will stop iteration. */
SFUNC size_t fio_http_cookie_each(fio_http_s *,
                                  int (*callback)(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value,
                                                  void *udata),
                                  void *udata);

/**
 * Iterates through all response set cookies.
 *
 * A non-zero return value from the callback will stop iteration.
 */
SFUNC size_t
fio_http_set_cookie_each(fio_http_s *h,
                         int (*callback)(fio_http_s *,
                                         fio_str_info_s set_cookie_header,
                                         fio_str_info_s value,
                                         void *udata),
                         void *udata);

/* *****************************************************************************
Responding to an HTTP event.
***************************************************************************** */

/** Returns true if the HTTP handle's response was sent. */
SFUNC int fio_http_is_finished(fio_http_s *);

/** Returns true if the HTTP handle's response is streaming. */
SFUNC int fio_http_is_streaming(fio_http_s *);

/** Gets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status_get(fio_http_s *);

/** Sets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status_set(fio_http_s *, size_t status);

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
SFUNC fio_str_info_s fio_http_response_header_get(fio_http_s *,
                                                  fio_str_info_s name,
                                                  size_t index);

/**
 * Sets the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
SFUNC fio_str_info_s fio_http_response_header_set(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);
/**
 * Sets the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
SFUNC fio_str_info_s
fio_http_response_header_set_if_missing(fio_http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);

/**
 * Adds to the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
SFUNC fio_str_info_s fio_http_response_header_add(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);

/**
 * Iterates through all response headers (except cookies!).
 *
 * A non-zero return will stop iteration.
 * */
SFUNC size_t fio_http_response_header_each(fio_http_s *,
                                           int (*callback)(fio_http_s *,
                                                           fio_str_info_s name,
                                                           fio_str_info_s value,
                                                           void *udata),
                                           void *udata);

/** Arguments for the fio_http_write function. */
typedef struct fio_http_write_args_s {
  /** The data to be written. */
  const void *data;
  /** The length of the data to be written. */
  size_t len;
  /** If streaming a file, set this value. The file is always closed. */
  int fd;
  /** If the data is a buffer, this callback may be set to free it once sent. */
  void (*dealloc)(void *);
  /** If the data is a buffer / a file - should it be copied? */
  int copy;
  /**
   * If `finish` is set, this data marks the end of the response.
   *
   * Otherwise the response will stream the data.
   */
  int finish;
} fio_http_write_args_s;

/**
 * Writes `data` to the response body associated with the HTTP handle after
 * sending all headers (no further headers may be sent).
 */
SFUNC void fio_http_write(fio_http_s *, fio_http_write_args_s args);

#ifndef __cplusplus
/** Named arguments helper. See fio_http_write and fio_http_write_args_s. */
#define fio_http_write(http_handle, ...)                                       \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)
#endif

/* *****************************************************************************
General Helpers
***************************************************************************** */

/** Returns a human readable string related to the HTTP status number. */
SFUNC fio_str_info_s fio_http_status2str(size_t status);

/** Logs an HTTP (response) to STDOUT. */
SFUNC void fio_http_write_log(fio_http_s *h, fio_buf_info_s peer_addr);

/* *****************************************************************************
The HTTP Controller
***************************************************************************** */

/** (TODO: review necessary callbacks)
 * The HTTP Controller manages all the callbacks required by the HTTP Handler in
 * order for HTTP responses and requests to be sent.
 */
struct fio_http_controller_s {
  /* MUST be initialized to zero, used internally by the HTTP Handle. */
  uintptr_t private_flags;
  /** Called before an HTTP handler link to an HTTP Controller is revoked. */
  void (*on_unlinked)(fio_http_s *h, void *cdata);
  /** Informs the controller that a response is starting. */
  int (*start_response)(fio_http_s *h, int status, int will_stream);
  /** Informs the controller that a request is starting. */
  int (*start_request)(fio_http_s *h, int reserved, int will_stream);
  /** Informs the controller that all headers must be sent. */
  void (*send_headers)(fio_http_s *h);
  /** called by the HTTP handle for each body chunk (or to finish a response. */
  void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
  /** called once a request / response had finished */
  void (*on_finish)(fio_http_s *h);
};

/* *****************************************************************************
HTTP Handle Implementation - inlined static functions
***************************************************************************** */

#define FIO___HTTP_GETSET_PTR(type, name, index_)                              \
  /** Used internally to set / get the propecrty at its known pointer index.   \
   */                                                                          \
  FIO_IFUNC type *fio_http_##name##_get(fio_http_s *h) {                       \
    return ((type **)h)[index_];                                               \
  }                                                                            \
  /** Used internally to set / get the propercty at its known pointer index.   \
   */                                                                          \
  FIO_IFUNC type *fio_http_##name##_set(fio_http_s *h, type *ptr) {            \
    return (((type **)h)[index_] = ptr);                                       \
  }

/* Create fio_http_udata_(get|set) functions */
FIO___HTTP_GETSET_PTR(void, udata, 0)
/* Create fio_http_cdata_(get|set) functions */
FIO___HTTP_GETSET_PTR(void, cdata, 1)
/* Create fio_http_controller_(get|set) functions */
FIO___HTTP_GETSET_PTR(fio_http_controller_s, controller, 2)

#undef FIO___HTTP_GETSET_PTR
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
HTTP Handle Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Helpers - memory allocation & logging time collection
***************************************************************************** */

FIO_SFUNC void fio___http_keystr_free(void *ptr, size_t len) {
  FIO_MEM_FREE_(ptr, len);
  (void)len; /* if unused */
}
FIO_SFUNC void *fio___http_keystr_alloc(size_t capa) {
  return FIO_MEM_REALLOC_(NULL, 0, capa, 0);
}

#if FIO_HTTP_EXACT_LOGGING
FIO_IFUNC int64_t fio_http_get_timestump(void) { return fio_time_milli(); }
#else
int64_t fio_last_tick(void);
FIO_IFUNC int64_t fio_http_get_timestump(void) {
  return (int64_t)fio_last_tick();
}
#endif

FIO_SFUNC fio_str_info_s fio_http_date(uint64_t now_milli) {
  static char date_buf[128];
  static size_t date_len;
  static uint64_t date_buf_val;
  const uint64_t now_time = now_milli / 1000;
  if (date_buf_val == now_time)
    return FIO_STR_INFO2(date_buf, date_len);
  date_len = fio_time2rfc7231(date_buf, now_time);
  date_buf[date_len] = 0;
  date_buf_val = now_time;
  return FIO_STR_INFO2(date_buf, date_len);
}

#define FIO_STL_KEEP__ 1
/* *****************************************************************************
String Cache
***************************************************************************** */

#define FIO_MAP_NAME fio___http_str_cache
#define FIO_MAP_LRU  1024
#define FIO_MAP_KEY_BSTR
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)http_new)
#include FIO_INCLUDE_FILE

static fio___http_str_cache_s FIO___HTTP_STRING_CACHE[3] = {{0}};
#define FIO___HTTP_STR_CACHE_NAME   0
#define FIO___HTTP_STR_CACHE_COOKIE 1
#define FIO___HTTP_STR_CACHE_VALUE  2

static fio_str_info_s fio___http_str_copy(size_t group, fio_str_info_s s) {
  fio_str_info_s r =
      fio___http_str_cache_set_if_missing(FIO___HTTP_STRING_CACHE + group, s);
  r.buf = fio_bstr_copy(r.buf);
  return r;
}

FIO_DESTRUCTOR(fio___http_str_cache_cleanup) {
  fio___http_str_cache_destroy(FIO___HTTP_STRING_CACHE);
  fio___http_str_cache_destroy(FIO___HTTP_STRING_CACHE + 1);
  fio___http_str_cache_destroy(FIO___HTTP_STRING_CACHE + 2);
}

/* *****************************************************************************
Headers Maps
***************************************************************************** */

#define FIO_ARRAY_NAME              fio___http_sary
#define FIO_ARRAY_TYPE              char *
#define FIO_ARRAY_TYPE_DESTROY(obj) fio_bstr_free(obj)

#define FIO_MAP_NAME                 fio___http_hmap
#define FIO_MAP_KEY                  fio_str_info_s
#define FIO_MAP_KEY_INTERNAL         char *
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_bstr_info((k))
#define FIO_MAP_KEY_CMP(a, b)        fio_bstr_is_eq2info((a), (b))
#define FIO_MAP_KEY_DESTROY(key)     fio_bstr_free((key))
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  (dest) = fio___http_str_copy(FIO___HTTP_STR_CACHE_NAME, (src))
#define FIO_MAP_KEY_DISCARD(key)
#define FIO_MAP_VALUE fio___http_sary_s
#define FIO_MAP_VALUE_COPY(a, b)                                               \
  do {                                                                         \
    (a) = (fio___http_sary_s)FIO_ARRAY_INIT;                                   \
    (void)(b);                                                                 \
  } while (0) /*no-op*/
#define FIO_MAP_VALUE_DESTROY(o) fio___http_sary_destroy(&(o))
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)http_new)
#include FIO_INCLUDE_FILE

/** set `add` to positive to add multiple values or negative to overwrite. */
FIO_IFUNC fio_str_info_s fio___http_hmap_set2(fio___http_hmap_s *map,
                                              fio_str_info_s key,
                                              fio_str_info_s val,
                                              int add) {
  fio_str_info_s r = {0};
  fio___http_sary_s *o;
  if (!key.buf || !key.len || !map)
    return r;
  if (!val.buf || !val.len)
    goto remove_key;
  o = fio___http_hmap_node2val_ptr(fio___http_hmap_get_ptr(map, key));
  if (!o) {
    fio___http_sary_s va = {0};
    o = fio___http_hmap_node2val_ptr(
        fio___http_hmap_set_ptr(map, key, va, NULL, 1));
    add = 1;
  }
  if (FIO_UNLIKELY(!o)) {
    FIO_LOG_ERROR("Couldn't add value to header: %.*s:%.*s",
                  (int)key.len,
                  key.buf,
                  (int)val.len,
                  val.buf);
    return r;
  }
  if (add) {
    if (add < 0) {
      fio___http_sary_destroy(o);
    }
    r = fio___http_str_copy(FIO___HTTP_STR_CACHE_VALUE, val);
    fio___http_sary_push(o, r.buf);
    return r;
  }
  r = fio_bstr_info(fio___http_sary_get(o, -1));
  return r;

remove_key:
  if (!add)
    fio___http_hmap_remove(map, key, NULL);
  return r;
}

FIO_IFUNC fio_str_info_s http_hmap_get2(fio___http_hmap_s *map,
                                        fio_str_info_s key,
                                        int32_t index) {
  fio_str_info_s r = {0};
  fio___http_sary_s *a =
      fio___http_hmap_node2val_ptr(fio___http_hmap_get_ptr(map, key));
  if (!a)
    return r;
  const uint32_t count = fio___http_sary_count(a);
  if (!count)
    return r;
  if (index < 0) {
    index += count;
    if (index < 0)
      index = 0;
  }
  if ((uint32_t)index >= count)
    index = count - 1;
  r = fio_bstr_info(fio___http_sary_get(a, index));
  return r;
}

/* *****************************************************************************
Header iteration Task
***************************************************************************** */

typedef struct {
  fio_http_s *h;
  int (*callback)(fio_http_s *, fio_str_info_s, fio_str_info_s, void *);
  void *udata;
} fio___http_hmap_each_info_s;

FIO_SFUNC int http___h_each_task_wrapper(fio___http_hmap_each_s *e) {
  fio___http_hmap_each_info_s *data = (fio___http_hmap_each_info_s *)(e->udata);
  FIO_ARRAY_EACH(fio___http_sary, &e->value, pos) {
    if (data->callback(data->h, e->key, fio_bstr_info(*pos), data->udata) == -1)
      return -1;
  }
  return 0;
}

/* *****************************************************************************
Cookie Maps
***************************************************************************** */

#define FIO_MAP_NAME                 fio___http_cmap /* cached names */
#define FIO_MAP_KEY                  fio_str_info_s
#define FIO_MAP_KEY_INTERNAL         char *
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_bstr_info((k))
#define FIO_MAP_KEY_CMP(a, b)        fio_bstr_is_eq2info((a), (b))
#define FIO_MAP_KEY_DESTROY(key)     fio_bstr_free((key))
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  (dest) = fio___http_str_copy(FIO___HTTP_STR_CACHE_COOKIE, (src))
#define FIO_MAP_KEY_DISCARD(key)

#define FIO_MAP_VALUE_BSTR /* not cached */
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)http_new)
#include FIO_INCLUDE_FILE

/* *****************************************************************************
HTTP Handle Type
***************************************************************************** */

#define FIO_HTTP_STATE_FINISHED       1
#define FIO_HTTP_STATE_STREAMING      2
#define FIO_HTTP_STATE_COOKIES_PARSED 4

struct fio_http_s {
  void *udata;
  void *cdata;
  fio_http_controller_s *controller;
  int64_t received_at;
  size_t state;
  size_t sent;
  size_t status;
  fio_keystr_s method;
  fio_keystr_s path;
  fio_keystr_s query;
  fio_keystr_s version;
  fio___http_hmap_s headers[2]; /* request, response */
  fio___http_cmap_s cookies[2]; /* read, write */
  struct {
    char *buf;
    size_t len;
    size_t pos;
    size_t capa;
    int fd;
  } body;
};

#define HTTP_HDR_REQUEST(h)  (h->headers + 0)
#define HTTP_HDR_RESPONSE(h) (h->headers + 1)

void fio_http_destroy(fio_http_s *h) {
  if (!h)
    return;
  if (h->controller)
    h->controller->on_unlinked(h, h->cdata);
  fio_keystr_destroy(&h->method, fio___http_keystr_free);
  fio_keystr_destroy(&h->path, fio___http_keystr_free);
  fio_keystr_destroy(&h->query, fio___http_keystr_free);
  fio_keystr_destroy(&h->version, fio___http_keystr_free);
  fio___http_hmap_destroy(h->headers);
  fio___http_hmap_destroy(h->headers + 1);
  fio___http_cmap_destroy(h->cookies);
  fio___http_cmap_destroy(h->cookies + 1);
  FIO_MEM_FREE(h->body.buf, h->body.capa);
  if (h->body.fd != -1)
    close(h->body.fd);
  *h = (fio_http_s){.received_at = fio_http_get_timestump(), .body.fd = -1};
}

#define FIO_REF_NAME fio_http
#define FIO_REF_INIT(h)                                                        \
  h = (fio_http_s) { .received_at = fio_http_get_timestump(), .body.fd = -1 }
#define FIO_REF_DESTROY(h) fio_http_destroy(&(h))
#include FIO_INCLUDE_FILE

/** Create a new http_s handle. */
SFUNC fio_http_s *fio_http_new(void) { return fio_http_new2(); }

/** Reduces an http_s handle's reference count or frees it. */
SFUNC void fio_http_free(fio_http_s *h) { fio_http_free2(h); }

/** Increases an http_s handle's reference count. */
SFUNC fio_http_s *fio_http_dup(fio_http_s *h) { return fio_http_dup2(h); }

#undef FIO_STL_KEEP__
/* *****************************************************************************
Short String Property Set / Get
***************************************************************************** */

#define HTTP___MAKE_GET_SET(property)                                          \
  fio_str_info_s fio_http_##property##_get(fio_http_s *h) {                    \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    return fio_keystr_info(&h->property);                                      \
  }                                                                            \
                                                                               \
  fio_str_info_s fio_http_##property##_set(fio_http_s *h,                      \
                                           fio_str_info_s value) {             \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    fio_keystr_destroy(&h->property, fio___http_keystr_free);                  \
    h->property = fio_keystr_copy(value, fio___http_keystr_alloc);             \
    return fio_keystr_info(&h->property);                                      \
  }

HTTP___MAKE_GET_SET(method)
HTTP___MAKE_GET_SET(path)
HTTP___MAKE_GET_SET(query)
HTTP___MAKE_GET_SET(version)

#undef HTTP___MAKE_GET_SET
/* *****************************************************************************
HTTP Handle Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MODULE_NAME)(void) {
  /*
   * TODO: test module here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_HTTP_HANDLE */
#undef FIO_HTTP_HANDLE
