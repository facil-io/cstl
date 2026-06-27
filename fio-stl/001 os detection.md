# OS Detection

[`./000 core.h`](./000%20core.h) detects the host OS and defines a few portable helpers so the rest of the library can use one code path per platform.

## Platform macros

| Macro | POSIX value | Windows value | Notes |
|---|---|---|---|
| `FIO_OS_POSIX` | `1` | `0` | Set on Linux, macOS, BSD, and other `__unix__`/`__linux__`/`__APPLE__` systems. |
| `FIO_OS_WIN` | `0` | `1` | Set on Windows, including MinGW, MSYS2, Cygwin, and Borland. |

The Windows check is evaluated first, because MinGW/MSYS2 define both `_WIN32` and `__unix__`. Treating the system as Windows avoids calling POSIX socket functions on Winsock `SOCKET` handles.

After detection, `FIO_OS_WIN` always wins over `FIO_OS_POSIX`:

```c
#if FIO_OS_WIN
#undef FIO_OS_POSIX
#define FIO_OS_POSIX 0
#endif
```

## Unix-tools level

`FIO_HAVE_UNIX_TOOLS` tells the library how Unix-like the environment is:

- `0` — no Unix tools (pure MSVC).
- `1` — full POSIX.
- `2` — MinGW.
- `3` — Cygwin.

It is used to decide whether functions such as `pipe()`, `fcntl()`, and POSIX signal APIs are available.

## Process helpers

| Macro | POSIX | Windows |
|---|---|---|
| `fio_getpid` | `getpid` | `_getpid` |
| `FIO_KILL_SELF()` | `kill(0, SIGINT)` | `GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)` |

`FIO_KILL_SELF` is guarded by `#ifndef`, so you can define your own version before including the header. The default implementation is used by the assertion/debug machinery to deliver a signal to the running process.

## Thread sleep and yield

| Macro | POSIX | Windows |
|---|---|---|
| `FIO_THREAD_WAIT(nano_sec)` | `nanosleep` | `Sleep` |
| `FIO_THREAD_YIELD()` | see below | see below |
| `FIO_THREAD_RESCHEDULE()` | `FIO_THREAD_WAIT(4)` | `FIO_THREAD_WAIT(4)` |

`FIO_THREAD_WAIT(nano_sec)` sleeps the current thread for the requested number of nanoseconds. On POSIX it builds a `struct timespec` and calls `nanosleep`. On Windows it converts the interval to milliseconds and calls `Sleep`; any non-zero request is rounded up to at least 1 ms.

`FIO_THREAD_YIELD()` hints the processor that the thread is in a spin loop. On x86/x86-64 with GCC or Clang it emits a `pause` instruction; on ARM/AArch64 with GCC or Clang it emits a `yield` instruction; with MSVC it calls `YieldProcessor()`; otherwise it falls back to `sched_yield()` on POSIX.

`FIO_THREAD_RESCHEDULE()` yields the thread by sleeping for 4 nanoseconds (`FIO_THREAD_WAIT(4)`). It is used by the lock code to back off busy waits without fully suspending the thread.

## Portable type patches

On pure MSVC `ssize_t` is missing, so the core header defines it from `SSIZE_T`:

```c
typedef SSIZE_T ssize_t;
```

`SSIZE_MAX` and `SSIZE_MIN` are also provided if the system headers do not:

```c
#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)((~(size_t)0) >> 1))
#endif
#ifndef SSIZE_MIN
#define SSIZE_MIN ((ssize_t)(~((~(size_t)0) >> 1)))
#endif
```

## `printf`-style format checking

The `FIO___PRINTF_STYLE(string_index, check_index)` macro is defined differently on each toolchain so the compiler can check `printf`-style arguments. MinGW/Cygwin use the MinGW format attribute, pure MSVC leaves it empty, and POSIX/GCC/Clang use the standard `printf` format attribute. It is documented in [`./001 compiler attributes.md`](./001%20compiler%20attributes.md).
