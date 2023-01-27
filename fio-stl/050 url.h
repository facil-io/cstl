/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_URL                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                  URI Parsing



Copyright: Boaz Segev, 2019-2021; License: ISC / MIT (choose your license)
***************************************************************************** */
#if (defined(FIO_URL) || defined(FIO_URI)) && !defined(H___FIO_URL___H)
#define H___FIO_URL___H
/** the result returned by `fio_url_parse` */
typedef struct {
  fio_buf_info_s scheme;
  fio_buf_info_s user;
  fio_buf_info_s password;
  fio_buf_info_s host;
  fio_buf_info_s port;
  fio_buf_info_s path;
  fio_buf_info_s query;
  fio_buf_info_s target;
} fio_url_s;

/**
 * Parses the URI returning it's components and their lengths (no decoding
 * performed, doesn't accept decoded URIs).
 *
 * The returned string are NOT NUL terminated, they are merely locations within
 * the original string.
 *
 * This function attempts to accept many different formats, including any of the
 * following:
 *
 * * `/complete_path?query#target`
 *
 *   i.e.: /index.html?page=1#list
 *
 * * `host:port/complete_path?query#target`
 *
 *   i.e.:
 *      example.com
 *      example.com:8080
 *      example.com/index.html
 *      example.com:8080/index.html
 *      example.com:8080/index.html?key=val#target
 *
 * * `user:password@host:port/path?query#target`
 *
 *   i.e.: user:1234@example.com:8080/index.html
 *
 * * `username[:password]@host[:port][...]`
 *
 *   i.e.: john:1234@example.com
 *
 * * `schema://user:password@host:port/path?query#target`
 *
 *   i.e.: http://example.com/index.html?page=1#list
 *
 * Invalid formats might produce unexpected results. No error testing performed.
 *
 * NOTE: the `unix`, `file` and `priv` schemas are reserved for file paths.
 */
SFUNC fio_url_s fio_url_parse(const char *url, size_t len);

/* *****************************************************************************
FIO_URL - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/**
 * Parses the URI returning it's components and their lengths (no decoding
 * performed, doesn't accept decoded URIs).
 *
 * The returned string are NOT NUL terminated, they are merely locations within
 * the original string.
 *
 * This function expects any of the following formats:
 *
 * * `/complete_path?query#target`
 *
 *   i.e.: /index.html?page=1#list
 *
 * * `host:port/complete_path?query#target`
 *
 *   i.e.:
 *      example.com/index.html
 *      example.com:8080/index.html
 *
 * * `schema://user:password@host:port/path?query#target`
 *
 *   i.e.: http://example.com/index.html?page=1#list
 *
 * Invalid formats might produce unexpected results. No error testing performed.
 */
