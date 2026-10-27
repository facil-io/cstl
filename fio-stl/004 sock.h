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
/** Native socket handle type: SOCKET (UINT_PTR) on Windows. */
typedef SOCKET fio_socket_i;
/** Sentinel value for an invalid socket handle. */
#define FIO_SOCKET_INVALID INVALID_SOCKET
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
#define FIO_SOCK_FD_ISVALID(fd) ((fio_socket_i)(fd) != FIO_SOCKET_INVALID)
#endif
/* Winsock function pointer table — populated at startup via GetProcAddress so
 * that Ruby's win32.h symbol replacements (rb_w32_bind, rb_w32_send, etc.)
 * never intercept our calls. Not static: IFUNC wrappers must share one copy. */
extern struct fio___winsock_fn_s {
  int(WSAAPI *bind)(SOCKET, const struct sockaddr *, int);
  int(WSAAPI *connect)(SOCKET, const struct sockaddr *, int);
  int(WSAAPI *listen)(SOCKET, int);
  SOCKET(WSAAPI *accept)(SOCKET, struct sockaddr *, int *);
  SOCKET(WSAAPI *socket)(int, int, int);
  int(WSAAPI *send)(SOCKET, const char *, int, int);
  int(WSAAPI *recv)(SOCKET, char *, int, int);
  int(WSAAPI *sendto)(SOCKET,
                      const char *,
                      int,
                      int,
                      const struct sockaddr *,
                      int);
  int(WSAAPI *recvfrom)(SOCKET, char *, int, int, struct sockaddr *, int *);
  int(WSAAPI *setsockopt)(SOCKET, int, int, const char *, int);
  int(WSAAPI *getsockopt)(SOCKET, int, int, char *, int *);
  int(WSAAPI *ioctlsocket)(SOCKET, long, u_long *);
  int(WSAAPI *closesocket)(SOCKET);
  int(WSAAPI *select)(int,
                      fd_set *,
                      fd_set *,
                      fd_set *,
                      const struct timeval *);
  int(WSAAPI *getsockname)(SOCKET, struct sockaddr *, int *);
  int(WSAAPI *getpeername)(SOCKET, struct sockaddr *, int *);
} fio___winsock_fn;
/** Acts as POSIX write. Use this function for portability with WinSock2.
 * send/recv are the correct Winsock2 functions for WSAPoll-based I/O;
 * WSASend/WSARecv are for IOCP/overlapped I/O which this library does not use.
 */
IFUNC ssize_t fio_sock_write(fio_socket_i fd, const void *buf, size_t len);
/** Acts as POSIX read. Use this function for portability with WinSock2. */
IFUNC ssize_t fio_sock_read(fio_socket_i fd, void *buf, size_t len);
/** Acts as POSIX close. Use this function for portability with WinSock2. */
IFUNC int fio_sock_close(fio_socket_i fd);
/** Acts as POSIX sendto. Use this function for portability with WinSock2. */
IFUNC ssize_t fio_sock_sendto(fio_socket_i fd,
                              const void *buf,
                              size_t len,
                              int flags,
                              const struct sockaddr *addr,
                              socklen_t addrlen);
/** Acts as POSIX recvfrom. Use this function for portability with WinSock2. */
IFUNC ssize_t fio_sock_recvfrom(fio_socket_i fd,
                                void *buf,
                                size_t len,
                                int flags,
                                struct sockaddr *addr,
                                socklen_t *addrlen);
/** Accepts a new connection, returning a native socket handle. */
IFUNC fio_socket_i fio_sock_accept(fio_socket_i s,
                                   struct sockaddr *addr,
                                   int *addrlen);
/** Acts as POSIX dup. Use this for portability with WinSock2.
 *
 * Uses WSADuplicateSocket + WSASocket to produce a valid Winsock SOCKET
 * that can be used with WSAPoll, send, recv, etc.
 *
 * dwFlags MUST be 0 when FROM_PROTOCOL_INFO is used — the WSAPROTOCOL_INFO
 * struct already encodes WSA_FLAG_OVERLAPPED from the original socket.
 * Passing WSA_FLAG_OVERLAPPED explicitly causes WSAEINVAL on some Winsock
 * implementations (notably MinGW) when the flags conflict.
 */
IFUNC fio_socket_i fio_sock_dup(fio_socket_i original);

/* *****************************************************************************
Portable wrappers for bind / connect / listen / setsockopt.

These mirror fio_sock_write / fio_sock_read / fio_sock_close etc. and
encapsulate the Winsock2 type coercions (addrlen int, optval char*, optlen int)
that differ from POSIX. fio_socket_i is typedef'd to the correct OS type
(SOCKET on Windows, int on POSIX), so no fd cast is needed at the call site.
***************************************************************************** */

/** Portable bind. Calls Winsock2 bind on Windows, POSIX bind on POSIX. */
IFUNC int fio_sock_bind(fio_socket_i fd,
                        const struct sockaddr *addr,
                        socklen_t addrlen);

/** Portable connect. Calls Winsock2 connect on Windows, POSIX connect on POSIX.
 */
IFUNC int fio_sock_connect(fio_socket_i fd,
                           const struct sockaddr *addr,
                           socklen_t addrlen);

/** Portable listen. Calls Winsock2 listen on Windows, POSIX listen on POSIX. */
IFUNC int fio_sock_listen(fio_socket_i fd, int backlog);

/** Portable setsockopt.
 * optval is (const char *) on Windows and (const void *) on POSIX;
 * optlen is int on Windows and socklen_t on POSIX.
 * Both coercions are applied internally; callers pass any pointer type. */
IFUNC int fio_sock_setsockopt(fio_socket_i fd,
                              int level,
                              int optname,
                              const void *optval,
                              socklen_t optlen);

/** Creates a connected socket pair via loopback TCP (Windows socketpair).
 * On Windows, fio_sock_pipe() delegates here since _pipe() returns CRT fds
 * (not Winsock SOCKETs) and is incompatible with WSAPoll-based I/O. */
IFUNC int fio_sock_socketpair(fio_socket_i fds[2]);

/** Creates a pipe-like connected pair. On Windows, delegates to
 * fio_sock_socketpair() (loopback TCP) since _pipe() returns CRT fds that are
 * incompatible with Winsock / WSAPoll-based I/O. */
FIO_IFUNC int fio_sock_pipe(fio_socket_i fds[2]) {
  return fio_sock_socketpair(fds);
}

