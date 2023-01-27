/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP_HANDLE        /* Development inclusion - ignore line */
#define FIO_STR                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                      An HTTP connection Handle helper




Copyright and License: see header file (000 copyright.h) or top of file
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

#ifndef FIO_HTTP_CACHE_LIMIT
/** Each of the 3 HTTP String Caches will be limited to this String count. */
#define FIO_HTTP_CACHE_LIMIT (1 << 10)
#endif

#ifndef FIO_HTTP_CACHE_STR_MAX_LEN
/** The HTTP handle will avoid caching strings longer than this value. */
#define FIO_HTTP_CACHE_STR_MAX_LEN (1 << 12)
#endif

#ifndef FIO_HTTP_CACHE_USES_MUTEX
/** The HTTP cache will use a mutex to allow headers to be set concurrently. */
#define FIO_HTTP_CACHE_USES_MUTEX 1
#endif

/* *****************************************************************************
HTTP Handle Type
***************************************************************************** */

/**
 * The HTTP Handle type.
 *
 * Note that the type is NOT designed to be thread-safe.
 */
typedef struct fio_http_s fio_http_s;

/**
 * The HTTP Controller points to all the callbacks required by the HTTP Handler.
 *
 * This allows the HTTP Handler to be somewhat protocol agnostic.
 *
 * Note: if the controller callbacks aren't thread-safe, than the `http_write`
 * function MUST NOT be called from any thread except the thread that the
 * controller is expecting.
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
FIO_IFUNC void *fio_http_udata(fio_http_s *);

/** Sets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata_set(fio_http_s *, void *);

/** Gets the HTTP Controller associated with the HTTP handle. */
FIO_IFUNC fio_http_controller_s *fio_http_controller(fio_http_s *h);

/** Gets the HTTP Controller associated with the HTTP handle. */
FIO_IFUNC fio_http_controller_s *fio_http_controller_set(
    fio_http_s *h,
    fio_http_controller_s *controller);

/** Returns the existing controller data (`void *` pointer). */
FIO_IFUNC void *fio_http_cdata(fio_http_s *h);

/** Sets a new controller data (`void *` pointer). */
FIO_IFUNC void *fio_http_cdata_set(fio_http_s *h, void *cdata);

/* *****************************************************************************
Data associated with the Request (usually set by the HTTP protocol)
***************************************************************************** */

/** Gets the method information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_method(fio_http_s *);

/** Sets the method information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s);

/** Gets the path information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_path(fio_http_s *);

/** Sets the path information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s);

/** Gets the query information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_query(fio_http_s *);

/** Sets the query information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s);

/** Gets the version information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_version(fio_http_s *);

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
SFUNC fio_str_info_s fio_http_request_header(fio_http_s *,
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
 * Possible values for the `same_site` property in the cookie settings.
 *
 * See: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
 */
typedef enum fio_http_cookie_same_site_e {
  /** allow the browser to dictate this property */
  FIO_HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT = 0,
  /** The browser sends the cookie with cross-site and same-site requests. */
  FIO_HTTP_COOKIE_SAME_SITE_NONE,
  /**
   * The cookie is withheld on cross-site sub-requests.
   *
   * The cookie is sent when a user navigates to the URL from an external
   * site.
   */
  FIO_HTTP_COOKIE_SAME_SITE_LAX,
  /** The browser sends the cookie only for same-site requests. */
  FIO_HTTP_COOKIE_SAME_SITE_STRICT,
} fio_http_cookie_same_site_e;

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
typedef struct fio_http_cookie_args_s {
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
  /** SameSite value. */
  fio_http_cookie_same_site_e same_site;
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
SFUNC fio_str_info_s fio_http_cookie(fio_http_s *,
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
SFUNC size_t fio_http_status(fio_http_s *);

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
SFUNC fio_str_info_s fio_http_response_header(fio_http_s *,
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
WebSocket / SSE Helpers
***************************************************************************** */

/** Returns non-zero if request headers ask for a WebSockets Upgrade.*/
SFUNC int fio_http_websockets_requested(fio_http_s *);

/** Sets response data to agree to a WebSockets Upgrade.*/
SFUNC int fio_http_websockets_set_response(fio_http_s *);

/** Sets request data to request a WebSockets Upgrade.*/
SFUNC void fio_http_websockets_set_request(fio_http_s *);

/** Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_requested(fio_http_s *);

/** Sets response data to agree to an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_response(fio_http_s *);

/** Sets request data to request an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_request(fio_http_s *);

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

#define FIO___HTTP_GETSET_PTR(type, name, index_, pre_set_code)                \
  /** Used internally to set / get the propecrty at its known pointer index.   \
   */                                                                          \
  FIO_IFUNC type *fio_http_##name(fio_http_s *h) {                             \
    return ((type **)h)[index_];                                               \
  }                                                                            \
  /** Used internally to set / get the propercty at its known pointer index.   \
   */                                                                          \
  FIO_IFUNC type *fio_http_##name##_set(fio_http_s *h, type *ptr) {            \
    pre_set_code;                                                              \
    return (((type **)h)[index_] = ptr);                                       \
  }

SFUNC void fio___http_controller_validate(fio_http_controller_s *c);

/* Create fio_http_udata_(get|set) functions */
FIO___HTTP_GETSET_PTR(void, udata, 0, (void)0)
/* Create fio_http_cdata_(get|set) functions */
FIO___HTTP_GETSET_PTR(void, cdata, 1, (void)0)
/* Create fio_http_controller_(get|set) functions */
FIO___HTTP_GETSET_PTR(fio_http_controller_s,
                      controller,
                      2,
                      if (!ptr->private_flags)
                          fio___http_controller_validate(ptr))

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
#define FIO_MAP_LRU  FIO_HTTP_CACHE_LIMIT
#define FIO_MAP_KEY_BSTR
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)fio_http_new)
#include FIO_INCLUDE_FILE

