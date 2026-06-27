# Basic Socket Helpers

```c
#define FIO_SOCK
#include "fio-stl.h"
```

Portable socket helpers for POSIX and Windows: open, bind, connect, listen, wait, read, write, close, and a few sharp socket-shaped knives. Implemented in [`./004 sock.h`](./004%20sock.h).

`FIO_SOCK` depends on URL parsing internally and pulls in what it needs.

### Configuration Macros

#### `FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT`

```c
#ifndef FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT
#define FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT (1ULL << 24)
#endif
```

Default target used by `fio_sock_maximize_limits(0)`.

#### `FIO_SOCK_AVOID_UMASK`

```c
/* optional compile-time flag */
#define FIO_SOCK_AVOID_UMASK
```

If defined before including the implementation, Unix socket creation avoids the temporary `umask` call used when making public Unix sockets. This avoids `umask`'s process-global race, but may affect permissions on systems where `chmod` on Unix socket files is not enough.

Windows behaves as if this concern does not apply.

### Types and Flags

#### `fio_socket_i`

```c
#if FIO_OS_WIN
typedef SOCKET fio_socket_i;
#else
typedef int fio_socket_i;
#endif
```

Native socket handle type. Use this instead of `int` when writing portable code.

#### `FIO_SOCKET_INVALID`

```c
#if FIO_OS_WIN
#define FIO_SOCKET_INVALID INVALID_SOCKET
#else
#define FIO_SOCKET_INVALID (-1)
#endif
```

Invalid socket sentinel.

#### `FIO_SOCK_FD_ISVALID`

```c
#define FIO_SOCK_FD_ISVALID(fd) ((fio_socket_i)(fd) != FIO_SOCKET_INVALID)
```

Returns non-zero if `fd` is not the invalid sentinel.

#### `fio_sock_open_flags_e`

```c
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
```

Flags for `fio_sock_open`, `fio_sock_open2`, and `fio_sock_open_unix`.

**Values:**
- `FIO_SOCK_SERVER` - bind a local socket; TCP / stream sockets also call `listen`
- `FIO_SOCK_CLIENT` - connect a remote socket
- `FIO_SOCK_NONBLOCK` - set non-blocking mode
- `FIO_SOCK_TCP` - stream socket
- `FIO_SOCK_UDP` - datagram socket
- `FIO_SOCK_UNIX` - Unix domain socket where supported, otherwise `0`
- `FIO_SOCK_UNIX_PRIVATE` - Unix socket with private permissions where supported, otherwise `0`

`FIO_SOCK_TCP` and `FIO_SOCK_UDP` are exclusive. If neither is set, network sockets default to TCP.

### Opening Sockets

#### `fio_sock_open`

```c
FIO_IFUNC fio_socket_i fio_sock_open(const char *restrict address,
                                     const char *restrict port,
                                     uint16_t flags);
```

Creates a socket from an address / port pair and flags.

For `FIO_SOCK_UNIX`, `port` is ignored and `address` is treated as the Unix socket path. For server TCP sockets, `address == NULL` binds to all interfaces.

**Returns:** a socket handle, or `FIO_SOCKET_INVALID` on error.

#### `fio_sock_open2`

```c
SFUNC fio_socket_i fio_sock_open2(const char *url, uint16_t flags);
```

Creates a socket from a URL-ish string. If no port is present, the URL scheme is used as the port / service name. Schemes such as `tcp`, `udp`, `http`, and `https` can also imply socket type when flags omit it.

Unix path URLs are detected when supported. A `priv` scheme selects `FIO_SOCK_UNIX_PRIVATE`; other path-only Unix forms select `FIO_SOCK_UNIX`.

**Returns:** a socket handle, or `FIO_SOCKET_INVALID` on error.

#### `fio_sock_address_new`

```c
FIO_IFUNC struct addrinfo *fio_sock_address_new(const char *restrict address,
                                                const char *restrict port,
                                                int sock_type);
```

Resolves `address` / `port` into an `addrinfo` list for `sock_type`, usually `SOCK_STREAM` or `SOCK_DGRAM`.

Common service names are normalized before `getaddrinfo`: `ws` / `http` / `sse` to port `80`, and `wss` / `https` / `sses` to port `443`.

**Ownership:** free the result with `fio_sock_address_free`.

#### `fio_sock_address_free`

```c
FIO_IFUNC void fio_sock_address_free(struct addrinfo *a);
```

Frees an address list returned by `fio_sock_address_new`.

#### `fio_sock_open_local`

