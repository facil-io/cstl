/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_URL
#include FIO_INCLUDE_FILE

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
int main(void) {
  struct {
    char *url;
    size_t len;
    fio_url_s expected;
    fio_url_tls_info_s tls;
  } tests[] = {
      {
          .url = (char *)"file://go/home/",
          5,
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"file"),
                  .path = FIO_BUF_INFO1((char *)"go/home/"),
              },
      },
      {
          .url = (char *)"unix:///go/home/",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"unix"),
                  .path = FIO_BUF_INFO1((char *)"/go/home/"),
              },
      },
      {
          .url = (char *)"unix:///go/home/?query#target",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"unix"),
                  .path = FIO_BUF_INFO1((char *)"/go/home/"),
                  .query = FIO_BUF_INFO1((char *)"query"),
                  .target = FIO_BUF_INFO1((char *)"target"),
              },
      },
      {
          .url = (char *)"schema://user:password@host:port/path?query#target",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"schema"),
                  .user = FIO_BUF_INFO1((char *)"user"),
                  .password = FIO_BUF_INFO1((char *)"password"),
                  .host = FIO_BUF_INFO1((char *)"host"),
                  .port = FIO_BUF_INFO1((char *)"port"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
                  .query = FIO_BUF_INFO1((char *)"query"),
                  .target = FIO_BUF_INFO1((char *)"target"),
              },
      },
      {
          .url = (char *)"schema://user@host:port/path?query#target",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"schema"),
                  .user = FIO_BUF_INFO1((char *)"user"),
                  .host = FIO_BUF_INFO1((char *)"host"),
                  .port = FIO_BUF_INFO1((char *)"port"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
                  .query = FIO_BUF_INFO1((char *)"query"),
                  .target = FIO_BUF_INFO1((char *)"target"),
              },
      },
      {
          .url = (char *)"http://localhost.com:3000/home?is=1",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"http"),
                  .host = FIO_BUF_INFO1((char *)"localhost.com"),
                  .port = FIO_BUF_INFO1((char *)"3000"),
                  .path = FIO_BUF_INFO1((char *)"/home"),
                  .query = FIO_BUF_INFO1((char *)"is=1"),
              },
      },
      {
          .url = (char *)"/complete_path?query#target",
          .expected =
              {
                  .path = FIO_BUF_INFO1((char *)"/complete_path"),
                  .query = FIO_BUF_INFO1((char *)"query"),
                  .target = FIO_BUF_INFO1((char *)"target"),
              },
      },
      {
          .url = (char *)"/index.html?page=1#list",
          .expected =
              {
                  .path = FIO_BUF_INFO1((char *)"/index.html"),
                  .query = FIO_BUF_INFO1((char *)"page=1"),
                  .target = FIO_BUF_INFO1((char *)"list"),
              },
      },
      {
          .url = (char *)"example.com",
          .expected =
              {
                  .host = FIO_BUF_INFO1((char *)"example.com"),
              },
      },

      {
          .url = (char *)"example.com:8080",
          .expected =
              {
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .port = FIO_BUF_INFO1((char *)"8080"),
              },
      },
      {
          .url = (char *)"example.com:8080?q=true",
          .expected =
              {
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .port = FIO_BUF_INFO1((char *)"8080"),
                  .query = FIO_BUF_INFO1((char *)"q=true"),
              },
      },
      {
          .url = (char *)"example.com/index.html",
          .expected =
              {
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/index.html"),
              },
      },
      {
          .url = (char *)"example.com:8080/index.html",
          .expected =
              {
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .port = FIO_BUF_INFO1((char *)"8080"),
                  .path = FIO_BUF_INFO1((char *)"/index.html"),
              },
      },
      {
          .url = (char *)"example.com:8080/index.html?key=val#target",
          .expected =
              {
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .port = FIO_BUF_INFO1((char *)"8080"),
                  .path = FIO_BUF_INFO1((char *)"/index.html"),
                  .query = FIO_BUF_INFO1((char *)"key=val"),
                  .target = FIO_BUF_INFO1((char *)"target"),
              },
      },
      {
          .url = (char *)"user:1234@example.com:8080/index.html",
          .expected =
              {
                  .user = FIO_BUF_INFO1((char *)"user"),
                  .password = FIO_BUF_INFO1((char *)"1234"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .port = FIO_BUF_INFO1((char *)"8080"),
                  .path = FIO_BUF_INFO1((char *)"/index.html"),
              },
      },
      {
          .url = (char *)"user@example.com:8080/index.html",
          .expected =
              {
                  .user = FIO_BUF_INFO1((char *)"user"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .port = FIO_BUF_INFO1((char *)"8080"),
                  .path = FIO_BUF_INFO1((char *)"/index.html"),
              },
      },
      {
          .url = (char *)"https://example.com/path",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"https"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
              },
          .tls = {.tls = 1},
      },
      {
          .url = (char *)"wss://example.com/path",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"wss"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
              },
          .tls = {.tls = 1},
      },
      {
          .url = (char *)"sses://example.com/path",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"sses"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
              },
          .tls = {.tls = 1},
      },
      {
          .url = (char *)"sses://example.com/path",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"sses"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
              },
          .tls = {.tls = 1},
      },
      {
          .url = (char *)"http://example.com/path?tls=true",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"http"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
                  .query = FIO_BUF_INFO1((char *)"tls=true"),
              },
          .tls = {.tls = 1},
      },
      {
          .url = (char *)"http://example.com/path?tls",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"http"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
                  .query = FIO_BUF_INFO1((char *)"tls"),
              },
          .tls = {.tls = 1},
      },
      {
          .url = (char *)"http://example.com/path?tls=something",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"http"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
                  .query = FIO_BUF_INFO1((char *)"tls=something"),
              },
          .tls =
              {
                  .key = FIO_BUF_INFO1((char *)"something"),
                  .cert = FIO_BUF_INFO1((char *)"something"),
                  .tls = 1,
              },
      },
      {
          .url = (char *)"http://example.com/path?key=something&cert=pubthing",
          .expected =
              {
                  .scheme = FIO_BUF_INFO1((char *)"http"),
                  .host = FIO_BUF_INFO1((char *)"example.com"),
                  .path = FIO_BUF_INFO1((char *)"/path"),
                  .query = FIO_BUF_INFO1((char *)"key=something&cert=pubthing"),
              },
          .tls =
              {
                  .key = FIO_BUF_INFO1((char *)"something"),
                  .cert = FIO_BUF_INFO1((char *)"pubthing"),
                  .tls = 1,
              },
      },
      {.url = NULL},
  };
  for (size_t i = 0; tests[i].url; ++i) {
    tests[i].len = strlen(tests[i].url);
    fio_url_s result = fio_url_parse(tests[i].url, tests[i].len);
    fio_url_tls_info_s tls = fio_url_is_tls(result);
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

    FIO_ASSERT(
        1,
        "TSL detection result failed for:\n\ttest[%zu]: %s\n\texpected: "
        "%s key:%s, cert:%s, pass%s\n\tgot: %s key:%.*s, cert:%.*s, pass%.*s",
        i,
        tests[i].url,
        tests[i].tls.tls ? "TLS" : "none",
        tests[i].tls.key.buf,
        tests[i].tls.cert.buf,
        tests[i].tls.pass.buf,
        tls.tls ? "TLS" : "none",
        (int)tls.key.len,
        tls.key.buf,
        (int)tls.cert.len,
        tls.cert.buf,
        (int)tls.pass.len,
        tls.pass.buf);
  }
}
