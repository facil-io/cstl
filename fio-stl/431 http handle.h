/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP_HANDLE        /* Development inclusion - ignore line */
#define FIO_STR                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                      An HTTP connection Handle helper

See also:
https://www.rfc-editor.org/rfc/rfc9110.html



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP_HANDLE) && !defined(H___FIO_HTTP_HANDLE___H) &&           \
    !defined(FIO___RECURSIVE_INCLUDE)
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
#ifndef H___FIO_SERVER___H
#undef FIO_HTTP_EXACT_LOGGING
#define FIO_HTTP_EXACT_LOGGING 1
#endif
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
/** Each of the HTTP String Caches will be limited to this String count. */
#define FIO_HTTP_CACHE_LIMIT 0 /* ((1UL << 6) + (1UL << 5)) */
#endif

#ifndef FIO_HTTP_CACHE_STR_MAX_LEN
/** The HTTP handle will avoid caching strings longer than this value. */
#define FIO_HTTP_CACHE_STR_MAX_LEN (1 << 12)
#endif

#ifndef FIO_HTTP_CACHE_USES_MUTEX
/** The HTTP cache will use a mutex to allow headers to be set concurrently. */
#define FIO_HTTP_CACHE_USES_MUTEX 1
#endif

#ifndef FIO_HTTP_CACHE_STATIC_HEADERS
/** Adds a static cache for common HTTP header names. */
#define FIO_HTTP_CACHE_STATIC_HEADERS 1
#endif

#ifndef FIO_HTTP_DEFAULT_INDEX_FILENAME
/** The default file name when a static file response points to a folder. */
#define FIO_HTTP_DEFAULT_INDEX_FILENAME "index"
#endif

#ifndef FIO_HTTP_STATIC_FILE_COMPLETION
/** Attempts to auto-complete static file paths with missing extensions. */
#define FIO_HTTP_STATIC_FILE_COMPLETION 1
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

/** Creates a copy of an existing handle, copying only its request data. */
SFUNC fio_http_s *fio_http_new_copy_request(fio_http_s *old);

/** Reduces an fio_http_s handle's reference count or frees it. */
SFUNC void fio_http_free(fio_http_s *);

/** Increases an fio_http_s handle's reference count. */
SFUNC fio_http_s *fio_http_dup(fio_http_s *);

/** Destroyed the HTTP handle object, freeing all allocated resources. */
SFUNC fio_http_s *fio_http_destroy(fio_http_s *h);

/** Collects an updated timestamp for logging purposes. */
SFUNC void fio_http_start_time_set(fio_http_s *);

/* *****************************************************************************
Opaque User and Controller Data
***************************************************************************** */

/** Gets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata(fio_http_s *);

/** Sets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata_set(fio_http_s *, void *);

/** Gets the second opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata2(fio_http_s *);

/** Sets a second opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata2_set(fio_http_s *, void *);

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

/** Gets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status(fio_http_s *);

/** Sets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status_set(fio_http_s *, size_t status);

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
  fio_str_info_s name;
  /** The cookie's value (leave blank to delete cookie). */
  fio_str_info_s value;
  /** The cookie's domain (optional). */
  fio_str_info_s domain;
  /** The cookie's path (optional). */
  fio_str_info_s path;
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

/** Returns true if the HTTP connection was (or should have been) upgraded. */
SFUNC int fio_http_is_upgraded(fio_http_s *h);

/** Returns true if the HTTP handle establishes a WebSocket Upgrade. */
SFUNC int fio_http_is_websocket(fio_http_s *);

/** Returns true if the HTTP handle establishes an EventSource connection. */
SFUNC int fio_http_is_sse(fio_http_s *);

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
  const void *buf;
  /** The length of the data to be written. */
  size_t len;
  /** The offset at which writing should begin. */
  size_t offset;
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

/** Named arguments helper. See fio_http_write and fio_http_write_args_s. */
#define fio_http_write(http_handle, ...)                                       \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)

/** Closes a persistent HTTP connection (i.s., if upgraded). */
SFUNC void fio_http_close(fio_http_s *h);

/* *****************************************************************************
WebSocket / SSE Helpers
***************************************************************************** */

/** Returns non-zero if request headers ask for a WebSockets Upgrade.*/
SFUNC int fio_http_websockets_requested(fio_http_s *);

/** Sets response data to agree to a WebSockets Upgrade.*/
SFUNC void fio_http_upgrade_websockets(fio_http_s *);

/** Sets request data to request a WebSockets Upgrade.*/
SFUNC void fio_http_websockets_set_request(fio_http_s *);

/** Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.*/
SFUNC int fio_http_sse_requested(fio_http_s *);

/** Sets response data to agree to an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_upgrade_sse(fio_http_s *);

/** Sets request data to request an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_request(fio_http_s *);

/* *****************************************************************************
MIME File Type Helpers - NOT thread safe!
***************************************************************************** */

/** Registers a Mime-Type to be associated with the file extension. */
SFUNC int fio_http_mimetype_register(char *file_ext,
                                     size_t file_ext_len,
                                     fio_str_info_s mime_type);

/** Finds the Mime-Type associated with the file extension (if registered). */
SFUNC fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len);

/* *****************************************************************************
HTTP Body Parsing Helpers (TODO!)
***************************************************************************** */

/* *****************************************************************************
Header Parsing Helpers
***************************************************************************** */

/**
 * Copies all header data, from possibly an array of identical response headers,
 * resulting in a parsed format outputted to `buf_parsed`.
 *
 * Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough
 * for the parsed output).
 *
 * Note that the parsed output isn't readable as a string, but is designed to
 * work with the `FIO_HTTP_PARSED_HEADER_EACH` and
 * `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.
 *
 * See also `fio_http_response_header_parse`.
 */
SFUNC int fio_http_response_header_parse(fio_http_s *h,
                                         fio_str_info_s *buf_parsed,
                                         fio_str_info_s header_name);

/**
 * Copies all header data, from possibly an array of identical response headers,
 * resulting in a parsed format outputted to `buf_parsed`.
 *
 * Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough
 * for the parsed output).
 *
 * Note that the parsed output isn't readable as a string, but is designed to
 * work with the `FIO_HTTP_PARSED_HEADER_EACH` and
 * `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.
 *
 * i.e.:
 *
 * ```c
 *  FIO_STR_INFO_TMP_VAR(buf, 1023); // tmp buffer for the parsed output
 *  fio_http_s *h = fio_http_new();  // using a mock HTTP handle
 *  fio_http_request_header_add(
 *      h,
 *      FIO_STR_INFO2("accept", 6),
 *      FIO_STR_INFO1("text/html, application/json;q=0.9; d=500, image/png"));
 *  fio_http_request_header_add(h,
 *                              FIO_STR_INFO2("accept", 6),
 *                              FIO_STR_INFO1("text/yaml"));
 *  FIO_ASSERT(  // in production do NOT assert, but route to error instead!
 *      !fio_http_request_header_parse(h, &buf, FIO_STR_INFO2("accept", 6)),
 *      "parse returned error!");
 *  FIO_HTTP_PARSED_HEADER_EACH(buf, value) {
 *    printf("* processing value (%zu bytes): %s\n", value.len, value.buf);
 *    FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(value, prop) {
 *      printf("* for value %s: (%zu,%zu bytes) %s = %s\n",
 *             value.buf,
 *             prop.name.len,
 *             prop.value.len,
 *             prop.name.buf,
 *             prop.value.buf);
 *    }
 *  }
 * ```
 */
SFUNC int fio_http_request_header_parse(fio_http_s *h,
                                        fio_str_info_s *buf_parsed,
                                        fio_str_info_s header_name);

/**
 * Parses header for multiple values and properties and iterates over all
 * values.
 *
 * This MACRO will allocate 2048 bytes on the stack for parsing the header
 * values and properties, if more space is necessary dig deeper.
 *
 * Use FIO_HTTP_HEADER_VALUE_EACH_PROPERTY to iterate over a value's properties.
 */
