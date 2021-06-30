/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

/* *****************************************************************************
This is a simple HTTP "Hello World" / echo server example using `poll`.

Benchmark with keep-alive:

    ab -c 200 -t 4 -n 1000000 -k http://127.0.0.1:3000/
    wrk -c200 -d4 -t1 http://localhost:3000/

Note: This is a **TOY** example, no security whatsoever!!!
***************************************************************************** */

/* include some of the modules we use... */
#define FIO_LOG
#define FIO_URL
#define FIO_SOCK
#define FIO_SIGNAL
#define FIO_CLI
#define FIO_TIME
#define FIO_POLL
// #define FIO_POLL_DEBUG
#include "fio-stl.h"

// #define FIO_MEMORY_DISABLE 1

/* Short string object used for response objects. */
#define FIO_STREAM
#define FIO_MEMORY_NAME str_allocator /* response & stream string allocator */
#define FIO_STR_NAME    str
#include "fio-stl.h"

/* we use local global variables to make the code easier. */

/** A flag telling us when to stop reviewing IO events. */
static volatile uint8_t server_stop_flag = 0;

#define HTTP_CLIENT_BUFFER 32768
#define HTTP_MAX_HEADERS   16

/* an echo response is always dynamic (allocated on the heap). */
#define HTTP_RESPONSE_ECHO 1
/* if not echoing, the response is "Hello World" - but is it allocated? */
#define HTTP_RESPONSE_DYNAMIC 1

/* *****************************************************************************
Callbacks and object used by main()
***************************************************************************** */

/** Called when the socket have available space in the outgoing buffer. */
FIO_SFUNC void on_ready(int fd, void *arg);
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(int fd, void *arg);
/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(int fd, void *arg);

/** The IO polling object - it keeps a one-shot list of monitored IOs. */
static fio_poll_s monitor = FIO_POLL_INIT(on_data, on_ready, on_close);

/* facil.io delays signal callbacks so they can safely with no restrictions. */
FIO_SFUNC void on_signal(int sig, void *udata);

/* *****************************************************************************
Starting the program - main()
***************************************************************************** */

int main(int argc, char const *argv[]) {
  /* initialize the CLI options */
  int srv_fd;
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
  const char *url = fio_cli_unnamed(0);
  if (!url)
    url = "0.0.0.0:3000";
  size_t url_len = strlen(url);
  FIO_ASSERT(url_len < 1024, "URL address too long");
  fio_url_s a = fio_url_parse(url, url_len);
  if (!a.host.buf && !a.port.buf) {
    /* Unix Socket */
#if FIO_OS_WIN
    FIO_ASSERT(0, "Unix style sockets are unsupported on Windows.");
#else
    srv_fd = fio_sock_open(a.path.buf,
                           NULL,
                           FIO_SOCK_UNIX | FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);
    FIO_LOG_DEBUG("Opened a Unix Socket (%d).", srv_fd);
#endif
  } else if (!a.scheme.buf || a.scheme.len != 3 ||
             (a.scheme.buf[0] | 32) != 'u' || (a.scheme.buf[1] | 32) != 'd' ||
             (a.scheme.buf[2] | 32) != 'p') {
    /* TCP/IP Socket */
    char buf[1024];
    /* copy because we need to add NUL bytes between the address and the port
     */
    memcpy(buf, a.host.buf, a.host.len + a.port.len + 2);
    buf[a.host.len + a.port.len + 1] = 0;
    buf[a.host.len] = 0;
    /* open the socket, passing NUL terminated strings for address and port */
    srv_fd = fio_sock_open(buf,
                           buf + a.host.len + 1,
                           FIO_SOCK_TCP | FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);
    /* log */
    FIO_LOG_DEBUG("Opened a TCP/IP Socket (%d) to %s port %s.",
                  srv_fd,
                  buf,
                  buf + a.host.len + 1);
  } else {
    /* UDP Socket */
    FIO_ASSERT(0, "This example doesn't support UDP sockets.");
  }

  /* we're dome with the CLI, release resources */
  fio_cli_end();

  /* test socket / connection success */
  if (srv_fd == -1) {
    FIO_LOG_FATAL("Couldn't open connection");
  }

  /* select signals to be monitored */
  fio_signal_monitor(SIGINT, on_signal, NULL);
  fio_signal_monitor(SIGTERM, on_signal, NULL);
#if FIO_OS_POSIX
  fio_signal_monitor(SIGQUIT, on_signal, NULL);
#endif

  /* select IO objects to be monitored */
  fio_poll_monitor(&monitor, srv_fd, NULL, POLLIN);
  FIO_LOG_INFO("Listening for HTTP echo @ %s", url);

  /* loop until the stop flag is raised */
  while (!server_stop_flag) {
    /* review IO events (calls the registered callbacks) */
    fio_poll_review(&monitor, 1000);
    /* review signals (calls the registered callback) */
    fio_signal_review();
  }

  /* cleanup */
  fio_poll_close_and_destroy(&monitor);
  FIO_LOG_INFO("Shutdown complete.");
  return 0;
}

/* *****************************************************************************
Signal callback(s)
***************************************************************************** */

/* facil.io delays signal callbacks so they can safely with no restrictions. */
FIO_SFUNC void on_signal(int sig, void *udata) {
  /* since there are no restrictions, we can safely print to the log. */
  FIO_LOG_INFO("Exit signal %d detected, shutting down.", sig);
  /* If the signal repeats, crash. */
  if (fio_atomic_exchange(&server_stop_flag, 1))
    exit(-1);
  (void)sig;
  (void)udata;
}

/* *****************************************************************************
IO "Objects"and helpers
***************************************************************************** */
#include "http1_parser.h"

