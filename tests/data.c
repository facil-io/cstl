/* *****************************************************************************
Test: Data Store (fio_data_s) - RAM buffer that spills to a temporary file
***************************************************************************** */
#ifndef _WIN32
#define TEST_FD_HOOKS /* FIO___FD_PREAD, FSYNC and COPY_RANGE hooks */
#endif
#include "test-helpers.h"

/* Test hook: file writes (fail calls in a mask, limit the bytes per call). */
static uint64_t test_write_fail_mask; /* bit `n`: call `n` writes nothing */
static size_t test_write_max;         /* > 0: max bytes written per call */
static size_t test_write_calls;
static ssize_t test_data_write_hook(int fd, const void *buf, size_t len) {
  size_t call = test_write_calls++;
  if (call < 64 && (test_write_fail_mask & ((uint64_t)1 << call))) {
    errno = EIO;
    return -1;
  }
  if (test_write_max && len > test_write_max)
    len = test_write_max;
  return fio_fd_write(fd, buf, len);
}
static void test_write_hook_set(uint64_t fail_mask, size_t max) {
  test_write_fail_mask = fail_mask;
  test_write_max = max;
  test_write_calls = 0;
}
#define FIO___DATA_WRITE(fd, buf, len) test_data_write_hook((fd), (buf), (len))


#define FIO_DATA
#include FIO_INCLUDE_FILE

#if FIO_OS_POSIX
#include <signal.h>
#include <sys/resource.h>
#endif

/* Reads all remaining data in `chunk` sized reads and compares to `expect`. */
static void test_data_read_all(fio_data_s *d,
                               const char *name,
                               const char *expect,
                               size_t expect_len,
                               size_t chunk) {
  size_t got = 0;
  size_t pos = fio_data_pos(d);
  for (fio_str_info_s s; (s = fio_data_read(d, chunk)).len;) {
    FIO_ASSERT(s.len <= chunk, "%s: read exceeded chunk size", name);
    FIO_ASSERT(got + s.len <= expect_len &&
                   !memcmp(s.buf, expect + got, s.len),
               "%s: data mismatch at %zu",
               name,
               got);
    got += s.len;
    FIO_ASSERT(fio_data_pos(d) == pos + got, "%s: read position", name);
  }
  FIO_ASSERT(got == expect_len, "%s: read %zu / %zu", name, got, expect_len);
  FIO_ASSERT(!fio_data_read(d, chunk).buf, "%s: EOF should be empty", name);
  FIO_ASSERT(fio_data_pos(d) == pos + got, "%s: EOF read position", name);
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
  FIO_ASSERT(fio_data_fd(&d) == -1 && !fio_data_len(&d) &&
                 !fio_data_is_error(&d),
             "%s: init",
             name);
  for (size_t i = 0; i < sizeof(src); i += 256) {
    FIO_ASSERT(!fio_data_write(&d, src + i, 256), "%s: write failed", name);
    FIO_ASSERT(!fio_data_pos(&d), "%s: writes don't move the position", name);
  }
  FIO_ASSERT(!fio_data_write(&d, NULL, 5), "%s: NULL write is a no-op", name);
  FIO_ASSERT(fio_data_len(&d) == sizeof(src), "%s: length", name);
  FIO_ASSERT((fio_data_fd(&d) != -1) == expect_fd, "%s: fd state", name);
  FIO_ASSERT(!expect_fd || !d.buf,
             "%s: RAM buffer should be released after the spill",
             name);

  fio_data_seek(&d, 0);
  test_data_read_all(&d, name, src, sizeof(src), 7);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, name, src, sizeof(src), 4096);

  /* seek: -n == len - n (-1 is the last byte), clamping, >= len is EOF */
  const ssize_t slen = (ssize_t)sizeof(src);
  FIO_ASSERT(fio_data_seek(&d, -1) == sizeof(src) - 1 &&
                 fio_data_pos(&d) == 1023,
             "%s: seek -1 == last byte",
             name);
  test_data_read_all(&d, name, src + sizeof(src) - 1, 1, 16);
  FIO_ASSERT(fio_data_seek(&d, -10) == sizeof(src) - 10, "%s: seek -10", name);
  test_data_read_all(&d, name, src + sizeof(src) - 10, 10, 3);
  FIO_ASSERT(fio_data_seek(&d, -slen) == 0, "%s: seek -len", name);
  test_data_read_all(&d, name, src, sizeof(src), 512);
  FIO_ASSERT(fio_data_seek(&d, -slen + 1) == 1, "%s: seek -len+1", name);
  FIO_ASSERT(fio_data_seek(&d, -slen - 1) == 0, "%s: seek -len-1", name);
  FIO_ASSERT(fio_data_seek(&d, -100000) == 0, "%s: seek clamp low", name);
  FIO_ASSERT(fio_data_seek(&d, 100000) == sizeof(src), "%s: clamp high", name);
  FIO_ASSERT(fio_data_seek(&d, SSIZE_MAX) == sizeof(src),
             "%s: SSIZE_MAX clamps to EOF",
             name);
  FIO_ASSERT(fio_data_seek(&d, -SSIZE_MAX - 1) == 0, "%s: SSIZE_MIN", name);
  FIO_ASSERT(fio_data_seek(&d, 5) == 5 && fio_data_pos(&d) == 5,
             "%s: seek 5",
             name);
  FIO_ASSERT(fio_data_seek(&d, slen) == sizeof(src), "%s: seek len", name);
  test_data_read_all(&d, name, src, 0, 16);

  /* read_until: stops after the token, or at the limit */
  fio_data_seek(&d, 0);
  fio_str_info_s line = fio_data_read_until(&d, '\n', 0);
  FIO_ASSERT(line.len == 100 && line.buf[99] == '\n' &&
                 !memcmp(line.buf, src, 100) && fio_data_pos(&d) == 100,
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
  FIO_ASSERT(line.len == 5 && !memcmp(line.buf, src + sizeof(src) - 5, 5) &&
                 fio_data_pos(&d) == sizeof(src),
             "%s: read_until EOF",
             name);
  FIO_ASSERT(!fio_data_read_until(&d, '\n', 0).buf, "%s: until EOF", name);
  FIO_ASSERT(fio_data_pos(&d) == sizeof(src), "%s: until EOF pos", name);

  /* writing after reads appends, position unchanged */
  fio_data_seek(&d, 3);
  FIO_ASSERT(!fio_data_write(&d, "XYZ", 3) && fio_data_pos(&d) == 3 &&
                 fio_data_len(&d) == sizeof(src) + 3,
             "%s: append after read",
             name);
  fio_data_seek(&d, -3);
  test_data_read_all(&d, name, "XYZ", 3, 64);

  /* destroy resets and keeps the limit; the store is reusable */
  fio_data_destroy(&d);
  FIO_ASSERT(fio_data_fd(&d) == -1 && !fio_data_len(&d) && !fio_data_pos(&d) &&
                 d.limit == limit && !d.buf,
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
  FIO_ASSERT(!d.buf, "RAM buffer should be released after the spill");
  fio_data_write(&d, "abcdef", 6);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "expect", "0123456789abcdef", 16, 5);
  /* writing after a read must still append */
  fio_data_write(&d, "XYZ", 3);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "append", "0123456789abcdefXYZ", 19, 64);
  fio_data_destroy(&d);

  /* overflow-safe length checks: a huge expectation spills (doesn't wrap) */
  d.limit = 64;
  fio_data_write(&d, "0123456789", 10);
  fio_data_expect(&d, (size_t)-1);
  FIO_ASSERT(fio_data_fd(&d) != -1 && fio_data_len(&d) == 10,
             "expect(SIZE_MAX) should spill (overflow)");
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "expect overflow", "0123456789", 10, 64);
  fio_data_destroy(&d);

  /* the RAM limit is clamped below the bstr limit */
  fio_data_init(&d, (size_t)-1);
  FIO_ASSERT(fio___data_limit(&d) == FIO___DATA_RAM_MAX, "limit clamp");
  FIO_ASSERT(!fio_data_write(&d, "abc", 3) && fio_data_fd(&d) == -1,
             "huge limit stays in RAM");
  fio_data_destroy(&d);
}

