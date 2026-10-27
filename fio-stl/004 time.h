/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TIME               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                  Time Helpers



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TIME) && !defined(H___FIO_TIME___H)
#define H___FIO_TIME___H

/* *****************************************************************************
Collecting Monotonic / Real Time
***************************************************************************** */

/** Returns human (watch) time... this value isn't as safe for measurements. */
FIO_IFUNC struct timespec fio_time_real(void);

/** Returns monotonic time. */
FIO_IFUNC struct timespec fio_time_mono(void);

/** Returns monotonic time in nano-seconds (now in 1 billionth of a second). */
FIO_IFUNC int64_t fio_time_nano(void);

/** Returns monotonic time in micro-seconds (now in 1 millionth of a second). */
FIO_IFUNC int64_t fio_time_micro(void);

/** Returns monotonic time in milliseconds. */
FIO_IFUNC int64_t fio_time_milli(void);

/** Converts a `struct timespec` to milliseconds. */
FIO_IFUNC int64_t fio_time2milli(struct timespec);

/** Converts a `struct timespec` to microseconds. */
FIO_IFUNC int64_t fio_time2micro(struct timespec);

/**
 * A faster (yet less localized) alternative to `gmtime_r`.
 *
 * See the libc `gmtime_r` documentation for details.
 *
 * Falls back to `gmtime_r` for dates before epoch.
 */
SFUNC struct tm fio_time2gm(time_t time);

/** Converts a `struct tm` to time in seconds (assuming UTC). */
SFUNC time_t fio_gm2time(struct tm tm);

/**
 * Writes an RFC 7231 date representation (HTTP date format) to target.
 *
 * Usually requires 29 characters, although this may vary.
 */
SFUNC size_t fio_time2rfc7231(char *target, time_t time);

/**
 * Writes an RFC 2109 date representation to target (HTTP Cookie Format).
 *
 * Usually requires 31 characters, although this may vary.
 */
SFUNC size_t fio_time2rfc2109(char *target, time_t time);

/**
 * Writes an RFC 2822 date representation to target (Internet Message Format).
 *
 * Usually requires 28 to 29 characters, although this may vary.
 */
SFUNC size_t fio_time2rfc2822(char *target, time_t time);

/**
 * Writes a date representation to target in common log format. i.e.,
 *
 *         [DD/MMM/yyyy:hh:mm:ss +0000]
 *
 * Usually requires 29 characters (including square brackets and NUL).
 */
SFUNC size_t fio_time2log(char *target, time_t time);

/**
 * Writes a date representation to target in ISO 8601 format. i.e.,
 *
 *         YYYY-MM-DD HH:MM:SS
 *
 * Usually requires 20 characters (including NUL).
 */
SFUNC size_t fio_time2iso(char *target, time_t time);

/** Adds two `struct timespec` objects. */
FIO_IFUNC struct timespec fio_time_add(struct timespec t, struct timespec t2);

/** Adds milliseconds to a `struct timespec` object. */
FIO_IFUNC struct timespec fio_time_add_milli(struct timespec t, int64_t milli);

/** Compares two `struct timespec` objects. */
FIO_IFUNC int fio_time_cmp(struct timespec t1, struct timespec t2);

/* *****************************************************************************
Time Inline Helpers
***************************************************************************** */

/** Returns human (watch) time... this value isn't as safe for measurements. */
FIO_IFUNC struct timespec fio_time_real(void) {
  struct timespec t = {0};
  (void)clock_gettime(CLOCK_REALTIME, &t);
  return t;
}

/** Returns monotonic time. */
FIO_IFUNC struct timespec fio_time_mono(void) {
  struct timespec t = {0};
  (void)clock_gettime(CLOCK_MONOTONIC, &t);
  return t;
}

/** Returns monotonic time in nano-seconds (now in 1 micro of a second). */
FIO_IFUNC int64_t fio_time_nano(void) {
  struct timespec t = fio_time_mono();
  return ((int64_t)t.tv_sec * 1000000000) + (int64_t)t.tv_nsec;
}

/** Returns monotonic time in micro-seconds (now in 1 millionth of a second). */
FIO_IFUNC int64_t fio_time_micro(void) {
  struct timespec t = fio_time_mono();
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
}

/** Returns monotonic time in milliseconds. */
FIO_IFUNC int64_t fio_time_milli(void) {
  return fio_time2milli(fio_time_mono());
}

/** Converts a `struct timespec` to milliseconds. */
FIO_IFUNC int64_t fio_time2milli(struct timespec t) {
  return ((int64_t)t.tv_sec * 1000) + (int64_t)t.tv_nsec / 1000000;
}

