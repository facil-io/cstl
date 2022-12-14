/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___   /* Development inclusion - ignore line */
#define FIO_TIME       /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
/* *****************************************************************************




                                  Time Helpers



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_TIME) && !defined(H___FIO_TIME___H)
#define H___FIO_TIME___H

/* *****************************************************************************
Collecting Monotonic / Real Time
***************************************************************************** */

/** Returns human (watch) time... this value isn't as safe for measurements. */
FIO_IFUNC struct timespec fio_time_real();

/** Returns monotonic time. */
FIO_IFUNC struct timespec fio_time_mono();

/** Returns monotonic time in nano-seconds (now in 1 billionth of a second). */
FIO_IFUNC int64_t fio_time_nano();

/** Returns monotonic time in micro-seconds (now in 1 millionth of a second). */
FIO_IFUNC int64_t fio_time_micro();

/** Returns monotonic time in milliseconds. */
FIO_IFUNC int64_t fio_time_milli();

/** Converts a `struct timespec` to milliseconds. */
FIO_IFUNC int64_t fio_time2milli(struct timespec);

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
 * Writes an RFC 2109 date representation to target.
 *
 * Usually requires 31 characters, although this may vary.
 */
SFUNC size_t fio_time2rfc2109(char *target, time_t time);

/**
 * Writes an RFC 2822 date representation to target.
 *
 * Usually requires 28 to 29 characters, although this may vary.
 */
SFUNC size_t fio_time2rfc2822(char *target, time_t time);

/**
 * Writes a date representation to target in common log format. i.e.,
 *
 *         [DD/MMM/yyyy:hh:mm:ss +0000]
 *
 * Usually requires 29 characters (includiing square brackes and NUL).
 */
SFUNC size_t fio_time2log(char *target, time_t time);

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
FIO_IFUNC struct timespec fio_time_real() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return t;
}

/** Returns monotonic time. */
FIO_IFUNC struct timespec fio_time_mono() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t;
}

/** Returns monotonic time in nano-seconds (now in 1 micro of a second). */
FIO_IFUNC int64_t fio_time_nano() {
  struct timespec t = fio_time_mono();
  return ((int64_t)t.tv_sec * 1000000000) + (int64_t)t.tv_nsec;
}

/** Returns monotonic time in micro-seconds (now in 1 millionth of a second). */
FIO_IFUNC int64_t fio_time_micro() {
  struct timespec t = fio_time_mono();
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
}

/** Returns monotonic time in milliseconds. */
FIO_IFUNC int64_t fio_time_milli() {
  struct timespec t = fio_time_mono();
  return ((int64_t)t.tv_sec * 1000) + (int64_t)t.tv_nsec / 1000000;
}

/** Converts a `struct timespec` to milliseconds. */
FIO_IFUNC int64_t fio_time2milli(struct timespec t) {
  return ((int64_t)t.tv_sec * 1000) + (int64_t)t.tv_nsec / 1000000;
}

/* Normalizes a timespec struct after an `add` or `sub` operation. */
FIO_IFUNC void fio_time___normalize(struct timespec *t) {
  const long ns_norm[2] = {0, 1000000000LL};
  t->tv_nsec += ns_norm[(t->tv_nsec < 0)];
  t->tv_sec += (t->tv_nsec < 0);
  t->tv_nsec -= ns_norm[(1000000000LL < t->tv_nsec)];
  t->tv_sec += (1000000000LL < t->tv_nsec);
}

/** Adds to timespec. */
FIO_IFUNC struct timespec fio_time_add(struct timespec t, struct timespec t2) {
  t.tv_sec += t2.tv_sec;
  t.tv_nsec += t2.tv_nsec;
  fio_time___normalize(&t);
  return t;
}

/** Adds milliseconds to timespec. */
FIO_IFUNC struct timespec fio_time_add_milli(struct timespec t, int64_t milli) {
  t.tv_sec += milli >> 10; /* 1024 is close enough, will be normalized */
  t.tv_nsec += (milli & 1023) * 1000000;
  fio_time___normalize(&t);
  return t;
}