/* read_until on a spilled store: blocks, limits, missing token, RAM parity */
static void test_data_read_until_blocks(void) {
  fprintf(stderr, "  * read_until across blocks (RAM vs file parity)\n");
  const size_t blk = (size_t)FIO_DATA_READ_BLOCK;
  const size_t len = (blk * 3) + 77;
  char *src = (char *)malloc(len);
  FIO_ASSERT_ALLOC(src);
  for (size_t i = 0; i < len; ++i)
    src[i] = (char)('a' + (i % 23));
  /* tokens right before, at and after block boundaries */
  const size_t marks[] = {blk - 1, blk, blk + 1, (blk * 2) + 5, len - 1};
  for (size_t i = 0; i < sizeof(marks) / sizeof(marks[0]); ++i)
    src[marks[i]] = '\n';

  fio_data_s ram, file;
  fio_data_init(&ram, len);
  fio_data_init(&file, 16);
  FIO_ASSERT(!fio_data_write(&ram, src, len) &&
                 !fio_data_write(&file, src, len),
             "read_until: writes failed");
  FIO_ASSERT(fio_data_fd(&ram) == -1 && fio_data_fd(&file) != -1,
             "read_until: storage modes");

  /* token scan: both stores return the same lines */
  for (size_t i = 0, start = 0; i < sizeof(marks) / sizeof(marks[0]); ++i) {
    fio_str_info_s a = fio_data_read_until(&ram, '\n', 0);
    FIO_ASSERT(a.len == marks[i] + 1 - start &&
                   !memcmp(a.buf, src + start, a.len),
               "read_until RAM line %zu (len %zu)",
               i,
               a.len);
    fio_str_info_s b = fio_data_read_until(&file, '\n', 0);
    FIO_ASSERT(b.len == a.len && !memcmp(b.buf, src + start, b.len),
               "read_until file line %zu (len %zu != %zu)",
               i,
               b.len,
               a.len);
    start += a.len;
    FIO_ASSERT(fio_data_pos(&ram) == start && fio_data_pos(&file) == start,
               "read_until position after line %zu",
               i);
  }
  FIO_ASSERT(!fio_data_read_until(&file, '\n', 0).buf, "file: EOF");

  /* parity over many starts / limits / tokens (incl. absent tokens) */
  const char tokens[] = {'\n', 'q', '#'}; /* '#' is absent */
  const size_t limits[] = {0, 1, 10, blk - 1, blk, blk + 1, blk * 2, len * 2};
  const size_t starts[] = {0, 1, blk - 2, blk, blk + 2, len - 3};
  for (size_t t = 0; t < sizeof(tokens); ++t)
    for (size_t l = 0; l < sizeof(limits) / sizeof(limits[0]); ++l)
      for (size_t s = 0; s < sizeof(starts) / sizeof(starts[0]); ++s) {
        fio_data_seek(&ram, (ssize_t)starts[s]);
        fio_data_seek(&file, (ssize_t)starts[s]);
        fio_str_info_s a = fio_data_read_until(&ram, tokens[t], limits[l]);
        fio_str_info_s b = fio_data_read_until(&file, tokens[t], limits[l]);
        size_t max = limits[l] ? limits[l] : len;
        if (max > len - starts[s])
          max = len - starts[s];
        char *end = (char *)memchr(src + starts[s], tokens[t], max);
        size_t expect = end ? (size_t)(end + 1 - (src + starts[s])) : max;
        FIO_ASSERT(a.len == expect && b.len == expect &&
                       !memcmp(a.buf, src + starts[s], expect) &&
                       !memcmp(b.buf, src + starts[s], expect),
                   "read_until parity: token %c limit %zu start %zu "
                   "(RAM %zu, file %zu, expected %zu)",
                   tokens[t],
                   limits[l],
                   starts[s],
                   a.len,
                   b.len,
                   expect);
        FIO_ASSERT(fio_data_pos(&ram) == starts[s] + expect &&
                       fio_data_pos(&file) == starts[s] + expect,
                   "read_until parity position");
      }
  /* a limit smaller than the block never reads past the limit */
  fio_data_seek(&file, 0);
  fio_str_info_s r = fio_data_read_until(&file, '#', 10);
  FIO_ASSERT(r.len == 10 && fio_data_pos(&file) == 10,
             "read_until limit < block should read only `limit` bytes");
  fio_data_destroy(&ram);
  fio_data_destroy(&file);
  free(src);
}