/** Converts a `struct timespec` to microseconds. */
FIO_IFUNC int64_t fio_time2micro(struct timespec t) {
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
}

/* Normalizes a timespec struct after an `add` or `sub` operation. */
FIO_IFUNC void fio_time___normalize(struct timespec *t) {
  /* Accept noncanonical inputs as well as the one-second carry from add. */
  const int64_t carry = (int64_t)t->tv_nsec / 1000000000LL;
  int64_t ns = (int64_t)t->tv_nsec % 1000000000LL;
  int64_t sec = (int64_t)t->tv_sec;
  int64_t adjustment = carry;
  if (ns < 0) {
    ns += 1000000000LL;
    --adjustment;
  }
  if (adjustment > 0 && sec > INT64_MAX - adjustment)
    sec = INT64_MAX;
  else if (adjustment < 0 && sec < INT64_MIN - adjustment)
    sec = INT64_MIN;
  else
    sec += adjustment;
  if (sizeof(time_t) < sizeof(int64_t)) {
    if (sec > INT32_MAX) {
      sec = INT32_MAX;
      ns = 999999999;
    } else if (sec < INT32_MIN) {
      sec = INT32_MIN;
      ns = 0;
    }
  }
  t->tv_sec = (time_t)sec;
  t->tv_nsec = (long)ns;
}

/** Adds to timespec. */
FIO_IFUNC struct timespec fio_time_add(struct timespec t, struct timespec t2) {
  /* Normalize each operand first to avoid overflow of long nanoseconds. */
  fio_time___normalize(&t);
  fio_time___normalize(&t2);
  int64_t a = (int64_t)t.tv_sec, b = (int64_t)t2.tv_sec;
  if (b > 0 && a > INT64_MAX - b)
    a = INT64_MAX;
  else if (b < 0 && a < INT64_MIN - b)
    a = INT64_MIN;
  else
    a += b;
  if (sizeof(time_t) < sizeof(int64_t)) {
    if (a > INT32_MAX) {
      a = INT32_MAX;
      t.tv_nsec = 999999999;
      t2.tv_nsec = 0;
    } else if (a < INT32_MIN) {
      a = INT32_MIN;
      t.tv_nsec = 0;
      t2.tv_nsec = 0;
    }
  }
  /* Both normalized nanoseconds fit in int64_t even on a 32-bit long. */
  const int64_t ns = (int64_t)t.tv_nsec + (int64_t)t2.tv_nsec;
  if (ns >= 1000000000LL) {
    if (a == INT64_MAX ||
        (sizeof(time_t) < sizeof(int64_t) && a == INT32_MAX)) {
      a = sizeof(time_t) < sizeof(int64_t) ? INT32_MAX : INT64_MAX;
      t.tv_nsec = 999999999;
    } else {
      ++a;
      t.tv_nsec = (long)(ns - 1000000000LL);
    }
  } else {
    t.tv_nsec = (long)ns;
  }
  t.tv_sec = (time_t)a;
  return t;
}

/** Adds milliseconds to timespec. */
FIO_IFUNC struct timespec fio_time_add_milli(struct timespec t, int64_t milli) {
  const int64_t seconds = milli / 1000;
  const long nanos = (long)((milli % 1000) * 1000000);
  /* A 32-bit time_t cannot hold the whole millisecond quotient. Saturate
   * rather than narrowing an extreme duration into an early deadline. */
  if (sizeof(time_t) < sizeof(int64_t) && seconds > INT32_MAX)
    return (struct timespec){.tv_sec = INT32_MAX, .tv_nsec = 999999999};
  if (sizeof(time_t) < sizeof(int64_t) && seconds < INT32_MIN)
    return (struct timespec){.tv_sec = INT32_MIN, .tv_nsec = 0};
  return fio_time_add(
      t,
      (struct timespec){.tv_sec = (time_t)seconds, .tv_nsec = nanos});
}

/** Compares two timespecs. */
FIO_IFUNC int fio_time_cmp(struct timespec t1, struct timespec t2) {
  if (t1.tv_sec != t2.tv_sec)
    return (t1.tv_sec > t2.tv_sec) - (t1.tv_sec < t2.tv_sec);
  return (t1.tv_nsec > t2.tv_nsec) - (t1.tv_nsec < t2.tv_nsec);
}

/* *****************************************************************************
Time Implementation
***************************************************************************** */
#if !defined(FIO_EXTERN) || defined(FIO_EXTERN_COMPLETE)

