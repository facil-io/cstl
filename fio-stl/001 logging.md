# Logging and Assertions

```c
#define FIO_LOG
#include "fio-stl.h"
```

Heap-allocation-free logging macros and assertion utilities. `FIO_LOG2STDERR` and the `FIO_LOG_*` macros are functional when `FIO_LOG` or `FIO_LEAK_COUNTER` is defined; otherwise they are no-ops.

`FIO_LOG` uses `libc` functions (`vsnprintf`, `fwrite`) and the library helper `fio_memcpy32`. If you are building without `libc`, provide your own implementations or shadowing macros before including the module.

## Log Levels

```c
#define FIO_LOG_LEVEL_NONE    0  /* no logging */
#define FIO_LOG_LEVEL_FATAL   1  /* fatal errors */
#define FIO_LOG_LEVEL_ERROR   2  /* errors and above */
#define FIO_LOG_LEVEL_WARNING 3  /* warnings and above */
#define FIO_LOG_LEVEL_INFO    4  /* info and above */
#define FIO_LOG_LEVEL_DEBUG   5  /* everything, including debug */
```

## Log Level Control

### `FIO_LOG_LEVEL_SET`

```c
#define FIO_LOG_LEVEL_SET(new_level) fio___log_level_set(new_level)
```

Sets the application-wide logging level. Returns the new level.

### `FIO_LOG_LEVEL_GET`

```c
#define FIO_LOG_LEVEL_GET() ((fio___log_level()))
```

Returns the current logging level as an integer.

```c
FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING);
int level = FIO_LOG_LEVEL_GET(); /* 3 */
```

## Configuration

### `FIO_LOG_LEVEL_DEFAULT`

Initial level. Defaults to `FIO_LOG_LEVEL_INFO` unless `DEBUG` is defined and truthy (`defined(DEBUG) && DEBUG`), in which case it is `FIO_LOG_LEVEL_DEBUG`.

### `FIO_STDERR_FILE`

```c
#define FIO_STDERR_FILE stderr
```

Output destination for all logging. Define it before including `./fio-stl.h` to redirect logs to another `FILE*`.

### `FIO_LOG_LENGTH_LIMIT`

```c
#define FIO_LOG_LENGTH_LIMIT 1024
```

Log buffer size. Keep this above `128`.

- When `FIO_LOG_LENGTH_LIMIT > 128`, the formatted message is limited to `FIO_LOG_LENGTH_LIMIT - 34` bytes, followed by a 32-byte truncation warning.
- When `FIO_LOG_LENGTH_LIMIT <= 128`, `vsnprintf` is capped at `FIO_LOG_LENGTH_LIMIT - 2` bytes; the truncation warning is appended afterward.
- Values `<= 2` are dangerous: the `vsnprintf` size argument underflows/overflows.

## Core Logging Function

### `FIO_LOG2STDERR`

```c
void FIO_LOG2STDERR(const char *format, ...);
```

`printf`-style output to `FIO_STDERR_FILE` using only stack-allocated memory. Truncation follows the two `FIO_LOG_LENGTH_LIMIT` paths described above. In `./fio-stl/001 logging.h` the name is `#undef`-ed before the `static` function definition, so a pre-include macro is removed and a post-inclusion function definition would conflict. Override it after inclusion only by defining `FIO_LOG2STDERR` as a macro.

## Log Macros

Most macros check the current log level before printing. `FIO_LOG_WRITE` has no level check and always prefixes output with the file and line number.

| Macro | Level | Prefix |
|-------|-------|--------|
| `FIO_LOG_FATAL(...)` | ≥ `FATAL` | `FATAL:` (bold inverse) |
| `FIO_LOG_ERROR(...)` | ≥ `ERROR` | `ERROR:` (bold) |
| `FIO_LOG_SECURITY(...)` | ≥ `ERROR` | `SECURITY:` (bold) |
| `FIO_LOG_WARNING(...)` | ≥ `WARNING` | `WARNING:` (dim) |
| `FIO_LOG_INFO(...)` | ≥ `INFO` | `INFO:` |
| `FIO_LOG_DEBUG(...)` | ≥ `DEBUG` | `DEBUG:` + file:line |
| `FIO_LOG_DEBUG2(...)` | ≥ `DEBUG` | `DEBUG:` |
| `FIO_LOG_WRITE(...)` | always | file:line |

## Debug-Only Log Macros

These expand to their non-`D` counterparts when `DEBUG` is defined, and to no-ops otherwise.

- `FIO_LOG_DDEBUG(...)`
- `FIO_LOG_DDEBUG2(...)`
- `FIO_LOG_DERROR(...)`
- `FIO_LOG_DSECURITY(...)`
- `FIO_LOG_DWARNING(...)`
- `FIO_LOG_DINFO(...)`

## Assertions

### `FIO_ASSERT`

```c
#define FIO_ASSERT(cond, ...)
```

If `cond` is false, prints a fatal message, prints `errno` and its string, sends `SIGINT` in debug builds, and calls `exit(-1)`.

### `FIO_ASSERT_ALLOC`

```c
#define FIO_ASSERT_ALLOC(ptr) FIO_ASSERT((ptr), "memory allocation failed.")
```

Convenience wrapper for allocation failures.

### `FIO_ASSERT_DEBUG`

```c
#define FIO_ASSERT_DEBUG(cond, ...)
```

Active only when `DEBUG` is defined. On failure it behaves like `FIO_ASSERT`, printing the file and line number before terminating.

### `FIO_ASSERT_STATIC`

```c
#define FIO_ASSERT_STATIC(cond, msg)
```

Compile-time assertion. `cond` must be a constant expression; `msg` is a string literal with no format specifiers. Falls back to a sized-array trick on pre-C11 compilers.

See also: [Compiler Attributes](./001 compiler attributes.md).

## Leak Counter Helpers

Enabled by `FIO_LEAK_COUNTER` (default `1`). `FIO_NO_LOG` and `FIO_LEAK_COUNTER` are mutually exclusive because leak reports are printed through the log.

### `FIO_LEAK_COUNTER_DEF`

```c
#define FIO_LEAK_COUNTER_DEF(name)
```

Defines a named counter and a cleanup function that reports remaining allocations at process exit.

### `FIO_LEAK_COUNTER_ON_ALLOC`

```c
#define FIO_LEAK_COUNTER_ON_ALLOC(name)
```

Increment the named counter. Call after a successful allocation.

### `FIO_LEAK_COUNTER_ON_FREE`

```c
#define FIO_LEAK_COUNTER_ON_FREE(name)
```

Decrement the named counter. Call when memory is freed. Detects double-free.

### `FIO_LEAK_COUNTER_COUNT`

```c
#define FIO_LEAK_COUNTER_COUNT(name)
```

Returns the current value of the named counter.

### `FIO_LEAK_COUNTER_SKIP_EXIT`

```c
#define FIO_LEAK_COUNTER_SKIP_EXIT 0
```

Set to `1` to prevent automatic leak reporting at exit.

## Example

```c
#define FIO_LOG
#include "fio-stl.h"

int main(void) {
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING);

  FIO_LOG_INFO("not printed");
  FIO_LOG_WARNING("number invalid: %d", 42);

  FIO_LOG2STDERR("direct message");

  void *ptr = malloc(100);
  FIO_ASSERT_ALLOC(ptr);
  FIO_ASSERT(ptr != NULL, "expected a pointer");

  free(ptr);
  return 0;
}
```