#if FIO_OS_POSIX
/* Spilled data leaves no named temporary files behind. */
static void test_data_no_tmp_leftovers(void) {
  fprintf(stderr, "  * spills leave no temporary files behind\n");
  char dir[128];
  snprintf(dir, sizeof(dir), "tmp/test-data-tmp-%d", (int)getpid());
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "couldn't create %s", dir);
  char *old = test_tmpdir_set(dir);
  fio_data_s d[3];
  for (size_t i = 0; i < 3; ++i) {
    fio_data_init(d + i, 8);
    FIO_ASSERT(!fio_data_write(d + i, "spill me to a file", 18) &&
                   fio_data_fd(d + i) != -1,
               "spill %zu failed",
               i);
  }
  long open_count = test_dir_count(dir);
  for (size_t i = 0; i < 3; ++i)
    fio_data_destroy(d + i);
  long closed_count = test_dir_count(dir);
  test_tmpdir_restore(old);
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!open_count, "%ld temporary file name(s) visible", open_count);
  FIO_ASSERT(!closed_count, "%ld temporary file(s) left behind", closed_count);
}

/* A failed spill (no temporary folder) enters a sticky error state. */
static void test_data_spill_failure(void) {
  fprintf(stderr, "  * spill failure -> sticky error state\n");
  char *old = test_tmpdir_set("tmp/test-data-no-such-folder/nope");
  fio_data_s d;
  fio_data_init(&d, 64);
  char src[64];
  for (size_t i = 0; i < sizeof(src); ++i)
    src[i] = (char)('A' + (i % 26));
  FIO_ASSERT(!fio_data_write(&d, src, 50), "RAM write should succeed");
  /* expect is a hint: a failed early spill isn't an error */
  fio_data_expect(&d, 1000);
  FIO_ASSERT(!fio_data_is_error(&d) && fio_data_fd(&d) == -1 &&
                 fio_data_len(&d) == 50,
             "failed expect spill shouldn't set the error state");
  FIO_ASSERT(fio_data_write(&d, src, 50) == -1,
             "write past the limit without a temporary file should fail");
  FIO_ASSERT(fio_data_is_error(&d), "error state should be set");
  FIO_ASSERT(fio_data_len(&d) == 50 && fio_data_fd(&d) == -1,
             "length should be unchanged (%zu)",
             fio_data_len(&d));
  FIO_ASSERT(fio_bstr_len(d.buf) == 50 && fio_bstr_info(d.buf).capa <= 128,
             "RAM shouldn't grow past the limit (len %zu, capa %zu)",
             fio_bstr_len(d.buf),
             fio_bstr_info(d.buf).capa);
  FIO_ASSERT(fio_data_write(&d, "x", 1) == -1 && fio_data_len(&d) == 50,
             "later writes should fail without writing");
  fio_data_expect(&d, 1); /* no-op in error state */
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "spill failure", src, 50, 16);
  test_tmpdir_restore(old);
  /* destroy clears the error state */
  fio_data_destroy(&d);
  FIO_ASSERT(!fio_data_is_error(&d) && !fio_data_len(&d) && d.limit == 64,
             "destroy should clear the error state");
  FIO_ASSERT(!fio_data_write(&d, src, 64) && !fio_data_write(&d, src, 10) &&
                 fio_data_fd(&d) != -1 && fio_data_len(&d) == 74,
             "store should work again after destroy");
  fio_data_destroy(&d);
}

