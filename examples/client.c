/* *****************************************************************************
Copyright: Boaz Segev, 2019-2023
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

/* *****************************************************************************
This is a simple TCP/IP and Unix Socket client example. UDP is also available
but untested.

This demonstrates simple usage of the facil.io C STL core features for network
applications.

However, the facil.io IO core library might be a better (easier) choice for this
task.

Note that this program uses a single thread, which allows it to ignore some
possible race conditions.
***************************************************************************** */

/* include some of the modules we use... */
#define FIO_CLI
#define FIO_LOG
#define FIO_SERVER
#define FIO_HTTP
#include "fio-stl.h"

/** Called When the client socket is attached to the server. */
FIO_SFUNC void on_attach(fio_s *io);
/** Called (once) when the client socket's buffer is empty. */
FIO_SFUNC void on_ready(fio_s *io);
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io);
/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *udata);

/** Socket client protocol */
static fio_protocol_s CLIENT_PROTOCOL_CONNECTING = {
    .on_attach = on_attach,
    .on_ready = on_ready,
    .on_data = on_data,
    .on_close = on_close,
    .on_pubsub = FIO_ON_MESSAGE_SEND_MESSAGE,
};
/** Socket client protocol */
static fio_protocol_s CLIENT_PROTOCOL = {
    .on_data = on_data,
    .on_close = on_close,
    .on_pubsub = FIO_ON_MESSAGE_SEND_MESSAGE,
};

/** Called there's incoming data (from STDIN / the client socket). */
FIO_SFUNC void on_input(fio_s *io);

/** STDIN protocol (REPL) */
static fio_protocol_s STDIN_PROTOCOL = {
    .on_data = on_input,
};

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
      FIO_CLI_BOOL("--verbose -V -d print out debugging messages."));

  /* review CLI for logging */
  if (fio_cli_get_bool("-V")) {
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  }

  /* review CLI connection address (in URL format) */
  if (!fio_cli_unnamed(0) && fio_cli_get("-b"))
    fio_cli_set_unnamed(0, fio_cli_get("-b"));
  FIO_ASSERT(fio_cli_unnamed(0), "client address missing");
  size_t url_len = strlen(fio_cli_unnamed(0));
  FIO_ASSERT(url_len, "client address too short");
  FIO_ASSERT(url_len < 1024, "URL address too long");

  /* connect & attach STDIN */
  FIO_ASSERT(!fio_srv_connect(fio_cli_unnamed(0),
                              &CLIENT_PROTOCOL_CONNECTING,
                              NULL,
                              NULL),
             "Connection error");
  fio_srv_attach_fd(fileno(stdin), &STDIN_PROTOCOL, NULL, NULL);

  /* we're dome with the CLI, release resources */
  fio_srv_start(0);
  return 0;
}

/* *****************************************************************************
IO callback(s)
***************************************************************************** */

/** Called When the client socket is attached to the server. */
FIO_SFUNC void on_attach(fio_s *io) {
  fio_subscribe(.io = io, .channel = FIO_BUF_INFO1("client"));
  FIO_LOG_DEBUG2("Connected client IO to pub/sub");
}
/** Called When the client socket's buffer is empty. */
FIO_SFUNC void on_ready(fio_s *io) {
  printf("\t* connection established.\n");
  fio_protocol_set(io, &CLIENT_PROTOCOL);
}
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io) {
  FIO_LOG_DEBUG2("on_data callback called for: %p", io);
  fio_udata_set(io, (void *)1);
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
  if (!arg)
    FIO_LOG_ERROR("Connection failed / no data received: %s",
                  fio_cli_unnamed(0));
  FIO_LOG_DEBUG2("Connection lost, shutting down client.");
  fio_srv_stop();
  (void)arg;
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
