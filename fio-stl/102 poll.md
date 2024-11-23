## Basic IO Polling

```c
#define FIO_POLL
#include "fio-stl.h"
```

IO polling using `kqueue`, `epoll` or the portable `poll` POSIX function is another area that's of common need and where many solutions are required.

The facil.io standard library provides a persistent polling container for evented management of (small) IO (file descriptor) collections using the "one-shot" model.

"One-Shot" means that once a specific even has "fired" (occurred), it will no longer be monitored (unless re-submitted). If the same file desciptor is waiting on multiple event, only those events that occurred will be removed from the monitored collection.

There's no real limit on the number of file descriptors that can be monitored, except possible system limits that the system may impose on the `kqueue`/`epoll`/`poll` system calls. However, performance will degrade significantly as the ratio between inactive vs. active IO objects being monitored increases when using the `poll` system call.

It is recommended to use the system specific polling "engine" (`epoll` / `kqueue`) if polling thousands of persistent file descriptors.

By defining `FIO_POLL`, the following functions will be defined.

**Note**: the same type and range limitations that apply to the Sockets implementation on Windows apply to the `poll` implementation.

**Note**: when using `epoll` then the file descriptor (`fd`) will **NOT** be passed on to the callback (as `epoll` doesn't retain the data).

### `FIO_POLL` API


#### `fio_poll_s`

```c
typedef struct fio_poll_s fio_poll_s;
```

The `fio_poll_s` type should be considered opaque and should **not** be accessed directly.

#### `fio_poll_init`

```c
fio_poll_s *fio_poll_new(fio_poll_settings_s settings);
/* named argument support */
#define fio_poll_new(...) fio_poll_new((fio_poll_settings_s){__VA_ARGS__})
```

Creates a new polling object / queue.

The settings arguments set the `on_data`, `on_ready` and `on_close` callbacks:

```c
typedef struct {
  /** callback for when data is available in the incoming buffer. */
  void (*on_data)(void *udata);
  /** callback for when the outgoing buffer allows a call to `write`. */
  void (*on_ready)(void *udata);
  /** callback for closed connections and / or connections with errors. */
  void (*on_close)(void *udata);
} fio_poll_settings_s;
```

#### `fio_poll_destroy`

```c
void fio_poll_destroy(fio_poll_s *p);
```

Destroys the polling object, freeing its resources.

**Note**: the monitored file descriptors will remain untouched (possibly open).

#### `fio_poll_monitor`

```c
int fio_poll_monitor(fio_poll_s *p, int fd, void *udata, unsigned short flags);
```

Adds a file descriptor to be monitored, adds events to be monitored or updates the monitored file's `udata`.

Possible flags are: `POLLIN` and `POLLOUT`. Other flags may be set but might be ignored.

On systems where `POLLRDHUP` is supported, it is always monitored for.

Monitoring mode is always one-shot. If an event if fired, it is removed from the monitoring state.

Returns -1 on error.

#### `fio_poll_review`

```c
int fio_poll_review(fio_poll_s *p, int timeout);
```

Reviews if any of the monitored file descriptors has any events.

`timeout` is in milliseconds.

Returns the number of events called.

**Note**:

Polling is thread safe, but has different effects on different threads.

Adding a new file descriptor from one thread while polling in a different thread will not poll that IO untill `fio_poll_review` is called again.

#### `fio_poll_forget`

```c
void *fio_poll_forget(fio_poll_s *p, int fd);
```

Stops monitoring the specified file descriptor even if some of it's event's hadn't occurred just yet, returning its `udata` (if any).

### `FIO_POLL` Compile Time Macros

#### `FIO_POLL_ENGINE`

```c
#define FIO_POLL_ENGINE_POLL   1
#define FIO_POLL_ENGINE_EPOLL  2
#define FIO_POLL_ENGINE_KQUEUE 3
```

Allows for both the detection and the manual selection (override) of the underlying IO multiplexing API.

When multiplexing a small number of IO sockets, using the `poll` engine might be faster, as it uses less system calls.

```c
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_POLL
```

#### `FIO_POLL_ENGINE_STR`

```c
#if FIO_POLL_ENGINE == FIO_POLL_ENGINE_POLL
#define FIO_POLL_ENGINE_STR "poll"
#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_EPOLL
#define FIO_POLL_ENGINE_STR "epoll"
#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_KQUEUE
#define FIO_POLL_ENGINE_STR "kqueue"
#endif

```

A string MACRO representing the used IO multiplexing "engine".

-------------------------------------------------------------------------------
