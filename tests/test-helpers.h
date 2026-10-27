#ifndef H___TEST_HELPERS___H
#define H___TEST_HELPERS___H

/* *****************************************************************************
Opt-in file I/O hooks (define TEST_FD_HOOKS before including this file)
***************************************************************************** */
#ifdef TEST_FD_HOOKS
#include <stdint.h>
#include <sys/types.h>
static ssize_t test_fd_pread_hook(int fd, void *buf, size_t len, off_t at);
static int test_fd_fsync_hook(int fd);
static ssize_t test_fd_copy_range_hook(int in,
                                       int64_t *in_at,
                                       int out,
                                       int64_t *out_at,
                                       size_t len);
#define FIO___FD_PREAD(fd, buf, len, at)                                       \
  test_fd_pread_hook((fd), (buf), (len), (at))
#define FIO___FILENAME_FSYNC(fd) test_fd_fsync_hook((fd))
#define FIO___FILENAME_COPY_RANGE(in, in_at, out, out_at, len)                 \
  test_fd_copy_range_hook((in), (in_at), (out), (out_at), (len))
#endif /* TEST_FD_HOOKS */

#define FIO_LOG
#define FIO_CORE
#include "fio-stl/include.h"

#include <math.h>

FIO_SFUNC uintptr_t fio___dynamic_types_test_tag(uintptr_t i) { return i | 1; }
FIO_SFUNC uintptr_t fio___dynamic_types_test_untag(uintptr_t i) {
  return i & (~((uintptr_t)1UL));
}

#define FIO_TEST_REPEAT (1ULL << 12U)

static void poison_stack(void *ignr_) {
  (void)ignr_;
#define FIO___STACK_POISON_LENGTH (1ULL << 16)
  uint8_t buf[FIO___STACK_POISON_LENGTH];
  FIO_COMPILER_GUARD;
  FIO_MEMSET(buf, (int)(0xA0U), FIO___STACK_POISON_LENGTH);
  FIO_COMPILER_GUARD;
  /* Runs on every worker thread start: avoid the shared `fio_rand` state
     (intentionally unsynchronized; reported by tsan). On failure (e.g., no
     /dev/urandom on Windows) the 0xA0 fill above remains. */
  (void)fio_rand_bytes_secure(buf, FIO___STACK_POISON_LENGTH);
  FIO_COMPILER_GUARD;
  fio_trylock(buf);
  FIO_LOG_DDEBUG2("(%d) Stack Poisoned: %zu Kb",
                  getpid(),
                  (size_t)(FIO___STACK_POISON_LENGTH >> 10));
#undef FIO___STACK_POISON_LENGTH
}

FIO_CONSTRUCTOR(fio____test_stack_poisener) {
  poison_stack(NULL);
  fio_state_callback_add(FIO_CALL_ON_WORKER_THREAD_START, poison_stack, NULL);
  fio_state_callback_add(FIO_CALL_IN_CHILD, poison_stack, NULL);
}

