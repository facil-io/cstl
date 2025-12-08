#ifndef H___TEST_HELPERS___H
#define H___TEST_HELPERS___H

#define FIO_TEST_ALL
#define FIO_CORE
#include "../fio-stl/include.h"

#include <math.h>

FIO_SFUNC uintptr_t fio___dynamic_types_test_tag(uintptr_t i) { return i | 1; }
FIO_SFUNC uintptr_t fio___dynamic_types_test_untag(uintptr_t i) {
  return i & (~((uintptr_t)1UL));
}

#define FIO_TEST_REPEAT (1ULL << 12U)

FIO_CONSTRUCTOR(fio____test_stack_poisener) {
#define FIO___STACK_POISON_LENGTH (1ULL << 18)
  uint8_t buf[FIO___STACK_POISON_LENGTH];
  FIO_COMPILER_GUARD;
  FIO_MEMSET(buf, (int)(0xA0U), FIO___STACK_POISON_LENGTH);
  FIO_COMPILER_GUARD;
  fio_rand_bytes(buf, FIO___STACK_POISON_LENGTH);
  FIO_COMPILER_GUARD;
  fio_trylock(buf);
#undef FIO___STACK_POISON_LENGTH
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
                 i,
                 a2l(&bf),
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

#endif