/* A failed file write (RLIMIT_FSIZE) keeps the stored prefix. */
static void test_data_fd_write_failure(void) {
  fprintf(stderr, "  * file write failure (RLIMIT_FSIZE) -> error state\n");
  const size_t cap = 1000;
  char src[4096];
  for (size_t i = 0; i < sizeof(src); ++i)
    src[i] = (char)('a' + (i % 26));
  fio_data_s d;
  fio_data_init(&d, 100);
  FIO_ASSERT(!fio_data_write(&d, src, 50), "RAM write should succeed");

  struct rlimit old_limit;
  FIO_ASSERT(!getrlimit(RLIMIT_FSIZE, &old_limit), "getrlimit failed");
  if (old_limit.rlim_cur != RLIM_INFINITY && old_limit.rlim_cur < cap) {
    fprintf(stderr, "    (skipped: RLIMIT_FSIZE already below the test cap)\n");
    fio_data_destroy(&d);
    return;
  }
  void (*old_handler)(int) = signal(SIGXFSZ, SIG_IGN);
  struct rlimit lim = old_limit;
  lim.rlim_cur = (rlim_t)cap;
  FIO_ASSERT(!setrlimit(RLIMIT_FSIZE, &lim), "setrlimit failed");
  int r1 = fio_data_write(&d, src + 50, sizeof(src) - 50);
  int err1 = fio_data_is_error(&d);
  size_t len1 = fio_data_len(&d);
  int r2 = fio_data_write(&d, "x", 1);
  size_t len2 = fio_data_len(&d);
  setrlimit(RLIMIT_FSIZE, &old_limit); /* restore before asserting */
  signal(SIGXFSZ, old_handler);

  FIO_ASSERT(r1 == -1 && err1, "write past RLIMIT_FSIZE should fail (%d)", r1);
  FIO_ASSERT(len1 == cap,
             "length should be the bytes actually stored (%zu != %zu)",
             len1,
             cap);
  FIO_ASSERT(r2 == -1 && len2 == cap, "later writes should fail");
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "fd failure", src, cap, 333);
  fio_data_destroy(&d);
  FIO_ASSERT(!fio_data_is_error(&d), "destroy should clear the error state");
}
#endif /* FIO_OS_POSIX */

/* Spilled reads: whole content, mapping (or heap) and write invalidation. */
static void test_data_spilled_reads(void) {
  fprintf(stderr,
          "  * spilled reads (%s), write invalidates buffers\n",
          FIO___DATA_MMAP ? "mmap" : "heap fallback");
  const size_t len = ((size_t)FIO_DATA_READ_BLOCK * 4) + 321;
  char *src = (char *)malloc(len + 64);
  FIO_ASSERT_ALLOC(src);
  for (size_t i = 0; i < len + 64; ++i)
    src[i] = (char)('A' + (i % 53));
  src[len - 10] = '\n';
  fio_data_s d;
  fio_data_init(&d, 1000);
  FIO_ASSERT(!fio_data_write(&d, src, len) && fio_data_fd(&d) != -1,
             "spilled reads: write");
  fio_str_info_s r = fio_data_read(&d, (size_t)-1);
  FIO_ASSERT(r.len == len && !memcmp(r.buf, src, len) &&
                 fio_data_pos(&d) == len,
             "spilled reads: whole content (%zu)",
             r.len);
#if FIO___DATA_MMAP
  FIO_ASSERT(d.map == 1 && d.capa == len && r.buf == d.buf,
             "spilled reads: should read through the mapping");
  fio_data_seek(&d, 100);
  r = fio_data_read(&d, 50);
  FIO_ASSERT(r.buf == d.buf + 100, "mapped reads are offsets (no copy)");
#else
  FIO_ASSERT(d.map != 1, "no mapping with FIO_DATA_NO_MMAP");
#endif
  fio_data_seek(&d, 5);
  r = fio_data_read_until(&d, '\n', 0);
  FIO_ASSERT(r.len == len - 10 + 1 - 5 && !memcmp(r.buf, src + 5, r.len),
             "spilled read_until (%zu)",
             r.len);
  /* write after read: the mapping is replaced, new bytes visible */
  FIO_ASSERT(!fio_data_write(&d, src + len, 64), "append after read");
#if FIO___DATA_MMAP
  FIO_ASSERT(d.map == 0 && !d.buf, "a write should unmap the file");
#endif
  FIO_ASSERT(fio_data_pos(&d) == len - 10 + 1, "write keeps the position");
  r = fio_data_read(&d, (size_t)-1);
  FIO_ASSERT(r.len == 9 + 64 && !memcmp(r.buf, src + len - 9, r.len),
             "appended data should be visible (%zu)",
             r.len);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "after append", src, len + 64, 4093);
  fio_data_seek(&d, 0);
  r = fio_data_read(&d, (size_t)-1);
  FIO_ASSERT(r.len == len + 64 && !memcmp(r.buf, src, len + 64),
             "whole content after append");
  fio_data_destroy(&d);
  FIO_ASSERT(!d.buf && !d.capa && !d.map && fio_data_fd(&d) == -1,
             "destroy releases the mapping / read buffer");
  free(src);
}