#ifndef FIO_PRINT_SIZE_OF
#define FIO_PRINT_SIZE_OF(T)                                                   \
  fprintf(stderr, "\t\t%-19s%zu Bytes\n", #T, sizeof(T))
#endif

/* *****************************************************************************
ATOL helpers and speed testing
***************************************************************************** */

#ifndef FIO_ATOL_TEST_MAX
#define FIO_ATOL_TEST_MAX 1048576
#endif

FIO_IFUNC int64_t FIO_NAME_TEST(stl, atol_time)(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
}

FIO_SFUNC size_t sprintf_wrapper(char *dest, int64_t num, uint8_t base) {
  switch (base) {
  case 2: /* overflow - unsupported */
  case 8: /* overflow - unsupported */
  case 10: return snprintf(dest, 256, "%" PRId64, num);
  case 16:
    if (num >= 0)
      return snprintf(dest, 256, "0x%.16" PRIx64, num);
    return snprintf(dest, 256, "-0x%.8" PRIx64, (0 - num));
  }
  return snprintf(dest, 256, "%" PRId64, num);
}

FIO_SFUNC int64_t strtoll_wrapper(char **pstr) {
  return strtoll(*pstr, pstr, 0);
}
FIO_SFUNC int64_t fio_aton_wrapper(char **pstr) {
  fio_aton_s r = fio_aton(pstr);
  if (r.is_float)
    return (int64_t)r.f;
  return r.i;
}

FIO_SFUNC void FIO_NAME_TEST(stl, atol_speed)(const char *name,
                                              int64_t (*a2l)(char **),
                                              size_t (*l2a)(char *,
                                                            int64_t,
                                                            uint8_t)) {
  int64_t start;
  int64_t tw = 0;
  int64_t trt = 0;
  char buf[1024];
  struct {
    const char *str;
    const char *prefix;
    uint8_t prefix_len;
    uint8_t base;
  } * pb, b[] = {
              {.str = "Base 10", .base = 10},
              {.str = "Hex    ", .prefix = "0x", .prefix_len = 2, .base = 16},
              {.str = "Binary ", .prefix = "0b", .prefix_len = 2, .base = 2},
              // {.str = "Oct    ", .prefix = "0", .prefix_len = 1, .base = 8},
              /* end marker */
              {.str = NULL},
          };
  fprintf(stderr, "    * %s test performance:\n", name);
  if (l2a == sprintf_wrapper)
    b[2].str = NULL;
  for (pb = b; pb->str; ++pb) {
    start = FIO_NAME_TEST(stl, atol_time)();
    for (int64_t i = -FIO_ATOL_TEST_MAX; i < FIO_ATOL_TEST_MAX; ++i) {
      char *bf = buf + pb->prefix_len;
      size_t len = l2a(bf, i, pb->base);
      bf[len] = 0;
      if (bf[0] == '-') {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test + 1] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      } else {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      }
      FIO_COMPILER_GUARD; /* don't optimize this loop */
      int64_t n = a2l(&bf);
      bf = buf;
      FIO_ASSERT(n == i,
                 "roundtrip error for %s: %s != %lld (got %lld stopped: %s)",
                 name,
                 buf,
                 (long long)i,
                 (long long)a2l(&bf),
                 bf);
    }
    trt = FIO_NAME_TEST(stl, atol_time)() - start;
    start = FIO_NAME_TEST(stl, atol_time)();
    for (int64_t i = -FIO_ATOL_TEST_MAX; i < FIO_ATOL_TEST_MAX; ++i) {
      char *bf = buf + pb->prefix_len;
      size_t len = l2a(bf, i, pb->base);
      bf[len] = 0;
      if (bf[0] == '-') {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test + 1] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      } else {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      }
      FIO_COMPILER_GUARD; /* don't optimize this loop */
    }
    tw = FIO_NAME_TEST(stl, atol_time)() - start;
    // clang-format off
    fprintf(stderr, "        - %s roundtrip   %zd us\n", pb->str, (size_t)trt);
    fprintf(stderr, "        - %s write       %zd us\n", pb->str, (size_t)tw);
    fprintf(stderr, "        - %s read (calc) %zd us\n", pb->str, (size_t)(trt - tw));
    // clang-format on
  }
}

/* *****************************************************************************
Hashing speed testing
***************************************************************************** */

typedef uintptr_t (*fio__hashing_func_fn)(char *, size_t);

