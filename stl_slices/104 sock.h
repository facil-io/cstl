/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_SOCK                    /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                        Basic Socket Helpers / IO Polling



Example:
********************************************************************************

#define FIO_SOCK
#define FIO_CLI
#define FIO_LOG
#include "fio-stl.h" // __FILE__

typedef struct {
  int fd;
  unsigned char is_client;
} state_s;

static void on_data_server(int fd, size_t index, void *udata) {
  (void)udata; // unused for server
  (void)index; // we don't use the array index in this example
  char buf[65536];
  FIO_MEMCPY(buf, "echo: ", 6);
  ssize_t len = 0;
  struct sockaddr_storage peer;
  socklen_t peer_addrlen = sizeof(peer);
  len = recvfrom(fd, buf + 6, (65536 - 7), 0, (struct sockaddr *)&peer,
                 &peer_addrlen);
  if (len <= 0)
    return;
  buf[len + 6] = 0;
  fprintf(stderr, "Recieved: %s", buf + 6);
  // sends all data in UDP, with TCP sending may be partial
  len =
      sendto(fd, buf, len + 6, 0, (const struct sockaddr *)&peer, peer_addrlen);
  if (len < 0)
    perror("error");
}

static void on_data_client(int fd, size_t index, void *udata) {
  state_s *state = (state_s *)udata;
  fprintf(stderr, "on_data_client %zu\n", index);
  if (!index) // stdio is index 0 in the fd list
    goto is_stdin;
  char buf[65536];
  ssize_t len = 0;
  struct sockaddr_storage peer;
  socklen_t peer_addrlen = sizeof(peer);
  len = recvfrom(fd, buf, 65535, 0, (struct sockaddr *)&peer, &peer_addrlen);
  if (len <= 0)
    return;
  buf[len] = 0;
  fprintf(stderr, "%s", buf);
  return;
is_stdin:
  len = read(fd, buf, 65535);
  if (len <= 0)
    return;
  buf[len] = 0;
  // sends all data in UDP, with TCP sending may be partial
  len = send(state->fd, buf, len, 0);
  fprintf(stderr, "Sent: %zd bytes\n", len);
  if (len < 0)
    perror("error");
  return;
  (void)udata;
}

int main(int argc, char const *argv[]) {
  // Using CLI to set address, port and client/server mode.
  fio_cli_start(
      argc, argv, 0, 0, "UDP echo server / client example.",
      FIO_CLI_PRINT_HEADER("Address Binding"),
      FIO_CLI_STRING("-address -b address to listen / connect to."),
      FIO_CLI_INT("-port -p port to listen / connect to. Defaults to 3030."),
      FIO_CLI_PRINT_HEADER("Operation Mode"),
      FIO_CLI_BOOL("-client -c Client mode."),
      FIO_CLI_BOOL("-verbose -v verbose mode (debug messages on)."));

  if (fio_cli_get_bool("-v"))
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  fio_cli_set_default("-p", "3030");

  // Using FIO_SOCK functions for setting up UDP server / client
  state_s state = {.is_client = fio_cli_get_bool("-c")};
  state.fd = fio_sock_open(
      fio_cli_get("-b"), fio_cli_get("-p"),
      FIO_SOCK_UDP | FIO_SOCK_NONBLOCK |
          (fio_cli_get_bool("-c") ? FIO_SOCK_CLIENT : FIO_SOCK_SERVER));

  if (state.fd == -1) {
    FIO_LOG_FATAL("Couldn't open socket!");
    exit(1);
  }
  FIO_LOG_DEBUG("UDP socket open on fd %d", state.fd);

  if (state.is_client) {
    int i =
        send(state.fd, "Client hello... further data will be sent using REPL\n",
             53, 0);
    fprintf(stderr, "Sent: %d bytes\n", i);
    if (i < 0)
      perror("error");
    while (fio_sock_poll(.on_data = on_data_client, .udata = (void *)&state,
                         .timeout = 1000,
                         .fds = FIO_SOCK_POLL_LIST(
                             FIO_SOCK_POLL_R(fileno(stdin)),
                             FIO_SOCK_POLL_R(state.fd))) >= 0)
      ;
  } else {
    while (fio_sock_poll(.on_data = on_data_server, .udata = (void *)&state,
                         .timeout = 1000,
                         .fds = FIO_SOCK_POLL_LIST(
                             FIO_SOCK_POLL_R(state.fd))) >= 0)
      ;
  }
  // we should cleanup, though we'll exit with Ctrl+C, so it's won't matter.
  fio_cli_end();
  fio_sock_close(state.fd);
  return 0;
  (void)argv;
}


