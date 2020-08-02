/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************













                                  URI Parsing













***************************************************************************** */
#if (defined(FIO_URL) || defined(FIO_URI)) && !defined(H___FIO_URL___H)
#define H___FIO_URL___H
/** the result returned by `fio_url_parse` */
typedef struct {
  fio_str_info_s scheme;
  fio_str_info_s user;
  fio_str_info_s password;
  fio_str_info_s host;
  fio_str_info_s port;
  fio_str_info_s path;
  fio_str_info_s query;
  fio_str_info_s target;
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
 */
SFUNC fio_url_s fio_url_parse(const char *url, size_t len);

/* *****************************************************************************
FIO_URL - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE)

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
    r.host = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto finish;
  }
  switch (pos[0]) {
  case '@':
    /* username@[host] */
    r.user = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    goto start_host;
  case '/':
    /* host[/path] */
    r.host = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto start_path;
  case '?':
    /* host?[query] */
    r.host = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    goto start_query;
  case '#':
    /* host#[target] */
    r.host = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
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
      r.user = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
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
    r.host = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto finish;
  }

  switch (pos[0]) {
  case '/':
    /* scheme://host[/path] */
    r.host = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    goto start_path;
  case '@':
    /* scheme://username@[host]... */
    r.user = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    goto start_host;
  case ':':
    /* scheme://username:[password]@[host]... OR */
    /* scheme://host:[port][/...] */
    r.user = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    break;
  }

start_password:
  url = pos;
  while (pos < end && pos[0] != '/' && pos[0] != '@')
    ++pos;

  if (pos >= end) {
    /* was host:port */
    r.port = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    r.host = r.user;
    r.user.len = 0;
    goto finish;
    ;
  }

  switch (pos[0]) {
  case '/':
    r.port = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    r.host = r.user;
    r.user.len = 0;
    goto start_path;
  case '@':
    r.password =
        (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
    ++pos;
    break;
  }

start_host:
  url = pos;
  while (pos < end && pos[0] != '/' && pos[0] != ':' && pos[0] != '#' &&
         pos[0] != '?')
    ++pos;

  r.host = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
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

  r.port = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};

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

  r.path = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};

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

  r.query = (fio_str_info_s){.buf = (char *)url, .len = (size_t)(pos - url)};
  ++pos;

  if (pos >= end)
    goto finish;

start_target:
  r.target = (fio_str_info_s){.buf = (char *)pos, .len = (size_t)(end - pos)};

finish:

  if (r.scheme.len == 4 && r.host.buf &&
      (((r.scheme.buf[0] | 32) == 'f' && (r.scheme.buf[1] | 32) == 'i' &&
        (r.scheme.buf[2] | 32) == 'l' && (r.scheme.buf[3] | 32) == 'e') ||
       ((r.scheme.buf[0] | 32) == 'u' && (r.scheme.buf[1] | 32) == 'n' &&
        (r.scheme.buf[2] | 32) == 'i' && (r.scheme.buf[3] | 32) == 'x'))) {
    r.path.len += (r.path.buf - (r.scheme.buf + 7));
    r.path.buf = r.scheme.buf + 7;
    r.user.len = r.password.len = r.port.len = r.host.len = 0;
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
FIO_URL - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_URL
#undef FIO_URI
#endif /* FIO_URL || FIO_URI */