#define FIO_HTTP_HEADER_EACH_VALUE(/* fio_http_s */ http_handle,               \
                                   /* int / bool */ is_request,                \
                                   /* fio_str_info_s */ header_name,           \
                                   /* chosen var named */ value)               \
  for (char fio___buf__##value##__[2048], /* allocate buffer on stack */       \
           *fio___buf__##value##_ptr = NULL;                                   \
       !fio___buf__##value##_ptr;                                              \
       fio___buf__##value##_ptr = fio___buf__##value##__)                      \
    for (fio_str_info_s fio___buf__##value##__str = /* declare buffer var */   \
         FIO_STR_INFO3(fio___buf__##value##__, 0, 2048);                       \
         fio___buf__##value##__str.buf == fio___buf__##value##__;              \
         fio___buf__##value##__str.buf = fio___buf__##value##__ + 1)           \
      if (!((is_request ? fio_http_request_header_parse                        \
                        : fio_http_response_header_parse)(                     \
              http_handle, /* parse headers */                                 \
              &fio___buf__##value##__str,                                      \
              header_name)))                                                   \
  FIO_HTTP_PARSED_HEADER_EACH(fio___buf__##value##__str, value) /* loop        \
                                                                 */

/** Iterated through the properties associated with a parsed header values. */
#define FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(/* fio_str_info_s   */ value,      \
                                            /* chosen var named */ property)

/** Used internally to iterate over a parsed header buffer. */
#define FIO_HTTP_PARSED_HEADER_EACH(/* fio_str_info_s   */ buf_parsed,         \
                                    /* chosen var named */ value)

/* *****************************************************************************
General Helpers
***************************************************************************** */

/** Sends the requested error message and finishes the response. */
SFUNC void fio_http_send_error_response(fio_http_s *h, size_t status);

/** Returns true (1) if the ETag response matches an if-none-match request. */
SFUNC int fio_http_etag_is_match(fio_http_s *h);

/**
 * Attempts to send a static file from the `root` folder. On success the
 * response is complete and 0 is returned. Otherwise returns -1.
 */
SFUNC int fio_http_static_file_response(fio_http_s *h,
                                        fio_str_info_s root_folder,
                                        fio_str_info_s file_name,
                                        size_t max_age);

/** Returns a human readable string related to the HTTP status number. */
SFUNC fio_str_info_s fio_http_status2str(size_t status);

/** Logs an HTTP (response) to STDOUT. */
SFUNC void fio_http_write_log(fio_http_s *h, fio_buf_info_s peer_addr);

/* *****************************************************************************
The HTTP Controller
***************************************************************************** */

/**
 * The HTTP Controller manages all the callbacks required by the HTTP Handler in
 * order for HTTP responses and requests to be sent.
 */
struct fio_http_controller_s {
  /* MUST be initialized to zero, used internally by the HTTP Handle. */
  uintptr_t private_flags;
  /** Called when an HTTP handle is freed. */
  void (*on_destroyed)(fio_http_s *h);
  /** Informs the controller that request / response headers must be sent. */
  void (*send_headers)(fio_http_s *h);
  /** called by the HTTP handle for each body chunk (or to finish a response. */
  void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
  /** called once a request / response had finished */
  void (*on_finish)(fio_http_s *h);
  /** called to close an HTTP connection */
  void (*close)(fio_http_s *h);
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

SFUNC fio_http_controller_s *fio___http_controller_validate(
    fio_http_controller_s *c);

/* Create fio_http_udata_(get|set) functions */
FIO___HTTP_GETSET_PTR(void, udata, 0, (void)0)
/* Create fio_http_pdata_(get|set) functions */
FIO___HTTP_GETSET_PTR(void, udata2, 1, (void)0)
/* Create fio_http_cdata_(get|set) functions */
FIO___HTTP_GETSET_PTR(void, cdata, 2, (void)0)
/* Create fio_http_controller_(get|set) functions */
FIO___HTTP_GETSET_PTR(fio_http_controller_s,
                      controller,
                      3,
                      ptr = fio___http_controller_validate(ptr))

#undef FIO___HTTP_GETSET_PTR
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
Header Parsing Helpers - inlined helpers
***************************************************************************** */

#define FIO___HTTP_PARSED_HEADER_VALUE              0
#define FIO___HTTP_PARSED_HEADER_PROPERTY_BLOCK_LEN 1
#define FIO___HTTP_PARSED_HEADER_PROPERTY_DATA      2

typedef struct {
  fio_str_info_s name;
  fio_str_info_s value;
} fio___http_header_property_s;

/**
 * Assumes a Buffer of bytes containing length info and string data as such:
 *   [ NUL byte - 1 byte at head of format ]
 *   repeat
 *   [ 2 byte info: (type | (len << 2)) ]
 *   [ Optional 2 byte info: (len << 2) (if type was 1)]
 *   [ String of `len` bytes][ NUL byte (not counted in `len`)]
 */

FIO_IFUNC fio_str_info_s fio___http_parsed_headers_next(fio_str_info_s value) {
  for (;;) {
    const size_t coded = (size_t)fio_buf2u16u(value.buf + value.len + 1U);
    if (!coded)
      return (value = (fio_str_info_s){0});
    const size_t block_len = coded >> 2;
    value.buf += value.len + 3;
    value.len = block_len;
    if (!(coded & 3))
      return value;
    value.buf -= 3; /* reposition to read NUL + value rather than text start */
  }
}

FIO_IFUNC fio___http_header_property_s
fio___http_parsed_property_next(fio___http_header_property_s property) {
  for (;;) {
    size_t coded =
        (size_t)fio_buf2u16u(property.value.buf + property.value.len + 1);
    if (!(coded & 3))
      return (property = (fio___http_header_property_s){{0}, {0}});
    if ((coded & 3) == FIO___HTTP_PARSED_HEADER_PROPERTY_BLOCK_LEN) {
      property.value.buf += 2;
      coded = (size_t)fio_buf2u16u(property.value.buf + property.value.len + 1);
    }
    if ((coded & 3) != 2)
      return (property = (fio___http_header_property_s){{0}, {0}});
    coded >>= 2;
    property.name.buf = property.value.buf + property.value.len + 3;
    property.name.len = coded;
    coded = (size_t)fio_buf2u16u(property.name.buf + property.name.len + 1);
    FIO_ASSERT_DEBUG((coded & 3) == 2,
                     "header property value parsing format error");
    property.value.buf = property.name.buf + property.name.len + 3;
    property.value.len = coded >> 2;
    return property;
  }
}

#undef FIO_HTTP_PARSED_HEADER_EACH
#define FIO_HTTP_PARSED_HEADER_EACH(buf_parsed, value)                         \
  for (fio_str_info_s value =                                                  \
           fio___http_parsed_headers_next(FIO_STR_INFO2(buf_parsed.buf, 0));   \
       value.len;                                                              \
       value = fio___http_parsed_headers_next(value))

#undef FIO_HTTP_HEADER_VALUE_EACH_PROPERTY
#define FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(value_, property)                  \
  for (fio___http_header_property_s property =                                 \
           fio___http_parsed_property_next(                                    \
               (fio___http_header_property_s){.value = value_});               \
       property.name.len;                                                      \
       property = fio___http_parsed_property_next(property))

/* *****************************************************************************
HTTP Handle Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Helpers - memory allocation & logging time collection
***************************************************************************** */

FIO___LEAK_COUNTER_DEF(http___keystr_allocator)

FIO_SFUNC void fio___http_keystr_free(void *ptr, size_t len) {
  if (!ptr)
    return;
  FIO___LEAK_COUNTER_ON_FREE(http___keystr_allocator);
  FIO_MEM_FREE_(ptr, len);
  (void)len; /* if unused */
}
FIO_SFUNC void *fio___http_keystr_alloc(size_t capa) {
  FIO___LEAK_COUNTER_ON_ALLOC(http___keystr_allocator);
  return FIO_MEM_REALLOC_(NULL, 0, capa, 0);
}

#if FIO_HTTP_EXACT_LOGGING
#define FIO___HTTP_TIME_DIV  1000000
#define FIO___HTTP_TIME_UNIT "us"
FIO_IFUNC int64_t fio_http_get_timestump(void) {
  return fio_time2micro(fio_time_real());
}
#else
#define FIO___HTTP_TIME_DIV  1000
#define FIO___HTTP_TIME_UNIT "ms"
int64_t fio_srv_last_tick(void);
FIO_IFUNC int64_t fio_http_get_timestump(void) {
  return (int64_t)fio_srv_last_tick();
}
#endif

FIO_SFUNC fio_str_info_s fio_http_date(uint64_t now_in_seconds) {
  static char date_buf[128];
  static size_t date_len;
  static uint64_t date_buf_val;
  if (date_buf_val == now_in_seconds)
    return FIO_STR_INFO2(date_buf, date_len);
  date_len = fio_time2rfc7231(date_buf, now_in_seconds);
  date_buf[date_len] = 0;
  date_buf_val = now_in_seconds;
  return FIO_STR_INFO2(date_buf, date_len);
}

#define FIO___RECURSIVE_INCLUDE 1
/* *****************************************************************************
String Cache
***************************************************************************** */

#define FIO_MAP_NAME fio___http_str_cache
#define FIO_MAP_LRU  FIO_HTTP_CACHE_LIMIT
#define FIO_MAP_KEY_BSTR
#include FIO_INCLUDE_FILE

static struct {
  fio___http_str_cache_s cache;
  FIO___LOCK_TYPE lock;
} FIO___HTTP_STRING_CACHE[2] = {{.lock = FIO___LOCK_INIT},
                                {.lock = FIO___LOCK_INIT}};
#define FIO___HTTP_STR_CACHE_COOKIE 0
#define FIO___HTTP_STR_CACHE_VALUE  1

#if FIO_HTTP_CACHE_STATIC_HEADERS

#define FIO___HTTP_STATIC_CACHE_MASK       127
#define FIO___HTTP_STATIC_CACHE_FOLD       22
#define FIO___HTTP_STATIC_CACHE_STEP       1
#define FIO___HTTP_STATIC_CACHE_STEP_LIMIT 3

static struct {
  fio___bstr_meta_s meta;
  char str[32];
} FIO___HTTP_STATIC_CACHE[] = {
#define FIO___HTTP_STATIC_CACHE_SET(s)                                         \
  { .meta = {.ref = 3, .len = (uint32_t)(sizeof(s) - 1)}, .str = s }
    FIO___HTTP_STATIC_CACHE_SET("a-im"),
    FIO___HTTP_STATIC_CACHE_SET("accept"),
    FIO___HTTP_STATIC_CACHE_SET("accept-charset"),
    FIO___HTTP_STATIC_CACHE_SET("accept-datetime"),
    FIO___HTTP_STATIC_CACHE_SET("accept-encoding"),
    FIO___HTTP_STATIC_CACHE_SET("accept-language"),
    FIO___HTTP_STATIC_CACHE_SET("accept-ranges"),
    FIO___HTTP_STATIC_CACHE_SET("access-control-allow-origin"),
    FIO___HTTP_STATIC_CACHE_SET("access-control-request-headers"),
    FIO___HTTP_STATIC_CACHE_SET("access-control-request-method"),
    FIO___HTTP_STATIC_CACHE_SET("age"),
    FIO___HTTP_STATIC_CACHE_SET("allow"),
    FIO___HTTP_STATIC_CACHE_SET("authorization"),
    FIO___HTTP_STATIC_CACHE_SET("cache-control"),
    FIO___HTTP_STATIC_CACHE_SET("connection"),
    FIO___HTTP_STATIC_CACHE_SET("content-disposition"),
    FIO___HTTP_STATIC_CACHE_SET("content-encoding"),
    FIO___HTTP_STATIC_CACHE_SET("content-language"),
    FIO___HTTP_STATIC_CACHE_SET("content-length"),
    FIO___HTTP_STATIC_CACHE_SET("content-location"),
    FIO___HTTP_STATIC_CACHE_SET("content-range"),
    FIO___HTTP_STATIC_CACHE_SET("content-type"),
    FIO___HTTP_STATIC_CACHE_SET("cookie"),
    FIO___HTTP_STATIC_CACHE_SET("date"),
    FIO___HTTP_STATIC_CACHE_SET("dnt"),
    FIO___HTTP_STATIC_CACHE_SET("etag"),
    FIO___HTTP_STATIC_CACHE_SET("expect"),
    FIO___HTTP_STATIC_CACHE_SET("expires"),
    FIO___HTTP_STATIC_CACHE_SET("forwarded"),
    FIO___HTTP_STATIC_CACHE_SET("from"),
    FIO___HTTP_STATIC_CACHE_SET("host"),
    FIO___HTTP_STATIC_CACHE_SET("if-match"),
    FIO___HTTP_STATIC_CACHE_SET("if-modified-since"),
    FIO___HTTP_STATIC_CACHE_SET("if-none-match"),
    FIO___HTTP_STATIC_CACHE_SET("if-range"),
    FIO___HTTP_STATIC_CACHE_SET("if-unmodified-since"),
    FIO___HTTP_STATIC_CACHE_SET("last-modified"),
    FIO___HTTP_STATIC_CACHE_SET("link"),
    FIO___HTTP_STATIC_CACHE_SET("location"),
    FIO___HTTP_STATIC_CACHE_SET("max-forwards"),
    FIO___HTTP_STATIC_CACHE_SET("origin"),
    FIO___HTTP_STATIC_CACHE_SET("pragma"),
    FIO___HTTP_STATIC_CACHE_SET("proxy-authenticate"),
    FIO___HTTP_STATIC_CACHE_SET("proxy-authorization"),
    FIO___HTTP_STATIC_CACHE_SET("range"),
    FIO___HTTP_STATIC_CACHE_SET("referer"),
    FIO___HTTP_STATIC_CACHE_SET("refresh"),
    FIO___HTTP_STATIC_CACHE_SET("retry-after"),
    FIO___HTTP_STATIC_CACHE_SET("sec-ch-ua"),
    FIO___HTTP_STATIC_CACHE_SET("sec-ch-ua-mobile"),
    FIO___HTTP_STATIC_CACHE_SET("sec-ch-ua-platform"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-dest"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-mode"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-site"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-user"),
    FIO___HTTP_STATIC_CACHE_SET("server"),
    FIO___HTTP_STATIC_CACHE_SET("set-cookie"),
    FIO___HTTP_STATIC_CACHE_SET("strict-transport-security"),
    FIO___HTTP_STATIC_CACHE_SET("te"),
    FIO___HTTP_STATIC_CACHE_SET("transfer-encoding"),
    FIO___HTTP_STATIC_CACHE_SET("upgrade"),
    FIO___HTTP_STATIC_CACHE_SET("upgrade-insecure-requests"),
    FIO___HTTP_STATIC_CACHE_SET("user-agent"),
    FIO___HTTP_STATIC_CACHE_SET("vary"),
    FIO___HTTP_STATIC_CACHE_SET("via"),
    FIO___HTTP_STATIC_CACHE_SET("warning"),
    FIO___HTTP_STATIC_CACHE_SET("www-authenticate"),
    FIO___HTTP_STATIC_CACHE_SET("x-csrf-token"),
    FIO___HTTP_STATIC_CACHE_SET("x-forwarded-for"),
    FIO___HTTP_STATIC_CACHE_SET("x-forwarded-host"),
    FIO___HTTP_STATIC_CACHE_SET("x-forwarded-proto"),
    FIO___HTTP_STATIC_CACHE_SET("x-requested-with"),
    {{0}}};

static uint16_t FIO___HTTP_STATIC_CACHE_IMAP[FIO___HTTP_STATIC_CACHE_MASK + 1] =
    {0};

static void fio___http_str_cached_init(void) {
  FIO_MEMSET(FIO___HTTP_STATIC_CACHE_IMAP,
             0,
             (FIO___HTTP_STATIC_CACHE_MASK + 1) *
                 sizeof(FIO___HTTP_STATIC_CACHE_IMAP[0]));

  for (size_t i = 0; FIO___HTTP_STATIC_CACHE[i].meta.ref; ++i) {
    uint64_t hash = fio_stable_hash(FIO___HTTP_STATIC_CACHE[i].str,
                                    FIO___HTTP_STATIC_CACHE[i].meta.len,
                                    0); /* use stable hash (change resilient) */
    hash ^= hash >> FIO___HTTP_STATIC_CACHE_FOLD;
    size_t protection = 0;
    while (FIO___HTTP_STATIC_CACHE_IMAP[hash & FIO___HTTP_STATIC_CACHE_MASK]) {
      FIO_ASSERT(
          (FIO___HTTP_STATIC_CACHE_IMAP[hash & FIO___HTTP_STATIC_CACHE_MASK] &
           0xFF80) != (hash & 0xFF80),
          "full collision for HTTP static hash (%zu == %zu!",
          (size_t)(hash & FIO___HTTP_STATIC_CACHE_MASK),
          i);
      hash += hash >> FIO___HTTP_STATIC_CACHE_STEP;
      FIO_ASSERT((protection++) < FIO___HTTP_STATIC_CACHE_STEP_LIMIT,
                 "HTTP static cache collision overflow @ %zu (%s)",
                 i,
                 FIO___HTTP_STATIC_CACHE[i].str);
    }
    FIO___HTTP_STATIC_CACHE_IMAP[hash & FIO___HTTP_STATIC_CACHE_MASK] =
        (hash & 0xFF80) | i;
  }
}

static char *fio___http_str_cached_static(char *str, size_t len) {
  uint64_t hash =
      fio_stable_hash(str, len, 0); /* use stable hash (change resilient) */
  hash ^= hash >> FIO___HTTP_STATIC_CACHE_FOLD;
  for (size_t attempts = 0; attempts < FIO___HTTP_STATIC_CACHE_STEP_LIMIT;
       ++attempts) {
    if ((FIO___HTTP_STATIC_CACHE_IMAP[hash & FIO___HTTP_STATIC_CACHE_MASK] &
         0xFF80) == (hash & 0xFF80))
      break;
    hash += hash >> FIO___HTTP_STATIC_CACHE_STEP;
  }
  size_t pos =
      FIO___HTTP_STATIC_CACHE_IMAP[hash & FIO___HTTP_STATIC_CACHE_MASK] &
      0x007F;
  if (FIO___HTTP_STATIC_CACHE[pos].meta.len == len &&
      !FIO_MEMCMP(str, FIO___HTTP_STATIC_CACHE[pos].str, len)) {
    return FIO___HTTP_STATIC_CACHE[pos].str;
  }
  return NULL;
}

#undef FIO___HTTP_STATIC_CACHE_MASK
#undef FIO___HTTP_STATIC_CACHE_FOLD
#undef FIO___HTTP_STATIC_CACHE_STEP
#undef FIO___HTTP_STATIC_CACHE_STEP_LIMIT
#else
#define fio___http_str_cached_init() (void)0
#endif /* FIO_HTTP_CACHE_STATIC_HEADERS */

FIO_IFUNC char *fio___http_str_cached_inner(size_t group,
                                            uint64_t hash,
                                            fio_str_info_s s) {
#if !FIO_HTTP_CACHE_LIMIT
  return fio_bstr_write(NULL, s.buf, s.len);
#endif
  fio_str_info_s cached;
  hash ^= (uint64_t)(uintptr_t)fio_http_new;
#if FIO_HTTP_CACHE_USES_MUTEX
  FIO___LOCK_LOCK(FIO___HTTP_STRING_CACHE[group].lock);
#endif
  cached =
      fio___http_str_cache_set_if_missing(&FIO___HTTP_STRING_CACHE[group].cache,
                                          hash,
                                          s);
#if FIO_HTTP_CACHE_USES_MUTEX
  FIO___LOCK_UNLOCK(FIO___HTTP_STRING_CACHE[group].lock);
#endif
  return fio_bstr_copy(cached.buf);
}
FIO_IFUNC char *fio___http_str_cached(size_t group, fio_str_info_s s) {
#if !FIO_HTTP_CACHE_LIMIT
  return fio_bstr_write(NULL, s.buf, s.len);
#endif
  if (!s.len)
    return NULL;
  if (s.len > FIO_HTTP_CACHE_STR_MAX_LEN)
    goto avoid_caching;
  return fio___http_str_cached_inner(group, fio_risky_hash(s.buf, s.len, 0), s);
avoid_caching:
  return fio_bstr_write(NULL, s.buf, s.len);
}

FIO_IFUNC char *fio___http_str_cached_with_static(fio_str_info_s s) {
#if FIO_HTTP_CACHE_STATIC_HEADERS
  char *tmp;
  if (!s.len)
    return NULL;
  if (s.len > FIO_HTTP_CACHE_STR_MAX_LEN)
    goto skip_cache_test;
  tmp = fio___http_str_cached_static(s.buf, s.len);
  if (tmp)
    return fio_bstr_copy(tmp);
skip_cache_test:
#endif /* FIO_HTTP_CACHE_STATIC_HEADERS */
  return fio_bstr_write(NULL, s.buf, s.len);
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
  (dest) = fio___http_str_cached_with_static((src))
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
  if (add < 1)
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
    return r;
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

FIO_SFUNC int fio___http_h_each_task_wrapper(fio___http_hmap_each_s *e) {
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

FIO_SFUNC void fio___mock_controller_cb(fio_http_s *h) { (void)h; }
FIO_SFUNC void fio___mock_c_write_body(fio_http_s *h,
                                       fio_http_write_args_s args) {
  if (args.buf) {
    if (args.dealloc)
      args.dealloc((void *)args.buf);
  } else if (args.fd != -1) {
    close(args.fd);
  }
  (void)h;
}

static fio_http_controller_s FIO___MOCK_CONTROLLER = {
    .on_destroyed = fio___mock_controller_cb,
    .send_headers = fio___mock_controller_cb,
    .write_body = fio___mock_c_write_body,
    .on_finish = fio___mock_controller_cb,
    .close = fio___mock_controller_cb,
};

SFUNC fio_http_controller_s *fio___http_controller_validate(
    fio_http_controller_s *c) {
  if (!c)
    c = &FIO___MOCK_CONTROLLER;
  if (c->private_flags)
    return c;
  if (!c->on_destroyed)
    c->on_destroyed = fio___mock_controller_cb;
  if (!c->send_headers)
    c->send_headers = fio___mock_controller_cb;
  if (!c->write_body)
    c->write_body = fio___mock_c_write_body;
  if (!c->on_finish)
    c->on_finish = fio___mock_controller_cb;
  if (!c->close)
    c->close = fio___mock_controller_cb;
  return c;
}

/* *****************************************************************************
HTTP Handle Type
***************************************************************************** */

#define FIO_HTTP_STATE_STREAMING      1
#define FIO_HTTP_STATE_FINISHED       2
#define FIO_HTTP_STATE_UPGRADED       4
#define FIO_HTTP_STATE_WEBSOCKET      8
#define FIO_HTTP_STATE_SSE            16
#define FIO_HTTP_STATE_COOKIES_PARSED 32

FIO_SFUNC int fio____http_write_start(fio_http_s *, fio_http_write_args_s *);
FIO_SFUNC int fio____http_write_cont(fio_http_s *, fio_http_write_args_s *);

struct fio_http_s {
  void *udata;
  void *udata2;
  void *cdata;
  fio_http_controller_s *controller;
  int (*writer)(fio_http_s *, fio_http_write_args_s *);
  int64_t received_at;
  size_t sent;
  uint32_t state;
  uint32_t status;
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

#define FIO_REF_NAME fio_http
#define FIO_REF_INIT(h)                                                        \
  h = (fio_http_s) {                                                           \
    .controller = &FIO___MOCK_CONTROLLER, .writer = fio____http_write_start,   \
    .received_at = fio_http_get_timestump(), .body.fd = -1                     \
  }
#define FIO_REF_DESTROY(h) fio_http_destroy(&(h))
SFUNC fio_http_s *fio_http_destroy(fio_http_s *h) {
  if (!h)
    return h;
  h->controller->on_destroyed(h);

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
  FIO_REF_INIT(*h);
  return h;
}
#include FIO_INCLUDE_FILE

/** Create a new http_s handle. */
SFUNC fio_http_s *fio_http_new(void) { return fio_http_new2(); }

/** Reduces an http_s handle's reference count or frees it. */
SFUNC void fio_http_free(fio_http_s *h) { fio_http_free2(h); }

/** Increases an http_s handle's reference count. */
SFUNC fio_http_s *fio_http_dup(fio_http_s *h) { return fio_http_dup2(h); }

/** Collects an updated timestamp for logging purposes. */
SFUNC void fio_http_start_time_set(fio_http_s *h) {
  h->received_at = fio_http_get_timestump();
}

/** Closes a persistent HTTP connection (i.s., if upgraded). */
SFUNC void fio_http_close(fio_http_s *h) { h->controller->close(h); }

/** Creates a copy of an existing handle, copying only its request data. */
SFUNC fio_http_s *fio_http_new_copy_request(fio_http_s *o) {
  fio_http_s *h = fio_http_new();
  FIO_ASSERT_ALLOC(h);
  fio_http_path_set(h, fio_http_path(o));
  fio_http_method_set(h, fio_http_method(o));
  fio_http_query_set(h, fio_http_query(o));
  fio_http_version_set(h, fio_http_version(o));
  /* copy headers */
  fio___http_hmap_reserve(h->headers, fio___http_hmap_count(o->headers));
  FIO_MAP_EACH(fio___http_hmap, o->headers, i) {
    fio___http_sary_s *a = fio___http_hmap_node2val_ptr(
        fio___http_hmap_set_ptr(h->headers,
                                i.key,
                                (fio___http_sary_s){0},
                                NULL,
                                0));
    FIO_ARRAY_EACH(fio___http_sary, &i.value, v) {
      fio___http_sary_push(a, fio_bstr_copy(*v));
    }
  }
  /* copy cookies */
  FIO_MAP_EACH(fio___http_cmap, o->cookies, i) {
    fio___http_cmap_set(h->cookies, i.key, i.value, NULL);
  }
  return h;
}

#undef FIO___RECURSIVE_INCLUDE
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
SFUNC size_t fio_http_status(fio_http_s *h) { return (size_t)(h->status); }

/** Sets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status_set(fio_http_s *h, size_t status) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  if (status > 1023)
    status = 500;
  if (!status)
    status = 200;
  return (h->status = (uint32_t)status);
}
/* *****************************************************************************
Handler State
***************************************************************************** */

/** Returns true if the HTTP handle's response was sent. */
SFUNC int fio_http_is_finished(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO_HTTP_STATE_FINISHED));
}

/** Returns true if the HTTP handle's response is streaming. */
SFUNC int fio_http_is_streaming(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO_HTTP_STATE_STREAMING));
}

/** Returns true if the HTTP connection was (or should have been) upgraded. */
SFUNC int fio_http_is_upgraded(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO_HTTP_STATE_UPGRADED));
}

