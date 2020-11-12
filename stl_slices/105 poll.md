## Basic IO Polling

IO polling using the portable `poll` POSIX function is another area that's of common need and where many solutions are required.

The facil.io standard library provides a persistent polling container for evented management of small IO (file descriptor) collections using the "one-shot" model.

"One-Shot" means that once a specific even has "fired" (occurred), it will no longer be monitored (unless re-submitted). If the same file desciptor is waiting on multiple event, only those events that occurred will be removed from the monitored collection.

There's no real limit on the number of file descriptors that can be monitored, except possible system limits that the system may impose on the `poll` system call. However, performance will degrade significantly as the ratio between inactive vs. active IO objects being monitored increases.

It is recommended to use a system specific polling "engine" (`epoll` / `kqueue`) if polling thousands of persistent file descriptors.

By defining `FIO_POLL` on a POSIX system, the following functions will be defined.

### `FIO_POLL` API


#### `fio_poll_s`

```c
typedef struct fio_poll_s fio_poll_s;
```

The `fio_poll_s` type should be considered opaque and should **not** be accessed directly.

#### `FIO_POLL_INIT`

```c
#define FIO_POLL_INIT(on_data_func, on_ready_func, on_close_func)              \
  {                                                                            \
    .settings =                                                                \
        {                                                                      \
            .on_data = on_data_func,                                           \
            .on_ready = on_ready_func,                                         \
            .on_close = on_close_func,                                         \
        },                                                                     \
    .lock = FIO_LOCK_INIT                                                      \
  }
```

A `fio_poll_s` object initialization macro.

#### `fio_poll_new`

```c
fio_poll_s *fio_poll_new(fio_poll_settings_s settings);
/* named argument support */
#define fio_poll_new(...) fio_poll_new((fio_poll_settings_s){__VA_ARGS__})
```

Creates a new polling object / queue.

The settings arguments set the `on_data`, `on_ready` and `on_close` callbacks:

```c
typedef struct {
  /** callback for when data is availabl in the incoming buffer. */
  void (*on_data)(int fd, void *udata);
  /** callback for when the outgoing buffer allows a call to `write`. */
  void (*on_ready)(int fd, void *udata);
  /** callback for closed connections and / or connections with errors. */
  void (*on_close)(int fd, void *udata);
} fio_poll_settings_s;
```

Returns NULL on error (no memory).

#### `fio_poll_free`

```c
int fio_poll_free(fio_poll_s *p);
```

Frees the polling object and its resources.

#### `fio_poll_destroy`

```c
void fio_poll_destroy(fio_poll_s *p);
```

Destroys the polling object, freeing its resources.

**Note**: the monitored file descriptors will remain untouched (possibly open). To close all the monitored file descriptors, call `fio_poll_close_and_destroy` instead.

#### `fio_poll_close_and_destroy`

```c
void fio_poll_close_and_destroy(fio_poll_s *p);
```

Closes all monitored connections, calling the `on_close` callbacks for all of them.

#### `fio_poll_monitor`

```c
int fio_poll_monitor(fio_poll_s *p, int fd, void *udata, unsigned short flags);
```

Adds a file descriptor to be monitored, adds events to be monitored or updates the monitored file's `udata`.

Possible flags are: `POLLIN` and `POLLOUT`. Other flags may be set but might be ignored.

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

#### `FIO_POLL_HAS_UDATA_COLLECTION`

```c
#ifndef FIO_POLL_HAS_UDATA_COLLECTION
#define FIO_POLL_HAS_UDATA_COLLECTION 1
#endif
```

When set to true (the default value), the `udata` value is unique per file descriptor, using an array of `udata` values.

When false, a global `udata` is used and it is updated whenever a `udata` value is supplied (`NULL` values are ignored).

#### `FIO_POLL_DEBUG`

If defined before the first time `FIO_POLL` is included, this will add debug messages to the polling logic.

-------------------------------------------------------------------------------
