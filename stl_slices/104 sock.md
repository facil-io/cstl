## Basic Socket / IO Helpers

The facil.io standard library provides a few simple IO / Sockets helpers for POSIX systems.

By defining `FIO_SOCK`, the following functions will be defined.

**Note**:

On Windows that `fd` is a 64 bit number with no promises made as to its value. On POSIX systems the `fd` is a 32 bit number which is sequential. 

Since facil.io prefers the POSIX approach, it will validate the `fd` value for overflow and might fail to open / accept sockets when their value overflows the 32bit type limit set on POSIX machines.

However, for most implementations this should be a non-issue as it seems (from observation, not knowledge) that Windows maps `fd` values to a kernel array (rather than a process specific array) and it is unlikely that any Windows machine will actually open more than 2 Giga "handles" unless it's doing something wrong.

#### `fio_sock_open`

```c
int fio_sock_open(const char *restrict address,
                 const char *restrict port,
                 uint16_t flags);
```

Creates a new socket according to the provided flags.

The `port` string will be ignored when `FIO_SOCK_UNIX` is set.

The `address` can be NULL for Server sockets (`FIO_SOCK_SERVER`) when binding to all available interfaces (this is actually recommended unless network filtering is desired).

The `flag` integer can be a combination of any of the following flags:

*  `FIO_SOCK_TCP` - Creates a TCP/IP socket.

*  `FIO_SOCK_UDP` - Creates a UDP socket.

*  `FIO_SOCK_UNIX ` - Creates a Unix socket (requires a POSIX system). If an existing file / Unix socket exists, they will be deleted and replaced.

*  `FIO_SOCK_SERVER` - Initializes a Server socket. For TCP/IP and Unix sockets, the new socket will be listening for incoming connections (`listen` will be automatically called).

*  `FIO_SOCK_CLIENT` - Initializes a Client socket, calling `connect` using the `address` and `port` arguments.

*  `FIO_SOCK_NONBLOCK` - Sets the new socket to non-blocking mode.

If neither `FIO_SOCK_SERVER` nor `FIO_SOCK_CLIENT` are specified, the function will default to a server socket.

**Note**:

UDP Server Sockets might need to handle traffic from multiple clients, which could require a significantly larger OS buffer then the default buffer offered.

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
int fio_sock_open(const char *url, uint16_t flags);
```

See [`fio_sock_open`](#fio_sock_open) for details. Accepts a single, URL style string instead of an address / port pair.

The `tcp` / `udp` information **may** appear in the URL schema if missing from the flags (i.e., `tcp://localhost:3000/`);

If a Unix socket URL is detected on a POSIX system, a `FIO_SOCK_UNIX` socket flag will override any `FIO_SOCK_TCP` or 
`FIO_SOCK_UDP` that were originally given.

#### `fio_sock_write`, `fio_sock_read`, `fio_sock_close`

```c
#define fio_sock_write(fd, data, len) write((fd), (data), (len))
#define fio_sock_read(fd, buf, len)   read((fd), (buf), (len))
#define fio_sock_close(fd)            close(fd)
/* on Windows only */
#define accept fio_sock_accept
```

Behaves the same as the POSIX function calls... however, on Windows these will be function wrappers around the WinSock2 API variants. It is better to use these macros / functions for portability.

#### `fio_sock_wait_io`

```c
short fio_sock_wait_io(int fd, short events, int timeout)
```

Uses `poll` to wait until an IO device has one or more of the evens listed in `events` (`POLLIN | POLLOUT`) or `timeout` (in milliseconds) have passed.

Returns 0 on timeout, -1 on error or the events that are valid.

#### `fio_sock_poll`

```c
typedef struct {
  void (*on_ready)(int fd, void *udata);
  void (*on_data)(int fd, void *udata);
  void (*on_error)(int fd, void *udata);
  void *udata;
  int timeout;
  struct pollfd *fds;
} fio_sock_poll_args;

int fio_sock_poll(fio_sock_poll_args args);
#define fio_sock_poll(...) fio_sock_poll((fio_sock_poll_args){__VA_ARGS__})
```

The `fio_sock_poll` function is shadowed by the `fio_sock_poll` MACRO, which allows the function to accept the following "named arguments":

* `on_ready`:

    This callback will be called if a socket can be written to and the socket is polled for the **W**rite event.

        // callback example:
        void on_ready(int fd, void *udata);

* `on_data`:

    This callback will be called if data is available to be read from a socket and the socket is polled for the **R**ead event.

        // callback example:
        void on_data(int fd, void *udata);