```c
SFUNC fio_socket_i fio_sock_open_local(struct addrinfo *addr, int nonblock);
```

Creates a network socket from an `addrinfo` list and binds it locally. `SO_REUSEADDR` is enabled. If `nonblock` is non-zero, non-blocking mode is requested before binding.

**Returns:** a socket handle, or `FIO_SOCKET_INVALID`.

#### `fio_sock_open_remote`

```c
SFUNC fio_socket_i fio_sock_open_remote(struct addrinfo *addr, int nonblock);
```

Creates a network socket from an `addrinfo` list and connects it remotely. Non-blocking connect progress (`EINPROGRESS` / Windows equivalents) returns the socket.

**Returns:** a socket handle, or `FIO_SOCKET_INVALID`.

#### `fio_sock_open_unix`

```c
SFUNC fio_socket_i fio_sock_open_unix(const char *address, uint16_t flags);
```

Creates a Unix domain socket where supported. Server sockets unlink an existing path before binding. Non-UDP server sockets call `listen`.

If Unix sockets are unsupported, the function asserts and returns `FIO_SOCKET_INVALID`.

### Portable Socket Operations

Use these wrappers when code should run on both POSIX and WinSock2.

#### `fio_sock_write`

```c
FIO_IFUNC ssize_t fio_sock_write(fio_socket_i fd, const void *buf, size_t len);
```

Acts like POSIX `write` on POSIX and uses `send` on Windows.

#### `fio_sock_read`

```c
FIO_IFUNC ssize_t fio_sock_read(fio_socket_i fd, void *buf, size_t len);
```

Acts like POSIX `read` on POSIX and uses `recv` on Windows.

#### `fio_sock_sendto`

```c
FIO_IFUNC ssize_t fio_sock_sendto(fio_socket_i fd,
                                  const void *buf,
                                  size_t len,
                                  int flags,
                                  const struct sockaddr *addr,
                                  socklen_t addrlen);
```

Portable `sendto` wrapper.

#### `fio_sock_recvfrom`

```c
FIO_IFUNC ssize_t fio_sock_recvfrom(fio_socket_i fd,
                                    void *buf,
                                    size_t len,
                                    int flags,
                                    struct sockaddr *addr,
                                    socklen_t *addrlen);
```

Portable `recvfrom` wrapper.

#### `fio_sock_close`

```c
FIO_IFUNC int fio_sock_close(fio_socket_i fd);
```

Portable close wrapper: `close` on POSIX, `closesocket` on Windows.

#### `fio_sock_accept`

```c
#if FIO_OS_WIN
IFUNC fio_socket_i fio_sock_accept(fio_socket_i s,
                                   struct sockaddr *addr,
                                   int *addrlen);
#else
#define fio_sock_accept(fd, addr, addrlen) accept(fd, addr, addrlen)
#endif
```

Portable accept wrapper.

#### `fio_sock_dup`

```c
FIO_IFUNC fio_socket_i fio_sock_dup(fio_socket_i fd);
```

Duplicates a socket handle. POSIX uses `dup` and sets `FD_CLOEXEC`; Windows uses `WSADuplicateSocket` and `WSASocket`.

#### `fio_sock_bind`

```c
FIO_IFUNC int fio_sock_bind(fio_socket_i fd,
                            const struct sockaddr *addr,
                            socklen_t addrlen);
```

Portable `bind` wrapper.

#### `fio_sock_connect`

```c
FIO_IFUNC int fio_sock_connect(fio_socket_i fd,
                               const struct sockaddr *addr,
                               socklen_t addrlen);
```

Portable `connect` wrapper.

#### `fio_sock_listen`

```c
FIO_IFUNC int fio_sock_listen(fio_socket_i fd, int backlog);
```

Portable `listen` wrapper.

#### `fio_sock_setsockopt`

```c
FIO_IFUNC int fio_sock_setsockopt(fio_socket_i fd,
                                  int level,
                                  int optname,
                                  const void *optval,
                                  socklen_t optlen);
```

Portable `setsockopt` wrapper. Pointer and length type differences are handled internally.

#### `fio_sock_socketpair`

```c
FIO_IFUNC int fio_sock_socketpair(fio_socket_i fds[2]);
```

Creates a connected socket pair. POSIX uses `socketpair(AF_UNIX, SOCK_STREAM, 0, fds)`. Windows creates a loopback TCP pair.

#### `fio_sock_pipe`

```c
FIO_IFUNC int fio_sock_pipe(fio_socket_i fds[2]);
```

