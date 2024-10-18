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

/** The type used by the `FIO_URL_QUERY_EACH` iterator macro. */
typedef struct {
  fio_buf_info_s name;
  fio_buf_info_s value;
  fio_buf_info_s private___;
} fio_url_query_each_s;

/** A helper function for the `FIO_URL_QUERY_EACH` macro implementation. */
FIO_SFUNC fio_url_query_each_s fio_url_query_each_next(fio_url_query_each_s);

/** Iterates through each of the query elements. */
#define FIO_URL_QUERY_EACH(query_buf, i)                                       \
  for (fio_url_query_each_s i = fio_url_query_each_next(                       \
           (fio_url_query_each_s){.private___ = (query_buf)});                 \
       i.name.buf;                                                             \
       i = fio_url_query_each_next(i))

/* Return type for  `fio_url_is_tls` */
typedef struct {
  fio_buf_info_s key;
  fio_buf_info_s cert;
  fio_buf_info_s pass;
  bool tls;
} fio_url_tls_info_s;

/**
 * Returns TLS data associated with the URL.
 *
 * This function supports implicit TLS by scheme data for the following possible
 * values:
 *
 * - `wss`   - Secure WebSockets.
 * - `sses`  - Secure SSE (Server Sent Events).
 * - `https` - Secure HTTP.
 * - `tcps`  - Secure TCP/IP.
 * - `tls`   - Secure TCP/IP.
 * - `udps`  - Secure UDP.
 *
 * i.e.:
 *     tls://example.com/
 *     tcps://example.com/
 *     udps://example.com/
 *
 *     wss://example.com/
 *     https://example.com/
 *     sses://example.com/
 *
 * This function also supports explicit TLS by query data for the following
 * possible key-pair values:
 *
 * - `tls`                   - self-signed TLS (unless key / cert are provided).
 * - `tls=true`              - self-signed TLS (unless key / cert are provided).
 * - `tls=<file>`            - key and certificate files (same path, different
 *                             file extensions).
 * - `key=<file/env_data>`   - path or env variable name for the private key.
 * - `cert=<file/env_data>`  - path or env variable name for the public
 *                             certificate.
 *
 * - `pass`                  - password for decrypting key / cert data.
 *
 *i.e.:
 *
 *     tcp://example.com/?tls          (anonymous TLS)
 *     udp://example.com/?tls=true
 *
 *     https://example.com/?tls=key_cert_folder_or_prefix&pass=key_password
 *
 *     https://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password
 *     wss://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password
 *     tcp://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password
 */
SFUNC fio_url_tls_info_s fio_url_is_tls(fio_url_s u);
/* *****************************************************************************
FIO_URL - Implementation (static)
***************************************************************************** */

/** A helper function for the `FIO_URL_QUERY_EACH` macro implementation. */
FIO_SFUNC fio_url_query_each_s fio_url_query_each_next(fio_url_query_each_s i) {
  i.name = i.private___;
  if (!i.name.buf)
    return i;
  char *amp = (char *)FIO_MEMCHR(i.name.buf, '&', i.name.len);
  if (amp) {
    i.name.len = amp - i.name.buf;
    i.private___.len -= i.name.len + 1;
    i.private___.buf += i.name.len + 1;
  } else {
    i.private___ = FIO_BUF_INFO0;
  }
  char *equ = (char *)FIO_MEMCHR(i.name.buf, '=', i.name.len);
  if (equ) {
    i.value.buf = equ + 1;
    i.value.len = (i.name.buf + i.name.len) - i.value.buf;
    i.name.len = equ - i.name.buf;
  } else {
    i.value = FIO_BUF_INFO0;
  }
  return i;
}

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
  [schema://][[user][:password]@][host.com][:port][path][?quary][#target]
  */
  const char *end = url + len;
  const char *pos = url;
  fio_url_s r = {.scheme = {.buf = (char *)url}};
  if (len == 0) {
    goto finish;
  }

  if (*pos == '/') /* start at path */
    goto start_path;

  while (pos < end && *pos && *pos != ':' && *pos != '/' && *pos != '@' &&
         *pos != '#' && *pos != '?')
    ++pos;

  if (pos == end) {
    /* was only host (path starts with '/') */
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    goto finish;
  }

  switch (*pos) {
  case '@':
    /* username@[host] */
    r.user = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    ++pos;
    goto start_host;
  case '/':
    /* host[/path] */
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    goto start_path;
  case '?':
    /* host?[query] */
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    ++pos;
    goto start_query;
  case '#':
    /* host#[target] */
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
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
      r.user = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
      ++pos;
      goto start_password;
    }
    break;
  }

  /* start_username: */
  url = pos;
  while (pos < end && *pos && *pos != ':' && *pos != '/' && *pos != '@' &&
         *pos != '#' && *pos != '?')
    ++pos;

  if (pos >= end) { /* scheme://host */
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    goto finish;
  }

  switch (*pos) {
  case '/':
    /* scheme://host[/path] */
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    goto start_path;
  case '@':
    /* scheme://username@[host]... */
    r.user = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    ++pos;
    goto start_host;
  case '?':
    /* scheme://host[?query] (bad)*/
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    ++pos;
    goto start_query;
  case '#':
    /* scheme://host[#target] (bad)*/
    r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    ++pos;
    goto start_query;
  case ':':
    /* scheme://username:[password]@[host]... OR */
    /* scheme://host:[port][/...] */
    r.user = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    ++pos;
    break;
  }

start_password:
  url = pos;
  while (pos < end && *pos && *pos != '/' && *pos != '@' && *pos != '?')
    ++pos;

  if (pos >= end) {
    /* was host:port */
    r.port = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    r.host = r.user;
    r.user.len = 0;
    goto finish;
  }

  switch (*pos) {
  case '?': /* fall through */
  case '/':
    r.port = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    r.host = r.user;
    r.user.len = 0;
    goto start_path;
  case '@':
    r.password = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
    ++pos;
    break;
  }