* `on_error`:

    This callback will be called if an error occurred when polling the file descriptor.

        // callback example:
        void on_error(int fd, void *udata);

* `timeout`:

    Polling timeout in milliseconds.

        // type:
        int timeout;

* `udata`:

    Opaque user data.

        // type:
        void *udata;

* `fds`:

    A list of `struct pollfd` with file descriptors to be polled. The list **should** end with a `struct pollfd` containing an empty `events` field (and no empty `events` field should appear in the middle of the list).

    Use the `FIO_SOCK_POLL_LIST(...)`, `FIO_SOCK_POLL_RW(fd)`, `FIO_SOCK_POLL_R(fd)` and `FIO_SOCK_POLL_W(fd)` macros to build the list.

* `count`:

    If supplied, should contain the valid length of the `fds` array.

    If `0` or empty and `fds` isn't `NULL`, the length of the `fds` array will be auto-calculated by seeking through the array for a member in which `events == 0`.

    **Note**: this could cause a buffer overflow, which is why the last member of the `fds` array **should** end with a `struct pollfd` member containing an empty `events` field. Use the `FIO_SOCK_POLL_LIST` macro as a helper.

The `fio_sock_poll` function uses the `poll` system call to poll a simple IO list.

The list must end with a `struct pollfd` with it's `events` set to zero. No other member of the list should have their `events` data set to zero.

It is recommended to use the `FIO_SOCK_POLL_LIST(...)` and
`FIO_SOCK_POLL_[RW](fd)` macros. i.e.:

```c
int io_fd = fio_sock_open(NULL, "8888", FIO_SOCK_UDP | FIO_SOCK_NONBLOCK | FIO_SOCK_SERVER);
int count = fio_sock_poll(.on_ready = on_ready,
                    .on_data = on_data,
                    .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(io_fd)));
```

**Note**: The `poll` system call should perform reasonably well for light loads (short lists). However, for complex IO needs or heavier loads, use the system's native IO API, such as `kqueue` or `epoll`.


#### `FIO_SOCK_POLL_RW` (macro)

```c
#define FIO_SOCK_POLL_RW(fd_)                                                  \
  (struct pollfd) { .fd = fd_, .events = (POLLIN | POLLOUT) }
```

This helper macro helps to author a `struct pollfd` member who's set to polling for both read and write events (data availability and/or space in the outgoing buffer).

#### `FIO_SOCK_POLL_R` (macro)

```c
#define FIO_SOCK_POLL_R(fd_)                                                   \
  (struct pollfd) { .fd = fd_, .events = POLLIN }
```

This helper macro helps to author a `struct pollfd` member who's set to polling for incoming data availability.


#### `FIO_SOCK_POLL_W` (macro)

```c
#define FIO_SOCK_POLL_W(fd_)                                                   \
  (struct pollfd) { .fd = fd_, .events = POLLOUT }
```

This helper macro helps to author a `struct pollfd` member who's set to polling for space in the outgoing `fd`'s buffer.

#### `FIO_SOCK_POLL_LIST` (macro)

```c
#define FIO_SOCK_POLL_LIST(...)                                                \
  (struct pollfd[]) {                                                          \
    __VA_ARGS__, (struct pollfd) { .fd = -1 }                                  \
  }
```

This helper macro helps to author a `struct pollfd` array who's last member has an empty `events` value (for auto-length detection).

#### `fio_sock_address_new`

```c
struct addrinfo *fio_sock_address_new(const char *restrict address,
                                      const char *restrict port,
                                      int sock_type);
```

Attempts to resolve an address to a valid IP6 / IP4 address pointer.

The `sock_type` element should be a socket type, such as `SOCK_DGRAM` (UDP) or `SOCK_STREAM` (TCP/IP).

The address should be freed using `fio_sock_address_free`.

#### `fio_sock_address_free`

```c
void fio_sock_address_free(struct addrinfo *a);
```

Frees the pointer returned by `fio_sock_address_new`.

#### `fio_sock_set_non_block`

```c
int fio_sock_set_non_block(int fd);
```

Sets a file descriptor / socket to non blocking state.

#### `fio_sock_open_local`

```c
int fio_sock_open_local(struct addrinfo *addr);
```

Creates a new network socket and binds it to a local address.

#### `fio_sock_open_remote`

```c
int fio_sock_open_remote(struct addrinfo *addr, int nonblock);
```

Creates a new network socket and connects it to a remote address.

#### `fio_sock_open_unix`

```c
int fio_sock_open_unix(const char *address, int is_client, int nonblock);
```

Creates a new Unix socket and binds it to a local address.

**Note**: available only on POSIX systems.

-------------------------------------------------------------------------------