/** Returns true if the HTTP handle establishes a WebSocket Upgrade. */
SFUNC int fio_http_is_websocket(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO_HTTP_STATE_WEBSOCKET));
}

/** Returns true if the HTTP handle establishes an EventSource connection. */
SFUNC int fio_http_is_sse(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO_HTTP_STATE_SSE));
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
  fio_str_info_s fio_http_##category##_header_##name_(fio_http_s *h,           \
                                                      fio_str_info_s name,     \
                                                      fio_str_info_s value) {  \
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
  return fio___http_hmap_get2(HTTP_HDR_REQUEST(h), name, (int32_t)index);
}
fio_str_info_s fio_http_response_header(fio_http_s *h,
                                        fio_str_info_s name,
                                        size_t index) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return fio___http_hmap_get2(HTTP_HDR_RESPONSE(h), name, (int32_t)index);
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
                              fio___http_h_each_task_wrapper,
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
                              fio___http_h_each_task_wrapper,
                              &d,
                              0);
}

/* *****************************************************************************
Cookies
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
    char *div = (char *)FIO_MEMCHR(s.buf, '=', s.len);
    char *end = (char *)FIO_MEMCHR(s.buf, ';', s.len);
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
  if ((cookie.name.len + cookie.value.len + cookie.domain.len +
       cookie.path.len + 128) > 5119) {
    FIO_LOG_ERROR("cookie data too long!");
  }
  char tmp_buf[5120];
  fio_str_info_s t = FIO_STR_INFO3(tmp_buf, 0, 5119);

#define fio___http_h_copy_cookie_ch(ch_var)                                    \
  if (!invalid_cookie_##ch_var##_char[(uint8_t)cookie.ch_var.buf[tmp]]) {      \
    t.buf[t.len++] = cookie.ch_var.buf[tmp];                                   \
  } else {                                                                     \
    need2warn |= 1;                                                            \
    t.buf[t.len++] = '%';                                                      \
    t.buf[t.len++] = fio_i2c(((uint8_t)cookie.ch_var.buf[tmp] >> 4) & 0x0F);   \
    t.buf[t.len++] = fio_i2c((uint8_t)cookie.ch_var.buf[tmp] & 0x0F);          \
  }                                                                            \
  tmp += 1;                                                                    \
  if (t.capa <= t.len + 3) {                                                   \
    ((t.buf == tmp_buf)                                                        \
         ? FIO_STRING_ALLOC_COPY                                               \
         : FIO_STRING_REALLOC)(&t, fio_string_capa4len(t.len + 3));            \
  }

  if (cookie.name.buf) {
    size_t tmp = 0;
    if (cookie.name.len) {
      while (tmp < cookie.name.len) {
        fio___http_h_copy_cookie_ch(name);
      }
    } else {
      while (cookie.name.buf[tmp]) {
        fio___http_h_copy_cookie_ch(name);
      }
    }
    if (need2warn && !warn_illegal) {
      warn_illegal |= 1;
      FIO_LOG_WARNING("illegal char 0x%.2x in cookie name (in %s)\n"
                      "         automatic %% encoding applied",
                      cookie.name.buf[tmp],
                      cookie.name.buf);
    }
  }
  t.buf[t.len++] = '=';
  if (cookie.value.buf) {
    size_t tmp = 0;
    if (cookie.value.len) {
      while (tmp < cookie.value.len) {
        fio___http_h_copy_cookie_ch(value);
      }
    } else {
      while (cookie.value.buf[tmp]) {
        fio___http_h_copy_cookie_ch(value);
      }
    }
    if (need2warn && !warn_illegal) {
      warn_illegal |= 1;
      FIO_LOG_WARNING("illegal char 0x%.2x in cookie value (in %s)\n"
                      "         automatic %% encoding applied",
                      cookie.value.buf[tmp],
                      cookie.value.buf);
    }
  } else
    cookie.max_age = -1;
#undef fio___http_h_copy_cookie_ch

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

  if (cookie.domain.buf && cookie.domain.len) {
    fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"domain=", 7),
        FIO_STRING_WRITE_STR2((char *)cookie.domain.buf, cookie.domain.len),
        FIO_STRING_WRITE_STR2((char *)"; ", 2));
  }
  if (cookie.path.buf && cookie.path.len) {
    fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"path=", 5),
        FIO_STRING_WRITE_STR2((char *)cookie.path.buf, cookie.path.len),
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
  fio___http_cmap_set(h->cookies + 1, cookie.name, t, NULL);
  /* set the "read" cookie store data */
  fio___http_cmap_set(h->cookies, cookie.name, cookie.value, NULL);
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
Body Management - file descriptor
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
  if (h->body.fd == -1) {
#if 1
    static int error_printed = 0;
    if (!error_printed) {
      error_printed = 1;
      FIO_LOG_ERROR("fio_http_s couldn't open temporary file! (%d) %s",
                    errno,
                    strerror(errno));
    }
#endif
    return -1;
  }
  fio_buf_info_s b = fio_bstr_buf(h->body.buf);
  if (!b.len)
    return 0;
  ssize_t written = fio_fd_write(h->body.fd, b.buf, b.len);
  if (written == (ssize_t)b.len)
    return 0;
  close(h->body.fd);
  FIO_LOG_ERROR("fio_http_s couldn't transfer data to temporary file "
                "(transferred %zd / %zu)",
                written,
                b.len);
  return (h->body.fd = -1);
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
    r.len = (end - r.buf) + 1;
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
  if (!fio_http_etag_is_match(h))
    return 0;
  h->status = 304;
  fio___http_hmap_set2(HTTP_HDR_RESPONSE(h),
                       FIO_STR_INFO2((char *)"content-length", 14),
                       FIO_STR_INFO2((char *)"0", 1),
                       -1);
  h->controller->send_headers(h);
  h->controller->on_finish(h);
  return -1;
}