***************************************************************************** */
#if defined(FIO_SOCK) && !defined(FIO_SOCK_POLL_LIST)

/* *****************************************************************************
OS specific patches.
***************************************************************************** */
#if FIO_OS_WIN
#if _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef FIO_SOCK_FD_ISVALID
#define FIO_SOCK_FD_ISVALID(fd) ((size_t)fd <= (size_t)0x7FFFFFFF)
#endif
/** Acts as POSIX write. Use this macro for portability with WinSock2. */
#define fio_sock_write(fd, data, len) send((fd), (data), (len), 0)
/** Acts as POSIX read. Use this macro for portability with WinSock2. */
#define fio_sock_read(fd, buf, len) recv((fd), (buf), (len), 0)
/** Acts as POSIX close. Use this macro for portability with WinSock2. */
#define fio_sock_close(fd) closesocket(fd)
/** Protects against type size overflow on Windows, where FD > MAX_INT. */
FIO_IFUNC int fio_sock_accept(int s, struct sockaddr *addr, int *addrlen) {
  int r = -1;
  SOCKET c = accept(s, addr, addrlen);
  if (c == INVALID_SOCKET)
    return r;
  if (FIO_SOCK_FD_ISVALID(c)) {
    r = (int)c;
    return r;
  }
  closesocket(c);
  errno = ERANGE;
  FIO_LOG_ERROR("Windows SOCKET value overflowed int limits (was: %zu)",
                (size_t)c);
  return r;
}
#define accept fio_sock_accept

#elif FIO_HAVE_UNIX_TOOLS
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#ifndef FIO_SOCK_FD_ISVALID
#define FIO_SOCK_FD_ISVALID(fd) ((int)fd != (int)-1)
#endif
/** Acts as POSIX write. Use this macro for portability with WinSock2. */
#define fio_sock_write(fd, data, len) write((fd), (data), (len))
/** Acts as POSIX read. Use this macro for portability with WinSock2. */
#define fio_sock_read(fd, buf, len)   read((fd), (buf), (len))
/** Acts as POSIX close. Use this macro for portability with WinSock2. */
#define fio_sock_close(fd)            close(fd)
#else
#error FIO_SOCK requires a supported OS (Windows / POSIX).
#endif

/* *****************************************************************************
IO Poll - API
***************************************************************************** */
#define FIO_SOCK_POLL_RW(fd_)                                                  \
  (struct pollfd) { .fd = fd_, .events = (POLLIN | POLLOUT) }
#define FIO_SOCK_POLL_R(fd_)                                                   \
  (struct pollfd) { .fd = fd_, .events = POLLIN }
#define FIO_SOCK_POLL_W(fd_)                                                   \
  (struct pollfd) { .fd = fd_, .events = POLLOUT }
#define FIO_SOCK_POLL_LIST(...)                                                \
  (struct pollfd[]) {                                                          \
    __VA_ARGS__, (struct pollfd) { .fd = -1 }                                  \
  }

typedef enum {
  FIO_SOCK_SERVER = 0,
  FIO_SOCK_CLIENT = 1,
  FIO_SOCK_NONBLOCK = 2,
  FIO_SOCK_TCP = 4,
  FIO_SOCK_UDP = 8,
#if FIO_OS_POSIX
  FIO_SOCK_UNIX = 16,
#endif
} fio_sock_open_flags_e;

/**
 * Creates a new socket according to the provided flags.
 *
 * The `port` string will be ignored when `FIO_SOCK_UNIX` is set.
 */
FIO_IFUNC int fio_sock_open(const char *restrict address,
                            const char *restrict port,
                            uint16_t flags);

/** Creates a new socket, according to the provided flags. */
SFUNC int fio_sock_open2(const char *url, uint16_t flags);

/**
 * Attempts to resolve an address to a valid IP6 / IP4 address pointer.
 *
 * The `sock_type` element should be a socket type, such as `SOCK_DGRAM` (UDP)
 * or `SOCK_STREAM` (TCP/IP).
 *
 * The address should be freed using `fio_sock_address_free`.
 */
FIO_IFUNC struct addrinfo *fio_sock_address_new(const char *restrict address,
                                                const char *restrict port,
                                                int sock_type);

/** Frees the pointer returned by `fio_sock_address_new`. */
FIO_IFUNC void fio_sock_address_free(struct addrinfo *a);

/** Creates a new network socket and binds it to a local address. */
SFUNC int fio_sock_open_local(struct addrinfo *addr, int nonblock);

/** Creates a new network socket and connects it to a remote address. */
SFUNC int fio_sock_open_remote(struct addrinfo *addr, int nonblock);

