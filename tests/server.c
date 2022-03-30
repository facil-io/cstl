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

Note: This is a **TOY** example, with only minimal security features.
***************************************************************************** */

/* include some of the modules we use... */
#define FIO_LOG
#define FIO_CLI
#define FIO_SERVER
// #define FIO_MEMORY_DISABLE 1
// #define FIO_USE_THREAD_MUTEX 1
#include "fio-stl.h"

/* we use local global variables to make the code easier. */

#define HTTP_CLIENT_BUFFER 32768
#define HTTP_MAX_BODY_SIZE (1 << 27)
#define HTTP_MAX_HEADERS   16
#define HTTP_TIMEOUTS      5000

/* an echo response (vs. Hello World). */
#define HTTP_RESPONSE_ECHO 1

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
The HTTP Response Function
***************************************************************************** */
#define FIO_STR_NAME str
#define FIO_MEM_NAME str_allocator
#include "fio-stl.h"

FIO_SFUNC int http_write_headers_to_string(http_s *h,
                                           fio_str_info_s name,
                                           fio_str_info_s value,
                                           void *out_) {
  str_s *out = (str_s *)out_;
  (void)h;
  str_write(out, name.buf, name.len);
  str_write(out, ": ", 2);
  str_write(out, value.buf, value.len);
  str_write(out, "\r\n", 2);
  return 0;
}

FIO_SFUNC void http_respond(http_s *h) {
#if HTTP_RESPONSE_ECHO
  str_s out = FIO_STR_INIT;
  http_request_header_each(h, http_write_headers_to_string, &out);
  if (http_body_length(h)) {
    fio_str_info_s body = http_body_read(h, -1);
    str_write(&out, "\r\n", 2);
    str_write(&out, body.buf, body.len);
  }
  size_t len = str_len((&out));
  FIO_LOG_DEBUG("echoing:\n%s", str2ptr(&out));
  http_write(h,
             .data = str_detach(&out),
             .len = len,
             .dealloc = str_dealloc,
             .copy = 0,
             .finish = 1);
#else
  http_response_header_set(h,
                           FIO_STR_INFO1("server"),
                           FIO_STR_INFO1("fio-stl"));
  http_write(h, .data = "Hello World!", .len = 12, .finish = 1);
#endif
}

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
  fio_str_info_s out;
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
    FIO_MEM_FREE((c).out.buf, (c).out.capa);                                   \
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
FIO_SFUNC void on_data__internal(client_s *c) {
  size_t tmp;
  if ((tmp = http1_parse(&c->parser,
                         c->buf + c->buf_con,
                         (size_t)(c->buf_pos - c->buf_con)))) {
    c->buf_con += tmp;
    if (!http1_complete(&c->parser))
      return;
    if (c->buf_con == c->buf_pos)
      c->buf_pos = c->buf_con = 0;
    else {
      c->buf_pos = c->buf_pos - c->buf_con;
      memmove(c->buf, c->buf + c->buf_con, c->buf_pos);
      c->buf_con = 0;
    }
  }
  return;
}

/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io) {
  client_s *c = fio_udata_get(io);
  ssize_t r =
      fio_read(io, c->buf + c->buf_pos, HTTP_CLIENT_BUFFER - c->buf_pos);
  if (r > 0)
    c->buf_pos += r;
  on_data__internal(fio_udata_get(io));
}

/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *c_) {
  client_s *c = (client_s *)c_;
  c->io = NULL;
  client_free(c);
}

/* *****************************************************************************
HTTP/1.1 Protocol Controller
***************************************************************************** */
/* Use fio_malloc. */
#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#define FIO_MALLOC
#include "fio-stl.h"

