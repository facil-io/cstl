/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_TIME
#include FIO_INCLUDE_FILE

#define FIO___GMTIME_TEST_INTERVAL ((60LL * 60 * 23) + 1027) /* 23:17:07 */
#if 1 || FIO_OS_WIN
#define FIO___GMTIME_TEST_RANGE (1001LL * 376) /* test 0.5 millenia */
#else
#define FIO___GMTIME_TEST_RANGE (3003LL * 376) /* test ~3  millenia */
#endif

#if FIO_OS_WIN && !defined(gmtime_r)
FIO_IFUNC struct tm *gmtime_r(const time_t *timep, struct tm *result) {
  struct tm *t = gmtime(timep);
  if (t && result)
    *result = *t;
  return result;
}
#endif

int main(void) {
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
    FIO_MEMSET(buf, 'X', 47);
    fio_time2rfc2109(buf, now);
    FIO_MEMSET(buf, 'X', 47);
    fio_time2rfc2822(buf, now);
    FIO_MEMSET(buf, 'X', 47);
    fio_time2log(buf, now);
  }
  {
    struct timespec zero = {0, 0}, ten = {10, 0};
    struct timespec result = fio_time_add(zero, (struct timespec){0, 1000000001L});
    FIO_ASSERT(result.tv_sec == 1 && result.tv_nsec == 1,
               "nanosecond carry must normalize");
    result = fio_time_add(ten, (struct timespec){0, -1});
    FIO_ASSERT(result.tv_sec == 9 && result.tv_nsec == 999999999L,
               "nanosecond borrow must normalize");
    result = fio_time_add_milli(zero, 30000);
    FIO_ASSERT(result.tv_sec == 30 && result.tv_nsec == 0,
               "milliseconds must use base 1000");
    result = fio_time_add_milli(zero, -1);
    FIO_ASSERT(result.tv_sec == -1 && result.tv_nsec == 999000000L,
               "negative milliseconds must normalize");
    FIO_ASSERT(fio_time_cmp((struct timespec){0, 100},
                            (struct timespec){1, 0}) < 0,
               "comparison must prioritize seconds");
  }
  if (sizeof(time_t) > sizeof(ssize_t)) {
    struct tm future = fio_time2gm((time_t)2147483648LL);
    FIO_ASSERT(future.tm_year == 138 && future.tm_mon == 0 &&
                   future.tm_mday == 19 && future.tm_wday == 2,
               "2038 must not narrow through ssize_t");
  }
}
#undef FIO___GMTIME_TEST_INTERVAL
#undef FIO___GMTIME_TEST_RANGE