#if FIO_OS_POSIX
/** Creates a new Unix socket and binds it to a local address. */
SFUNC int fio_sock_open_unix(const char *address, int is_client, int nonblock);
#endif

/** Sets a file descriptor / socket to non blocking state. */
SFUNC int fio_sock_set_non_block(int fd);

/**
 * Returns 0 on timeout, -1 on error or the events that are valid.
 *
 * Possible events are POLLIN | POLLOUT
 */
SFUNC short fio_sock_wait_io(int fd, short events, int timeout);

/** A helper macro that waits on a single IO with no callbacks (0 = no event) */
#define FIO_SOCK_WAIT_RW(fd, timeout_)                                         \
  fio_sock_wait_io(fd, POLLIN | POLLOUT, timeout_)

/** A helper macro that waits on a single IO with no callbacks (0 = no event) */
#define FIO_SOCK_WAIT_R(fd, timeout_) fio_sock_wait_io(fd, POLLIN, timeout_)

/** A helper macro that waits on a single IO with no callbacks (0 = no event) */
#define FIO_SOCK_WAIT_W(fd, timeout_) fio_sock_wait_io(fd, POLLOUT, timeout_)

/* *****************************************************************************
Small Poll API
***************************************************************************** */

typedef struct {
  /** Called after polling but before any events are processed. */
  void (*before_events)(void *udata);
  /** Called when the fd can be written too (available outgoing buffer). */
  void (*on_ready)(int fd, size_t index, void *udata);
  /** Called when data iis available to be read from the fd. */
  void (*on_data)(int fd, size_t index, void *udata);
  /** Called on error or when the fd was closed. */
  void (*on_error)(int fd, size_t index, void *udata);
  /** Called after polling and after all events are processed. */
  void (*after_events)(void *udata);
  /** An opaque user data pointer. */
  void *udata;
  /** A pointer to the fd pollin array. */
  struct pollfd *fds;
  /**
   * the number of fds to listen to.
   *
   * If zero, and `fds` is set, it will be auto-calculated trying to find the
   * first array member where `events == 0`. Make sure to supply this end
   * marker, of the buffer may overrun!
   */
  uint32_t count;
  /** timeout for the polling system call. */
  int timeout;
} fio_sock_poll_args;

/**
 * The `fio_sock_poll` function uses the `poll` system call to poll a simple IO
 * list.
 *
 * The list must end with a `struct pollfd` with it's `events` set to zero. No
 * other member of the list should have their `events` data set to zero.
 *
 * It is recommended to use the `FIO_SOCK_POLL_LIST(...)` and
 * `FIO_SOCK_POLL_[RW](fd)` macros. i.e.:
 *
 *     int count = fio_sock_poll(.on_ready = on_ready,
 *                         .on_data = on_data,
 *                         .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(io_fd)));
 *
 * NOTE: The `poll` system call should perform reasonably well for light loads
 * (short lists). However, for complex IO needs or heavier loads, use the
 * system's native IO API, such as kqueue or epoll.
 */
FIO_IFUNC int fio_sock_poll(fio_sock_poll_args args);
#define fio_sock_poll(...) fio_sock_poll((fio_sock_poll_args){__VA_ARGS__})

/* *****************************************************************************
IO Poll - Implementation (always static / inlined)
***************************************************************************** */

FIO_SFUNC void fio___sock_poll_mock_ev(int fd, size_t index, void *udata) {
  (void)fd;
  (void)index;
  (void)udata;
}

int fio_sock_poll____(void); /* sublime text marker */
FIO_IFUNC int fio_sock_poll FIO_NOOP(fio_sock_poll_args args) {
  size_t event_count = 0;
  size_t limit = 0;
  if (!args.fds)
    goto empty_list;
  if (!args.count)
    while (args.fds[args.count].events)
      ++args.count;
  if (!args.count)
    goto empty_list;

  /* move if statement out of loop using a move callback */
  if (!args.on_ready)
    args.on_ready = fio___sock_poll_mock_ev;
  if (!args.on_data)
    args.on_data = fio___sock_poll_mock_ev;
  if (!args.on_error)
    args.on_error = fio___sock_poll_mock_ev;
#if FIO_OS_WIN
  event_count = WSAPoll(args.fds, args.count, args.timeout);
#else
  event_count = poll(args.fds, args.count, args.timeout);
#endif
  if (args.before_events)
    args.before_events(args.udata);
  if (event_count <= 0)
    goto finish;
  for (size_t i = 0; i < args.count && limit < event_count; ++i) {
    if (!args.fds[i].revents)
      continue;
    ++limit;
    if ((args.fds[i].revents & POLLOUT))
      args.on_ready(args.fds[i].fd, i, args.udata);
    if ((args.fds[i].revents & POLLIN))
      args.on_data(args.fds[i].fd, i, args.udata);
    if ((args.fds[i].revents & (POLLERR | POLLNVAL)))
      args.on_error(args.fds[i].fd, i, args.udata); /* TODO: POLLHUP ? */
  }
finish:
  if (args.after_events)
    args.after_events(args.udata);
  return event_count;
empty_list:
  if (args.timeout)
    FIO_THREAD_WAIT(args.timeout);
  if (args.before_events)
    args.before_events(args.udata);
  if (args.after_events)
    args.after_events(args.udata);
  return 0;
}

