#define FIO_HTTP1_PARSER
#include "../fio-stl/include.h"

/* *****************************************************************************















HTTP/1.1 TESTING















***************************************************************************** */
#define FIO_URL
#define FIO_TIME
#include "../fio-stl/include.h"

static size_t http1_test_pos;
static char http1_test_temp_buf[8092];
static size_t http1_test_temp_buf_pos;
static struct {
  char *test_name;
  char *request[16];
  struct {
    char body[1024];
    size_t body_len;
    const char *method;
    ssize_t status;
    const char *path;
    const char *query;
    const char *version;
    struct http1_test_header_s {
      const char *name;
      size_t name_len;
      const char *val;
      size_t val_len;
    } headers[12];
  } result, expect;
} http1_test_data[] = {
    {
        .test_name = "simple empty request",
        .request = {"GET / HTTP/1.1\r\nHost:localhost\r\n\r\n"},
        .expect =
            {
                .body = "",
                .body_len = 0,
                .method = "GET",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{.name = "host",
                             .name_len = 4,
                             .val = "localhost",
                             .val_len = 9}},
            },
    },
    {
        .test_name = "empty request with bad EOL",
        .request = {"GET / HTTP/1.1\nHost:localhost\n\n"},
        .expect =
            {
                .body = "",
                .body_len = 0,
                .method = "GET",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{.name = "host",
                             .name_len = 4,
                             .val = "localhost",
                             .val_len = 9}},
            },
    },
    {
        .test_name = "space before header data",
        .request = {"POST /my/path HTTP/1.2\r\nHost: localhost\r\n\r\n"},
        .expect =
            {
                .body = "",
                .body_len = 0,
                .method = "POST",
                .path = "/my/path",
                .query = NULL,
                .version = "HTTP/1.2",
                .headers = {{.name = "host",
                             .name_len = 4,
                             .val = "localhost",
                             .val_len = 9}},
            },
    },
    {
        .test_name = "simple request, fragmented header (in new line)",
        .request = {"GET / HTTP/1.1\r\n", "Host:localhost\r\n\r\n"},
        .expect =
            {
                .body = "",
                .body_len = 0,
                .method = "GET",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{.name = "host",
                             .name_len = 4,
                             .val = "localhost",
                             .val_len = 9}},
            },
    },
    {
        .test_name = "request with query",
        .request = {"METHOD /path?q=query HTTP/1.3\r\nHost:localhost\r\n\r\n"},
        .expect =
            {
                .body = "",
                .body_len = 0,
                .method = "METHOD",
                .path = "/path",
                .query = "q=query",
                .version = "HTTP/1.3",
                .headers = {{.name = "host",
                             .name_len = 4,
                             .val = "localhost",
                             .val_len = 9}},
            },
    },
    {
        .test_name = "mid-fragmented header",
        .request = {"GET / HTTP/1.1\r\nHost: loca", "lhost\r\n\r\n"},
        .expect =
            {
                .body = "",
                .body_len = 0,
                .method = "GET",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{.name = "host",
                             .name_len = 4,
                             .val = "localhost",
                             .val_len = 9}},
            },
    },
    {
        .test_name = "simple with body",
        .request = {"GET / HTTP/1.1\r\nHost:with body\r\n"
                    "Content-lEnGth: 5\r\n\r\nHello"},
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "GET",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{
                                .name = "host",
                                .name_len = 4,
                                .val = "with body",
                                .val_len = 9,
                            },
                            {
                                .name = "content-length",
                                .name_len = 14,
                                .val = "5",
                                .val_len = 1,
                            }},
            },
    },
    {
        .test_name = "simple with body + Expect",
        .request = {"GET / HTTP/1.1\r\nHost:with body\r\n"
                    "Content-lEnGth: 5\r\nExpect: 100-continue\r\n\r\nHello"},
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "GET",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{
                                .name = "host",
                                .name_len = 4,
                                .val = "with body",
                                .val_len = 9,
                            },
                            {
                                .name = "content-length",
                                .name_len = 14,
                                .val = "5",
                                .val_len = 1,
                            }},
            },
    },
    {
        .test_name = "fragmented body",
        .request = {"GET / HTTP/1.1\r\nHost:with body\r\n",
                    "Content-lEnGth: 5\r\n\r\nHe",
                    "llo"},
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "GET",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{
                                .name = "host",
                                .name_len = 4,
                                .val = "with body",
                                .val_len = 9,
                            },
                            {
                                .name = "content-length",
                                .name_len = 14,
                                .val = "5",
                                .val_len = 1,
                            }},
            },
    },
    {
        .test_name = "fragmented body 2 (cuts EOL)",
        .request = {"POST / HTTP/1.1\r\nHost:with body\r\n",
                    "Content-lEnGth: 5\r\n",
                    "\r\n",
                    "He",
                    "llo"},
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{
                                .name = "host",
                                .name_len = 4,
                                .val = "with body",
                                .val_len = 9,
                            },
                            {
                                .name = "content-length",
                                .name_len = 14,
                                .val = "5",
                                .val_len = 1,
                            }},
            },
    },
    {
        .test_name = "chunked body (simple)",
        .request = {"POST / HTTP/1.1\r\nHost:with body\r\n"
                    "Transfer-Encoding: chunked\r\n"
                    "\r\n"
                    "5\r\n"
                    "Hello"
                    "\r\n0\r\n\r\n"},
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers =
                    {
                        {
                            .name = "host",
                            .name_len = 4,
                            .val = "with body",
                            .val_len = 9,
                        },
                    },
            },
    },
    {
        .test_name = "chunked body (empty)",
        .request = {"POST / HTTP/1.1\r\nHost:with body\r\n"
                    "Transfer-Encoding: chunked\r\n"
                    "\r\n0\r\n\r\n"},
        .expect =
            {
                .body = "",
                .body_len = 0,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers =
                    {
                        {
                            .name = "host",
                            .name_len = 4,
                            .val = "with body",
                            .val_len = 9,
                        },
                    },
            },
    },
    {
        .test_name = "chunked body (end of list)",
        .request = {"POST / HTTP/1.1\r\nHost:with body\r\n"
                    "Transfer-Encoding: gzip, foo, chunked\r\n"
                    "\r\n"
                    "5\r\n"
                    "Hello"
                    "\r\n0\r\n\r\n"},
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers =
                    {
                        {
                            .name = "host",
                            .name_len = 4,
                            .val = "with body",
                            .val_len = 9,
                        },
                        {
                            .name = "transfer-encoding",
                            .name_len = 17,
                            .val = "gzip, foo",
                            .val_len = 9,
                        },
                    },
            },
    },
