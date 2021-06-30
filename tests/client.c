/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
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

/* include some of the moduls we use... */
#define FIO_CLI
#define FIO_LOG
#define FIO_URL
#define FIO_SOCK
#define FIO_STREAM
#define FIO_SIGNAL
#define FIO_POLL
#include "fio-stl.h"

/* we use local global variables to make the code easier. */

/** This is used as a user-land buffer in case of partial `write` call. */
fio_stream_s output_stream = FIO_STREAM_INIT(output_stream);

/** A flag telling us when to stop reviewing IO events. */
static volatile uint8_t stop = 0;

/** The client / cconnection socket. */
int client_fd;

/** Called when the socket have available space in the outgoing buffer. */
FIO_SFUNC void on_ready(int fd, void *arg);
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(int fd, void *arg);
/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(int fd, void *arg);

/** The IO polling object - it keeps a one-shot list of monitored IOs. */
fio_poll_s monitor = FIO_POLL_INIT(on_data, on_ready, on_close);

/* facil.io delays signal callbacks so they can safely with no restrictions. */
FIO_SFUNC void on_signal(int sig, void *udata);

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
  size_t url_len = strlen(fio_cli_unnamed(0));
  FIO_ASSERT(url_len < 1024, "URL address too long");
  fio_url_s a = fio_url_parse(fio_cli_unnamed(0), url_len);
  if (!a.host.buf && !a.port.buf) {
#if FIO_OS_WIN
    FIO_ASSERT(0, "Unix style sockets are unsupported on Windows.");
#else
    /* Unix Socket */
    client_fd =
        fio_sock_open(a.path.buf,
                      NULL,
                      FIO_SOCK_UNIX | FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK);
    FIO_LOG_DEBUG("Opened a Unix Socket (%d).", client_fd);
#endif
  } else if (!a.scheme.buf || a.scheme.len != 3 ||
             (a.scheme.buf[0] | 32) != 'u' || (a.scheme.buf[1] | 32) != 'd' ||
             (a.scheme.buf[2] | 32) != 'p') {
    /* TCP/IP Socket */
    char buf[1024];
    /* copy because we need to add NUL bytes between the address and the port */
    memcpy(buf, a.host.buf, a.host.len + a.port.len + 2);
    buf[a.host.len + a.port.len + 1] = 0;
    buf[a.host.len] = 0;
    /* open the socket, passing NUL terminated strings for address and port */
    client_fd =
        fio_sock_open(buf,
                      buf + a.host.len + 1,
                      FIO_SOCK_TCP | FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK);
    /* log */
    FIO_LOG_DEBUG("Opened a TCP/IP Socket (%d) to %s port %s.",
                  client_fd,
                  buf,
                  buf + a.host.len + 1);
  } else {
    /* UDP Socket */
    char buf[1024];
    /* copy because we need to add NUL bytes between the address and the port */
    memcpy(buf, a.host.buf, a.host.len + a.port.len + 2);
    buf[a.host.len] = 0;
    buf[a.host.len + a.port.len + 1] = 0;
    /* open the socket, passing NUL terminated strings for address and port */
    client_fd =
        fio_sock_open(buf,
                      buf + a.host.len + 1,
                      FIO_SOCK_UDP | FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK);
    FIO_LOG_DEBUG("Opened a UDP Socket (%d).", client_fd);
  }

  /* we're dome with the CLI, release resources */
  fio_cli_end();

  /* test socket / connection sccess */
  if (client_fd == -1) {
    FIO_LOG_FATAL("Couldn't open connection");
  }

  /* select signals to be monitored */
  fio_signal_monitor(SIGINT, on_signal, NULL);
  fio_signal_monitor(SIGTERM, on_signal, NULL);
  fio_signal_monitor(SIGQUIT, on_signal, NULL);

  /* select IO objects to be monitored */
  fio_poll_monitor(&monitor, client_fd, NULL, POLLIN | POLLOUT);
  fio_poll_monitor(&monitor, fileno(stdin), (void *)1, POLLIN); /* mark STDIO */

  /* loop until the stop flag is raised */
  while (!stop) {
    /* review IO events (calls the registered callbacks) */
    fio_poll_review(&monitor, 1000);
    /* review signals (calls the registered callback) */
    fio_signal_review();
  }

  /* cleanup */
  fio_sock_close(client_fd);
  fio_poll_destroy(&monitor);
  return 0;
}

