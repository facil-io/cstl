/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_GLOB_MATCH Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_GLOB_MATCH_TEST___H)
#define H___FIO_GLOB_MATCH_TEST___H

#ifndef H___FIO_GLOB_MATCH___H
#define FIO_GLOB_MATCH
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

FIO_SFUNC void FIO_NAME_TEST(stl, glob_matching)(void) {
  struct {
    char *pat;
    char *str;
    uint8_t expect;
  } t[] = {
      // clang-format off
      /* test empty string */
      {.pat = (char *)"", .str = (char *)"", .expect = 1},
      /* test exact match */
      {.pat = (char *)"a", .str = (char *)"a", .expect = 1},
      /* test empty pattern */
      {.pat = (char *)"", .str = (char *)"a", .expect = 0},
      /* test longer pattern */
      {.pat = (char *)"a", .str = (char *)"", .expect = 0},
      /* test empty string with glob pattern */
      {.pat = (char *)"*", .str = (char *)"", .expect = 1},
      /* test glob pattern */
      {.pat = (char *)"*", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern at end */
      {.pat = (char *)"W*", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern as bookends */
      {.pat = (char *)"*Whatever*", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern in the middle */
      {.pat = (char *)"W*er", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern in the middle - empty match*/
      {.pat = (char *)"W*hatever", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern in the middle  - no match */
      {.pat = (char *)"W*htever", .str = (char *)"Whatever", .expect = 0},
      /* test partial match with glob at end */
      {.pat = (char *)"h*", .str = (char *)"Whatever", .expect = 0},
      /* test partial match with glob in the middle */
      {.pat = (char *)"h*er", .str = (char *)"Whatever", .expect = 0},
      /* test glob match with "?"  */
      {.pat = (char *)"?h*er", .str = (char *)"Whatever", .expect = 1},
      /* test "?" for length restrictions */
      {.pat = (char *)"?", .str = (char *)"Whatever", .expect = 0},
      /* test ? in the middle */
      {.pat = (char *)"What?ver", .str = (char *)"Whatever", .expect = 1},
      /* test letter list */
      {.pat = (char *)"[ASW]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test letter range */
      {.pat = (char *)"[A-Z]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test letter range (fail) */
      {.pat = (char *)"[a-z]hat?ver", .str = (char *)"Whatever", .expect = 0},
      /* test inverted letter range */
      {.pat = (char *)"[!a-z]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test inverted list */
      {.pat = (char *)"[!F]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test escaped range */
      {.pat = (char *)"[!a-z\\]]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test "?" after range (no skip) */
      {.pat = (char *)"[A-Z]?at?ver", .str = (char *)"Whatever", .expect = 1},
      /* test error after range (no skip) */
      {.pat = (char *)"[A-Z]Fat?ver", .str = (char *)"Whatever", .expect = 0},
      /* end of test marker */
      {.pat = (char *)NULL, .str = (char *)NULL, .expect = 0},
      // clang-format on
  };
  fprintf(stderr, "* Testing glob matching.\n");
  for (size_t i = 0; t[i].pat; ++i) {
    fio_str_info_s p = FIO_STR_INFO1(t[i].pat);
    fio_str_info_s s = FIO_STR_INFO1(t[i].str);
    FIO_ASSERT(t[i].expect == fio_glob_match(p, s),
               "glob matching error for:\n\t String: %s\n\t Pattern: %s",
               s.buf,
               p.buf);
  }
}
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