static struct {
  fio___http_str_cache_s cache;
  FIO___LOCK_TYPE lock;
} FIO___HTTP_STRING_CACHE[3] = {{.lock = FIO___LOCK_INIT},
                                {.lock = FIO___LOCK_INIT},
                                {.lock = FIO___LOCK_INIT}};
#define FIO___HTTP_STR_CACHE_NAME   0
#define FIO___HTTP_STR_CACHE_COOKIE 1
#define FIO___HTTP_STR_CACHE_VALUE  2

static char *fio___http_str_cached(size_t group, fio_str_info_s s) {
  fio_str_info_s cached;
  if (s.len > FIO_HTTP_CACHE_STR_MAX_LEN)
    goto avoid_caching;
#if FIO_HTTP_CACHE_USES_MUTEX
  FIO___LOCK_LOCK(FIO___HTTP_STRING_CACHE[group].lock);
#endif
  cached =
      fio___http_str_cache_set_if_missing(&FIO___HTTP_STRING_CACHE[group].cache,
                                          s);
#if FIO_HTTP_CACHE_USES_MUTEX
  FIO___LOCK_UNLOCK(FIO___HTTP_STRING_CACHE[group].lock);
#endif
  return fio_bstr_copy(cached.buf);
avoid_caching:
  return fio_bstr_write(NULL, s.buf, s.len);
}

FIO_DESTRUCTOR(fio___http_str_cache_cleanup) {
  for (size_t i = 0; i < 3; ++i) {
    const char *names[] = {"header names", "cookie names", "header values"};
    FIO_LOG_DEBUG2(
        "(%d) freeing %zu strings from %s cache",
        getpid(),
        fio___http_str_cache_count(&FIO___HTTP_STRING_CACHE[i].cache),
        names[i]);
    fio___http_str_cache_destroy(&FIO___HTTP_STRING_CACHE[i].cache);
    FIO___LOCK_DESTROY(FIO___HTTP_STRING_CACHE[i].lock);
  }
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
  (dest) = fio___http_str_cached(FIO___HTTP_STR_CACHE_NAME, (src))
#define FIO_MAP_KEY_DISCARD(key)
#define FIO_MAP_VALUE fio___http_sary_s
#define FIO_MAP_VALUE_COPY(a, b)                                               \
  do {                                                                         \
    (a) = (fio___http_sary_s)FIO_ARRAY_INIT;                                   \
    (void)(b);                                                                 \
  } while (0) /*no-op*/
#define FIO_MAP_VALUE_DESTROY(o) fio___http_sary_destroy(&(o))
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)fio___http_sary_destroy)
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
    r = fio_bstr_info(fio___http_str_cached(FIO___HTTP_STR_CACHE_VALUE, val));
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

FIO_IFUNC fio_str_info_s fio___http_hmap_get2(fio___http_hmap_s *map,
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
  (dest) = fio___http_str_cached(FIO___HTTP_STR_CACHE_COOKIE, (src))
#define FIO_MAP_KEY_DISCARD(key)

#define FIO_MAP_VALUE_BSTR /* not cached */
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)fio___http_cmap_destroy)
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Controller Validation
***************************************************************************** */

