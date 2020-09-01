/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                                  Time Helpers










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
FIO_IFUNC uint64_t fio_time_nano();

/** Returns monotonic time in micro-seconds (now in 1 millionth of a second). */
FIO_IFUNC uint64_t fio_time_micro();

/** Returns monotonic time in milliseconds. */
FIO_IFUNC uint64_t fio_time_milli();

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

/* *****************************************************************************
Patch for OSX version < 10.12 from https://stackoverflow.com/a/9781275/4025095
***************************************************************************** */
#if defined(__MACH__) && !defined(CLOCK_REALTIME)
#include <sys/time.h>
#define CLOCK_REALTIME 0
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif
#define clock_gettime fio___patch_clock_gettime
// clock_gettime is not implemented on older versions of OS X (< 10.12).
// If implemented, CLOCK_MONOTONIC will have already been defined.
FIO_IFUNC int fio___patch_clock_gettime(int clk_id, struct timespec *t) {
  struct timeval now;
  int rv = gettimeofday(&now, NULL);
  if (rv)
    return rv;
  t->tv_sec = now.tv_sec;
  t->tv_nsec = now.tv_usec * 1000;
  return 0;
  (void)clk_id;
}
#warning fio_time functions defined using gettimeofday patch.
#endif

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
FIO_IFUNC uint64_t fio_time_nano() {
  struct timespec t = fio_time_mono();
  return ((uint64_t)t.tv_sec * 1000000000) + (uint64_t)t.tv_nsec;
}

/** Returns monotonic time in micro-seconds (now in 1 millionth of a second). */
FIO_IFUNC uint64_t fio_time_micro() {
  struct timespec t = fio_time_mono();
  return ((uint64_t)t.tv_sec * 1000000) + (uint64_t)t.tv_nsec / 1000;
}

/** Returns monotonic time in milliseconds. */
FIO_IFUNC uint64_t fio_time_milli() {
  struct timespec t = fio_time_mono();
  return ((uint64_t)t.tv_sec * 1000) + (uint64_t)t.tv_nsec / 1000000;
}

/* *****************************************************************************
Time Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE)

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
    const uint16_t yoe = (uint16_t)(
        (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365); // year of era
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
  time_t time = 0;
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
    time = era * 146097L + doe - 719468L; // time == days from epoch
  }

  /* Adjust for hour, minute and second */
  time = time * 24L + tm.tm_hour;
  time = time * 60L + tm.tm_min;
  time = time * 60L + tm.tm_sec;

  if (tm.tm_isdst > 0) {
    time -= 60 * 60;
  }
#if HAVE_TM_TM_ZONE || defined(BSD)
  if (tm.tm_gmtoff) {
    time += tm.tm_gmtoff;
  }
#endif
  return time;
}

static const char *FIO___DAY_NAMES[] =
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
// clang-format off
static const char *FIO___MONTH_NAMES[] =
    {"Jan ", "Feb ", "Mar ", "Apr ", "May ", "Jun ",
     "Jul ", "Aug ", "Sep ", "Oct ", "Nov ", "Dec "};
// clang-format on
static const char *FIO___GMT_STR = "GMT";

