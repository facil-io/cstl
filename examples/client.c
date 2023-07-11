/* *****************************************************************************
Copyright: Boaz Segev, 2019-2023
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

/* *****************************************************************************
This is a simple TCP/IP and Unix Socket client example. UDP is also available
but untested.

Note that this program uses a single thread, which allows it to ignore some
possible race conditions.
***************************************************************************** */

/* include some of the modules we use... */
#define FIO_CLI
#define FIO_LOG
#define FIO_SERVER
#define FIO_HTTP
#include "fio-stl/include.h"

/** Called When the client socket is attached to the server. */
FIO_SFUNC void on_attach(fio_s *io);
/** Called there's incoming data (from the server). */
FIO_SFUNC void on_data(fio_s *io);
/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *udata);

/** Socket client protocol */
static fio_protocol_s CLIENT_PROTOCOL = {
    .on_attach = on_attach,
    .on_data = on_data,
    .on_close = on_close,
    .on_pubsub = FIO_ON_MESSAGE_SEND_MESSAGE,
    .timeout = 30,
};

/** Called there's incoming data (from STDIN / the client socket). */
FIO_SFUNC void on_input(fio_s *io);
/** Called if connection failed to establish. */
FIO_SFUNC void on_failed(void *arg);

/** STDIN protocol (REPL) */
static fio_protocol_s STDIN_PROTOCOL = {
    .on_data = on_input,
};

/* Opens the client connection after the server starts (avoid SIGPIPE) */
FIO_SFUNC void open_client_connection(void *is_http);

/* *****************************************************************************
The main code.
***************************************************************************** */

int main(int argc, char const *argv[]) {
  /* initialize the CLI options */
  fio_cli_start(
      argc,
      argv,
      1, /* require 1 unnamed argument - the address to connect to */
      1,
      "A simple TCP/IP, Unix or UDP client application. Requires a URL "
      "type address. i.e.\n"
      "\tNAME <url>\n\n"
      "Unix socket examples:\n"
      "\tNAME unix://./my.sock\n"
      "\tNAME /full/path/to/my.sock\n"
      "\nTCP/IP socket examples:\n"
      "\tNAME tcp://localhost:3000/\n"
      "\tNAME localhost://3000\n"
      "\nUDP socket examples:\n"
      "\tNAME udp://localhost:3000/\n",
      FIO_CLI_INT("--timeout -t (50) ongoing connection timeout in seconds."),
      FIO_CLI_INT("--wait -w (5) connection attempt timeout in seconds."),
      FIO_CLI_BOOL("--verbose -V -d print out debugging messages."));

  /* review CLI for logging */
  if (fio_cli_get_bool("-V"))
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  /* review connection timeout */
  if (fio_cli_get_i("-t") > 0)
    CLIENT_PROTOCOL.timeout = (uint32_t)fio_cli_get_i("-t") * 1000;

  void *is_http = NULL;

  /* review CLI connection address (in URL format) */
  if (!fio_cli_unnamed(0) && fio_cli_get("-b"))
    fio_cli_set_unnamed(0, fio_cli_get("-b"));
  FIO_ASSERT(fio_cli_unnamed(0), "client address missing");
  size_t url_len = strlen(fio_cli_unnamed(0));
  FIO_ASSERT(url_len, "client address too short");
  FIO_ASSERT(url_len < 1024, "URL address too long");
  { /* select connection client style / protocol */
    fio_url_s url = fio_url_parse(fio_cli_unnamed(0), url_len);
    if (((url.scheme.len == 4 ||
          (url.scheme.len == 5 && ((url.scheme.buf[4] | 0x20) == 's'))) &&
         fio_buf2u32u("http") ==
             (fio_buf2u32u(url.scheme.buf) | 0x20202020UL)) ||
        ((url.scheme.len == 2 ||
          (url.scheme.len == 3 && ((url.scheme.buf[2] | 0x20) == 's'))) &&
         fio_buf2u16u("ws") == (fio_buf2u16u(url.scheme.buf) | 0x2020UL)) ||
        ((url.scheme.len == 3 ||
          (url.scheme.len == 4 && ((url.scheme.buf[3] | 0x20) == 's'))) &&
         fio_buf2u32u("sse\xFF") == (fio_buf2u32u(url.scheme.buf) |
                                     fio_buf2u32u("\x20\x20\x20\xFF")))) {
      is_http = (void *)1;
    }
  }
  /* set connection task in master process. */
  fio_state_callback_add(FIO_CALL_ON_START, open_client_connection, is_http);
  /* start server, connection termination will stop it. */
  fio_srv_start(0);
  FIO_LOG_INFO("* connection terminated.\n");
  return 0;
}

/* *****************************************************************************
Opening the client connection.
***************************************************************************** */

FIO_SFUNC void open_client_connection(void *is_http) {

  if (is_http) {
    /* TODO! HTTP / WebSocket / SSE client */
    FIO_LOG_FATAL("HTTP, WebSocket and SSE clients aren't supported yet"
                  "\n\t\tfor URL: %s",
                  fio_cli_unnamed(0));
    fio_srv_stop();
  } else {
    FIO_ASSERT(fio_srv_connect(fio_cli_unnamed(0),
                               .protocol = &CLIENT_PROTOCOL,
                               .on_failed = on_failed,
                               .timeout = (fio_cli_get_i("-w") * 1000)),
               "Connection error!");
  }
}

/* *****************************************************************************
IO callback(s)
***************************************************************************** */

/** Called When the client socket is attached to the server. */
FIO_SFUNC void on_attach(fio_s *io) {
  fio_subscribe(.io = io, .channel = FIO_BUF_INFO1("client"));
  fio_udata_set(io, (void *)1);
  FIO_LOG_INFO("* connection established.\n");
  FIO_LOG_DEBUG2("Connected client IO to pub/sub");
  /* attach STDIN */
  FIO_LOG_DEBUG2("listening to user input on STDIN.");
  fio_srv_attach_fd(fileno(stdin), &STDIN_PROTOCOL, NULL, NULL);
}
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io) {
  FIO_LOG_DEBUG2("on_data callback called for: %p", io);
  char buf[4080];
  for (;;) { /* read until done */
    size_t l = fio_read(io, buf, 4080);
    if (!l)
      return;
    fwrite(buf, 1, l, stdout); /* test for incomplete `write`? */
  }
}

/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *arg) {
  FIO_LOG_DEBUG2("Connection lost, shutting down client.");
  fio_srv_stop();
  (void)arg;
}

/** Called if connection failed to establish. */
FIO_SFUNC void on_failed(void *arg) {
  FIO_LOG_ERROR("Connection failed / no data received: %s", fio_cli_unnamed(0));
  on_close(arg);
}
/** Called there's incoming data (from STDIN / the client socket). */
FIO_SFUNC void on_input(fio_s *io) {
  struct {
    size_t len;
    char buf[4080];
  } info;
  for (;;) { /* read until done */
    info.len = fread(info.buf, 1, 4080, stdin);
    if (!info.len)
      return;
    FIO_LOG_DEBUG2("Publishing: %.*s", (int)info.len, info.buf);
    fio_publish(.from = io,
                .channel = FIO_BUF_INFO1("client"),
                .message = FIO_BUF_INFO2(info.buf, info.len));
  }
}