FIO_SFUNC int fio____http_write_done(fio_http_s *h,
                                     fio_http_write_args_s *args) {
  return -1;
  (void)h, (void)args;
}

FIO_SFUNC int fio____http_write_upgraded(fio_http_s *h,
                                         fio_http_write_args_s *args) {
  h->controller->write_body(h, *args);
  h->sent += args->len;
  return 0;
}

FIO_SFUNC int fio____http_write_start(fio_http_s *h,
                                      fio_http_write_args_s *args) {
  /* if response has an `etag` header matching `if-none-match`, skip */
  fio___http_hmap_s *hdrs = h->headers + (!!h->status);
  if (h->status && args->len && fio___http_response_etag_if_none_match(h))
    return -1;
  /* test if streaming / single body response */
  if (!fio___http_hmap_get_ptr(hdrs,
                               FIO_STR_INFO2((char *)"content-length", 14))) {
    if (args->finish) {
      /* validate / set Content-Length (not streaming) */
      char ibuf[32];
      fio_str_info_s k = FIO_STR_INFO2((char *)"content-length", 14);
      fio_str_info_s v = FIO_STR_INFO3(ibuf, 0, 32);
      v.len = fio_digits10u(args->len);
      fio_ltoa10u(v.buf, args->len, v.len);
      fio___http_hmap_set2(hdrs, k, v, -1);
    } else {
      h->state |= FIO_HTTP_STATE_STREAMING;
    }
  }
  /* validate Date header */
  fio___http_hmap_set2(
      hdrs,
      FIO_STR_INFO2((char *)"date", 4),
      fio_http_date(fio_http_get_timestump() / FIO___HTTP_TIME_DIV),
      0);

  /* start a response, unless status == 0 (which starts a request). */
  h->controller->send_headers(h);
  return (h->writer = fio____http_write_cont)(h, args);
}
FIO_SFUNC int fio____http_write_cont(fio_http_s *h,
                                     fio_http_write_args_s *args) {
  if (args->buf || args->fd) {
    h->controller->write_body(h, *args);
    h->sent += args->len;
  }
  if (args->finish) {
    h->state |= FIO_HTTP_STATE_FINISHED;
    h->writer = (h->state & FIO_HTTP_STATE_UPGRADED)
                    ? fio____http_write_upgraded
                    : fio____http_write_done;
    h->controller->on_finish(h);
  }
  return 0;
}

