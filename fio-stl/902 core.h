/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                Core Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_CORE_TEST___H)
#define H___FIO_CORE_TEST___H

FIO_SFUNC void FIO_NAME_TEST(stl, core)(void) {
  fprintf(stderr, "* Testing fio_memcpy primitives.\n");
  {
    struct {
      void *(*fn)(void *, const void *, size_t);
      const char *name;
      size_t len;
    } tests[] = {
        {fio_memcpy7x, "fio_memcpy7x", 7},
        {fio_memcpy15x, "fio_memcpy15x", 15},
        {fio_memcpy31x, "fio_memcpy31x", 31},
        {fio_memcpy63x, "fio_memcpy63x", 63},
        {fio_memcpy127x, "fio_memcpy127x", 127},
        {fio_memcpy255x, "fio_memcpy255x", 255},
        {fio_memcpy511x, "fio_memcpy511x", 511},
        {fio_memcpy1023x, "fio_memcpy1023x", 1023},
        {fio_memcpy2047x, "fio_memcpy2047x", 2047},
        {fio_memcpy4095x, "fio_memcpy4095x", 4095},
        {NULL},
    };
    char buf[(4096 << 1) + 64];
    fio_rand_bytes(buf + (4096 + 32), (4096 + 32));
    for (size_t ifn = 0; tests[ifn].fn; ++ifn) {
      /* test all x primitives */
      size_t len = tests[ifn].len;
      for (size_t i = 0; i < 31; ++i) {
        memset(buf, 0, 4096 + 32);
        buf[i + len] = '\xFF';
        tests[ifn].fn(buf + i, buf + (4096 + 32), len);
        FIO_ASSERT(!memcmp(buf + i, buf + (4096 + 32), len),
                   "%s failed @ %zu\n",
                   tests[ifn].name,
                   i);
        FIO_ASSERT(fio_ct_is_eq(buf + i, buf + (4096 + 32), len),
                   "fio_ct_is_eq claims that %s failed @ %zu\n",
                   tests[ifn].name,
                   i);
        FIO_ASSERT(buf[i + len] == '\xFF', "%s overflow?", tests[ifn].name);
      }
    }
  }
  fprintf(stderr, "* Testing fio_bswapX macros.\n");
  FIO_ASSERT(fio_bswap16(0x0102) == (uint16_t)0x0201, "fio_bswap16 failed");
  FIO_ASSERT(fio_bswap32(0x01020304) == (uint32_t)0x04030201,
             "fio_bswap32 failed");
  FIO_ASSERT(fio_bswap64(0x0102030405060708ULL) == 0x0807060504030201ULL,
             "fio_bswap64 failed");

  fprintf(stderr, "* Testing fio_lrotX and fio_rrotX macros.\n");
  {
    uint64_t tmp = 1;
    tmp = FIO_RROT(tmp, 1);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << ((sizeof(uint64_t) << 3) - 1)),
               "fio_rrot failed");
    tmp = FIO_LROT(tmp, 3);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot failed");
    tmp = 1;
    tmp = fio_rrot32(tmp, 1);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 31), "fio_rrot32 failed");
    tmp = fio_lrot32(tmp, 3);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot32 failed");
    tmp = 1;
    tmp = fio_rrot64(tmp, 1);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 63), "fio_rrot64 failed");
    tmp = fio_lrot64(tmp, 3);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot64 failed");
  }
  for (size_t i = 0; i < 63; ++i) {
#if !defined(__has_builtin) || !__has_builtin(__builtin_ctzll) ||              \
    !__has_builtin(__builtin_clzll)
    FIO_ASSERT(fio___single_bit_index_unsafe((1ULL << i)) == i,
               "bit index map[%zu] error != %zu",
               (size_t)(1ULL << i),
               i);
#endif
    FIO_ASSERT(fio_bits_msb_index(((1ULL << i) | 1)) == i,
               "fio_bits_msb_index(%zu) != %zu",
               ((1ULL << i)),
               (size_t)fio_bits_msb_index(((1ULL << i) | 1)));
    FIO_ASSERT(fio_bits_lsb_index(((~0ULL) << i)) == i,
               "fio_bits_lsb_index(%zu) != %zu",
               1,
               (size_t)fio_bits_lsb_index(((~0ULL) << i)));
  }

  fprintf(stderr, "* Testing fio_buf2uX and fio_u2bufX helpers.\n");
