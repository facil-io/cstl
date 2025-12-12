## Signal Monitoring

```c
#define FIO_SIGNAL
#include "fio-stl.h"
```

OS signal callbacks are very limited in the actions they are allowed to take. In fact, one of the only actions they are allowed to take is to set a volatile atomic flag.

The facil.io STL offers helpers that perform this very common pattern of declaring a flag, watching a signal, setting a flag and (later) calling a callback outside of the signal handler that would handle the actual event.

**Note**: Either POSIX or Windows is required for the `fio_signal` API.

### Configuration Macros

#### `FIO_SIGNAL_MONITOR_MAX`

```c
#ifndef FIO_SIGNAL_MONITOR_MAX
/* The maximum number of signals the implementation will be able to monitor */
#define FIO_SIGNAL_MONITOR_MAX 24
#endif
```

Defines the maximum number of signals that can be monitored simultaneously. Defaults to `24`.

### Signal Monitoring API

#### `fio_signal_monitor`

```c
int fio_signal_monitor(fio_signal_monitor_args_s args);
/* Named arguments using macro. */
#define fio_signal_monitor(...)                                                \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})

typedef struct {
  /** The signal number to listen for. */
  int sig;
  /** The callback to run - leave NULL to ignore signal. */
  void (*callback)(int sig, void *udata);
  /** Opaque user data. */
  void *udata;
  /** Should the signal propagate to existing handler(s)? */
  bool propagate;
  /** Call (safe) callback immediately? or wait for `fio_signal_review`? */
  bool immediate;
} fio_signal_monitor_args_s;
```

Starts to monitor for the specified signal, setting an optional callback.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_signal_monitor(.sig = SIGINT,
                   .callback = on_sigint,
                   .udata = my_data);
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `sig` | `int` | The signal number to listen for (required) |
| `callback` | `void (*)(int, void *)` | The callback to run; leave `NULL` to ignore signal |
| `udata` | `void *` | Opaque user data passed to the callback |
| `propagate` | `bool` | If `true` and a previous handler was set, it will be called |
| `immediate` | `bool` | If `true`, callback runs immediately in signal context; otherwise waits for `fio_signal_review` |

**Returns:** `0` on success, `-1` on error.

**Note**: if `callback` is `NULL`, the signal will be ignored.

**Note**: if the signal is already being monitored, the callback and `udata` pointers are updated.

**Note**: `udata` stands for "user data", it is an opaque pointer that is simply passed along to the callback.

**Note**: when `immediate` is `true`, the callback must be signal-safe (async-signal-safe). Most operations are not safe to perform in a signal handler context.

#### `fio_signal_review`

```c
int fio_signal_review(void);
```

Reviews all signals, calling any relevant callbacks that were deferred (i.e., monitors where `immediate` was `false`).

**Returns:** the number of callbacks that were called.

#### `fio_signal_forget`

```c
int fio_signal_forget(int sig);
```

Stops monitoring the specified signal and restores the default signal handler.

**Returns:** `0` on success, `-1` on error (e.g., signal was not being monitored).

### Example

```c
#define FIO_SIGNAL
#define FIO_LOG
#include "fio-stl.h"

static volatile int running = 1;

static void on_sigint(int sig, void *udata) {
  (void)sig;
  (void)udata;
  FIO_LOG_INFO("SIGINT received, shutting down...");
  running = 0;
}

int main(void) {
  /* Monitor SIGINT - callback deferred until fio_signal_review is called */
  fio_signal_monitor(.sig = SIGINT,
                     .callback = on_sigint,
                     .udata = NULL);

  FIO_LOG_INFO("Running... Press Ctrl+C to stop.");

  while (running) {
    /* Check for signals and call deferred callbacks */
    fio_signal_review();
    /* ... do work ... */
  }

  /* Stop monitoring */
  fio_signal_forget(SIGINT);

  return 0;
}
```

-------------------------------------------------------------------------------