void fio_http_write___(void); /* IDE Marker */
/**
 * Writes `data` to the response body associated with the HTTP handle after
 * sending all headers (no further headers may be sent).
 */
SFUNC void fio_http_write FIO_NOOP(fio_http_s *h, fio_http_write_args_s args) {
  if (!h || !h->controller)
    goto handle_error;
  if (h->writer(h, &args))
    goto handle_error;
  return;

handle_error:
  if (args.fd)
    close(args.fd);
  if (args.dealloc && args.buf)
    args.dealloc((void *)args.buf);
}

/* *****************************************************************************
WebSocket / SSE Helpers
***************************************************************************** */

/** Returns non-zero if request headers ask for a WebSockets Upgrade.*/
SFUNC int fio_http_websockets_requested(fio_http_s *h) {
  fio_str_info_s val =
      fio_http_request_header(h, FIO_STR_INFO2((char *)"connection", 10), 0);
  /* test for "Connection: Upgrade" (TODO? allow for multi-value?) */
  if (val.len < 7 ||
      !(((fio_buf2u32u(val.buf) | 0x20202020UL) == fio_buf2u32u("upgr")) ||
        ((fio_buf2u32u(val.buf + 3) | 0x20202020) == fio_buf2u32u("rade"))))
    return 0;
  /* test for "Upgrade: websocket" (TODO? allow for multi-value?) */
  val = fio_http_request_header(h, FIO_STR_INFO2((char *)"upgrade", 7), 0);
  if (val.len < 7 ||
      !(((fio_buf2u64u(val.buf) | 0x2020202020202020ULL) ==
         fio_buf2u64u("websocke")) ||
        ((fio_buf2u32u(val.buf + 5) | 0x20202020UL) == fio_buf2u32u("cket"))))
    return 0;
  val = fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                                0);
  if (val.len != 24)
    return 0;
  return 1;
}

/** Sets response data to agree to a WebSockets Upgrade.*/
SFUNC void fio_http_upgrade_websockets(fio_http_s *h) {
  h->status = 101;
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"connection", 10),
                               FIO_STR_INFO2((char *)"Upgrade", 7));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"upgrade", 7),
                               FIO_STR_INFO2((char *)"websocket", 9));
  /* we ignore client version and force the RFC final version instead */
  fio_http_response_header_set(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-version", 21),
      FIO_STR_INFO2((char *)"13", 2));
  { /* Sec-WebSocket-Accept */
    fio_str_info_s k =
        fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                                0);
    FIO_STR_INFO_TMP_VAR(accept_val, 63);
    if (k.len != 24)
      goto handshake_error;
    fio_string_write(&accept_val, NULL, k.buf, k.len);
    fio_string_write(&accept_val,
                     NULL,
                     "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                     36);
    fio_sha1_s sha = fio_sha1(accept_val.buf, accept_val.len);
    fio_sha1_digest(&sha);
    accept_val.len = 0;
    fio_string_write_base64enc(&accept_val,
                               NULL,
                               fio_sha1_digest(&sha),
                               fio_sha1_len(),
                               0);
    fio_http_response_header_set(
        h,
        FIO_STR_INFO2((char *)"sec-websocket-accept", 20),
        accept_val);
  }
  { /* finish up */
    h->state |= FIO_HTTP_STATE_UPGRADED | FIO_HTTP_STATE_WEBSOCKET;
    fio_http_write_args_s args = {.finish = 1};
    fio_http_write FIO_NOOP(h, args);
  }
  return;
handshake_error:
  fio_http_send_error_response(h, 403);
  return;
}

/** Sets request data to request a WebSockets Upgrade.*/
SFUNC void fio_http_websockets_set_request(fio_http_s *h) {
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"connection", 10),
                              FIO_STR_INFO2((char *)"Upgrade", 7));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"pragma", 6),
                              FIO_STR_INFO2((char *)"no-cache", 8));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"cache-control", 13),
                              FIO_STR_INFO2((char *)"no-cache", 8));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"connection", 10),
                              FIO_STR_INFO2((char *)"keep-alive", 10));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"upgrade", 7),
                              FIO_STR_INFO2((char *)"websocket", 9));
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-version", 21),
      FIO_STR_INFO2((char *)"13", 2));

  {
    uint64_t tmp[2] = {fio_rand64(), fio_rand64()};
    FIO_STR_INFO_TMP_VAR(key, 64);
    fio_string_write_base64enc(&key, NULL, tmp, 16, 0);
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                                FIO_STR_INFO2((char *)"13", 2));
  }
  /* sec-websocket-extensions ? */
  /* send request? */
}

/** Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.*/
SFUNC int fio_http_sse_requested(fio_http_s *h) {
  fio_str_info_s val =
      fio_http_request_header(h, FIO_STR_INFO2((char *)"accept", 6), 0);
  if (val.len == 17 && fio_buf2u64u(val.buf) == fio_buf2u64u("text/eve") &&
      fio_buf2u64u(val.buf + 8) == fio_buf2u64u("nt-strea") &&
      val.buf[16] == 'm') {
    FIO_LOG_DDEBUG("EventSource connection requested.");
    return 1;
  }
  return 0;
}

/** Sets response data to agree to an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_upgrade_sse(fio_http_s *h) {
  if (h->state)
    return;
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-type", 12),
                               FIO_STR_INFO2((char *)"text/event-stream", 17));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"cache-control", 13),
                               FIO_STR_INFO2((char *)"no-store", 8));
  fio___http_hmap_remove(HTTP_HDR_RESPONSE(h),
                         FIO_STR_INFO2((char *)"content-length", 14),
                         NULL);
  h->state |=
      FIO_HTTP_STATE_FINISHED | FIO_HTTP_STATE_UPGRADED | FIO_HTTP_STATE_SSE;
  h->controller->send_headers(h);
  h->writer = fio____http_write_upgraded;
  h->controller->on_finish(h);
}

/** Sets request data to request an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_request(fio_http_s *h) {
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"accept", 6),
                              FIO_STR_INFO2((char *)"text/event-stream", 17));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"connection", 10),
                              FIO_STR_INFO2((char *)"keep-alive", 10));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"cache-control", 13),
                              FIO_STR_INFO2((char *)"no-cache", 8));
  /* TODO: send request? */
}

/* *****************************************************************************
Header Parsing Helpers - Implementation
***************************************************************************** */

/**
 * Assumes a Buffer of bytes containing length info and string data as such:
 *
 *   [ 2 byte info: (type | (len << 2)) ]
 *   [ Optional 2 byte info: (len << 2) (if type was 1)]
 *   [ String of `len` bytes][ NUL byte (not counted in `len`)]
 */

FIO_SFUNC int fio___http_header_parse_properties(fio_str_info_s *dst,
                                                 char *start,
                                                 char *const end) {
  for (;;) {
    char *nxt = (char *)FIO_MEMCHR(start, ';', end - start);
    if (!nxt)
      nxt = end;
    char *eq = (char *)FIO_MEMCHR(start, '=', nxt - start);
    if (!eq)
      eq = nxt;
    /* write value to dst */
    size_t len = eq - start;
    if ((len & (~(size_t)0x3FFF)) | (dst->len + len + 3 > dst->capa))
      return -1; /* too long */
    fio_u2buf16u(dst->buf + dst->len,
                 ((len << 2) | FIO___HTTP_PARSED_HEADER_PROPERTY_DATA));
    dst->len += 2;
    if (len)
      FIO_MEMCPY(dst->buf + dst->len, start, len);
    dst->len += len;
    dst->buf[dst->len++] = 0;

    eq += (eq[0] == '=');
    eq += (eq[0] == ' ' || eq[0] == '\t');
    len = nxt - eq;
    if ((len & (~(size_t)0x3FFF)) | (dst->len + len + 3 > dst->capa))
      return -1; /* too long */
    fio_u2buf16u(dst->buf + dst->len,
                 ((len << 2) | FIO___HTTP_PARSED_HEADER_PROPERTY_DATA));
    dst->len += 2;
    if (len)
      FIO_MEMCPY(dst->buf + dst->len, eq, len);
    dst->len += len;
    dst->buf[dst->len++] = 0;

    if (nxt == end)
      return 0;
    nxt += (*nxt == ';');
    while (*nxt == ' ' || *nxt == '\t')
      ++nxt;
    start = nxt;
  }
  return 0;
}