#if HTTP1_ALLOW_CHUNKED_IN_MIDDLE_OF_HEADER
    {
        .test_name = "chunked body (middle of list - RFC violation)",
        .request = {"POST / HTTP/1.1\r\nHost:with body\r\n"
                    "Transfer-Encoding: gzip, chunked, foo\r\n"
                    "\r\n",
                    "5\r\n"
                    "Hello"
                    "\r\n0\r\n\r\n"},
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers =
                    {
                        {
                            .name = "host",
                            .name_len = 4,
                            .val = "with body",
                            .val_len = 9,
                        },
                        {
                            .name = "transfer-encoding",
                            .name_len = 17,
                            .val = "gzip,foo",
                            .val_len = 8,
                        },
                    },
            },
    },
#endif /* HTTP1_ALLOW_CHUNKED_IN_MIDDLE_OF_HEADER */
    {
        .test_name = "chunked body (fragmented)",
        .request =
            {
                "POST / HTTP/1.1\r\nHost:with body\r\n",
                "Transfer-Encoding: chunked\r\n",
                "\r\n"
                "5\r\n",
                "He",
                "llo",
                "\r\n0\r\n\r\n",
            },
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers =
                    {
                        {
                            .name = "host",
                            .name_len = 4,
                            .val = "with body",
                            .val_len = 9,
                        },
                    },
            },
    },
    {
        .test_name = "chunked body (fragmented + multi-message)",
        .request =
            {
                "POST / HTTP/1.1\r\nHost:with body\r\n",
                "Transfer-Encoding: chunked\r\n",
                "\r\n"
                "2\r\n",
                "He",
                "3\r\nl",
                "lo",
                "\r\n0\r\n\r\n",
            },
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers =
                    {
                        {
                            .name = "host",
                            .name_len = 4,
                            .val = "with body",
                            .val_len = 9,
                        },
                    },
            },
    },
    {
        .test_name = "chunked body (fragmented + broken-multi-message)",
        .request =
            {
                "POST / HTTP/1.1\r\nHost:with body\r\n",
                "Transfer-Encoding: chunked\r\n",
                "\r\n",
                "2\r\n",
                "H",
                "e",
                "3\r\nl",
                "l"
                "o",
                "\r\n0\r\n\r\n",
            },
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers =
                    {
                        {
                            .name = "host",
                            .name_len = 4,
                            .val = "with body",
                            .val_len = 9,
                        },
                    },
            },
    },
    {
        .test_name = "chunked body (...longer + trailer + empty value...)",
        .request =
            {
                "POST / HTTP/1.1\r\nHost:with body\r\n",
                "Transfer-Encoding: chunked\r\n",
                "\r\n",
                "4\r\n",
                "Wiki\r\n",
                "5\r\n",
                "pedia\r\n",
                "E\r\n",
                " in\r\n",
                "\r\n",
                "chunks.\r\n",
                "0\r\n",
                "X-Foo: trailer\r\n",
                "sErvEr-tiMing:   \r\n",
                "\r\n",
            },
        .expect =
            {
                .body = "Wikipedia in\r\n\r\nchunks.",
                .body_len = 23,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{
                                .name = "host",
                                .name_len = 4,
                                .val = "with body",
                                .val_len = 9,
                            },
                            {
                                .name = "x-foo",
                                .name_len = 5,
                                .val = "trailer",
                                .val_len = 7,
                            },
                            {
                                .name = "server-timing",
                                .name_len = 13,
                                .val = "",
                                .val_len = 0,
                            }},
            },
    },
    {
        .test_name = "chunked body (fragmented + surprise trailer)",
        .request =
            {
                "POST / HTTP/1.1\r\nHost:with body\r\n",
                "Transfer-Encoding: chunked\r\n",
                "\r\n"
                "5\r\n",
                "He",
                "llo",
                "\r\n0\r\nX-Foo: trailer\r\n\r\n",
            },
        .expect =
            {
                .body = "Hello",
                .body_len = 5,
                .method = "POST",
                .path = "/",
                .query = NULL,
                .version = "HTTP/1.1",
                .headers = {{
                                .name = "host",
                                .name_len = 4,
                                .val = "with body",
                                .val_len = 9,
                            },
                            {
                                .name = "x-foo",
                                .name_len = 5,
                                .val = "trailer",
                                .val_len = 7,
                            }},
            },
    },
    /* stop marker */
    {
        .request = {NULL},
    },
};

