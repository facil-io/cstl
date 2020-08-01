## Time Helpers

By defining `FIO_TIME` or `FIO_QUEUE`, the following time related helpers functions are defined:

#### `fio_time_real`

```c
struct timespec fio_time_real();
```

Returns human (watch) time... this value isn't as safe for measurements.

#### `fio_time_mono`

```c
struct timespec fio_time_mono();
```

Returns monotonic time.

#### `fio_time_nano`

```c
uint64_t fio_time_nano();
```

Returns monotonic time in nano-seconds (now in 1 micro of a second).

#### `fio_time_micro`

```c
uint64_t fio_time_micro();
```

Returns monotonic time in micro-seconds (now in 1 millionth of a second).

#### `fio_time_milli`

```c
uint64_t fio_time_milli();
```

Returns monotonic time in milliseconds.

#### `fio_time2gm`

```c
struct tm fio_time2gm(time_t timer);
```

A faster (yet less localized) alternative to `gmtime_r`.

See the libc `gmtime_r` documentation for details.

Returns a `struct tm` object filled with the date information.

This function is used internally for the formatting functions: , `fio_time2rfc7231`, `fio_time2rfc2109`, and `fio_time2rfc2822`.

#### `fio_gm2time`

```c
time_t fio_gm2time(struct tm tm)
```

Converts a `struct tm` to time in seconds (assuming UTC).

This function is less localized then the `mktime` / `timegm` library functions.

#### `fio_time2rfc7231`

```c
size_t fio_time2rfc7231(char *target, time_t time);
```

Writes an RFC 7231 date representation (HTTP date format) to target.

Requires 29 characters (for positive, 4 digit years).

The format is similar to DDD, dd, MON, YYYY, HH:MM:SS GMT

i.e.: Sun, 06 Nov 1994 08:49:37 GMT

#### `fio_time2rfc2109`

```c
size_t fio_time2rfc2109(char *target, time_t time);
```

Writes an RFC 2109 date representation to target.

Requires 31 characters (for positive, 4 digit years).

#### `fio_time2rfc2822`

```c
size_t fio_time2rfc2822(char *target, time_t time);
```

Writes an RFC 2822 date representation to target.

Requires 28 or 29 characters (for positive, 4 digit years).

-------------------------------------------------------------------------------
