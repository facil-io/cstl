## Logging and Assertions

```c
#define FIO_LOG
#include "fio-stl.h"
```

The logging module provides heap-allocation-free logging macros and assertion utilities. When `FIO_LOG` is defined, the `FIO_LOG2STDERR` function and related logging macros become functional (otherwise they are no-ops).

**Note:** `FIO_LOG` uses `libc` functions (`memcpy`, `vsnprintf`, `fwrite`) and cannot be used for authoring apps without `libc` unless these functions are implemented separately and shadowed by macros before the module is included.

**Note:** in **all** logging macros, `msg` **must** be a string literal (`const char *`).

### Logging Levels

The following logging level constants are always defined:

```c
#define FIO_LOG_LEVEL_NONE    0  /* No logging */
#define FIO_LOG_LEVEL_FATAL   1  /* Log fatal errors */
#define FIO_LOG_LEVEL_ERROR   2  /* Log errors and fatal errors */
#define FIO_LOG_LEVEL_WARNING 3  /* Log warnings, errors and fatal errors */
#define FIO_LOG_LEVEL_INFO    4  /* Log info, warnings, errors and fatal errors */
#define FIO_LOG_LEVEL_DEBUG   5  /* Log everything, including debug messages */
```

### Configuration Macros

#### `FIO_LOG_LENGTH_LIMIT`

```c
#define FIO_LOG_LENGTH_LIMIT 1024
```

Defines the maximum length of a log message. Messages exceeding this limit will be truncated. The default value is `1024`. It's recommended that this value be greater than 128.

#### `FIO_LOG_LEVEL_DEFAULT`

```c
#ifndef FIO_LOG_LEVEL_DEFAULT
#if DEBUG
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_DEBUG
#else
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_INFO
#endif
#endif
```

Sets the initial logging level. By default, the level is `FIO_LOG_LEVEL_INFO` (4) for normal compilation and `FIO_LOG_LEVEL_DEBUG` (5) when `DEBUG` is defined.

### Logging Level Control

#### `FIO_LOG_LEVEL_GET`

```c
#define FIO_LOG_LEVEL_GET() ((fio___log_level()))
```

Returns the current application-wide logging level as an integer.

#### `FIO_LOG_LEVEL_SET`

```c
#define FIO_LOG_LEVEL_SET(new_level) fio___log_level_set(new_level)
```

Sets the application-wide logging level to `new_level`. Returns the new level.

Example:

```c
FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING); // Only log warnings and above
int level = FIO_LOG_LEVEL_GET();          // Returns 3
```

### Core Logging Functions

#### `FIO_LOG2STDERR`

```c
void FIO_LOG2STDERR(const char *format, ...);
```

A `printf`-style function that logs a message to `stderr` without allocating heap memory for the string (unlike `fprintf` which might).

Messages exceeding `FIO_LOG_LENGTH_LIMIT` will be truncated with a warning.

**Note:** this function is defined as a static function, allowing it to be overridden by defining your own version before including the module.

#### `FIO_LOG_WRITE`

```c
#define FIO_LOG_WRITE(...) FIO_LOG2STDERR("(" FIO__FILE__ ":" FIO_MACRO2STR(__LINE__) "): " __VA_ARGS__)
```

Routes to `FIO_LOG2STDERR` after prefixing the message with the file name and line number where the log statement occurs.

### Logging Macros

All logging macros check the current log level before printing. If the log level is below the macro's threshold, the message is not printed.

#### `FIO_LOG_FATAL`

```c
#define FIO_LOG_FATAL(...) /* logs if level >= FIO_LOG_LEVEL_FATAL */
```

Logs a fatal error message **if** the log level is equal to or above `FIO_LOG_LEVEL_FATAL` (1).

Output is prefixed with `FATAL:` in bold inverse text.

#### `FIO_LOG_ERROR`

```c
#define FIO_LOG_ERROR(...) /* logs if level >= FIO_LOG_LEVEL_ERROR */
```

Logs an error message **if** the log level is equal to or above `FIO_LOG_LEVEL_ERROR` (2).

Output is prefixed with `ERROR:` in bold text.

#### `FIO_LOG_SECURITY`

```c
#define FIO_LOG_SECURITY(...) /* logs if level >= FIO_LOG_LEVEL_ERROR */
```

Logs a security-related message **if** the log level is equal to or above `FIO_LOG_LEVEL_ERROR` (2).

Output is prefixed with `SECURITY:` in bold text.

#### `FIO_LOG_WARNING`

```c
#define FIO_LOG_WARNING(...) /* logs if level >= FIO_LOG_LEVEL_WARNING */
```

Logs a warning message **if** the log level is equal to or above `FIO_LOG_LEVEL_WARNING` (3).

Output is prefixed with `WARNING:` in dim text.

#### `FIO_LOG_INFO`

```c
#define FIO_LOG_INFO(...) /* logs if level >= FIO_LOG_LEVEL_INFO */
```

Logs an informational message **if** the log level is equal to or above `FIO_LOG_LEVEL_INFO` (4).

Output is prefixed with `INFO:`.

#### `FIO_LOG_DEBUG`

```c
#define FIO_LOG_DEBUG(...) /* logs if level >= FIO_LOG_LEVEL_DEBUG */
```