FIO_IFUNC int fio___http_header_parse(fio___http_hmap_s *map,
                                      fio_str_info_s *dst,
                                      fio_str_info_s header_name) {
  fio___http_sary_s *a =
      fio___http_hmap_node2val_ptr(fio___http_hmap_get_ptr(map, header_name));
  if (!a)
    return -1;
  dst->len = 0;
  if (dst->capa < 3)
    return -1;
  dst->buf[dst->len++] = 0; /* first byte is a pretend NUL */
  FIO_ARRAY_EACH(fio___http_sary, a, pos) {
    fio_buf_info_s i = fio_bstr_buf(*pos);
    if (!i.len)
      continue;
    char *const end = i.buf + i.len;
    char *sep;
    do {
      sep = (char *)FIO_MEMCHR(i.buf, ',', end - i.buf);
      if (!sep)
        sep = end;
      char *prop = (char *)FIO_MEMCHR(i.buf, ';', sep - i.buf);
      if (!prop)
        prop = sep;
      size_t len = prop - i.buf;
      if ((len & (~(size_t)0x3FFF)) | (dst->len + len + 3 > dst->capa))
        return -1; /* too long */
      fio_u2buf16u(dst->buf + dst->len, (len << 2));
      dst->len += 2;
      FIO_MEMCPY(dst->buf + dst->len, i.buf, len);
      dst->len += len;
      dst->buf[dst->len++] = 0;
      if (prop != sep) { /* parse properties */
        ++prop;
        len = sep - prop;
        if ((len & (~(size_t)0x3FFF)) | (dst->len + len + 3 > dst->capa))
          return -1;
        const size_t old_len = dst->len;
        dst->len += 2;
        if (fio___http_header_parse_properties(dst, prop, sep))
          return -1;
        len = dst->len - old_len;
        if ((len & (~(size_t)0x3FFF)) | (dst->len + len + 3 > dst->capa))
          return -1;
        fio_u2buf16u(
            dst->buf + old_len,
            ((len << 2) | FIO___HTTP_PARSED_HEADER_PROPERTY_BLOCK_LEN));
      }
      sep += (*sep == ',');
      while (*sep == ' ' || *sep == '\t')
        ++sep;
      i.buf = sep;
    } while (sep < end);
  }
  if (dst->len + 2 > dst->capa)
    return -1;
  /* last u16 must be zero (end marker) */
  dst->buf[dst->len++] = 0;
  dst->buf[dst->len++] = 0;
  return 0;
}

SFUNC int fio_http_response_header_parse(fio_http_s *h,
                                         fio_str_info_s *buf_parsed,
                                         fio_str_info_s header_name) {
  return fio___http_header_parse(HTTP_HDR_RESPONSE(h), buf_parsed, header_name);
}

SFUNC int fio_http_request_header_parse(fio_http_s *h,
                                        fio_str_info_s *buf_parsed,
                                        fio_str_info_s header_name) {
  return fio___http_header_parse(HTTP_HDR_REQUEST(h), buf_parsed, header_name);
}

/* *****************************************************************************
Error Handling
***************************************************************************** */

/** Sends the requested error message and finishes the response. */
SFUNC void fio_http_send_error_response(fio_http_s *h, size_t status) {
  if (!h || h->writer != fio____http_write_start)
    return;
  if (!status || status > 1000)
    status = 404;
  h->status = (uint32_t)status;
  FIO_STR_INFO_TMP_VAR(filename, 127);
  /* read static error code file */
  fio_string_write2(&filename,
                    NULL,
                    FIO_STRING_WRITE_UNUM(status),
                    FIO_STRING_WRITE_STR2(".html", 5));
  char *body = fio_bstr_readfile(NULL, filename.buf, 0, 0);
  fio_http_write_args_s args = {.buf = body,
                                .len = fio_bstr_len(body),
                                .dealloc = (void (*)(void *))fio_bstr_free,
                                .finish = 1};
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-type", 12),
                               body ? FIO_STR_INFO2((char *)"text/html", 9)
                                    : FIO_STR_INFO2((char *)"text/plain", 10));
  if (!body) { /* write a short error response (plain text fallback) */
    fio_str_info_s status_str = fio_http_status2str(status);
    filename.len = 0;
    fio_string_write2(&filename,
                      NULL,
                      FIO_STRING_WRITE_STR2("Error ", 6),
                      FIO_STRING_WRITE_UNUM(status),
                      FIO_STRING_WRITE_STR2(": ", 2),
                      FIO_STRING_WRITE_STR_INFO(status_str),
                      FIO_STRING_WRITE_STR2(".", 1));
    args.buf = filename.buf;
    args.len = filename.len;
    args.copy = 1;
    args.dealloc = NULL;
  }
  fio_http_write FIO_NOOP(h, args);
}

/* *****************************************************************************
HTTP Logging
***************************************************************************** */

/** Logs an HTTP (response) to STDOUT. */
SFUNC void fio_http_write_log(fio_http_s *h, fio_buf_info_s peer_addr) {
  FIO_STR_INFO_TMP_VAR(buf, 1023);
  intptr_t bytes_sent = h->sent;
  uint64_t time_start, time_end;
  time_start = h->received_at;
  time_end = fio_http_get_timestump();
  fio_str_info_s date = fio_http_date(time_end / FIO___HTTP_TIME_DIV);

  { /* try to gather address from request headers */
    /* Guess IP address from headers (forwarded) where possible */
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
          FIO_MEMCPY(buf.buf, forwarded.buf, buf.len);
        break;
      }
    }
    if (!buf.len) { /* if we failed, use peer_addr */
      if (peer_addr.len) {
        FIO_MEMCPY(buf.buf, peer_addr.buf, peer_addr.len);
        buf.len = peer_addr.len;
      } else {
        FIO_MEMCPY(buf.buf, "[unknown]", 9);
        buf.len = 9;
      }
    }
  }
  FIO_MEMCPY(buf.buf + buf.len, " - - [", 6);
  FIO_MEMCPY(buf.buf + buf.len + 6, date.buf, date.len);
  buf.len += date.len + 6;
  fio_string_write2(
      &buf,
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
      ((bytes_sent > 0) ? (FIO_STRING_WRITE_UNUM(bytes_sent))
                        : (FIO_STRING_WRITE_STR2((const char *)"---", 3))),
      FIO_STRING_WRITE_STR2((const char *)" ", 1),
      FIO_STRING_WRITE_NUM(time_end - time_start),
      FIO_STRING_WRITE_STR2((const char *)(FIO___HTTP_TIME_UNIT "\r\n"), 4));

  if (buf.buf[buf.len - 1] != '\n')
    buf.buf[buf.len++] = '\n'; /* log was truncated, data too long */

  fwrite(buf.buf, 1, buf.len, stdout);
  h->received_at = time_end;
}

/* *****************************************************************************
ETag helper
***************************************************************************** */

/** Returns true (1) if the ETag response matches an if-none-match request. */
SFUNC int fio_http_etag_is_match(fio_http_s *h) {
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
  if (!cond.len)
    return 0;
  char *end = cond.buf + cond.len;
  for (;;) {
    cond.buf += (cond.buf[0] == ',');
    while (cond.buf[0] == ' ')
      ++cond.buf;
    if (cond.buf > end || (size_t)(end - cond.buf) < (size_t)etag.len)
      return 0;
    if (FIO_MEMCMP(cond.buf, etag.buf, etag.len)) {
      cond.buf = (char *)FIO_MEMCHR(cond.buf, ',', end - cond.buf);
      if (!cond.buf)
        return 0;
      continue;
    }
    return 1;
  }
}

/* *****************************************************************************
Param Parsing (TODO! - parse query, parse mime/multipart parse text/json)
***************************************************************************** */

/* *****************************************************************************


                                TODO WIP Marker!!!


***************************************************************************** */

/* *****************************************************************************
Static file helper
***************************************************************************** */

/**
 * Attempts to send a static file from the `root` folder. On success the
 * response is complete and 0 is returned. Otherwise returns -1.
 */
