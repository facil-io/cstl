/* *****************************************************************************
Test: Data Store (fio_data_s) - RAM buffer that spills to a temporary file
***************************************************************************** */
#include "test-helpers.h"
#define FIO_DATA
#include FIO_INCLUDE_FILE

/* Reads all remaining data in `chunk` sized reads and compares to `expect`. */
static void test_data_read_all(fio_data_s *d,
                               const char *name,
                               const char *expect,
                               size_t expect_len,
                               size_t chunk) {
  size_t got = 0;
  for (fio_str_info_s s; (s = fio_data_read(d, chunk)).len;) {
    FIO_ASSERT(s.len <= chunk, "%s: read exceeded chunk size", name);
    FIO_ASSERT(got + s.len <= expect_len &&
                   !memcmp(s.buf, expect + got, s.len),
               "%s: data mismatch at %zu",
               name,
               got);
    got += s.len;
  }
  FIO_ASSERT(got == expect_len, "%s: read %zu / %zu", name, got, expect_len);
  FIO_ASSERT(!fio_data_read(d, chunk).buf, "%s: EOF should be empty", name);
}

static void test_data_mode(size_t limit, int expect_fd) {
  const char *name = expect_fd ? "file" : "RAM";
  fprintf(stderr, "  * %s storage (limit %zu)\n", name, limit);
  char src[1024];
  for (size_t i = 0; i < sizeof(src); ++i)
    src[i] = (char)('a' + (i % 23));
  for (size_t i = 99; i < sizeof(src); i += 100)
    src[i] = '\n';

  fio_data_s d;
  fio_data_init(&d, limit);
  FIO_ASSERT(fio_data_fd(&d) == -1 && !fio_data_len(&d), "%s: init", name);
  for (size_t i = 0; i < sizeof(src); i += 256)
    FIO_ASSERT(!fio_data_write(&d, src + i, 256), "%s: write failed", name);
  FIO_ASSERT(!fio_data_write(&d, NULL, 5), "%s: NULL write is a no-op", name);
  FIO_ASSERT(fio_data_len(&d) == sizeof(src), "%s: length", name);
  FIO_ASSERT((fio_data_fd(&d) != -1) == expect_fd, "%s: fd state", name);

  fio_data_seek(&d, 0);
  test_data_read_all(&d, name, src, sizeof(src), 7);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, name, src, sizeof(src), 4096);

  /* seek: negative from end, clamping, SSIZE_MAX keeps position */
  FIO_ASSERT(fio_data_seek(&d, -10) == sizeof(src) - 10, "%s: seek -10", name);
  FIO_ASSERT(fio_data_seek(&d, SSIZE_MAX) == sizeof(src) - 10,
             "%s: seek SSIZE_MAX",
             name);
  FIO_ASSERT(fio_data_seek(&d, -100000) == 0, "%s: seek clamp low", name);
  FIO_ASSERT(fio_data_seek(&d, 100000) == sizeof(src), "%s: clamp high", name);
  test_data_read_all(&d, name, src, 0, 16);

  /* read_until: stops after the token, or at the limit */
  fio_data_seek(&d, 0);
  fio_str_info_s line = fio_data_read_until(&d, '\n', 0);
  FIO_ASSERT(line.len == 100 && line.buf[99] == '\n' &&
                 !memcmp(line.buf, src, 100),
             "%s: read_until token",
             name);
  line = fio_data_read_until(&d, '\n', 10);
  FIO_ASSERT(line.len == 10 && !memcmp(line.buf, src + 100, 10) &&
                 fio_data_pos(&d) == 110,
             "%s: read_until limit (len %zu, pos %zu)",
             name,
             line.len,
             fio_data_pos(&d));
  fio_data_seek(&d, -5);
  line = fio_data_read_until(&d, '\n', 0);
  FIO_ASSERT(line.len == 5 && !memcmp(line.buf, src + sizeof(src) - 5, 5),
             "%s: read_until EOF",
             name);
  FIO_ASSERT(!fio_data_read_until(&d, '\n', 0).buf, "%s: until EOF", name);

  /* destroy resets and keeps the limit; the store is reusable */
  fio_data_destroy(&d);
  FIO_ASSERT(fio_data_fd(&d) == -1 && !fio_data_len(&d) && !fio_data_pos(&d) &&
                 d.limit == limit,
             "%s: destroy should reset",
             name);
  FIO_ASSERT(!fio_data_write(&d, "abc", 3) && fio_data_len(&d) == 3,
             "%s: reuse after destroy",
             name);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, name, "abc", 3, 2);
  fio_data_destroy(&d);
  fio_data_destroy(&d); /* double destroy is safe */
}

static void test_data_expect(void) {
  fprintf(stderr, "  * expect spills early, writes append\n");
  fio_data_s d = FIO_DATA_INIT;
  d.limit = 64;
  fio_data_expect(&d, 32);
  FIO_ASSERT(fio_data_fd(&d) == -1, "expect within limit stays in RAM");
  fio_data_write(&d, "0123456789", 10);
  fio_data_expect(&d, 100);
  FIO_ASSERT(fio_data_fd(&d) != -1, "expect past limit should spill");
  fio_data_write(&d, "abcdef", 6);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "expect", "0123456789abcdef", 16, 5);
  /* writing after a read must still append */
  fio_data_write(&d, "XYZ", 3);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "append", "0123456789abcdefXYZ", 19, 64);
  fio_data_destroy(&d);
}

int main(void) {
  fprintf(stderr, "Testing Data Store (fio_data_s):\n");
  test_data_mode(0, 0);  /* default limit: RAM */
  test_data_mode(100, 1); /* spills to a temporary file */
  test_data_expect();
  fprintf(stderr, "All Data Store tests passed!\n");
  return 0;
}