/**
 * A faster (yet less localized) alternative to `gmtime_r`.
 *
 * See the libc `gmtime_r` documentation for details.
 *
 * Falls back to `gmtime_r` for dates before epoch.
 */
SFUNC struct tm fio_time2gm(time_t timer) {
  struct tm tm;
  int64_t a, b;
#if (defined(HAVE_TM_TM_ZONE) && HAVE_TM_TM_ZONE) || defined(BSD)
  tm = (struct tm){
      .tm_isdst = 0,
      .tm_zone = (char *)"UTC",
  };
#else
  tm = (struct tm){
      .tm_isdst = 0,
  };
#endif

  // convert seconds from epoch to days from epoch + extract data
  if (timer >= 0) {
    // for seconds up to weekdays, we reduce the reminder every step.
    a = (int64_t)timer;
    b = a / 60; // b == time in minutes
    tm.tm_sec = (int)(a - (b * 60));
    a = b / 60; // b == time in hours
    tm.tm_min = (int)(b - (a * 60));
    b = a / 24; // b == time in days since epoch
    tm.tm_hour = (int)(a - (b * 24));
    // b == number of days since epoch
    // day of epoch was a thursday. Add + 4 so sunday == 0...
    tm.tm_wday = (int)((b + 4) % 7);
  } else {
    // for seconds up to weekdays, we reduce the reminder every step.
    a = (int64_t)timer;
    b = a / 60; // b == time in minutes
    if (b * 60 != a) {
      /* seconds passed */
      tm.tm_sec = (int)((a - (b * 60)) + 60);
      --b;
    } else {
      /* no seconds */
      tm.tm_sec = 0;
    }
    a = b / 60; // b == time in hours
    if (a * 60 != b) {
      /* minutes passed */
      tm.tm_min = (int)((b - (a * 60)) + 60);
      --a;
    } else {
      /* no minutes */
      tm.tm_min = 0;
    }
    b = a / 24; // b == time in days since epoch?
    if (b * 24 != a) {
      /* hours passed */
      tm.tm_hour = (int)((a - (b * 24)) + 24);
      --b;
    } else {
      /* no hours */
      tm.tm_hour = 0;
    }
    // day of epoch was a thursday. Add + 4 so sunday == 0...
    tm.tm_wday = (int)((b % 7 + 11) % 7);
    /* b == days from epoch */
  }

  // at this point we can apply the algorithm described here:
  // http://howardhinnant.github.io/date_algorithms.html#civil_from_days
  // Credit to Howard Hinnant.
  {
    b += 719468L; // adjust to March 1st, 2000 (post leap of 400 year era)
    // 146,097 = days in era (400 years)
    const int64_t era = (b >= 0 ? b : b - 146096) / 146097;
    const uint32_t doe = (uint32_t)(b - (era * 146097)); // day of era
    const uint16_t yoe =
        (uint16_t)((doe - doe / 1460 + doe / 36524 - doe / 146096) /
                   365); // year of era
    a = yoe;
    a += era * 400; // a == year number, assuming year starts on March 1st...
    const uint16_t doy = (uint16_t)(doe - (365 * yoe + yoe / 4 - yoe / 100));
    const uint16_t mp = (uint16_t)((5U * doy + 2) / 153);
    const uint16_t d = (uint16_t)(doy - (153U * mp + 2) / 5 + 1);
    const uint8_t m = (uint8_t)(mp + (mp < 10 ? 2 : -10));
    a += (m <= 1);
    /* struct tm cannot represent years beyond int; return a safe epoch
     * fallback instead of narrowing and risking invalid formatter indexes. */
    if (a - 1900 > INT_MAX || a - 1900 < INT_MIN)
      return fio_time2gm((time_t)0);
    tm.tm_year = (int)(a - 1900); // tm_year == years since 1900
    tm.tm_mon = m;
    tm.tm_mday = d;
    const uint8_t is_leap = (a % 4 == 0 && (a % 100 != 0 || a % 400 == 0));
    tm.tm_yday = (doy + (is_leap) + 28 + 31) % (365 + is_leap);
  }

  return tm;
}

