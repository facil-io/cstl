## Basic Socket / IO Helpers

```c
#define FIO_SOCK
#include "fio-stl.h"
```

The facil.io standard library provides a few simple IO / Sockets helpers for POSIX systems and Windows.

By defining `FIO_SOCK`, the following functions will be defined.

**Note**: On Windows, `fd` is a 64-bit number with no promises made as to its value. On POSIX systems the `fd` is a 32-bit number which is sequential.

Since facil.io prefers the POSIX approach, it will validate the `fd` value for overflow and might fail to open / accept sockets when their value overflows the 32-bit type limit set on POSIX machines.

However, for most implementations this should be a non-issue as it seems (from observation, not knowledge) that Windows maps `fd` values to a kernel array (rather than a process specific array) and it is unlikely that any Windows machine will actually open more than 2 Giga "handles" unless it's doing something wrong.

**Note**: this module depends on the `FIO_URL` module which will be automatically included.

### Configuration Macros

#### `FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT`

```c
#ifndef FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT
#define FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT (1ULL << 24)
#endif
```

The default maximum limit used by `fio_sock_maximize_limits` when called with `0` as the argument.

#### `FIO_SOCK_AVOID_UMASK`

This compilation flag, if defined before including the `FIO_SOCK` implementation, will avoid using `umask` (only using `chmod`).

Using `umask` in multi-threaded environments could cause `umask` data corruption due to race condition (as two calls are actually required, making the operation non-atomic).

If more than one thread is expected to create Unix sockets or call `umask` at the same time, it is recommended that the `FIO_SOCK_AVOID_UMASK` be used.

This, however, may affect permissions on some systems (i.e., some Linux distributions) where calling `chmod` on a Unix socket file doesn't properly update access permissions.

**Note**: on Windows facil.io behaves as if this flag was set.

### Types

#### `fio_sock_open_flags_e`

```c
typedef enum {
  FIO_SOCK_SERVER = 0,        /* Server socket (binds to local address) */
  FIO_SOCK_CLIENT = 1,        /* Client socket (connects to remote address) */
  FIO_SOCK_NONBLOCK = 2,      /* Set socket to non-blocking mode */
  FIO_SOCK_TCP = 4,           /* TCP/IP socket */
  FIO_SOCK_UDP = 8,           /* UDP socket */
  FIO_SOCK_UNIX = 16,         /* Unix domain socket (POSIX only) */
  FIO_SOCK_UNIX_PRIVATE = 48, /* Unix socket with restricted permissions */
} fio_sock_open_flags_e;
```

Socket type flags used with `fio_sock_open` and `fio_sock_open2`.

**Values:**

- `FIO_SOCK_SERVER` - Initializes a Server socket. For TCP/IP and Unix sockets, the new socket will be listening for incoming connections (`listen` will be automatically called).
- `FIO_SOCK_CLIENT` - Initializes a Client socket, calling `connect` using the `address` and `port` arguments.
- `FIO_SOCK_NONBLOCK` - Sets the new socket to non-blocking mode.
- `FIO_SOCK_TCP` - Creates a TCP/IP socket.
- `FIO_SOCK_UDP` - Creates a UDP socket.
- `FIO_SOCK_UNIX` - Creates a Unix socket (requires a POSIX system). If an existing file / Unix socket exists, they will be deleted and replaced.
- `FIO_SOCK_UNIX_PRIVATE` - Same as `FIO_SOCK_UNIX`, only does not use `umask` and `chmod` to make the socket publicly available.

**Note**: `FIO_SOCK_UNIX` and `FIO_SOCK_UNIX_PRIVATE` are only available on systems that support Unix domain sockets (defined as `0` otherwise).

**Note**: if neither `FIO_SOCK_SERVER` nor `FIO_SOCK_CLIENT` are specified, the function will default to a server socket.

### API Functions

#### `fio_sock_open`

```c
int fio_sock_open(const char *restrict address,
                  const char *restrict port,
                  uint16_t flags);
```

Creates a new socket according to the provided flags.

The `port` string will be ignored when `FIO_SOCK_UNIX` is set.

The `address` can be NULL for Server sockets (`FIO_SOCK_SERVER`) when binding to all available interfaces (this is actually recommended unless network filtering is desired).

The `flags` integer can be a combination of any of the `fio_sock_open_flags_e` flags.

**Note**: UDP Server Sockets might need to handle traffic from multiple clients, which could require a significantly larger OS buffer than the default buffer offered.

