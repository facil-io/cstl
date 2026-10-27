# POSIX Polling — epoll Backend

```c
#define FIO_POLL_ENGINE_EPOLL
#define FIO_POLL
#include "fio-stl.h"
```

Implementation of the portable polling API on top of Linux `epoll`. Normally selected automatically on Linux; define `FIO_POLL_ENGINE_EPOLL` to force it.

The public API is documented in [`102 poll api.md`](./102%20poll%20api.md). This file describes backend-specific behavior.

---

## Backend Details

#### `struct fio_poll_s`

```c
struct fio_poll_s {
  fio_poll_settings_s settings;
  struct pollfd fds[2];
  int fd[2];
};
```

Two close-on-exec `epoll` file descriptors are used: one for `POLLOUT` readiness and one for `POLLIN` events. Both report errors and hang-ups. A `pollfd` pair wakes the internal `poll` used to wait on both epoll FDs at once.

#### `fio_poll_engine`

Returns `"epoll"`.

#### Fork Safety

A state callback re-creates the epoll FDs in the child process after `fork`.

---

## Notes

- Events are monitored in one-shot mode (`EPOLLONESHOT`). After an event fires, the descriptor must be re-armed with `fio_poll_monitor`.
- Error and hang-up events on a write-only registration are delivered to `on_close`, even without `EPOLLOUT`. Read registrations dispatch `on_close` on errors and hang-ups.
- `POLLPRI` monitors urgent input through `EPOLLPRI` and invokes `on_data`.
- `EPOLLRDHUP` and `EPOLLHUP` are always included in the monitored events.
- Timeouts greater than `INT_MAX` milliseconds are capped at `INT_MAX` before calling `poll`.

------------------------------------------------------------
