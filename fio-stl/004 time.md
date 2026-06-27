# Time Helpers

```c
#define FIO_TIME
#include "fio-stl.h"
```

Wall-clock and monotonic time helpers, UTC calendar conversion, and a handful of date formatters. Tiny clock toolbox, no sundial required. Implemented in [`./004 time.h`](./004%20time.h).

### Collecting Time

#### `fio_time_real`

```c
FIO_IFUNC struct timespec fio_time_real(void);
```

Returns human / watch time using `CLOCK_REALTIME`. Good for timestamps, less good for measuring elapsed time because the system clock may jump.

#### `fio_time_mono`

```c
FIO_IFUNC struct timespec fio_time_mono(void);
```

Returns monotonic time using `CLOCK_MONOTONIC`. Prefer this for intervals and deadlines.

#### `fio_time_nano`

```c
FIO_IFUNC int64_t fio_time_nano(void);
```

Returns monotonic time in nanoseconds.

#### `fio_time_micro`

```c
FIO_IFUNC int64_t fio_time_micro(void);
```

Returns monotonic time in microseconds.

#### `fio_time_milli`

```c
FIO_IFUNC int64_t fio_time_milli(void);
```

Returns monotonic time in milliseconds.

### Conversion

#### `fio_time2milli`

```c
FIO_IFUNC int64_t fio_time2milli(struct timespec);
```

Converts a `struct timespec` to milliseconds.

#### `fio_time2micro`

```c
FIO_IFUNC int64_t fio_time2micro(struct timespec);
```

Converts a `struct timespec` to microseconds.

#### `fio_time2gm`

```c
SFUNC struct tm fio_time2gm(time_t time);
```

Converts `time` to a UTC `struct tm`. This is a faster, less localized alternative to `gmtime_r`.

#### `fio_gm2time`

```c
SFUNC time_t fio_gm2time(struct tm tm);
```

Converts a UTC `struct tm` to seconds. If `tm.tm_isdst > 0`, one hour is subtracted. On platforms with `tm_gmtoff`, that offset is added.

### Date Formatting

All formatting functions write to `target`, NUL-terminate the output, and return the number of bytes written before the NUL. Provide a buffer with enough space; `64` bytes is comfortably boring.

#### `fio_time2rfc7231`

```c
SFUNC size_t fio_time2rfc7231(char *target, time_t time);
```

Writes an RFC 7231 / HTTP date, usually 29 characters:

```text
Sun, 06 Nov 1994 08:49:37 GMT
```

#### `fio_time2rfc2109`

```c
SFUNC size_t fio_time2rfc2109(char *target, time_t time);
```

Writes an RFC 2109 cookie date, usually 31 characters:

```text
Sun, 06 Nov 1994 08:49:37 -0000
```

#### `fio_time2rfc2822`

```c
SFUNC size_t fio_time2rfc2822(char *target, time_t time);
```

Writes an RFC 2822 date, usually 28-29 characters:

```text
Sun, 6-Nov-1994 08:49:37 GMT
```

#### `fio_time2log`

```c
SFUNC size_t fio_time2log(char *target, time_t time);
```

Writes common log format:

```text
[DD/MMM/yyyy:hh:mm:ss +0000]
```

Usually needs 29 bytes including the NUL terminator.

#### `fio_time2iso`

```c
SFUNC size_t fio_time2iso(char *target, time_t time);
```

Writes an ISO-ish representation. The header comment says `YYYY-MM-DD HH:MM:SS`; the implementation writes a 3-letter month name:

```text
YYYY-MMM-DD HH:MM:SS
```

Usually needs 20 bytes including the NUL terminator.

### Arithmetic

#### `fio_time_add`

```c
FIO_IFUNC struct timespec fio_time_add(struct timespec t, struct timespec t2);
```

Adds two `struct timespec` values and normalizes the nanoseconds field.

#### `fio_time_add_milli`

```c
FIO_IFUNC struct timespec fio_time_add_milli(struct timespec t, int64_t milli);
```

Adds milliseconds to `t` and normalizes the result. The implementation splits milliseconds with a `1024`-millisecond approximation before normalization; cute, but be aware.

#### `fio_time_cmp`

```c
FIO_IFUNC int fio_time_cmp(struct timespec t1, struct timespec t2);
```

Compares two `struct timespec` values.

**Returns:** `-1` if `t1 < t2`, `0` if equal, `1` if `t1 > t2`.

### Example

```c
#define FIO_TIME
#include "fio-stl.h"
#include <stdio.h>
#include <time.h>

int main(void) {
  struct timespec start = fio_time_mono();
  struct timespec later = fio_time_add_milli(start, 500);

  printf("now: %lld ms\n", (long long)fio_time_milli());
  printf("later is after start: %d\n", fio_time_cmp(later, start) > 0);

  char date[64];
  fio_time2rfc7231(date, fio_time_real().tv_sec);
  printf("http date: %s\n", date);

  return 0;
}
```

---