/** Compares two timespecs. */
FIO_IFUNC int fio_time_cmp(struct timespec t1, struct timespec t2) {
  size_t a = (t2.tv_sec < t1.tv_sec) << 1;
  a |= (t2.tv_nsec < t1.tv_nsec);
  size_t b = (t1.tv_sec < t2.tv_sec) << 1;
  b |= (t1.tv_nsec < t2.tv_nsec);
  return (0 - (a < b)) + (b < a);
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
  ssize_t a, b;
#if HAVE_TM_TM_ZONE || defined(BSD)
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
    a = (ssize_t)timer;
    b = a / 60; // b == time in minutes
    tm.tm_sec = (int)(a - (b * 60));
    a = b / 60; // b == time in hours
    tm.tm_min = (int)(b - (a * 60));
    b = a / 24; // b == time in days since epoch
    tm.tm_hour = (int)(a - (b * 24));
    // b == number of days since epoch
    // day of epoch was a thursday. Add + 4 so sunday == 0...
    tm.tm_wday = (b + 4) % 7;
  } else {
    // for seconds up to weekdays, we reduce the reminder every step.
    a = (ssize_t)timer;
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
    tm.tm_wday = ((b - 3) % 7);
    if (tm.tm_wday)
      tm.tm_wday += 7;
    /* b == days from epoch */
  }

  // at this point we can apply the algorithm described here:
  // http://howardhinnant.github.io/date_algorithms.html#civil_from_days
  // Credit to Howard Hinnant.
  {
    b += 719468L; // adjust to March 1st, 2000 (post leap of 400 year era)
    // 146,097 = days in era (400 years)
    const size_t era = (b >= 0 ? b : b - 146096) / 146097;
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

  if (tm.tm_isdst > 0) {
    time -= 60 * 60;
  }
#if HAVE_TM_TM_ZONE || defined(BSD)
  if (tm.tm_gmtoff) {
    time += tm.tm_gmtoff;
  }
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
  int64_t year = tm->tm_year + 1900;
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
  *pos++ = '0' + tmp;
  *pos++ = '0' + (tm.tm_mday - (tmp * 10));
  *pos++ = ' ';
  pos = fio_time_write_month(pos, &tm);
  *pos++ = ' ';
  pos = fio_time_write_year(pos, &tm);
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
  *pos++ = '0' + tmp;
  *pos++ = '0' + (tm.tm_mday - (tmp * 10));
  *pos++ = ' ';
  pos = fio_time_write_month(pos, &tm);
  *pos++ = ' ';
  pos = fio_time_write_year(pos, &tm);
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
    *pos++ = '0' + tm.tm_mday;
  } else {
    tmp = tm.tm_mday / 10;
    *pos++ = '0' + tmp;
    *pos++ = '0' + (tm.tm_mday - (tmp * 10));
  }
  *pos++ = '-';
  pos = fio_time_write_month(pos, &tm);
  *pos++ = '-';
  pos = fio_time_write_year(pos, &tm);
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
 * Usually requires 29 characters (includiing square brackes and NUL).
 */