/** Converts a `struct tm` to time in seconds (assuming UTC). */
SFUNC time_t fio_gm2time(struct tm tm) {
  int64_t time = 0;
  // we start with the algorithm described here:
  // http://howardhinnant.github.io/date_algorithms.html#days_from_civil
  // Credit to Howard Hinnant.
  {
    const int32_t y = (tm.tm_year + 1900) - (tm.tm_mon < 2);
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const uint16_t yoe = (y - era * 400L); // 0-399
    const uint32_t doy =
        (153L * (tm.tm_mon + (tm.tm_mon > 1 ? -2 : 10)) + 2) / 5 + tm.tm_mday -
        1;                                                       // 0-365
    const uint32_t doe = yoe * 365L + yoe / 4 - yoe / 100 + doy; // 0-146096
    time = era * 146097LL + doe - 719468LL; // time == days from epoch
  }

  /* Adjust for hour, minute and second */
  time = time * 24LL + tm.tm_hour;
  time = time * 60LL + tm.tm_min;
  time = time * 60LL + tm.tm_sec;

  time -= (int64_t)((60 * 60) & (0ULL - (tm.tm_isdst > 0)));
#if (defined(HAVE_TM_TM_ZONE) && HAVE_TM_TM_ZONE) || defined(BSD)
  time += tm.tm_gmtoff;
#endif
  return (time_t)time;
}

FIO_SFUNC char *fio_time_write_day(char *dest, const struct tm *tm) {
  static const char *FIO___DAY_NAMES[] =
      {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  dest[0] = FIO___DAY_NAMES[tm->tm_wday][0];
  dest[1] = FIO___DAY_NAMES[tm->tm_wday][1];
  dest[2] = FIO___DAY_NAMES[tm->tm_wday][2];
  return dest + 3;
}

FIO_SFUNC char *fio_time_write_month(char *dest, const struct tm *tm) {
  // clang-format off
  static const char *FIO___MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  // clang-format on
  dest[0] = FIO___MONTH_NAMES[tm->tm_mon][0];
  dest[1] = FIO___MONTH_NAMES[tm->tm_mon][1];
  dest[2] = FIO___MONTH_NAMES[tm->tm_mon][2];
  return dest + 3;
}

FIO_SFUNC char *fio_time_write_year(char *dest, const struct tm *tm) {
  int64_t year = (int64_t)tm->tm_year + 1900;
  const size_t digits = fio_digits10(year);
  fio_ltoa10(dest, year, digits);
  return dest + digits;
}

/** Writes an RFC 7231 date representation (HTTP date format) to target. */
SFUNC size_t fio_time2rfc7231(char *target, time_t time) {
  const struct tm tm = fio_time2gm(time);
  /* note: day of month is always 2 digits */
  char *pos = target;
  uint16_t tmp;
  pos = fio_time_write_day(pos, &tm);
  *pos++ = ',';
  *pos++ = ' ';
  tmp = tm.tm_mday / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_mday - (tmp * 10));
  *pos++ = ' ';
  pos = fio_time_write_month(pos, &tm);
  *pos++ = ' ';
  pos = fio_time_write_year(pos, &tm);
  *pos++ = ' ';
  tmp = tm.tm_hour / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_hour - (tmp * 10));
  *pos++ = ':';
  tmp = tm.tm_min / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_min - (tmp * 10));
  *pos++ = ':';
  tmp = tm.tm_sec / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_sec - (tmp * 10));
  *pos++ = ' ';
  *pos++ = 'G';
  *pos++ = 'M';
  *pos++ = 'T';
  *pos = 0;
  return pos - target;
}
/** Writes an RFC 2109 date representation to target. */
SFUNC size_t fio_time2rfc2109(char *target, time_t time) {
  const struct tm tm = fio_time2gm(time);
  /* note: day of month is always 2 digits */
  char *pos = target;
  uint16_t tmp;
  pos = fio_time_write_day(pos, &tm);
  *pos++ = ',';
  *pos++ = ' ';
  tmp = tm.tm_mday / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_mday - (tmp * 10));
  *pos++ = ' ';
  pos = fio_time_write_month(pos, &tm);
  *pos++ = ' ';
  pos = fio_time_write_year(pos, &tm);
  *pos++ = ' ';
  tmp = tm.tm_hour / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_hour - (tmp * 10));
  *pos++ = ':';
  tmp = tm.tm_min / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_min - (tmp * 10));
  *pos++ = ':';
  tmp = tm.tm_sec / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_sec - (tmp * 10));
  *pos++ = ' ';
  *pos++ = '-';
  *pos++ = '0';
  *pos++ = '0';
  *pos++ = '0';
  *pos++ = '0';
  *pos = 0;
  return pos - target;
}