FIO_SFUNC void fio___mock_c_on_unlinked(fio_http_s *h, void *cdata) {
  (void)h, (void)cdata;
}
FIO_SFUNC int fio___mock_c_start_response(fio_http_s *h,
                                          int status,
                                          int streaming) {
  return -1;
  (void)h, (void)status, (void)streaming;
}
FIO_SFUNC void fio___mock_c_send_headers(fio_http_s *h) { (void)h; }
FIO_SFUNC void fio___mock_c_write_body(fio_http_s *h,
                                       fio_http_write_args_s args) {
  if (args.data) {
    if (args.dealloc)
      args.dealloc((void *)args.data);
  } else if (args.fd != -1) {
    close(args.fd);
  }
  (void)h;
}

FIO_SFUNC void fio___mock_c_on_finish(fio_http_s *h) { (void)h; }

SFUNC void fio___http_controller_validate(fio_http_controller_s *c) {
  if (!c->on_unlinked)
    c->on_unlinked = fio___mock_c_on_unlinked;
  if (!c->start_response)
    c->start_response = fio___mock_c_start_response;
  if (!c->start_request)
    c->start_request = fio___mock_c_start_response;
  if (!c->send_headers)
    c->send_headers = fio___mock_c_send_headers;
  if (!c->write_body)
    c->write_body = fio___mock_c_write_body;
  if (!c->on_finish)
    c->on_finish = fio___mock_c_on_finish;
}

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
  fio_bstr_free(h->body.buf);
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
Simple Property Set / Get
***************************************************************************** */

#define HTTP___MAKE_GET_SET(property)                                          \
  fio_str_info_s fio_http_##property(fio_http_s *h) {                          \
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

/** Gets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status(fio_http_s *h) { return h->status; }

/** Sets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status_set(fio_http_s *h, size_t status) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (h->status = status);
}
/* *****************************************************************************
Handler State
***************************************************************************** */

/** Returns true if the HTTP handle's response was sent. */
SFUNC int fio_http_is_finished(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (FIO_HTTP_STATE_STREAMING == (h->state & FIO_HTTP_STATE_FINISHED));
}

/** Returns true if the HTTP handle's response is streaming. */
SFUNC int fio_http_is_streaming(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (FIO_HTTP_STATE_STREAMING == (h->state & FIO_HTTP_STATE_STREAMING));
}

/* *****************************************************************************
Header Data Management
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

#define FIO___HTTP_HEADER_SET_FN(category, name_, headers, add_val)            \
  /** Sets the header information associated with the HTTP handle. */          \
  fio_str_info_s http_##category##_header_##name_(fio_http_s *h,               \
                                                  fio_str_info_s name,         \
                                                  fio_str_info_s value) {      \
    FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");                                  \
    return fio___http_hmap_set2(headers(h), name, value, add_val);             \
  }
FIO___HTTP_HEADER_SET_FN(request, set, HTTP_HDR_REQUEST, -1)
FIO___HTTP_HEADER_SET_FN(request, set_if_missing, HTTP_HDR_REQUEST, 0)
FIO___HTTP_HEADER_SET_FN(request, add, HTTP_HDR_REQUEST, 1)
FIO___HTTP_HEADER_SET_FN(response, set, HTTP_HDR_RESPONSE, -1)
FIO___HTTP_HEADER_SET_FN(response, set_if_missing, HTTP_HDR_RESPONSE, 0)
FIO___HTTP_HEADER_SET_FN(response, add, HTTP_HDR_RESPONSE, 1)
#undef FIO___HTTP_HEADER_SET_FN

fio_str_info_s fio_http_request_header(fio_http_s *h,
                                       fio_str_info_s name,
                                       size_t index) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return fio___http_hmap_get2(HTTP_HDR_REQUEST(h), name, index);
}
fio_str_info_s fio_http_response_header(fio_http_s *h,
                                        fio_str_info_s name,
                                        size_t index) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return fio___http_hmap_get2(HTTP_HDR_RESPONSE(h), name, index);
}

