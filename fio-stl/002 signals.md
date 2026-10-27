# Signal Monitoring

```c
#define FIO_SIGNAL
#include "fio-stl.h"
```

A small wrapper around OS signals. It sets a handler, records that the signal fired, and lets you call the real callback later from normal execution context — the safe pattern, since signal handlers are heavily restricted. POSIX and Windows are supported.

### Configuration Macros

#### `FIO_SIGNAL_MONITOR_MAX`

```c
#ifndef FIO_SIGNAL_MONITOR_MAX
#define FIO_SIGNAL_MONITOR_MAX 24
#endif
```

Maximum number of signals that can be monitored at once.

### Portable Signal Aliases

#### `FIO_SIGNAL_USER1` / `FIO_SIGNAL_USER2` / `FIO_SIGNAL_USER_UNREGISTERED`

```c
#if FIO_OS_POSIX
#define FIO_SIGNAL_USER1             SIGUSR1
#define FIO_SIGNAL_USER2             SIGUSR2
#define FIO_SIGNAL_USER_UNREGISTERED SIGUSR2
#elif FIO_OS_WIN
#define FIO_SIGNAL_USER1             SIGBREAK
#define FIO_SIGNAL_USER2             SIGABRT
#define FIO_SIGNAL_USER_UNREGISTERED SIGBREAK
#endif
```

Maps user signals across POSIX and Windows.

### Types

#### `fio_signal_monitor_args_s`

```c
typedef struct {
  int sig;
  void (*callback)(int sig, void *udata);
  void *udata;
  bool propagate;
  bool immediate;
} fio_signal_monitor_args_s;
```

- `sig` — signal number to watch.
- `callback` — function to run; `NULL` ignores the signal.
- `udata` — borrowed opaque pointer passed to the callback; the module never frees it.
- `propagate` — if true, call any previous handler after recording the signal.
- `immediate` — if true, run the callback inside the signal handler; otherwise defer to `fio_signal_review`.

### API Functions

#### `fio_signal_monitor`

```c
SFUNC int fio_signal_monitor(fio_signal_monitor_args_s args);
#define fio_signal_monitor(...)                                                \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})
```

Starts monitoring positive signal number `sig`. Updating an existing monitor replaces its callback and borrowed `udata`, retaining any pending deferred delivery. Returns `0` on success, `-1` for an invalid signal, exhausted capacity, an OS error, or unsupported propagation. A failed update leaves the monitor unchanged.

Register, update and forget from one thread at phase boundaries, not concurrently with signal delivery. Call `fio_signal_review` from one thread. Neither updates nor removal waits for an immediate callback already running in another thread.

**Note**: Immediate callbacks and propagated handlers run inside the signal handler: they must be async-signal-safe and must not call the signal monitoring API. Most code is not async-signal-safe. Do not use this API to recover from synchronous faults such as an invalid memory access.

On POSIX, propagation forwards the original `siginfo_t` and context to `SA_SIGINFO` handlers, and preserves the saved handler's mask, restart, alternate-stack and child-notification flags. `SIG_DFL` and `SIG_IGN` are not called. Propagation to a saved action reported as `SA_RESETHAND` returns `-1`; its one-shot disposition cannot be preserved by direct forwarding. Do not request propagation to one-shot handlers on systems that omit this flag when querying `sigaction`. Serialize external `signal`/`sigaction` changes to the same signal with this API.

On Windows, the catcher reinstalls itself before callbacks, preserving the saved previous handler for repeated propagation. If reinstallation fails, it skips callbacks. The CRT's reset-before-entry window means this wrapper cannot guarantee lossless delivery. `SIGFPE` is not rearmed automatically, as required by the CRT.

#### `fio_signal_review`

```c
SFUNC int fio_signal_review(void);
```

Calls deferred callbacks for pending signals and returns the number of callbacks actually invoked. Multiple deliveries to one monitor coalesce into one pending callback; this is not a signal counter. Pending state is consumed atomically, so delivery racing review remains pending for this or the next review. A `NULL` callback never queues or counts a callback.

Use one review thread. Review callbacks may update or forget monitors. Complete review before releasing borrowed callback data.

#### `fio_signal_forget`

```c
SFUNC int fio_signal_forget(int sig);
```

Stops monitoring `sig` and discards pending delivery. If the module's handler is still installed, the handler that was active before the first `fio_signal_monitor` call for `sig` is restored (including its POSIX mask and flags). If another handler replaced the module's handler since, the OS state is left untouched. Returns `0` on success, `-1` for an unknown signal or OS error. On OS failure, the watcher and pending state remain intact. Successful removal releases the watcher's capacity for reuse. It does not wait for callbacks on other threads; do not remove a monitor while delivery is in progress.

### Example

```c
#define FIO_SIGNAL
#define FIO_LOG
#include "fio-stl.h"

static int running = 1;

static void on_sigint(int sig, void *udata) {
  (void)sig; (void)udata;
  FIO_LOG_INFO("SIGINT received, shutting down...");
  running = 0;
}

int main(void) {
  if (fio_signal_monitor(.sig = SIGINT, .callback = on_sigint))
    return 1;

  while (running) {
    fio_signal_review();
    /* do work */
  }

  return fio_signal_forget(SIGINT) ? 1 : 0;
}
```

---
