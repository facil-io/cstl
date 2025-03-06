## Signal Monitoring

```c
#define FIO_SIGNAL
#include "fio-stl.h"
```

OS signal callbacks are very limited in the actions they are allowed to take. In fact, one of the only actions they are allowed to take is to set a volatile atomic flag.

The facil.io STL offers helpers that perform this very common pattern of declaring a flag, watching a signal, setting a flag and (later) calling a callback outside of the signal handler that would handle the actual event.

When defining `FIO_SIGNAL`, the following function are defined.

#### `fio_signal_monitor`

```c
typedef struct {
  /** The signal number to listen for. */
  int sig;
  /** The callback to run - leave NULL to ignore signal. */
  void (*callback)(int sig, void *udata);
  /** Opaque user data. */
  void *udata;
  /** Should the signal propagate to existing handler(s)? */
  bool propagate;
  /** Should the callback run immediately (signal safe code) or wait for `fio_signal_review`? */
  bool immediate;
} fio_signal_monitor_args_s;

SFUNC int fio_signal_monitor(fio_signal_monitor_args_s args);
#define fio_signal_monitor(...)                                                \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})
```

Starts to monitor for the specified signal, setting an optional callback.

If `callback` is `NULL`, the signal will be ignored.

If the signal is already being monitored, the callback and `udata` pointers are updated.

If `propagate` is true and a previous signal handler was set, it will be called.

**Note**: `udata` stands for "user data", it is an opaque pointer that is simply passed along to the callback.

#### `fio_signal_review`

```c
int fio_signal_review(void);
```

Reviews all signals, calling any relevant callbacks.

#### `fio_signal_forget`

```c
int fio_signal_forget(int sig);
```

Stops monitoring the specified signal.

-------------------------------------------------------------------------------