/** Iterates through all headers. A non-zero return will stop iteration. */
size_t fio_http_request_header_each(fio_http_s *h,
                                    int (*callback)(fio_http_s *,
                                                    fio_str_info_s name,
                                                    fio_str_info_s value,
                                                    void *udata),
                                    void *udata) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!callback)
    return fio___http_hmap_count(HTTP_HDR_REQUEST(h));
  fio___http_hmap_each_info_s d = {.h = h,
                                   .callback = callback,
                                   .udata = udata};
  return fio___http_hmap_each(HTTP_HDR_REQUEST(h),
                              http___h_each_task_wrapper,
                              &d,
                              0);
}

/** Iterates through all headers. A non-zero return will stop iteration. */
size_t fio_http_response_header_each(
    fio_http_s *h,
    int (*callback)(fio_http_s *, fio_str_info_s, fio_str_info_s, void *),
    void *udata) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!callback)
    return fio___http_hmap_count(HTTP_HDR_RESPONSE(h));
  fio___http_hmap_each_info_s d = {.h = h,
                                   .callback = callback,
                                   .udata = udata};
  return fio___http_hmap_each(HTTP_HDR_RESPONSE(h),
                              http___h_each_task_wrapper,
                              &d,
                              0);
}

/* *****************************************************************************
Cookies (TODO!)
***************************************************************************** */

/** (Helper) HTTP Cookie Parser */
FIO_IFUNC void fio___http_cookie_parse_cookie(fio_http_s *h, fio_str_info_s s) {
  /* loop and read Cookie: name=value; name2=value2; name3=value3 */
  while (s.len) {
    fio_str_info_s k = {0}, v = {0};
    /* remove white-space */
    while ((s.buf[0] == ' ' || s.buf[0] == '\t') && s.len) {
      ++s.buf;
      --s.len;
    }
    if (!s.len)
      return;
    char *div = (char *)memchr(s.buf, '=', s.len);
    char *end = (char *)memchr(s.buf, ';', s.len);
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
    fio___http_cmap_set_if_missing(h->cookies, k, v);
  }
}

/** (Helper) Parses all HTTP Cookies */
FIO_SFUNC void fio___http_cookie_collect(fio_http_s *h) {
  fio___http_sary_s *header = NULL;
  {
    header = fio___http_hmap_node2val_ptr(
        fio___http_hmap_get_ptr(h->headers, FIO_STR_INFO1((char *)"cookie")));
  }
  if (!header)
    return;
  FIO_ARRAY_EACH(fio___http_sary, header, pos) {
    fio___http_cookie_parse_cookie(h, fio_bstr_info(*pos));
  }
  return;
}

int fio_http_cookie_set___(void); /* IDE Marker */
/* Sets a response cookie. */
SFUNC int fio_http_cookie_set FIO_NOOP(fio_http_s *h,
                                       fio_http_cookie_args_s cookie) {
  FIO_ASSERT_DEBUG(h, "Can't set cookie for NULL HTTP handler!");
  if (!h || (h->state & (FIO_HTTP_STATE_FINISHED | FIO_HTTP_STATE_STREAMING)))
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
  if ((cookie.name_len + cookie.value_len + cookie.domain_len +
       cookie.path_len + 128) > 5119) {
    FIO_LOG_ERROR("cookie data too long!");
  }
  char tmp_buf[5120];
  fio_str_info_s t = FIO_STR_INFO3(tmp_buf, 0, 5119);

#define copy_cookie_ch(ch_var)                                                 \
  if (!invalid_cookie_##ch_var##_char[(uint8_t)cookie.ch_var[tmp]]) {          \
    t.buf[t.len++] = cookie.ch_var[tmp];                                       \
  } else {                                                                     \
    need2warn |= 1;                                                            \
    t.buf[t.len++] = '%';                                                      \
    t.buf[t.len++] = fio_i2c(((uint8_t)cookie.ch_var[tmp] >> 4) & 0x0F);       \
    t.buf[t.len++] = fio_i2c((uint8_t)cookie.ch_var[tmp] & 0x0F);              \
  }                                                                            \
  tmp += 1;                                                                    \
  if (t.capa <= t.len + 3) {                                                   \
    ((t.buf == tmp_buf)                                                        \
         ? FIO_STRING_ALLOC_COPY                                               \
         : FIO_STRING_REALLOC)(&t, fio_string_capa4len(t.len + 3));            \
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
  t.buf[t.len++] = '=';
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
  t.buf[t.len++] = ';';
  t.buf[t.len++] = ' ';

  if (cookie.max_age) {
    fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"Max-Age=", 8),
        FIO_STRING_WRITE_NUM(cookie.max_age),
        FIO_STRING_WRITE_STR2((char *)"; ", 2));
  }

  if (cookie.domain && cookie.domain_len) {
    fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"domain=", 7),
        FIO_STRING_WRITE_STR2((char *)cookie.domain, cookie.domain_len),
        FIO_STRING_WRITE_STR2((char *)"; ", 2));
  }
  if (cookie.path && cookie.path_len) {
    fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"path=", 5),
        FIO_STRING_WRITE_STR2((char *)cookie.path, cookie.path_len),
        FIO_STRING_WRITE_STR2((char *)"; ", 2));
  }
  if (cookie.http_only) {
    fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "HttpOnly; ",
        10);
  }
  if (cookie.secure) {
    fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "secure; ",
        8);
  }
  switch (cookie.same_site) {
  case FIO_HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT: /* fall through */
  default: break;
  case FIO_HTTP_COOKIE_SAME_SITE_NONE:
    fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "SameSite=None;",
        14);
    break;
  case FIO_HTTP_COOKIE_SAME_SITE_LAX:
    fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "SameSite=Lax;",
        13);
    break;
  case FIO_HTTP_COOKIE_SAME_SITE_STRICT:
    fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "SameSite=Strict;",
        16);
    break;
  }
  if (t.buf[t.len - 1] == ' ')
    --t.len;

  /* set the "write" cookie store data */
  fio___http_cmap_set(h->cookies + 1,
                      FIO_STR_INFO2((char *)cookie.name, cookie.name_len),
                      t,
                      NULL);
  /* set the "read" cookie store data */
  fio___http_cmap_set(h->cookies,
                      FIO_STR_INFO2((char *)cookie.name, cookie.name_len),
                      FIO_STR_INFO2((char *)cookie.value, cookie.value_len),
                      NULL);
  if (t.buf != tmp_buf)
    FIO_STRING_FREE2(t);
  return 0;
}

