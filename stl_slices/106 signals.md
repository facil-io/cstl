## Signal Monitoring

OS signal callbacks are very limited in the actions they are allowed to take. In fact, one of the only actions they are allowed to take is to set a volatile atomic flag.

The facil.io STL offers helpers that perform this very common pattern of declaring a flag, watching a signal, setting a flag and (later) calling a callback outside of the signal handler that would handle the actual event.

When defining `FIO_SIGNAL`, the following function are defined.

#### `fio_signal_monitor`

```c
int fio_signal_monitor(int sig, void (*callback)(int sig, void *), void *udata);
```

Starts to monitor for the specified signal, setting an optional callback.

If the signal is already being monitored, the callback and `udata` pointers are updated.

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