/** Writes an RFC 2822 date representation to target. */
SFUNC size_t fio_time2rfc2822(char *target, time_t time) {
  const struct tm tm = fio_time2gm(time);
  /* note: day of month is either 1 or 2 digits */
  char *pos = target;
  uint16_t tmp;
  pos = fio_time_write_day(pos, &tm);
  *pos++ = ',';
  *pos++ = ' ';
  if (tm.tm_mday < 10) {
    *pos++ = '0' + (uint8_t)tm.tm_mday;
  } else {
    tmp = tm.tm_mday / 10;
    *pos++ = '0' + (uint8_t)tmp;
    *pos++ = '0' + (uint8_t)(tm.tm_mday - (tmp * 10));
  }
  *pos++ = '-';
  pos = fio_time_write_month(pos, &tm);
  *pos++ = '-';
  pos = fio_time_write_year(pos, &tm);
  *pos++ = ' ';
  tmp = tm.tm_hour / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_hour - (tmp * 10));
  *pos++ = ':';
  tmp = tm.tm_min / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_min - (tmp * 10));
  *pos++ = ':';
  tmp = tm.tm_sec / 10;
  *pos++ = '0' + (uint8_t)tmp;
  *pos++ = '0' + (uint8_t)(tm.tm_sec - (tmp * 10));
  *pos++ = ' ';
  *pos++ = 'G';
  *pos++ = 'M';
  *pos++ = 'T';
  *pos = 0;
  return pos - target;
}

/**
 * Writes a date representation to target in common log format. i.e.,
 *
 *         [DD/MMM/yyyy:hh:mm:ss +0000]
 *
 * Usually requires 29 characters (including square brackets and NUL).
 */
SFUNC size_t fio_time2log(char *target, time_t time) {
  {
    const struct tm tm = fio_time2gm(time);
    /* note: day of month is either 1 or 2 digits */
    char *pos = target;
    uint16_t tmp;
    *pos++ = '[';
    tmp = tm.tm_mday / 10;
    *pos++ = '0' + (uint8_t)tmp;
    *pos++ = '0' + (uint8_t)(tm.tm_mday - (tmp * 10));
    *pos++ = '/';
    pos = fio_time_write_month(pos, &tm);
    *pos++ = '/';
    pos = fio_time_write_year(pos, &tm);
    *pos++ = ':';
    tmp = tm.tm_hour / 10;
    *pos++ = '0' + (uint8_t)tmp;
    *pos++ = '0' + (uint8_t)(tm.tm_hour - (tmp * 10));
    *pos++ = ':';
    tmp = tm.tm_min / 10;
    *pos++ = '0' + (uint8_t)tmp;
    *pos++ = '0' + (uint8_t)(tm.tm_min - (tmp * 10));
    *pos++ = ':';
    tmp = tm.tm_sec / 10;
    *pos++ = '0' + (uint8_t)tmp;
    *pos++ = '0' + (uint8_t)(tm.tm_sec - (tmp * 10));
    *pos++ = ' ';
    *pos++ = '+';
    *pos++ = '0';
    *pos++ = '0';
    *pos++ = '0';
    *pos++ = '0';
    *pos++ = ']';
    *(pos) = 0;
    return pos - target;
  }
}

/**
 * Writes a date representation to target in ISO 8601 format. i.e.,
 *
 *         YYYY-MM-DD HH:MM:SS
 *
 * Usually requires 20 characters (including NUL).
 */
SFUNC size_t fio_time2iso(char *target, time_t time) {
  {
    const struct tm tm = fio_time2gm(time);
    /* note: day of month is either 1 or 2 digits */
    char *pos = target;
    uint16_t tmp;
    pos = fio_time_write_year(pos, &tm);
    *pos++ = '-';
    pos = fio_time_write_month(pos, &tm);
    *pos++ = '-';
    tmp = tm.tm_mday / 10;
    *pos++ = '0' + tmp;
    *pos++ = '0' + (tm.tm_mday - (tmp * 10));
    *pos++ = ' ';
    tmp = tm.tm_hour / 10;
    *pos++ = '0' + tmp;
    *pos++ = '0' + (tm.tm_hour - (tmp * 10));
    *pos++ = ':';
    tmp = tm.tm_min / 10;
    *pos++ = '0' + tmp;
    *pos++ = '0' + (tm.tm_min - (tmp * 10));
    *pos++ = ':';
    tmp = tm.tm_sec / 10;
    *pos++ = '0' + tmp;
    *pos++ = '0' + (tm.tm_sec - (tmp * 10));
    *(pos) = 0;
    return pos - target;
  }
}
/* *****************************************************************************
Time Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_TIME
#endif /* FIO_TIME */
