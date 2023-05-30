/* *****************************************************************************
Copyright: Boaz Segev, 2019-2022
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

/* *****************************************************************************
This is a simple HTTP "Hello World" / echo server example.

This example provides only a minimal number of security features such as timeout
management and throttling of busy / slow clients.

Benchmark with keep-alive:

    ab -c 200 -t 4 -n 1000000 -k http://127.0.0.1:3000/
    wrk -c200 -d4 -t2 http://localhost:3000/


***************************************************************************** */

/* we use local global variables to make the code easier. */

#define HTTP_CLIENT_BUFFER 32768
#define HTTP_MAX_BODY_SIZE (1ULL << 27)
#define HTTP_MAX_HEADERS   16
#define HTTP_TIMEOUT       (40 * 1000) /* is in milliseconds */

/* an echo response (vs. Hello World). */
#define HTTP_RESPONSE_ECHO 1

/* *****************************************************************************
Used facil.io Modules
***************************************************************************** */

// #define FIO_MEMORY_DISABLE 1
// #define FIO_USE_THREAD_MUTEX 1
// #define FIO_POLL_ENGINE FIO_POLL_ENGINE_POLL
#define FIO_LEAK_COUNTER 1
#define FIO_BASIC
#include "fio-stl/include.h"
#define FIO_SERVER
#define FIO_HTTP1_PARSER
#define FIO_HTTP_HANDLE
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Callbacks and object used by main()
***************************************************************************** */

/** Called when a new connection is created. */
FIO_SFUNC void on_attach(fio_s *io);
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io);
/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *udata);

static fio_protocol_s HTTP_PROTOCOL_1 = {
    .on_attach = on_attach,
    .on_data = on_data,
    .on_close = on_close,
    .timeout = HTTP_TIMEOUT,
};

/* *****************************************************************************
IO "Objects"and helpers
***************************************************************************** */

typedef struct {
  fio_http1_parser_s parser;
  fio_s *io;
  fio_http_s *h;
  int buf_pos;
  int buf_con;
  char buf[]; /* header and data buffer */
} client_s;

#define FIO_REF_NAME client
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_DESTROY(c)                                                     \
  do {                                                                         \
    fio_http_free((c).h);                                                      \
    (c).h = NULL;                                                              \
  } while (0)
#define FIO_REF_FLEX_TYPE char
#include "fio-stl.h"

/** Called when a new connection is created. */
FIO_SFUNC void on_attach(fio_s *io) {
  client_s *c = client_new(HTTP_CLIENT_BUFFER);
  FIO_ASSERT_ALLOC(c);
  c->io = io;
  fio_udata_set(io, c);
}

/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *c_) {
  client_s *c = (client_s *)c_;
  c->io = NULL;
  fio_http_free(c->h);
  c->h = NULL;
  client_free(c);
}
/* *****************************************************************************
The HTTP Response Function
***************************************************************************** */
static int http_should_log_responses = 0;
FIO_SFUNC int http_write_headers_to_string(fio_http_s *h,
                                           fio_str_info_s name,
                                           fio_str_info_s value,
                                           void *out_) {
  char **out = (char **)out_;
  (void)h;
  *out = fio_bstr_write(*out, name.buf, name.len);
  *out = fio_bstr_write(*out, ": ", 2);
  *out = fio_bstr_write(*out, value.buf, value.len);
  *out = fio_bstr_write(*out, "\r\n", 2);
  return 0;
}