/** Returns a cookie value (either received of newly set), if any. */
SFUNC fio_str_info_s fio_http_cookie(fio_http_s *h,
                                     const char *name,
                                     size_t name_len) {
  if (!(fio_atomic_or(&h->state, FIO_HTTP_STATE_COOKIES_PARSED) &
        FIO_HTTP_STATE_COOKIES_PARSED))
    fio___http_cookie_collect(h);
  fio_str_info_s r =
      fio___http_cmap_get(h->cookies, FIO_STR_INFO2((char *)name, name_len));
  return r;
}

/** Iterates through all cookies. A non-zero return will stop iteration. */
SFUNC size_t fio_http_cookie_each(fio_http_s *h,
                                  int (*callback)(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value,
                                                  void *udata),
                                  void *udata) {
  size_t i = 0;
  FIO_MAP_EACH(fio___http_cmap, h->cookies, pos) {
    ++i;
    if (callback(h, pos.key, pos.value, udata))
      return i;
  }
  return i;
}

/**
 * Iterates through all response set cookies.
 *
 * A non-zero return value from the callback will stop iteration.
 */
SFUNC size_t
fio_http_set_cookie_each(fio_http_s *h,
                         int (*callback)(fio_http_s *h,
                                         fio_str_info_s set_cookie_header,
                                         fio_str_info_s value,
                                         void *udata),
                         void *udata) {
  size_t i = 0;
  fio___http_cmap_s *set_cookies = h->cookies + 1;
  fio_str_info_s header_name = FIO_STR_INFO2((char *)"set-cookie", 10);
  FIO_MAP_EACH(fio___http_cmap, set_cookies, pos) {
    ++i;
    if (callback(h, header_name, pos.value, udata))
      return i;
  }
  return i;
}

/* *****************************************************************************
Body Management - file descriptor (TODO!)
***************************************************************************** */

FIO_SFUNC fio_str_info_s fio___http_body_read_fd(fio_http_s *h, size_t len) {
  h->body.buf = fio_bstr_len_set(h->body.buf, 0);
  h->body.buf = fio_bstr_readfd(h->body.buf, h->body.fd, h->body.pos, len);
  fio_str_info_s r = fio_bstr_info(h->body.buf);
  h->body.pos += r.len;
  return r;
}
FIO_SFUNC fio_str_info_s fio___http_body_read_until_fd(fio_http_s *h,
                                                       char token,
                                                       size_t limit) {
  h->body.buf = fio_bstr_len_set(h->body.buf, 0);
  h->body.buf =
      fio_bstr_getdelim_fd(h->body.buf, h->body.fd, h->body.pos, token, limit);
  fio_str_info_s r = fio_bstr_info(h->body.buf);
  h->body.pos += r.len;
  return r;
}
FIO_SFUNC void fio___http_body_expect_fd(fio_http_s *h, size_t len) {
  (void)h, (void)len;
}
FIO_SFUNC void fio___http_body_write_fd(fio_http_s *h,
                                        const void *data,
                                        size_t len) {
  ssize_t written = fio_fd_write(h->body.fd, data, len);
  if (written > 0)
    h->body.len += written;
}

