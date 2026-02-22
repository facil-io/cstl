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
/** Acts as POSIX write. Use this macro for portability with WinSock2. */
#define fio_sock_write(fd, data, len) send((fd), (data), (len), 0)
/** Acts as POSIX read. Use this macro for portability with WinSock2. */
#define fio_sock_read(fd, buf, len) recv((fd), (buf), (len), 0)
/** Acts as POSIX close. Use this macro for portability with WinSock2. */
#define fio_sock_close(fd) closesocket(fd)
/** Accepts a new connection, returning a native socket handle. */
FIO_IFUNC fio_socket_i fio_sock_accept(fio_socket_i s,
                                       struct sockaddr *addr,
                                       int *addrlen) {
  fio_socket_i c = accept(s, addr, addrlen);
  if (c == INVALID_SOCKET)
    return FIO_SOCKET_INVALID;
  return c;
}
#define accept fio_sock_accept
/** Acts as POSIX dup. Use this for portability with WinSock2.
 *
 * Uses DuplicateHandle for same-process socket duplication.
 *
 * MSDN (WSADuplicateSocket): "The special WSAPROTOCOL_INFO structure can only
 * be used once by the target process." and the WSASocket dwFlags must exactly
 * match the original socket's overlapped flag — mismatches cause WSAEINVAL.
 * DuplicateHandle avoids all of this: it duplicates any kernel object handle
 * (including Winsock SOCKETs) within the same process without restrictions.
 * This is the same approach used by libuv for same-process socket duplication.
 * See:
 * https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle
 */