/** Writes an RFC 7231 date representation (HTTP date format) to target. */
SFUNC size_t fio_time2rfc7231(char *target, time_t time) {
  const struct tm tm = fio_time2gm(time);
  /* note: day of month is always 2 digits */
  char *pos = target;
  uint16_t tmp;
  pos[0] = FIO___DAY_NAMES[tm.tm_wday][0];
  pos[1] = FIO___DAY_NAMES[tm.tm_wday][1];
  pos[2] = FIO___DAY_NAMES[tm.tm_wday][2];
  pos[3] = ',';
  pos[4] = ' ';
  pos += 5;
  tmp = tm.tm_mday / 10;
  pos[0] = '0' + tmp;
  pos[1] = '0' + (tm.tm_mday - (tmp * 10));
  pos += 2;
  *(pos++) = ' ';
  pos[0] = FIO___MONTH_NAMES[tm.tm_mon][0];
  pos[1] = FIO___MONTH_NAMES[tm.tm_mon][1];
  pos[2] = FIO___MONTH_NAMES[tm.tm_mon][2];
  pos[3] = ' ';
  pos += 4;
  // write year.
  pos += fio_ltoa(pos, tm.tm_year + 1900, 10);
  *(pos++) = ' ';
  tmp = tm.tm_hour / 10;
  pos[0] = '0' + tmp;
  pos[1] = '0' + (tm.tm_hour - (tmp * 10));
  pos[2] = ':';
  tmp = tm.tm_min / 10;
  pos[3] = '0' + tmp;
  pos[4] = '0' + (tm.tm_min - (tmp * 10));
  pos[5] = ':';
  tmp = tm.tm_sec / 10;
  pos[6] = '0' + tmp;
  pos[7] = '0' + (tm.tm_sec - (tmp * 10));
  pos += 8;
  pos[0] = ' ';
  pos[1] = FIO___GMT_STR[0];
  pos[2] = FIO___GMT_STR[1];
  pos[3] = FIO___GMT_STR[2];
  pos[4] = 0;
  pos += 4;
  return pos - target;
}
/** Writes an RFC 2109 date representation to target. */
SFUNC size_t fio_time2rfc2109(char *target, time_t time) {
  const struct tm tm = fio_time2gm(time);
  /* note: day of month is always 2 digits */
  char *pos = target;
  uint16_t tmp;
  pos[0] = FIO___DAY_NAMES[tm.tm_wday][0];
  pos[1] = FIO___DAY_NAMES[tm.tm_wday][1];
  pos[2] = FIO___DAY_NAMES[tm.tm_wday][2];
  pos[3] = ',';
  pos[4] = ' ';
  pos += 5;
  tmp = tm.tm_mday / 10;
  pos[0] = '0' + tmp;
  pos[1] = '0' + (tm.tm_mday - (tmp * 10));
  pos += 2;
  *(pos++) = ' ';
  pos[0] = FIO___MONTH_NAMES[tm.tm_mon][0];
  pos[1] = FIO___MONTH_NAMES[tm.tm_mon][1];
  pos[2] = FIO___MONTH_NAMES[tm.tm_mon][2];
  pos[3] = ' ';
  pos += 4;
  // write year.
  pos += fio_ltoa(pos, tm.tm_year + 1900, 10);
  *(pos++) = ' ';
  tmp = tm.tm_hour / 10;
  pos[0] = '0' + tmp;
  pos[1] = '0' + (tm.tm_hour - (tmp * 10));
  pos[2] = ':';
  tmp = tm.tm_min / 10;
  pos[3] = '0' + tmp;
  pos[4] = '0' + (tm.tm_min - (tmp * 10));
  pos[5] = ':';
  tmp = tm.tm_sec / 10;
  pos[6] = '0' + tmp;
  pos[7] = '0' + (tm.tm_sec - (tmp * 10));
  pos += 8;
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
  pos[0] = FIO___DAY_NAMES[tm.tm_wday][0];
  pos[1] = FIO___DAY_NAMES[tm.tm_wday][1];
  pos[2] = FIO___DAY_NAMES[tm.tm_wday][2];
  pos[3] = ',';
  pos[4] = ' ';
  pos += 5;
  if (tm.tm_mday < 10) {
    *pos = '0' + tm.tm_mday;
    ++pos;
  } else {
    tmp = tm.tm_mday / 10;
    pos[0] = '0' + tmp;
    pos[1] = '0' + (tm.tm_mday - (tmp * 10));
    pos += 2;
  }
  *(pos++) = '-';
  pos[0] = FIO___MONTH_NAMES[tm.tm_mon][0];
  pos[1] = FIO___MONTH_NAMES[tm.tm_mon][1];
  pos[2] = FIO___MONTH_NAMES[tm.tm_mon][2];
  pos += 3;
  *(pos++) = '-';
  // write year.
  pos += fio_ltoa(pos, tm.tm_year + 1900, 10);
  *(pos++) = ' ';
  tmp = tm.tm_hour / 10;
  pos[0] = '0' + tmp;
  pos[1] = '0' + (tm.tm_hour - (tmp * 10));
  pos[2] = ':';
  tmp = tm.tm_min / 10;
  pos[3] = '0' + tmp;
  pos[4] = '0' + (tm.tm_min - (tmp * 10));
  pos[5] = ':';
  tmp = tm.tm_sec / 10;
  pos[6] = '0' + tmp;
  pos[7] = '0' + (tm.tm_sec - (tmp * 10));
  pos += 8;
  pos[0] = ' ';
  pos[1] = FIO___GMT_STR[0];
  pos[2] = FIO___GMT_STR[1];
  pos[3] = FIO___GMT_STR[2];
  pos[4] = 0;
  pos += 4;
  return pos - target;
}

/* *****************************************************************************
Time - test
***************************************************************************** */
#ifdef FIO_TEST_CSTL

#define FIO___GMTIME_TEST_INTERVAL ((60L * 60 * 24) - 7) /* 1day - 7seconds */
#define FIO___GMTIME_TEST_RANGE    (4093L * 365) /* test ~4 millenium  */

FIO_SFUNC void FIO_NAME_TEST(stl, time)(void) {
  fprintf(stderr, "* Testing facil.io fio_time2gm vs gmtime_r\n");
  struct tm tm1, tm2;
  const time_t now = fio_time_real().tv_sec;
  const time_t end =
      now + (FIO___GMTIME_TEST_RANGE * FIO___GMTIME_TEST_INTERVAL);
  time_t t = now - (FIO___GMTIME_TEST_RANGE * FIO___GMTIME_TEST_INTERVAL);
  while (t < end) {
    time_t tmp = t;
    t += FIO___GMTIME_TEST_INTERVAL;
    tm2 = fio_time2gm(tmp);
    FIO_ASSERT(fio_gm2time(tm2) == tmp,
               "fio_gm2time roundtrip error (%ld != %ld)",
               (long)fio_gm2time(tm2),
               (long)tmp);
    gmtime_r(&tmp, &tm1);
    if (tm1.tm_year != tm2.tm_year || tm1.tm_mon != tm2.tm_mon ||
        tm1.tm_mday != tm2.tm_mday || tm1.tm_yday != tm2.tm_yday ||
        tm1.tm_hour != tm2.tm_hour || tm1.tm_min != tm2.tm_min ||
        tm1.tm_sec != tm2.tm_sec || tm1.tm_wday != tm2.tm_wday) {
      char buf[256];
      fio_time2rfc7231(buf, tmp);
      FIO_ASSERT(0,
                 "system gmtime_r != fio_time2gm for %ld!\n"
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
                 (long)t,
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
  }
  {
    uint64_t start, stop;
#if DEBUG
    fprintf(stderr, "PERFOMEANCE TESTS IN DEBUG MODE ARE BIASED\n");
#endif
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      volatile struct tm tm = fio_time2gm(now);
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
      (void)t_tmp;
    }
    stop = fio_time_micro();
    fprintf(stderr,
            "\t- mktime speed test took:    \t%zuus\n",
            (size_t)(stop - start));
    fprintf(stderr, "\n");
  }
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