/* *****************************************************************************
Body Management - buffer
***************************************************************************** */

FIO_SFUNC int fio___http_body___move_buf2fd(fio_http_s *h) {
  h->body.fd = fio_filename_tmp();
  fio_buf_info_s b = fio_bstr_buf(h->body.buf);
  fio_fd_write(h->body.fd, b.buf, b.len);
  return 0 - (h->body.fd == -1);
}
FIO_SFUNC fio_str_info_s fio___http_body_read_buf(fio_http_s *h, size_t len) {
  fio_str_info_s r = FIO_STR_INFO2((h->body.buf + h->body.pos), len);
  h->body.pos += len;
  return r;
}
FIO_SFUNC fio_str_info_s fio___http_body_read_until_buf(fio_http_s *h,
                                                        char token,
                                                        size_t limit) {
  fio_str_info_s r = FIO_STR_INFO2((h->body.buf + h->body.pos), limit);
  char *end = (char *)FIO_MEMCHR(r.buf, token, limit);
  if (end)
    r.len = end - r.buf;
  return r;
}
FIO_SFUNC void fio___http_body_expect_buf(fio_http_s *h, size_t len) {
  if (len + h->body.len > FIO_HTTP_BODY_RAM_LIMIT) {
    fio___http_body___move_buf2fd(h);
    return;
  }
  h->body.buf = fio_bstr_reserve(h->body.buf, len);
}
FIO_SFUNC void fio___http_body_write_buf(fio_http_s *h,
                                         const void *data,
                                         size_t len) {
  if (len + h->body.len > FIO_HTTP_BODY_RAM_LIMIT)
    goto switch_to_fd;
write_to_buf:
  h->body.buf = fio_bstr_write(h->body.buf, data, len);
  h->body.len += len;
  return;
switch_to_fd:
  if (fio___http_body___move_buf2fd(h))
    goto write_to_buf;
  fio___http_body_write_fd(h, data, len);
}

/* *****************************************************************************
Body Management - Public API
***************************************************************************** */

/** Gets the body (payload) length associated with the HTTP handle. */
SFUNC size_t fio_http_body_length(fio_http_s *h) { return h->body.len; }

/** Adjusts the body's reading position. Negative values start at the end. */
SFUNC size_t fio_http_body_seek(fio_http_s *h, ssize_t pos) {
  if (pos < 0) {
    pos += h->body.len;
    if (pos < 0)
      pos = 0;
  }
  if ((size_t)pos >= h->body.len)
    pos = h->body.len;
  h->body.pos = pos;
  return pos;
}

/** Reads up to `length` of data from the body, returns nothing on EOF. */
SFUNC fio_str_info_s fio_http_body_read(fio_http_s *h, size_t length) {
  fio_str_info_s r = {0};
  if (h->body.pos == h->body.len)
    return r;
  if (h->body.pos + length > h->body.len)
    length = h->body.len - h->body.pos;
  r = ((h->body.fd == -1) ? fio___http_body_read_buf
                          : fio___http_body_read_fd)(h, length);
  return r;
}

/**
 * Reads from the body until finding `token`, reaching `limit` or EOF.
 *
 * Note: `limit` is ignored if the
 */
SFUNC fio_str_info_s fio_http_body_read_until(fio_http_s *h,
                                              char token,
                                              size_t limit) {
  fio_str_info_s r = {0};
  if (h->body.pos == h->body.len)
    return r;
  if (!limit || (h->body.pos + limit) > h->body.len)
    limit = h->body.len - h->body.pos;
  r = ((h->body.fd == -1) ? fio___http_body_read_until_buf
                          : fio___http_body_read_until_fd)(h, token, limit);
  return r;
}

/** Allocates a body (payload) of (at least) the `expected_length`. */
SFUNC void fio_http_body_expect(fio_http_s *h, size_t expected_length) {
  ((h->body.fd == -1) ? fio___http_body_expect_buf
                      : fio___http_body_expect_fd)(h, expected_length);
}