FIO_SFUNC void fio_test_hash_function(fio__hashing_func_fn h,
                                      char *name,
                                      uint8_t size_log,
                                      uint8_t mem_alignment_offset,
                                      uint8_t fast) {
  /* test based on code from BearSSL with credit to Thomas Pornin */
  if (size_log >= 21 || ((sizeof(uint64_t) - 1) >> size_log)) {
    FIO_LOG_ERROR("fio_test_hash_function called with a log size too big.");
    return;
  }
  mem_alignment_offset &= 7;
  size_t const buffer_len = (1ULL << size_log) - mem_alignment_offset;
  uint64_t cycles_start_at = (1ULL << (14 + (fast * 3)));
  if (size_log < 13)
    cycles_start_at <<= (13 - size_log);
  else if (size_log > 13)
    cycles_start_at >>= (size_log - 13);

#ifdef DEBUG
  fprintf(stderr,
          "\t* Testing %s speed with %zu byte blocks"
          " (DEBUG mode detected - speed may be affected).\n",
          name,
          buffer_len);
#else
  fprintf(stderr,
          "\t* Testing %s speed with %zu byte blocks.\n",
          name,
          buffer_len);
#endif

  uint8_t *buffer_mem = (uint8_t *)
      FIO_MEM_REALLOC(NULL, 0, (buffer_len + mem_alignment_offset) + 64, 0);
  uint8_t *buffer = buffer_mem + mem_alignment_offset;

  FIO_MEMSET(buffer, 'T', buffer_len);
  /* warmup */
  uint64_t hash = 0;
  for (size_t i = 0; i < 4; i++) {
    hash += h((char *)buffer, buffer_len);
    fio_memcpy8(buffer, &hash);
  }
  /* loop until test runs for more than 2 seconds */
  for (uint64_t cycles = cycles_start_at;;) {
    clock_t start, end;
    start = clock();
    for (size_t i = cycles; i > 0; i--) {
      hash += h((char *)buffer, buffer_len);
      FIO_COMPILER_GUARD;
    }
    end = clock();
    fio_memcpy8(buffer, &hash);
    if ((end - start) > CLOCKS_PER_SEC || cycles >= ((uint64_t)1 << 62)) {
      fprintf(stderr,
              "\t\t%-40s %8.2f MB/s\n",
              name,
              (double)(buffer_len * cycles) /
                  (((end - start) * (1000000.0 / CLOCKS_PER_SEC))));
      break;
    }
    cycles <<= 1;
  }
  FIO_MEM_FREE(buffer_mem, (buffer_len + mem_alignment_offset) + 64);
}

/* *****************************************************************************
Test-owned temporary folder helpers (POSIX only)
***************************************************************************** */
#if FIO_OS_POSIX
#include <dirent.h>

/** Counts the entries of folder `path` (without `.` / `..`); -1 on error. */
FIO_SFUNC long test_dir_count(const char *path) {
  DIR *dir = opendir(path);
  if (!dir)
    return -1;
  long count = 0;
  for (struct dirent *e; (e = readdir(dir));) {
    if (e->d_name[0] == '.' &&
        (!e->d_name[1] || (e->d_name[1] == '.' && !e->d_name[2])))
      continue;
    ++count;
  }
  closedir(dir);
  return count;
}

/** Sets `TMPDIR` to `path`, returning the old value (free with restore). */
FIO_SFUNC char *test_tmpdir_set(const char *path) {
  const char *old = getenv("TMPDIR");
  char *r = old ? strdup(old) : NULL;
  FIO_ASSERT(!setenv("TMPDIR", path, 1), "setenv(TMPDIR) failed");
  return r;
}

/** Restores the `TMPDIR` value returned by `test_tmpdir_set`. */
FIO_SFUNC void test_tmpdir_restore(char *old) {
  if (old) {
    setenv("TMPDIR", old, 1);
    free(old);
  } else {
    unsetenv("TMPDIR");
  }
}
#endif /* FIO_OS_POSIX */

/* *****************************************************************************
Opt-in file I/O hooks - implementation (see TEST_FD_HOOKS above)
***************************************************************************** */
#ifdef TEST_FD_HOOKS
#if defined(__linux__)
#include <sys/syscall.h>
#endif

/** `FIO___FD_PREAD` hook state (reset with `test_fd_hooks_reset`). */
static struct {
  size_t eintr;      /* the next `eintr` calls fail with EINTR */
  size_t max;        /* > 0: max bytes per call (partial reads) */
  int64_t fail_at;   /* >= 0: reads stop at this offset (short read) */
  int fail_errno;    /* at `fail_at`: 0 = EOF (returns 0), else -1 + errno */
  size_t calls;      /* calls made */
  size_t eintr_seen; /* EINTR failures injected */
  size_t bytes;      /* bytes read (successful calls) */
} test_fd_read;

/** `FIO___FILENAME_FSYNC` hook state. */
static struct {
  int fail_errno; /* != 0: fsync fails with this errno */
  size_t calls;
} test_fd_fsync;