SFUNC int fio_http_static_file_response(fio_http_s *h,
                                        fio_str_info_s rt,
                                        fio_str_info_s fnm,
                                        size_t max_age) {
  int fd = -1;
  /* combine public folder with path to get file name */
  fio_str_info_s mime_type = {0};
  FIO_STR_INFO_TMP_VAR(etag, 31);
  FIO_STR_INFO_TMP_VAR(filename, 4095);
  { /* test for HEAD and OPTIONS requests */
    fio_str_info_s m = fio_keystr_info(&h->method);
    if ((m.len == 7 && (fio_buf2u64u(m.buf) | 0x2020202020202020ULL) ==
                           (fio_buf2u64u("options") | 0x2020202020202020ULL)))
      goto file_not_found;
  }
  rt.len -= ((rt.len > 0) && fnm.buf[0] == '/' &&
             (rt.buf[rt.len - 1] == '/' ||
              rt.buf[rt.len - 1] == FIO_FOLDER_SEPARATOR));
  fio_string_write(&filename, NULL, rt.buf, rt.len);
  fio_string_write_url_dec(&filename, NULL, fnm.buf, fnm.len);
  if (fio_filename_is_unsafe_url(filename.buf))
    goto file_not_found;

  { /* Test for incomplete file name */
    size_t file_type = fio_filename_type(filename.buf);
#if defined(S_IFDIR) && defined(FIO_HTTP_DEFAULT_INDEX_FILENAME)
    if (file_type == S_IFDIR) {
      filename.len -= (filename.buf[filename.len - 1] == '/' ||
                       filename.buf[filename.len - 1] == FIO_FOLDER_SEPARATOR);
#if FIO_HTTP_STATIC_FILE_COMPLETION
      fio_string_write(&filename,
                       NULL,
                       "/" FIO_HTTP_DEFAULT_INDEX_FILENAME,
                       sizeof(FIO_HTTP_DEFAULT_INDEX_FILENAME));
      file_type = 0;
#else
      fio_string_write(
          &filename,
          NULL, /* note that sizeof will count NUL, so we skip 1 char: */
          "/" FIO_HTTP_DEFAULT_INDEX_FILENAME ".html",
          sizeof(FIO_HTTP_DEFAULT_INDEX_FILENAME ".html"));
      file_type = fio_filename_type(filename.buf);
#endif /* FIO_HTTP_STATIC_FILE_COMPLETION */
    }
#endif /* S_IFDIR */
#if FIO_HTTP_STATIC_FILE_COMPLETION
    const fio_buf_info_s extensions[] = {FIO_BUF_INFO1((char *)".html"),
                                         FIO_BUF_INFO1((char *)".htm"),
                                         FIO_BUF_INFO1((char *)".txt"),
                                         FIO_BUF_INFO1((char *)".md"),
                                         FIO_BUF_INFO0};
    const fio_buf_info_s *pext = extensions;
    while (!file_type) {
      fio_string_write(&filename, NULL, pext->buf, pext->len);
      file_type = fio_filename_type(filename.buf);
      if (file_type)
        break;
      filename.len -= pext->len;
      ++pext;
      if (!pext->buf)
        goto file_not_found;
    }
    switch (file_type) {
    case S_IFREG: break;
#ifdef S_IFLNK
    case S_IFLNK: break;
#endif
    default: goto file_not_found;
    }
#else  /* FIO_HTTP_STATIC_FILE_COMPLETION */
    if (!file_type)
      goto file_not_found;
#endif /* FIO_HTTP_STATIC_FILE_COMPLETION */
  }
  {
    /* find mime type if registered */
    char *end = filename.buf + filename.len;
    char *ext = end;
    do {
      --ext;
    } while (ext[0] != '.' && ext[0] != '/');
    if ((ext++)[0] == '.') {
      mime_type = fio_http_mimetype(ext, end - ext);
      if (!mime_type.len)
        FIO_LOG_WARNING("missing mime-type for extension %s (not registered).",
                        ext);
    }
  }
  {
    fio_str_info_s ac =
        fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                0);
    if (!ac.len)
      goto accept_encoding_header_test_done;
    struct {
      char *value;
      fio_buf_info_s ext;
    } options[] = {{(char *)"gzip", FIO_BUF_INFO2((char *)".gz", 3)},
                   {(char *)"br", FIO_BUF_INFO2((char *)".br", 3)},
                   {(char *)"deflate", FIO_BUF_INFO2((char *)".zip", 4)},
                   {NULL}};
    for (size_t i = 0; options[i].value; ++i) {
      if (!strstr(ac.buf, options[i].value))
        continue;
      fio_string_write(&filename, NULL, options[i].ext.buf, options[i].ext.len);
      if (!fio_filename_type(filename.buf)) {
        filename.len -= options[i].ext.len;
        filename.buf[filename.len] = 0;
        continue;
      }
      fio_http_response_header_set(
          h,
          FIO_STR_INFO2((char *)"vary", 4),
          FIO_STR_INFO2((char *)"accept-encoding", 15));
      fio_http_response_header_set(
          h,
          FIO_STR_INFO2((char *)"content-encoding", 16),
          FIO_STR_INFO1(options[i].value));
      break;
    }
  }

accept_encoding_header_test_done:
  /* attempt to open file */
  fd = fio_filename_open(filename.buf, O_RDONLY);
  if (fd == -1)
    goto file_not_found;

  { /* test / validate etag */
    struct stat stt;
    if (fstat(fd, &stt))
      goto file_not_found;
    uint64_t etag_hash = fio_risky_hash(&stt, sizeof(stt), 0);
    fio_string_write_hex(&etag, NULL, etag_hash);
    fio_http_response_header_set(h, FIO_STR_INFO2((char *)"etag", 4), etag);
    filename.len = 0;
    filename.len = fio_time2rfc7231(filename.buf, stt.st_mtime);
    fio_http_response_header_set(h,
                                 FIO_STR_INFO1((char *)"last-modified"),
                                 filename);
    if (max_age) {
      filename.len = 0;
      fio_string_write2(&filename,
                        NULL,
                        FIO_STRING_WRITE_STR2("max-age=", 8),
                        FIO_STRING_WRITE_UNUM(max_age));
      fio_http_response_header_set(h,
                                   FIO_STR_INFO1((char *)"cache-control"),
                                   filename);
    }
    filename.len = stt.st_size;
    filename.capa = 0;
    if (fio___http_response_etag_if_none_match(h))
      return 0;
  }
  /* Note: at this point filename.len holds the length of the file */

  /* test for range requests. */
  {
    /* test / validate range requests */
    fio_str_info_s rng =
        fio_http_request_header(h, FIO_STR_INFO2((char *)"range", 5), 0);
    if (!rng.len)
      goto range_request_review_finished;
    {
      fio_str_info_s ifrng =
          fio_http_request_header(h, FIO_STR_INFO2((char *)"if-range", 8), 0);
      if (ifrng.len && !FIO_STR_INFO_IS_EQ(ifrng, etag))
        goto range_request_review_finished;
    }
    if (rng.len < 7 || fio_buf2u32u(rng.buf) != fio_buf2u32u("byte") ||
        fio_buf2u16u(rng.buf + 4) != fio_buf2u16u("s="))
      goto range_request_review_finished;
    const size_t file_length = filename.len;
    char *ipos = rng.buf + 6;
    size_t start_range = fio_atol10u(&ipos);
    if (ipos == rng.buf + 6)
      start_range = (size_t)-1;
    if (*ipos != '-')
      goto range_request_review_finished;
    ++ipos;
    size_t end_range = fio_atol10u(&ipos);
    if (end_range > file_length)
      goto range_request_review_finished;
    if (!end_range)
      end_range = file_length - 1;
    if (start_range > end_range) {
      start_range = file_length - end_range;
      end_range = file_length - 1;
    }
    if (!start_range && end_range + 1 == file_length)
      goto range_request_review_finished;
    /* update response headers and info */
    h->status = 206;
    filename.len = 0;
    filename.capa = 1024;
    fio_string_write2(&filename,
                      NULL,
                      FIO_STRING_WRITE_STR2("bytes ", 6),
                      FIO_STRING_WRITE_UNUM(start_range),
                      FIO_STRING_WRITE_STR2("-", 1),
                      FIO_STRING_WRITE_UNUM((end_range)),
                      FIO_STRING_WRITE_STR2("/", 1),
                      FIO_STRING_WRITE_UNUM(file_length));
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-range", 13),
                                 filename);
    filename.len = (end_range - start_range) + 1;
    filename.capa = start_range;
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"etag", 4),
                                 FIO_STR_INFO2(NULL, 0));
  }

range_request_review_finished:
  /* allow interrupted downloads to resume */
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"accept-ranges", 13),
                               FIO_STR_INFO2((char *)"bytes", 5));
  /* test for HEAD requests */
  {
    fio_str_info_s m = fio_keystr_info(&h->method);
    if ((m.len == 4 && (fio_buf2u32u(m.buf) | 0x20202020UL) ==
                           (fio_buf2u32u("head") | 0x20202020UL)))
      goto head_request;
  }

  /* finish up (set mime type and send file) */
  if (mime_type.len)
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-type", 12),
                                 mime_type);
  { /* send response (avoid macro for C++ compatibility) */
    fio_http_write_args_s args = {
        .fd = fd,
        .len = filename.len,     /* now holds body length */
        .offset = filename.capa, /* now holds starting offset */
        .finish = 1};
    fio_http_write FIO_NOOP(h, args);
  }
  return 0;

file_not_found:
  if (fd != -1)
    close(fd);
  return -1;

head_request:
  /* TODO! HEAD responses should close?. */
  if (fd != -1)
    close(fd);
  {
    fio_http_write_args_s args = {.finish = 1};
    fio_http_write FIO_NOOP(h, args);
  }
  return 0;
}

/* *****************************************************************************
Status Strings
***************************************************************************** */