/** Informs the controller that a request is starting. */
int http1_start_request(http_s *h, int reserved, int streaming) {
  (void)reserved;
  (void)streaming;
  client_s *c = http_controller_data(h);
  if (!c->io)
    return -1;
  return -1;
}
/** called by the HTTP handle for each header. */
void http1_write_header(http_s *h, fio_str_info_s name, fio_str_info_s value) {
  client_s *c = http_controller_data(h);
  if (!c->io)
    return;
  const size_t minimal_capa = (c->out.len + name.len + value.len + 3);
  if (c->out.capa < minimal_capa) {
    const size_t new_capa = (minimal_capa + 17) & (~((size_t)15ULL));
    void *tmp = FIO_MEM_REALLOC(c->out.buf, c->out.capa, new_capa, c->out.len);
    if (!tmp) {
      FIO_LOG_ERROR(
          "Couldn't allocate memory for header output (required %zu bytes)!",
          new_capa);
      return;
    }
    c->out.buf = tmp;
    c->out.capa = new_capa;
  }
  memcpy(c->out.buf + c->out.len, name.buf, name.len);
  c->out.buf[c->out.len + name.len] = ':';
  memcpy(c->out.buf + c->out.len + name.len + 1, value.buf, value.len);
  c->out.buf[c->out.len + name.len + value.len + 1] = '\r';
  c->out.buf[c->out.len + name.len + value.len + 2] = '\n';
  c->out.len = minimal_capa;
}

/** Called before an HTTP handler link to an HTTP Controller is revoked. */
void http1_on_unlinked(http_s *h, void *c_) {
  client_s *c = c_; // client_s *c = http_controller_data(h);
  if (c->h == h)
    c->h = NULL;
  client_free(c);
  (void)h;
}

/** Informs the controller that a response is starting. */
int http1_start_response(http_s *h, int status, int streaming) {
  client_s *c = http_controller_data(h);
  if (!c->io)
    return -1;
  char buf[32];
  fio_str_info_s ver = http_version_get(h);
  size_t n_len = fio_ltoa(buf, status, 10);
  fio_str_info_s ststr = http_status2str(status);
  FIO_ASSERT(!c->out.buf,
             "we shouldn't handle http_finish twice concurrently!");
  size_t capa = (ver.len + ststr.len + n_len + 4);
  capa = (capa + 15) & (~((size_t)15ULL));
  c->out.buf = FIO_MEM_REALLOC(NULL, 0, capa, 0);
  if (!c->out.buf) {
    FIO_LOG_ERROR("memory allocation error!");
    return -1;
  }
  c->out.capa = capa;
  c->out.len = (ver.len + ststr.len + n_len + 4);
  if (ver.len > 15) {
    FIO_LOG_ERROR("HTTP/1.1 version string too long!");
    return -1;
  }
  memcpy(c->out.buf, ver.buf, ver.len);
  c->out.buf[ver.len] = ' ';
  memcpy(c->out.buf + ver.len + 1, buf, n_len);
  c->out.buf[n_len + ver.len + 1] = ' ';
  memcpy(c->out.buf + n_len + ver.len + 2, ststr.buf, ststr.len);
  c->out.buf[n_len + ver.len + ststr.len + 2] = '\r';
  c->out.buf[n_len + ver.len + ststr.len + 3] = '\n';
  if (streaming) {
    /* TODO: add streaming headers */
  }
  return 0;
}

static void http_1_free_wrapper(void *ptr) { FIO_MEM_FREE(ptr, 0); }

/** Informs the controller that all headers were provided. */
void http1_finish_headers(http_s *h) {
  client_s *c = http_controller_data(h);
  if (!c->io)
    return;
  c->out.buf[c->out.len] = '\r';
  c->out.buf[c->out.len + 1] = '\n';
  c->out.len += 2;
  fio_write2(c->io,
             .buf = c->out.buf,
             .len = c->out.len,
             .copy = 0,
             .dealloc = http_1_free_wrapper);
  c->out.len = c->out.capa = 0;
  c->out.buf = NULL;
}
/** called by the HTTP handle for each body chunk (or to finish a response. */
void http1_write_body(http_s *h, http_write_args_s args) {
  client_s *c = http_controller_data(h);
  if (!c->io)
    return;
  if (http_is_streaming(h)) {
    /* TODO: add streaming wrapper for chunked data */
  }
  fio_write2(c->io,
             .buf = (void *)args.data,
             .len = args.len,
             .fd = args.fd,
             .dealloc = args.dealloc,
             .copy = args.copy);
  if (args.finish) {
    http_free(c->h);
  }
}

