/* Verify that CLI-owned allocations are cleared before reaching the allocator. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static size_t checked_frees;
static int inspect_frees;
static void *cli_test_realloc(void *ptr, size_t old_size, size_t new_size,
                              size_t copy_len) {
  if (!new_size) return NULL;
  void *next = malloc(new_size);
  if (!next) abort();
  if (ptr) {
    if (copy_len > old_size || copy_len > new_size) abort();
    memcpy(next, ptr, copy_len);
    free(ptr);
  }
  return next;
}
static void cli_test_free(void *ptr, size_t size) {
  /* The alias map owns non-CLI-strings and does not promise wiping. */
  if (ptr && inspect_frees &&
      (size == 128 || size == 256 || size == 512 ||
       size == sizeof("synthetic-owned-value-long") ||
       size == sizeof("synthetic-replacement-long") ||
       size == sizeof("synthetic-default"))) {
    const unsigned char *bytes = (const unsigned char *)ptr;
    for (size_t i = 0; i < size; ++i)
      if (bytes[i]) {
        fprintf(stderr, "CLI freed uncleared byte %zu of %zu\n", i, size);
        abort();
      }
    ++checked_frees;
  }
  free(ptr);
}
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len) \
  cli_test_realloc((ptr), (old_size), (new_size), (copy_len))
#define FIO_MEM_FREE(ptr, size) cli_test_free((ptr), (size))
#define FIO_MEM_REALLOC_IS_SAFE 0
#define FIO_CLI
#include "fio-stl/include.h"

int main(void) {
  fio___cli_ary_s values = {0};
  inspect_frees = 1;
  for (unsigned i = 0; i < 17; ++i) {
    uint32_t index = fio___cli_ary_new_index(&values);
    fio___cli_ary_set(&values, index,
                      FIO_BUF_INFO1((char *)"synthetic-owned-value-long"));
  }
  fio___cli_ary_set(&values, 0, FIO_BUF_INFO1((char *)"short"));
  fio___cli_ary_destroy(&values);
  const char *argv[] = {"application"};
  fio_cli_start(1, argv, 0, 0, "description NAME",
                FIO_CLI_STRING("-password (synthetic-default) a test option"));
  fio_cli_set("-password", "synthetic-replacement-long");
  fio_cli_end();
#if !defined(FIO_MEMORY_DISABLE)
  if (checked_frees < 20) {
    fprintf(stderr, "too few allocator checks: %zu\n", checked_frees);
    return 1;
  }
#endif
  inspect_frees = 0;
  return 0;
}