Logs a debug message **if** the log level is equal to or above `FIO_LOG_LEVEL_DEBUG` (5).

Output is prefixed with `DEBUG:` followed by the file name and line number.

#### `FIO_LOG_DEBUG2`

```c
#define FIO_LOG_DEBUG2(...) /* logs if level >= FIO_LOG_LEVEL_DEBUG */
```

Same as `FIO_LOG_DEBUG` but without the file name and line number prefix.

Output is prefixed with `DEBUG:` only.

### Debug-Only Logging Macros

These macros only produce output when `DEBUG` is defined at compile time. Otherwise, they are no-ops with zero runtime overhead.

#### `FIO_LOG_DDEBUG`

```c
#define FIO_LOG_DDEBUG(...) /* FIO_LOG_DEBUG if DEBUG defined, else no-op */
```

Same as `FIO_LOG_DEBUG` when `DEBUG` is defined. Otherwise a no-op.

#### `FIO_LOG_DDEBUG2`

```c
#define FIO_LOG_DDEBUG2(...) /* FIO_LOG_DEBUG2 if DEBUG defined, else no-op */
```

Same as `FIO_LOG_DEBUG2` when `DEBUG` is defined. Otherwise a no-op.

#### `FIO_LOG_DERROR`

```c
#define FIO_LOG_DERROR(...) /* FIO_LOG_ERROR if DEBUG defined, else no-op */
```

Same as `FIO_LOG_ERROR` when `DEBUG` is defined. Otherwise a no-op.

#### `FIO_LOG_DSECURITY`

```c
#define FIO_LOG_DSECURITY(...) /* FIO_LOG_SECURITY if DEBUG defined, else no-op */
```

Same as `FIO_LOG_SECURITY` when `DEBUG` is defined. Otherwise a no-op.

#### `FIO_LOG_DWARNING`

```c
#define FIO_LOG_DWARNING(...) /* FIO_LOG_WARNING if DEBUG defined, else no-op */
```

Same as `FIO_LOG_WARNING` when `DEBUG` is defined. Otherwise a no-op.

#### `FIO_LOG_DINFO`

```c
#define FIO_LOG_DINFO(...) /* FIO_LOG_INFO if DEBUG defined, else no-op */
```

Same as `FIO_LOG_INFO` when `DEBUG` is defined. Otherwise a no-op.

### Assertion Macros

#### `FIO_ASSERT`

```c
#define FIO_ASSERT(cond, ...)
```

Reports an error unless `cond` is true. If the assertion fails:

1. Prints the message using `FIO_LOG_FATAL`
2. Prints the current `errno` value and its string description
3. Raises `SIGINT` (when `DEBUG` is defined) to allow debuggers to pause execution
4. Calls `abort()` to terminate the program

Example:

```c
void *ptr = malloc(size);
FIO_ASSERT(ptr, "memory allocation of %zu bytes failed", size);
```

#### `FIO_ASSERT_ALLOC`

```c
#define FIO_ASSERT_ALLOC(ptr) FIO_ASSERT((ptr), "memory allocation failed.")
```

A convenience macro for testing allocation failures. Equivalent to `FIO_ASSERT(ptr, "memory allocation failed.")`.

Example:

```c
void *ptr = malloc(size);
FIO_ASSERT_ALLOC(ptr);
```

#### `FIO_ASSERT_DEBUG`

```c
#define FIO_ASSERT_DEBUG(cond, ...)
```

Ignored unless `DEBUG` is defined.

When `DEBUG` is defined, reports an error unless `cond` is true. If the assertion fails:

1. Prints the message using `FIO_LOG_FATAL` with file name and line number
2. Prints the current `errno` value and its string description
3. Raises `SIGINT` to allow debuggers to catch the failure
4. Calls `exit(-1)` to terminate the program

**Note:** unlike `FIO_ASSERT`, this macro calls `exit(-1)` instead of `abort()`, and only raises `SIGINT` when `DEBUG` is defined.

#### `FIO_ASSERT_STATIC`

```c
#define FIO_ASSERT_STATIC(cond, msg)
```

Performs a static assertion test at compile time. If `cond` is false, compilation fails with the message `msg`.

**Note:** `cond` **must** be a constant expression and `msg` cannot contain format specifiers.

Example:

```c
FIO_ASSERT_STATIC(sizeof(int) >= 4, "int must be at least 32 bits");
```

### Example Usage

```c
#define FIO_LOG
#include "fio-stl.h"

int main(void) {
  // Set logging level
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING);
  
  // These will not print (below warning level)
  FIO_LOG_INFO("This info message won't appear");
  FIO_LOG_DEBUG("This debug message won't appear");
  
  // These will print
  FIO_LOG_WARNING("number invalid: %d", 42);
  FIO_LOG_ERROR("something went wrong");
  
  // Direct logging (always prints, ignores level)
  FIO_LOG2STDERR("Direct message to stderr");
  
  // Assertions
  void *ptr = malloc(100);
  FIO_ASSERT_ALLOC(ptr);
  
  int value = 5;
  FIO_ASSERT(value > 0, "value must be positive, got %d", value);
  
  free(ptr);
  return 0;
}
```

-------------------------------------------------------------------------------