#elif FIO_HAVE_UNIX_TOOLS
/** Native socket handle type: int on POSIX. */
typedef int fio_socket_i;
/** Sentinel value for an invalid socket handle. */
#define FIO_SOCKET_INVALID (-1)
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
#define FIO_SOCK_FD_ISVALID(fd) ((fio_socket_i)(fd) != FIO_SOCKET_INVALID)
#endif
/** Acts as POSIX write. Use this function for portability with WinSock2. */
FIO_IFUNC ssize_t fio_sock_write(fio_socket_i fd, const void *buf, size_t len) {
  return write(fd, buf, len);
}
/** Acts as POSIX read. Use this function for portability with WinSock2. */
FIO_IFUNC ssize_t fio_sock_read(fio_socket_i fd, void *buf, size_t len) {
  return read(fd, buf, len);
}
/** Acts as POSIX sendto. Use this function for portability with WinSock2. */
FIO_IFUNC ssize_t fio_sock_sendto(fio_socket_i fd,
                                  const void *buf,
                                  size_t len,
                                  int flags,
                                  const struct sockaddr *addr,
                                  socklen_t addrlen) {
  return sendto(fd, buf, len, flags, addr, addrlen);
}
/** Acts as POSIX recvfrom. Use this function for portability with WinSock2. */
FIO_IFUNC ssize_t fio_sock_recvfrom(fio_socket_i fd,
                                    void *buf,
                                    size_t len,
                                    int flags,
                                    struct sockaddr *addr,
                                    socklen_t *addrlen) {
  return recvfrom(fd, buf, len, flags, addr, addrlen);
}
/** Acts as POSIX dup. Sets O_CLOEXEC on the new fd. */
FIO_IFUNC fio_socket_i fio_sock_dup(fio_socket_i fd) {
  fio_socket_i r = dup(fd);
  if (r == -1) {
    FIO_LOG_ERROR("(fio_sock_dup) dup(%d) failed: %s", fd, strerror(errno));
    return FIO_SOCKET_INVALID;
  }
  if (fcntl(r, F_SETFD, FD_CLOEXEC) == -1)
    FIO_LOG_ERROR("(fio_sock_dup) fcntl(FD_CLOEXEC) failed on fd %d: %s",
                  r,
                  strerror(errno));
  return r;
}
/** Acts as POSIX close. Use this function for portability with WinSock2. */
FIO_IFUNC int fio_sock_close(fio_socket_i fd) { return close(fd); }
/** Acts as POSIX accept. Use this macro for portability with WinSock2. */
#define fio_sock_accept(fd, addr, addrlen) accept(fd, addr, addrlen)

/** Portable bind. POSIX version. */
FIO_IFUNC int fio_sock_bind(fio_socket_i fd,
                            const struct sockaddr *addr,
                            socklen_t addrlen) {
  return bind(fd, addr, addrlen);
}

/** Portable connect. POSIX version. */
FIO_IFUNC int fio_sock_connect(fio_socket_i fd,
                               const struct sockaddr *addr,
                               socklen_t addrlen) {
  return connect(fd, addr, addrlen);
}

/** Portable listen. POSIX version. */
FIO_IFUNC int fio_sock_listen(fio_socket_i fd, int backlog) {
  return listen(fd, backlog);
}

/** Portable setsockopt. POSIX version. */
FIO_IFUNC int fio_sock_setsockopt(fio_socket_i fd,
                                  int level,
                                  int optname,
                                  const void *optval,
                                  socklen_t optlen) {
  return setsockopt(fd, level, optname, optval, optlen);
}

/** Creates a connected socket pair using POSIX socketpair(). */
FIO_IFUNC int fio_sock_socketpair(fio_socket_i fds[2]) {
  return socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
}

/** Creates a POSIX pipe. fds[0] = read end, fds[1] = write end.
 * On Windows, fio_sock_pipe() is defined in the FIO_OS_WIN block above and
 * delegates to fio_sock_socketpair() (loopback TCP). */
FIO_IFUNC int fio_sock_pipe(fio_socket_i fds[2]) { return pipe(fds); }
#else
#error FIO_SOCK requires a supported OS (Windows / POSIX).
#endif

/* Set to 1 if in need to debug unexpected IO closures. */
#if defined(DEBUG) && 0
#define close(fd)                                                              \
  do {                                                                         \
    FIO_LOG_DWARNING("(%d) (" FIO___FILE__ ":" FIO_MACRO2STR(                  \
                         __LINE__) ") fio_sock_close called for fd %d",        \
                     fio_getpid(),                                             \
                     (int)fd);                                                 \
    close(fd);                                                                 \
  } while (0)
#endif /* DEBUG */

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
FIO_IFUNC fio_socket_i fio_sock_open(const char *restrict address,
                                     const char *restrict port,
                                     uint16_t flags);

/** Creates a new socket, according to the provided flags. */
SFUNC fio_socket_i fio_sock_open2(const char *url, uint16_t flags);

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

/**
 * Returns the socket's local numeric host, or host:port when with_port is true
 * (IPv6 endpoints use [host]:port). Unix sockets return their pathname without
 * a port, regardless of with_port. On error, returns a NULL buffer with zero
 * length. Results share the 128-slot static ring with fio_sock_peer_addr;
 * copy the result to retain it.
 */
SFUNC fio_buf_info_s fio_sock_addr(fio_socket_i s, bool with_port);

/**
 * Returns the socket's peer numeric host, or host:port when with_port is true
 * (IPv6 endpoints use [host]:port). Unix sockets return their pathname without
 * a port, regardless of with_port. On error, returns a NULL buffer with zero
 * length. Results share a ring of 128 static 128-byte slots with fio_sock_addr;
 * the 129th call reuses the first slot. Copy the result to retain it.
 */
SFUNC fio_buf_info_s fio_sock_peer_addr(fio_socket_i s, bool with_port);

/** Creates a new network socket and binds it to a local address. */
SFUNC fio_socket_i fio_sock_open_local(struct addrinfo *addr, int nonblock);

/** Creates a new network socket and connects it to a remote address. */
SFUNC fio_socket_i fio_sock_open_remote(struct addrinfo *addr, int nonblock);

/** Creates a new Unix socket and binds it to a local address. */
SFUNC fio_socket_i fio_sock_open_unix(const char *address, uint16_t flags);

/** Sets a file descriptor / socket to non blocking state. */
SFUNC int fio_sock_set_non_block(fio_socket_i fd);

/** Returns the pending socket error (`SO_ERROR`), 0 if none, -1 on failure. */
SFUNC int fio_sock_error(fio_socket_i fd);

/** Attempts to maximize the allowed open file limits. returns known limit */
SFUNC size_t fio_sock_maximize_limits(size_t maximum_limit);

/**
 * Returns 0 on timeout, -1 on error or the events that are valid.
 *
 * A zero timeout returns immediately.
 *
 * Possible events include POLLIN | POLLOUT
 *
 * Possible return values include POLLIN | POLLOUT | POLLHUP | POLLNVAL
 */
SFUNC short fio_sock_wait_io(fio_socket_i fd, short events, int timeout);

/** A helper macro that waits on a single IO with no callbacks (0 = no event) */
#define FIO_SOCK_WAIT_RW(fd, timeout_)                                         \
  fio_sock_wait_io(fd, POLLIN | POLLOUT, timeout_)