/**
 * Creates a new socket according to the provided flags.
 *
 * The `port` string will be ignored when `FIO_SOCK_UNIX` is set.
 */
FIO_IFUNC int fio_sock_open(const char *restrict address,
                            const char *restrict port,
                            uint16_t flags) {
  struct addrinfo *addr = NULL;
  int fd;
  switch ((flags & ((uint16_t)FIO_SOCK_TCP | (uint16_t)FIO_SOCK_UDP
#if FIO_OS_POSIX
                    | (uint16_t)FIO_SOCK_UNIX
#endif
                    ))) {
  case FIO_SOCK_UDP:
    addr = fio_sock_address_new(address, port, SOCK_DGRAM);
    if (!addr) {
      FIO_LOG_ERROR("(fio_sock_open) address error: %s", strerror(errno));
      return -1;
    }
    if ((flags & FIO_SOCK_CLIENT)) {
      fd = fio_sock_open_remote(addr, (flags & FIO_SOCK_NONBLOCK));
    } else {
      fd = fio_sock_open_local(addr, (flags & FIO_SOCK_NONBLOCK));
    }
    fio_sock_address_free(addr);
    return fd;
  case FIO_SOCK_TCP:
    addr = fio_sock_address_new(address, port, SOCK_STREAM);
    if (!addr) {
      FIO_LOG_ERROR("(fio_sock_open) address error: %s", strerror(errno));
      return -1;
    }
    if ((flags & FIO_SOCK_CLIENT)) {
      fd = fio_sock_open_remote(addr, (flags & FIO_SOCK_NONBLOCK));
    } else {
      fd = fio_sock_open_local(addr, (flags & FIO_SOCK_NONBLOCK));
      if (fd != -1 && listen(fd, SOMAXCONN) == -1) {
        FIO_LOG_ERROR("(fio_sock_open) failed on call to listen: %s",
                      strerror(errno));
        fio_sock_close(fd);
        fd = -1;
      }
    }
    fio_sock_address_free(addr);
    return fd;
#if FIO_OS_POSIX
  case FIO_SOCK_UNIX:
    return fio_sock_open_unix(address,
                              (flags & FIO_SOCK_CLIENT),
                              (flags & FIO_SOCK_NONBLOCK));
#endif
  }
  FIO_LOG_ERROR("(fio_sock_open) the FIO_SOCK_TCP, FIO_SOCK_UDP, and "
                "FIO_SOCK_UNIX flags are exclusive");
  return -1;
}

FIO_IFUNC struct addrinfo *fio_sock_address_new(
    const char *restrict address,
    const char *restrict port,
    int sock_type /*i.e., SOCK_DGRAM */) {
  struct addrinfo addr_hints = (struct addrinfo){0}, *a;
  int e;
  addr_hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
  addr_hints.ai_socktype = sock_type;
  addr_hints.ai_flags = AI_PASSIVE; // use my IP

  if ((e = getaddrinfo(address, (port ? port : "0"), &addr_hints, &a)) != 0) {
    FIO_LOG_ERROR("(fio_sock_address_new(\"%s\", \"%s\")) error: %s",
                  (address ? address : "NULL"),
                  (port ? port : "0"),
                  gai_strerror(e));
    return NULL;
  }
  return a;
}

FIO_IFUNC void fio_sock_address_free(struct addrinfo *a) { freeaddrinfo(a); }