/* File writes are retried while they make progress (write hook). */
static void test_data_write_retry(void) {
  fprintf(stderr, "  * write retries continue while progressing\n");
  char src[2000];
  for (size_t i = 0; i < sizeof(src); ++i)
    src[i] = (char)('a' + (i % 26));
  fio_data_s d;
  /* spill copy: 300 RAM bytes, 64 bytes per call, every other call fails */
  fio_data_init(&d, 400);
  FIO_ASSERT(!fio_data_write(&d, src, 300), "retry: RAM write");
  test_write_hook_set(0xAAAAAAAAAAAAAAAAULL, 64);
  int r1 = fio_data_write(&d, src + 300, 200); /* spills, then appends */
  size_t calls = test_write_calls;
  test_write_hook_set(0, 0);
  FIO_ASSERT(!r1 && !fio_data_is_error(&d) && fio_data_len(&d) == 500 &&
                 fio_data_fd(&d) != -1,
             "retry: writes making progress should succeed (len %zu)",
             fio_data_len(&d));
  FIO_ASSERT(calls > 4, "retry: the hook should be used (%zu calls)", calls);
  /* fd append: same pattern */
  test_write_hook_set(0x5555555555555554ULL, 50);
  int r2 = fio_data_write(&d, src + 500, 1000);
  test_write_hook_set(0, 0);
  FIO_ASSERT(!r2 && fio_data_len(&d) == 1500, "retry: append with progress");
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "retry", src, 1500, 333);
  /* two consecutive zero-progress attempts fail (after partial progress) */
  test_write_hook_set(0xFFFFFFFFFFFFFFFCULL, 100); /* 2 calls, then fails */
  int r3 = fio_data_write(&d, src + 1500, 500);
  size_t calls3 = test_write_calls;
  test_write_hook_set(0, 0);
  FIO_ASSERT(r3 == -1 && fio_data_is_error(&d) && fio_data_len(&d) == 1700,
             "retry: 2 consecutive failures enter the error state (len %zu)",
             fio_data_len(&d));
  FIO_ASSERT(calls3 == 4, "retry: exactly 2 failed attempts (%zu)", calls3);
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "retry prefix", src, 1700, 512);
  fio_data_destroy(&d);
  /* a spill copy that never progresses fails, RAM data intact */
  fio_data_init(&d, 100);
  FIO_ASSERT(!fio_data_write(&d, src, 100), "retry: RAM write 2");
  test_write_hook_set(~(uint64_t)0, 0);
  int r4 = fio_data_write(&d, src + 100, 10);
  test_write_hook_set(0, 0);
  FIO_ASSERT(r4 == -1 && fio_data_is_error(&d) && fio_data_fd(&d) == -1 &&
                 fio_data_len(&d) == 100,
             "retry: failed spill copy keeps RAM data");
  fio_data_seek(&d, 0);
  test_data_read_all(&d, "failed spill", src, 100, 64);
  fio_data_destroy(&d);
}


#if FIO_OS_POSIX
/* Reads the whole file `name` (a bstr, free with `fio_bstr_free`). */
static char *test_data_file(const char *name) {
  return fio_bstr_readfile(NULL, name, 0, 0);
}

/* fio_data_save: RAM, spilled, overwrite, error state, no leftovers. */
static void test_data_save(void) {
  fprintf(stderr, "  * save (RAM, spilled, overwrite, error state)\n");
  char dir[128], name[192];
  snprintf(dir, sizeof(dir), "tmp/test-data-save-%d", (int)getpid());
  snprintf(name, sizeof(name), "%s/saved.bin", dir);
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "couldn't create %s", dir);
  const size_t len = ((size_t)FIO_DATA_READ_BLOCK * 3) + 17;
  char *src = (char *)malloc(len);
  FIO_ASSERT_ALLOC(src);
  for (size_t i = 0; i < len; ++i)
    src[i] = (char)('0' + (i % 71));
  const mode_t um = umask(022);
  umask(um);

  /* RAM store, new file */
  fio_data_s d;
  fio_data_init(&d, len + 1);
  FIO_ASSERT(!fio_data_write(&d, src, 100) && fio_data_fd(&d) == -1,
             "save: RAM write");
  fio_data_seek(&d, 3);
  FIO_ASSERT(!fio_data_save(&d, name), "save: RAM store failed");
  char *f = test_data_file(name);
  FIO_ASSERT(fio_bstr_len(f) == 100 && !memcmp(f, src, 100), "save: RAM data");
  fio_bstr_free(f);
  struct stat st;
  FIO_ASSERT(!stat(name, &st) && (st.st_mode & 0777) == (0666 & ~um),
             "save: file mode %o (umask %o)",
             (unsigned)(st.st_mode & 0777),
             (unsigned)um);
  FIO_ASSERT(fio_data_pos(&d) == 3 && fio_data_len(&d) == 100,
             "save: RAM store unchanged");
  FIO_ASSERT(test_dir_count(dir) == 1, "save: sibling left (RAM)");
  /* store usable after save; spill it; overwrite the existing file */
  FIO_ASSERT(!fio_data_write(&d, src + 100, len - 100), "save: write more");
  fio_data_destroy(&d);
  fio_data_init(&d, 1000);
  FIO_ASSERT(!fio_data_write(&d, src, len) && fio_data_fd(&d) != -1,
             "save: spilled write");
  fio_data_seek(&d, 1234);
  FIO_ASSERT(!fio_data_save(&d, name), "save: spilled store failed");
  f = test_data_file(name);
  FIO_ASSERT(fio_bstr_len(f) == len && !memcmp(f, src, len),
             "save: spilled data overwrote the file (%zu bytes)",
             fio_bstr_len(f));
  fio_bstr_free(f);
  FIO_ASSERT(fio_data_pos(&d) == 1234 && fio_data_len(&d) == len,
             "save: spilled store unchanged");
  FIO_ASSERT(test_dir_count(dir) == 1, "save: sibling left (spilled)");
  /* still usable: append, read, save again */
  FIO_ASSERT(!fio_data_write(&d, "tail", 4), "save: append after save");
  fio_str_info_s r = fio_data_read(&d, 10);
  FIO_ASSERT(r.len == 10 && !memcmp(r.buf, src + 1234, 10),
             "save: read after save");
  FIO_ASSERT(!fio_data_save(&d, name), "save: second save");
  f = test_data_file(name);
  FIO_ASSERT(fio_bstr_len(f) == len + 4 && !memcmp(f + len, "tail", 4),
             "save: second save content");
  fio_bstr_free(f);
  /* empty store saves an empty file */
  fio_data_destroy(&d);
  FIO_ASSERT(!fio_data_save(&d, name), "save: empty store");
  FIO_ASSERT(fio_filename_size(name) == 0, "save: empty file");
  /* a missing folder fails, nothing created */
  char bad[224];
  snprintf(bad, sizeof(bad), "%s/no-such-folder/x.bin", dir);
  FIO_ASSERT(!fio_data_write(&d, src, 10), "save: write");
  FIO_ASSERT(fio_data_save(&d, bad) == -1, "save: missing folder fails");
  /* error state: refused, existing file untouched */
  FIO_ASSERT(!fio_data_save(&d, name), "save: 10 bytes");
  fio_data_destroy(&d);
  fio_data_init(&d, 16);
  FIO_ASSERT(!fio_data_write(&d, src, 16), "save: RAM write 3");
  test_write_hook_set(~(uint64_t)0, 0);
  FIO_ASSERT(fio_data_write(&d, src, 100) == -1, "save: force error");
  test_write_hook_set(0, 0);
  FIO_ASSERT(fio_data_is_error(&d), "save: error state");
  FIO_ASSERT(fio_data_save(&d, name) == -1, "save: refused in error state");
  FIO_ASSERT(fio_filename_size(name) == 10, "save: file untouched on refusal");
  /* spilled saves copy the file: no read buffer / mapping needed */
  fio_data_destroy(&d);
  fio_data_init(&d, 16);
  FIO_ASSERT(!fio_data_write(&d, src, 5000), "save: spilled write 2");
  fio_data_seek(&d, 100);
  fio_str_info_s before = fio_data_read(&d, 50);
  int ra = fio_data_save(&d, name);
  FIO_ASSERT(!ra && fio_filename_size(name) == 5000,
             "save: spilled save doesn't need a read buffer");
  FIO_ASSERT(before.len == 50 && !memcmp(before.buf, src + 100, 50) &&
                 fio_data_pos(&d) == 150,
             "save: previously returned buffer still valid");
  f = test_data_file(name);
  FIO_ASSERT(fio_bstr_len(f) == 5000 && !memcmp(f, src, 5000),
             "save: spilled content (2)");
  fio_bstr_free(f);
