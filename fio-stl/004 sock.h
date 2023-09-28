/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SOCK               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                            Basic Socket Helpers



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SOCK) && !defined(H___FIO_SOCK___H)
#define H___FIO_SOCK___H

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
#ifdef AF_UNIX
#include <afunix.h>
#endif
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
  WSAPROTOCOL_INFO info;
  if (!WSADuplicateSocket(original, GetCurrentProcessId(), &info) &&
      (tmpfd = WSASocket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, &info, 0, 0)) !=
          INVALID_SOCKET) {
    if (FIO_SOCK_FD_ISVALID(tmpfd))
      fd = (int)tmpfd;
    else
      fio_sock_close(tmpfd);
  }
  return fd;
}

#elif FIO_HAVE_UNIX_TOOLS
#include <arpa/inet.h>
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
Socket OS abstraction - API
***************************************************************************** */

#ifndef FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT
#define FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT (1ULL << 24)
#endif

/** Socket type flags */
typedef enum {
  FIO_SOCK_SERVER = 0,
  FIO_SOCK_CLIENT = 1,
  FIO_SOCK_NONBLOCK = 2,
  FIO_SOCK_TCP = 4,
  FIO_SOCK_UDP = 8,
#ifdef AF_UNIX
  FIO_SOCK_UNIX = 16,
  FIO_SOCK_UNIX_PRIVATE = (16 | 32),
#else
#define FIO_SOCK_UNIX         0
#define FIO_SOCK_UNIX_PRIVATE 0
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

/** Creates a new Unix socket and binds it to a local address. */
SFUNC int fio_sock_open_unix(const char *address, uint16_t flags);

/** Sets a file descriptor / socket to non blocking state. */
SFUNC int fio_sock_set_non_block(int fd);

/** Attempts to maximize the allowed open file limits. returns known limit */
SFUNC size_t fio_sock_maximize_limits(size_t maximum_limit);

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
#ifdef AF_UNIX
  if ((flags & FIO_SOCK_UNIX))
    return fio_sock_open_unix(address, flags);
#endif

  switch ((flags & ((uint16_t)FIO_SOCK_TCP | (uint16_t)FIO_SOCK_UDP))) {
  case 0: /* fall through - default to TCP/IP*/
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
  }

  FIO_LOG_ERROR(
      "(fio_sock_open) the FIO_SOCK_TCP and FIO_SOCK_UDP flags are exclusive");
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
  fio_url_s u = fio_url_parse(url, FIO_STRLEN(url));
#ifdef AF_UNIX
  if (!u.host.buf && !u.port.buf && u.path.buf) {
    /* Unix socket - force flag validation */
    flags &= ~((uint16_t)(FIO_SOCK_UNIX | FIO_SOCK_TCP));
    flags |= (u.scheme.len == 4 &&
              fio_buf2u32u(u.scheme.buf) == fio_buf2u32u("priv"))
                 ? FIO_SOCK_UNIX_PRIVATE
                 : FIO_SOCK_UNIX;
    if (u.path.len > 2047) {
      errno = EINVAL;
      FIO_LOG_ERROR(
          "Couldn't open unix socket to %s - host name too long (%zu).",
          url,
          u.path.len);
      return -1;
    }
    FIO_MEMCPY(buf, u.path.buf, u.path.len);
    buf[u.path.len] = 0;
    pr = NULL;
    return fio_sock_open_unix(buf, flags);
  }
#endif
  if (!u.port.len)
    u.port = u.scheme;
  if (!u.port.len) {
    pr = NULL;
  } else {
    if (u.port.len > 63) {
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
               (u.scheme.buf[0] | 32) == 'h' && (u.scheme.buf[1] | 32) == 't' &&
               (u.scheme.buf[2] | 32) == 't' && (u.scheme.buf[3] | 32) == 'p' &&
               (u.scheme.len == 4 ||
                (u.scheme.len == 5 && (u.scheme.buf[4] | 32) == 's')))
        flags |= FIO_SOCK_TCP;
    }
  }
  if (u.host.len) {
    if (u.host.len > 2047) {
      errno = EINVAL;
      FIO_LOG_ERROR("Couldn't open socket to %s - host name too long.", url);
      return -1;
    }
    FIO_MEMCPY(buf, u.host.buf, u.host.len);
    buf[u.host.len] = 0;
  } else {
    addr = NULL;
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
    { // avoid the "address taken"
      int optval = 1;
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));
    }
    if (nonblock && fio_sock_set_non_block(fd) == -1) {
      FIO_LOG_DEBUG("Couldn't set socket (%d) to non-blocking mode %s",
                    fd,
                    strerror(errno));
      fio_sock_close(fd);
      fd = -1;
      continue;
    }
    if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
      FIO_LOG_DEBUG("Failed attempt to bind socket (%d) to address %s",
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
  short r = 0;
#ifdef FIO_OS_WIN
  if (fd == -1) {
    FIO_THREAD_WAIT((timeout * 1000000));
    return r;
  }
#endif
  struct pollfd pfd = {.fd = fd, .events = events};
  r = (short)poll(&pfd, 1, timeout);
  if (r == 1)
    r = pfd.revents;
  return r;
}

