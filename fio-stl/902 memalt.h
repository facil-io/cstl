/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_MEMALT Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_MEMALT_TEST___H)
#define H___FIO_MEMALT_TEST___H

#ifndef H___FIO_MEMALT___H
#define FIO_MEMALT
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

FIO_SFUNC void FIO_NAME_TEST(stl, memalt)(void) {
  uint64_t start, end;
  fprintf(stderr, "* Testing memcpy, memchr and memset alternatives.\n");

  { /* test fio_memcpy possible overflow. */
    uint64_t buf1[64];
    uint8_t *buf = (uint8_t *)buf1;
    fio_memset(buf1, ~(uint64_t)0, sizeof(*buf1) * 64);
    char *data =
        (char *)"This should be an uneven amount of characters, say 53";
    fio_memcpy(buf, data, FIO_STRLEN(data));
    FIO_ASSERT(!memcmp(buf, data, FIO_STRLEN(data)) &&
                   buf[FIO_STRLEN(data)] == 0xFF,
               "fio_memcpy should not overflow or underflow on uneven "
               "amounts of bytes.");
  }
  { /* test fio_memcpy as memmove */
    fprintf(stderr, "* testing fio_memcpy with overlapping memory (memmove)\n");
    char *msg = (char *)"fio_memcpy should work also as memmove, "
                        "so undefined behavior should not occur. "
                        "Should be true for larger offsets too. At least over "
                        "128 Bytes.";
    size_t len = FIO_STRLEN(msg);
    char buf[512];
    for (size_t offset = 1; offset < len; ++offset) {
      memset(buf, 0, sizeof(buf));
      memmove(buf, msg, len);
      fio_memcpy(buf + offset, buf, len);
      FIO_ASSERT(!memcmp(buf + offset, msg, len),
                 "fio_memcpy failed on overlapping data (offset +%d, len %zu)",
                 offset,
                 len);
      memset(buf, 0, sizeof(buf));
      memmove(buf + offset, msg, len);
      fio_memcpy(buf, buf + offset, len);
      FIO_ASSERT(!memcmp(buf, msg, len),
                 "fio_memcpy failed on overlapping data (offset -%d, len %zu)",
                 offset,
                 len);
    }
  }
  { /* test fio_memcmp */
    for (size_t i = 0; i < 4096; ++i) {
      uint64_t a = fio_rand64(), b = fio_rand64();
      int s = memcmp(&a, &b, sizeof(a));
      int f = fio_memcmp(&a, &b, sizeof(a));
      FIO_ASSERT((s < 0 && f < 0) || (s > 0 && f > 0) || (!s && !f),
                 "fio_memcmp != memcmp (result meaning, not value).");
      FIO_ASSERT(fio_ct_is_eq(&a, &b, sizeof(a)) == (!s),
                 "fio_ct_is_eq differs from memcmp result");
    }
  }
  { /* test fio_memchr and fio_strlen */
    char membuf[4096];
    memset(membuf, 0xff, 4096);
    membuf[4095] = 0;
    for (size_t i = 0; i < 4095; ++i) {
      membuf[i] = 0;
      char *result = (char *)fio_memchr(membuf, 0, 4096);
      size_t len = fio_strlen(membuf);
      membuf[i] = (char)((i & 0xFFU) | 1U);
      FIO_ASSERT(result == membuf + i, "fio_memchr failed.");
      FIO_ASSERT(len == i, "fio_strlen failed.");
    }
  }
#ifndef DEBUG
  const size_t base_repetitions = 8192;
  fprintf(stderr, "* Speed testing core memcpy primitives:\n");
  {
    struct {
      void *(*fn)(void *, const void *);
      size_t bytes;
    } tests[] = {
        {fio_memcpy8, 8},
        {fio_memcpy16, 16},
        {fio_memcpy32, 32},
        {fio_memcpy64, 64},
        {fio_memcpy128, 128},
        {fio_memcpy256, 256},
        {fio_memcpy512, 512},
        {fio_memcpy1024, 1024},
        {fio_memcpy2048, 2048},
        {fio_memcpy4096, 4096},
        {NULL},
    };
    char buf[4096 * 2];
    memset(buf, 0x80, 4096 * 2);
    for (size_t i = 0; tests[i].bytes; ++i) {
      start = fio_time_micro();
      for (size_t r = 0; r < (base_repetitions << 4); ++r) {
        tests[i].fn(buf, buf + 4096);
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\tfio_memcpy%zu\tmemcpy(a,b,%zu)   \t%zuus\t",
              tests[i].bytes,
              tests[i].bytes,
              (size_t)(end - start));
      start = fio_time_micro();
      for (size_t r = 0; r < (base_repetitions << 4); ++r) {
        memcpy(buf, buf + 4096, tests[i].bytes);
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fprintf(stderr, "%zuus\n", (size_t)(end - start));
    }
  }
  {
    fprintf(stderr, "\n");
    struct {
      void *(*fn)(void *, const void *, size_t);
      size_t bytes;
    } tests[] = {
        {fio_memcpy7x, 7},
        {fio_memcpy15x, 15},
        {fio_memcpy31x, 31},
        {fio_memcpy63x, 63},
        {fio_memcpy127x, 127},
        {fio_memcpy255x, 255},
        {fio_memcpy511x, 511},
        {fio_memcpy1023x, 1023},
        {fio_memcpy2047x, 2047},
        {fio_memcpy4095x, 4095},
        {NULL},
    };
    char buf[4096 * 2];
    memset(buf, 0x80, 4096 * 2);
    for (size_t i = 0; tests[i].bytes; ++i) {
      start = fio_time_micro();
      for (size_t r = 0; r < (base_repetitions << 4); ++r) {
        tests[i].fn(buf, buf + 4096, ((tests[i].bytes + r) & tests[i].bytes));
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\tfio_memcpy%zux\tmemcpy(a,b,%zu)   \t%zuus\t",
              tests[i].bytes,
              tests[i].bytes,
              (size_t)(end - start));
      start = fio_time_micro();
      for (size_t r = 0; r < (base_repetitions << 4); ++r) {
        memcpy(buf, buf + 4096, ((tests[i].bytes + r) & tests[i].bytes));
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fprintf(stderr, "%zuus\n", (size_t)(end - start));
    }
  }
  fprintf(stderr, "* Speed testing memset:\n");

  for (size_t len_i = 5; len_i < 20; ++len_i) {
    const size_t repetitions = base_repetitions
                               << (len_i < 15 ? (15 - (len_i & 15)) : 0);
    const size_t mem_len = (1ULL << len_i);
    void *mem = malloc(mem_len + 32);
    FIO_ASSERT_ALLOC(mem);
    uint64_t sig = (uintptr_t)mem;
    sig ^= sig >> 13;
    sig ^= sig << 17;
    sig ^= sig << 29;
    sig ^= sig << 31;
    for (size_t rlen = mem_len - 1; rlen < mem_len + 2; ++rlen) {
      start = fio_time_micro();
      for (size_t i = 0; i < repetitions; ++i) {
        fio_memset(mem, sig, rlen);
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fio___memset_test_aligned(mem,
                                sig,
                                rlen,
                                "fio_memset sanity test FAILED");
      fprintf(stderr,
              "\tfio_memset\t(%zu bytes):\t%zuus\t/ %zu\n",
              rlen,
              (size_t)(end - start),
              repetitions);
      start = fio_time_micro();
      for (size_t i = 0; i < repetitions; ++i) {
        memset(mem, (int)sig, rlen);
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\tsystem memset\t(%zu bytes):\t%zuus\t/ %zu\n",
              rlen,
              (size_t)(end - start),
              repetitions);
    }
    free(mem);
  }

  fprintf(stderr, "* Speed testing memcpy:\n");

  for (int len_i = 5; len_i < 21; ++len_i) {
    const size_t repetitions = base_repetitions
                               << (len_i < 15 ? (15 - (len_i & 15)) : 0);
    for (size_t mem_len = (1ULL << len_i) - 1; mem_len <= (1ULL << len_i) + 1;
         ++mem_len) {
      void *mem = malloc(mem_len << 1);
      FIO_ASSERT_ALLOC(mem);
      uint64_t sig = (uintptr_t)mem;
      sig ^= sig >> 13;
      sig ^= sig << 17;
      sig ^= sig << 29;
      sig ^= sig << 31;
      fio_memset(mem, sig, mem_len);

      start = fio_time_micro();
      for (size_t i = 0; i < repetitions; ++i) {
        fio_memcpy((char *)mem + mem_len, mem, mem_len);
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fio___memset_test_aligned((char *)mem + mem_len,
                                sig,
                                mem_len,
                                "fio_memcpy sanity test FAILED");
      fprintf(stderr,
              "\tfio_memcpy\t(%zu bytes):\t%zuus\t/ %zu\n",
              mem_len,
              (size_t)(end - start),
              repetitions);

      // size_t threads_used = 0;
      // start = fio_time_micro();
      // for (size_t i = 0; i < repetitions; ++i) {
      //   threads_used = fio_thread_memcpy((char *)mem + mem_len, mem,
      //   mem_len); if (threads_used == 1)
      //     break;
      //   FIO_COMPILER_GUARD;
      // }
      // end = fio_time_micro();
      // fio___memset_test_aligned((char *)mem + mem_len,
      //                           sig,
      //                           mem_len,
      //                           "fio_thread_memcpy sanity test FAILED");
      // fprintf(stderr,
      //         "   fio_thread_memcpy (%zut)\t(%zu bytes):\t%zu"
      //         "us\t/ %zu\n", threads_used, mem_len, (size_t)(end
      //         - start), repetitions);

      start = fio_time_micro();
      for (size_t i = 0; i < repetitions; ++i) {
        memcpy((char *)mem + mem_len, mem, mem_len);
        FIO_COMPILER_GUARD;
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\tsystem memcpy\t(%zu bytes):\t%zuus\t/ %zu\n",
              mem_len,
              (size_t)(end - start),
              repetitions);
      free(mem);
    }
  }

  fprintf(stderr, "* Speed testing memchr:\n");

  for (int len_i = 2; len_i < 20; ++len_i) {
    const size_t repetitions = base_repetitions
                               << (len_i < 15 ? (15 - (len_i & 15)) : 0);
    const size_t mem_len = (1ULL << len_i) - 1;
    size_t token_index = ((mem_len >> 1) + (mem_len >> 2)) + 1;
    void *mem = malloc(mem_len + 1);
    FIO_ASSERT_ALLOC(mem);
    fio_memset(mem, ((uint64_t)0x0101010101010101ULL * 0x80), mem_len + 1);
    ((uint8_t *)mem)[token_index >> 1] = 0xFFU;       /* edge case? */
    ((uint8_t *)mem)[(token_index >> 1) + 1] = 0x01U; /* edge case? */
    ((uint8_t *)mem)[(token_index >> 1) + 2] = 0x7FU; /* edge case? */
    ((uint8_t *)mem)[token_index] = 0;
    ((uint8_t *)mem)[token_index + 1] = 0;
    FIO_ASSERT(memchr((char *)mem + 1, 0, mem_len) ==
                   fio_memchr((char *)mem + 1, 0, mem_len),
               "fio_memchr != memchr");
    ((uint8_t *)mem)[token_index] = (char)0x80;
    ((uint8_t *)mem)[token_index + 1] = (char)0x80;

    token_index = mem_len;
    start = fio_time_micro();
    for (size_t i = 0; i < repetitions; ++i) {
      char *result = (char *)fio_memchr((char *)mem, 0, mem_len);
      FIO_ASSERT(result == ((char *)mem + token_index) ||
                     (!result && token_index == mem_len),
                 "fio_memchr failed? @ %zu",
                 token_index);
      FIO_COMPILER_GUARD;
      ((uint8_t *)mem)[token_index] = 0x80;
      token_index = (token_index - 1) & ((1ULL << len_i) - 1);
      ((uint8_t *)mem)[token_index] = 0;
    }
    end = fio_time_micro();
    ((uint8_t *)mem)[token_index] = 0x80;
    fprintf(stderr,
            "\tfio_memchr\t(up to %zu bytes):\t%zuus\t/ %zu\n",
            mem_len,
            (size_t)(end - start),
            repetitions);

    token_index = mem_len;
    start = fio_time_micro();
    for (size_t i = 0; i < repetitions; ++i) {
      char *result = (char *)memchr((char *)mem, 0, mem_len);
      FIO_ASSERT(result == ((char *)mem + token_index) ||
                     (!result && token_index == mem_len),
                 "memchr failed? @ %zu",
                 token_index);
      FIO_COMPILER_GUARD;
      ((uint8_t *)mem)[token_index] = 0x80;
      token_index = (token_index - 1) & ((1ULL << len_i) - 1);
      ((uint8_t *)mem)[token_index] = 0;
    }
    end = fio_time_micro();
    ((uint8_t *)mem)[token_index] = 0x80;
    fprintf(stderr,
            "\tsystem memchr\t(up to %zu bytes):\t%zuus\t/ %zu\n",
            mem_len,
            (size_t)(end - start),
            repetitions);

    free(mem);
  }

  fprintf(stderr, "* Speed testing memcmp:\n");

  for (int len_i = 2; len_i < 21; ++len_i) {
    const size_t repetitions = base_repetitions
                               << (len_i < 13 ? (15 - (len_i & 15)) : 2);
    const size_t mem_len = (1ULL << len_i);
    char *mem = (char *)malloc(mem_len << 1);
    FIO_ASSERT_ALLOC(mem);
    uint64_t sig = (uintptr_t)mem;
    sig ^= sig >> 13;
    sig ^= sig << 17;
    sig ^= sig << 29;
    sig ^= sig << 31;
    fio_memset(mem, sig, mem_len);
    fio_memset(mem + mem_len, sig, mem_len);
    size_t twister = 0;

    FIO_ASSERT(!fio_memcmp(mem + mem_len, mem, mem_len),
               "fio_memcmp sanity test FAILED (%zu eq)",
               mem_len);
    FIO_ASSERT(fio_ct_is_eq(mem + mem_len, mem, mem_len),
               "fio_ct_is_eq sanity test FAILED (%zu eq)",
               mem_len);
    {
      mem[mem_len - 2]--;
      int r1 = fio_memcmp(mem + mem_len, mem, mem_len);
      int r2 = memcmp(mem + mem_len, mem, mem_len);
      FIO_ASSERT((r1 > 0 && r2 > 0) | (r1 < 0 && r2 < 0),
                 "fio_memcmp sanity test FAILED (%zu !eq)",
                 mem_len);
      FIO_ASSERT(!fio_ct_is_eq(mem + mem_len, mem, mem_len),
                 "fio_ct_is_eq sanity test FAILED (%zu !eq)",
                 mem_len);
      mem[mem_len - 2]++;
    }

    twister = mem_len - 3;
    start = fio_time_micro();
    for (size_t i = 0; i < repetitions; ++i) {
      int cmp = fio_memcmp(mem + mem_len, mem, mem_len);
      FIO_COMPILER_GUARD;
      if (cmp) {
        ++mem[twister--];
        twister &= ((1ULL << (len_i - 1)) - 1);
      } else {
        --mem[twister];
      }
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\tfio_memcmp\t(up to %zu bytes):\t%zuus\t/ %zu\n",
            mem_len,
            (size_t)(end - start),
            repetitions);

    FIO_MEMCPY(mem,
               mem + mem_len,
               mem_len); /* shouldn't be needed, but anyway */

    twister = mem_len - 3;
    start = fio_time_micro();
    for (size_t i = 0; i < repetitions; ++i) {
      int cmp = memcmp(mem + mem_len, mem, mem_len);
      FIO_COMPILER_GUARD;
      if (cmp) {
        ++mem[twister--];
        twister &= ((1ULL << (len_i - 1)) - 1);
      } else {
        --mem[twister];
      }
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\tsystem memcmp\t(up to %zu bytes):\t%zuus\t/ %zu\n",
            mem_len,
            (size_t)(end - start),
            repetitions);

    twister = mem_len - 3;
    start = fio_time_micro();
    for (size_t i = 0; i < repetitions; ++i) {
      int cmp = fio_ct_is_eq(mem + mem_len, mem, mem_len);
      FIO_COMPILER_GUARD;
      if (!cmp) {
        ++mem[twister--];
        twister &= ((1ULL << (len_i - 1)) - 1);
      } else {
        --mem[twister];
      }
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\tfio_ct_is_eq\t(up to %zu bytes):\t%zuus\t/ %zu\n",
            mem_len,
            (size_t)(end - start),
            repetitions);

    free(mem);
  }

  fprintf(stderr, "* Speed testing strlen:\n");

  for (int len_i = 2; len_i < 20; ++len_i) {
    const size_t repetitions = base_repetitions
                               << (len_i < 15 ? (15 - (len_i & 15)) : 0);
    const size_t mem_len = (1ULL << len_i) - 1;
    size_t token_index = ((mem_len >> 1) + (mem_len >> 2)) + 1;
    void *mem = malloc(mem_len + 1);
    FIO_ASSERT_ALLOC(mem);
    fio_memset(mem, ((uint64_t)0x0101010101010101ULL * 0x80), mem_len + 1);
    ((uint8_t *)mem)[token_index >> 1] = 0xFFU;       /* edge case? */
    ((uint8_t *)mem)[(token_index >> 1) + 1] = 0x01U; /* edge case? */
    ((uint8_t *)mem)[(token_index >> 1) + 2] = 0x7FU; /* edge case? */
    ((uint8_t *)mem)[token_index] = 0;
    ((uint8_t *)mem)[token_index + 1] = 0;
    FIO_ASSERT(fio_strlen((char *)mem + 1) == strlen((char *)mem + 1),
               "fio_strlen != strlen");
    FIO_ASSERT(fio_strlen((char *)mem) == strlen((char *)mem),
               "fio_strlen != strlen");
    ((uint8_t *)mem)[token_index] = 0x80U;
    ((uint8_t *)mem)[token_index + 1] = 0x80U;
    ((uint8_t *)mem)[mem_len] = 0;
    FIO_ASSERT(fio_strlen((char *)mem) == strlen((char *)mem) &&
                   fio_strlen((char *)mem) == mem_len,
               "fio_strlen != strlen");

    token_index = mem_len - 1;
    ((uint8_t *)mem)[token_index] = 0;
    start = fio_time_micro();
    for (size_t i = 0; i < repetitions; ++i) {
      size_t result = fio_strlen((char *)mem);
      FIO_ASSERT(result == token_index,
                 "fio_strlen failed? @ %zu",
                 token_index);
      FIO_COMPILER_GUARD;
      ((uint8_t *)mem)[token_index] = 0x80;
      token_index = (token_index - 1) & ((1ULL << len_i) - 1);
      token_index -= token_index == mem_len;
      ((uint8_t *)mem)[token_index] = 0;
    }
    end = fio_time_micro();
    ((uint8_t *)mem)[token_index] = 0x80;
    fprintf(stderr,
            "\tfio_strlen\t(up to %zu bytes):\t%zuus\t/ %zu\n",
            mem_len,
            (size_t)(end - start),
            repetitions);

    token_index = mem_len - 1;
    ((uint8_t *)mem)[token_index] = 0;
    start = fio_time_micro();
    for (size_t i = 0; i < repetitions; ++i) {
      size_t result = strlen((char *)mem);
      FIO_ASSERT(result == token_index, "strlen failed? @ %zu", token_index);
      FIO_COMPILER_GUARD;
      ((uint8_t *)mem)[token_index] = 0x80;
      token_index = (token_index - 1) & ((1ULL << len_i) - 1);
      token_index -= (token_index == mem_len);
      ((uint8_t *)mem)[token_index] = 0;
    }
    end = fio_time_micro();
    ((uint8_t *)mem)[token_index] = 0x80;
    fprintf(stderr,
            "\tsystem strlen\t(up to %zu bytes):\t%zuus\t/ %zu\n",
            mem_len,
            (size_t)(end - start),
            repetitions);

    free(mem);
  }
#endif /* DEBUG */
  ((void)start), ((void)end);
}
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