#define FIO___BITMAP_TEST_BITS(itype, utype, bits)                             \
  for (size_t i = 0; i < (bits); ++i) {                                        \
    char tmp_buf[32];                                                          \
    itype n = ((utype)1 << i);                                                 \
    FIO_NAME2(fio_u, buf##bits##u)(tmp_buf, n);                                \
    itype r = FIO_NAME2(fio_buf, u##bits##u)(tmp_buf);                         \
    FIO_ASSERT(r == n,                                                         \
               "roundtrip failed for U" #bits " at bit %zu\n\t%zu != %zu",     \
               i,                                                              \
               (size_t)n,                                                      \
               (size_t)r);                                                     \
    FIO_ASSERT(!memcmp(tmp_buf, &n, (bits) >> 3),                              \
               "memory ordering implementation error for U" #bits "!");        \
  }
  FIO___BITMAP_TEST_BITS(int8_t, uint8_t, 8);
  FIO___BITMAP_TEST_BITS(int16_t, uint16_t, 16);
  FIO___BITMAP_TEST_BITS(int32_t, uint32_t, 32);
  FIO___BITMAP_TEST_BITS(int64_t, uint64_t, 64);
#undef FIO___BITMAP_TEST_BITS

  fprintf(stderr, "* Testing constant-time helpers.\n");
  FIO_ASSERT(fio_ct_true(0) == 0, "fio_ct_true(0) should be zero!");
  for (uintptr_t i = 1; i; i <<= 1) {
    FIO_ASSERT(fio_ct_true(i) == 1,
               "fio_ct_true(%p) should be true!",
               (void *)i);
  }
  for (uintptr_t i = 1; i + 1 != 0; i = (i << 1) | 1) {
    FIO_ASSERT(fio_ct_true(i) == 1,
               "fio_ct_true(%p) should be true!",
               (void *)i);
  }
  FIO_ASSERT(fio_ct_true(((uintptr_t)~0ULL)) == 1,
             "fio_ct_true(%p) should be true!",
             (void *)(uintptr_t)(~0ULL));

  FIO_ASSERT(fio_ct_false(0) == 1, "fio_ct_false(0) should be true!");
  for (uintptr_t i = 1; i; i <<= 1) {
    FIO_ASSERT(fio_ct_false(i) == 0,
               "fio_ct_false(%p) should be zero!",
               (void *)i);
  }
  for (uintptr_t i = 1; i + 1 != 0; i = (i << 1) | 1) {
    FIO_ASSERT(fio_ct_false(i) == 0,
               "fio_ct_false(%p) should be zero!",
               (void *)i);
  }
  FIO_ASSERT(fio_ct_false(((uintptr_t)~0ULL)) == 0,
             "fio_ct_false(%p) should be zero!",
             (void *)(uintptr_t)(~0ULL));
  FIO_ASSERT(fio_ct_true(8), "fio_ct_true should be true.");
  FIO_ASSERT(!fio_ct_true(0), "fio_ct_true should be false.");
  FIO_ASSERT(!fio_ct_false(8), "fio_ct_false should be false.");
  FIO_ASSERT(fio_ct_false(0), "fio_ct_false should be true.");
  FIO_ASSERT(fio_ct_if_bool(0, 1, 2) == 2,
             "fio_ct_if_bool selection error (false).");
  FIO_ASSERT(fio_ct_if_bool(1, 1, 2) == 1,
             "fio_ct_if_bool selection error (true).");
  FIO_ASSERT(fio_ct_if(0, 1, 2) == 2, "fio_ct_if selection error (false).");
  FIO_ASSERT(fio_ct_if(8, 1, 2) == 1, "fio_ct_if selection error (true).");
  FIO_ASSERT(fio_ct_max(1, 2) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(2, 1) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-1, 2) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(2, -1) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(1, -2) == 1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-2, 1) == 1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-1, -2) == -1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-2, -1) == -1, "fio_ct_max error.");
  {
    uint8_t bitmap[1024];
    FIO_MEMSET(bitmap, 0, 1024);
    fprintf(stderr, "* Testing bitmap helpers.\n");
    FIO_ASSERT(!fio_bit_get(bitmap, 97), "fio_bit_get should be 0.");
    fio_bit_set(bitmap, 97);
    FIO_ASSERT(fio_bit_get(bitmap, 97) == 1,
               "fio_bit_get should be 1 after being set");
    FIO_ASSERT(!fio_bit_get(bitmap, 96),
               "other bits shouldn't be effected by set.");
    FIO_ASSERT(!fio_bit_get(bitmap, 98),
               "other bits shouldn't be effected by set.");
    fio_bit_flip(bitmap, 96);
    fio_bit_flip(bitmap, 97);
    FIO_ASSERT(!fio_bit_get(bitmap, 97), "fio_bit_get should be 0 after flip.");
    FIO_ASSERT(fio_bit_get(bitmap, 96) == 1,
               "other bits shouldn't be effected by flip");
    fio_bit_unset(bitmap, 96);
    fio_bit_flip(bitmap, 97);
    FIO_ASSERT(!fio_bit_get(bitmap, 96),
               "fio_bit_get should be 0 after unset.");
    FIO_ASSERT(fio_bit_get(bitmap, 97) == 1,
               "other bits shouldn't be effected by unset");
    fio_bit_unset(bitmap, 96);
  }
  {
    uint8_t bitmap[1024];
    FIO_MEMSET(bitmap, 0, 1024);
    fprintf(stderr, "* Testing atomic bitmap helpers.\n");
    FIO_ASSERT(!fio_atomic_bit_get(bitmap, 97),
               "fio_atomic_bit_get should be 0.");
    fio_atomic_bit_set(bitmap, 97);
    FIO_ASSERT(fio_atomic_bit_get(bitmap, 97) == 1,
               "fio_atomic_bit_get should be 1 after being set");
    FIO_ASSERT(!fio_atomic_bit_get(bitmap, 96),
               "other bits shouldn't be effected by set.");
    FIO_ASSERT(!fio_atomic_bit_get(bitmap, 98),
               "other bits shouldn't be effected by set.");
    fio_atomic_bit_flip(bitmap, 96);
    fio_atomic_bit_flip(bitmap, 97);
    FIO_ASSERT(!fio_atomic_bit_get(bitmap, 97),
               "fio_atomic_bit_get should be 0 after flip.");
    FIO_ASSERT(fio_atomic_bit_get(bitmap, 96) == 1,
               "other bits shouldn't be effected by flip");
    fio_atomic_bit_unset(bitmap, 96);
    fio_atomic_bit_flip(bitmap, 97);
    FIO_ASSERT(!fio_atomic_bit_get(bitmap, 96),
               "fio_atomic_bit_get should be 0 after unset.");
    FIO_ASSERT(fio_atomic_bit_get(bitmap, 97) == 1,
               "other bits shouldn't be effected by unset");
    fio_atomic_bit_unset(bitmap, 96);
  }
  {
    fprintf(stderr, "* Testing popcount and hemming distance calculation.\n");
    for (int i = 0; i < 64; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)1 << i) == 1,
                 "fio_popcount error for 1 bit");
    }
    for (int i = 0; i < 63; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)3 << i) == 2,
                 "fio_popcount error for 2 bits");
    }
    for (int i = 0; i < 62; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)7 << i) == 3,
                 "fio_popcount error for 3 bits");
    }
    for (int i = 0; i < 59; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)21 << i) == 3,
                 "fio_popcount error for 3 alternating bits");
    }
    for (int i = 0; i < 64; ++i) {
      FIO_ASSERT(fio_hemming_dist(((uint64_t)1 << i) - 1, 0) == i,
                 "fio_hemming_dist error at %d",
                 i);
    }
  }
  {
    struct test_s {
      int a;
      char force_padding;
      int b;
    } stst = {.a = 1};

    struct test_s *stst_p = FIO_PTR_FROM_FIELD(struct test_s, b, &stst.b);
    FIO_ASSERT(stst_p == &stst, "FIO_PTR_FROM_FIELD failed to retrace pointer");
  }
  {
    fprintf(stderr, "* Testing fio_xmask.\n");
    char data[128], buf[256];
    uint64_t mask;
    uint64_t counter;
    do {
      mask = fio_rand64();
      counter = fio_rand64();
    } while (fio_has_zero_byte64(mask) || !counter);
    fio_rand_bytes(data, 128);
    const size_t len = 127;
    for (uint8_t i = 0; i < 16; ++i) {
      FIO_MEMCPY(buf + i, data, len);
      buf[len + i] = '\xFF';
      fio_xmask(buf + i, len, mask);
      FIO_ASSERT(buf[len + i] == '\xFF', "fio_xmask overflow?");
      FIO_ASSERT(memcmp(buf + i, data, len), "fio_xmask masking error");
      FIO_ASSERT(memcmp(buf + i, data, 8), "fio_xmask didn't mask data head?");
      FIO_ASSERT(
          !(len & 7) ||
              memcmp(buf + i + (len & (~7U)), data + (len & (~7U)), (len & 7)),
          "fio_xmask mask didn't mask data's tail?");
      fio_xmask(buf + i, len, mask);
      FIO_ASSERT(!memcmp(buf + i, data, len), "fio_xmask rountrip error");
      fio_xmask(buf + i, len, mask);
      FIO_MEMMOVE(buf + i + 1, buf + i, len);
      fio_xmask(buf + i + 1, len, mask);
      FIO_ASSERT(!memcmp(buf + i + 1, data, len),
                 "fio_xmask rountrip (with move) error");
    }
  }
}
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