SFUNC fio_url_s fio_url_parse(const char *url, size_t len) {
  /*
  Intention:
  [schema://][user[:]][password[@]][host.com[:/]][:port/][/path][?quary][#target]
  */
  const char *end = url + len;
  const char *pos = url;
  fio_url_s r = {.scheme = {.buf = (char *)url}};
  if (len == 0) {
    goto finish;
  }

  if (pos[0] == '/') {
    /* start at path */
    goto start_path;
  }

  while (pos < end && pos[0] != ':' && pos[0] != '/' && pos[0] != '@' &&
         pos[0] != '#' && pos[0] != '?')
    ++pos;

  if (pos == end) {
    /* was only host (path starts with '/') */
    r.host = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto finish;
  }

  switch (pos[0]) {
  case '@':
    /* username@[host] */
    r.user = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    goto start_host;
  case '/':
    /* host[/path] */
    r.host = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto start_path;
  case '?':
    /* host?[query] */
    r.host = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    goto start_query;
  case '#':
    /* host#[target] */
    r.host = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    goto start_target;
  case ':':
    if (pos + 2 <= end && pos[1] == '/' && pos[2] == '/') {
      /* scheme:// */
      r.scheme.len = pos - url;
      pos += 3;
    } else {
      /* username:[password] OR */
      /* host:[port] */
      r.user = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
      ++pos;
      goto start_password;
    }
    break;
  }

  // start_username:
  url = pos;
  while (pos < end && pos[0] != ':' && pos[0] != '/' && pos[0] != '@'
         /* && pos[0] != '#' && pos[0] != '?' */)
    ++pos;

  if (pos >= end) { /* scheme://host */
    r.host = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto finish;
  }

  switch (pos[0]) {
  case '/':
    /* scheme://host[/path] */
    r.host = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto start_path;
  case '@':
    /* scheme://username@[host]... */
    r.user = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    goto start_host;
  case ':':
    /* scheme://username:[password]@[host]... OR */
    /* scheme://host:[port][/...] */
    r.user = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    break;
  }

start_password:
  url = pos;
  while (pos < end && pos[0] != '/' && pos[0] != '@')
    ++pos;

  if (pos >= end) {
    /* was host:port */
    r.port = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    r.host = r.user;
    r.user.len = 0;
    goto finish;
    ;
  }

  switch (pos[0]) {
  case '/':
    r.port = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    r.host = r.user;
    r.user.len = 0;
    goto start_path;
  case '@':
    r.password =
        (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    break;
  }

start_host:
  url = pos;
  while (pos < end && pos[0] != '/' && pos[0] != ':' && pos[0] != '#' &&
         pos[0] != '?')
    ++pos;

  r.host = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
  if (pos >= end) {
    goto finish;
  }
  switch (pos[0]) {
  case '/':
    /* scheme://[...@]host[/path] */
    goto start_path;
  case '?':
    /* scheme://[...@]host?[query] (bad)*/
    ++pos;
    goto start_query;
  case '#':
    /* scheme://[...@]host#[target] (bad)*/
    ++pos;
    goto start_target;
    // case ':':
    /* scheme://[...@]host:[port] */
  }
  ++pos;

  // start_port:
  url = pos;
  while (pos < end && pos[0] != '/' && pos[0] != '#' && pos[0] != '?')
    ++pos;

  r.port = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};

  if (pos >= end) {
    /* scheme://[...@]host:port */
    goto finish;
  }
  switch (pos[0]) {
  case '?':
    /* scheme://[...@]host:port?[query] (bad)*/
    ++pos;
    goto start_query;
  case '#':
    /* scheme://[...@]host:port#[target] (bad)*/
    ++pos;
    goto start_target;
    // case '/':
    /* scheme://[...@]host:port[/path] */
  }

start_path:
  url = pos;
  while (pos < end && pos[0] != '#' && pos[0] != '?')
    ++pos;

  r.path = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};

  if (pos >= end) {
    goto finish;
  }
  ++pos;
  if (pos[-1] == '#')
    goto start_target;

start_query:
  url = pos;
  while (pos < end && pos[0] != '#')
    ++pos;

  r.query = (fio_buf_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
  ++pos;

  if (pos >= end)
    goto finish;

start_target:
  r.target = (fio_buf_info_s){.buf = (char *)pos, .len = (size_t)(end - pos)};

finish:

  if (r.scheme.len == 4 && r.host.buf) {
    uint32_t s, file_str, unix_str, priv_str;
    fio_memcpy4(&file_str, "file");
    fio_memcpy4(&unix_str, "unix");
    fio_memcpy4(&priv_str, "priv");
    fio_memcpy4(&s, r.scheme.buf);
    s |= 0x20202020U; /* downcase */
    if (s == file_str || s == unix_str || s == priv_str) {
      r.path.len = end - (r.scheme.buf + 7);
      r.path.buf = r.scheme.buf + 7;
      r.user.len = r.password.len = r.port.len = r.host.len = r.query.len =
          r.target.len = 0;
    }
  } else if (!r.scheme.len && r.host.buf && r.host.buf[0] == '.') {
    r.path.len = end - r.host.buf;
    r.path.buf = r.host.buf;
    r.query.len = r.target.len = r.host.len = 0;
  }

  /* set any empty values to NULL */
  if (!r.scheme.len)
    r.scheme.buf = NULL;
  if (!r.user.len)
    r.user.buf = NULL;
  if (!r.password.len)
    r.password.buf = NULL;
  if (!r.host.len)
    r.host.buf = NULL;
  if (!r.port.len)
    r.port.buf = NULL;
  if (!r.path.len)
    r.path.buf = NULL;
  if (!r.query.len)
    r.query.buf = NULL;
  if (!r.target.len)
    r.target.buf = NULL;

  return r;
}