SFUNC size_t fio_time2log(char *target, time_t time) {
  {
    const struct tm tm = fio_time2gm(time);
    /* note: day of month is either 1 or 2 digits */
    char *pos = target;
    uint16_t tmp;
    *pos++ = '[';
    tmp = tm.tm_mday / 10;
    *pos++ = '0' + tmp;
    *pos++ = '0' + (tm.tm_mday - (tmp * 10));
    *pos++ = '/';
    pos = fio_time_write_month(pos, &tm);
    *pos++ = '/';
    pos = fio_time_write_year(pos, &tm);
    *pos++ = ':';
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

/* *****************************************************************************
Time - test
***************************************************************************** */
#ifdef FIO_TEST_CSTL

#define FIO___GMTIME_TEST_INTERVAL ((60LL * 60 * 23) + 1027) /* 23:17:07 */
#if 1 || FIO_OS_WIN
#define FIO___GMTIME_TEST_RANGE (1001LL * 376) /* test 0.5 millenia */
#else
#define FIO___GMTIME_TEST_RANGE (3003LL * 376) /* test ~3  millenia */
#endif

FIO_SFUNC void FIO_NAME_TEST(stl, time)(void) {
  fprintf(stderr, "* Testing facil.io fio_time2gm vs gmtime_r\n");
  struct tm tm1 = {0}, tm2 = {0};
  const time_t now = fio_time_real().tv_sec;
#if FIO_OS_WIN
  const time_t end = (FIO___GMTIME_TEST_RANGE * FIO___GMTIME_TEST_INTERVAL);
  time_t t = 1; /* Windows fails on some date ranges. */
#else
  const time_t end =
      now + (FIO___GMTIME_TEST_RANGE * FIO___GMTIME_TEST_INTERVAL);
  time_t t = now - (FIO___GMTIME_TEST_RANGE * FIO___GMTIME_TEST_INTERVAL);
#endif
  FIO_ASSERT(t < end, "time testing range overflowed.");
  do {
    time_t tmp = t;
    t += FIO___GMTIME_TEST_INTERVAL;
    tm2 = fio_time2gm(tmp);
    FIO_ASSERT(fio_gm2time(tm2) == tmp,
               "fio_gm2time roundtrip error (%zu != %zu)",
               (size_t)fio_gm2time(tm2),
               (size_t)tmp);
    gmtime_r(&tmp, &tm1);
    if (tm1.tm_year != tm2.tm_year || tm1.tm_mon != tm2.tm_mon ||
        tm1.tm_mday != tm2.tm_mday || tm1.tm_yday != tm2.tm_yday ||
        tm1.tm_hour != tm2.tm_hour || tm1.tm_min != tm2.tm_min ||
        tm1.tm_sec != tm2.tm_sec || tm1.tm_wday != tm2.tm_wday) {
      char buf[256];
      FIO_LOG_ERROR("system gmtime_r != fio_time2gm for %ld!\n", (long)t);
      fio_time2rfc7231(buf, tmp);
      FIO_ASSERT(0,
                 "\n"
                 "-- System:\n"
                 "\ttm_year: %d\n"
                 "\ttm_mon: %d\n"
                 "\ttm_mday: %d\n"
                 "\ttm_yday: %d\n"
                 "\ttm_hour: %d\n"
                 "\ttm_min: %d\n"
                 "\ttm_sec: %d\n"
                 "\ttm_wday: %d\n"
                 "-- facil.io:\n"
                 "\ttm_year: %d\n"
                 "\ttm_mon: %d\n"
                 "\ttm_mday: %d\n"
                 "\ttm_yday: %d\n"
                 "\ttm_hour: %d\n"
                 "\ttm_min: %d\n"
                 "\ttm_sec: %d\n"
                 "\ttm_wday: %d\n"
                 "-- As String:\n"
                 "\t%s",
                 tm1.tm_year,
                 tm1.tm_mon,
                 tm1.tm_mday,
                 tm1.tm_yday,
                 tm1.tm_hour,
                 tm1.tm_min,
                 tm1.tm_sec,
                 tm1.tm_wday,
                 tm2.tm_year,
                 tm2.tm_mon,
                 tm2.tm_mday,
                 tm2.tm_yday,
                 tm2.tm_hour,
                 tm2.tm_min,
                 tm2.tm_sec,
                 tm2.tm_wday,
                 buf);
    }
  } while (t < end);
  {
    char buf[48];
    buf[47] = 0;
    FIO_MEMSET(buf, 'X', 47);
    fio_time2rfc7231(buf, now);
    FIO_LOG_DEBUG2("fio_time2rfc7231:   %s", buf);
    FIO_MEMSET(buf, 'X', 47);
    fio_time2rfc2109(buf, now);
    FIO_LOG_DEBUG2("fio_time2rfc2109:   %s", buf);
    FIO_MEMSET(buf, 'X', 47);
    fio_time2rfc2822(buf, now);
    FIO_LOG_DEBUG2("fio_time2rfc2822:   %s", buf);
    FIO_MEMSET(buf, 'X', 47);
    fio_time2log(buf, now);
    FIO_LOG_DEBUG2("fio_time2log:       %s", buf);
  }
  {
    uint64_t start, stop;
#if DEBUG
    fprintf(stderr, "PERFOMEANCE TESTS IN DEBUG MODE ARE BIASED\n");
#endif
    fprintf(stderr, "  Performance testing fio_time2gm vs gmtime_r\n");
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      volatile struct tm tm = fio_time2gm(now);
      FIO_COMPILER_GUARD;
      (void)tm;
    }
    stop = fio_time_micro();
    fprintf(stderr,
            "\t- fio_time2gm speed test took:\t%zuus\n",
            (size_t)(stop - start));
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      volatile struct tm tm;
      time_t tmp = now;
      gmtime_r(&tmp, (struct tm *)&tm);
      FIO_COMPILER_GUARD;
    }
    stop = fio_time_micro();
    fprintf(stderr,
            "\t- gmtime_r speed test took:  \t%zuus\n",
            (size_t)(stop - start));
    fprintf(stderr, "\n");
    struct tm tm_now = fio_time2gm(now);
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      tm_now = fio_time2gm(now + i);
      time_t t_tmp = fio_gm2time(tm_now);
      FIO_COMPILER_GUARD;
      (void)t_tmp;
    }
    stop = fio_time_micro();
    fprintf(stderr,
            "\t- fio_gm2time speed test took:\t%zuus\n",
            (size_t)(stop - start));
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      tm_now = fio_time2gm(now + i);
      volatile time_t t_tmp = mktime((struct tm *)&tm_now);
      FIO_COMPILER_GUARD;
      (void)t_tmp;
    }
    stop = fio_time_micro();
    fprintf(stderr,
            "\t- mktime speed test took:    \t%zuus\n",
            (size_t)(stop - start));
    fprintf(stderr, "\n");
  }
  /* TODO: test fio_time_add, fio_time_add_milli, and fio_time_cmp */
}
#undef FIO___GMTIME_TEST_INTERVAL
#undef FIO___GMTIME_TEST_RANGE
#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
Time Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_TIME
#endif /* FIO_TIME */