FIO_SFUNC void http_respond(fio_http_s *h) {
  fio_http_response_header_set(h,
                               FIO_STR_INFO1("server"),
                               FIO_STR_INFO1("fio-stl"));
  if (!fio_http_static_file_response(h,
                                     FIO_STR_INFO2(".", 1),
                                     fio_http_path(h),
                                     3600))
    return;
#if HTTP_RESPONSE_ECHO
  char *out = fio_bstr_write2(
      NULL,
      FIO_STRING_WRITE_STR2(fio_http_method(h).buf, fio_http_method(h).len),
      FIO_STRING_WRITE_STR2(" ", 1),
      FIO_STRING_WRITE_STR2(fio_http_path(h).buf, fio_http_path(h).len),
      FIO_STRING_WRITE_STR2("?", (fio_http_query(h).len ? 1 : 0)),
      FIO_STRING_WRITE_STR2(fio_http_query(h).buf, fio_http_query(h).len),
      FIO_STRING_WRITE_STR2(" ", 1),
      FIO_STRING_WRITE_STR2(fio_http_version(h).buf, fio_http_version(h).len),
      FIO_STRING_WRITE_STR2("\r\n", 2));
  fio_http_request_header_each(h, http_write_headers_to_string, &out);
  if (fio_http_body_length(h)) {
    fio_str_info_s body = fio_http_body_read(h, (size_t)-1);
    out = fio_bstr_write2(out,
                          FIO_STRING_WRITE_STR2("\r\n", 2),
                          FIO_STRING_WRITE_STR2(body.buf, body.len),
                          FIO_STRING_WRITE_STR2("\r\n", 2));
  }
  if (1) {
    fio_env_set(((client_s *)fio_http_cdata(h))->io,
                .name = FIO_BUF_INFO2("my key", 6),
                .udata = fio_bstr_write(NULL, "my env data", 11),
                .on_close = (void (*)(void *))fio_bstr_free);
  }
  if (1) {
    uint64_t hash =
        fio_risky_hash(fio_http_path(h).buf, fio_http_path(h).len, 0);
    char hash_buf[18];
    fio_str_info_s etag = FIO_STR_INFO3(hash_buf, 0, 18);
    fio_string_write_hex(&etag, NULL, hash);
    fio_http_response_header_set(h, FIO_STR_INFO2("etag", 4), etag);
  }
  // FIO_LOG_DEBUG2("echoing back:\n%s", str2ptr(&out));
  fio_http_write(h,
                 .buf = out,
                 .len = fio_bstr_len(out),
                 .dealloc = (void (*)(void *))fio_bstr_free,
                 .copy = 0,
                 .finish = 1);
#else
  fio_http_write(h, .buf = "Hello World!", .len = 12, .finish = 1);
#endif
}

/* *****************************************************************************
Starting the program - main()
***************************************************************************** */

int main(int argc, char const *argv[]) {
  fio_thread_priority_set(FIO_THREAD_PRIORITY_HIGHEST);
  /* initialize the CLI options */
  fio_cli_start(
      argc,
      argv,
      0, /* allow 1 unnamed argument - the address to connect to */
      1,
      "A simple HTTP \"hello world\" / echo example, using " FIO_POLL_ENGINE_STR
      " and listening on the "
      "specified URL. i.e.\n"
      "\tNAME <url>\n\n"
      "Unix socket examples:\n"
      "\tNAME unix://./my.sock\n"
      "\tNAME /full/path/to/my.sock\n"
      "\nTCP/IP socket examples:\n"
      "\tNAME tcp://localhost:3000/\n"
      "\tNAME localhost://3000\n",
      FIO_CLI_BOOL("--verbose -V -d print out debugging messages."),
      FIO_CLI_BOOL("--log -v log HTTP messages."),
      FIO_CLI_INT("--workers -w (1) number of worker processes to use."),
      FIO_CLI_PRINT_LINE(
          "NOTE: requests are limited to 32Kb and 16 headers each."));

  /* review CLI for logging */
  if (fio_cli_get_bool("-V")) {
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  }
  http_should_log_responses = fio_cli_get_bool("-v");
  /* review CLI connection address (in URL format) */
  FIO_ASSERT(
      fio_srv_listen(.url = fio_cli_unnamed(0), .protocol = &HTTP_PROTOCOL_1),
      "Could not open listening socket as requested.");
  FIO_LOG_INFO("\n\tStarting HTTP echo server example app."
               "\n\tEngine: " FIO_POLL_ENGINE_STR "\n\tWorkers: %d"
               "\n\tPress ^C to exit.",
               fio_srv_workers(fio_cli_get_i("-w")));
  fio_srv_start(fio_cli_get_i("-w"));
  FIO_LOG_INFO("Shutdown complete.");
  fio_cli_end();
  return 0;
}

/* *****************************************************************************
IO callback(s)
***************************************************************************** */

/** called when a protocol error occurred. */
static int fio_http1_on_error(void *udata) {
  client_s *c = (client_s *)udata;
  if (c->h) {
    fio_http_status_set(c->h, 400);
    fio_http_write(c->h,
                   .buf = "Bad Request... be nicer next time!",
                   .len = 34,
                   .finish = 1);
  }
  FIO_LOG_DEBUG2("HTTP error occurred, closing fd %d", c->io->fd);
  fio_close(c->io);
  return -1;
}