typedef struct {
  http1_parser_s parser;
  fio_stream_s out;
  struct {
    char *name;
    char *value;
    int32_t name_len;
    int32_t value_len;
  } headers[HTTP_MAX_HEADERS];
  char *method;
  char *path;
  char *body;
  int method_len;
  int path_len;
  int headers_len;
  int body_len;
  int buf_pos;
  int buf_consumed;
  int fd;
  char buf[]; /* header and data buffer */
} client_s;

#define FIO_MEMORY_NAME fio_client_memory_allocator
#define FIO_REF_NAME    client
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_FLEX_TYPE char
#define FIO_REF_DESTROY(obj)                                                   \
  do {                                                                         \
    fio_stream_destroy(&(obj).out);                                            \
  } while (0)
#include "fio-stl.h"

static void http_send_response(client_s *c,
                               int status,
                               fio_str_info_s status_str,
                               size_t header_count,
                               fio_str_info_s headers[][2],
                               fio_str_info_s body);

/* *****************************************************************************
IO callback(s)
***************************************************************************** */

/** Called when the socket have available space in the outgoing buffer. */
FIO_SFUNC void on_ready(int fd, void *arg) {
  if (!arg)
    return;
  client_s *c = arg;
  char mem[32768];
  size_t len = 32768;
  /* send as much data as we can until the system buffer is full */
  do {
    /* set buffer to copy to, in case a copy is performed */
    char *buf = mem;
    len = 32768;
    /* read from the stream, copy might not be required. updates buf and len. */
    fio_stream_read(&c->out, &buf, &len);
    /* write to the IO object */
    if (!len || fio_sock_write(fd, buf, len) <= 0)
      goto finish;
    /* advance the stream by the amount actually written to the IO (partial?) */
    fio_stream_advance(&c->out, len);
    /* log */
    FIO_LOG_DEBUG2("on_ready send %zu bytes to %d.", len, fd);
  } while (len);

finish:
  /* if there's data left to write, monitor the outgoing buffer. */
  if (fio_stream_any(&c->out))
    fio_poll_monitor(&monitor, fd, arg, POLLOUT);
}

/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(int fd, void *arg) {
  client_s *c = arg;
  if (!arg)
    goto accept_new_connections;
  ssize_t r =
      fio_sock_read(fd, c->buf + c->buf_pos, HTTP_CLIENT_BUFFER - c->buf_pos);
  if (r > 0) {
    c->buf_pos += r;
    c->buf[c->buf_pos] = 0;
    while ((r = http1_parse(&c->parser,
                            c->buf + c->buf_consumed,
                            (size_t)(c->buf_pos - c->buf_consumed)))) {
      c->buf_consumed += r;
      if (!http1_complete(&c->parser))
        break;
      if (c->buf_consumed == c->buf_pos)
        c->buf_pos = c->buf_consumed = 0;
      else {
        c->buf_pos = c->buf_pos - c->buf_consumed;
        memmove(c->buf, c->buf + c->buf_consumed, c->buf_pos);
        c->buf_consumed = 0;
      }
    }
  }

  /* remember to reschedule event monitoring (one-shot by design) */
  fio_poll_monitor(&monitor, fd, arg, POLLIN);
  return;

accept_new_connections : {
  int cl = accept(fd, NULL, NULL);
  if (cl == -1)
    goto accept_error;
  fio_sock_set_non_block(cl);
  c = client_new(HTTP_CLIENT_BUFFER + 1);
  c->fd = cl;
  fio_poll_monitor(&monitor, cl, c, POLLIN | POLLOUT);
  /* remember to reschedule event monitoring (one-shot by design) */
  fio_poll_monitor(&monitor, fd, NULL, POLLIN);
  return;
}

accept_error:
  FIO_LOG_ERROR("Couldn't accept connection? %s", strerror(errno));
  /* remember to reschedule event monitoring (one-shot by design) */
  fio_poll_monitor(&monitor, fd, NULL, POLLIN);
}

/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(int fd, void *arg) {
  client_free(arg);
  if (arg) {
    fio_sock_close(fd);
  } else {
    FIO_LOG_DEBUG2("on_close callback called for %d, stopping.", fd);
    server_stop_flag = 1;
  }
}

/* *****************************************************************************
HTTP/1.1 callback(s)
***************************************************************************** */

/** called when a request was received. */
static int http1_on_request(http1_parser_s *parser) {
  client_s *c = (client_s *)parser;
#ifdef HTTP_RESPONSE_ECHO
  http_send_response(c,
                     200,
                     (fio_str_info_s){"OK", 2},
                     0,
                     NULL,
                     (fio_str_info_s){c->method, strlen(c->method)});

#elif HTTP_RESPONSE_DYNAMIC
  http_send_response(c,
                     200,
                     (fio_str_info_s){"OK", 2},
                     0,
                     NULL,
                     (fio_str_info_s){"Hello World!", 12});
#else
  char *response =
      "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World!\n";
  fio_stream_add(&c->out,
                 fio_stream_pack_data(response, strlen(response), 0, 0, NULL));
  (void)http_send_response; /* unused in this branch */
#endif

  /* reset client request data */
  c->method = NULL;
  c->path = NULL;
  c->body = NULL;
  c->method_len = 0;
  c->path_len = 0;
  c->headers_len = 0;
  c->body_len = 0;

  fio_poll_monitor(&monitor, c->fd, c, POLLOUT);
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
  fio_sock_close(c->fd);
  c->fd = -1;
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
  fio_stream_add(
      &c->out,
      fio_stream_pack_data(response,
                           final.len,
                           ((uintptr_t) final.buf) - (uintptr_t)response,
                           0,
                           (void (*)(void *))str_free));
  FIO_LOG_DEBUG2("Sending response %d, %zu bytes", status, final.len);
}