/* *****************************************************************************
FIO_SOCK - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE)

/** Creates a new socket, according to the provided flags. */
SFUNC int fio_sock_open2(const char *url, uint16_t flags) {
  char buf[2048];
  char port[64];
  char *addr = buf;
  char *pr = port;

  /* parse URL */
  fio_url_s u = fio_url_parse(url, strlen(url));
#if FIO_OS_POSIX
  if (!u.host.buf && !u.port.buf && u.path.buf) {
    /* unix socket */
    flags &= FIO_SOCK_SERVER | FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK;
    flags |= FIO_SOCK_UNIX;
    if (u.path.len >= 2048) {
      errno = EINVAL;
      FIO_LOG_ERROR("Couldn't open socket to %s - host name too long.", url);
      return -1;
    }
    FIO_MEMCPY(buf, u.path.buf, u.path.len);
    buf[u.path.len] = 0;
    pr = NULL;
  } else
#endif
  {
    if (!u.port.len)
      u.port = u.scheme;
    if (!u.port.len) {
      pr = NULL;
    } else {
      if (u.port.len >= 64) {
        errno = EINVAL;
        FIO_LOG_ERROR("Couldn't open socket to %s - port / scheme too long.",
                      url);
        return -1;
      }
      FIO_MEMCPY(port, u.port.buf, u.port.len);
      port[u.port.len] = 0;
      if (!(flags & (FIO_SOCK_TCP | FIO_SOCK_UDP))) {
        /* TODO? prefer...? TCP? */
        if (u.scheme.len == 3 && (u.scheme.buf[0] | 32) == 'u' &&
            (u.scheme.buf[1] | 32) == 'd' && (u.scheme.buf[2] | 32) == 'p')
          flags |= FIO_SOCK_UDP;
        else if (u.scheme.len == 3 && (u.scheme.buf[0] | 32) == 't' &&
                 (u.scheme.buf[1] | 32) == 'c' && (u.scheme.buf[2] | 32) == 'p')
          flags |= FIO_SOCK_TCP;
        else if ((u.scheme.len == 4 || u.scheme.len == 5) &&
                 (u.scheme.buf[0] | 32) == 'h' &&
                 (u.scheme.buf[1] | 32) == 't' &&
                 (u.scheme.buf[2] | 32) == 't' &&
                 (u.scheme.buf[3] | 32) == 'p' &&
                 (u.scheme.len == 4 ||
                  (u.scheme.len == 5 && (u.scheme.buf[4] | 32) == 's')))
          flags |= FIO_SOCK_TCP;
      }
    }
    if (u.host.len) {
      if (u.host.len >= 2048) {
        errno = EINVAL;
        FIO_LOG_ERROR("Couldn't open socket to %s - host name too long.", url);
        return -1;
      }
      FIO_MEMCPY(buf, u.host.buf, u.host.len);
      buf[u.host.len] = 0;
    } else {
      addr = NULL;
    }
  }
  return fio_sock_open(addr, pr, flags);
}

/** Sets a file descriptor / socket to non blocking state. */
SFUNC int fio_sock_set_non_block(int fd) {
/* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
  /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
  int flags;
  if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
    flags = 0;
#ifdef O_CLOEXEC
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC);
#else
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
#elif defined(FIONBIO)
  /* Otherwise, use the old way of doing it */
#if FIO_OS_WIN
  unsigned long flags = 1;
  if (ioctlsocket(fd, FIONBIO, &flags)) {
    switch (WSAGetLastError()) {
    case WSANOTINITIALISED:
      FIO_LOG_DEBUG("Windows non-blocking ioctl failed with WSANOTINITIALISED");
      break;
    case WSAENETDOWN:
      FIO_LOG_DEBUG("Windows non-blocking ioctl failed with WSAENETDOWN");
      break;
    case WSAEINPROGRESS:
      FIO_LOG_DEBUG("Windows non-blocking ioctl failed with WSAEINPROGRESS");
      break;
    case WSAENOTSOCK:
      FIO_LOG_DEBUG("Windows non-blocking ioctl failed with WSAENOTSOCK");
      break;
    case WSAEFAULT:
      FIO_LOG_DEBUG("Windows non-blocking ioctl failed with WSAEFAULT");
      break;
    }
    return -1;
  }
  return 0;
#else
  int flags = 1;
  return ioctl(fd, FIONBIO, &flags);
#endif /* FIO_OS_WIN */
#else
#error No functions / argumnet macros for non-blocking sockets.
#endif
}

