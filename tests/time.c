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
    uint64_t start, stop;
#if DEBUG
    fprintf(stderr, "\tPERFOMEANCE TESTS IN DEBUG MODE ARE BIASED\n");
#endif
    fprintf(stderr, "\tPerformance testing fio_time2gm vs gmtime_r\n");
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      volatile struct tm tm = fio_time2gm(now);
      FIO_COMPILER_GUARD;
      (void)tm;
    }
    stop = fio_time_micro();
    fprintf(stderr,
            "\t\t- fio_time2gm speed test took:\t%zuus\n",
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
            "\t\t- gmtime_r speed test took:  \t%zuus\n",
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
            "\t\t- fio_gm2time speed test took:\t%zuus\n",
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
            "\t\t- mktime speed test took:    \t%zuus\n",
            (size_t)(stop - start));
    fprintf(stderr, "\n");
  }
  /* TODO: test fio_time_add, fio_time_add_milli, and fio_time_cmp */
}
#undef FIO___GMTIME_TEST_INTERVAL
#undef FIO___GMTIME_TEST_RANGE