#ifndef _WIN32
  /* a short read of the temporary file fails the save (never truncates) */
  FIO_ASSERT(!fio_data_save(&d, name) && fio_filename_size(name) == 5000,
             "save: before short read");
  FIO_ASSERT(!fio_data_write(&d, src, 1000), "save: write more");
  for (int i = 0; i < 2; ++i) {
    test_fd_hooks_reset();
    test_fd_copy.limit = 0; /* force the block copy (no kernel copy) */
    test_fd_copy.fail_errno = EXDEV;
    test_fd_read.fail_at = 3000;
    test_fd_read.fail_errno = i ? EIO : 0;
    errno = 0;
    int rs = fio_data_save(&d, name);
    int es = errno;
    test_fd_hooks_reset();
    FIO_ASSERT(rs == -1 && es == EIO && fio_filename_size(name) == 5000,
               "save: short read (%s) fails with EIO (%d), file untouched",
               (i ? "error" : "EOF"),
               es);
  }
  /* fsync is part of the save */
  test_fd_hooks_reset();
  FIO_ASSERT(!fio_data_save(&d, name) && test_fd_fsync.calls == 1 &&
                 fio_filename_size(name) == 6000,
             "save: fsync called");
  test_fd_fsync.fail_errno = ENOSPC;
  errno = 0;
  FIO_ASSERT(fio_data_save(&d, name) == -1 && errno == ENOSPC,
             "save: fsync failure fails, errno kept");
  test_fd_hooks_reset();
  /* RAM data is saved from the buffer ("~/" expanded) */
  fio_data_destroy(&d);
  FIO_ASSERT(!fio_data_write(&d, "ram", 3) && fio_data_fd(&d) == -1,
             "save: RAM write 4");
  {
    const char *old_home = getenv("HOME");
    char *old = old_home ? strdup(old_home) : NULL;
    setenv("HOME", dir, 1);
    test_fd_hooks_reset();
    int rh = fio_data_save(&d, "~/saved.bin");
    const size_t rh_syncs = test_fd_fsync.calls;
    if (old) {
      setenv("HOME", old, 1);
      free(old);
    } else {
      unsetenv("HOME");
    }
    FIO_ASSERT(!rh && fio_filename_size(name) == 3, "save: ~/ expansion");
    FIO_ASSERT(rh_syncs == 1, "save: RAM save fsyncs once (%zu)", rh_syncs);
  }
#endif
  FIO_ASSERT(test_dir_count(dir) == 1, "save: sibling left (errors)");
  fio_data_destroy(&d);
  free(src);
  fio_filename_remove(.path = dir, .recursive = 1);
}
#endif /* FIO_OS_POSIX */