/** Creates a new network socket and binds it to a local address. */
SFUNC int fio_sock_open_local(struct addrinfo *addr, int nonblock) {
  int fd = -1;
  for (struct addrinfo *p = addr; p != NULL; p = p->ai_next) {
#if FIO_OS_WIN
    SOCKET fd_tmp;
    if ((fd_tmp = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        INVALID_SOCKET) {
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
      continue;
    }
    if (!FIO_SOCK_FD_ISVALID(fd_tmp)) {
      FIO_LOG_DEBUG("windows socket value out of valid portable range.");
      errno = ERANGE;
    }
    fd = (int)fd_tmp;
#else
    if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
      continue;
    }
#endif
    {
      // avoid the "address taken"
      int optval = 1;
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));
    }
    if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
      FIO_LOG_DEBUG("Failed attempt to bind socket (%d) to address %s",
                    fd,
                    strerror(errno));
      fio_sock_close(fd);
      fd = -1;
      continue;
    }
    if (nonblock && fio_sock_set_non_block(fd) == -1) {
      FIO_LOG_DEBUG("Couldn't set socket (%d) to non-blocking mode %s",
                    fd,
                    strerror(errno));
      fio_sock_close(fd);
      fd = -1;
      continue;
    }
    break;
  }
  if (fd == -1) {
    FIO_LOG_DEBUG("socket binding/creation error %s", strerror(errno));
  }
  return fd;
}

/** Creates a new network socket and connects it to a remote address. */
SFUNC int fio_sock_open_remote(struct addrinfo *addr, int nonblock) {
  int fd = -1;
  for (struct addrinfo *p = addr; p != NULL; p = p->ai_next) {
#if FIO_OS_WIN
    SOCKET fd_tmp;
    if ((fd_tmp = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        INVALID_SOCKET) {
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
      continue;
    }
    if (!FIO_SOCK_FD_ISVALID(fd_tmp)) {
      FIO_LOG_DEBUG("windows socket value out of valid portable range.");
      errno = ERANGE;
    }
    fd = (int)fd_tmp;
#else
    if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
      continue;
    }
#endif

    if (nonblock && fio_sock_set_non_block(fd) == -1) {
      FIO_LOG_DEBUG(
          "Failed attempt to set client socket (%d) to non-blocking %s",
          fd,
          strerror(errno));
      fio_sock_close(fd);
      fd = -1;
      continue;
    }
    if (connect(fd, p->ai_addr, p->ai_addrlen) == -1 &&
#if FIO_OS_WIN
        (WSAGetLastError() != WSAEWOULDBLOCK || errno != EINPROGRESS)
#else
        errno != EINPROGRESS
#endif
    ) {
#if FIO_OS_WIN
      FIO_LOG_DEBUG(
          "Couldn't connect client socket (%d) to remote address %s (%d)",
          fd,
          strerror(errno),
          WSAGetLastError());
#else
      FIO_LOG_DEBUG("Couldn't connect client socket (%d) to remote address %s",
                    fd,
                    strerror(errno));
#endif
      fio_sock_close(fd);
      fd = -1;
      continue;
    }
    break;
  }
  if (fd == -1) {
    FIO_LOG_DEBUG("socket connection/creation error %s", strerror(errno));
  }
  return fd;
}

/** Returns 0 on timeout, -1 on error or the events that are valid. */
SFUNC short fio_sock_wait_io(int fd, short events, int timeout) {
  short r;
  struct pollfd pfd = {.fd = fd, .events = events};
#if FIO_OS_WIN
  r = (short)WSAPoll(&pfd, 1, timeout);
#else
  r = (short)poll(&pfd, 1, timeout);
#endif
  if (r == 1)
    r = events;
  return r;
}

#if FIO_OS_POSIX
/** Creates a new Unix socket and binds it to a local address. */
SFUNC int fio_sock_open_unix(const char *address, int is_client, int nonblock) {
  /* Unix socket */
  struct sockaddr_un addr = {0};
  size_t addr_len = strlen(address);
  if (addr_len >= sizeof(addr.sun_path)) {
    FIO_LOG_ERROR(
        "(fio_sock_open_unix) address too long (%zu bytes > %zu bytes).",
        addr_len,
        sizeof(addr.sun_path) - 1);
    errno = ENAMETOOLONG;
    return -1;
  }
  addr.sun_family = AF_UNIX;
  FIO_MEMCPY(addr.sun_path, address, addr_len + 1); /* copy the NUL byte. */
#if defined(__APPLE__)
  addr.sun_len = addr_len;
#endif
  // get the file descriptor
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    FIO_LOG_DEBUG("couldn't open unix socket (client? == %d) %s",
                  is_client,
                  strerror(errno));
    return -1;
  }
  /* chmod for foreign connections */
  fchmod(fd, S_IRWXO | S_IRWXG | S_IRWXU);
  if (nonblock && fio_sock_set_non_block(fd) == -1) {
    FIO_LOG_DEBUG("couldn't set socket to nonblocking mode");
    fio_sock_close(fd);
    return -1;
  }
  if (is_client) {
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1 &&
        errno != EINPROGRESS) {
      FIO_LOG_DEBUG("couldn't connect unix client: %s", strerror(errno));
      fio_sock_close(fd);
      return -1;
    }
  } else {
    unlink(addr.sun_path);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      FIO_LOG_DEBUG("couldn't bind unix socket to %s", address);
      // umask(old_umask);
      fio_sock_close(fd);
      return -1;
    }
    // umask(old_umask);
    if (listen(fd, SOMAXCONN) < 0) {
      FIO_LOG_DEBUG("couldn't start listening to unix socket at %s", address);
      fio_sock_close(fd);
      return -1;
    }
  }
  return fd;
}
#elif FIO_OS_WIN

