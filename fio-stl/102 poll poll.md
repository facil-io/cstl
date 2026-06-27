# POSIX Polling — poll Backend

```c
#define FIO_POLL_ENGINE_POLL
#define FIO_POLL
#include "fio-stl.h"
```

Implementation of the portable polling API on top of POSIX `poll()` / Windows `WSAPoll`. This is the fallback backend and is selected automatically when neither epoll nor kqueue is available.

The public API is documented in [`102 poll api.md`](./102%20poll%20api.md). This file describes backend-specific behavior.

---

## Backend Details

#### `struct fio_poll_s`

```c
struct fio_poll_s {
  fio_poll_settings_s settings;
  fio___poll_map_s map;
  FIO___LOCK_TYPE lock;
};
```

The poll backend keeps monitored descriptors in an internal imap (`fio___poll_map_s`) and takes a snapshot before each `poll()` call. Descriptors added or re-armed during `fio_poll_review` are merged with surviving one-shot flags when the call returns.

Do not rely on `fio_poll_forget` from another thread to cancel a descriptor that is already inside the in-flight snapshot: if the descriptor does not fire, the surviving snapshot entry can be merged back.

#### `fio_poll_engine`

Returns `"poll"`.

#### `fio_poll_close_all`

```c
SFUNC void fio_poll_close_all(fio_poll_s *p);
```

Additional helper available only in the poll backend. Closes every monitored socket and calls `on_close` for each one. Useful during shutdown.

---

## Notes

- `POLLRDHUP` is used when available; otherwise the backend relies on `POLLHUP`, `POLLERR`, and `POLLNVAL` for close/error detection.
- On Windows, `POLLPRI` is omitted because `WSAPoll` rejects it.
- Fired events are stripped from the descriptor’s flags (one-shot semantics). Surviving flags are merged back into the poll set after `poll()` returns; additions / re-arms merge cleanly, while concurrent removals of in-flight snapshot entries are not a cancellation guarantee.

------------------------------------------------------------
