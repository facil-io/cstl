/* *****************************************************************************
Copyright: Boaz Segev, 2019-2023
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

#ifndef HTTP_RESPONSE_ECHO
/* Set to 0 to replace the echo response with "Hello World". */
#define HTTP_RESPONSE_ECHO 1
#endif

/* Lazy? let the linker drop any unused functions... */
#define FIO_EVERYTHING
#include "fio-stl/include.h"

// clang-format off
/* *****************************************************************************
This is a simple HTTP "Hello World" / echo server example.

WebSocket connections join a simple chat server in this example.

Benchmark with keep-alive:

    ab -c 200 -t 4 -n 1000000 -k http://127.0.0.1:3000/
    wrk -c200 -d4 -t2 http://localhost:3000/

Connect to chat server with WebSockets:

    ws = new WebSocket("ws://" + document.location.host + document.location.pathname);
    ws.onmessage = function(e) {console.log("Got message!"); console.log(e.data);};
    ws.onclose = function(e) {console.log("closed")};
    ws.onopen = function(e) {ws.send("hi");};

Listen to chat messages with Server Sent Events (EventSource / SSE):

    const listener = new EventSource(document.location.href);
    listener.onmessage = (e) => { console.log(e); }
    listener.onevent = (e) => { console.log(e); }

***************************************************************************** */
// clang-format on
/* *****************************************************************************
WebSocket / SSE Callbacks
***************************************************************************** */

static void websocket_on_open(fio_http_s *h) { /* also for SSE connections */
  fio_http_subscribe(h, .filter = 1);
  FIO_STR_INFO_TMP_VAR(welcome, 256);
  fio_string_write2(&welcome,
                    NULL,
                    FIO_STRING_WRITE_STR1("Welcome to the chat room @"),
                    FIO_STRING_WRITE_HEX(fio_thread_getpid()));
  fio_http_write(h, .buf = welcome.buf, .len = welcome.len, .copy = 1);
}

static void websocket_on_message(fio_http_s *h,
                                 fio_buf_info_s msg,
                                 uint8_t is_text) {
  fio_publish(.filter = 1, .message = msg);
  (void)h, (void)is_text;
}

static void websocket_on_shutdown(fio_http_s *h) {
  /* works for both WebSocket and SSE connections */
  fio_http_write(h, .buf = "Server going away, goodbye!", .len = 27);
}

/* *****************************************************************************
HTTP Callbacks (see later)
***************************************************************************** */

static void http_respond(fio_http_s *h);

