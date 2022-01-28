/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

/* *****************************************************************************
This is a simple HTTP "Hello World" / echo server example using `poll`.

Benchmark with keep-alive:

    ab -c 200 -t 4 -n 1000000 -k http://127.0.0.1:3000/
    wrk -c200 -d4 -t2 http://localhost:3000/

Note: This is a **TOY** example, with only minimal security.
***************************************************************************** */

/* include some of the modules we use... */
#define FIO_LOG
#define FIO_CLI
#define FIO_SERVER
// #define FIO_MEMORY_DISABLE 1
// #define FIO_USE_THREAD_MUTEX 1
#include "fio-stl.h"

/* Short string object used for response objects. */
#define FIO_MEMORY_NAME str_allocator /* response & stream string allocator */
#define FIO_STR_NAME    str
#include "fio-stl.h"

/* we use local global variables to make the code easier. */

#define HTTP_CLIENT_BUFFER 32768
#define HTTP_MAX_HEADERS   16
#define HTTP_TIMEOUTS      5000

/* an echo response (vs. Hello World). */
#define HTTP_RESPONSE_ECHO 0

/* *****************************************************************************
Callbacks and object used by main()
***************************************************************************** */
#include "http/http-handle.h"

/** Called when a new connection is created. */
FIO_SFUNC void on_open(int fd, void *udata);
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io);
/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *udata);

fio_protocol_s HTTP_PROTOCOL_1 = {
    .on_data = on_data,
    .on_close = on_close,
};
/* *****************************************************************************
Starting the program - main()
***************************************************************************** */

int main(int argc, char const *argv[]) {
#ifdef TEST
  FIO_LOG_INFO("Testing HTTP Handle.");
  http_test();
#endif
  /* initialize the CLI options */
  fio_cli_start(argc,
                argv,
                0, /* allow 1 unnamed argument - the address to connect to */
                1,
                "A simple HTTP \"hello world\" example, listening on the "
                "specified URL. i.e.\n"
                "\tNAME <url>\n\n"
                "Unix socket examples:\n"
                "\tNAME unix://./my.sock\n"
                "\tNAME /full/path/to/my.sock\n"
                "\nTCP/IP socket examples:\n"
                "\tNAME tcp://localhost:3000/\n"
                "\tNAME localhost://3000\n",
                FIO_CLI_BOOL("--verbose -V -d print out debugging messages."),
                FIO_CLI_PRINT_LINE(
                    "NOTE: requests are limited to 32Kb and 16 headers each."));

  /* review CLI for logging */
  if (fio_cli_get_bool("-V")) {
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  }
  /* review CLI connection address (in URL format) */
  FIO_ASSERT(!fio_listen(.url = fio_cli_unnamed(0), .on_open = on_open),
             "Could not open listening socket as requested.");
  FIO_LOG_INFO("Starting HTTP echo server. Press ^C to exit.");
  fio_srv_run();
  FIO_LOG_INFO("Shutdown complete.");
  fio_cli_end();
  return 0;
}

/* *****************************************************************************
IO "Objects"and helpers
***************************************************************************** */
#include "../parsers/http1_parser.h"

#include "http/http-handle.c"

typedef struct {
  http1_parser_s parser;
  fio_s *io;
  http_s *h;
  int buf_pos;
  int buf_con;
  char buf[]; /* header and data buffer */
} client_s;

#define FIO_REF_NAME client
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_DESTROY(c)                                                     \
  do {                                                                         \
    http_free((c).h);                                                          \
    (c).h = NULL;                                                              \
  } while (0)
#define FIO_REF_FLEX_TYPE char
#include "fio-stl.h"

/** Called when a new connection is created. */
FIO_SFUNC void on_open(int fd, void *udata) {
  client_s *c = client_new(HTTP_CLIENT_BUFFER);
  FIO_ASSERT_ALLOC(c);
  c->io = fio_attach_fd(fd, &HTTP_PROTOCOL_1, c, udata);
}

/* *****************************************************************************
IO callback(s)
***************************************************************************** */

/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io) {
  client_s *c = fio_udata_get(io);
  ssize_t r =
      fio_read(io, c->buf + c->buf_pos, HTTP_CLIENT_BUFFER - c->buf_pos);
  if (r > 0) {
    c->buf_pos += r;
    while ((r = http1_parse(&c->parser,
                            c->buf + c->buf_con,
                            (size_t)(c->buf_pos - c->buf_con)))) {
      c->buf_con += r;
      if (!http1_complete(&c->parser))
        break;
      if (c->buf_con == c->buf_pos)
        c->buf_pos = c->buf_con = 0;
      else {
        c->buf_pos = c->buf_pos - c->buf_con;
        memmove(c->buf, c->buf + c->buf_con, c->buf_pos);
        c->buf_con = 0;
      }
    }
  }
}

/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *c_) { client_free((client_s *)c_); }

/* *****************************************************************************
HTTP/1.1 callback(s)
***************************************************************************** */

