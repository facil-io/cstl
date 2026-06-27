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
- `udata` — opaque pointer passed to the callback.
- `propagate` — if true, call any previous handler after recording the signal.
- `immediate` — if true, run the callback inside the signal handler; otherwise defer to `fio_signal_review`.

### API Functions

#### `fio_signal_monitor`

```c
SFUNC int fio_signal_monitor(fio_signal_monitor_args_s args);
#define fio_signal_monitor(...)                                                \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})
```

Starts monitoring `sig`. Updating an existing monitor replaces its callback and `udata`. Returns `0` on success, `-1` on error.

**Note:** `immediate` callbacks must be async-signal-safe. Most code is not.

#### `fio_signal_review`

```c
SFUNC int fio_signal_review(void);
```

Calls deferred callbacks for any signals that fired since the last review. Returns the number of callbacks invoked.

#### `fio_signal_forget`

```c
SFUNC int fio_signal_forget(int sig);
```

Stops monitoring `sig` and restores the default handler. Returns `0` on success, `-1` on error.

### Example

```c
#define FIO_SIGNAL
#define FIO_LOG
#include "fio-stl.h"

static volatile int running = 1;

static void on_sigint(int sig, void *udata) {
  (void)sig; (void)udata;
  FIO_LOG_INFO("SIGINT received, shutting down...");
  running = 0;
}

int main(void) {
  fio_signal_monitor(.sig = SIGINT,
                     .callback = on_sigint,
                     .udata = NULL);

  while (running) {
    fio_signal_review();
    /* do work */
  }

  fio_signal_forget(SIGINT);
  return 0;
}
```

------------------------------------------------------------