/* *****************************************************************************
Timers
***************************************************************************** */
static int heartbeat(void *i1, void *i2) {
  (void)i1, (void)i2;
  if (i1 && !fio_io_is_master())
    return -1;
  FIO_LOG_INFO("(%d) heartbeat", fio_io_pid());
  return 0;
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(int argc, char const *argv[]) {
  static fio_io_async_s http_queue; /* async queue for worker threads. */

  /* setup CLI options */
  fio_cli_start(
      argc,
      argv,
      0, /* don't require any unnamed arguments */
      1, /* allow up to 1 unnamed argument - the address to bind to */
      "HTTP echo example, using \x1B[1m" FIO_POLL_ENGINE_STR "\x1B[0m."
      "\nListens on the specified URL (defaults to 0.0.0.0:3000). i.e.\n"
      "\tNAME <url>\n\n"
      "Unix socket examples:\n"
      "\tNAME ./my.sock\n"
      "\tNAME unix://./my.sock\n"
      "\tNAME /full/path/to/my.sock\n"
      "\tNAME unix:///full/path/to/my.sock\n"
      "\nTCP/IP socket examples:\n"
      "\tNAME tcp://localhost:3000/\n"
      "\tNAME localhost:3000\n",

      FIO_CLI_PRINT_HEADER("Address Binding"),
      FIO_CLI_PRINT_LINE(
          "NOTE: also controlled by the ADDRESS or PORT environment vars."),
      FIO_CLI_STRING("-bind -b address to listen to in URL format."),
      FIO_CLI_INT("-port -p port number to listen to if URL is missing."),
      FIO_CLI_PRINT(
          "Note: these are optional and supersede previous instructions."),

      FIO_CLI_PRINT_HEADER("Concurrency"),
      FIO_CLI_INT("--threads -t number of worker threads to use."),
      FIO_CLI_INT("--workers -w number of worker processes to use."),

      FIO_CLI_PRINT_HEADER("HTTP"),
      FIO_CLI_STRING("--public -www public folder for static file service."),
      FIO_CLI_INT("--max-line -maxln (" FIO_MACRO2STR(
          FIO_HTTP_DEFAULT_MAX_LINE_LEN) ") per-header line limit, in bytes."),
      FIO_CLI_INT("--max-header -maxhd (" FIO_MACRO2STR(
          FIO_HTTP_DEFAULT_MAX_HEADER_SIZE) ") total header limit per request, "
                                            "in bytes."),
      FIO_CLI_INT("--max-body -maxbd (" FIO_MACRO2STR(
          FIO_HTTP_DEFAULT_MAX_BODY_SIZE) ") total message payload limit per "
                                          "request, in bytes."),
      FIO_CLI_INT("--keep-alive -k (" FIO_MACRO2STR(
          FIO_HTTP_DEFAULT_TIMEOUT) ") HTTP keep-alive timeout in seconds "
                                    "(0..255)"),
      FIO_CLI_BOOL("--log -v log HTTP messages."),

      FIO_CLI_PRINT_HEADER("WebSocket / SSE"),
      FIO_CLI_INT("--ws-max-msg -maxms (" FIO_MACRO2STR(
          FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE) ") incoming WebSocket message "
                                            "limit, in bytes."),
      FIO_CLI_INT("--timeout -ping (" FIO_MACRO2STR(
          FIO_HTTP_DEFAULT_TIMEOUT_LONG) ") WebSocket / SSE timeout, in "
                                         "seconds."),

      FIO_CLI_PRINT_HEADER("TLS / SSL"),
      FIO_CLI_PRINT_LINE(
          "NOTE: crashes if no crypto library implementation is found."),
      FIO_CLI_BOOL(
          "--tls-self -tls uses SSL/TLS with a self signed certificate."),
      FIO_CLI_STRING("--tls-name -name The host name for the SSL/TLS "
                     "certificate (if any)."),
      FIO_CLI_STRING("--tls-cert -cert The SSL/TLS certificate .pem file."),
      FIO_CLI_STRING("--tls-key -key The SSL/TLS private key .pem file."),
      FIO_CLI_STRING(
          "--tls-password -tls-pass The SSL/TLS password for the private key."),

      FIO_CLI_PRINT_HEADER("Clustering Pub/Sub"),
      FIO_CLI_INT("--broadcast -bp Cluster Broadcast Port."),
      FIO_CLI_STRING("--secret -scrt Cluster Secret."),
      FIO_CLI_PRINT("NOTE: also controlled by the SECRET and SECRET_LENGTH "
                    "environment vars."),

      FIO_CLI_PRINT_HEADER("Misc"),
      FIO_CLI_BOOL("--verbose -V -d print out debugging messages."),
      FIO_CLI_BOOL(
          "--contained -C attempts to handle possible container restrictions."),
      FIO_CLI_PRINT(
          "Containers sometimes impose file-system restrictions, i.e.,"),
      FIO_CLI_PRINT("the IPC Unix Socket might need to be placed in `/tmp`."),
      FIO_CLI_INT("--heartbeat -ph Prints a heartbeat every requested number "
                  "of seconds."),
      FIO_CLI_BOOL("-phw Prints the heartbeat also on worker processes."));

  /* review CLI for logging */
  if (fio_cli_get_bool("-V")) {
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  }
  if (fio_cli_get_bool("-phw") && !fio_cli_get_i("-ph"))
    fio_cli_set_i("-ph", 1);
  /* schedule heartbeat */
  if (fio_cli_get_i("-ph"))
    fio_io_run_every(.every = (fio_cli_get_i("-ph") * 1000),
                     .fn = heartbeat,
                     .udata1 = (void *)(uintptr_t)fio_cli_get_bool("-phw"),
                     .repetitions = -1);

  if (fio_cli_get_bool("-C")) { /* container - place pub/sub socket in tmp */
    char *u = (char *)fio_pubsub_ipc_url();
    memcpy((void *)(u + 7), "/tmp/", 5);
  }

  fio_io_restart_on_signal(SIGUSR1);

  /* Debug data: print type sizes */
  FIO_LOG_DEBUG2("IO overhead: %zu bytes", sizeof(fio_io_s) + 8);
  FIO_LOG_DEBUG2("HTTP connection overhead: %zu bytes state + %zu bytes buffer",
                 sizeof(fio___http_connection_s) + 8,
                 FIO_HTTP_DEFAULT_MAX_LINE_LEN);
  FIO_LOG_DEBUG2("HTTP handle overhead: %zu", sizeof(fio_http_s) + 8);
  FIO_LOG_DEBUG2("Total HTTP overhead: %zu+%zu bytes",
                 sizeof(fio_io_s) + sizeof(fio___http_connection_s) +
                     sizeof(fio_http_s) + 24,
                 FIO_HTTP_DEFAULT_MAX_LINE_LEN);
  if (fio_cli_get_i("-w"))
    FIO_LOG_DEBUG2("Hot restart signal: SIGUSR1");

  /* initialize Async HTTP queue */
  fio_io_async_attach(&http_queue, fio_io_workers(fio_cli_get_i("-t")));

  /* Clustering */
  if (fio_cli_get_i("-bp") > 0) {
    fio_buf_info_s scrt = FIO_BUF_INFO1((char *)fio_cli_get("-scrt"));
    fio_secret_set(scrt.buf, scrt.len, 0);
    fio_pubsub_broadcast_on_port(fio_cli_get_i("-bp"));
  }

  /* Test for TLS */
  fio_io_tls_s *tls = (fio_cli_get("--tls-cert") && fio_cli_get("--tls-key"))
                          ? fio_io_tls_cert_add(fio_io_tls_new(),
                                                fio_cli_get("--tls-name"),
                                                fio_cli_get("--tls-cert"),
                                                fio_cli_get("--tls-key"),
                                                fio_cli_get("-tls-pass"))
                      : fio_cli_get("-tls")
                          ? fio_io_tls_cert_add(fio_io_tls_new(),
                                                fio_cli_get("-tls-name"),
                                                NULL,
                                                NULL,
                                                NULL)
                          : NULL;
  /* support -b and -p for when a URL isn't provided */
  if (!fio_cli_get("-b"))
    fio_cli_set(fio_cli_get("-b"), fio_cli_unnamed(0));
  if (fio_cli_get("-p")) {
    fio_buf_info_s tmp = fio_cli_get_str("-b");
    if (tmp.buf) {
      FIO_STR_INFO_TMP_VAR(url, 2048);
      FIO_ASSERT(tmp.len < 2000, "binding address / url too long.");
      fio_url_s u = fio_url_parse(tmp.buf, tmp.len);
      tmp.buf = (char *)fio_cli_get("-p");
      tmp.len = strlen(tmp.buf);
      FIO_ASSERT(tmp.len < 6, "port number too long.");
      fio_string_write2(&url,
                        NULL,
                        FIO_STRING_WRITE_STR2(u.scheme.buf, u.scheme.len),
                        (u.scheme.len ? FIO_STRING_WRITE_STR2("://", 3)
                                      : FIO_STRING_WRITE_STR2(NULL, 0)),
                        FIO_STRING_WRITE_STR2(u.host.buf, u.host.len),
                        FIO_STRING_WRITE_STR2(":", 1),
                        FIO_STRING_WRITE_STR2(tmp.buf, tmp.len),
                        (u.query.len ? FIO_STRING_WRITE_STR2("?", 1)
                                     : FIO_STRING_WRITE_STR2(NULL, 0)),
                        FIO_STRING_WRITE_STR2(u.query.buf, u.query.len));
      fio_cli_set("-b", url.buf);
    } else {
      setenv("PORT", fio_cli_get("-p"), 1);
    }
  }

  /* listen to incoming HTTP connections */
  void *listener = fio_http_listen(
      fio_cli_get("-b"),
      .on_http = http_respond,
      .on_authenticate_sse = FIO_HTTP_AUTHENTICATE_ALLOW,
      .on_authenticate_websocket = FIO_HTTP_AUTHENTICATE_ALLOW,
      .on_open = websocket_on_open,
      .on_message = websocket_on_message,
      .on_shutdown = websocket_on_shutdown,
      .public_folder = fio_cli_get("-www")
                           ? FIO_STR_INFO1((char *)fio_cli_get("-www"))
                           : FIO_STR_INFO2(NULL, 0),
      .max_age = 0,
      .max_header_size = fio_cli_get_i("-maxhd"),
      .max_line_len = fio_cli_get_i("-maxln"),
      .max_body_size = fio_cli_get_i("-maxbd"),
      .ws_max_msg_size = fio_cli_get_i("-maxms"),
      .ws_timeout = fio_cli_get_i("-ping"),
      .sse_timeout = fio_cli_get_i("-ping"),
      .timeout = fio_cli_get_i("-k"),
      .queue = &http_queue,
      .tls = tls,
      .log = fio_cli_get_bool("-v"));
  FIO_ASSERT(listener, "Could not open listening socket as requested.");
  FIO_ASSERT(fio_http_listener_settings(listener)->on_http == http_respond,
             "HTP listener error.");

  /* we don't need the tls object any more. */
  fio_io_tls_free(tls);

  FIO_LOG_INFO(
      "\n\tStarting HTTP echo server example app."
      "\n\tEngine: " FIO_POLL_ENGINE_STR "\n\tWorkers: %d\t(%s)"
      "\n\tThreads: 1+%d\t(per worker)"
      "\n\tPress ^C to exit.",
      fio_io_workers(fio_cli_get_i("-w")),
      (fio_io_workers(fio_cli_get_i("-w")) ? "cluster mode" : "single process"),
      (int)http_queue.count);

  /* start server reactor */
  fio_io_start(fio_cli_get_i("-w"));

  /* shutdown finished */
  FIO_LOG_INFO("Shutdown complete.");
  return 0;
}

/* *****************************************************************************
HTTP/1.1 response authorship helper
***************************************************************************** */

FIO_SFUNC int http_write_headers_to_string(fio_http_s *h,
                                           fio_str_info_s name,
                                           fio_str_info_s value,
                                           void *out_) {
  char **out = (char **)out_;
  *out = fio_bstr_write2(*out,
                         FIO_STRING_WRITE_STR2(name.buf, name.len),
                         FIO_STRING_WRITE_STR2(": ", 2),
                         FIO_STRING_WRITE_STR2(value.buf, value.len),
                         FIO_STRING_WRITE_STR2("\r\n", 2));
  return 0;
  (void)h;
}

/* *****************************************************************************
HTTP/1.1 response callback
***************************************************************************** */

static void http_respond(fio_http_s *h) {
  fio_http_response_header_set(h,
                               FIO_STR_INFO1("server"),
                               FIO_STR_INFO1("facil.io"));
  if (0) { /* setting cookie / header data example */
    FIO_STR_INFO_TMP_VAR(tmp, 64);
    fio_string_write_hex(&tmp, NULL, fio_rand64());
    fio_http_cookie_set(h, .name = FIO_STR_INFO1("fio-rand"), .value = tmp);
    fio_http_response_header_set(h, FIO_STR_INFO1("x-fio-rand"), tmp);
  }
#if HTTP_RESPONSE_ECHO /* write request to string to be sent as response */
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
  /* writes each header to `out` */
  fio_http_request_header_each(h, http_write_headers_to_string, &out);
  /* write body */
  if (fio_http_body_length(h)) {
    fio_http_body_seek(h, 0);
    fio_str_info_s body = fio_http_body_read(h, (size_t)-1);
    FIO_LOG_DDEBUG2("writing HTTP body echo to memory (%zu bytes)", body.len);
    out = fio_bstr_write2(out,
                          FIO_STRING_WRITE_STR2("\r\n", 2),
                          FIO_STRING_WRITE_STR2(body.buf, body.len),
                          FIO_STRING_WRITE_STR2("\r\n", 2));
  }
  /* fio_io_env_set(io, ...) example */
  if (0) {
    fio_io_env_set(fio_http_io(h),
                   .name = FIO_BUF_INFO2("my key", 6),
                   .udata = fio_bstr_write(NULL, "my env data", 11),
                   .on_close = (void (*)(void *))fio_bstr_free);
  }
  /* ETag header example - setting an ETag automatically enables caching */
  if (1) {
    uint64_t hash =
        fio_risky_hash(fio_http_path(h).buf, fio_http_path(h).len, 0);
    char hash_buf[18];
    fio_str_info_s etag = FIO_STR_INFO3(hash_buf, 0, 18);
    fio_string_write_hex(&etag, NULL, hash);
    fio_http_response_header_set(h, FIO_STR_INFO2("etag", 4), etag);
  }
  FIO_LOG_DDEBUG2("echoing back:\n%s", out);
  fio_http_write(h,
                 .buf = out,
                 .len = fio_bstr_len(out),
                 .dealloc = (void (*)(void *))fio_bstr_free,
                 .copy = 0,
                 .finish = 1);
#else  /* HTTP_RESPONSE_ECHO */
  fio_http_write(h, .buf = "Hello World!", .len = 12, .finish = 1);
#endif /* HTTP_RESPONSE_ECHO */
}

/* *****************************************************************************
Pub/Sub Logger / Recorder
***************************************************************************** */
#if 0
FIO_SFUNC void logger_detached(const fio_pubsub_engine_s *eng) {
  FIO_LOG_INFO("%d (logger) detached", fio_io_pid());
  (void)eng;
}
FIO_SFUNC void logger_on_msg(fio_msg_s *msg) {
  FIO_LOG_INFO("%d (logger) pub/sub message for %s (%d):\n%s",
               fio_io_pid(),
               (msg->channel.len ? msg->channel.buf : "<null>"),
               (int)msg->filter,
               (msg->message.len ? msg->message.buf : "<null>"));
}
FIO_SFUNC void logger_publish(const fio_pubsub_engine_s *eng, fio_msg_s *msg) {
  logger_on_msg(msg);
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = msg->channel,
              .message = msg->message,
              .filter = msg->filter,
              .is_json = msg->is_json);
  (void)eng;
}

fio_pubsub_engine_s FIO_PUBSUB_LOGGER = {
    .detached = logger_detached,
    .publish = logger_publish,
};

FIO_CONSTRUCTOR(pubsub_logger) {
  FIO_PUBSUB_DEFAULT = &FIO_PUBSUB_LOGGER;
  fio_pubsub_attach(&FIO_PUBSUB_LOGGER);
  fio_subscribe(.filter = 1, .on_message = logger_on_msg);
}
#endif