/** Writes `data` to the body (payload) associated with the HTTP handle. */
SFUNC void fio_http_body_write(fio_http_s *h, const void *data, size_t len) {
  if (!data || !len)
    return;
  ((h->body.fd == -1) ? fio___http_body_write_buf
                      : fio___http_body_write_fd)(h, data, len);
}

/* *****************************************************************************
A Response Payload
***************************************************************************** */

/** ETag Helper */
FIO_IFUNC int fio___http_response_etag_if_none_match(fio_http_s *h) {
  if (!h->status)
    return 0;
  fio_str_info_s method = fio_keystr_info(&h->method);
  if ((method.len < 3) | (method.len > 4))
    return 0;
  if (!(((method.buf[0] | 32) == 'g') & ((method.buf[1] | 32) == 'e') &
        ((method.buf[2] | 32) == 't')) &&
      !(((method.buf[0] | 32) == 'h') & ((method.buf[1] | 32) == 'e') &
        ((method.buf[2] | 32) == 'a') & ((method.buf[3] | 32) == 'd')))
    return 0;
  fio_str_info_s etag = fio___http_hmap_get2(HTTP_HDR_RESPONSE(h),
                                             FIO_STR_INFO2((char *)"etag", 4),
                                             0);
  if (!etag.len)
    return 0;
  fio_str_info_s cond =
      fio___http_hmap_get2(HTTP_HDR_REQUEST(h),
                           FIO_STR_INFO2((char *)"if-none-match", 13),
                           0);
  return FIO_STR_INFO_IS_EQ(etag, cond);
}

void fio_http_write___(void); /* IDE Marker */
/**
 * Writes `data` to the response body associated with the HTTP handle after
 * sending all headers (no further headers may be sent).
 */
SFUNC void fio_http_write FIO_NOOP(fio_http_s *h, fio_http_write_args_s args) {
  fio_http_controller_s *c;
  fio___http_hmap_s *hdrs;
  if (!h || (fio_http_is_finished(h) | (!h->controller)))
    goto handle_error;
  c = h->controller;
  hdrs = h->headers + (!!h->status);
  if (!(h->state & FIO_HTTP_STATE_STREAMING)) { /* first call to http_write */
    /* if response has an `etag` header matching `if-none-match`, skip */
    if (fio___http_response_etag_if_none_match(h)) {
      h->status = 304;
      if (args.fd)
        close(args.fd);
      if (args.dealloc && args.data)
        args.dealloc((void *)args.data);
      args.len = args.fd = 0;
      args.data = NULL;
      args.finish = 1;
    }
    /* test if streaming / single body response */
    if (args.finish) {
      /* validate / set Content-Length (not streaming) */
      char ibuf[32];
      fio_str_info_s k = FIO_STR_INFO2((char *)"content-length", 14);
      fio_str_info_s v = FIO_STR_INFO3(ibuf, 0, 32);
      fio_string_write_u(&v, NULL, args.len);
      fio___http_hmap_set2(hdrs, k, v, -1);
    } else {
      h->state |= FIO_HTTP_STATE_STREAMING;
    }
    /* validate Date header */
    fio___http_hmap_set2(hdrs,
                         FIO_STR_INFO2((char *)"date", 4),
                         fio_http_date(fio_http_get_timestump()),
                         0);

    /* start a response, unless status == 0 (which starts a request). */
    (&c->start_response)[h->status == 0](h, h->status, !args.finish);
    c->send_headers(h);
  }
  if (args.data || args.fd) {
    c->write_body(h, args);
    h->sent += args.len;
  }
  if (args.finish) {
    h->state |= FIO_HTTP_STATE_FINISHED;
    c->on_finish(h);
  }
  return;

handle_error:
  if (args.fd)
    close(args.fd);
  if (args.dealloc && args.data)
    args.dealloc((void *)args.data);
}

/* *****************************************************************************
WebSocket / SSE Helpers
***************************************************************************** */

/* *****************************************************************************


                                TODO WIP Marker!!!


***************************************************************************** */

/** Returns non-zero if request headers ask for a WebSockets Upgrade.*/
SFUNC int fio_http_websockets_requested(fio_http_s *h);

/** Sets response data to agree to a WebSockets Upgrade.*/
SFUNC int fio_http_websockets_set_response(fio_http_s *h);