#if !FIO___DATA_MMAP && !defined(_WIN32)
/* Heap fallback short reads fail loud: empty result, pos unchanged, errno. */
static void test_data_short_reads(void) {
  fprintf(stderr, "  * heap fallback short reads fail, EINTR is retried\n");
  char src[5000];
  for (size_t i = 0; i < sizeof(src); ++i)
    src[i] = (char)('a' + (i % 26));
  src[4500] = '#';
  fio_data_s d;
  fio_data_init(&d, 100);
  FIO_ASSERT(!fio_data_write(&d, src, sizeof(src)) && fio_data_fd(&d) != -1,
             "short read: write");
  for (int i = 0; i < 2; ++i) { /* EOF (file truncated) / read error */
    test_fd_hooks_reset();
    test_fd_read.fail_at = 3000;
    test_fd_read.fail_errno = i ? EBADF : 0;
    const int expect = i ? EBADF : EIO;
    fio_data_seek(&d, 10);
    errno = 0;
    fio_str_info_s r1 = fio_data_read(&d, 4000);
    int e1 = errno;
    size_t p1 = fio_data_pos(&d);
    errno = 0;
    fio_str_info_s r2 = fio_data_read_until(&d, '#', 0);
    int e2 = errno;
    size_t p2 = fio_data_pos(&d);
    fio_str_info_s r3 = fio_data_read(&d, 100); /* before the failure: ok */
    test_fd_hooks_reset();
    FIO_ASSERT(!r1.buf && !r1.len && p1 == 10 && e1 == expect,
               "short read (%d): read should fail (pos %zu, errno %d)",
               i,
               p1,
               e1);
    FIO_ASSERT(!r2.buf && !r2.len && p2 == 10 && e2 == expect,
               "short read (%d): read_until should fail (pos %zu, errno %d)",
               i,
               p2,
               e2);
    FIO_ASSERT(r3.len == 100 && !memcmp(r3.buf, src + 10, 100) &&
                   fio_data_pos(&d) == 110,
               "short read (%d): reads before the failure work",
               i);
    FIO_ASSERT(!fio_data_is_error(&d), "short reads aren't a store error");
  }
  /* EINTR and partial reads are retried */
  test_fd_hooks_reset();
  test_fd_read.eintr = 2;
  test_fd_read.max = 700;
  fio_data_seek(&d, 0);
  fio_str_info_s all = fio_data_read(&d, (size_t)-1);
  FIO_ASSERT(all.len == sizeof(src) && !memcmp(all.buf, src, sizeof(src)) &&
                 test_fd_read.eintr_seen == 2,
             "EINTR: read retried");
  test_fd_read.eintr = 2;
  FIO_ASSERT(!fio_data_write(&d, "x", 1), "EINTR: append (drops the window)");
  fio_data_seek(&d, 0);
  fio_str_info_s line = fio_data_read_until(&d, '#', 0);
  FIO_ASSERT(line.len == 4501 && line.buf[4500] == '#' &&
                 fio_data_pos(&d) == 4501 && test_fd_read.eintr_seen == 4,
             "EINTR: read_until retried");
  test_fd_hooks_reset();
  fio_data_destroy(&d);
}
#endif

/* Reads `len` bytes at `at`, checking the content against `src`. */
static void test_data_window_read(fio_data_s *d,
                                  const char *src,
                                  size_t at,
                                  size_t len,
                                  const char *what) {
  fio_data_seek(d, (ssize_t)at);
  fio_str_info_s r = fio_data_read(d, len);
  if (len > fio_data_len(d) - at)
    len = fio_data_len(d) - at;
  FIO_ASSERT(r.len == len && (!len || !memcmp(r.buf, src + at, len)) &&
                 fio_data_pos(d) == at + len,
             "window (%s): read %zu at %zu (got %zu)",
             what,
             len,
             at,
             r.len);
}

/* Sliding / growing / backward windows, writes and read_until interleaved. */
static void test_data_windows(void) {
  fprintf(stderr,
          "  * spilled read windows (sliding, growing, backward, writes)\n");
  const size_t len = ((size_t)FIO_DATA_READ_BLOCK * 6) + 777;
  char *src = (char *)malloc(len + 4096);
  FIO_ASSERT_ALLOC(src);
  for (size_t i = 0; i < len + 4096; ++i)
    src[i] = (char)('!' + ((i * 7 + (i >> 9)) % 89));
  for (size_t i = 1000; i < len; i += 3001)
    src[i] = '\n';
  fio_data_s d;
  fio_data_init(&d, 1000);
  FIO_ASSERT(!fio_data_write(&d, src, len) && fio_data_fd(&d) != -1,
             "window: write");
  /* multipart like: seek(base) read(want), keep the tail, grow */
  size_t base = 0, want = 4096;
  while (base < len) {
    test_data_window_read(&d, src, base, want, "sliding");
    size_t got = (want > len - base) ? (len - base) : want;
    size_t consumed = got / 3 + 1;
    size_t keep = got - consumed;
    base += consumed;
    want = keep + ((keep > 65536) ? keep : 65536);
  }
  /* backward, overlapping tail, disjoint, sub-ranges and EOF */
  test_data_window_read(&d, src, len / 2, 10000, "fwd");
  test_data_window_read(&d, src, len / 2 - 3000, 10000, "back overlap");
  test_data_window_read(&d, src, len / 2 - 1000, 500, "inside");
  test_data_window_read(&d, src, 7, 100, "back disjoint");
  test_data_window_read(&d, src, 50, 200, "overlap tail");
  test_data_window_read(&d, src, len - 100, 1000, "EOF");
  test_data_window_read(&d, src, 0, (size_t)-1, "all");
  /* read_until interleaved with reads */
  fio_data_seek(&d, 900);
  for (size_t at = 900; at < len;) {
    fio_str_info_s r = fio_data_read_until(&d, '\n', 0);
    FIO_ASSERT(r.len && !memcmp(r.buf, src + at, r.len) &&
                   (r.buf[r.len - 1] == '\n' || at + r.len == len),
               "window: read_until at %zu",
               at);
    at += r.len;
    if (at >= len)
      break;
    r = fio_data_read(&d, 37);
    size_t n = (37 > len - at) ? (len - at) : 37;
    FIO_ASSERT(r.len == n && !memcmp(r.buf, src + at, n),
               "window: read after read_until at %zu",
               at);
    at += n;
  }
  fio_data_seek(&d, 999);
  fio_str_info_s r = fio_data_read_until(&d, '\n', 0);
  FIO_ASSERT(r.len == 2 && !memcmp(r.buf, src + 999, 2), "window: short line");
  /* reads after writes / appends see the new data */
  test_data_window_read(&d, src, len - 500, 400, "before append");
  FIO_ASSERT(!fio_data_write(&d, src + len, 4096), "window: append");
#if !FIO___DATA_MMAP && !defined(_WIN32)
  /* (no window after the append) overlapping reads skip held bytes */
  test_fd_hooks_reset();
  test_data_window_read(&d, src, 1000, 8192, "count 1");
  test_data_window_read(&d, src, 3000, 16384, "count 2");
  test_data_window_read(&d, src, 5000, 1000, "count 3 (held)");
  FIO_ASSERT(test_fd_read.bytes == 8192 + 10192,
             "window: overlapping reads re-read data (%zu bytes)",
             test_fd_read.bytes);
  fio_data_seek(&d, 4001); /* held: 3000 .. 19384 */
  r = fio_data_read_until(&d, '\n', 0);
  FIO_ASSERT(r.len == 1 && test_fd_read.bytes == 18384,
             "window: read_until should reuse the window (%zu)",
             test_fd_read.bytes);
  r = fio_data_read_until(&d, '\n', 0); /* 4002 .. 7002 held */
  FIO_ASSERT(r.len == 3001 && !memcmp(r.buf, src + 4002, 3001) &&
                 test_fd_read.bytes == 18384,
             "window: read_until in window (%zu)",
             test_fd_read.bytes);
  r = fio_data_read(&d, 20000); /* 7003 .. 27003: reads 7619 bytes */
  FIO_ASSERT(r.len == 20000 && !memcmp(r.buf, src + 7003, 20000) &&
                 test_fd_read.bytes == 18384 + 7619,
             "window: read after read_until reuses the window (%zu)",
             test_fd_read.bytes);
  test_fd_hooks_reset();
#endif
  test_data_window_read(&d, src, len - 500, 4596, "after append");
  test_data_window_read(&d, src, 0, (size_t)-1, "all after append");
  fio_data_destroy(&d);
  FIO_ASSERT(!d.wlen && !d.buf, "destroy clears the window");
  free(src);
}