/* UNIX Sockets?
 * https://devblogs.microsoft.com/commandline/af_unix-comes-to-windows/
 */

static WSADATA fio___sock_useless_windows_data;
FIO_CONSTRUCTOR(fio___sock_win_init) {
  static uint8_t flag = 0;
  if (!flag) {
    flag |= 1;
    if (WSAStartup(MAKEWORD(2, 2), &fio___sock_useless_windows_data)) {
      FIO_LOG_FATAL("WinSock2 unavailable.");
      exit(-1);
    }
    atexit((void (*)(void))(WSACleanup));
  }
}

// FIO_DESTRUCTOR void fio___sock_win_cleanup(void) { (); }
#endif /* FIO_OS_WIN / FIO_OS_POSIX */

/* *****************************************************************************
Socket helper testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void fio___sock_test_before_events(void *udata) {
  *(size_t *)udata = 0;
}
FIO_SFUNC void fio___sock_test_on_event(int fd, size_t index, void *udata) {
  *(size_t *)udata += 1;
  if (errno) {
    FIO_LOG_WARNING("(possibly expected) %s", strerror(errno));
    errno = 0;
  }
  (void)fd;
  (void)index;
}
FIO_SFUNC void fio___sock_test_after_events(void *udata) {
  if (*(size_t *)udata)
    *(size_t *)udata += 1;
}

FIO_SFUNC void FIO_NAME_TEST(stl, sock)(void) {
  fprintf(stderr,
          "* Testing socket helpers (FIO_SOCK) - partial tests only!\n");
#ifdef __cplusplus
  FIO_LOG_WARNING("fio_sock_poll test only runs in C - the FIO_SOCK_POLL_LIST "
                  "macro doesn't work in C++ and writing the test without it "
                  "is a headache.");
#else
  struct {
    const char *address;
    const char *port;
    const char *msg;
    uint16_t flag;
  } server_tests[] = {
    {"127.0.0.1", "9437", "TCP", FIO_SOCK_TCP},
#if FIO_OS_POSIX
#ifdef P_tmpdir
    {P_tmpdir "/tmp_unix_testing_socket_facil_io.sock",
     NULL,
     "Unix",
     FIO_SOCK_UNIX},
#else
    {"./tmp_unix_testing_socket_facil_io.sock", NULL, "Unix", FIO_SOCK_UNIX},
#endif
#endif
    /* accept doesn't work with UDP, not like this... UDP test is seperate */
    // {"127.0.0.1", "9437", "UDP", FIO_SOCK_UDP},
    {.address = NULL},
  };
  for (size_t i = 0; server_tests[i].address; ++i) {
    size_t flag = (size_t)-1;
    errno = 0;
    fprintf(stderr, "* Testing %s socket API\n", server_tests[i].msg);
    int srv = fio_sock_open(server_tests[i].address,
                            server_tests[i].port,
                            server_tests[i].flag | FIO_SOCK_SERVER);
    FIO_ASSERT(srv != -1, "server socket failed to open: %s", strerror(errno));
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag);
    FIO_ASSERT(!flag, "before_events not called for missing list! (%zu)", flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST({.fd = -1}));
    FIO_ASSERT(!flag, "before_events not called for empty list! (%zu)", flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = fio___sock_test_on_event,
                  .on_data = NULL,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_ASSERT(!flag, "No event should have occured here! (%zu)", flag);

    int cl = fio_sock_open(server_tests[i].address,
                           server_tests[i].port,
                           server_tests[i].flag | FIO_SOCK_CLIENT);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
               "client socket failed to open (%d)",
               cl);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    // // is it possible to write to a still-connecting socket?
    // fio_sock_poll(.before_events = fio___sock_test_before_events,
    //               .after_events = fio___sock_test_after_events,
    //               .on_ready = fio___sock_test_on_event, .on_data = NULL,
    //               .on_error = NULL, .udata = &flag,
    //               .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    // FIO_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    FIO_LOG_INFO("error may print when polling server for `write`.");
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .timeout = 100,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_ASSERT(flag == 2, "Event should have occured here! (%zu)", flag);
    FIO_LOG_INFO("error may have been emitted.");

    intptr_t accepted = accept(srv, NULL, NULL);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted),
               "accepted socket failed to open (%zd)",
               (ssize_t)accepted);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = fio___sock_test_on_event,
                  .on_data = NULL,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .timeout = 100,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_ASSERT(flag, "Event should have occured here! (%zu)", flag);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = fio___sock_test_on_event,
                  .on_data = NULL,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .timeout = 100,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(accepted)));
    FIO_ASSERT(flag, "Event should have occured here! (%zu)", flag);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_ASSERT(!flag, "No event should have occured here! (%zu)", flag);

    if (fio_sock_write(accepted, "hello", 5) > 0) {
      // wait for read
      FIO_ASSERT(fio_sock_wait_io(cl, POLLIN, 0) != -1 &&
                     (fio_sock_wait_io(cl, POLLIN, 0) | POLLIN),
                 "fio_sock_wait_io should have returned a POLLIN event.");
      fio_sock_poll(.before_events = fio___sock_test_before_events,
                    .on_ready = NULL,
                    .on_data = fio___sock_test_on_event,
                    .on_error = NULL,
                    .after_events = fio___sock_test_after_events,
                    .udata = &flag,
                    .timeout = 100,
                    .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_R(cl)));
      // test read/write
      fio_sock_poll(.before_events = fio___sock_test_before_events,
                    .on_ready = fio___sock_test_on_event,
                    .on_data = fio___sock_test_on_event,
                    .on_error = NULL,
                    .after_events = fio___sock_test_after_events,
                    .udata = &flag,
                    .timeout = 100,
                    .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
      {
        char buf[64];
        errno = 0;
        FIO_ASSERT(fio_sock_read(cl, buf, 64) > 0,
                   "Read should have read some data...\n\t"
                   "error: %s",
                   strerror(errno));
      }
      FIO_ASSERT(flag == 3, "Event should have occured here! (%zu)", flag);
    } else
      FIO_ASSERT(0,
                 "send(fd:%ld) failed! error: %s",
                 accepted,
                 strerror(errno));
    fio_sock_close(accepted);
    fio_sock_close(cl);
    fio_sock_close(srv);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL,
                  .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events,
                  .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_ASSERT(flag, "Event should have occured here! (%zu)", flag);