/** called when a request was received. */
static int http1_on_complete(void *udata) {
  (void)udata;
  return 0;
}
/** called when a request method is parsed. */
static int http1_on_method(fio_buf_info_s method, void *udata) {
  (void)udata;
  http1_test_data[http1_test_pos].result.method = method.buf;
  FIO_ASSERT(method.len ==
                 strlen(http1_test_data[http1_test_pos].expect.method),
             "method.len test error for: %s",
             http1_test_data[http1_test_pos].test_name);
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int http1_on_status(size_t istatus, fio_buf_info_s status, void *udata) {
  (void)udata;
  http1_test_data[http1_test_pos].result.status = istatus;
  http1_test_data[http1_test_pos].result.method = status.buf;
  FIO_ASSERT(status.len ==
                 strlen(http1_test_data[http1_test_pos].expect.method),
             "status length test error for: %s",
             http1_test_data[http1_test_pos].test_name);
  return 0;
}
/** called when a request path (excluding query) is parsed. */
static int http1_on_url(fio_buf_info_s path, void *udata) {
  (void)udata;
  fio_url_s u = fio_url_parse(path.buf, path.len);
  http1_test_data[http1_test_pos].result.path = u.path.buf;
  FIO_ASSERT(u.path.len == strlen(http1_test_data[http1_test_pos].expect.path),
             "path length test error for: %s",
             http1_test_data[http1_test_pos].test_name);
  if (http1_test_data[http1_test_pos].expect.query) {
    http1_test_data[http1_test_pos].result.query = u.query.buf;
    FIO_ASSERT(u.query.len ==
                   strlen(http1_test_data[http1_test_pos].expect.query),
               "query length test error for: %s",
               http1_test_data[http1_test_pos].test_name);
  }
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int http1_on_version(fio_buf_info_s version, void *udata) {
  (void)udata;
  http1_test_data[http1_test_pos].result.version = version.buf;
  FIO_ASSERT(version.len ==
                 strlen(http1_test_data[http1_test_pos].expect.version),
             "version length test error for: %s",
             http1_test_data[http1_test_pos].test_name);
  return 0;
}
/** called when a header is parsed. */
static int http1_on_header(fio_buf_info_s name,
                           fio_buf_info_s value,
                           void *udata) {
  (void)udata;
  size_t pos = 0;
  while (pos < 12 && http1_test_data[http1_test_pos].result.headers[pos].name)
    ++pos;
  FIO_ASSERT(pos < 12,
             "header result overflow for: %s",
             http1_test_data[http1_test_pos].test_name);
  memcpy(http1_test_temp_buf + http1_test_temp_buf_pos, name.buf, name.len);
  name.buf = http1_test_temp_buf + http1_test_temp_buf_pos;
  http1_test_temp_buf_pos += name.len;
  http1_test_temp_buf[http1_test_temp_buf_pos++] = 0;
  memcpy(http1_test_temp_buf + http1_test_temp_buf_pos, value.buf, value.len);
  value.buf = http1_test_temp_buf + http1_test_temp_buf_pos;
  http1_test_temp_buf_pos += value.len;
  http1_test_temp_buf[http1_test_temp_buf_pos++] = 0;
  http1_test_data[http1_test_pos].result.headers[pos].name = name.buf;
  http1_test_data[http1_test_pos].result.headers[pos].name_len = name.len;
  http1_test_data[http1_test_pos].result.headers[pos].val = value.buf;
  http1_test_data[http1_test_pos].result.headers[pos].val_len = value.len;
  return 0;
}

/** called when the special content-length header is parsed. */
static int http1_on_header_content_length(fio_buf_info_s name,
                                          fio_buf_info_s value,
                                          size_t content_length,
                                          void *udata) {
  (void)content_length;
  return http1_on_header(name, value, udata);
}

/** called when a body chunk is parsed. */
static int http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  (void)udata;
  http1_test_data[http1_test_pos]
      .result.body[http1_test_data[http1_test_pos].result.body_len] = 0;
  FIO_ASSERT(chunk.len + http1_test_data[http1_test_pos].result.body_len <=
                 http1_test_data[http1_test_pos].expect.body_len,
             "body overflow for: %s"
             "\r\n Expect:\n%s\nGot:\n%s%s\n",
             http1_test_data[http1_test_pos].test_name,
             http1_test_data[http1_test_pos].expect.body,
             http1_test_data[http1_test_pos].result.body,
             chunk.buf);
  memcpy(http1_test_data[http1_test_pos].result.body +
             http1_test_data[http1_test_pos].result.body_len,
         chunk.buf,
         chunk.len);
  http1_test_data[http1_test_pos].result.body_len += chunk.len;
  http1_test_data[http1_test_pos]
      .result.body[http1_test_data[http1_test_pos].result.body_len] = 0;
  return 0;
}
/** called when `Expect` arrives and may require a 100 continue response. */
static int http1_on_expect(fio_buf_info_s expected, void *udata) {
  (void)expected, (void)udata;
  return 0;
}

#define HTTP1_TEST_STRING_FIELD(field, i)                                      \
  FIO_ASSERT((!http1_test_data[i].expect.field &&                              \
              !http1_test_data[i].result.field) ||                             \
                 (http1_test_data[i].expect.field &&                           \
                  http1_test_data[i].result.field &&                           \
                  !memcmp(http1_test_data[i].expect.field,                     \
                          http1_test_data[i].result.field,                     \
                          strlen(http1_test_data[i].expect.field))),           \
             "string field error for %s - " #field " \n%s\n%s",                \
             http1_test_data[i].test_name,                                     \
             http1_test_data[i].expect.field,                                  \
             http1_test_data[i].result.field);
static void http1_parser_test(void) {
  http1_test_pos = 0;

  for (size_t i = 0; http1_test_data[i].request[0]; ++i) {
    fprintf(stderr, "* http1 parser test: %s\n", http1_test_data[i].test_name);
    /* parse each request / response */
    http1_parser_s parser = {0};
    char buf[4096];
    size_t r = 0;
    size_t w = 0;
    http1_test_temp_buf_pos = 0;
    for (int j = 0; http1_test_data[i].request[j]; ++j) {
      memcpy(buf + w,
             http1_test_data[i].request[j],
             strlen(http1_test_data[i].request[j]));
      w += strlen(http1_test_data[i].request[j]);
      size_t p = http1_parse(&parser, FIO_BUF_INFO2(buf + r, w - r), NULL);
      if (p == HTTP1_PARSER_ERROR) {
        p = 0;
        FIO_LOG_WARNING("(HTTP/1.1 parser) was an error expected.");
      }
      r += p;
      FIO_ASSERT(r <= w, "parser consumed more than the buffer holds!");
    }
    /* test each request / response before overwriting the buffer */
    HTTP1_TEST_STRING_FIELD(body, i);
    HTTP1_TEST_STRING_FIELD(method, i);
    HTTP1_TEST_STRING_FIELD(path, i);
    HTTP1_TEST_STRING_FIELD(version, i);
    r = 0;
    while (http1_test_data[i].result.headers[r].name) {
      HTTP1_TEST_STRING_FIELD(headers[r].name, i);
      HTTP1_TEST_STRING_FIELD(headers[r].val, i);
      FIO_ASSERT(http1_test_data[i].expect.headers[r].val_len ==
                         http1_test_data[i].result.headers[r].val_len &&
                     http1_test_data[i].expect.headers[r].name_len ==
                         http1_test_data[i].result.headers[r].name_len,
                 "--- name / value length error");
      ++r;
    }
    FIO_ASSERT(!http1_test_data[i].expect.headers[r].name,
               "Expected header missing:\n\t%s: %s",
               http1_test_data[i].expect.headers[r].name,
               http1_test_data[i].expect.headers[r].val);
    /* test performance */
    w = 0;
    for (int j = 0; http1_test_data[i].request[j]; ++j) {
      memcpy(buf + w,
             http1_test_data[i].request[j],
             strlen(http1_test_data[i].request[j]));
      w += strlen(http1_test_data[i].request[j]);
    }
    uint64_t start = fio_time_milli();
    for (size_t repetition = 0; repetition < 1000000; ++repetition) {
      parser = (http1_parser_s){0};
      http1_test_temp_buf_pos = 0;
      http1_test_data[http1_test_pos].result.body_len = 0;
      FIO_MEMSET(http1_test_data[http1_test_pos].result.headers,
                 0,
                 sizeof(http1_test_data[http1_test_pos].result.headers));
      FIO_COMPILER_GUARD;
      http1_parse(&parser, FIO_BUF_INFO2(buf, w), NULL);
    }
    uint64_t end = fio_time_milli();
    fprintf(stderr,
            "* %zums per 500k %s\n\n",
            (size_t)(end - start),
            http1_test_data[i].test_name);
    /* advance counter */
    ++http1_test_pos;
  }
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;
  http1_parser_test();
  return 0;
}