/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data__internal(client_s *c) {
  size_t tmp = fio_http1_parse(
      &c->parser,
      FIO_BUF_INFO2(c->buf + c->buf_con, (size_t)(c->buf_pos - c->buf_con)),
      c);
  if (!tmp)
    return;
  if (tmp == FIO_HTTP1_PARSER_ERROR) {
    fio_http1_on_error(c);
    return;
  }
  c->buf_con += tmp;
  if (c->h)
    return;
  if (c->buf_con == c->buf_pos)
    c->buf_pos = c->buf_con = 0;
  else {
    c->buf_pos = c->buf_pos - c->buf_con;
    FIO_MEMMOVE(c->buf, c->buf + c->buf_con, c->buf_pos);
    c->buf_con = 0;
  }
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

/* *****************************************************************************
HTTP/1.1 Protocol Controller
***************************************************************************** */

/** Called before an HTTP handler link to an HTTP Controller is revoked. */
static void fio_http1_on_destroyed(fio_http_s *h) {
  client_s *c = fio_http_cdata(h);
  if (c->h == h)
    c->h = NULL;
  client_free(c);
  (void)h;
}

/** called by the HTTP handle for each header. */
static int fio_http1___write_header_callback(fio_http_s *h,
                                             fio_str_info_s name,
                                             fio_str_info_s value,
                                             void *out_) {
  (void)h;
  /* manually copy, as this is an "all or nothing" copy (no truncation) */
  fio_str_info_s *out = (fio_str_info_s *)out_;
  size_t new_len = out->len + name.len + 1 + value.len + 2;
  if (out->capa < new_len + 1) {
    FIO_STRING_REALLOC(
        out,
        ((new_len + 15LL + (!(new_len & 15ULL))) & (~((size_t)15ULL))));
  }
  if (out->capa < new_len + 1)
    return -1;
  memcpy(out->buf + out->len, name.buf, name.len);
  out->buf[out->len + name.len] = ':';
  memcpy(out->buf + out->len + name.len + 1, value.buf, value.len);
  out->buf[out->len + name.len + value.len + 1] = '\r';
  out->buf[out->len + name.len + value.len + 2] = '\n';
  out->len = new_len;
  return 0;
}

/** Informs the controller that all headers were provided. */
static void fio_http1_send_headers(fio_http_s *h) {
  client_s *c = fio_http_cdata(h);
  if (!c->io)
    return;
  fio_str_info_s buf = FIO_STR_INFO2(NULL, 0);
  /* write status string */
  {
    fio_str_info_s ver = fio_http_version(h);
    fio_str_info_s status = fio_http_status2str(fio_http_status(h));
    if (ver.len > 15) {
      FIO_LOG_ERROR("HTTP/1.1 client version string too long!");
      ver = FIO_STR_INFO1("HTTP/1.1");
    }
    fio_string_write2(&buf,
                      FIO_STRING_REALLOC,
                      FIO_STRING_WRITE_STR2(ver.buf, ver.len),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_NUM(fio_http_status(h)),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_STR2(status.buf, status.len),
                      FIO_STRING_WRITE_STR2("\r\n", 2));
  }
  /* write headers */
  fio_http_response_header_each(h, fio_http1___write_header_callback, &buf);
  /* write cookies */
  fio_http_set_cookie_each(h, fio_http1___write_header_callback, &buf);
  /* add streaming headers? */
  if (fio_http_is_streaming(h))
    fio_string_write(&buf,
                     FIO_STRING_REALLOC,
                     "transfer-encoding: chunked\r\n",
                     28);
  fio_string_write(&buf, FIO_STRING_REALLOC, "\r\n", 2);
  /* send data (move memory ownership) */
  fio_write2(c->io,
             .buf = buf.buf,
             .len = buf.len,
             .copy = 0,
             .dealloc = FIO_STRING_FREE);
}

/** called by the HTTP handle for each body chunk (or to finish a response. */
static void fio_http1_write_body(fio_http_s *h, fio_http_write_args_s args) {
  client_s *c = fio_http_cdata(h);
  if (!c->io)
    return;
  if (fio_http_is_streaming(h)) {
    /* TODO: add streaming wrapper for chunked data */
    if (!args.len) {
      FIO_LOG_ERROR("HTTP1 streaming requires a correctly pre-determined "
                    "length per chunk.");
    } else {
      char buf[24];
      fio_str_info_s i = FIO_STR_INFO3(buf, 0, 24);
      fio_string_write_hex(&i, NULL, args.len);
      fio_string_write(&i, NULL, "\r\n", 2);
      fio_write2(c->io, .buf = (void *)i.buf, .len = i.len, .copy = 1);
    }
  }
  fio_write2(c->io,
             .buf = (void *)args.buf,
             .len = args.len,
             .fd = args.fd,
             .offset = args.offset,
             .dealloc = args.dealloc,
             .copy = args.copy);
  if (fio_http_is_streaming(h))
    fio_write2(c->io, .buf = "\r\n", .len = 2, .copy = 1);
  return;
}

static void fio_http1_on_finish(fio_http_s *h) {
  client_s *c = fio_http_cdata(h);
  if (!c->io)
    goto finish;
  if (fio_http_is_streaming(h)) {
    fio_write2(c->io, .buf = "0\r\n\r\n", .len = 5, .copy = 1);
  }
  if (http_should_log_responses)
    fio_http_write_log(h, FIO_BUF_INFO2(NULL, 0)); /* TODO: get_peer_addr */
finish:
  fio_http_free(h);
}

static fio_http_controller_s HTTP1_CONTROLLER = {
    .on_destroyed = fio_http1_on_destroyed,
    .send_headers = fio_http1_send_headers,
    .write_body = fio_http1_write_body,
    .on_finish = fio_http1_on_finish,
};

/* *****************************************************************************
HTTP/1.1 callback(s)
***************************************************************************** */

FIO_SFUNC void http_deferred_response(void *h_, void *ignr_) {
  fio_http_s *h = (fio_http_s *)h_;
  (void)ignr_;
  http_respond(h);
  if (!fio_http_is_streaming(h) || !fio_http_is_finished(h))
    fio_http_finish(h);
}

/** called when either a request or a response was received. */
static void fio_http1_on_complete(void *udata) {
  client_s *c = (client_s *)udata;
  fio_http_status_set(c->h, 200);
  http_respond(c->h);
}
/** called when a request method is parsed. */
static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  client_s *c = (client_s *)udata;
  if (c->h)
    return -1;
  c->h = fio_http_new();
  // http_destroy(c->h);
  fio_http_controller_set(c->h, &HTTP1_CONTROLLER);
  fio_http_cdata_set(c->h, client_dup(c));
  fio_http_method_set(c->h, FIO_BUF2STR_INFO(method));
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  FIO_LOG_ERROR("response received instead of a request. Silently ignored.");
  return -1; /* do not accept responses */
  (void)istatus, (void)status, (void)udata;
}
/** called when a request URL is parsed. */
static int fio_http1_on_url(fio_buf_info_s url, void *udata) {
  client_s *c = (client_s *)udata;
  fio_url_s u = fio_url_parse(url.buf, url.len);
  if (!u.path.len || u.path.buf[0] != '/')
    return -1;
  fio_http_path_set(c->h, FIO_BUF2STR_INFO(u.path));
  if (u.query.len)
    fio_http_query_set(c->h, FIO_BUF2STR_INFO(u.query));
  if (u.host.len)
    fio_http_request_header_set(c->h,
                                FIO_STR_INFO1("host"),
                                FIO_BUF2STR_INFO(u.host));
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  client_s *c = (client_s *)udata;
  fio_http_version_set(c->h, FIO_BUF2STR_INFO(version));
  return 0;
}
/** called when a header is parsed. */
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  client_s *c = (client_s *)udata;
  fio_http_request_header_add(c->h,
                              FIO_BUF2STR_INFO(name),
                              FIO_BUF2STR_INFO(value));
  return 0;
}
/** called when the special content-length header is parsed. */
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  client_s *c = (client_s *)udata;
  if (content_length > HTTP_MAX_BODY_SIZE)
    return -1; /* TODO: send "payload too big" response */
  if (content_length)
    fio_http_body_expect(c->h, content_length);
  fio_http_request_header_add(c->h,
                              FIO_BUF2STR_INFO(name),
                              FIO_BUF2STR_INFO(value));
  return 0;
  (void)name, (void)value;
}
/** called when `Expect` arrives and may require a 100 continue response. */
static int fio_http1_on_expect(fio_buf_info_s expected, void *udata) {
  client_s *c = (client_s *)udata;
  fio_write2(c->io, .buf = "100 Continue\r\n", .len = 14, .copy = 0);
  return 0; /* TODO: improve support for `expect` */
  (void)expected;
}

/** called when a body chunk is parsed. */
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  client_s *c = (client_s *)udata;
  if (chunk.len + fio_http_body_length(c->h) > HTTP_MAX_BODY_SIZE)
    return -1;
  fio_http_body_write(c->h, chunk.buf, chunk.len);
  return 0;
}
