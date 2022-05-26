/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE___H /* Development inclusion - ignore line*/
#define FIO_SOCK                      /* Development inclusion - ignore line */
#include "000 header.h"               /* Development inclusion - ignore line */
#include "050 url.h"                  /* Development inclusion - ignore line */
#endif                                /* Development inclusion - ignore line */
/* *****************************************************************************




                        Basic Socket Helpers / IO Polling




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
#define poll   WSAPoll
/** Acts as POSIX dup. Use this for portability with WinSock2. */
FIO_IFUNC int fio_sock_dup(int original) {
  int fd = -1;
  SOCKET tmpfd = INVALID_SOCKET;
  WSAPROTOCOL_INFOA info;
  if (!WSADuplicateSocketA(original, GetCurrentProcessId(), &info) &&
      (tmpfd = WSASocketA(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, &info, 0, 0)) !=
          INVALID_SOCKET) {
    if (FIO_SOCK_FD_ISVALID(tmpfd))
      fd = (int)tmpfd;
    else
      fio_sock_close(tmpfd);
  }
  return fd;
}

#elif FIO_HAVE_UNIX_TOOLS
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
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
/** Acts as POSIX dup. Use this macro for portability with WinSock2. */
#define fio_sock_dup(fd)              dup(fd)
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

/** Attempts to maximize the allowed open file limits. returns known limit */
SFUNC size_t fio_sock_maximize_limits(void);

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
IO Poll - Implementation (always static / inlined)
***************************************************************************** */

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
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

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
        if (u.scheme.len == 3 && (u.scheme.buf[0] | 32) == 't' &&
            (u.scheme.buf[1] | 32) == 'c' && (u.scheme.buf[2] | 32) == 'p')
          flags |= FIO_SOCK_TCP;
        else if (u.scheme.len == 3 && (u.scheme.buf[0] | 32) == 'u' &&
                 (u.scheme.buf[1] | 32) == 'd' && (u.scheme.buf[2] | 32) == 'p')
          flags |= FIO_SOCK_UDP;
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
  if (ioctlsocket(fd, FIONBIO, &flags) == SOCKET_ERROR) {
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
  r = (short)poll(&pfd, 1, timeout);
  if (r == 1)
    r = pfd.revents;
  return r;
}

/** Attempts to maximize the allowed open file limits. returns known limit */
SFUNC size_t fio_sock_maximize_limits(void) {
  ssize_t capa = 0;
#if FIO_OS_POSIX

#ifdef _SC_OPEN_MAX
  capa = sysconf(_SC_OPEN_MAX);
#elif defined(FOPEN_MAX)
  capa = FOPEN_MAX;
#endif
  // try to maximize limits - collect max and set to max
  struct rlimit rlim = {.rlim_max = 0};
  if (getrlimit(RLIMIT_NOFILE, &rlim) == -1) {
    FIO_LOG_WARNING("`getrlimit` failed (%d): %s", errno, strerror(errno));
    return capa;
  }

  FIO_LOG_DEBUG2("existing / maximum open file limit detected: %zd / %zd",
                 (ssize_t)rlim.rlim_cur,
                 (ssize_t)rlim.rlim_max);

  rlim_t original = rlim.rlim_cur;
  rlim.rlim_cur = rlim.rlim_max;
  while (setrlimit(RLIMIT_NOFILE, &rlim) == -1 && rlim.rlim_cur > original)
    rlim.rlim_cur >>= 1;

  FIO_LOG_DEBUG2("new open file limit: %zd", (ssize_t)rlim.rlim_cur);

  getrlimit(RLIMIT_NOFILE, &rlim);
  capa = rlim.rlim_cur;
#elif FIO_OS_WIN
  capa = 1ULL << 10;
  while (_setmaxstdio(capa) > 0)
    capa <<= 1;
  capa >>= 1;
  FIO_LOG_DEBUG("new open file limit: %zd", (ssize_t)capa);
#else
  FIO_LOG_ERROR("No OS detected, couldn't maximize open file limit.");
#endif
  return capa;
}

