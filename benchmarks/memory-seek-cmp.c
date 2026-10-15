/* Memory seek, comparison and constant-time test benchmarks.
 * Run with: make benchmarks/memory-seek-cmp
 */
#include "tests/test-helpers.h"
#undef FIO_MEMALT
#define FIO_MEMALT
#include FIO_INCLUDE_FILE
#include <string.h>
#if defined(HAVE_OPENSSL)
#include <openssl/crypto.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <string.h> /* timingsafe_bcmp */
#endif
#include "memory-compare.h"

static intptr_t bench_fio_seek(const unsigned char *a, const unsigned char *b,
                               size_t len) {
  const unsigned char *p = fio_memchr(a, (char)b[0], len);
  return p ? (intptr_t)(p - a) : -1;
}
static intptr_t bench_std_seek(const unsigned char *a, const unsigned char *b,
                               size_t len) {
  const unsigned char *p = memchr(a, b[0], len);
  return p ? (intptr_t)(p - a) : -1;
}
static intptr_t bench_fio_cmp(const unsigned char *a, const unsigned char *b,
                              size_t len) { return fio_memcmp(a, b, len); }
static intptr_t bench_std_cmp(const unsigned char *a, const unsigned char *b,
                              size_t len) { return memcmp(a, b, len); }
#if defined(HAVE_OPENSSL) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static intptr_t bench_fio_ct(const unsigned char *a, const unsigned char *b,
                             size_t len) { return fio_ct_is_eq(a, b, len); }
static intptr_t bench_secure_eq(const unsigned char *a, const unsigned char *b,
                                size_t len) {
#if defined(HAVE_OPENSSL)
  return CRYPTO_memcmp(a, b, len) == 0;
#else
  return timingsafe_bcmp(a, b, len) == 0;
#endif
}
#endif

static void bench_memalt_search_compare(void) {
  const bench_op_s seek = {"fio_memchr", "memchr", bench_fio_seek,
                           bench_std_seek, 1, 0};
  const bench_op_s cmp = {"fio_memcmp", "memcmp", bench_fio_cmp,
                          bench_std_cmp, 2, 0};
  bench_memory_run(&seek);
  bench_memory_run(&cmp);
#if defined(HAVE_OPENSSL)
  const bench_op_s ct = {"fio_ct_is_eq", "CRYPTO_memcmp", bench_fio_ct,
                         bench_secure_eq, 0, 0};
  bench_memory_run(&ct);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  const bench_op_s ct = {"fio_ct_is_eq", "timingsafe_bcmp", bench_fio_ct,
                         bench_secure_eq, 0, 0};
  bench_memory_run(&ct);
#endif
}


int main(void) { bench_memalt_search_compare(); return 0; }