http_controller_s HTTP1_CONTROLLER = {
    .on_unlinked = http1_on_unlinked,
    .start_response = http1_start_response,
    .start_request = http1_start_request,
    .write_header = http1_write_header,
    .finish_headers = http1_finish_headers,
    .write_body = http1_write_body,
};

/* *****************************************************************************
HTTP/1.1 callback(s)
***************************************************************************** */

FIO_SFUNC void http_deferred_response(void *h_, void *ignr_) {
  http_s *h = (http_s *)h_;
  (void)ignr_;
  http_respond(h);
  if (!http_is_streaming(h) || !http_is_finished(h))
    http_finish(h);
}

/** called when a request was received. */
static int http1_on_request(http1_parser_s *parser) {
  client_s *c = (client_s *)parser;
  http_status_set(c->h, 200);
  http_respond(c->h);
  return 0;
}

/** called when a response was received. */
static int http1_on_response(http1_parser_s *parser) {
  (void)parser;
  FIO_LOG_ERROR("response received instead of a request. Silently ignored.");
  return -1;
}
/** called when a request method is parsed. */
static int http1_on_method(http1_parser_s *parser,
                           char *method,
                           size_t method_len) {
  client_s *c = (client_s *)parser;
  if (!c->h) {
    c->h = http_new();
    http_controller_set(c->h, &HTTP1_CONTROLLER, client_dup(c));
  }
  http_method_set(c->h, FIO_STR_INFO2(method, method_len));
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int http1_on_status(http1_parser_s *parser,
                           size_t status,
                           char *status_str,
                           size_t len) {
  return -1;
  client_s *c = (client_s *)parser;
  http_status_set(c->h, status);
  (void)parser;
  (void)status;
  (void)status_str;
  (void)len;
}
/** called when a request path (excluding query) is parsed. */
static int http1_on_path(http1_parser_s *parser, char *path, size_t path_len) {
  client_s *c = (client_s *)parser;
  http_path_set(c->h, FIO_STR_INFO2(path, path_len));
  return 0;
}
/** called when a request path (excluding query) is parsed. */
static int http1_on_query(http1_parser_s *parser,
                          char *query,
                          size_t query_len) {
  client_s *c = (client_s *)parser;
  http_query_set(c->h, FIO_STR_INFO2(query, query_len));
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int http1_on_version(http1_parser_s *parser, char *version, size_t len) {
  client_s *c = (client_s *)parser;
  http_version_set(c->h, FIO_STR_INFO2(version, len));
  return 0;
}
/** called when a header is parsed. */
static int http1_on_header(http1_parser_s *parser,
                           char *name,
                           size_t name_len,
                           char *value,
                           size_t value_len) {
  client_s *c = (client_s *)parser;
  http_request_header_add(c->h,
                          FIO_STR_INFO2(name, name_len),
                          FIO_STR_INFO2(value, value_len));
  return 0;
}
/** called when a body chunk is parsed. */
static int http1_on_body_chunk(http1_parser_s *parser,
                               char *data,
                               size_t data_len) {
  client_s *c = (client_s *)parser;
  if (data_len + http_body_length(c->h) > HTTP_MAX_BODY_SIZE)
    return -1;
  http_body_write(c->h, data, data_len);
  return 0;
}
/** called when a protocol error occurred. */
static int http1_on_error(http1_parser_s *parser) {
  client_s *c = (client_s *)parser;
  if (c->h) {
    http_status_set(c->h, 400);
    http_write(c->h,
               .data = "Bad Request... be nicer next time!",
               .len = 34,
               .finish = 1);
  }
  fio_close(c->io);
  return -1;
}