#if FIO_OS_POSIX || defined(AF_UNIX)
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
      FIO_LOG_DEBUG("couldn't connect unix client @ %s : %s",
                    addr.sun_path,
                    strerror(errno));
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

/* TODO: UNIX Sockets?
 * https://devblogs.microsoft.com/commandline/af_unix-comes-to-windows/
 */

#endif /* FIO_OS_WIN / FIO_OS_POSIX */

/* *****************************************************************************
WinSock initialization
***************************************************************************** */
#if FIO_OS_WIN
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
#endif /* FIO_OS_WIN / FIO_OS_POSIX */

/* *****************************************************************************
Socket helper testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL

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
    short ev = (short)-1;
    errno = 0;
    fprintf(stderr, "* Testing %s socket API\n", server_tests[i].msg);
    int srv = fio_sock_open(server_tests[i].address,
                            server_tests[i].port,
                            server_tests[i].flag | FIO_SOCK_SERVER);
    FIO_ASSERT(srv != -1, "server socket failed to open: %s", strerror(errno));
    ev = fio_sock_wait_io(-1, POLLIN | POLLOUT, 0);
    FIO_ASSERT(!ev, "no error should have been returned for IO -1 (%d)", ev);
    ev = fio_sock_wait_io(srv, POLLIN, 0);
    FIO_ASSERT(!ev, "no events should have been returned (%d)", ev);
    int cl = fio_sock_open(server_tests[i].address,
                           server_tests[i].port,
                           server_tests[i].flag | FIO_SOCK_CLIENT);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
               "client socket failed to open (%d)",
               cl);
    ev = fio_sock_wait_io(cl, POLLIN /* | POLLOUT <= OS dependent */, 0);
    FIO_ASSERT(!ev,
               "no events should have been returned for connecting client(%d)",
               ev);
    ev = fio_sock_wait_io(srv, POLLIN, 100);
    FIO_ASSERT(ev == POLLIN,
               "incoming connection should have been detected (%d)",
               ev);
    intptr_t accepted = accept(srv, NULL, NULL);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted),
               "accepted socket failed to open (%zd)",
               (ssize_t)accepted);
    ev = fio_sock_wait_io(cl, POLLIN | POLLOUT, 0);
    FIO_ASSERT(ev == POLLOUT,
               "POLLOUT should have been returned for connected client(%d)",
               ev);
    ev = fio_sock_wait_io(accepted, POLLIN | POLLOUT, 0);
    FIO_ASSERT(ev == POLLOUT,
               "POLLOUT should have been returned for connected client 2(%d)",
               ev);
    if (fio_sock_write(accepted, "hello", 5) > 0) {
      // wait for read
      FIO_ASSERT(
          fio_sock_wait_io(cl, POLLIN, 10) != -1 &&
              ((fio_sock_wait_io(cl, POLLIN | POLLOUT, 0) & POLLIN) == POLLIN),
          "fio_sock_wait_io should have returned a POLLIN event for client.");
      {
        char buf[64];
        errno = 0;
        FIO_ASSERT(fio_sock_read(cl, buf, 64) > 0,
                   "Read should have read some data...\n\t"
                   "error: %s",
                   strerror(errno));
      }
      FIO_ASSERT(!fio_sock_wait_io(cl, POLLIN, 0),
                 "No events should have occurred here! (%zu)",
                 ev);
    } else {
      FIO_ASSERT(0,
                 "send(fd:%ld) failed! error: %s",
                 accepted,
                 strerror(errno));
    }
    fio_sock_close(accepted);
    fio_sock_close(cl);
    fio_sock_close(srv);
    FIO_ASSERT((fio_sock_wait_io(cl, POLLIN | POLLOUT, 0) & POLLNVAL),
               "POLLNVAL should have been returned for closed socket (%d & %d) "
               "(POLLERR == %d)",
               fio_sock_wait_io(cl, POLLIN | POLLOUT, 0),
               (int)POLLNVAL,
               (int)POLLERR);
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
