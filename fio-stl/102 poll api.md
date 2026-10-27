# POSIX Portable Polling API

```c
#define FIO_POLL
#include "fio-stl.h"
```

A small abstraction over `poll`, `epoll`, and `kqueue`. Define `FIO_POLL_ENGINE_POLL`, `FIO_POLL_ENGINE_EPOLL`, or `FIO_POLL_ENGINE_KQUEUE` before inclusion to force a backend; otherwise the best available engine is selected automatically.

The API is the same for every backend. Backend-specific details live in:

- [`102 poll poll.md`](./102%20poll%20poll.md)
- [`102 poll epoll.md`](./102%20poll%20epoll.md)
- [`102 poll kqueue.md`](./102%20poll%20kqueue.md)

---

## Configuration Macros

#### `FIO_POLL_POSSIBLE_FLAGS`

```c
#define FIO_POLL_POSSIBLE_FLAGS (POLLIN | POLLOUT | POLLPRI)
```

Mask of event flags the polling layer recognizes.

#### `FIO_POLL_MAX_EVENTS`

```c
#define FIO_POLL_MAX_EVENTS 128
```

Maximum events returned in one `fio_poll_review` call. Relevant for epoll and kqueue. Larger on 32-bit platforms.

---

## Types

#### `fio_poll_s`

```c
typedef struct fio_poll_s fio_poll_s;
```

Opaque polling object. The actual layout depends on the selected backend.

#### `fio_poll_settings_s`

```c
typedef struct {
  void (*on_data)(void *udata);
  void (*on_ready)(void *udata);
  void (*on_close)(void *udata);
} fio_poll_settings_s;
```

Callback settings. Each callback receives the `udata` pointer supplied when the file descriptor was monitored.

- `on_data` — data is available to read.
- `on_ready` — the socket is ready to write.
- `on_close` — connection closed or an error occurred.

---

## Functions

#### `fio_poll_engine`

```c
FIO_IFUNC const char *fio_poll_engine(void);
```

Returns the name of the active backend (`"epoll"`, `"kqueue"`, or `"poll"`) as a constant string.

#### `fio_poll_init`

```c
FIO_IFUNC int fio_poll_init(fio_poll_s *p, fio_poll_settings_s settings);

#define fio_poll_init(p, ...) \
  fio_poll_init((p), (fio_poll_settings_s){__VA_ARGS__})
```

Initializes the polling object. The macro form accepts designated initializers. Returns `0` on success and `-1` with `errno` set on failure; a failed poller may be destroyed. By default, backend initialization syscall failures terminate the process. Define `FIO_POLL_EXIT_ON_SYSERR 0` before including the library to return `-1` instead. Invalid arguments return `-1` rather than terminating the process.

**Parameters:**
- `p` — pointer to the polling object.
- `settings` — callback configuration.

```c
fio_poll_init(&poll,
              .on_data = on_data,
              .on_ready = on_ready,
              .on_close = on_close);
```

#### `fio_poll_destroy`

```c
FIO_IFUNC void fio_poll_destroy(fio_poll_s *p);
```

Frees backend resources held by the polling object.

#### `fio_poll_monitor`

```c
SFUNC int fio_poll_monitor(fio_poll_s *p,
                           fio_socket_i fd,
                           void *udata,
                           unsigned short flags);
```

Adds `fd` to the poll set, updates its `udata`, or adds events. `POLLIN` and `POLLOUT` are supported by all backends. `POLLPRI` is supported by POSIX `poll` but may be ignored by other backends; do not rely on it for portable monitoring. Other flags may be ignored. Monitoring is one-shot: once an event fires, it is removed until re-armed.

**Returns:** `0` on success, `-1` on error.

#### `fio_poll_review`

```c
SFUNC int fio_poll_review(fio_poll_s *p, size_t timeout);
```

Waits up to `timeout` milliseconds for events and dispatches callbacks. Adding a file descriptor from another thread while `fio_poll_review` is running will not include that descriptor until the next call.

**Returns:** the number of events dispatched, or `-1` on error.

#### `fio_poll_forget`

```c
SFUNC int fio_poll_forget(fio_poll_s *p, fio_socket_i fd);
```

Removes `fd` from the poll set. This is not a cancellation guarantee: an event already returned by the system call may still invoke a callback. Keep `udata` alive until any concurrent review and dispatch have finished; polling thread safety protects the poller's internal state, not `udata`.

**Returns:** `0` on success, `-1` on error.

---

## Example

```c
#define FIO_POLL
#include "fio-stl.h"

static void on_data(void *udata) { fprintf(stderr, "readable\n"); }
static void on_ready(void *udata) { fprintf(stderr, "writable\n"); }
static void on_close(void *udata) { fprintf(stderr, "closed\n"); }

int main(void) {
  fio_poll_s p;
  if (fio_poll_init(&p,
                    .on_data = on_data,
                    .on_ready = on_ready,
                    .on_close = on_close))
    return 1;

  fio_poll_monitor(&p, some_fd, NULL, POLLIN);
  for (int i = 0; i < 10; ++i)
    fio_poll_review(&p, 1000);

  fio_poll_destroy(&p);
  return 0;
}
```

------------------------------------------------------------
