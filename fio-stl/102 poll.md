## Basic IO Polling

```c
#define FIO_POLL
#include "fio-stl.h"
```

IO polling using `kqueue`, `epoll` or the portable `poll` POSIX function is another area that's of common need and where many solutions are required.

The facil.io standard library provides a persistent polling container for evented management of (small) IO (file descriptor) collections using the "one-shot" model.

"One-Shot" means that once a specific event has "fired" (occurred), it will no longer be monitored (unless re-submitted). If the same file descriptor is waiting on multiple events, only those events that occurred will be removed from the monitored collection.

There's no real limit on the number of file descriptors that can be monitored, except possible system limits that the system may impose on the `kqueue`/`epoll`/`poll` system calls. However, performance will degrade significantly as the ratio between inactive vs. active IO objects being monitored increases when using the `poll` system call.

It is recommended to use the system specific polling "engine" (`epoll` / `kqueue`) if polling thousands of persistent file descriptors.

By defining `FIO_POLL`, the following functions will be defined.

**Note**: the same type and range limitations that apply to the Sockets implementation on Windows apply to the `poll` implementation.

### `FIO_POLL` API


#### `fio_poll_s`

```c
typedef struct fio_poll_s fio_poll_s;
```

The `fio_poll_s` type should be considered opaque and should **not** be accessed directly.

#### `fio_poll_init`

```c
void fio_poll_init(fio_poll_s *p, fio_poll_settings_s settings);
/* Named arguments using macro. */
#define fio_poll_init(p, ...) fio_poll_init((p), (fio_poll_settings_s){__VA_ARGS__})

typedef struct {
  /** callback for when data is available in the incoming buffer. */
  void (*on_data)(void *udata);
  /** callback for when the outgoing buffer allows a call to `write`. */
  void (*on_ready)(void *udata);
  /** callback for closed connections and / or connections with errors. */
  void (*on_close)(void *udata);
} fio_poll_settings_s;
```

Initializes a polling object, allocating its resources.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_poll_s poller;
fio_poll_init(&poller,
              .on_data = my_on_data_callback,
              .on_ready = my_on_ready_callback,
              .on_close = my_on_close_callback);
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `on_data` | `void (*)(void *)` | Callback for when data is available in the incoming buffer |
| `on_ready` | `void (*)(void *)` | Callback for when the outgoing buffer allows a call to `write` |
| `on_close` | `void (*)(void *)` | Callback for closed connections and/or connections with errors |

**Note**: callbacks that are not provided will default to a no-op function.

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
int fio_poll_review(fio_poll_s *p, size_t timeout);
```

Reviews if any of the monitored file descriptors has any events.

**Parameters:**
- `p` - pointer to the polling object
- `timeout` - timeout in milliseconds

**Returns:** the number of events called.

**Note**: polling is thread safe, but has different effects on different threads. Adding a new file descriptor from one thread while polling in a different thread will not poll that IO until `fio_poll_review` is called again.

#### `fio_poll_forget`

```c
int fio_poll_forget(fio_poll_s *p, int fd);
```

Stops monitoring the specified file descriptor even if some of its events hadn't occurred yet.

**Parameters:**
- `p` - pointer to the polling object
- `fd` - the file descriptor to stop monitoring

**Returns:** `0` on success, `-1` on error (e.g., if the file descriptor was not being monitored).

#### `fio_poll_close_all`

```c
void fio_poll_close_all(fio_poll_s *p);
```

Closes all monitored sockets, calling the `on_close` callback for each.

**Parameters:**
- `p` - pointer to the polling object

**Note**: this function is only available when using the `poll` engine (`FIO_POLL_ENGINE_POLL`).

#### `fio_poll_engine`

```c
const char *fio_poll_engine(void);
```

Returns the system call used for polling as a constant string.

**Returns:** `"poll"`, `"epoll"`, or `"kqueue"` depending on the selected engine (always `"poll"` on Windows).

### `FIO_POLL` Compile Time Macros

#### Engine Selection Macros

Define one of the following before including the library to select the polling engine. If none is defined, the best available engine for the current platform is selected automatically.

```c
#define FIO_POLL_ENGINE_POLL    /* POSIX poll() / WSAPoll - any platform */
#define FIO_POLL_ENGINE_EPOLL   /* Linux epoll */
#define FIO_POLL_ENGINE_KQUEUE  /* BSD/macOS kqueue */
```

When multiplexing a small number of IO sockets, using the `poll` engine might be faster, as it uses fewer system calls.

Auto-detection order (when no engine is explicitly selected): `epoll` on Linux, `kqueue` on BSD/macOS, `poll` as universal fallback (including Windows).

#### `FIO_POLL_ENGINE_STR`

```c
#define FIO_POLL_ENGINE_STR "poll"    /* set by FIO_POLL_ENGINE_POLL */
#define FIO_POLL_ENGINE_STR "epoll"   /* set by FIO_POLL_ENGINE_EPOLL */
#define FIO_POLL_ENGINE_STR "kqueue"  /* set by FIO_POLL_ENGINE_KQUEUE */
```

A string macro representing the selected IO multiplexing engine.

#### `FIO_POLL_POSSIBLE_FLAGS`

```c
#define FIO_POLL_POSSIBLE_FLAGS (POLLIN | POLLOUT | POLLPRI)
```

Defines the user flags that IO events recognize. This can be overridden before including the header.

#### `FIO_POLL_MAX_EVENTS`

```c
#define FIO_POLL_MAX_EVENTS 128 /* or 256 on 32-bit systems */
```

Defines the maximum number of events per review call. This is relevant only for `epoll` and `kqueue` engines.

The default value is `128` on 64-bit systems and `256` on 32-bit systems.

-------------------------------------------------------------------------------