FIO_IFUNC fio_socket_i fio_sock_dup(fio_socket_i original) {
  HANDLE dup_handle;
  if (!DuplicateHandle(GetCurrentProcess(),
                       (HANDLE)(UINT_PTR)original,
                       GetCurrentProcess(),
                       &dup_handle,
                       0,
                       FALSE,
                       DUPLICATE_SAME_ACCESS)) {
    FIO_LOG_ERROR("(fio_sock_dup) DuplicateHandle failed (error %lu)",
                  (unsigned long)GetLastError());
    return FIO_SOCKET_INVALID;
  }
  return (fio_socket_i)(UINT_PTR)dup_handle;
}
/** Creates a connected socket pair via loopback TCP (Windows socketpair). */
FIO_IFUNC int fio_sock_socketpair(fio_socket_i fds[2]) {
  fio_socket_i listener = INVALID_SOCKET;
  fio_socket_i writer = INVALID_SOCKET;
  fio_socket_i reader = INVALID_SOCKET;
  struct sockaddr_in addr;
  int addrlen = (int)sizeof(addr);
  fds[0] = fds[1] = INVALID_SOCKET;
  listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener == INVALID_SOCKET)
    goto fail;
  FIO_MEMSET(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0; /* OS assigns an ephemeral port */
  if (bind(listener, (struct sockaddr *)&addr, addrlen) == SOCKET_ERROR)
    goto fail;
  if (getsockname(listener, (struct sockaddr *)&addr, &addrlen) == SOCKET_ERROR)
    goto fail;
  if (listen(listener, 1) == SOCKET_ERROR)
    goto fail;
  writer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (writer == INVALID_SOCKET)
    goto fail;
  if (connect(writer, (struct sockaddr *)&addr, addrlen) == SOCKET_ERROR)
    goto fail;
  reader = accept(listener, NULL, NULL);
  if (reader == INVALID_SOCKET)
    goto fail;
  closesocket(listener);
  fds[0] = reader; /* read end */
  fds[1] = writer; /* write end */
  return 0;
fail:
  if (listener != INVALID_SOCKET)
    closesocket(listener);
  if (reader != INVALID_SOCKET)
    closesocket(reader);
  if (writer != INVALID_SOCKET)
    closesocket(writer);
  return -1;
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
/** Acts as POSIX write. Use this macro for portability with WinSock2. */
#define fio_sock_write(fd, data, len)      write((fd), (data), (len))
/** Acts as POSIX read. Use this macro for portability with WinSock2. */
#define fio_sock_read(fd, buf, len)        read((fd), (buf), (len))
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
/** Acts as POSIX close. Use this macro for portability with WinSock2. */
#define fio_sock_close(fd)                 close(fd)
/** Acts as POSIX accept. Use this macro for portability with WinSock2. */
#define fio_sock_accept(fd, addr, addrlen) accept(fd, addr, addrlen)
/** Creates a connected socket pair using POSIX socketpair(). */
FIO_IFUNC int fio_sock_socketpair(fio_socket_i fds[2]) {
  return socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
}
#else
#error FIO_SOCK requires a supported OS (Windows / POSIX).
#endif

/* Set to 1 if in need to debug unexpected IO closures. */
#if defined(DEBUG) && 0
#define close(fd)                                                              \
  do {                                                                         \
    FIO_LOG_DWARNING("(%d) (" FIO__FILE__ ":" FIO_MACRO2STR(                   \
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
 * Returns a human readable address representation of the socket's peer address.
 *
 * On error, returns a NULL buffer with zero length.
 *
 * Buffer lengths are limited to 63 bytes.
 *
 * This function is limited in its thread safety to 128 threads / calls.
 */
SFUNC fio_buf_info_s fio_sock_peer_addr(fio_socket_i s);

/** Creates a new network socket and binds it to a local address. */
SFUNC fio_socket_i fio_sock_open_local(struct addrinfo *addr, int nonblock);

/** Creates a new network socket and connects it to a remote address. */
SFUNC fio_socket_i fio_sock_open_remote(struct addrinfo *addr, int nonblock);

/** Creates a new Unix socket and binds it to a local address. */
SFUNC fio_socket_i fio_sock_open_unix(const char *address, uint16_t flags);

/** Sets a file descriptor / socket to non blocking state. */
SFUNC int fio_sock_set_non_block(fio_socket_i fd);

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
  (!(fio_sock_wait_io(fd, (POLLOUT | POLLRDHUP), 0) &                          \
     (POLLRDHUP | POLLHUP | POLLNVAL)))
#else
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (!(fio_sock_wait_io(fd, POLLOUT, 0) & (POLLHUP | POLLNVAL)))
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
      if (FIO_SOCK_FD_ISVALID(fd) && listen(fd, SOMAXCONN) == -1) {
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
      FIO_LOG_ERROR(
          "Couldn't open unix socket to %s - host name too long (%zu).",
          url,
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
      FIO_LOG_ERROR("Couldn't open socket to %s - port / scheme too long.",
                    url);
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
      FIO_LOG_ERROR("Couldn't open socket to %s - host name too long.", url);
      return FIO_SOCKET_INVALID;
    }
    FIO_MEMCPY(buf, u.host.buf, u.host.len);
    buf[u.host.len] = 0;
  } else {
    addr = NULL;
  }
  return fio_sock_open(addr, pr, flags);
}

/** Sets a file descriptor / socket to non blocking state. */
SFUNC int fio_sock_set_non_block(fio_socket_i fd) {
/* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK) && defined(F_GETFL) && defined(F_SETFL)
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
SFUNC fio_socket_i fio_sock_open_local(struct addrinfo *addr, int nonblock) {
  fio_socket_i fd = FIO_SOCKET_INVALID;
  for (struct addrinfo *p = addr; p != NULL; p = p->ai_next) {
    fd = (fio_socket_i)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (!FIO_SOCK_FD_ISVALID(fd)) {
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
      fd = FIO_SOCKET_INVALID;
      continue;
    }
    { // avoid the "address taken"
      int optval = 1;
      setsockopt(fd,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 (const char *)&optval,
                 sizeof(optval));
    }
    if (nonblock && fio_sock_set_non_block(fd) == -1) {
      FIO_LOG_DEBUG("Couldn't set socket to non-blocking mode %s",
                    strerror(errno));
      fio_sock_close(fd);
      fd = FIO_SOCKET_INVALID;
      continue;
    }
    if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
      FIO_LOG_DEBUG("Failed attempt to bind socket to address %s",
                    strerror(errno));
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
    fd = (fio_socket_i)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (!FIO_SOCK_FD_ISVALID(fd)) {
      FIO_LOG_DEBUG("socket creation error %s", strerror(errno));
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
    if (connect(fd, p->ai_addr, p->ai_addrlen) == -1 &&
#if FIO_OS_WIN
        (WSAGetLastError() != WSAEWOULDBLOCK || errno != EINPROGRESS)
#else
        errno != EINPROGRESS
#endif
    ) {
#if FIO_OS_WIN
      FIO_LOG_DEBUG("Couldn't connect client socket to remote address %s (%d)",
                    strerror(errno),
                    WSAGetLastError());
#else
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
    FIO_LOG_DEBUG("socket connection/creation error %s", strerror(errno));
  }
  return fd;
}

/** Returns 0 on timeout, -1 on error or the events that are valid. */
SFUNC short fio_sock_wait_io(fio_socket_i fd, short events, int timeout) {
  short r = 0;
#if FIO_OS_WIN
  if (fd == FIO_SOCKET_INVALID) {
    FIO_THREAD_WAIT((timeout * 1000000));
    return r;
  }
#endif
  struct pollfd pfd = {.fd = fd, .events = events};
#if FIO_OS_WIN
  r = (short)WSAPoll(&pfd, 1, timeout);
#else
  r = (short)poll(&pfd, 1, timeout);
#endif
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
SFUNC fio_socket_i fio_sock_open_unix(const char *address, uint16_t flags) {
  /* Unix socket */
  struct sockaddr_un addr = {0};
  size_t addr_len = strlen(address);
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
  fio_socket_i fd =
      (fio_socket_i)socket(AF_UNIX,
                           (flags & FIO_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM,
                           0);
  if (!FIO_SOCK_FD_ISVALID(fd)) {
    FIO_LOG_ERROR("couldn't open unix socket (flags == %d) %s\n\t%s",
                  (int)flags,
                  strerror(errno),
                  address);
    return FIO_SOCKET_INVALID;
  }
  if ((flags & FIO_SOCK_NONBLOCK) && fio_sock_set_non_block(fd) == -1) {
    FIO_LOG_ERROR("couldn't set socket to non-blocking mode");
    fio_sock_close(fd);
    unlink(addr.sun_path);
    return FIO_SOCKET_INVALID;
  }
  if ((flags & FIO_SOCK_CLIENT)) {
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1 &&
        errno != EINPROGRESS) {
      FIO_LOG_ERROR("couldn't connect unix client @ %s : %s",
                    addr.sun_path,
                    strerror(errno));
      fio_sock_close(fd);
      return FIO_SOCKET_INVALID;
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
      FIO_LOG_ERROR("couldn't bind unix socket to %s\n\terrno(%d): %s",
                    address,
                    errno,
                    strerror(errno));
      fio_sock_close(fd);
      // unlink(addr.sun_path);
      return FIO_SOCKET_INVALID;
    }
#ifndef FIO_OS_WIN
    if ((flags & FIO_SOCK_UNIX_PRIVATE) == FIO_SOCK_UNIX) {
      chmod(address, S_IRWXO | S_IRWXG | S_IRWXU);
      fchmod(fd, S_IRWXO | S_IRWXG | S_IRWXU);
    }
#endif
    if (!(flags & FIO_SOCK_UDP) && listen(fd, SOMAXCONN) < 0) {
      FIO_LOG_ERROR("couldn't start listening to unix socket at %s", address);
      fio_sock_close(fd);
      unlink(addr.sun_path);
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
Peer Address
***************************************************************************** */

/**
 * Returns a human readable address representation of the socket's peer address.
 *
 * On error, returns a NULL buffer with zero length.
 *
 * Buffer lengths are limited to 63 bytes.
 *
 * This function is limited in its thread safety to 128 threads / calls.
 */
SFUNC fio_buf_info_s fio_sock_peer_addr(fio_socket_i s) {
  static char buffer[8129]; /* 64 byte per buffer x 128 threads */
  static unsigned pos = 0;
  fio_buf_info_s r =
      FIO_BUF_INFO2(buffer + (fio_atomic_add(&pos, 63) & 127), 0);
  struct sockaddr addr[8] = {0};
  socklen_t len = sizeof(addr);
  if (!FIO_SOCK_FD_ISVALID(s))
    goto finish;
  if (getpeername(s, addr, &len))
    goto finish;
  if (getnameinfo(addr, len, r.buf, 64, NULL, 0, NI_NUMERICHOST))
    goto finish;
  r.len = FIO_STRLEN(r.buf);
finish:
  if (!r.len)
    r.buf = NULL;
  return r;
}

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