#if FIO_OS_POSIX
    if (FIO_SOCK_UNIX == server_tests[i].flag)
      unlink(server_tests[i].address);
#endif
  }
  {
    /* UDP semi test */
    fprintf(stderr, "* Testing UDP socket (abbreviated test)\n");
    int srv =
        fio_sock_open("127.0.0.1", "9437", FIO_SOCK_UDP | FIO_SOCK_SERVER);
    int n = 0; /* try for 32Mb */
    socklen_t sn = sizeof(n);
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t- UDP default receive buffer is %d bytes\n", n);
    n = 32 * 1024 * 1024; /* try for 32Mb */
    sn = sizeof(n);
    while (setsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, sn) == -1) {
      /* failed - repeat attempt at 0.5Mb interval */
      if (n >= (1024 * 1024)) // OS may have returned max value
        n -= 512 * 1024;
      else
        break;
    }
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t- UDP receive buffer could be set to %d bytes\n", n);
    FIO_ASSERT(srv != -1,
               "Couldn't open UDP server socket: %s",
               strerror(errno));
    FIO_LOG_INFO("Opening client UDP socket.");
    int cl = fio_sock_open("127.0.0.1", "9437", FIO_SOCK_UDP | FIO_SOCK_CLIENT);
    FIO_ASSERT(cl != -1,
               "Couldn't open UDP client socket: %s",
               strerror(errno));
    FIO_LOG_INFO("Starting UDP roundtrip.");
    FIO_ASSERT(fio_sock_write(cl, "hello", 5) != -1,
               "couldn't send datagram from client");
    char buf[64];
    FIO_LOG_INFO("Receiving UDP msg.");
    FIO_ASSERT(recvfrom(srv, buf, 64, 0, NULL, NULL) != -1,
               "couldn't read datagram");
    FIO_ASSERT(!memcmp(buf, "hello", 5), "transmission error");
    FIO_LOG_INFO("cleaning up UDP sockets.");
    fio_sock_close(srv);
    fio_sock_close(cl);
  }
#endif /* !__cplusplus */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
FIO_SOCK - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SOCK
#endif
