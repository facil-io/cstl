/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_MUSTACHE
#include FIO_INCLUDE_FILE

int main(void) {
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
  FIO_LOG_DDEBUG("error should print on next line.");
  m = fio_mustache_load(.data = FIO_BUF_INFO1(example2));
  FIO_ASSERT(!m, "invalid example load returned an object.");
  fio_mustache_free(m);
}
