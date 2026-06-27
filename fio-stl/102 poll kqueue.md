# POSIX Polling — kqueue Backend

```c
#define FIO_POLL_ENGINE_KQUEUE
#define FIO_POLL
#include "fio-stl.h"
```

Implementation of the portable polling API on top of BSD/macOS `kqueue`. Normally selected automatically when `<sys/event.h>` is available; define `FIO_POLL_ENGINE_KQUEUE` to force it.

The public API is documented in [`102 poll api.md`](./102%20poll%20api.md). This file describes backend-specific behavior.

---

## Backend Details

#### `struct fio_poll_s`

```c
struct fio_poll_s {
  fio_poll_settings_s settings;
  int fd;
};
```

A single kqueue descriptor is used for both read and write filters.

#### `fio_poll_engine`

Returns `"kqueue"`.

#### Fork Safety

A state callback re-creates the kqueue descriptor in the child process after `fork`.

---

## Notes

- Read and write filters are registered with `EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT`.
- `EV_EOF` and `EV_ERROR` flags are routed to `on_close`.
- `fio_poll_review` uses a `timespec` built from the millisecond timeout.

------------------------------------------------------------