/** called when a request was received. */
static int http1_on_request(http1_parser_s *parser) {
  client_s *c = (client_s *)parser;
#if HTTP_RESPONSE_ECHO
  /* TODO */
#else
  http_response_header_set(c->h,
                           FIO_STR_INFO1("server"),
                           FIO_STR_INFO1("fio-stl"));
  http_write(c->h, .data = "Hello World", .len = 11, .finish = 1);
#endif

  /* reset client request data */
  http_free(c->h);
  c->h = NULL;
  return 0;
}

/** called when a response was received. */
static int http1_on_response(http1_parser_s *parser) {
  (void)parser;
  FIO_LOG_ERROR("response receieved instead of a request. Silently ignored.");
  return -1;
}
/** called when a request method is parsed. */
static int http1_on_method(http1_parser_s *parser,
                           char *method,
                           size_t method_len) {
  client_s *c = (client_s *)parser;
  c->method = method;
  c->method_len = method_len;
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int http1_on_status(http1_parser_s *parser,
                           size_t status,
                           char *status_str,
                           size_t len) {
  return -1;
  (void)parser;
  (void)status;
  (void)status_str;
  (void)len;
}
/** called when a request path (excluding query) is parsed. */
static int http1_on_path(http1_parser_s *parser, char *path, size_t path_len) {
  client_s *c = (client_s *)parser;
  c->path = path;
  c->path_len = path_len;
  return 0;
}
/** called when a request path (excluding query) is parsed. */
static int http1_on_query(http1_parser_s *parser,
                          char *query,
                          size_t query_len) {
  return 0;
  (void)parser;
  (void)query;
  (void)query_len;
}
/** called when a the HTTP/1.x version is parsed. */
static int http1_on_version(http1_parser_s *parser, char *version, size_t len) {
  return 0;
  (void)parser;
  (void)version;
  (void)len;
}
/** called when a header is parsed. */
static int http1_on_header(http1_parser_s *parser,
                           char *name,
                           size_t name_len,
                           char *value,
                           size_t value_len) {
  client_s *c = (client_s *)parser;
  if (c->headers_len >= HTTP_MAX_HEADERS)
    return -1;
  c->headers[c->headers_len].name = name;
  c->headers[c->headers_len].name_len = name_len;
  c->headers[c->headers_len].value = value;
  c->headers[c->headers_len].value_len = value_len;
  ++c->headers_len;
  return 0;
}
/** called when a body chunk is parsed. */
static int http1_on_body_chunk(http1_parser_s *parser,
                               char *data,
                               size_t data_len) {
  if (parser->state.content_length >= HTTP_CLIENT_BUFFER)
    return -1;
  client_s *c = (client_s *)parser;
  if (!c->body)
    c->body = data;
  c->body_len += data_len;
  return 0;
}
/** called when a protocol error occurred. */
static int http1_on_error(http1_parser_s *parser) {
  client_s *c = (client_s *)parser;
  http_send_response(
      c,
      400,
      (fio_str_info_s){"Bad Request", 11},
      0,
      NULL,
      (fio_str_info_s){"Bad Request... be nicer next time!", 34});
  fio_close(c->io);
  return -1;
}

/* *****************************************************************************
HTTP/1.1 response authorship helper
***************************************************************************** */

static char http_date_buf[128];
static size_t http_date_len;
static time_t http_date_tm;

static void http_send_response(client_s *c,
                               int status,
                               fio_str_info_s status_str,
                               size_t header_count,
                               fio_str_info_s headers[][2],
                               fio_str_info_s body) {
  struct timespec tm = fio_time_real();
  if (http_date_tm != tm.tv_sec) {
    http_date_len = fio_time2rfc7231(http_date_buf, tm.tv_sec);
    http_date_tm = tm.tv_sec;
  }

  size_t total_len = 9 + 4 + 15 + 20 /* max content length */ + 2 +
                     status_str.len + 2 + http_date_len + 5 + 7 + 2 + body.len;
  for (size_t i = 0; i < header_count; ++i) {
    total_len += headers[i][0].len + 1 + headers[i][1].len + 2;
  }
  if (status < 100 || status > 999)
    status = 500;
  str_s *response = str_new();
  str_reserve(response, total_len);
  str_write(response, "HTTP/1.1 ", 9);
  str_write_i(response, status);
  str_write(response, " ", 1);
  str_write(response, status_str.buf, status_str.len);
  str_write(response, "\r\nContent-Length:", 17);
  str_write_i(response, body.len);
  str_write(response, "\r\nDate: ", 8);
  str_write(response, http_date_buf, http_date_len);
  str_write(response, "\r\n", 2);

  for (size_t i = 0; i < header_count; ++i) {
    str_write(response, headers[i][0].buf, headers[i][0].len);
    str_write(response, ":", 1);
    str_write(response, headers[i][1].buf, headers[i][1].len);
    str_write(response, "\r\n", 2);
  }
  fio_str_info_s final = str_write(response, "\r\n", 2);
  if (body.len && body.buf)
    final = str_write(response, body.buf, body.len);
  fio_write2(c->io,
             .buf = response,
             .offset = (((uintptr_t) final.buf) - (uintptr_t)response),
             .len = final.len,
             .dealloc = (void (*)(void *))str_free);
  FIO_LOG_DEBUG2("Sending response %d, %zu bytes, to io %p",
                 status,
                 final.len,
                 c->io);
}