/* *****************************************************************************
Signal callback(s)
***************************************************************************** */

/* facil.io delays signal callbacks so they can safely with no restrictions. */
FIO_SFUNC void on_signal(int sig, void *udata) {
  /* since there are no restrictions, we can safely print to the log. */
  FIO_LOG_INFO("Exit signal %d detected", sig);
  /* If the signal repeats, crash. */
  if (fio_atomic_exchange(&stop, 1))
    exit(-1);
  (void)sig;
  (void)udata;
}

/* *****************************************************************************
IO callback(s)
***************************************************************************** */

/** Called when the socket have available space in the outgoing buffer. */
FIO_SFUNC void on_ready(int fd, void *arg) {
  FIO_LOG_DEBUG2("on_ready callback called for %d.", fd);
  char mem[4080];
  size_t len = 4080;
  /* send as much data as we can until the system buffer is full */
  do {
    /* set buffer to copy to, in case a copy is performed */
    char *buf = mem;
    len = 4080;
    /* read from the stream, copy might not be required. updates buf and len. */
    fio_stream_read(&output_stream, &buf, &len);
    /* write to the IO object */
    if (!len || fio_sock_write(fd, buf, len) <= 0)
      goto finish;
    /* advance the stream by the amount actually written to the IO (partial?) */
    fio_stream_advance(&output_stream, len);
    /* log */
    FIO_LOG_DEBUG2("on_ready send %zu bytes to %d.", len, fd);
  } while (len);

finish:
  /* if there's data left to write, monitor the outgoing buffer. */
  if (fio_stream_any(&output_stream))
    fio_poll_monitor(&monitor, fd, arg, POLLOUT);
}

/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(int fd, void *arg) {
  FIO_LOG_DEBUG2("on_data callback called for %d.", fd);
  char buf[4080];
  /* is this the STDIO file descriptor? (see `main` for details) */
  if (arg) {
    /* read from STDIO and add data to outgoing stream */
    ssize_t l = fio_sock_read(fd, buf, 4080);
    if (l > 0) {
      fio_stream_add(&output_stream,
                     fio_stream_pack_data(buf, (size_t)l, 0, 1, NULL));
      /* make sure the outgoing buffer is moniitored, so data is written. */
      fio_poll_monitor(&monitor, client_fd, NULL, POLLOUT);
    }
    FIO_LOG_DEBUG2("Read %zu bytes from %d", l, fd);
    goto done;
  }
  /* read data until non-blocking read operation fails. */
  for (;;) {
    ssize_t l = read(fd, buf, 4080);
    switch (l + 1) { /* -1 becomes 0 and 0 becomes 1. */
    case 0:          /* read returned -1, which means there was an error */
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ||
          errno == ENOTCONN) /* desired failures */
        goto done;
    /* fallthrough */
    case 1: /* read returned 0, which means we reached EOF */
      FIO_LOG_DEBUG("socket (%d) error / EOF, shutting down: %s",
                    fd,
                    strerror(errno));
      stop = 1;
      return;
    }
    /* log and print out to STDOUT  */
    FIO_LOG_DEBUG2("Read %zu bytes from %d", l, fd);
    fprintf(stdout, "%.*s", (int)l, buf);
    fflush(stdout);
    /* if we didn't fill the buffer, the incoming buffer MIGHT be empty. */
    if (l < 4080)
      goto done;
  }

done:
  /* remember to reschedule event monitoring (one-shot by design) */
  fio_poll_monitor(&monitor, fd, arg, POLLIN);
}

/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(int fd, void *arg) {
  stop = 1;
  FIO_LOG_DEBUG2("on_close callback called for %d, stopping.", fd);
  (void)arg;
}