/** Returns a human readable string related to the HTTP status number. */
SFUNC fio_str_info_s fio_http_status2str(size_t status) {
  fio_str_info_s r = {0};
#define HTTP_RETURN_STATUS(str)                                                \
  do {                                                                         \
    r.len = FIO_STRLEN(str);                                                   \
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
  case 418: HTTP_RETURN_STATUS("I am a Teapot"); /* April Fool's Day, 1998 */
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
MIME File Type Helpers
***************************************************************************** */

typedef struct {
  uint64_t ext;
  uint16_t len;
  char mime[118]; /* all together 128 bytes per node */
} fio___http_mime_info_s;

#define FIO___HTTP_MIME_IS_VALID(o) ((o)->ext != 0)
#define FIO___HTTP_MIME_CMP(a, b)   ((a)->ext == (b)->ext)
#define FIO___HTTP_MIME_HASH(o)     fio_risky_num(((o)->ext), 0)

#undef FIO_TYPEDEF_IMAP_REALLOC
#define FIO_TYPEDEF_IMAP_REALLOC(ptr, old_size, new_size, copy_len)            \
  realloc(ptr, new_size)
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE 0
#undef FIO_TYPEDEF_IMAP_FREE
#define FIO_TYPEDEF_IMAP_FREE(ptr, len) free(ptr)

FIO_TYPEDEF_IMAP_ARRAY(fio___http_mime_map,
                       fio___http_mime_info_s,
                       uint32_t,
                       FIO___HTTP_MIME_HASH,
                       FIO___HTTP_MIME_CMP,
                       FIO___HTTP_MIME_IS_VALID)

static fio___http_mime_map_s FIO___HTTP_MIMETYPES;
#undef FIO___HTTP_MIME_IS_VALID
#undef FIO___HTTP_MIME_CMP
#undef FIO___HTTP_MIME_HASH

#undef FIO_TYPEDEF_IMAP_REALLOC
#define FIO_TYPEDEF_IMAP_REALLOC FIO_MEM_REALLOC
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE FIO_MEM_REALLOC_IS_SAFE
#undef FIO_TYPEDEF_IMAP_FREE
#define FIO_TYPEDEF_IMAP_FREE FIO_MEM_FREE

/** Registers a Mime-Type to be associated with the file extension. */
SFUNC int fio_http_mimetype_register(char *file_ext,
                                     size_t file_ext_len,
                                     fio_str_info_s mime_type) {
  fio___http_mime_info_s tmp, *old;
  if (file_ext_len > 7 || mime_type.len > 117)
    return -1;
  tmp.ext = 0;
  FIO_MEMCPY(&tmp.ext, file_ext, file_ext_len);
  if (!mime_type.len)
    goto remove_mime;
  FIO_MEMCPY(&tmp.mime, mime_type.buf, mime_type.len);
  tmp.len = mime_type.len;
  tmp.mime[mime_type.len] = 0;
  old = fio___http_mime_map_get(&FIO___HTTP_MIMETYPES, tmp);
  if (old && old->len == tmp.len && !FIO_MEMCMP(old->mime, tmp.mime, tmp.len)) {
    FIO_LOG_WARNING("mime-type collision: %.*s was %s, now %s",
                    (int)file_ext_len,
                    file_ext,
                    old->mime,
                    tmp.mime);
  }
  fio___http_mime_map_set(&FIO___HTTP_MIMETYPES, tmp, 1);
  return 0;

remove_mime:
  return fio___http_mime_map_remove(&FIO___HTTP_MIMETYPES, tmp);
}

/** Finds the Mime-Type associated with the file extension (if registered). */
SFUNC fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len) {
  fio_str_info_s r = {0};
  fio___http_mime_info_s tmp, *val;
  tmp.ext = 0;
  FIO_MEMCPY(&tmp.ext, file_ext, file_ext_len);
  val = fio___http_mime_map_get(&FIO___HTTP_MIMETYPES, tmp);
  if (!val)
    return r;
  r.len = val->len;
  r.buf = val->mime;
  return r;
}

#define REGISTER_MIME(ext, type)                                               \
  fio_http_mimetype_register((char *)ext,                                      \
                             sizeof(ext) - 1,                                  \
                             FIO_STR_INFO2((char *)type, sizeof(type) - 1))

/** Registers known mime-types that aren't often used by Web Servers. */
FIO_SFUNC void fio_http_mime_register_essential(void) {
  /* clang-format off */
  REGISTER_MIME("3ds", "image/x-3ds");
  REGISTER_MIME("3g2", "video/3gpp");
  REGISTER_MIME("3gp", "video/3gpp");
  REGISTER_MIME("7z", "application/x-7z-compressed");
  REGISTER_MIME("aac", "audio/aac");
  REGISTER_MIME("abw", "application/x-abiword");
  REGISTER_MIME("aif", "audio/x-aiff");
  REGISTER_MIME("aifc", "audio/x-aiff");
  REGISTER_MIME("aiff", "audio/x-aiff");
  REGISTER_MIME("arc", "application/x-freearc");
  REGISTER_MIME("atom", "application/atom+xml");
  REGISTER_MIME("avi", "video/x-msvideo");
  REGISTER_MIME("avif", "image/avif");
  REGISTER_MIME("azw", "application/vnd.amazon.ebook");
  REGISTER_MIME("bin", "application/octet-stream");
  REGISTER_MIME("bmp", "image/bmp");
  REGISTER_MIME("bz", "application/x-bzip");
  REGISTER_MIME("bz2", "application/x-bzip2");
  REGISTER_MIME("cda", "application/x-cdf");
  REGISTER_MIME("csh", "application/x-csh");
  REGISTER_MIME("css", "text/css");
  REGISTER_MIME("csv", "text/csv");
  REGISTER_MIME("dmg", "application/x-apple-diskimage");
  REGISTER_MIME("doc", "application/msword");
  REGISTER_MIME("docx", "application/" "vnd.openxmlformats-officedocument.wordprocessingml.document");
  REGISTER_MIME("eot", "application/vnd.ms-fontobject");
  REGISTER_MIME("epub", "application/epub+zip");
  REGISTER_MIME("gif", "image/gif");
  REGISTER_MIME("gz", "application/gzip");
  REGISTER_MIME("htm", "text/html");
  REGISTER_MIME("html", "text/html");
  REGISTER_MIME("ico", "image/vnd.microsoft.icon");
  REGISTER_MIME("ics", "text/calendar");
  REGISTER_MIME("iso", "application/x-iso9660-image");
  REGISTER_MIME("jar", "application/java-archive");
  REGISTER_MIME("jpe", "image/jpeg");
  REGISTER_MIME("jpeg", "image/jpeg");
  REGISTER_MIME("jpg", "image/jpeg");
  REGISTER_MIME("jpgm", "video/jpm");
  REGISTER_MIME("jpgv", "video/jpeg");
  REGISTER_MIME("jpm", "video/jpm");
  REGISTER_MIME("js", "application/javascript");
  REGISTER_MIME("json", "application/json");
  REGISTER_MIME("jsonld", "application/ld+json");
  REGISTER_MIME("jsonml", "application/jsonml+json");
  REGISTER_MIME("md", "text/markdown");  
  REGISTER_MIME("mid", "audio/midi");
  REGISTER_MIME("midi", "audio/midi");
  REGISTER_MIME("mjs", "text/javascript");
  REGISTER_MIME("mp3", "audio/mpeg");
  REGISTER_MIME("mp4", "video/mp4");
  REGISTER_MIME("m4v", "video/mp4");
  REGISTER_MIME("mpeg", "video/mpeg");
  REGISTER_MIME("mpkg", "application/vnd.apple.installer+xml");
  REGISTER_MIME("odp", "application/vnd.oasis.opendocument.presentation");
  REGISTER_MIME("ods", "application/vnd.oasis.opendocument.spreadsheet");
  REGISTER_MIME("odt", "application/vnd.oasis.opendocument.text");
  REGISTER_MIME("oga", "audio/ogg");
  REGISTER_MIME("ogv", "video/ogg");
  REGISTER_MIME("ogx", "application/ogg");
  REGISTER_MIME("opus", "audio/opus");
  REGISTER_MIME("otf", "font/otf");
  REGISTER_MIME("pdf", "application/pdf");
  REGISTER_MIME("php", "application/x-httpd-php");
  REGISTER_MIME("png", "image/png");
  REGISTER_MIME("ppt", "application/vnd.ms-powerpoint");
  REGISTER_MIME("pptx","application/""vnd.openxmlformats-officedocument.presentationml.presentation");
  REGISTER_MIME("rar", "application/vnd.rar");
  REGISTER_MIME("rtf", "application/rtf");
  REGISTER_MIME("sh", "application/x-sh");
  REGISTER_MIME("svg", "image/svg+xml");
  REGISTER_MIME("svgz", "image/svg+xml");
  REGISTER_MIME("tar", "application/x-tar");
  REGISTER_MIME("tif", "image/tiff");
  REGISTER_MIME("tiff", "image/tiff");
  REGISTER_MIME("ts", "video/mp2t");
  REGISTER_MIME("ttf", "font/ttf");
  REGISTER_MIME("txt", "text/plain");
  REGISTER_MIME("vsd", "application/vnd.visio");
  REGISTER_MIME("wav", "audio/wav");
  REGISTER_MIME("weba", "audio/webm");
  REGISTER_MIME("webm", "video/webm");
  REGISTER_MIME("webp", "image/webp");
  REGISTER_MIME("woff", "font/woff");
  REGISTER_MIME("woff2", "font/woff2");
  REGISTER_MIME("xhtml", "application/xhtml+xml");
  REGISTER_MIME("xls", "application/vnd.ms-excel");
  REGISTER_MIME("xlsx","application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
  REGISTER_MIME("xml", "application/xml");
  REGISTER_MIME("xul", "application/vnd.mozilla.xul+xml");
  REGISTER_MIME("zip", "application/zip");
  /* clang-format on */
}

#undef REGISTER_MIME

/* *****************************************************************************
Constructor / Destructor
***************************************************************************** */

FIO_SFUNC void fio___http_cleanup(void *ignr_) {
  (void)ignr_;
#if FIO_HTTP_CACHE_LIMIT
  for (size_t i = 0; i < 2; ++i) {
    const char *names[] = {"cookie names", "header values"};
    FIO_LOG_DEBUG2(
        "freeing %zu strings from %s cache (capacity was: %zu)",
        fio___http_str_cache_count(&FIO___HTTP_STRING_CACHE[i].cache),
        names[i],
        fio___http_str_cache_capa(&FIO___HTTP_STRING_CACHE[i].cache));
#ifdef FIO_LOG_LEVEL_DEBUG
    if (FIO_LOG_LEVEL_DEBUG == FIO_LOG_LEVEL) {
      FIO_MAP_EACH(fio___http_str_cache,
                   (&FIO___HTTP_STRING_CACHE[i].cache),
                   pos) {
        fprintf(stderr, "\t \"%s\" (%zu bytes)\n", pos.key.buf, pos.key.len);
      }
    }
#endif
    fio___http_str_cache_destroy(&FIO___HTTP_STRING_CACHE[i].cache);
    FIO___LOCK_DESTROY(FIO___HTTP_STRING_CACHE[i].lock);
    (void)names; /* if unused */
  }
#endif /* FIO_HTTP_CACHE_LIMIT */
  FIO_LOG_DEBUG2("HTTP MIME hash storage count/capa: %zu / %zu",
                 FIO___HTTP_MIMETYPES.count,
                 fio___http_mime_map_capa(&FIO___HTTP_MIMETYPES));
  fio___http_mime_map_destroy(&FIO___HTTP_MIMETYPES);
}

FIO_CONSTRUCTOR(fio___http_str_cache_static_builder) {
  fio___http_str_cached_init();
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___http_cleanup, NULL);
  fio_http_mime_register_essential();
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#undef FIO___HTTP_TIME_DIV
#undef FIO___HTTP_TIME_UNIT

#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_HTTP_HANDLE
#endif /* FIO_HTTP_HANDLE */
