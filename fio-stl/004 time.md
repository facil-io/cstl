## Time Helpers

```c
#define FIO_TIME
#include "fio-stl.h"
```

By defining `FIO_TIME` or `FIO_QUEUE`, the following time-related helper functions are defined.

**Note**: this module depends on the `FIO_ATOL` module which will be automatically included.

### Collecting Monotonic / Real Time

#### `fio_time_real`

```c
struct timespec fio_time_real(void);
```

Returns human (wall clock) time. This value isn't as safe for measurements since it can be affected by system time adjustments.

#### `fio_time_mono`

```c
struct timespec fio_time_mono(void);
```

Returns monotonic time. This is the preferred time source for measuring elapsed time, as it is not affected by system time changes.

#### `fio_time_nano`

```c
int64_t fio_time_nano(void);
```

Returns monotonic time in nanoseconds (1 billionth of a second).

#### `fio_time_micro`

```c
int64_t fio_time_micro(void);
```

Returns monotonic time in microseconds (1 millionth of a second).

#### `fio_time_milli`

```c
int64_t fio_time_milli(void);
```

Returns monotonic time in milliseconds.

### Time Conversion Functions

#### `fio_time2milli`

```c
int64_t fio_time2milli(struct timespec t);
```

Converts a `struct timespec` to milliseconds.

#### `fio_time2micro`

```c
int64_t fio_time2micro(struct timespec t);
```

Converts a `struct timespec` to microseconds.

#### `fio_time2gm`

```c
struct tm fio_time2gm(time_t time);
```

A faster (yet less localized) alternative to `gmtime_r`.

See the libc `gmtime_r` documentation for details.

Returns a `struct tm` object filled with the date information.

Falls back to `gmtime_r` for dates before epoch.

This function is used internally for the formatting functions: `fio_time2rfc7231`, `fio_time2rfc2109`, `fio_time2rfc2822`, `fio_time2log`, and `fio_time2iso`.

#### `fio_gm2time`

```c
time_t fio_gm2time(struct tm tm);
```

Converts a `struct tm` to time in seconds (assuming UTC).

This function is less localized than the `mktime` / `timegm` library functions.

### Time Arithmetic Functions

#### `fio_time_add`

```c
struct timespec fio_time_add(struct timespec t, struct timespec t2);
```

Adds two `struct timespec` objects together.

Returns a normalized `struct timespec` with the sum of both time values.

#### `fio_time_add_milli`

```c
struct timespec fio_time_add_milli(struct timespec t, int64_t milli);
```

Adds milliseconds to a `struct timespec` object.

Returns a normalized `struct timespec` with the adjusted time value.

#### `fio_time_cmp`

```c
int fio_time_cmp(struct timespec t1, struct timespec t2);
```

Compares two `struct timespec` objects.

**Returns:**
- `-1` if `t1 < t2`
- `0` if `t1 == t2`
- `1` if `t1 > t2`

### Time Formatting Functions

#### `fio_time2rfc7231`

```c
size_t fio_time2rfc7231(char *target, time_t time);
```

Writes an RFC 7231 date representation (HTTP date format) to `target`.

Usually requires 29 characters, although this may vary.

The format is: `DDD, dd MON YYYY HH:MM:SS GMT`

i.e.: `Sun, 06 Nov 1994 08:49:37 GMT`

**Returns:** the number of characters written (excluding NUL terminator).

#### `fio_time2rfc2109`

```c
size_t fio_time2rfc2109(char *target, time_t time);
```

Writes an RFC 2109 date representation to `target` (HTTP Cookie format).

Usually requires 31 characters, although this may vary.

**Returns:** the number of characters written (excluding NUL terminator).

#### `fio_time2rfc2822`

```c
size_t fio_time2rfc2822(char *target, time_t time);
```

Writes an RFC 2822 date representation to `target` (Internet Message Format).

Usually requires 28 to 29 characters, although this may vary.

**Returns:** the number of characters written (excluding NUL terminator).

#### `fio_time2log`

```c
size_t fio_time2log(char *target, time_t time);
```

Writes a date representation to `target` in common log format.

Format: `[DD/MMM/yyyy:hh:mm:ss +0000]`

Usually requires 29 characters (including square brackets and NUL).

**Returns:** the number of characters written (excluding NUL terminator).

#### `fio_time2iso`

```c
size_t fio_time2iso(char *target, time_t time);
```

Writes a date representation to `target` in ISO 8601 format.

Format: `YYYY-MMM-DD HH:MM:SS`

Usually requires 20 characters (including NUL).

**Note**: the month is written as a 3-letter abbreviation (e.g., `Jan`, `Feb`), not as a numeric value.

**Returns:** the number of characters written (excluding NUL terminator).

### Example

```c
#define FIO_TIME
#include "fio-stl.h"

int main(void) {
  /* Get current time */
  struct timespec mono = fio_time_mono();
  struct timespec real = fio_time_real();
  
  /* Convert to different units */
  int64_t ms = fio_time_milli();
  int64_t us = fio_time_micro();
  int64_t ns = fio_time_nano();
  
  printf("Monotonic time: %lld ms, %lld us, %lld ns\n",
         (long long)ms, (long long)us, (long long)ns);
  
  /* Time arithmetic */
  struct timespec future = fio_time_add_milli(mono, 5000); /* 5 seconds later */
  if (fio_time_cmp(future, mono) > 0) {
    printf("Future time is greater than current time\n");
  }
  
  /* Format current time */
  time_t now = real.tv_sec;
  char buf[64];
  
  fio_time2rfc7231(buf, now);
  printf("RFC 7231: %s\n", buf);
  
  fio_time2rfc2109(buf, now);
  printf("RFC 2109: %s\n", buf);
  
  fio_time2rfc2822(buf, now);
  printf("RFC 2822: %s\n", buf);
  
  fio_time2log(buf, now);
  printf("Log format: %s\n", buf);
  
  fio_time2iso(buf, now);
  printf("ISO 8601: %s\n", buf);
  
  /* Convert between struct tm and time_t */
  struct tm tm = fio_time2gm(now);
  printf("Year: %d, Month: %d, Day: %d\n",
         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  
  time_t converted = fio_gm2time(tm);
  printf("Converted back: %s", ctime(&converted));
  
  return 0;
}
```

-------------------------------------------------------------------------------
