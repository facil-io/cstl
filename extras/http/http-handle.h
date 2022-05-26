#ifndef H_HTTP_HANDLE_H
/*
Copyright: Boaz Segev, 2016-2022
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
*/
#define H_HTTP_HANDLE_H

#include <fio-stl.h>

/* support C++ */
#ifdef __cplusplus
extern "C" {
#endif

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

#ifndef HTTP_MAX_HEADER_LENGTH
/** the default maximum length for a single header line */
#define HTTP_MAX_HEADER_LENGTH 8192
#endif

/** An opaque HTTP handle type. */
typedef struct http_s http_s;

/** Named arguments for the http_write function. */
typedef struct http_write_args_s http_write_args_s;
/* *****************************************************************************
HTTP Controller Type
***************************************************************************** */

/**
 * The HTTP Controller manages all the callbacks required by the HTTP Handler in
 * order for HTTP responses and requests to be sent.
 */
typedef struct http_controller_s {
  /* MUST be initialized to zero, used internally by the HTTP Handle. */
  uintptr_t private_flags;
  /** Called before an HTTP handler link to an HTTP Controller is revoked. */
  void (*on_unlinked)(http_s *h, void *cdata);
  /** Informs the controller that a response is starting. */
  int (*start_response)(http_s *h, int status, int will_stream);
  /** Informs the controller that a request is starting. */
  int (*start_request)(http_s *h, int reserved, int will_stream);
  /** Informs the controller that all headers must be sent. */
  void (*send_headers)(http_s *h);
  /** called by the HTTP handle for each body chunk (or to finish a response. */
  void (*write_body)(http_s *h, http_write_args_s args);
  /** called once a request / response had finished */
  void (*on_finish)(http_s *h);
} http_controller_s;

/* *****************************************************************************
Constructor / Destructor
***************************************************************************** */

/** Create a new http_s handle. */
http_s *http_new(void);

/** Reduces an http_s handle's reference count or frees it. */
void http_free(http_s *);

/** Increases an http_s handle's reference count. */
http_s *http_dup(http_s *);

/* *****************************************************************************
Linking to a controller
***************************************************************************** */

/** Gets the HTTP Controller associated with the HTTP handle. */
http_controller_s *http_controller_get(http_s *h);

/** Returns the `void *` pointer set with `http_controller_set`. */
void *http_controller_data(http_s *h);

/** Sets the HTTP Controller, calling the `on_link` callback as required. */
void http_controller_set(http_s *h, http_controller_s *c, void *cdata);

/* *****************************************************************************
Opaque User data
***************************************************************************** */

/** Gets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *http_udata_get(http_s *);

/** Sets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *http_udata_set(http_s *, void *);

/* *****************************************************************************
Data associated with the Request (usually set by the HTTP protocol)
***************************************************************************** */

/** Gets the method information associated with the HTTP handle. */
fio_str_info_s http_method_get(http_s *);

/** Sets the method information associated with the HTTP handle. */
fio_str_info_s http_method_set(http_s *, fio_str_info_s);

/** Gets the path information associated with the HTTP handle. */
fio_str_info_s http_path_get(http_s *);

/** Sets the path information associated with the HTTP handle. */
fio_str_info_s http_path_set(http_s *, fio_str_info_s);

/** Gets the query information associated with the HTTP handle. */
fio_str_info_s http_query_get(http_s *);

/** Sets the query information associated with the HTTP handle. */
fio_str_info_s http_query_set(http_s *, fio_str_info_s);

/** Gets the version information associated with the HTTP handle. */
fio_str_info_s http_version_get(http_s *);

/** Sets the version information associated with the HTTP handle. */
fio_str_info_s http_version_set(http_s *, fio_str_info_s);

/**
 * Gets the header information associated with the HTTP handle.
 *
 * Since more than a single value may be associated with a header name, the
 * index may be used to collect subsequent values.
 *
 * An empty value is returned if no header value is available (or index is
 * exceeded).
 */
fio_str_info_s http_request_header_get(http_s *,
                                       fio_str_info_s name,
                                       size_t index);

/** Sets the header information associated with the HTTP handle. */
fio_str_info_s http_request_header_set(http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);

/** Sets the header information associated with the HTTP handle. */
fio_str_info_s http_request_header_set_if_missing(http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);

/** Adds to the header information associated with the HTTP handle. */
fio_str_info_s http_request_header_add(http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);

/**
 * Iterates through all request headers (except cookies!).
 *
 * A non-zero return will stop iteration.
 * */
size_t http_request_header_each(http_s *,
                                int (*callback)(http_s *,
                                                fio_str_info_s name,
                                                fio_str_info_s value,
                                                void *udata),
                                void *udata);

/** Gets the body (payload) length associated with the HTTP handle. */
size_t http_body_length(http_s *);

/** Adjusts the body's reading position. Negative values start at the end. */
size_t http_body_seek(http_s *, ssize_t pos);

/** Reads up to `length` of data from the body, returns nothing on EOF. */
fio_str_info_s http_body_read(http_s *, size_t length);

/**
 * Reads from the body until finding `token`, reaching `limit` or EOF.
 *
 * Note: `limit` is ignored if the
 */
fio_str_info_s http_body_read_until(http_s *, char token, size_t limit);

/** Allocates a body (payload) of (at least) the `expected_length`. */
void http_body_expect(http_s *, size_t expected_length);

/** Writes `data` to the body (payload) associated with the HTTP handle. */
void http_body_write(http_s *, const void *data, size_t len);

/* *****************************************************************************
Cookies
***************************************************************************** */

/**
 * This is a helper for setting cookie data.
 *
 * This struct is used together with the `http_cookie_set` macro. i.e.:
 *
 *       http_set_cookie(h,
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
} http_cookie_args_s;

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
int http_cookie_set(http_s *h, http_cookie_args_s);

#ifndef __cplusplus
/** Named arguments helper. See http_cookie_args_s for details. */
#define http_cookie_set(http___handle, ...)                                    \
  http_cookie_set((http___handle), (http_cookie_args_s){__VA_ARGS__})
#endif

/** Returns a cookie value (either received of newly set), if any. */
fio_str_info_s http_cookie_get(http_s *, const char *name, size_t name_len);

/** Iterates through all cookies. A non-zero return will stop iteration. */
size_t http_cookie_each(http_s *,
                        int (*callback)(http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value,
                                        void *udata),
                        void *udata);

/**
 * Iterates through all response set cookies.
 * A non-zero return will stop iteration.
 */
size_t http_set_cookie_each(http_s *h,
                            int (*callback)(http_s *,
                                            fio_str_info_s set_cookie_header,
                                            fio_str_info_s value,
                                            void *udata),
                            void *udata);

/* *****************************************************************************
Responding to an HTTP event.
***************************************************************************** */

/** Returns true if the HTTP handle's response was sent. */
int http_is_finished(http_s *);

/** Returns true if the HTTP handle's response is streaming. */
int http_is_streaming(http_s *);

/** Gets the status associated with the HTTP handle (response). */
size_t http_status_get(http_s *);

/** Sets the status associated with the HTTP handle (response). */
size_t http_status_set(http_s *, size_t status);

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
fio_str_info_s http_response_header_get(http_s *,
                                        fio_str_info_s name,
                                        size_t index);

/**
 * Sets the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
fio_str_info_s http_response_header_set(http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);
/**
 * Sets the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
fio_str_info_s http_response_header_set_if_missing(http_s *,
                                                   fio_str_info_s name,
                                                   fio_str_info_s value);

/**
 * Adds to the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
fio_str_info_s http_response_header_add(http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);

/**
 * Iterates through all response headers (except cookies!).
 *
 * A non-zero return will stop iteration.
 * */
size_t http_response_header_each(http_s *,
                                 int (*callback)(http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value,
                                                 void *udata),
                                 void *udata);

/** Arguments for the http_write function. */
struct http_write_args_s {
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
};

/**
 * Writes `data` to the response body associated with the HTTP handle after
 * sending all headers (no further headers may be sent).
 */
void http_write(http_s *, http_write_args_s args);

#ifndef __cplusplus
/** Named arguments helper. See http_write and http_write_args_s for details. */
#define http_write(http_handle, ...)                                           \
  http_write(http_handle, (http_write_args_s){__VA_ARGS__})
#define http_finish(http_handle) http_write(http_handle, .finish = 1)
#endif

/* *****************************************************************************
General Helpers
***************************************************************************** */

/** Returns a human readable string related to the HTTP status number. */
fio_str_info_s http_status2str(size_t status);

/** Logs an HTTP (response) to STDOUT. */
void http_write_log(http_s *h, fio_buf_info_s peer_addr);

/* *****************************************************************************
Testing
***************************************************************************** */

#ifdef TEST
void http_test(void);
#else
#define http_test() ((void)0)
#endif

/* support C++ */
#ifdef __cplusplus
}
#endif

/* *****************************************************************************
Inline implementations
***************************************************************************** */

/** Used internally to set / get the `udata` at its known pointer index. */
FIO_IFUNC void *http___gX(http_s *h, int i) { return ((void **)h)[i]; }

/** Used internally to set / get the `udata` at its known pointer index. */
FIO_IFUNC void *http___sX(http_s *h, int i, void *ptr) {
  return (((void **)h)[i] = ptr);
}

/** Gets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *http_udata_get(http_s *h) { return http___gX(h, 0); }

/** Sets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *http_udata_set(http_s *h, void *u) {
  return http___sX(h, 0, u);
}

#endif /* H_HTTP_HANDLE_H */