/** A helper macro that waits on a single IO with no callbacks (0 = no event) */
#define FIO_SOCK_WAIT_R(fd, timeout_) fio_sock_wait_io(fd, POLLIN, timeout_)

/** A helper macro that waits on a single IO with no callbacks (0 = no event) */
#define FIO_SOCK_WAIT_W(fd, timeout_) fio_sock_wait_io(fd, POLLOUT, timeout_)

#ifdef POLLRDHUP
/** A helper macro that tests if a socket was closed.  */
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (FIO_SOCK_FD_ISVALID(fd) &&                                                  \
   !(fio_sock_wait_io(fd, (POLLOUT | POLLRDHUP), 0) &                          \
     (POLLRDHUP | POLLHUP | POLLNVAL)))
#else
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (FIO_SOCK_FD_ISVALID(fd) &&                                                  \
   !(fio_sock_wait_io(fd, POLLOUT, 0) & (POLLHUP | POLLNVAL)))
#endif

/* *****************************************************************************
IO Poll - Implementation (always static / inlined)
***************************************************************************** */

/**
 * Creates a new socket according to the provided flags.
 *
 * The `port` string will be ignored when `FIO_SOCK_UNIX` is set.
 */
FIO_IFUNC fio_socket_i fio_sock_open(const char *restrict address,
                                     const char *restrict port,
                                     uint16_t flags) {
  struct addrinfo *addr = NULL;
  fio_socket_i fd;
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
      return FIO_SOCKET_INVALID;
    }
    if ((flags & FIO_SOCK_CLIENT)) {
      fd = fio_sock_open_remote(addr, (flags & FIO_SOCK_NONBLOCK));
    } else {
      fd = fio_sock_open_local(addr, (flags & FIO_SOCK_NONBLOCK));
      if (FIO_SOCK_FD_ISVALID(fd) && fio_sock_listen(fd, SOMAXCONN) == -1) {
        FIO_LOG_ERROR("(fio_sock_open) failed on call to listen: %s",
                      strerror(errno));
        fio_sock_close(fd);
        fd = FIO_SOCKET_INVALID;
      }
    }
    fio_sock_address_free(addr);
    return fd;
  case FIO_SOCK_UDP:
    addr = fio_sock_address_new(address, port, SOCK_DGRAM);
    if (!addr) {
      FIO_LOG_ERROR("(fio_sock_open) address error: %s", strerror(errno));
      return FIO_SOCKET_INVALID;
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
  return FIO_SOCKET_INVALID;
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

  size_t port_len = (port ? FIO_STRLEN(port) : 0U);
  switch (port_len) { /* skip system service lookup for common web stuff */
  case 2:
    if ((port[0] | 32) == 'w' && (port[1] | 32) == 's')
      port = "80";
    break;
  case 3:
    if ((port[0] | 32) == 'w' && (port[1] | 32) == 's' && (port[2] | 32) == 's')
      port = "443";
    else if ((port[0] | 32) == 's' && (port[1] | 32) == 's' &&
             (port[2] | 32) == 'e')
      port = "80";
    break;
  case 4:
    if ((port[0] | 32) == 'h' && (port[1] | 32) == 't' &&
        (port[2] | 32) == 't' && (port[3] | 32) == 'p')
      port = "80";
    else if ((port[0] | 32) == 's' && (port[1] | 32) == 's' &&
             (port[2] | 32) == 'e' && (port[3] | 32) == 's')
      port = "443";
    break;
  case 5:
    if ((port[0] | 32) == 'h' && (port[1] | 32) == 't' &&
        (port[2] | 32) == 't' && (port[3] | 32) == 'p' && (port[4] | 32) == 's')
      port = "443";
    break;
  }

#if 1 /* override system resolution for localhost ? */
  size_t address_len = (address ? FIO_STRLEN(address) : 0U);
  if (address && address_len == 9 && (address[0] | 32) == 'l' &&
      (fio_buf2u64u(address + 1) | (uint64_t)0x2020202020202020ULL) ==
          fio_buf2u64u("ocalhost"))
    address = "127.0.0.1";
  else if (sock_type != SOCK_DGRAM && address_len == 7 &&
           (fio_buf2u64u("0.0.0.0") |
            fio_buf2u64u("\x00\x00\x00\x00\x00\x00\x00\xFF")) ==
               (fio_buf2u64u(address) |
                fio_buf2u64u("\x00\x00\x00\x00\x00\x00\x00\xFF")))
    address = NULL; /* bind to everything INADDR_ANY */
#endif
  /* call for OS address resolution */
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

#if FIO_OS_WIN
/* WinSock errno bridge */
FIO_IFUNC int fio___sock_wsa_set_errno_from(int wsa_err);
/* WinSock errno bridge */
FIO_IFUNC int fio___sock_wsa_set_errno_last(void);
#endif

/** Creates a new socket, according to the provided flags. */
SFUNC fio_socket_i fio_sock_open2(const char *url, uint16_t flags) {
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
      FIO_LOG_ERROR("Couldn't open unix socket - path too long (%zu).",
                    u.path.len);
      return FIO_SOCKET_INVALID;
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
      FIO_LOG_ERROR("Couldn't open socket - port / scheme too long (%zu).",
                    u.port.len);
      return FIO_SOCKET_INVALID;
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
      FIO_LOG_ERROR("Couldn't open socket - host name too long (%zu).",
                    u.host.len);
      return FIO_SOCKET_INVALID;
    }
    FIO_MEMCPY(buf, u.host.buf, u.host.len);
    buf[u.host.len] = 0;
  } else {
    addr = NULL;
  }
  return fio_sock_open(addr, pr, flags);
}

/** Returns the pending socket error (`SO_ERROR`), 0 if none, -1 on query
 * failure. Used to detect failed non-blocking `connect` attempts. */
SFUNC int fio_sock_error(fio_socket_i fd) {
  int err = 0;
#if FIO_OS_WIN
  int len = (int)sizeof(err);
  if (fio___winsock_fn.getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
    return -1;
#else
  socklen_t len = (socklen_t)sizeof(err);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len))
    return -1;
#endif
  return err;
}