/* fio_data_bstr: the RAM bstr (NULL once spilled); copies survive writes. */
static void test_data_bstr(void) {
  fprintf(stderr, "  * fio_data_bstr (RAM fio_bstr, copy on write)\n");
  fio_data_s d;
  fio_data_init(&d, 1000);
  FIO_ASSERT(!fio_data_bstr(&d), "bstr: empty store is NULL");
  FIO_ASSERT(!fio_data_write(&d, "0123456789", 10), "bstr: write");
  fio_data_expect(&d, 100); /* spare capacity: writes could go in place */
  char *b = fio_data_bstr(&d);
  FIO_ASSERT(b && fio_bstr_len(b) == 10 && !memcmp(b, "0123456789", 10),
             "bstr: RAM content");
  fio_str_info_s r = fio_data_read(&d, (size_t)-1);
  FIO_ASSERT(r.buf == b && r.len == 10, "bstr: same buffer as RAM reads");
  char *ref = fio_bstr_copy(fio_data_bstr(&d));
  FIO_ASSERT(ref == b, "bstr: fio_bstr_copy is a reference (no data copy)");
  FIO_ASSERT(!fio_data_write(&d, "abcdef", 6) && fio_data_len(&d) == 16,
             "bstr: write after copy");
  FIO_ASSERT(fio_bstr_len(ref) == 10 && !memcmp(ref, "0123456789", 10) &&
                 !ref[10],
             "bstr: copy unchanged by later writes (copy on write)");
  b = fio_data_bstr(&d);
  FIO_ASSERT(b && b != ref && fio_bstr_len(b) == 16 &&
                 !memcmp(b, "0123456789abcdef", 16),
             "bstr: store content after write");
  fio_bstr_free(ref);
  ref = fio_bstr_copy(fio_data_bstr(&d));
  fio_data_destroy(&d); /* the reference outlives the store */
  FIO_ASSERT(!fio_data_bstr(&d), "bstr: NULL after destroy");
  FIO_ASSERT(ref && fio_bstr_len(ref) == 16 &&
                 !memcmp(ref, "0123456789abcdef", 16),
             "bstr: copy outlives the store");
  fio_bstr_free(ref);
  fio_data_init(&d, 8);
  FIO_ASSERT(!fio_data_write(&d, "0123", 4) && fio_data_bstr(&d),
             "bstr: RAM before spill");
  FIO_ASSERT(!fio_data_write(&d, "456789", 6) && fio_data_fd(&d) != -1,
             "bstr: spill");
  FIO_ASSERT(!fio_data_bstr(&d), "bstr: spilled store is NULL");
  r = fio_data_read(&d, (size_t)-1); /* buf now maps / buffers the file */
  FIO_ASSERT(r.len == 10 && !fio_data_bstr(&d), "bstr: NULL after reads");
  fio_data_destroy(&d);
}

int main(void) {
  fprintf(stderr,
          "Testing Data Store (fio_data_s%s):\n",
          FIO___DATA_MMAP ? "" : ", FIO_DATA_NO_MMAP");
  test_data_mode(0, 0);   /* default limit: RAM */
  test_data_mode(100, 1); /* spills to a temporary file */
  test_data_expect();
  test_data_read_until_blocks();
  test_data_spilled_reads();
  test_data_write_retry();
#if !FIO___DATA_MMAP && !defined(_WIN32)
  test_data_short_reads();
#endif
  test_data_windows();
  test_data_bstr();
#if FIO_OS_POSIX
  test_data_save();
  test_data_no_tmp_leftovers();
  test_data_spill_failure();
  test_data_fd_write_failure();
#endif
  fprintf(stderr, "All Data Store tests passed!\n");
  return 0;
}
