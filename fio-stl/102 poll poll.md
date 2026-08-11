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

The poll backend keeps monitored descriptors in an internal imap (`fio___poll_map_s`) and snapshots its armed flags before each `poll()` call. Descriptors added or re-armed during `fio_poll_review` update the retained map directly; surviving one-shot flags are restored when the call returns.

`fio_poll_forget` removes the descriptor from the retained map, which also prevents any surviving snapshot flags from being restored. It is not a cancellation guarantee: an event already returned by an in-flight `poll()` call may still be dispatched. This matches the epoll/kqueue backends, whose in-hand event lists always dispatch. As on every backend, `udata` lifetime during dispatch is the caller's responsibility (the facil.io IO layer only forgets from the reactor thread, never concurrently with a review).

One-shot flag accounting strips whole event groups: `WSAPoll` reports sub-band bits (e.g. `POLLRDNORM`) while `POLLIN`/`POLLOUT` are supersets, so a raw bitwise strip would leave band bits (e.g. `POLLRDBAND`) armed on Windows. Fired `POLLIN`/`POLLOUT` groups are therefore stripped in full.

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
- Fired events are stripped from the descriptor’s flags (one-shot semantics). Surviving flags are restored to retained entries after `poll()` returns; concurrent removals (which delete the retained entry) prevent that restoration.

------------------------------------------------------------