/** Sets a file descriptor / socket to non blocking state. */
SFUNC int fio_sock_set_non_block(fio_socket_i fd) {
/* On Windows, always use ioctlsocket — MinGW defines O_NONBLOCK/F_GETFL/F_SETFL
 * but fcntl does not work on Winsock SOCKETs (only on CRT fds). */
#if FIO_OS_WIN
  unsigned long f = 1;
  if (fio___winsock_fn.ioctlsocket(fd, FIONBIO, &f) == SOCKET_ERROR) {
    const int wsa_err = WSAGetLastError();
    fio___sock_wsa_set_errno_from(wsa_err);
    switch (wsa_err) {
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
#elif defined(O_NONBLOCK) && defined(F_GETFL) && defined(F_SETFL)
  /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
  int flags;
  if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
    flags = 0;
#if defined(O_CLOEXEC)
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC);
#else
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
#elif defined(FIONBIO)
  int flags = 1;
  return ioctl(fd, FIONBIO, &flags);
#else
#error No functions / argument macros for non-blocking sockets.
#endif
}

/** Creates a new network socket and binds it to a local address. */
SFUNC fio_socket_i fio_sock_open_local(struct addrinfo *addr, int nonblock) {
  fio_socket_i fd = FIO_SOCKET_INVALID;
  for (struct addrinfo *p = addr; p != NULL; p = p->ai_next) {
#if FIO_OS_WIN
    /* WSASocket always returns a true Winsock SOCKET — no CRT fd shim. */
    fd = (fio_socket_i)WSASocket(p->ai_family,
                                 p->ai_socktype,
                                 p->ai_protocol,
                                 NULL,
                                 0,
                                 WSA_FLAG_OVERLAPPED);
#else
    fd = (fio_socket_i)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
#endif
    if (!FIO_SOCK_FD_ISVALID(fd)) {
#if FIO_OS_WIN
      const int wsa_err = WSAGetLastError();
      fio___sock_wsa_set_errno_from(wsa_err);
#endif
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
#if FIO_OS_WIN
      FIO_LOG_DEBUG("(fio_sock_open_local) WSASocket failed (WSA error %d)",
                    wsa_err);
#endif
      fd = FIO_SOCKET_INVALID;
      continue;
    }
    { // avoid the "address taken"
      int optval = 1;
#if FIO_OS_WIN
      /* SO_REUSEADDR allows another Windows process to bind this port. */
      if (fio_sock_setsockopt(fd,
                              SOL_SOCKET,
                              SO_EXCLUSIVEADDRUSE,
                              &optval,
                              sizeof(optval)) == -1) {
        fio_sock_close(fd);
        fd = FIO_SOCKET_INVALID;
        continue;
      }
#else
      fio_sock_setsockopt(fd,
                          SOL_SOCKET,
                          SO_REUSEADDR,
                          &optval,
                          sizeof(optval));
#endif
    }
    if (nonblock && fio_sock_set_non_block(fd) == -1) {
      FIO_LOG_DEBUG("Couldn't set socket to non-blocking mode %s",
                    strerror(errno));
      fio_sock_close(fd);
      fd = FIO_SOCKET_INVALID;
      continue;
    }
    if (fio_sock_bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
#if FIO_OS_WIN
      { /* capture error before any other call clears it */
        DWORD wsa_err = WSAGetLastError();
        char wsa_msg[256] = {0};
        DWORD msg_len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                                           FORMAT_MESSAGE_IGNORE_INSERTS,
                                       NULL,
                                       wsa_err,
                                       0,
                                       wsa_msg,
                                       sizeof(wsa_msg),
                                       NULL);
        if (!msg_len)
          FIO_MEMCPY(wsa_msg,
                     "message unavailable",
                     sizeof("message unavailable"));
        wsa_msg[sizeof(wsa_msg) - 1] = 0;
        FIO_LOG_ERROR("(fio_sock_open_local) bind failed (WSA %lu: %s)",
                      (unsigned long)wsa_err,
                      wsa_msg);
      }
#else
      FIO_LOG_DEBUG("Failed attempt to bind socket to address %s",
                    strerror(errno));
#endif
      fio_sock_close(fd);
      fd = FIO_SOCKET_INVALID;
      continue;
    }
    break;
  }
  if (!FIO_SOCK_FD_ISVALID(fd)) {
    FIO_LOG_DEBUG("socket binding/creation error %s", strerror(errno));
  }
  return fd;
}

/** Creates a new network socket and connects it to a remote address. */
SFUNC fio_socket_i fio_sock_open_remote(struct addrinfo *addr, int nonblock) {
  fio_socket_i fd = FIO_SOCKET_INVALID;
  for (struct addrinfo *p = addr; p != NULL; p = p->ai_next) {
#if FIO_OS_WIN
    /* WSASocket always returns a true Winsock SOCKET — no CRT fd shim. */
    fd = (fio_socket_i)WSASocket(p->ai_family,
                                 p->ai_socktype,
                                 p->ai_protocol,
                                 NULL,
                                 0,
                                 WSA_FLAG_OVERLAPPED);
#else
    fd = (fio_socket_i)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
#endif
    if (!FIO_SOCK_FD_ISVALID(fd)) {
#if FIO_OS_WIN
      const int wsa_err = WSAGetLastError();
      fio___sock_wsa_set_errno_from(wsa_err);
#endif
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
#if FIO_OS_WIN
      FIO_LOG_DEBUG("(fio_sock_open_remote) WSASocket failed (WSA error %d)",
                    wsa_err);
#endif
      fd = FIO_SOCKET_INVALID;
      continue;
    }

    if (nonblock && fio_sock_set_non_block(fd) == -1) {
      FIO_LOG_DEBUG("Failed attempt to set client socket to non-blocking %s",
                    strerror(errno));
      fio_sock_close(fd);
      fd = FIO_SOCKET_INVALID;
      continue;
    }
    if (fio_sock_connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
#if FIO_OS_WIN
      if (errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY)
        break;
      FIO_LOG_ERROR("(fio_sock_open_remote) connect failed (errno %d: %s)",
                    errno,
                    strerror(errno));
#else
      if (errno == EINPROGRESS)
        break;
      FIO_LOG_DEBUG("Couldn't connect client socket to remote address %s",
                    strerror(errno));
#endif
      fio_sock_close(fd);
      fd = FIO_SOCKET_INVALID;
      continue;
    }
    break;
  }
  if (!FIO_SOCK_FD_ISVALID(fd)) {
#if FIO_OS_WIN
    FIO_LOG_ERROR("(fio_sock_open_remote) failed to connect socket (WSA %d)",
                  WSAGetLastError());
#else
    FIO_LOG_DEBUG("socket connection/creation error %s", strerror(errno));
#endif
  }
  return fd;
}

/** Returns 0 on timeout, -1 on error or the events that are valid. */
SFUNC short fio_sock_wait_io(fio_socket_i fd, short events, int timeout) {
  short r = 0;
#if FIO_OS_WIN
  if (fd == FIO_SOCKET_INVALID) {
    if (timeout > 0)
      Sleep((DWORD)timeout);
    return POLLNVAL;
  }
#endif
  struct pollfd pfd = {.fd = fd, .events = events};
#if FIO_OS_WIN
  r = (short)WSAPoll(&pfd, 1, timeout);
#else
  r = (short)poll(&pfd, 1, timeout);
#endif
  if (r == -1) {
#if FIO_OS_WIN
    if (fio___sock_wsa_set_errno_last() == ENOTSOCK)
      return POLLNVAL;
#endif
    return -1;
  }
  if (r == 1)
    r = pfd.revents;
  return r;
}