/** `FIO___FILENAME_COPY_RANGE` hook state. */
static struct {
  int emulate;    /* 1: pread + pwrite; 0: Linux syscall (else ENOSYS) */
  size_t limit;   /* stop after `limit` bytes (total, see `fail_errno`) */
  int fail_errno; /* at `limit`: 0 = returns 0 (EOF like), else -1 + errno */
  size_t max;     /* > 0: max bytes per call */
  size_t calls;   /* calls made */
  size_t bytes;   /* bytes copied */
} test_fd_copy;

/** Resets all file I/O hooks (no injected failures). */
FIO_SFUNC void test_fd_hooks_reset(void) {
  FIO_MEMSET(&test_fd_read, 0, sizeof(test_fd_read));
  FIO_MEMSET(&test_fd_fsync, 0, sizeof(test_fd_fsync));
  FIO_MEMSET(&test_fd_copy, 0, sizeof(test_fd_copy));
  test_fd_read.fail_at = -1;
  test_fd_copy.limit = (size_t)-1;
}
FIO_CONSTRUCTOR(test_fd_hooks_init) { test_fd_hooks_reset(); }

static ssize_t test_fd_pread_hook(int fd, void *buf, size_t len, off_t at) {
  ++test_fd_read.calls;
  if (test_fd_read.eintr) {
    --test_fd_read.eintr;
    ++test_fd_read.eintr_seen;
    errno = EINTR;
    return -1;
  }
  if (test_fd_read.fail_at >= 0 && at >= 0 &&
      (int64_t)at + (int64_t)len > test_fd_read.fail_at) {
    if ((int64_t)at >= test_fd_read.fail_at) {
      if (!test_fd_read.fail_errno)
        return 0;
      errno = test_fd_read.fail_errno;
      return -1;
    }
    len = (size_t)(test_fd_read.fail_at - (int64_t)at);
  }
  if (test_fd_read.max && len > test_fd_read.max)
    len = test_fd_read.max;
  ssize_t r = fio___fd_pread(fd, buf, len, at);
  if (r > 0)
    test_fd_read.bytes += (size_t)r;
  return r;
}

static int test_fd_fsync_hook(int fd) {
  ++test_fd_fsync.calls;
  if (test_fd_fsync.fail_errno) {
    errno = test_fd_fsync.fail_errno;
    return -1;
  }
#if FIO_OS_WIN
  return _commit(fd);
#else
  return fsync(fd);
#endif
}

static ssize_t test_fd_copy_range_hook(int in,
                                       int64_t *in_at,
                                       int out,
                                       int64_t *out_at,
                                       size_t len) {
  ++test_fd_copy.calls;
  if (test_fd_copy.limit != (size_t)-1) {
    if (test_fd_copy.bytes >= test_fd_copy.limit) {
      if (!test_fd_copy.fail_errno)
        return 0;
      errno = test_fd_copy.fail_errno;
      return -1;
    }
    if (len > test_fd_copy.limit - test_fd_copy.bytes)
      len = test_fd_copy.limit - test_fd_copy.bytes;
  }
  if (test_fd_copy.max && len > test_fd_copy.max)
    len = test_fd_copy.max;
  ssize_t r = -1;
  if (test_fd_copy.emulate) {
    char tmp[4096];
    if (len > sizeof(tmp))
      len = sizeof(tmp);
    r = fio___fd_pread(in, tmp, len, (off_t)*in_at);
    if (r > 0)
      r = pwrite(out, tmp, (size_t)r, (off_t)*out_at);
    if (r > 0) {
      *in_at += r;
      *out_at += r;
    }
  } else {
#if defined(__linux__) && defined(SYS_copy_file_range)
    r = (ssize_t)syscall(SYS_copy_file_range, in, in_at, out, out_at, len, 0U);
#else
    errno = ENOSYS;
    (void)in, (void)in_at, (void)out, (void)out_at;
#endif
  }
  if (r > 0)
    test_fd_copy.bytes += (size_t)r;
  return r;
}
#endif /* TEST_FD_HOOKS */

#endif