Consider (from [this SO answer](https://stackoverflow.com/questions/2090850/specifying-udp-receive-buffer-size-at-runtime-in-linux/2090902#2090902), see [this blog post](https://medium.com/@CameronSparr/increase-os-udp-buffers-to-improve-performance-51d167bb1360), [this article](http://fasterdata.es.net/network-tuning/udp-tuning/) and [this article](https://access.redhat.com/documentation/en-US/JBoss_Enterprise_Web_Platform/5/html/Administration_And_Configuration_Guide/jgroups-perf-udpbuffer.html)):

```c
int n = 32*1024*1024; /* try for 32Mb */
while (n >= (4*1024*1024) && setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1) {
  /* failed - repeat attempt at 1Mb interval */
  if (n >= (4 * 1024 * 1024)) // OS may have returned max value
    n -= 1024 * 1024;
}
```

#### `fio_sock_open2`

```c
int fio_sock_open2(const char *url, uint16_t flags);
```

See [`fio_sock_open`](#fio_sock_open) for details. Accepts a single, URL style string instead of an address / port pair.

The `tcp` / `udp` information **may** appear in the URL schema if missing from the flags (i.e., `tcp://localhost:3000/`).

If a Unix socket URL is detected on a POSIX system, a `FIO_SOCK_UNIX` socket flag will override any `FIO_SOCK_TCP` or `FIO_SOCK_UDP` that were originally given.

**Note**: a `file://` or `unix://` (or even a simple `./file.sock`) URL will create a publicly available Unix Socket (permissions set to allow everyone RW access). To create a private Unix Socket (one with permissions equal to the process's `umask`), use a `priv://` schema (i.e., `priv://my.sock`).

#### `fio_sock_address_new`

```c
struct addrinfo *fio_sock_address_new(const char *restrict address,
                                      const char *restrict port,
                                      int sock_type);
```

Attempts to resolve an address to a valid IP6 / IP4 address pointer.

The `sock_type` element should be a socket type, such as `SOCK_DGRAM` (UDP) or `SOCK_STREAM` (TCP/IP).

The address should be freed using `fio_sock_address_free`.

**Note**: common web service names are automatically resolved to port numbers:
- `ws` → `80`
- `wss`, `sse`, `sses` → `443`
- `http` → `80`
- `https` → `443`

#### `fio_sock_address_free`

```c
void fio_sock_address_free(struct addrinfo *a);
```

Frees the pointer returned by `fio_sock_address_new`.

#### `fio_sock_peer_addr`

```c
fio_buf_info_s fio_sock_peer_addr(int s);
```

Returns a human readable address representation of the socket's peer address.

On error, returns a NULL buffer with zero length.

Buffer lengths are limited to 63 bytes.

**Note**: this function is limited in its thread safety to 128 threads / calls.

#### `fio_sock_open_local`

```c
int fio_sock_open_local(struct addrinfo *addr, int nonblock);
```

Creates a new network socket and binds it to a local address.

- `addr` - address information from `fio_sock_address_new`
- `nonblock` - if non-zero, sets the socket to non-blocking mode

Returns the file descriptor on success, or -1 on error.

#### `fio_sock_open_remote`

```c
int fio_sock_open_remote(struct addrinfo *addr, int nonblock);
```

Creates a new network socket and connects it to a remote address.

- `addr` - address information from `fio_sock_address_new`
- `nonblock` - if non-zero, sets the socket to non-blocking mode

Returns the file descriptor on success, or -1 on error.

#### `fio_sock_open_unix`

```c
int fio_sock_open_unix(const char *address, uint16_t flags);
```

Creates a new Unix socket and binds it to a local address.

- `address` - the Unix socket path
- `flags` - socket flags (see `fio_sock_open_flags_e`)

Returns the file descriptor on success, or -1 on error.

**Note**: not available on all systems. On Windows, when Unix Sockets are available (which isn't always), the permissions for the socket are system defined (facil.io doesn't change them).

#### `fio_sock_set_non_block`

```c
int fio_sock_set_non_block(int fd);
```

Sets a file descriptor / socket to non-blocking state.

Returns 0 on success, -1 on error.

#### `fio_sock_maximize_limits`

```c
size_t fio_sock_maximize_limits(size_t max_limit);
```

Attempts to maximize the allowed open file limits (with values up to `max_limit`).

Returns the new known limit.

If `max_limit` is 0, uses `FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT`.

#### `fio_sock_wait_io`

```c
short fio_sock_wait_io(int fd, short events, int timeout);
```

Uses `poll` to wait until an IO device has one or more of the events listed in `events` (`POLLIN | POLLOUT`) or `timeout` (in milliseconds) has passed.

A zero timeout returns immediately.

Returns 0 on timeout, -1 on error, or the events that are valid.

Possible valid return values also include `POLLIN | POLLOUT | POLLHUP | POLLNVAL`.

### Portability Macros

These macros provide cross-platform compatibility between POSIX and Windows (WinSock2).

#### `fio_sock_write`

```c
/* POSIX */
#define fio_sock_write(fd, data, len) write((fd), (data), (len))
/* Windows */
#define fio_sock_write(fd, data, len) send((fd), (data), (len), 0)
```

Acts as POSIX `write`. Use this macro for portability with WinSock2.

#### `fio_sock_read`

```c
/* POSIX */
#define fio_sock_read(fd, buf, len) read((fd), (buf), (len))
/* Windows */
#define fio_sock_read(fd, buf, len) recv((fd), (buf), (len), 0)
```

Acts as POSIX `read`. Use this macro for portability with WinSock2.

#### `fio_sock_close`

```c
/* POSIX */
#define fio_sock_close(fd) close(fd)
/* Windows */
#define fio_sock_close(fd) closesocket(fd)
```

Acts as POSIX `close`. Use this macro for portability with WinSock2.

#### `fio_sock_dup`

```c
/* POSIX */
#define fio_sock_dup(fd) dup(fd)
/* Windows - function wrapper */
int fio_sock_dup(int original);
```

Acts as POSIX `dup`. Use this for portability with WinSock2.

On Windows, this is a function that uses `WSADuplicateSocket` internally.

#### `fio_sock_accept`

```c
/* POSIX */
#define fio_sock_accept(fd, addr, addrlen) accept(fd, addr, addrlen)
/* Windows - function wrapper */
int fio_sock_accept(int s, struct sockaddr *addr, int *addrlen);
```

Acts as POSIX `accept`. Use this macro for portability with WinSock2.

On Windows, this is a function that validates the returned socket value doesn't overflow the portable int range.

**Note**: on Windows, `accept` is redefined to `fio_sock_accept`.

#### `FIO_SOCK_FD_ISVALID`

```c
/* POSIX */
#define FIO_SOCK_FD_ISVALID(fd) ((int)(fd) != (int)-1)
/* Windows */
#define FIO_SOCK_FD_ISVALID(fd) ((size_t)(fd) <= (size_t)0x7FFFFFFF)
```

Tests if a file descriptor value is valid.

On Windows, this also checks that the socket value fits within the portable 32-bit range.

### Helper Macros

#### `FIO_SOCK_WAIT_RW`

```c
#define FIO_SOCK_WAIT_RW(fd, timeout_) fio_sock_wait_io(fd, POLLIN | POLLOUT, timeout_)
```

A helper macro that waits on a single IO for both read and write readiness (0 = no event / timeout).

#### `FIO_SOCK_WAIT_R`

```c
#define FIO_SOCK_WAIT_R(fd, timeout_) fio_sock_wait_io(fd, POLLIN, timeout_)
```

A helper macro that waits on a single IO for read readiness (0 = no event / timeout).

#### `FIO_SOCK_WAIT_W`

```c
#define FIO_SOCK_WAIT_W(fd, timeout_) fio_sock_wait_io(fd, POLLOUT, timeout_)
```

A helper macro that waits on a single IO for write readiness (0 = no event / timeout).

#### `FIO_SOCK_IS_OPEN`

```c
/* When POLLRDHUP is available */
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (!(fio_sock_wait_io(fd, (POLLOUT | POLLRDHUP), 0) &                          \
     (POLLRDHUP | POLLHUP | POLLNVAL)))
/* Otherwise */
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (!(fio_sock_wait_io(fd, POLLOUT, 0) & (POLLHUP | POLLNVAL)))
```

A helper macro that tests if a socket is still open (not remotely closed).

Returns non-zero if the socket is open, zero if closed or invalid.

### Example

```c
#define FIO_SOCK
#define FIO_LOG
#include "fio-stl.h"

int main(void) {
  /* Maximize file descriptor limits */
  size_t max_fds = fio_sock_maximize_limits(0);
  FIO_LOG_INFO("Maximum file descriptors: %zu", max_fds);

  /* Create a TCP server socket */
  int server_fd = fio_sock_open(NULL, "8080",
                                FIO_SOCK_TCP | FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);
  if (server_fd == -1) {
    FIO_LOG_ERROR("Failed to create server socket");
    return 1;
  }
  FIO_LOG_INFO("Server listening on port 8080 (fd=%d)", server_fd);

  /* Alternative: create socket using URL */
  int server_fd2 = fio_sock_open2("tcp://0.0.0.0:8081",
                                  FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);
  if (server_fd2 != -1) {
    FIO_LOG_INFO("Server also listening on port 8081 (fd=%d)", server_fd2);
  }

  /* Wait for incoming connection (with 5 second timeout) */
  short events = FIO_SOCK_WAIT_R(server_fd, 5000);
  if (events & POLLIN) {
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = fio_sock_accept(server_fd,
                                    (struct sockaddr *)&client_addr,
                                    (int *)&addr_len);
    if (client_fd != -1) {
      fio_buf_info_s peer = fio_sock_peer_addr(client_fd);
      FIO_LOG_INFO("Accepted connection from %.*s", (int)peer.len, peer.buf);
      
      /* Set client to non-blocking */
      fio_sock_set_non_block(client_fd);
      
      /* Send a response */
      const char *response = "Hello, World!\n";
      fio_sock_write(client_fd, response, strlen(response));
      
      /* Close client connection */
      fio_sock_close(client_fd);
    }
  } else {
    FIO_LOG_INFO("No connection received (timeout)");
  }

  /* Check if socket is still open */
  if (FIO_SOCK_IS_OPEN(server_fd)) {
    FIO_LOG_INFO("Server socket is still open");
  }

  /* Cleanup */
  fio_sock_close(server_fd);
  if (server_fd2 != -1)
    fio_sock_close(server_fd2);

  return 0;
}
```

### Unix Socket Example

```c
#define FIO_SOCK
#define FIO_LOG
#include "fio-stl.h"

int main(void) {
  /* Create a Unix domain socket server */
  int unix_fd = fio_sock_open("/tmp/my_app.sock", NULL,
                              FIO_SOCK_UNIX | FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);
  if (unix_fd == -1) {
    FIO_LOG_ERROR("Failed to create Unix socket");
    return 1;
  }
  FIO_LOG_INFO("Unix socket server created at /tmp/my_app.sock");

  /* Alternative: using URL syntax */
  int unix_fd2 = fio_sock_open2("unix:///tmp/my_app2.sock",
                                FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);
  
  /* For a private socket (restricted permissions) */
  int priv_fd = fio_sock_open2("priv:///tmp/my_private.sock",
                               FIO_SOCK_SERVER | FIO_SOCK_NONBLOCK);

  /* Cleanup */
  fio_sock_close(unix_fd);
  if (unix_fd2 != -1) fio_sock_close(unix_fd2);
  if (priv_fd != -1) fio_sock_close(priv_fd);
  
  /* Remove socket files */
  unlink("/tmp/my_app.sock");
  unlink("/tmp/my_app2.sock");
  unlink("/tmp/my_private.sock");

  return 0;
}
```

### Client Connection Example

```c
#define FIO_SOCK
#define FIO_LOG
#include "fio-stl.h"

int main(void) {
  /* Connect to a remote server */
  int client_fd = fio_sock_open("example.com", "80",
                                FIO_SOCK_TCP | FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK);
  if (client_fd == -1) {
    FIO_LOG_ERROR("Failed to connect");
    return 1;
  }

  /* Wait for connection to complete */
  if (FIO_SOCK_WAIT_W(client_fd, 5000) & POLLOUT) {
    FIO_LOG_INFO("Connected to example.com:80");
    
    /* Send HTTP request */
    const char *request = "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n";
    fio_sock_write(client_fd, request, strlen(request));
    
    /* Wait for response */
    if (FIO_SOCK_WAIT_R(client_fd, 5000) & POLLIN) {
      char buffer[4096];
      ssize_t bytes = fio_sock_read(client_fd, buffer, sizeof(buffer) - 1);
      if (bytes > 0) {
        buffer[bytes] = '\0';
        FIO_LOG_INFO("Received %zd bytes:\n%s", bytes, buffer);
      }
    }
  }

  fio_sock_close(client_fd);
  return 0;
}
```

-------------------------------------------------------------------------------
