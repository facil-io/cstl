/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_MUSTACHE Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_MUSTACHE_TEST___H)
// #define H___FIO_MUSTACHE_TEST___H
// #ifndef H___FIO_MUSTACHE___H
// #define FIO_MUSTACHE
// #define FIO___TEST_REINCLUDE
// #include FIO_INCLUDE_FILE
// #undef FIO___TEST_REINCLUDE
// #endif

FIO_SFUNC void FIO_NAME_TEST(stl, mustache)(void) {
  fprintf(stderr, "* Testing mustache template parser.\n");
  char *example1 = (char *)"This is a {{tag}}, and so is {{ this_one }}.";
  char *example2 = (char *)"{{tag}} and {{ incomplete}";
  fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO1(example1));
  FIO_ASSERT(m, "valid example load failed!");
  char *result = (char *)fio_mustache_build(m, .ctx = NULL);
  FIO_ASSERT(result, "a valid fio_mustache_build returned NULL");
  FIO_ASSERT(
      FIO_BUF_INFO_IS_EQ(fio_bstr_buf(result),
                         FIO_BUF_INFO1((char *)"This is a , and so is .")),
      "valid example result failed: %s",
      result);
  fio_bstr_free(result);
  fio_mustache_free(m);
  fprintf(stderr, "\terror should print on next line.\n");
  m = fio_mustache_load(.data = FIO_BUF_INFO1(example2));
  FIO_ASSERT(!m, "invalid example load returned an object.");
  fio_mustache_free(m);
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