/** Attempts to maximize the allowed open file limits. returns known limit */
SFUNC size_t fio_sock_maximize_limits(size_t max_limit) {
  ssize_t capa = 0;
#if FIO_OS_POSIX
  if (!max_limit)
    max_limit = FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT;

#ifdef _SC_OPEN_MAX
  {
    const long sys_max = sysconf(_SC_OPEN_MAX);
    if (sys_max >= 0)
      capa = (ssize_t)sys_max;
  }
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
  if (!max_limit)
    max_limit = FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT;
  capa = (ssize_t)max_limit;
  FIO_LOG_DEBUG2(
      "Windows has no RLIMIT_NOFILE-equivalent for sockets; using advisory "
      "per-process cap %zu",
      max_limit);
#else
  FIO_LOG_ERROR("No OS detected, couldn't maximize open file limit.");
#endif
  return capa;
}

#ifdef AF_UNIX
/** Creates a new Unix socket and binds it to a local address. */
SFUNC fio_socket_i fio_sock_open_unix(const char *address, uint16_t flags) {
  /* Unix socket */
  struct sockaddr_un addr = {0};
  size_t addr_len = FIO_STRLEN(address);
  if (addr_len >= sizeof(addr.sun_path)) {
    FIO_LOG_ERROR(
        "(fio_sock_open_unix) address too long (%zu bytes > %zu bytes).",
        addr_len,
        sizeof(addr.sun_path) - 1);
    errno = ENAMETOOLONG;
    return FIO_SOCKET_INVALID;
  }
  addr.sun_family = AF_UNIX;
  FIO_MEMCPY(addr.sun_path, address, addr_len + 1); /* copy the NUL byte. */
#if defined(__APPLE__)
  addr.sun_len = addr_len;
#endif
#if FIO_OS_WIN
  /* WSASocket always returns a true Winsock SOCKET — no CRT fd shim. */
  fio_socket_i fd =
      (fio_socket_i)WSASocket(AF_UNIX,
                              (flags & FIO_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM,
                              0,
                              NULL,
                              0,
                              WSA_FLAG_OVERLAPPED);
#else
  fio_socket_i fd =
      (fio_socket_i)socket(AF_UNIX,
                           (flags & FIO_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM,
                           0);
#endif
  if (!FIO_SOCK_FD_ISVALID(fd)) {
#if FIO_OS_WIN
    fio___sock_wsa_set_errno_last();
#endif
    FIO_LOG_ERROR("couldn't open unix socket (flags == %d) %s\n\t%s",
                  (int)flags,
                  strerror(errno),
                  address);
    return FIO_SOCKET_INVALID;
  }
  if ((flags & FIO_SOCK_NONBLOCK) && fio_sock_set_non_block(fd) == -1) {
    FIO_LOG_ERROR("couldn't set socket to non-blocking mode");
    fio_sock_close(fd);
    return FIO_SOCKET_INVALID;
  }
  if ((flags & FIO_SOCK_CLIENT)) {
    if (fio_sock_connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
#if FIO_OS_WIN
      if (errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY)
        return fd;
#else
      if (errno == EINPROGRESS)
        return fd;
#endif
      FIO_LOG_ERROR("couldn't connect unix client @ %s : %s",
                    addr.sun_path,
                    strerror(errno));
      fio_sock_close(fd);
      return FIO_SOCKET_INVALID;
    }
  } else {
    unlink(addr.sun_path);
    /* no `umask` games (process-global, races other threads): public sockets
     * are opened up with `chmod` after `bind`. */
    if (fio_sock_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      FIO_LOG_ERROR("couldn't bind unix socket to %s\n\terrno(%d): %s",
                    address,
                    errno,
                    strerror(errno));
      fio_sock_close(fd);
      // unlink(addr.sun_path);
      return FIO_SOCKET_INVALID;
    }
#if !FIO_OS_WIN /* FIO_OS_WIN is always defined (0 / 1) */
    if ((flags & FIO_SOCK_UNIX_PRIVATE) == FIO_SOCK_UNIX) {
      /* Never chmod a pathname that may have become a symlink after bind.
       * On Linux, O_PATH pins the socket inode even when the path is renamed;
       * /proc/self/fd then gives chmod an inode-bound path. A process able to
       * replace the socket path can still disrupt availability. */
#if defined(__linux__) && defined(O_PATH) && defined(O_NOFOLLOW) &&            \
    defined(O_CLOEXEC)
      int pathfd = open(address, O_PATH | O_NOFOLLOW | O_CLOEXEC);
      struct stat st;
      if (pathfd == -1 || fstat(pathfd, &st) || !S_ISSOCK(st.st_mode)) {
        if (pathfd != -1)
          close(pathfd);
        fio_sock_close(fd);
        errno = EACCES;
        return FIO_SOCKET_INVALID;
      }
      char pinned[64];
      int n = snprintf(pinned, sizeof(pinned), "/proc/self/fd/%d", pathfd);
      int chmod_result = (n > 0 && (size_t)n < sizeof(pinned))
                             ? chmod(pinned, S_IRWXO | S_IRWXG | S_IRWXU)
                             : -1;
      close(pathfd);
#else
      /* AT_SYMLINK_NOFOLLOW must fail closed if the platform cannot enforce
       * it; falling back to chmod(address) would reintroduce the race. */
#if defined(AT_SYMLINK_NOFOLLOW) && defined(AT_FDCWD)
      int chmod_result = fchmodat(AT_FDCWD,
                                  address,
                                  S_IRWXO | S_IRWXG | S_IRWXU,
                                  AT_SYMLINK_NOFOLLOW);
#else
      int chmod_result = -1;
      errno = ENOTSUP;
#endif
#endif
      if (chmod_result == -1) {
        fio_sock_close(fd);
        return FIO_SOCKET_INVALID;
      }
    }
#endif
    if (!(flags & FIO_SOCK_UDP) && fio_sock_listen(fd, SOMAXCONN) < 0) {
      FIO_LOG_ERROR("couldn't start listening to unix socket at %s", address);
      fio_sock_close(fd);
      /* The path may have been replaced since bind; never unlink it here. */
      return FIO_SOCKET_INVALID;
    }
  }
  return fd;
}
#else
SFUNC fio_socket_i fio_sock_open_unix(const char *address, uint16_t flags) {
  (void)address, (void)flags;
  FIO_ASSERT(0, "this system does not support Unix sockets.");
  return FIO_SOCKET_INVALID;
}
#endif /* AF_UNIX */

/* *****************************************************************************
Socket Addresses
***************************************************************************** */
#ifndef FIO___SOCK_ADDR_CAP
#define FIO___SOCK_ADDR_CAP 128
#endif
FIO_ASSERT_STATIC(FIO___SOCK_ADDR_CAP >= 96,
                  "FIO___SOCK_ADDR_CAP can't be less than 96 bytes");
FIO_STATIC_ALLOC_DEF(fio___sock_addr_buffer_alloc,
                     char,
                     FIO___SOCK_ADDR_CAP,
                     128)

FIO_SFUNC fio_buf_info_s fio___sock_addr2txt(struct sockaddr *addr,
                                             socklen_t len,
                                             bool with_port) {
  fio_buf_info_s r = FIO_BUF_INFO2(fio___sock_addr_buffer_alloc(1), 0);
  /* IP endpoints only; Unix sockets have no TCP/UDP port. */
  if (addr[0].sa_family != AF_INET && addr[0].sa_family != AF_INET6)
    goto unix_socket;
  r.buf += 8;
  char *port = r.buf + 72;
  if (getnameinfo(addr,
                  len,
                  r.buf,
                  64,
                  port,
                  16,
                  (NI_NUMERICHOST | (with_port ? NI_NUMERICSERV : 0))))
    goto finish;
  r.len = FIO_STRLEN(r.buf);
  if (with_port) {
    with_port = (addr[0].sa_family == AF_INET6);
    r.buf -= with_port;
    r.len += with_port;
    *(r.buf - (!with_port)) = '[';
    r.buf[r.len] = ']';
    r.len += with_port;
    r.buf[r.len++] = ':';
    fio_memcpy8(r.buf + r.len, port);
    while (r.buf[r.len] && r.len < FIO___SOCK_ADDR_CAP)
      ++r.len;
    r.buf[r.len] = 0; /* just in case the loop reached the end of the buffer */
  }
finish:
  if (!r.len)
    r.buf = NULL;
  return r;

unix_socket:
#ifdef AF_UNIX
  if (addr->sa_family == AF_UNIX) {
    const struct sockaddr_un *un = (const struct sockaddr_un *)addr;
    const size_t offset = offsetof(struct sockaddr_un, sun_path);

    /* No pathname was returned. */
    if ((size_t)len <= offset)
      return FIO_BUF_INFO0;

    size_t available = (size_t)len - offset;
    if (available > sizeof(un->sun_path))
      available = sizeof(un->sun_path);

    /* Reject Linux abstract addresses and empty names. */
    if (un->sun_path[0] == '\0')
      return FIO_BUF_INFO0;

    /* Do not assume the returned pathname is NUL-terminated. */
    size_t path_len = 0;
    while (path_len < available && un->sun_path[path_len])
      ++path_len;

    if (path_len && path_len >= FIO___SOCK_ADDR_CAP)
      return FIO_BUF_INFO0;

    FIO_MEMCPY(r.buf, un->sun_path, path_len);
    r.buf[path_len] = '\0';
    r.len = path_len;
    return r;
  }
#endif
  return FIO_BUF_INFO0;
}

SFUNC fio_buf_info_s fio_sock_addr(fio_socket_i s, bool with_port) {
  struct sockaddr addr[8] = {0};
  socklen_t len = sizeof(addr);
  if (!FIO_SOCK_FD_ISVALID(s))
    return FIO_BUF_INFO0;
#if FIO_OS_WIN
  if (fio___winsock_fn.getsockname(s, addr, (int *)&len))
    return FIO_BUF_INFO0;
#else
  if (getsockname(s, addr, &len))
    return FIO_BUF_INFO0;
#endif
  if ((size_t)len > sizeof(addr))
    return FIO_BUF_INFO0;
  return fio___sock_addr2txt(addr, len, with_port);
}

SFUNC fio_buf_info_s fio_sock_peer_addr(fio_socket_i s, bool with_port) {
  struct sockaddr addr[8] = {0};
  socklen_t len = sizeof(addr);
  if (!FIO_SOCK_FD_ISVALID(s))
    return FIO_BUF_INFO0;
#if FIO_OS_WIN
  if (fio___winsock_fn.getpeername(s, addr, (int *)&len))
    return FIO_BUF_INFO0;
#else
  if (getpeername(s, addr, &len))
    return FIO_BUF_INFO0;
#endif
  if ((size_t)len > sizeof(addr))
    return FIO_BUF_INFO0;
  return fio___sock_addr2txt(addr, len, with_port);
}

/* *****************************************************************************
WinSock initialization
***************************************************************************** */
#if FIO_OS_WIN
/* Definition of the Winsock function pointer table (one copy per binary). */
struct fio___winsock_fn_s fio___winsock_fn;

/* Maps Winsock errors to POSIX-style errno values for I/O wrappers. */
FIO_IFUNC int fio___sock_wsa2errno(int wsa_err) {
  switch (wsa_err) {
  case WSAEWOULDBLOCK: return EWOULDBLOCK;
  case WSAEINPROGRESS: return EINPROGRESS;
  case WSAEINTR: return EINTR;
  case WSAEALREADY: return EALREADY;
  case WSAENOTSOCK: return ENOTSOCK;
  case WSAEMSGSIZE: return EMSGSIZE;
  case WSAEDESTADDRREQ: return EDESTADDRREQ;
  case WSAEPROTOTYPE: return EPROTOTYPE;
  case WSAENOPROTOOPT: return ENOPROTOOPT;
  case WSAEPROTONOSUPPORT: return EPROTONOSUPPORT;
  case WSAEOPNOTSUPP: return EOPNOTSUPP;
  case WSAEAFNOSUPPORT: return EAFNOSUPPORT;
  case WSAEADDRINUSE: return EADDRINUSE;
  case WSAEADDRNOTAVAIL: return EADDRNOTAVAIL;
  case WSAENETDOWN: return ENETDOWN;
  case WSAENETUNREACH: return ENETUNREACH;
  case WSAENETRESET: return ENETRESET;
  case WSAECONNABORTED: return ECONNABORTED;
  case WSAECONNRESET: return ECONNRESET;
  case WSAENOBUFS: return ENOBUFS;
  case WSAEISCONN: return EISCONN;
  case WSAENOTCONN: return ENOTCONN;
  case WSAETIMEDOUT: return ETIMEDOUT;
  case WSAECONNREFUSED: return ECONNREFUSED;
  case WSAEHOSTUNREACH: return EHOSTUNREACH;
  case WSAEHOSTDOWN:
#ifdef EHOSTDOWN
    return EHOSTDOWN;
#else
    return EHOSTUNREACH;
#endif
  case WSAEINVAL: return EINVAL;
  default: return EIO;
  }
}
FIO_IFUNC int fio___sock_wsa_set_errno_from(int wsa_err) {
  errno = fio___sock_wsa2errno(wsa_err);
  return errno;
}
FIO_IFUNC int fio___sock_wsa_set_errno_last(void) {
  return fio___sock_wsa_set_errno_from(WSAGetLastError());
}
FIO_IFUNC void fio___sock_wsa_set_errno(void) {
  (void)fio___sock_wsa_set_errno_last();
}

/** Acts as POSIX write. Use this function for portability with WinSock2.
 * send/recv are the correct Winsock2 functions for WSAPoll-based I/O;
 * WSASend/WSARecv are for IOCP/overlapped I/O which this library does not use.
 */
IFUNC ssize_t fio_sock_write(fio_socket_i fd, const void *buf, size_t len) {
  if (len > INT_MAX) {
    errno = EINVAL;
    return -1;
  }
  const int r = fio___winsock_fn.send(fd, (const char *)buf, (int)len, 0);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno();
    return -1;
  }
  return (ssize_t)r;
}
/** Acts as POSIX read. Use this function for portability with WinSock2. */
IFUNC ssize_t fio_sock_read(fio_socket_i fd, void *buf, size_t len) {
  if (len > INT_MAX) {
    errno = EINVAL;
    return -1;
  }
  const int r = fio___winsock_fn.recv(fd, (char *)buf, (int)len, 0);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno();
    return -1;
  }
  return (ssize_t)r;
}
/** Acts as POSIX close. Use this function for portability with WinSock2. */
IFUNC int fio_sock_close(fio_socket_i fd) {
  const int r = fio___winsock_fn.closesocket(fd);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno_last();
    return -1;
  }
  return r;
}
/** Acts as POSIX sendto. Use this function for portability with WinSock2. */
IFUNC ssize_t fio_sock_sendto(fio_socket_i fd,
                              const void *buf,
                              size_t len,
                              int flags,
                              const struct sockaddr *addr,
                              socklen_t addrlen) {
  if (len > INT_MAX || (uint64_t)addrlen > INT_MAX) {
    errno = EINVAL;
    return -1;
  }
  const int r = fio___winsock_fn.sendto(fd,
                                        (const char *)buf,
                                        (int)len,
                                        flags,
                                        addr,
                                        (int)addrlen);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno();
    return -1;
  }
  return (ssize_t)r;
}
/** Acts as POSIX recvfrom. Use this function for portability with WinSock2. */
IFUNC ssize_t fio_sock_recvfrom(fio_socket_i fd,
                                void *buf,
                                size_t len,
                                int flags,
                                struct sockaddr *addr,
                                socklen_t *addrlen) {
  if (len > INT_MAX || (addrlen && (uint64_t)*addrlen > INT_MAX)) {
    errno = EINVAL;
    return -1;
  }
  const int r = fio___winsock_fn.recvfrom(fd,
                                          (char *)buf,
                                          (int)len,
                                          flags,
                                          addr,
                                          (int *)addrlen);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno();
    return -1;
  }
  return (ssize_t)r;
}
/** Accepts a new connection, returning a native socket handle. */
IFUNC fio_socket_i fio_sock_accept(fio_socket_i s,
                                   struct sockaddr *addr,
                                   int *addrlen) {
  fio_socket_i c = fio___winsock_fn.accept(s, addr, addrlen);
  if (c == INVALID_SOCKET) {
    fio___sock_wsa_set_errno_last();
    return FIO_SOCKET_INVALID;
  }
  return c;
}
/** Duplicates a Winsock2 socket, acting as POSIX dup for portability. */
IFUNC fio_socket_i fio_sock_dup(fio_socket_i original) {
  WSAPROTOCOL_INFO info;
  if (WSADuplicateSocket(original, GetCurrentProcessId(), &info)) {
    fio___sock_wsa_set_errno_last();
    FIO_LOG_ERROR("(fio_sock_dup) WSADuplicateSocket failed (WSA error %d)",
                  WSAGetLastError());
    return FIO_SOCKET_INVALID;
  }
  fio_socket_i fd = (fio_socket_i)WSASocket(FROM_PROTOCOL_INFO,
                                            FROM_PROTOCOL_INFO,
                                            FROM_PROTOCOL_INFO,
                                            &info,
                                            0,
                                            0);
  if (!FIO_SOCK_FD_ISVALID(fd)) {
    fio___sock_wsa_set_errno_last();
    FIO_LOG_ERROR("(fio_sock_dup) WSASocket failed (WSA error %d)",
                  WSAGetLastError());
  }
  return fd;
}
/** Portable bind. Calls Winsock2 bind on Windows, POSIX bind on POSIX. */
IFUNC int fio_sock_bind(fio_socket_i fd,
                        const struct sockaddr *addr,
                        socklen_t addrlen) {
  if ((uint64_t)addrlen > INT_MAX) {
    errno = EINVAL;
    return -1;
  }
  const int r = fio___winsock_fn.bind(fd, addr, (int)addrlen);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno_last();
    return -1;
  }
  return r;
}
/** Portable connect. Calls Winsock2 connect on Windows, POSIX connect on POSIX.
 */