Creates a pipe-like pair. POSIX uses `pipe`; Windows delegates to `fio_sock_socketpair` so the handles are usable with WinSock polling.

### Socket State and Waiting

#### `fio_sock_set_non_block`

```c
SFUNC int fio_sock_set_non_block(fio_socket_i fd);
```

Sets a socket to non-blocking mode.

**Returns:** `0` on success, `-1` on error.

#### `fio_sock_maximize_limits`

```c
SFUNC size_t fio_sock_maximize_limits(size_t maximum_limit);
```

Attempts to raise the open-file limit up to `maximum_limit`. Passing `0` uses `FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT`.

On Windows this returns the advisory cap because there is no `RLIMIT_NOFILE` equivalent for sockets.

#### `fio_sock_wait_io`

```c
SFUNC short fio_sock_wait_io(fio_socket_i fd, short events, int timeout);
```

Waits for `events` on one socket using `poll` / `WSAPoll`. `timeout` is in milliseconds; `0` returns immediately.

**Returns:** `0` on timeout, `-1` on error, or event bits such as `POLLIN`, `POLLOUT`, `POLLHUP`, and `POLLNVAL`.

#### `FIO_SOCK_WAIT_RW`

```c
#define FIO_SOCK_WAIT_RW(fd, timeout_) \
  fio_sock_wait_io(fd, POLLIN | POLLOUT, timeout_)
```

Waits for read or write readiness.

#### `FIO_SOCK_WAIT_R`

```c
#define FIO_SOCK_WAIT_R(fd, timeout_) fio_sock_wait_io(fd, POLLIN, timeout_)
```

Waits for read readiness.

#### `FIO_SOCK_WAIT_W`

```c
#define FIO_SOCK_WAIT_W(fd, timeout_) fio_sock_wait_io(fd, POLLOUT, timeout_)
```

Waits for write readiness.

#### `FIO_SOCK_IS_OPEN`

```c
#ifdef POLLRDHUP
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (!(fio_sock_wait_io(fd, (POLLOUT | POLLRDHUP), 0) &                          \
     (POLLRDHUP | POLLHUP | POLLNVAL)))
#else
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (!(fio_sock_wait_io(fd, POLLOUT, 0) & (POLLHUP | POLLNVAL)))
#endif
```

Best-effort test for whether a socket still appears open.

#### `fio_sock_peer_addr`

```c
SFUNC fio_buf_info_s fio_sock_peer_addr(fio_socket_i s);
```

Returns a numeric, human-readable peer address.

On error, returns `{NULL, 0}`. The returned buffer is internal static storage, limited to 63 bytes. Thread-safety is limited to 128 threads / calls.

### Windows Notes

On Windows, the module initializes WinSock in a constructor and stores direct function pointers loaded from `Ws2_32.dll`. Wrapper functions translate common WinSock errors into `errno` values.

The `fio___sock_wsa*` helpers are internal implementation details, not regular application API. Tiny dragons; leave them napping.

### Ownership and Thread-Safety

Socket handles are owned by the caller and should be closed with `fio_sock_close`. `fio_sock_address_new` results are owned by the caller and should be freed with `fio_sock_address_free`.

Socket operations are as thread-safe as the operating system calls they wrap. `fio_sock_peer_addr` uses shared static buffers and has the 128-call/thread caveat described above. Unix socket creation may touch process-global `umask` unless `FIO_SOCK_AVOID_UMASK` is defined.

### Example

```c
#define FIO_SOCK
#include "fio-stl.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  fio_socket_i fds[2];
  char buf[8] = {0};

  if (fio_sock_pipe(fds))
    return 1;

  fio_sock_write(fds[1], "ping", 4);
  fio_sock_read(fds[0], buf, sizeof(buf) - 1);
  printf("%s\n", buf);

  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  return 0;
}
```

### TCP Server Sketch

```c
#define FIO_SOCK
#include "fio-stl.h"

int main(void) {
  fio_socket_i s = fio_sock_open(NULL, "8080",
                                 FIO_SOCK_SERVER | FIO_SOCK_TCP |
                                     FIO_SOCK_NONBLOCK);
  if (!FIO_SOCK_FD_ISVALID(s))
    return 1;

  if (FIO_SOCK_WAIT_R(s, 1000) & POLLIN) {
    fio_socket_i c = fio_sock_accept(s, NULL, NULL);
    if (FIO_SOCK_FD_ISVALID(c))
      fio_sock_close(c);
  }

  fio_sock_close(s);
  return 0;
}
```

---