/* *****************************************************************************
URL parsing - Test
***************************************************************************** */
#ifdef FIO_TEST_CSTL

/* Test for URI variations:
 *
 * * `/complete_path?query#target`
 *
 *   i.e.: /index.html?page=1#list
 *
 * * `host:port/complete_path?query#target`
 *
 *   i.e.:
 *      example.com
 *      example.com:8080
 *      example.com/index.html
 *      example.com:8080/index.html
 *      example.com:8080/index.html?key=val#target
 *
 * * `user:password@host:port/path?query#target`
 *
 *   i.e.: user:1234@example.com:8080/index.html
 *
 * * `username[:password]@host[:port][...]`
 *
 *   i.e.: john:1234@example.com
 *
 * * `schema://user:password@host:port/path?query#target`
 *
 *   i.e.: http://example.com/index.html?page=1#list
 */
FIO_SFUNC void FIO_NAME_TEST(stl, url)(void) {
  fprintf(stderr, "* Testing URL (URI) parser.\n");
  struct {
    char *url;
    size_t len;
    fio_url_s expected;
  } tests[] = {
      {
          .url = (char *)"file://go/home/",
          .len = 15,
          .expected =
              {
                  .scheme = {.buf = (char *)"file", .len = 4},
                  .path = {.buf = (char *)"go/home/", .len = 8},
              },
      },
      {
          .url = (char *)"unix:///go/home/",
          .len = 16,
          .expected =
              {
                  .scheme = {.buf = (char *)"unix", .len = 4},
                  .path = {.buf = (char *)"/go/home/", .len = 9},
              },
      },
      {
          .url = (char *)"schema://user:password@host:port/path?query#target",
          .len = 50,
          .expected =
              {
                  .scheme = {.buf = (char *)"schema", .len = 6},
                  .user = {.buf = (char *)"user", .len = 4},
                  .password = {.buf = (char *)"password", .len = 8},
                  .host = {.buf = (char *)"host", .len = 4},
                  .port = {.buf = (char *)"port", .len = 4},
                  .path = {.buf = (char *)"/path", .len = 5},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"schema://user@host:port/path?query#target",
          .len = 41,
          .expected =
              {
                  .scheme = {.buf = (char *)"schema", .len = 6},
                  .user = {.buf = (char *)"user", .len = 4},
                  .host = {.buf = (char *)"host", .len = 4},
                  .port = {.buf = (char *)"port", .len = 4},
                  .path = {.buf = (char *)"/path", .len = 5},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"http://localhost.com:3000/home?is=1",
          .len = 35,
          .expected =
              {
                  .scheme = {.buf = (char *)"http", .len = 4},
                  .host = {.buf = (char *)"localhost.com", .len = 13},
                  .port = {.buf = (char *)"3000", .len = 4},
                  .path = {.buf = (char *)"/home", .len = 5},
                  .query = {.buf = (char *)"is=1", .len = 4},
              },
      },
      {
          .url = (char *)"/complete_path?query#target",
          .len = 27,
          .expected =
              {
                  .path = {.buf = (char *)"/complete_path", .len = 14},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"/index.html?page=1#list",
          .len = 23,
          .expected =
              {
                  .path = {.buf = (char *)"/index.html", .len = 11},
                  .query = {.buf = (char *)"page=1", .len = 6},
                  .target = {.buf = (char *)"list", .len = 4},
              },
      },
      {
          .url = (char *)"example.com",
          .len = 11,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
              },
      },

      {
          .url = (char *)"example.com:8080",
          .len = 16,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
              },
      },
      {
          .url = (char *)"example.com/index.html",
          .len = 22,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"example.com:8080/index.html",
          .len = 27,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"example.com:8080/index.html?key=val#target",
          .len = 42,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
                  .query = {.buf = (char *)"key=val", .len = 7},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"user:1234@example.com:8080/index.html",
          .len = 37,
          .expected =
              {
                  .user = {.buf = (char *)"user", .len = 4},
                  .password = {.buf = (char *)"1234", .len = 4},
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"user@example.com:8080/index.html",
          .len = 32,
          .expected =
              {
                  .user = {.buf = (char *)"user", .len = 4},
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {.url = NULL},
  };
  for (size_t i = 0; tests[i].url; ++i) {
    fio_url_s result = fio_url_parse(tests[i].url, tests[i].len);
    FIO_LOG_DEBUG2("Result for: %s"
                   "\n\t     scheme   (%zu bytes):  %.*s"
                   "\n\t     user     (%zu bytes):  %.*s"
                   "\n\t     password (%zu bytes):  %.*s"
                   "\n\t     host     (%zu bytes):  %.*s"
                   "\n\t     port     (%zu bytes):  %.*s"
                   "\n\t     path     (%zu bytes):  %.*s"
                   "\n\t     query    (%zu bytes):  %.*s"
                   "\n\t     target   (%zu bytes):  %.*s\n",
                   tests[i].url,
                   result.scheme.len,
                   (int)result.scheme.len,
                   result.scheme.buf,
                   result.user.len,
                   (int)result.user.len,
                   result.user.buf,
                   result.password.len,
                   (int)result.password.len,
                   result.password.buf,
                   result.host.len,
                   (int)result.host.len,
                   result.host.buf,
                   result.port.len,
                   (int)result.port.len,
                   result.port.buf,
                   result.path.len,
                   (int)result.path.len,
                   result.path.buf,
                   result.query.len,
                   (int)result.query.len,
                   result.query.buf,
                   result.target.len,
                   (int)result.target.len,
                   result.target.buf);
    FIO_ASSERT(
        result.scheme.len == tests[i].expected.scheme.len &&
            (!result.scheme.len || !memcmp(result.scheme.buf,
                                           tests[i].expected.scheme.buf,
                                           tests[i].expected.scheme.len)),
        "scheme result failed for:\n\ttest[%zu]: %s\n\texpected: "
        "%s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.scheme.buf,
        (int)result.scheme.len,
        result.scheme.buf);
    FIO_ASSERT(
        result.user.len == tests[i].expected.user.len &&
            (!result.user.len || !memcmp(result.user.buf,
                                         tests[i].expected.user.buf,
                                         tests[i].expected.user.len)),
        "user result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.user.buf,
        (int)result.user.len,
        result.user.buf);
    FIO_ASSERT(
        result.password.len == tests[i].expected.password.len &&
            (!result.password.len || !memcmp(result.password.buf,
                                             tests[i].expected.password.buf,
                                             tests[i].expected.password.len)),
        "password result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: "
        "%.*s",
        i,
        tests[i].url,
        tests[i].expected.password.buf,
        (int)result.password.len,
        result.password.buf);
    FIO_ASSERT(
        result.host.len == tests[i].expected.host.len &&
            (!result.host.len || !memcmp(result.host.buf,
                                         tests[i].expected.host.buf,
                                         tests[i].expected.host.len)),
        "host result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.host.buf,
        (int)result.host.len,
        result.host.buf);
    FIO_ASSERT(
        result.port.len == tests[i].expected.port.len &&
            (!result.port.len || !memcmp(result.port.buf,
                                         tests[i].expected.port.buf,
                                         tests[i].expected.port.len)),
        "port result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.port.buf,
        (int)result.port.len,
        result.port.buf);
    FIO_ASSERT(
        result.path.len == tests[i].expected.path.len &&
            (!result.path.len || !memcmp(result.path.buf,
                                         tests[i].expected.path.buf,
                                         tests[i].expected.path.len)),
        "path result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.path.buf,
        (int)result.path.len,
        result.path.buf);
    FIO_ASSERT(result.query.len == tests[i].expected.query.len &&
                   (!result.query.len || !memcmp(result.query.buf,
                                                 tests[i].expected.query.buf,
                                                 tests[i].expected.query.len)),
               "query result failed for:\n\ttest[%zu]: %s\n\texpected: "
               "%s\n\tgot: %.*s",
               i,
               tests[i].url,
               tests[i].expected.query.buf,
               (int)result.query.len,
               result.query.buf);
    FIO_ASSERT(
        result.target.len == tests[i].expected.target.len &&
            (!result.target.len || !memcmp(result.target.buf,
                                           tests[i].expected.target.buf,
                                           tests[i].expected.target.len)),
        "target result failed for:\n\ttest[%zu]: %s\n\texpected: "
        "%s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.target.buf,
        (int)result.target.len,
        result.target.buf);
  }
}
#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
FIO_URL - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_URL || FIO_URI */
#undef FIO_URL
#undef FIO_URI