IFUNC int fio_sock_connect(fio_socket_i fd,
                           const struct sockaddr *addr,
                           socklen_t addrlen) {
  if ((uint64_t)addrlen > INT_MAX) {
    errno = EINVAL;
    return -1;
  }
  const int r = fio___winsock_fn.connect(fd, addr, (int)addrlen);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno_last();
    return -1;
  }
  return r;
}
/** Portable listen. Calls Winsock2 listen on Windows, POSIX listen on POSIX. */
IFUNC int fio_sock_listen(fio_socket_i fd, int backlog) {
  const int r = fio___winsock_fn.listen(fd, backlog);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno_last();
    return -1;
  }
  return r;
}
/** Portable setsockopt.
 * optval is (const char *) on Windows and (const void *) on POSIX;
 * optlen is int on Windows and socklen_t on POSIX.
 * Both coercions are applied internally; callers pass any pointer type. */
IFUNC int fio_sock_setsockopt(fio_socket_i fd,
                              int level,
                              int optname,
                              const void *optval,
                              socklen_t optlen) {
  if ((uint64_t)optlen > INT_MAX) {
    errno = EINVAL;
    return -1;
  }
  const int r = fio___winsock_fn.setsockopt(fd,
                                            level,
                                            optname,
                                            (const char *)optval,
                                            (int)optlen);
  if (r == SOCKET_ERROR) {
    fio___sock_wsa_set_errno_last();
    return -1;
  }
  return r;
}
/** Creates a connected socket pair via loopback TCP (Windows socketpair).
 * On Windows, fio_sock_pipe() delegates here since _pipe() returns CRT fds
 * (not Winsock SOCKETs) and is incompatible with WSAPoll-based I/O. */