/** Sets request data to request a WebSockets Upgrade.*/
SFUNC void fio_http_websockets_set_request(fio_http_s *h);

/** Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_requested(fio_http_s *h);

/** Sets response data to agree to an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_response(fio_http_s *h);

/** Sets request data to request an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_request(fio_http_s *h);

/* *****************************************************************************


                                TODO WIP Marker!!!


***************************************************************************** */

/* *****************************************************************************
HTTP Logging
***************************************************************************** */

/** Logs an HTTP (response) to STDOUT. */
SFUNC void fio_http_write_log(fio_http_s *h, fio_buf_info_s peer_addr) {
  char buf_mem[1024];
  fio_str_info_s buf = FIO_STR_INFO3(buf_mem, 0, 1023);
  intptr_t bytes_sent = h->sent;
  uint64_t milli_start, milli_end;
  milli_start = h->received_at;
  milli_end = fio_http_get_timestump();
  fio_str_info_s date = fio_http_date(milli_end);

  { /* try to gather address from request headers */
    /* TODO Guess IP address from headers (forwarded) where possible */
    /* if we failed */
    fio_str_info_s forwarded =
        fio_http_request_header(h, FIO_STR_INFO2((char *)"forwarded", 9), -1);
    if (forwarded.len) {
      forwarded.len &= 1023; /* limit possible attack surface */
      for (; forwarded.len > 5;) {
        if ((forwarded.buf[0] | 32) != 'f' || (forwarded.buf[1] | 32) != 'o' ||
            (forwarded.buf[2] | 32) != 'r' || forwarded.buf[3] != '=') {
          ++forwarded.buf;
          --forwarded.len;
          continue;
        }
        forwarded.buf += 4 + (forwarded.buf[4] == '"');
        char *end = forwarded.buf;
        while (*end && *end != '"' && *end != ',' && *end != ' ' &&
               *end != ';' && (end - forwarded.buf) < 48)
          ++end;
        buf.len = (size_t)(end - forwarded.buf);
        if (buf.len)
          memcpy(buf.buf, forwarded.buf, buf.len);
        break;
      }
    }
    if (!buf.len) {
      if (peer_addr.len) {
        memcpy(buf.buf, peer_addr.buf, peer_addr.len);
        buf.len = peer_addr.len;
      } else {
        memcpy(buf.buf, "[unknown]", 9);
        buf.len = 9;
      }
    }
  }
  memcpy(buf.buf + buf.len, " - - [", 6);
  memcpy(buf.buf + 6, date.buf, date.len);
  buf.len += date.len + 6;
  fio_string_write2(&buf,
                    NULL,
                    FIO_STRING_WRITE_STR2((const char *)"] \"", 3),
                    FIO_STRING_WRITE_STR_INFO(fio_keystr_info(&h->method)),
                    FIO_STRING_WRITE_STR2((const char *)" ", 1),
                    FIO_STRING_WRITE_STR_INFO(fio_keystr_info(&h->path)),
                    FIO_STRING_WRITE_STR2((const char *)" ", 1),
                    FIO_STRING_WRITE_STR_INFO(fio_keystr_info(&h->version)),
                    FIO_STRING_WRITE_STR2((const char *)"\" ", 2),
                    FIO_STRING_WRITE_NUM(h->status),
                    FIO_STRING_WRITE_STR2(" ", 1),
                    ((bytes_sent > 0)
                         ? (FIO_STRING_WRITE_UNUM(bytes_sent))
                         : (FIO_STRING_WRITE_STR2((const char *)"---", 3))),
                    FIO_STRING_WRITE_STR2((const char *)" ", 1),
                    FIO_STRING_WRITE_NUM((milli_end - milli_start)),
                    FIO_STRING_WRITE_STR2((const char *)"ms\r\n", 4));

  if (buf.buf[buf.len - 1] != '\n')
    buf.buf[buf.len++] = '\n'; /* log was truncated, data too long */

  fwrite(buf.buf, 1, buf.len, stdout);
}

/* *****************************************************************************
Status Strings
***************************************************************************** */

/** Returns a human readable string related to the HTTP status number. */
SFUNC fio_str_info_s fio_http_status2str(size_t status) {
  fio_str_info_s r = {0};
#define HTTP_RETURN_STATUS(str)                                                \
  do {                                                                         \
    r.len = strlen(str);                                                       \
    r.buf = (char *)str;                                                       \
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
HTTP Handle Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, http)(void) {
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