start_host:
  url = pos;
  while (pos < end && *pos && *pos != '/' && *pos != ':' && *pos != '#' &&
         *pos != '?')
    ++pos;

  r.host = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
  if (pos >= end) {
    goto finish;
  }
  switch (*pos) {
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
  while (pos < end && *pos && *pos != '/' && *pos != '#' && *pos != '?')
    ++pos;

  r.port = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));

  if (pos >= end) {
    /* scheme://[...@]host:port */
    goto finish;
  }
  switch (*pos) {
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
  while (pos < end && *pos && *pos != '#' && *pos != '?')
    ++pos;

  r.path = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));

  if (pos >= end) {
    goto finish;
  }
  ++pos;
  if (pos[-1] == '#')
    goto start_target;

start_query:
  url = pos;
  while (pos < end && *pos && *pos != '#')
    ++pos;

  r.query = FIO_BUF_INFO2((char *)url, (size_t)(pos - url));
  ++pos;

  if (pos >= end)
    goto finish;

start_target:
  r.target = FIO_BUF_INFO2((char *)pos, (size_t)(end - pos));

finish:

  if (r.scheme.len == 4 && r.host.buf) { /* recognize file paths */
    uint32_t s, file_str, unix_str, priv_str;
    fio_memcpy4(&file_str, "file");
    fio_memcpy4(&unix_str, "unix");
    fio_memcpy4(&priv_str, "priv");
    fio_memcpy4(&s, r.scheme.buf);
    s |= 0x20202020U; /* downcase */
    if (s == file_str || s == unix_str || s == priv_str) {
      r.path.buf = r.scheme.buf + 7;
      r.path.len = end - (r.scheme.buf + 7);
      if (r.query.len)
        r.path.len = r.query.buf - (r.path.buf + 1);
      else if (r.target.len)
        r.path.len = r.target.buf - (r.path.buf + 1);
      r.user.len = r.password.len = r.port.len = r.host.len = 0;
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

/* Returns TLS data associated with the URL. */
SFUNC fio_url_tls_info_s fio_url_is_tls(fio_url_s u) {
  fio_url_tls_info_s r = {0};
  switch (u.scheme.len) {
  case 3: /* wss || tls || ssl */
    r.tls =
        ((fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("wss:") ||
         (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("tls:") ||
         (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("ssl:"));

    break;
  case 4: /* ssse || sses || tcps || stcp || udps || sudp */
    r.tls =
        ((fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("sses") ||
         (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("ssse") ||
         (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("tcps") ||
         (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("stcp") ||
         (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("udps") ||
         (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("sudp"));
    break;
  case 5: /* https */
    r.tls = ((u.scheme.buf[4] | 32) == 's' &&
             (fio_buf2u32u(u.scheme.buf) | 0x20202020) == fio_buf2u32u("http"));
    break;
  }
  if (u.query.len) { /* key=, cert=, pass=, password=*/
    fio_buf_info_s key = {0};
    fio_buf_info_s cert = {0};
    fio_buf_info_s pass = {0};
    uint32_t name;
    const uint32_t wrd_key = fio_buf2u32u("key="); /* keyword's value */
    const uint32_t wrd_tls = fio_buf2u32u("tls=");
    const uint32_t wrd_ssl = fio_buf2u32u("ssl=");
    const uint32_t wrd_cert = fio_buf2u32u("cert");
    const uint32_t wrd_true = fio_buf2u32u("true");
    const uint32_t wrd_pass = fio_buf2u32u("pass");
    const uint64_t wrd_password = fio_buf2u64u("password");
    FIO_URL_QUERY_EACH(u.query, i) { /* iterates each name=value pair */
      switch (i.name.len) {
      case 8:
        if ((fio_buf2u64u(i.name.buf) | 0x2020202020202020ULL) == wrd_password)
          pass = i.value;
        break;
      case 3:
        name = fio_buf2u32u(i.name.buf) | 0x20202020UL; /* '=' stays the same */
        name &= fio_buf2u32u("\xFF\xFF\xFF\x00");
        name |= fio_buf2u32u("\x00\x00\x00="); /* in case there was no = sign */
        if (name == wrd_key) {
          if (i.value.len)
            key = i.value;
        } else if (name == wrd_tls || name == wrd_ssl) {
          r.tls = 1;
          if (i.value.len && !(i.value.len == 1 && i.value.buf[0] == '1') &&
              !(i.value.len == 4 &&
                (fio_buf2u32u(i.value.buf) | 0x20202020UL) == wrd_true))
            cert = key = i.value;
        }
        break;
      case 4:
        name = fio_buf2u32u(i.name.buf) | 0x20202020UL;
        if (name == wrd_cert)
          cert = i.value;
        else if (name == wrd_pass)
          pass = i.value;
        break;
      }
    }
    if (key.len && cert.len) {
      r.key = key;
      r.cert = cert;
      if (pass.len)
        r.pass = pass;
      r.tls = 1;
    }
  }
  FIO_LOG_DDEBUG2(
      "URL TLS detection:\n\t%s\n\tkey: %.*s\n\tcert: %.*s\n\tpass: %.*s",
      (r.tls ? "Secure" : "plaintext"),
      (int)r.key.len,
      r.key.buf,
      (int)r.cert.len,
      r.cert.buf,
      (int)r.pass.len,
      r.pass.buf);
  return r;
}
/* *****************************************************************************
FIO_URL - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_URL || FIO_URI */
#undef FIO_URL
#undef FIO_URI