IFUNC int fio_sock_socketpair(fio_socket_i fds[2]) {
  fio_socket_i listener = INVALID_SOCKET;
  fio_socket_i writer = INVALID_SOCKET;
  fio_socket_i reader = INVALID_SOCKET;
  struct sockaddr_in addr;
  int addrlen = (int)sizeof(addr);
  fds[0] = fds[1] = INVALID_SOCKET;
  /* WSASocket always returns a true Winsock SOCKET — no CRT fd shim. */
  listener = (fio_socket_i)WSASocket(AF_INET,
                                     SOCK_STREAM,
                                     IPPROTO_TCP,
                                     NULL,
                                     0,
                                     WSA_FLAG_OVERLAPPED);
  if (listener == INVALID_SOCKET) {
    fio___sock_wsa_set_errno_last();
    goto fail;
  }
  { /* Protect the temporary listener from same-port Windows binds. */
    int exclusive = 1;
    if (fio_sock_setsockopt(listener,
                            SOL_SOCKET,
                            SO_EXCLUSIVEADDRUSE,
                            &exclusive,
                            sizeof(exclusive)) == -1)
      goto fail;
  }
  FIO_MEMSET(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0; /* OS assigns an ephemeral port */
  if (fio_sock_bind(listener, (struct sockaddr *)&addr, addrlen) ==
      SOCKET_ERROR)
    goto fail;
  if (fio___winsock_fn.getsockname(listener,
                                   (struct sockaddr *)&addr,
                                   &addrlen) == SOCKET_ERROR) {
    fio___sock_wsa_set_errno_last();
    goto fail;
  }
  if (fio_sock_listen(listener, 1) == SOCKET_ERROR)
    goto fail;
  writer = (fio_socket_i)WSASocket(AF_INET,
                                   SOCK_STREAM,
                                   IPPROTO_TCP,
                                   NULL,
                                   0,
                                   WSA_FLAG_OVERLAPPED);
  if (writer == INVALID_SOCKET) {
    fio___sock_wsa_set_errno_last();
    goto fail;
  }
  if (fio_sock_connect(writer, (struct sockaddr *)&addr, addrlen) ==
      SOCKET_ERROR)
    goto fail;
  /* A different loopback process can race the writer. Accept only the peer
   * whose ephemeral endpoint belongs to our connected writer, and bound the
   * total wait so a substituted peer cannot hold setup indefinitely. */
  struct sockaddr_in writer_addr;
  int writer_addrlen = (int)sizeof(writer_addr);
  if (fio___winsock_fn.getsockname(writer,
                                   (struct sockaddr *)&writer_addr,
                                   &writer_addrlen) == SOCKET_ERROR) {
    fio___sock_wsa_set_errno_last();
    goto fail;
  }
  DWORD started = GetTickCount();
  for (;;) {
    DWORD elapsed = GetTickCount() - started;
    if (elapsed >= 1000) {
      errno = ETIMEDOUT;
      goto fail;
    }
    fd_set pending;
    FD_ZERO(&pending);
    FD_SET(listener, &pending);
    struct timeval wait = {.tv_sec = 0,
                           .tv_usec = (long)(1000 - elapsed) * 1000};
    int ready = fio___winsock_fn.select(0, &pending, NULL, NULL, &wait);
    if (ready <= 0) {
      if (ready == SOCKET_ERROR)
        fio___sock_wsa_set_errno_last();
      else
        errno = ETIMEDOUT;
      goto fail;
    }
    struct sockaddr_in peer_addr;
    int peer_addrlen = (int)sizeof(peer_addr);
    reader = fio___winsock_fn.accept(listener,
                                     (struct sockaddr *)&peer_addr,
                                     &peer_addrlen);
    if (reader == INVALID_SOCKET) {
      fio___sock_wsa_set_errno_last();
      goto fail;
    }
    if (peer_addrlen == writer_addrlen && peer_addr.sin_family == AF_INET &&
        peer_addr.sin_addr.s_addr == writer_addr.sin_addr.s_addr &&
        peer_addr.sin_port == writer_addr.sin_port)
      break;
    fio___winsock_fn.closesocket(reader);
    reader = INVALID_SOCKET;
  }
  fio___winsock_fn.closesocket(listener);
  fds[0] = reader; /* read end */
  fds[1] = writer; /* write end */
  return 0;
fail:
  if (listener != INVALID_SOCKET)
    fio___winsock_fn.closesocket(listener);
  if (reader != INVALID_SOCKET)
    fio___winsock_fn.closesocket(reader);
  if (writer != INVALID_SOCKET)
    fio___winsock_fn.closesocket(writer);
  return -1;
}

static WSADATA fio___sock_useless_windows_data;
FIO_CONSTRUCTOR(fio___sock_win_init) {
  static uint8_t flag = 0;
  /* Load real Winsock function pointers via GetProcAddress so that Ruby's
   * win32.h macro redefinitions (rb_w32_bind, rb_w32_send, etc.) can never
   * intercept our calls — symbol resolution happens at link time for static
   * inline functions, making #undef blocks ineffective. By calling through
   * these pointers we bypass the linker symbol table entirely.
   * NOTE: do NOT FreeLibrary(ws2) — pointers must remain valid for the
   * process lifetime. */
  HMODULE ws2 = LoadLibraryA("Ws2_32.dll");
  FIO_ASSERT(ws2, "failed to load Ws2_32.dll");
  if (!flag) {
    flag |= 1;
    {
      int(WSAAPI * fn_WSAStartup)(WORD, LPWSADATA) =
          (void *)GetProcAddress(ws2, "WSAStartup");
      FIO_ASSERT(fn_WSAStartup, "Ws2_32.dll missing: WSAStartup");
      if (fn_WSAStartup(MAKEWORD(2, 2), &fio___sock_useless_windows_data)) {
        FIO_LOG_FATAL("WinSock2 unavailable.");
        exit(-1);
      }
    }
    {
      void(WSAAPI * fn_WSACleanup)(void) =
          (void *)GetProcAddress(ws2, "WSACleanup");
      if (fn_WSACleanup)
        atexit(fn_WSACleanup);
    }
#define FIO___LOAD_WS2(name)                                                   \
  fio___winsock_fn.name = (void *)GetProcAddress(ws2, #name);                  \
  FIO_ASSERT(fio___winsock_fn.name, "Ws2_32.dll missing: " #name)
    FIO___LOAD_WS2(bind);
    FIO___LOAD_WS2(connect);
    FIO___LOAD_WS2(listen);
    FIO___LOAD_WS2(accept);
    FIO___LOAD_WS2(socket);
    FIO___LOAD_WS2(send);
    FIO___LOAD_WS2(recv);
    FIO___LOAD_WS2(sendto);
    FIO___LOAD_WS2(recvfrom);
    FIO___LOAD_WS2(setsockopt);
    FIO___LOAD_WS2(getsockopt);
    FIO___LOAD_WS2(ioctlsocket);
    FIO___LOAD_WS2(closesocket);
    FIO___LOAD_WS2(select);
    FIO___LOAD_WS2(getsockname);
    FIO___LOAD_WS2(getpeername);
#undef FIO___LOAD_WS2
  }
}
#endif /* FIO_OS_WIN / FIO_OS_POSIX */

/* *****************************************************************************
FIO_SOCK - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SOCK
#endif