/** Attempts to maximize the allowed open file limits. returns known limit */
SFUNC size_t fio_sock_maximize_limits(size_t max_limit) {
  ssize_t capa = 0;
  if (!max_limit)
    max_limit = FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT;
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

  if (rlim.rlim_cur >= max_limit) {
    FIO_LOG_DEBUG2("open file limit can't be maximized any further (%zd / %zu)",
                   (ssize_t)rlim.rlim_cur,
                   max_limit);
    return rlim.rlim_cur;
  }

  rlim_t original = rlim.rlim_cur;
  rlim.rlim_cur = rlim.rlim_max > max_limit ? max_limit : rlim.rlim_max;
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

#ifdef AF_UNIX
/** Creates a new Unix socket and binds it to a local address. */
SFUNC int fio_sock_open_unix(const char *address, uint16_t flags) {
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
  int fd =
      socket(AF_UNIX, (flags & FIO_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM, 0);
  if (fd == -1) {
    FIO_LOG_DEBUG("couldn't open unix socket (flags == %d) %s",
                  (int)flags,
                  strerror(errno));
    return -1;
  }
  if ((flags & FIO_SOCK_NONBLOCK) && fio_sock_set_non_block(fd) == -1) {
    FIO_LOG_DEBUG("couldn't set socket to non-blocking mode");
    fio_sock_close(fd);
    unlink(addr.sun_path);
    return -1;
  }
  if ((flags & FIO_SOCK_CLIENT)) {
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
    int btmp; // the bind result
#if !defined(FIO_SOCK_AVOID_UMASK) && !defined(FIO_OS_WIN)
    if ((flags & FIO_SOCK_UNIX_PRIVATE) == FIO_SOCK_UNIX) {
      int umask_org = umask(0x1FF);
      btmp = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
      int old_err = errno;
      umask(umask_org);
      errno = old_err;
      FIO_LOG_DEBUG("umask was used temporarily for Unix Socket (was 0x%04X)",
                    umask_org);
    } else
#endif /* FIO_SOCK_AVOID_UMASK */
      /* else */ btmp = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (btmp == -1) {
      FIO_LOG_DEBUG("couldn't bind unix socket to %s\n\terrno(%d): %s",
                    address,
                    errno,
                    strerror(errno));
      fio_sock_close(fd);
      // unlink(addr.sun_path);
      return -1;
    }
#ifndef FIO_OS_WIN
    if ((flags & FIO_SOCK_UNIX_PRIVATE) == FIO_SOCK_UNIX) {
      chmod(address, S_IRWXO | S_IRWXG | S_IRWXU);
      fchmod(fd, S_IRWXO | S_IRWXG | S_IRWXU);
    }
#endif
    if (!(flags & FIO_SOCK_UDP) && listen(fd, SOMAXCONN) < 0) {
      FIO_LOG_DEBUG("couldn't start listening to unix socket at %s", address);
      fio_sock_close(fd);
      unlink(addr.sun_path);
      return -1;
    }
  }
  return fd;
}
#else
SFUNC int fio_sock_open_unix(const char *address, uint16_t flags) {
  (void)address, (void)flags;
  FIO_ASSERT(0, "this system does not support Unix sockets.");
}
#endif /* AF_UNIX */

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
FIO_SOCK - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SOCK
#endif
