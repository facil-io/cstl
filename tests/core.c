/* *****************************************************************************
Test - Core Module
Covers 000 core.h, atomics, type-size sanity, state callbacks, and logging.
***************************************************************************** */
#define FIO_LOG
#define FIO___DIFF_INDEX                                                       \
  1                 /* Exercise the experimental first-difference helper. */
#define FIO_THREADS /* Enable thread type declarations for size checks. */
#include "test-helpers.h"

/* *****************************************************************************
Functional Tests
***************************************************************************** */

FIO_SFUNC void fio___test_core_memcpy_primitives(void) {
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
        {.fn = NULL},
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
        FIO_ASSERT(fio_buf_info_is_eq(FIO_BUF_INFO2(buf + i, len),
                                      FIO_BUF_INFO2(buf + (4096 + 32), len)),
                   "fio_buf_info_is_eq claims that %s failed @ %zu\n",
                   tests[ifn].name,
                   i);
        FIO_ASSERT(buf[i + len] == '\xFF', "%s overflow?", tests[ifn].name);
      }
    }
  }
}

FIO_SFUNC void fio___test_core_byte_swap(void) {
  FIO_ASSERT(fio_bswap16(0x0102) == (uint16_t)0x0201, "fio_bswap16 failed");
  FIO_ASSERT(fio_bswap32(0x01020304) == (uint32_t)0x04030201,
             "fio_bswap32 failed");
  FIO_ASSERT(fio_bswap64(0x0102030405060708ULL) == 0x0807060504030201ULL,
             "fio_bswap64 failed");
#ifdef __SIZEOF_INT128__
  {
    __uint128_t val =
        ((__uint128_t)0x0102030405060708ULL << 64) | 0x090A0B0C0D0E0F10ULL;
    __uint128_t expected =
        ((__uint128_t)0x100F0E0D0C0B0A09ULL << 64) | 0x0807060504030201ULL;
    FIO_ASSERT(fio_bswap128(val) == expected, "fio_bswap128 failed");
  }
#endif
}

FIO_SFUNC void fio___test_core_bit_rotation(void) {
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
    tmp = fio_rrot32((uint32_t)tmp, 1);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 31), "fio_rrot32 failed");
    tmp = fio_lrot32((uint32_t)tmp, 3);
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
  /* Test 8-bit and 16-bit rotations */
  {
    uint8_t v8 = 1;
    v8 = fio_rrot8(v8, 1);
    FIO_ASSERT(v8 == 0x80, "fio_rrot8 failed");
    v8 = fio_lrot8(v8, 2);
    FIO_ASSERT(v8 == 0x02, "fio_lrot8 failed");

    uint16_t v16 = 1;
    v16 = fio_rrot16(v16, 1);
    FIO_ASSERT(v16 == 0x8000, "fio_rrot16 failed");
    v16 = fio_lrot16(v16, 2);
    FIO_ASSERT(v16 == 0x0002, "fio_lrot16 failed");
  }
}

FIO_SFUNC void fio___test_core_bit_index(void) {
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
               (size_t)(1ULL << i),
               (size_t)fio_bits_msb_index(((1ULL << i) | 1)));
    FIO_ASSERT(fio_bits_lsb_index(((~0ULL) << i)) == i,
               "fio_bits_lsb_index(%zu) != %zu",
               (size_t)1,
               (size_t)fio_bits_lsb_index(((~0ULL) << i)));
  }
  /* Test fio_bits_lsb and fio_bits_msb */
  FIO_ASSERT(fio_bits_lsb(0x0F00) == 0x0100, "fio_bits_lsb failed");
  FIO_ASSERT(fio_bits_msb(0x0F00) == 0x0800, "fio_bits_msb failed");
  FIO_ASSERT(fio_bits_lsb(0) == 0, "fio_bits_lsb(0) should be 0");
  FIO_ASSERT(fio_bits_msb(0) == 0, "fio_bits_msb(0) should be 0");
}

FIO_SFUNC void fio___test_core_buf_helpers(void) {
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
}

FIO_SFUNC void fio___test_core_ct_operations(void) {
  /* fio_ct_true */
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

  /* fio_ct_false */
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

  /* fio_ct_if_bool and fio_ct_if */
  FIO_ASSERT(fio_ct_if_bool(0, 1, 2) == 2,
             "fio_ct_if_bool selection error (false).");
  FIO_ASSERT(fio_ct_if_bool(1, 1, 2) == 1,
             "fio_ct_if_bool selection error (true).");
  FIO_ASSERT(fio_ct_if(0, 1, 2) == 2, "fio_ct_if selection error (false).");
  FIO_ASSERT(fio_ct_if(8, 1, 2) == 1, "fio_ct_if selection error (true).");

  /* fio_ct_max */
  FIO_ASSERT(fio_ct_max(1, 2) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(2, 1) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-1, 2) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(2, -1) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(1, -2) == 1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-2, 1) == 1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-1, -2) == -1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-2, -1) == -1, "fio_ct_max error.");

  /* fio_ct_min */
  FIO_ASSERT(fio_ct_min(1, 2) == 1, "fio_ct_min(1,2) error.");
  FIO_ASSERT(fio_ct_min(2, 1) == 1, "fio_ct_min(2,1) error.");
  FIO_ASSERT(fio_ct_min(-1, 2) == -1, "fio_ct_min(-1,2) error.");
  FIO_ASSERT(fio_ct_min(2, -1) == -1, "fio_ct_min(2,-1) error.");
  FIO_ASSERT(fio_ct_min(1, -2) == -2, "fio_ct_min(1,-2) error.");
  FIO_ASSERT(fio_ct_min(-2, 1) == -2, "fio_ct_min(-2,1) error.");
  FIO_ASSERT(fio_ct_min(-1, -2) == -2, "fio_ct_min(-1,-2) error.");
  FIO_ASSERT(fio_ct_min(-2, -1) == -2, "fio_ct_min(-2,-1) error.");

  /* fio_ct_abs */
  FIO_ASSERT(fio_ct_abs(5) == 5, "fio_ct_abs(5) error.");
  FIO_ASSERT(fio_ct_abs(-5) == 5, "fio_ct_abs(-5) error.");
  FIO_ASSERT(fio_ct_abs(0) == 0, "fio_ct_abs(0) error.");
  FIO_ASSERT(fio_ct_abs(INTMAX_MAX) == (uintmax_t)INTMAX_MAX,
             "fio_ct_abs(INTMAX_MAX) error.");
}

FIO_SFUNC void fio___test_core_ct_bitwise(void) {
  /* fio_ct_mux32/64 - choose: (x & y) ^ (~x & z) */
  FIO_ASSERT(fio_ct_mux32(0xFFFFFFFF, 0xAAAAAAAA, 0x55555555) == 0xAAAAAAAA,
             "fio_ct_mux32 all-ones selector failed");
  FIO_ASSERT(fio_ct_mux32(0x00000000, 0xAAAAAAAA, 0x55555555) == 0x55555555,
             "fio_ct_mux32 all-zeros selector failed");
  FIO_ASSERT(fio_ct_mux32(0xF0F0F0F0, 0xAAAAAAAA, 0x55555555) == 0xA5A5A5A5,
             "fio_ct_mux32 mixed selector failed");

  FIO_ASSERT(fio_ct_mux64(0xFFFFFFFFFFFFFFFFULL,
                          0xAAAAAAAAAAAAAAAAULL,
                          0x5555555555555555ULL) == 0xAAAAAAAAAAAAAAAAULL,
             "fio_ct_mux64 all-ones selector failed");
  FIO_ASSERT(fio_ct_mux64(0x0000000000000000ULL,
                          0xAAAAAAAAAAAAAAAAULL,
                          0x5555555555555555ULL) == 0x5555555555555555ULL,
             "fio_ct_mux64 all-zeros selector failed");

  /* fio_ct_maj32/64 - majority: 1 if 2+ inputs have 1 */
  FIO_ASSERT(fio_ct_maj32(0xFFFFFFFF, 0xFFFFFFFF, 0x00000000) == 0xFFFFFFFF,
             "fio_ct_maj32 (1,1,0) failed");
  FIO_ASSERT(fio_ct_maj32(0xFFFFFFFF, 0x00000000, 0x00000000) == 0x00000000,
             "fio_ct_maj32 (1,0,0) failed");
  FIO_ASSERT(fio_ct_maj32(0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0) == 0xE8E8E8E8,
             "fio_ct_maj32 mixed failed");

  FIO_ASSERT(fio_ct_maj64(0xFFFFFFFFFFFFFFFFULL,
                          0xFFFFFFFFFFFFFFFFULL,
                          0x0000000000000000ULL) == 0xFFFFFFFFFFFFFFFFULL,
             "fio_ct_maj64 (1,1,0) failed");

  /* fio_ct_xor3_32/64 - parity: x ^ y ^ z */
  FIO_ASSERT(fio_ct_xor3_32(0xAAAAAAAA, 0x55555555, 0x00000000) == 0xFFFFFFFF,
             "fio_ct_xor3_32 failed");
  FIO_ASSERT(fio_ct_xor3_64(0xAAAAAAAAAAAAAAAAULL,
                            0x5555555555555555ULL,
                            0x0000000000000000ULL) == 0xFFFFFFFFFFFFFFFFULL,
             "fio_ct_xor3_64 failed");
}

FIO_SFUNC void fio___test_core_bitmap(void) {
  uint8_t bitmap[1024];
  FIO_MEMSET(bitmap, 0, 1024);
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
  FIO_ASSERT(!fio_bit_get(bitmap, 96), "fio_bit_get should be 0 after unset.");
  FIO_ASSERT(fio_bit_get(bitmap, 97) == 1,
             "other bits shouldn't be effected by unset");
  fio_bit_unset(bitmap, 96);
}

FIO_SFUNC void fio___test_core_atomic_bitmap(void) {
  uint8_t bitmap[1024];
  FIO_MEMSET(bitmap, 0, 1024);
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

FIO_SFUNC void fio___test_core_popcount(void) {
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

FIO_SFUNC void fio___test_core_has_byte(void) {
  /* fio_has_zero_byte32 */
  FIO_ASSERT(fio_has_zero_byte32(0x00112233) != 0,
             "fio_has_zero_byte32 should detect zero byte");
  FIO_ASSERT(fio_has_zero_byte32(0x11223344) == 0,
             "fio_has_zero_byte32 should not detect zero byte");
  FIO_ASSERT(fio_has_zero_byte32(0x11002233) != 0,
             "fio_has_zero_byte32 should detect middle zero byte");

  /* fio_has_zero_byte64 */
  FIO_ASSERT(fio_has_zero_byte64(0x0011223344556677ULL) != 0,
             "fio_has_zero_byte64 should detect zero byte");
  FIO_ASSERT(fio_has_zero_byte64(0x1122334455667788ULL) == 0,
             "fio_has_zero_byte64 should not detect zero byte");
  FIO_ASSERT(fio_has_zero_byte64(0x1122334400667788ULL) != 0,
             "fio_has_zero_byte64 should detect middle zero byte");

  /* fio_has_byte32 */
  FIO_ASSERT(fio_has_byte32(0x11223344, 0x22) != 0,
             "fio_has_byte32 should detect byte 0x22");
  FIO_ASSERT(fio_has_byte32(0x11223344, 0x55) == 0,
             "fio_has_byte32 should not detect byte 0x55");

  /* fio_has_byte64 */
  FIO_ASSERT(fio_has_byte64(0x1122334455667788ULL, 0x55) != 0,
             "fio_has_byte64 should detect byte 0x55");
  FIO_ASSERT(fio_has_byte64(0x1122334455667788ULL, 0x99) == 0,
             "fio_has_byte64 should not detect byte 0x99");
}

FIO_SFUNC void fio___test_core_ptr_from_field(void) {
  struct test_s {
    int a;
    char force_padding;
    int b;
  } stst = {.a = 1};

  struct test_s *stst_p = FIO_PTR_FROM_FIELD(struct test_s, b, &stst.b);
  FIO_ASSERT(stst_p == &stst, "FIO_PTR_FROM_FIELD failed to retrace pointer");
}

FIO_SFUNC void fio___test_core_xmask(void) {
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

FIO_SFUNC void fio___test_core_utf8(void) {
  struct {
    const char *buf;
    size_t clen;
    bool expect_fail;
  } utf8_core_tests[] = {
      {"\xf0\x9f\x92\x85", .clen = 4},
      {"\xf0\x9f\x92\x95", .clen = 4},
      {"\xe2\x9d\xa4", .clen = 3},
      {"\xE1\x9A\x80", .clen = 3},
      {"\xE2\x80\x80", .clen = 3},
      {"\xE2\x80\x81", .clen = 3},
      {"\xE2\x80\x82", .clen = 3},
      {"\xE2\x80\x83", .clen = 3},
      {"\xE2\x80\x84", .clen = 3},
      {"\xE2\x80\x85", .clen = 3},
      {"\xE2\x80\x86", .clen = 3},
      {"\xE2\x80\x87", .clen = 3},
      {"\xE2\x80\x88", .clen = 3},
      {"\xE2\x80\x89", .clen = 3},
      {"\xE2\x80\x8A", .clen = 3},
      {"\xE2\x80\xA8", .clen = 3},
      {"\xE2\x80\xA9", .clen = 3},
      {"\xE2\x80\xAF", .clen = 3},
      {"\xE2\x81\x9F", .clen = 3},
      {"\xE3\x80\x80", .clen = 3},
      {"\xEF\xBB\xBF", .clen = 3},
      {"\xc6\x92", .clen = 2},
      {"\xC2\xA0", .clen = 2},
      {"\x09", .clen = 1},
      {"\x0A", .clen = 1},
      {"\x0B", .clen = 1},
      {"\x0C", .clen = 1},
      {"\x0D", .clen = 1},
      {"\x20", .clen = 1},
      {"Z", .clen = 1},
      {"\0", .clen = 1},
      {"\xf0\x9f\x92\x35", 4, 1},
      {"\xf0\x9f\x32\x95", 4, 1},
      {"\xf0\x3f\x92\x95", 4, 1},
      {"\xFE\x9f\x92\x95", 4, 1},
      {"\xE1\x9A\x30", 3, 1},
      {"\xE1\x3A\x80", 3, 1},
      {"\xf0\x9A\x80", 3, 1},
      {"\xc6\x32", 2, 1},
      {"\xf0\x92", 2, 1},
      {0},
  };
  for (size_t i = 0; utf8_core_tests[i].buf; ++i) {
    char *pos = (char *)utf8_core_tests[i].buf;
    FIO_ASSERT(utf8_core_tests[i].expect_fail ||
                   (size_t)fio_utf8_char_len(pos) == utf8_core_tests[i].clen,
               "fio_utf8_char_len failed on %s ([%zu] == %X), %d != %u",
               utf8_core_tests[i].buf,
               i,
               (unsigned)(uint8_t)utf8_core_tests[i].buf[0],
               (int)fio_utf8_char_len(pos),
               (unsigned)utf8_core_tests[i].clen);
    uint32_t value = 0, validate = 0;
    void *tst_str = NULL;
    fio_memcpy7x(&tst_str, utf8_core_tests[i].buf, utf8_core_tests[i].clen);
#if __LITTLE_ENDIAN__
    tst_str = (void *)(uintptr_t)fio_lton32((uint32_t)(uintptr_t)tst_str);
#endif
    value = fio_utf8_read(&pos);
    uint32_t val_len = fio_utf8_code_len(value); /* val_len 0 (fail) == 1 */
    FIO_ASSERT(!utf8_core_tests[i].expect_fail ||
                   (!value && pos == utf8_core_tests[i].buf &&
                    !fio_utf8_char_len(utf8_core_tests[i].buf)),
               "Failed to detect invalid UTF-8");
    if (utf8_core_tests[i].expect_fail)
      continue;
    char output[32];
    pos = output;
    pos += fio_utf8_write(pos, value);
    *pos = 0;
    FIO_ASSERT(val_len == utf8_core_tests[i].clen,
               "fio_utf8_read + fio_utf8_code_len failed on %s / %p (%zu "
               "len => %zu != %zu)",
               utf8_core_tests[i].buf,
               tst_str,
               (size_t)value,
               (size_t)val_len,
               (size_t)utf8_core_tests[i].clen);
    pos = output;
    validate = fio_utf8_read(&pos);
    FIO_ASSERT(validate == value && (value > 0 || !utf8_core_tests[i].buf[0]),
               "fio_utf8_read + fio_utf8_write roundtrip failed on [%zu] %s\n"
               "\t %zu != %zu",
               i,
               utf8_core_tests[i].buf,
               (size_t)validate,
               (size_t)value);
  }
}

FIO_SFUNC void fio___test_core_multiprecision_basic(void) {
  char buf[1024];

  fio_u256 a = fio_u256_init64(2);
  fio_u256 b = fio_u256_init64(3);
  fio_u512 expected = fio_u512_init64(6);

  fio_u512 result = {0};
  fio_u256_mul(&result, &a, &b);

  FIO_ASSERT(!FIO_MEMCMP(&result, &expected, sizeof(result)),
             "2 * 3 should be 6");
  FIO_ASSERT(!fio_u512_cmp(&result, &expected),
             "fio_u512_cmp failed for result 6.");

  a = fio_u256_init64(2, 2);
  expected = fio_u512_init64(6, 6);
  fio_u256_mul(&result, &a, &b);
  FIO_ASSERT(!FIO_MEMCMP(&result, &expected, sizeof(result)),
             "2,2 * 3 should be 6,6");
  FIO_ASSERT(!fio_u512_cmp(&result, &expected),
             "fio_u512_cmp failed for result 6,6.");

  a = fio_u256_init64(2, 0x8000000000000000);
  expected = fio_u512_init64(6, 0x8000000000000000, 1);
  fio_u256_mul(&result, &a, &b);
  FIO_ASSERT(!FIO_MEMCMP(&result, &expected, sizeof(result)),
             "2,0x8... * 3 should be 6,0x8..., 1");
  FIO_ASSERT(!fio_u512_cmp(&result, &expected),
             "fio_u512_cmp failed for result 6,0x8..., 1");

  a = fio_u256_init64(0xFFFFFFFFFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF); // Max value
  b = fio_u256_init64(0xFFFFFFFFFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF); // Max value
  expected = fio_u512_init64(0x1,
                             0,
                             0,
                             0,
                             0xFFFFFFFFFFFFFFFE,
                             0xFFFFFFFFFFFFFFFF,
                             0xFFFFFFFFFFFFFFFF,
                             0xFFFFFFFFFFFFFFFF);
  fio_u256_mul(&result, &a, &b);
  buf[fio_u512_hex_write((char *)buf, &result)] = 0;

  FIO_ASSERT(!FIO_MEMCMP(&result, &expected, sizeof(result)),
             "Max * Max should be (Max << 256) + 1\n\t0x%s",
             buf);
  FIO_ASSERT(!fio_u512_cmp(&result, &expected),
             "fio_u512_cmp failed for Max * Max result.");
}

FIO_SFUNC void fio___test_core_vector_operations(void) {
  for (uint64_t a = 1; a; a = ((a << 2) | (((a >> 62) & 1) ^ 1))) {
    for (uint64_t b = 2; b; b = ((b << 2) | (((b >> 62) & 2) ^ 2))) {
      uint64_t expected[8] = {1, ~0, 4, ~0};
      uint64_t na[4] = {a, a, a, a};
      uint64_t nb[4] = {b, b, b, b};
      fio_u512 result = fio_u512_init64(~0, 1, ~0, 4);
      fio_u256 ua = fio_u256_init64(a, a, a, a);
      fio_u256 ub = fio_u256_init64(b, b, b, b);

      fio_u64x4_add(expected, na, nb);
      fio_u256_add64(&result.u256[0], &ua, &ub);
      FIO_ASSERT(
          !memcmp(result.u256[0].u64, expected, sizeof(result.u256[0].u64)),
          "Basic vector ADD error");

      fio_u64x4_sub(expected, na, nb);
      fio_u256_sub64(&result.u256[0], &ua, &ub);
      FIO_ASSERT(
          !memcmp(result.u256[0].u64, expected, sizeof(result.u256[0].u64)),
          "Basic vector SUB error");

      fio_u64x4_mul(expected, na, nb);
      fio_u256_mul64(&result.u256[0], &ua, &ub);
      FIO_ASSERT(
          !memcmp(result.u256[0].u64, expected, sizeof(result.u256[0].u64)),
          "Basic vector MUL error");

      /* the following will probably never detect an error */
      {
        uint64_t ignr_ = fio_math_add(expected, na, nb, 4);
        ignr_ += fio_u256_add(&result.u256[0], &ua, &ub);
        (void)ignr_;
      }
      FIO_ASSERT(
          !memcmp(result.u256[0].u64, expected, sizeof(result.u256[0].u64)),
          "Multi-Precision ADD error");

      {
        uint64_t ignr_ = fio_math_sub(expected, na, nb, 4);
        ignr_ += fio_u256_sub(&result.u256[0], &ua, &ub);
        (void)ignr_;
      }
      FIO_ASSERT(
          !memcmp(result.u256[0].u64, expected, sizeof(result.u256[0].u64)),
          "Multi-Precision SUB error");

      fio___math_mul_long(expected, na, nb, 4); /* test possible difference */
      fio_u256_mul(&result, &ua, &ub);
      FIO_ASSERT(!memcmp(result.u64, expected, sizeof(result.u64)),
                 "Multi-Precision MUL error");
      {
        /* Copy expected into a properly aligned fio_u512 before comparing */
        fio_u512 expected_aligned;
        FIO_MEMCPY(&expected_aligned, expected, sizeof(expected_aligned));
        FIO_ASSERT(fio_u512_is_eq(&result, &expected_aligned),
                   "Multi-Precision MUL error (is_eq)");
      }
      {
        fio_u512 cpy = result;
        fio_u512 tmp = result;
        fio_u512_cadd16(&tmp, &tmp, 1);
        FIO_ASSERT(fio_u512_is_eq(&result, &cpy),
                   "Should be equal(fio_u512_is_eq)");
        FIO_ASSERT(!fio_u512_is_eq(&result, &tmp),
                   "Shouldn't be equal(fio_u512_is_eq)");
        fio_u512_ct_swap_if(0, &cpy, &tmp);
        FIO_ASSERT(fio_u512_is_eq(&result, &cpy),
                   "Should be equal(fio_u512_is_eq)");
        FIO_ASSERT(!fio_u512_is_eq(&result, &tmp),
                   "Shouldn't be equal(fio_u512_is_eq)");
        fio_u512_ct_swap_if(1, &cpy, &tmp);
        FIO_ASSERT(!fio_u512_is_eq(&result, &cpy),
                   "Shouldn't be equal(fio_u512_is_eq)");
        FIO_ASSERT(fio_u512_is_eq(&result, &tmp),
                   "Should be equal(fio_u512_is_eq)");
      }
    }
  }
}

/* *****************************************************************************
Memory Function Edge Case Tests (FIO_MEMALT)
***************************************************************************** */

FIO_SFUNC void fio___test_memcpyXx_masking(void) {
  char src[256];
  char dest[256];
  char guard = (char)0xDD;

  fio_rand_bytes(src, sizeof(src));

  /* fio_memcpy7x: len = 0, 1, 7, 8 (8 & 7 = 0!), 15 (15 & 7 = 7) */
  {
    size_t test_lens[] = {0, 1, 7, 8, 15};
    size_t expected_lens[] = {0, 1, 7, 0, 7}; /* len & 7 */
    size_t num_tests = sizeof(test_lens) / sizeof(test_lens[0]);

    for (size_t i = 0; i < num_tests; ++i) {
      FIO_MEMSET(dest, 0, sizeof(dest));
      dest[expected_lens[i]] = guard;

      fio_memcpy7x(dest, src, test_lens[i]);

      FIO_ASSERT(!memcmp(dest, src, expected_lens[i]),
                 "fio_memcpy7x failed for len=%zu (expected %zu bytes)",
                 test_lens[i],
                 expected_lens[i]);

      FIO_ASSERT(dest[expected_lens[i]] == guard || expected_lens[i] == 0,
                 "fio_memcpy7x overflow for len=%zu",
                 test_lens[i]);
    }
  }

  /* fio_memcpy15x: len = 0, 1, 15, 16 (16 & 15 = 0!), 31 (31 & 15 = 15) */
  {
    size_t test_lens[] = {0, 1, 15, 16, 31};
    size_t expected_lens[] = {0, 1, 15, 0, 15}; /* len & 15 */
    size_t num_tests = sizeof(test_lens) / sizeof(test_lens[0]);

    for (size_t i = 0; i < num_tests; ++i) {
      FIO_MEMSET(dest, 0, sizeof(dest));
      dest[expected_lens[i]] = guard;

      fio_memcpy15x(dest, src, test_lens[i]);

      FIO_ASSERT(!memcmp(dest, src, expected_lens[i]),
                 "fio_memcpy15x failed for len=%zu (expected %zu bytes)",
                 test_lens[i],
                 expected_lens[i]);

      FIO_ASSERT(dest[expected_lens[i]] == guard || expected_lens[i] == 0,
                 "fio_memcpy15x overflow for len=%zu",
                 test_lens[i]);
    }
  }

  /* fio_memcpy31x: len = 0, 1, 31, 32 (32 & 31 = 0!), 63 (63 & 31 = 31) */
  {
    size_t test_lens[] = {0, 1, 31, 32, 63};
    size_t expected_lens[] = {0, 1, 31, 0, 31}; /* len & 31 */
    size_t num_tests = sizeof(test_lens) / sizeof(test_lens[0]);

    for (size_t i = 0; i < num_tests; ++i) {
      FIO_MEMSET(dest, 0, sizeof(dest));
      dest[expected_lens[i]] = guard;

      fio_memcpy31x(dest, src, test_lens[i]);

      FIO_ASSERT(!memcmp(dest, src, expected_lens[i]),
                 "fio_memcpy31x failed for len=%zu (expected %zu bytes)",
                 test_lens[i],
                 expected_lens[i]);

      FIO_ASSERT(dest[expected_lens[i]] == guard || expected_lens[i] == 0,
                 "fio_memcpy31x overflow for len=%zu",
                 test_lens[i]);
    }
  }

  /* fio_memcpy63x: len = 0, 1, 63, 64 (64 & 63 = 0!), 127 (127 & 63 = 63) */
  {
    size_t test_lens[] = {0, 1, 63, 64, 127};
    size_t expected_lens[] = {0, 1, 63, 0, 63}; /* len & 63 */
    size_t num_tests = sizeof(test_lens) / sizeof(test_lens[0]);

    for (size_t i = 0; i < num_tests; ++i) {
      FIO_MEMSET(dest, 0, sizeof(dest));
      dest[expected_lens[i]] = guard;

      fio_memcpy63x(dest, src, test_lens[i]);

      FIO_ASSERT(!memcmp(dest, src, expected_lens[i]),
                 "fio_memcpy63x failed for len=%zu (expected %zu bytes)",
                 test_lens[i],
                 expected_lens[i]);

      FIO_ASSERT(dest[expected_lens[i]] == guard || expected_lens[i] == 0,
                 "fio_memcpy63x overflow for len=%zu",
                 test_lens[i]);
    }
  }
}

FIO_SFUNC void fio___test_core_simd_value_ops(void) {
  /* Test fio_u128 */
  {
    fio_u128 a = fio_u128_init64(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL);
    fio_u128 b = fio_u128_init64(0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL);
    fio_u128 xor_expected =
        fio_u128_init64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    fio_u128 and_expected = fio_u128_init64(0, 0);
    fio_u128 or_expected =
        fio_u128_init64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    fio_u128 add_expected =
        fio_u128_init64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);

    fio_u128 xor_result = fio_u128_xorv(a, b);
    fio_u128 and_result = fio_u128_andv(a, b);
    fio_u128 or_result = fio_u128_orv(a, b);
    fio_u128 add_result = fio_u128_addv64(a, b);

    FIO_ASSERT(!memcmp(&xor_result, &xor_expected, sizeof(fio_u128)),
               "fio_u128_xorv failed");
    FIO_ASSERT(!memcmp(&and_result, &and_expected, sizeof(fio_u128)),
               "fio_u128_andv failed");
    FIO_ASSERT(!memcmp(&or_result, &or_expected, sizeof(fio_u128)),
               "fio_u128_orv failed");
    FIO_ASSERT(!memcmp(&add_result, &add_expected, sizeof(fio_u128)),
               "fio_u128_addv64 failed");
  }
  /* Test fio_u256 */
  {
    fio_u256 a = fio_u256_init64(0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL);
    fio_u256 b = fio_u256_init64(0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL);
    fio_u256 xor_expected = fio_u256_init64(0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL);
    fio_u256 and_expected = fio_u256_init64(0, 0, 0, 0);
    fio_u256 or_expected = fio_u256_init64(0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL);
    fio_u256 add_expected = fio_u256_init64(0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL);

    fio_u256 xor_result = fio_u256_xorv(a, b);
    fio_u256 and_result = fio_u256_andv(a, b);
    fio_u256 or_result = fio_u256_orv(a, b);
    fio_u256 add_result = fio_u256_addv64(a, b);

    FIO_ASSERT(!memcmp(&xor_result, &xor_expected, sizeof(fio_u256)),
               "fio_u256_xorv failed");
    FIO_ASSERT(!memcmp(&and_result, &and_expected, sizeof(fio_u256)),
               "fio_u256_andv failed");
    FIO_ASSERT(!memcmp(&or_result, &or_expected, sizeof(fio_u256)),
               "fio_u256_orv failed");
    FIO_ASSERT(!memcmp(&add_result, &add_expected, sizeof(fio_u256)),
               "fio_u256_addv64 failed");
  }
  /* Test fio_u512 */
  {
    fio_u512 a = fio_u512_init64(0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL);
    fio_u512 b = fio_u512_init64(0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0xAAAAAAAAAAAAAAAAULL);
    fio_u512 xor_expected = fio_u512_init64(0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL);
    fio_u512 and_expected = fio_u512_init64(0, 0, 0, 0, 0, 0, 0, 0);
    fio_u512 or_expected = fio_u512_init64(0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL,
                                           0xFFFFFFFFFFFFFFFFULL);
    fio_u512 add_expected = fio_u512_init64(0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL,
                                            0xFFFFFFFFFFFFFFFFULL);

    fio_u512 xor_result = fio_u512_xorv(a, b);
    fio_u512 and_result = fio_u512_andv(a, b);
    fio_u512 or_result = fio_u512_orv(a, b);
    fio_u512 add_result = fio_u512_addv64(a, b);

    FIO_ASSERT(!memcmp(&xor_result, &xor_expected, sizeof(fio_u512)),
               "fio_u512_xorv failed");
    FIO_ASSERT(!memcmp(&and_result, &and_expected, sizeof(fio_u512)),
               "fio_u512_andv failed");
    FIO_ASSERT(!memcmp(&or_result, &or_expected, sizeof(fio_u512)),
               "fio_u512_orv failed");
    FIO_ASSERT(!memcmp(&add_result, &add_expected, sizeof(fio_u512)),
               "fio_u512_addv64 failed");
  }
  /* Test XOR self = 0 property */
  {
    fio_u256 a = fio_u256_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 zero = fio_u256_init64(0, 0, 0, 0);
    fio_u256 result = fio_u256_xorv(a, a);
    FIO_ASSERT(!memcmp(&result, &zero, sizeof(fio_u256)),
               "fio_u256_xorv(a, a) should be zero");
  }
  /* Test AND with all-ones = identity property */
  {
    fio_u256 a = fio_u256_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 ones = fio_u256_init64(0xFFFFFFFFFFFFFFFFULL,
                                    0xFFFFFFFFFFFFFFFFULL,
                                    0xFFFFFFFFFFFFFFFFULL,
                                    0xFFFFFFFFFFFFFFFFULL);
    fio_u256 result = fio_u256_andv(a, ones);
    FIO_ASSERT(!memcmp(&result, &a, sizeof(fio_u256)),
               "fio_u256_andv(a, ~0) should be a");
  }
  /* Test OR with zero = identity property */
  {
    fio_u256 a = fio_u256_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 zero = fio_u256_init64(0, 0, 0, 0);
    fio_u256 result = fio_u256_orv(a, zero);
    FIO_ASSERT(!memcmp(&result, &a, sizeof(fio_u256)),
               "fio_u256_orv(a, 0) should be a");
  }
}

/* *****************************************************************************
Rotation Function Tests (RROT/LROT)
***************************************************************************** */

FIO_SFUNC void fio___test_core_rotation_basic_correctness(void) {
  /* Test fio_u128 right rotation with known values */
  {
    fio_u128 src =
        fio_u128_init64(0x8000000000000001ULL, 0x0000000000000001ULL);
    fio_u128 target;

    /* Right rotate by 1: 0x8000000000000001 >> 1 = 0xC000000000000000 */
    fio_u128_crrot64(&target, &src, 1);
    FIO_ASSERT(
        target.u64[0] == 0xC000000000000000ULL,
        "fio_u128_crrot64 by 1 failed: got 0x%llx, expected 0xC000000000000000",
        (unsigned long long)target.u64[0]);
    FIO_ASSERT(target.u64[1] == 0x8000000000000000ULL,
               "fio_u128_crrot64 by 1 lane 1 failed");
  }

  /* Test fio_u128 left rotation with known values */
  {
    fio_u128 src =
        fio_u128_init64(0x8000000000000001ULL, 0x0000000000000001ULL);
    fio_u128 target;

    /* Left rotate by 1: 0x8000000000000001 << 1 = 0x0000000000000003 */
    fio_u128_clrot64(&target, &src, 1);
    FIO_ASSERT(
        target.u64[0] == 0x0000000000000003ULL,
        "fio_u128_clrot64 by 1 failed: got 0x%llx, expected 0x0000000000000003",
        (unsigned long long)target.u64[0]);
    FIO_ASSERT(target.u64[1] == 0x0000000000000002ULL,
               "fio_u128_clrot64 by 1 lane 1 failed");
  }

  /* Test 32-bit rotations */
  {
    fio_u128 src =
        fio_u128_init32(0x80000001, 0x80000001, 0x80000001, 0x80000001);
    fio_u128 target;

    /* Right rotate 32-bit by 1 */
    fio_u128_crrot32(&target, &src, 1);
    FIO_ASSERT(target.u32[0] == 0xC0000000,
               "fio_u128_crrot32 by 1 failed: got 0x%x, expected 0xC0000000",
               target.u32[0]);

    /* Left rotate 32-bit by 1 */
    fio_u128_clrot32(&target, &src, 1);
    FIO_ASSERT(target.u32[0] == 0x00000003,
               "fio_u128_clrot32 by 1 failed: got 0x%x, expected 0x00000003",
               target.u32[0]);
  }

  /* Test 16-bit rotations */
  {
    fio_u128 src = fio_u128_init16(0x8001,
                                   0x8001,
                                   0x8001,
                                   0x8001,
                                   0x8001,
                                   0x8001,
                                   0x8001,
                                   0x8001);
    fio_u128 target;

    /* Right rotate 16-bit by 1 */
    fio_u128_crrot16(&target, &src, 1);
    FIO_ASSERT(target.u16[0] == 0xC000,
               "fio_u128_crrot16 by 1 failed: got 0x%x, expected 0xC000",
               target.u16[0]);

    /* Left rotate 16-bit by 1 */
    fio_u128_clrot16(&target, &src, 1);
    FIO_ASSERT(target.u16[0] == 0x0003,
               "fio_u128_clrot16 by 1 failed: got 0x%x, expected 0x0003",
               target.u16[0]);
  }

  /* Test 8-bit rotations */
  {
    fio_u128 src;
    for (int i = 0; i < 16; ++i)
      src.u8[i] = 0x81;
    fio_u128 target;

    /* Right rotate 8-bit by 1 */
    fio_u128_crrot8(&target, &src, 1);
    FIO_ASSERT(target.u8[0] == 0xC0,
               "fio_u128_crrot8 by 1 failed: got 0x%x, expected 0xC0",
               target.u8[0]);

    /* Left rotate 8-bit by 1 */
    fio_u128_clrot8(&target, &src, 1);
    FIO_ASSERT(target.u8[0] == 0x03,
               "fio_u128_clrot8 by 1 failed: got 0x%x, expected 0x03",
               target.u8[0]);
  }
}

FIO_SFUNC void fio___test_core_rotation_zero(void) {
  /* Test that rotation by 0 returns the original value */
  {
    fio_u256 src = fio_u256_init64(0x123456789ABCDEF0ULL,
                                   0xFEDCBA9876543210ULL,
                                   0x0F0F0F0F0F0F0F0FULL,
                                   0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 target;

    /* Right rotate by 0 */
    fio_u256_crrot64(&target, &src, 0);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_crrot64 by 0 should return original value");

    /* Left rotate by 0 */
    fio_u256_clrot64(&target, &src, 0);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_clrot64 by 0 should return original value");

    /* 32-bit rotations by 0 */
    fio_u256_crrot32(&target, &src, 0);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_crrot32 by 0 should return original value");

    fio_u256_clrot32(&target, &src, 0);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_clrot32 by 0 should return original value");
  }
}

FIO_SFUNC void fio___test_core_rotation_full_width(void) {
  /* Test that rotation by full width returns the original value */
  {
    fio_u256 src = fio_u256_init64(0x123456789ABCDEF0ULL,
                                   0xFEDCBA9876543210ULL,
                                   0x0F0F0F0F0F0F0F0FULL,
                                   0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 target;

    /* Right rotate 64-bit by 64 (should wrap to 0) */
    fio_u256_crrot64(&target, &src, 64);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_crrot64 by 64 should return original value");

    /* Left rotate 64-bit by 64 (should wrap to 0) */
    fio_u256_clrot64(&target, &src, 64);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_clrot64 by 64 should return original value");

    /* 32-bit rotations by 32 */
    fio_u256_crrot32(&target, &src, 32);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_crrot32 by 32 should return original value");

    fio_u256_clrot32(&target, &src, 32);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_clrot32 by 32 should return original value");

    /* 16-bit rotations by 16 */
    fio_u256_crrot16(&target, &src, 16);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_crrot16 by 16 should return original value");

    fio_u256_clrot16(&target, &src, 16);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_clrot16 by 16 should return original value");

    /* 8-bit rotations by 8 */
    fio_u256_crrot8(&target, &src, 8);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_crrot8 by 8 should return original value");

    fio_u256_clrot8(&target, &src, 8);
    FIO_ASSERT(!memcmp(&target, &src, sizeof(fio_u256)),
               "fio_u256_clrot8 by 8 should return original value");
  }
}

FIO_SFUNC void fio___test_core_rotation_inverse(void) {
  /* Test that lrot(rrot(x, n), n) == x */
  {
    fio_u512 src = fio_u512_init64(0x123456789ABCDEF0ULL,
                                   0xFEDCBA9876543210ULL,
                                   0x0F0F0F0F0F0F0F0FULL,
                                   0xF0F0F0F0F0F0F0F0ULL,
                                   0xAAAAAAAAAAAAAAAAULL,
                                   0x5555555555555555ULL,
                                   0x0123456789ABCDEFULL,
                                   0xFEDCBA9876543210ULL);
    fio_u512 temp, result;

    /* Test various rotation amounts for 64-bit lanes */
    uint8_t test_amounts[] = {1, 7, 13, 17, 31, 32, 33, 63};
    for (size_t i = 0; i < sizeof(test_amounts); ++i) {
      uint8_t n = test_amounts[i];

      /* rrot then lrot */
      fio_u512_crrot64(&temp, &src, n);
      fio_u512_clrot64(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: lrot64(rrot64(x, %u), %u) != x",
                 n,
                 n);

      /* lrot then rrot */
      fio_u512_clrot64(&temp, &src, n);
      fio_u512_crrot64(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: rrot64(lrot64(x, %u), %u) != x",
                 n,
                 n);
    }

    /* Test 32-bit lanes */
    uint8_t test_amounts_32[] = {1, 7, 15, 16, 17, 31};
    for (size_t i = 0; i < sizeof(test_amounts_32); ++i) {
      uint8_t n = test_amounts_32[i];

      fio_u512_crrot32(&temp, &src, n);
      fio_u512_clrot32(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: lrot32(rrot32(x, %u), %u) != x",
                 n,
                 n);

      fio_u512_clrot32(&temp, &src, n);
      fio_u512_crrot32(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: rrot32(lrot32(x, %u), %u) != x",
                 n,
                 n);
    }

    /* Test 16-bit lanes */
    uint8_t test_amounts_16[] = {1, 7, 8, 15};
    for (size_t i = 0; i < sizeof(test_amounts_16); ++i) {
      uint8_t n = test_amounts_16[i];

      fio_u512_crrot16(&temp, &src, n);
      fio_u512_clrot16(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: lrot16(rrot16(x, %u), %u) != x",
                 n,
                 n);

      fio_u512_clrot16(&temp, &src, n);
      fio_u512_crrot16(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: rrot16(lrot16(x, %u), %u) != x",
                 n,
                 n);
    }

    /* Test 8-bit lanes */
    uint8_t test_amounts_8[] = {1, 3, 4, 7};
    for (size_t i = 0; i < sizeof(test_amounts_8); ++i) {
      uint8_t n = test_amounts_8[i];

      fio_u512_crrot8(&temp, &src, n);
      fio_u512_clrot8(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: lrot8(rrot8(x, %u), %u) != x",
                 n,
                 n);

      fio_u512_clrot8(&temp, &src, n);
      fio_u512_crrot8(&result, &temp, n);
      FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
                 "fio_u512: rrot8(lrot8(x, %u), %u) != x",
                 n,
                 n);
    }
  }
}

FIO_SFUNC void fio___test_core_rotation_per_lane(void) {
  /*
   * Note: Per-lane rotation functions (fio_uXXX_rrotN, fio_uXXX_lrotN) have
   * platform-dependent behavior when using SIMD backends:
   * - On GCC vector_size: the .xN member is a single full-width vector, so
   *   only rotations[0] is used for all lanes.
   * - On NEON: the .xN member is an array of 128-bit vectors.
   * - On scalar fallback: the .xN member is a plain array matching .uN.
   *
   * For consistent cross-platform behavior, use the constant rotation
   * functions (crrotN, clrotN) or the low-level macros directly on .uN arrays.
   *
   * This test verifies the macros work correctly on scalar arrays.
   */

  /* Test the low-level macros directly on scalar arrays for per-lane rotation
   */
  {
    uint64_t src[4] = {0x8000000000000001ULL,
                       0x8000000000000001ULL,
                       0x8000000000000001ULL,
                       0x8000000000000001ULL};
    uint64_t target[4];
    uint8_t rotations[4] = {1, 2, 3, 4};

    /* Right rotate with per-lane amounts using macro directly */
    FIO_MATH_UXXX_OP_RROT(target, src, rotations, 64);

    /* Verify each lane was rotated by its specific amount */
    FIO_ASSERT(target[0] == fio_rrot64(0x8000000000000001ULL, 1),
               "FIO_MATH_UXXX_OP_RROT per-lane[0] failed");
    FIO_ASSERT(target[1] == fio_rrot64(0x8000000000000001ULL, 2),
               "FIO_MATH_UXXX_OP_RROT per-lane[1] failed");
    FIO_ASSERT(target[2] == fio_rrot64(0x8000000000000001ULL, 3),
               "FIO_MATH_UXXX_OP_RROT per-lane[2] failed");
    FIO_ASSERT(target[3] == fio_rrot64(0x8000000000000001ULL, 4),
               "FIO_MATH_UXXX_OP_RROT per-lane[3] failed");

    /* Left rotate with per-lane amounts using macro directly */
    FIO_MATH_UXXX_OP_LROT(target, src, rotations, 64);

    /* Verify each lane was rotated by its specific amount */
    FIO_ASSERT(target[0] == fio_lrot64(0x8000000000000001ULL, 1),
               "FIO_MATH_UXXX_OP_LROT per-lane[0] failed");
    FIO_ASSERT(target[1] == fio_lrot64(0x8000000000000001ULL, 2),
               "FIO_MATH_UXXX_OP_LROT per-lane[1] failed");
    FIO_ASSERT(target[2] == fio_lrot64(0x8000000000000001ULL, 3),
               "FIO_MATH_UXXX_OP_LROT per-lane[2] failed");
    FIO_ASSERT(target[3] == fio_lrot64(0x8000000000000001ULL, 4),
               "FIO_MATH_UXXX_OP_LROT per-lane[3] failed");
  }

  /* Test 32-bit per-lane rotation using macros */
  {
    uint32_t src[8] = {0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001};
    uint32_t target[8];
    uint8_t rotations[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    FIO_MATH_UXXX_OP_RROT(target, src, rotations, 32);
    for (int i = 0; i < 8; ++i) {
      FIO_ASSERT(target[i] == fio_rrot32(0x80000001, rotations[i]),
                 "FIO_MATH_UXXX_OP_RROT (32-bit) per-lane[%d] failed",
                 i);
    }

    FIO_MATH_UXXX_OP_LROT(target, src, rotations, 32);
    for (int i = 0; i < 8; ++i) {
      FIO_ASSERT(target[i] == fio_lrot32(0x80000001, rotations[i]),
                 "FIO_MATH_UXXX_OP_LROT (32-bit) per-lane[%d] failed",
                 i);
    }
  }
}

FIO_SFUNC void fio___test_core_rotation_all_sizes(void) {
  /* Test fio_u128 */
  {
    fio_u128 src =
        fio_u128_init64(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL);
    fio_u128 temp, result;

    fio_u128_crrot64(&temp, &src, 7);
    fio_u128_clrot64(&result, &temp, 7);
    FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u128)),
               "fio_u128 rotation roundtrip failed");
  }

  /* Test fio_u256 */
  {
    fio_u256 src = fio_u256_init64(0xAAAAAAAAAAAAAAAAULL,
                                   0x5555555555555555ULL,
                                   0xAAAAAAAAAAAAAAAAULL,
                                   0x5555555555555555ULL);
    fio_u256 temp, result;

    fio_u256_crrot64(&temp, &src, 13);
    fio_u256_clrot64(&result, &temp, 13);
    FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u256)),
               "fio_u256 rotation roundtrip failed");
  }

  /* Test fio_u512 */
  {
    fio_u512 src = fio_u512_init64(0xAAAAAAAAAAAAAAAAULL,
                                   0x5555555555555555ULL,
                                   0xAAAAAAAAAAAAAAAAULL,
                                   0x5555555555555555ULL,
                                   0xAAAAAAAAAAAAAAAAULL,
                                   0x5555555555555555ULL,
                                   0xAAAAAAAAAAAAAAAAULL,
                                   0x5555555555555555ULL);
    fio_u512 temp, result;

    fio_u512_crrot64(&temp, &src, 17);
    fio_u512_clrot64(&result, &temp, 17);
    FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u512)),
               "fio_u512 rotation roundtrip failed");
  }

  /* Test fio_u1024 */
  {
    fio_u1024 src;
    for (int i = 0; i < 16; ++i)
      src.u64[i] = (i & 1) ? 0x5555555555555555ULL : 0xAAAAAAAAAAAAAAAAULL;
    fio_u1024 temp, result;

    fio_u1024_crrot64(&temp, &src, 23);
    fio_u1024_clrot64(&result, &temp, 23);
    FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u1024)),
               "fio_u1024 rotation roundtrip failed");
  }

  /* Test fio_u2048 */
  {
    fio_u2048 src;
    for (int i = 0; i < 32; ++i)
      src.u64[i] = (i & 1) ? 0x5555555555555555ULL : 0xAAAAAAAAAAAAAAAAULL;
    fio_u2048 temp, result;

    fio_u2048_crrot64(&temp, &src, 29);
    fio_u2048_clrot64(&result, &temp, 29);
    FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u2048)),
               "fio_u2048 rotation roundtrip failed");
  }

  /* Test fio_u4096 */
  {
    fio_u4096 src;
    for (int i = 0; i < 64; ++i)
      src.u64[i] = (i & 1) ? 0x5555555555555555ULL : 0xAAAAAAAAAAAAAAAAULL;
    fio_u4096 temp, result;

    fio_u4096_crrot64(&temp, &src, 31);
    fio_u4096_clrot64(&result, &temp, 31);
    FIO_ASSERT(!memcmp(&result, &src, sizeof(fio_u4096)),
               "fio_u4096 rotation roundtrip failed");
  }
}

FIO_SFUNC void fio___test_core_rotation_macros(void) {
  /* Test the low-level macros directly */
  {
    uint64_t src[4] = {0x8000000000000001ULL,
                       0x8000000000000001ULL,
                       0x8000000000000001ULL,
                       0x8000000000000001ULL};
    uint64_t target[4];
    uint8_t rotations[4] = {1, 2, 3, 4};

    /* Test FIO_MATH_UXXX_OP_RROT */
    FIO_MATH_UXXX_OP_RROT(target, src, rotations, 64);
    for (int i = 0; i < 4; ++i) {
      FIO_ASSERT(target[i] == fio_rrot64(src[i], rotations[i]),
                 "FIO_MATH_UXXX_OP_RROT failed at lane %d",
                 i);
    }

    /* Test FIO_MATH_UXXX_OP_LROT */
    FIO_MATH_UXXX_OP_LROT(target, src, rotations, 64);
    for (int i = 0; i < 4; ++i) {
      FIO_ASSERT(target[i] == fio_lrot64(src[i], rotations[i]),
                 "FIO_MATH_UXXX_OP_LROT failed at lane %d",
                 i);
    }

    /* Test FIO_MATH_UXXX_OP_CRROT */
    FIO_MATH_UXXX_OP_CRROT(target, src, 7, 64);
    for (int i = 0; i < 4; ++i) {
      FIO_ASSERT(target[i] == fio_rrot64(src[i], 7),
                 "FIO_MATH_UXXX_OP_CRROT failed at lane %d",
                 i);
    }

    /* Test FIO_MATH_UXXX_OP_CLROT */
    FIO_MATH_UXXX_OP_CLROT(target, src, 7, 64);
    for (int i = 0; i < 4; ++i) {
      FIO_ASSERT(target[i] == fio_lrot64(src[i], 7),
                 "FIO_MATH_UXXX_OP_CLROT failed at lane %d",
                 i);
    }
  }

  /* Test 32-bit macros */
  {
    uint32_t src[8] = {0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001,
                       0x80000001};
    uint32_t target[8];
    uint8_t rotations[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    FIO_MATH_UXXX_OP_RROT(target, src, rotations, 32);
    for (int i = 0; i < 8; ++i) {
      FIO_ASSERT(target[i] == fio_rrot32(src[i], rotations[i]),
                 "FIO_MATH_UXXX_OP_RROT (32-bit) failed at lane %d",
                 i);
    }

    FIO_MATH_UXXX_OP_LROT(target, src, rotations, 32);
    for (int i = 0; i < 8; ++i) {
      FIO_ASSERT(target[i] == fio_lrot32(src[i], rotations[i]),
                 "FIO_MATH_UXXX_OP_LROT (32-bit) failed at lane %d",
                 i);
    }

    FIO_MATH_UXXX_OP_CRROT(target, src, 11, 32);
    for (int i = 0; i < 8; ++i) {
      FIO_ASSERT(target[i] == fio_rrot32(src[i], 11),
                 "FIO_MATH_UXXX_OP_CRROT (32-bit) failed at lane %d",
                 i);
    }

    FIO_MATH_UXXX_OP_CLROT(target, src, 11, 32);
    for (int i = 0; i < 8; ++i) {
      FIO_ASSERT(target[i] == fio_lrot32(src[i], 11),
                 "FIO_MATH_UXXX_OP_CLROT (32-bit) failed at lane %d",
                 i);
    }
  }
}

/* *****************************************************************************
Merged core-adjacent tests
***************************************************************************** */

FIO_SFUNC void fio___test_core_atomics_and_locks(void) {
  struct fio___atomic_test_s {
    size_t w;
    unsigned long l;
    unsigned short s;
    unsigned char c;
  } s = {0}, r1 = {0}, r2 = {0};
  fio_lock_i lock = FIO_LOCK_INIT;

  r1.c = fio_atomic_add(&s.c, 1);
  r1.s = fio_atomic_add(&s.s, 1);
  r1.l = fio_atomic_add(&s.l, 1);
  r1.w = fio_atomic_add(&s.w, 1);
  FIO_ASSERT(r1.c == 0 && s.c == 1, "fio_atomic_add failed for c");
  FIO_ASSERT(r1.s == 0 && s.s == 1, "fio_atomic_add failed for s");
  FIO_ASSERT(r1.l == 0 && s.l == 1, "fio_atomic_add failed for l");
  FIO_ASSERT(r1.w == 0 && s.w == 1, "fio_atomic_add failed for w");
  r2.c = fio_atomic_add_fetch(&s.c, 1);
  r2.s = fio_atomic_add_fetch(&s.s, 1);
  r2.l = fio_atomic_add_fetch(&s.l, 1);
  r2.w = fio_atomic_add_fetch(&s.w, 1);
  FIO_ASSERT(r2.c == 2 && s.c == 2, "fio_atomic_add_fetch failed for c");
  FIO_ASSERT(r2.s == 2 && s.s == 2, "fio_atomic_add_fetch failed for s");
  FIO_ASSERT(r2.l == 2 && s.l == 2, "fio_atomic_add_fetch failed for l");
  FIO_ASSERT(r2.w == 2 && s.w == 2, "fio_atomic_add_fetch failed for w");
  r1.c = fio_atomic_sub(&s.c, 1);
  r1.s = fio_atomic_sub(&s.s, 1);
  r1.l = fio_atomic_sub(&s.l, 1);
  r1.w = fio_atomic_sub(&s.w, 1);
  FIO_ASSERT(r1.c == 2 && s.c == 1, "fio_atomic_sub failed for c");
  FIO_ASSERT(r1.s == 2 && s.s == 1, "fio_atomic_sub failed for s");
  FIO_ASSERT(r1.l == 2 && s.l == 1, "fio_atomic_sub failed for l");
  FIO_ASSERT(r1.w == 2 && s.w == 1, "fio_atomic_sub failed for w");
  r2.c = fio_atomic_sub_fetch(&s.c, 1);
  r2.s = fio_atomic_sub_fetch(&s.s, 1);
  r2.l = fio_atomic_sub_fetch(&s.l, 1);
  r2.w = fio_atomic_sub_fetch(&s.w, 1);
  FIO_ASSERT(r2.c == 0 && s.c == 0, "fio_atomic_sub_fetch failed for c");
  FIO_ASSERT(r2.s == 0 && s.s == 0, "fio_atomic_sub_fetch failed for s");
  FIO_ASSERT(r2.l == 0 && s.l == 0, "fio_atomic_sub_fetch failed for l");
  FIO_ASSERT(r2.w == 0 && s.w == 0, "fio_atomic_sub_fetch failed for w");
  fio_atomic_add(&s.c, 1);
  fio_atomic_add(&s.s, 1);
  fio_atomic_add(&s.l, 1);
  fio_atomic_add(&s.w, 1);
  r1.c = fio_atomic_exchange(&s.c, 99);
  r1.s = fio_atomic_exchange(&s.s, 99);
  r1.l = fio_atomic_exchange(&s.l, 99);
  r1.w = fio_atomic_exchange(&s.w, 99);
  FIO_ASSERT(r1.c == 1 && s.c == 99, "fio_atomic_exchange failed for c");
  FIO_ASSERT(r1.s == 1 && s.s == 99, "fio_atomic_exchange failed for s");
  FIO_ASSERT(r1.l == 1 && s.l == 99, "fio_atomic_exchange failed for l");
  FIO_ASSERT(r1.w == 1 && s.w == 99, "fio_atomic_exchange failed for w");
  // clang-format off
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.c, &r1.c, &r1.c), "fio_atomic_compare_exchange_p didn't fail for c");
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.s, &r1.s, &r1.s), "fio_atomic_compare_exchange_p didn't fail for s");
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.l, &r1.l, &r1.l), "fio_atomic_compare_exchange_p didn't fail for l");
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.w, &r1.w, &r1.w), "fio_atomic_compare_exchange_p didn't fail for w");
  r1.c = 1;s.c = 99; r1.s = 1;s.s = 99; r1.l = 1;s.l = 99; r1.w = 1;s.w = 99; /* ignore system spefcific behavior. */
  r1.c = fio_atomic_compare_exchange_p(&s.c,&s.c, &r1.c);
  r1.s = fio_atomic_compare_exchange_p(&s.s,&s.s, &r1.s);
  r1.l = fio_atomic_compare_exchange_p(&s.l,&s.l, &r1.l);
  r1.w = fio_atomic_compare_exchange_p(&s.w,&s.w, &r1.w);
  FIO_ASSERT(r1.c == 1 && s.c == 1, "fio_atomic_compare_exchange_p failed for c (%zu got %zu)", (size_t)s.c, (size_t)r1.c);
  FIO_ASSERT(r1.s == 1 && s.s == 1, "fio_atomic_compare_exchange_p failed for s (%zu got %zu)", (size_t)s.s, (size_t)r1.s);
  FIO_ASSERT(r1.l == 1 && s.l == 1, "fio_atomic_compare_exchange_p failed for l (%zu got %zu)", (size_t)s.l, (size_t)r1.l);
  FIO_ASSERT(r1.w == 1 && s.w == 1, "fio_atomic_compare_exchange_p failed for w (%zu got %zu)", (size_t)s.w, (size_t)r1.w);
  // clang-format on

  uint64_t val = 1;
  FIO_ASSERT(fio_atomic_and(&val, 2) == 1,
             "fio_atomic_and should return old value");
  FIO_ASSERT(val == 0, "fio_atomic_and should update value");
  FIO_ASSERT(fio_atomic_xor(&val, 1) == 0,
             "fio_atomic_xor should return old value");
  FIO_ASSERT(val == 1, "fio_atomic_xor_fetch should update value");
  FIO_ASSERT(fio_atomic_xor_fetch(&val, 1) == 0,
             "fio_atomic_xor_fetch should return new value");
  FIO_ASSERT(val == 0, "fio_atomic_xor should update value");
  FIO_ASSERT(fio_atomic_or(&val, 2) == 0,
             "fio_atomic_or should return old value");
  FIO_ASSERT(val == 2, "fio_atomic_or should update value");
  FIO_ASSERT(fio_atomic_or_fetch(&val, 1) == 3,
             "fio_atomic_or_fetch should return new value");
  FIO_ASSERT(val == 3, "fio_atomic_or_fetch should update value");

  FIO_ASSERT(!fio_is_locked(&lock),
             "lock should be initialized in unlocked state");
  FIO_ASSERT(!fio_trylock(&lock), "fio_trylock should succeed");
  FIO_ASSERT(fio_trylock(&lock), "fio_trylock should fail");
  FIO_ASSERT(fio_is_locked(&lock), "lock should be engaged");
  fio_unlock(&lock);
  FIO_ASSERT(!fio_is_locked(&lock), "lock should be released");
  fio_lock(&lock);
  FIO_ASSERT(fio_is_locked(&lock), "lock should be engaged (fio_lock)");
  for (uint8_t i = 1; i < 8; ++i) {
    FIO_ASSERT(!fio_is_group_locked(&lock, FIO_LOCK_SUBLOCK(i)),
               "group lock flagged, but wasn't engaged (%u - %p)",
               (unsigned int)i,
               (void *)(uintptr_t)lock);
  }
  fio_unlock(&lock);
  FIO_ASSERT(!fio_is_locked(&lock), "lock should be released");
  lock = FIO_LOCK_INIT;
  for (size_t i = 0; i < 8; ++i) {
    FIO_ASSERT(!fio_is_group_locked(&lock, FIO_LOCK_SUBLOCK(i)),
               "group lock should be initialized in unlocked state");
    FIO_ASSERT(!fio_trylock_group(&lock, FIO_LOCK_SUBLOCK(i)),
               "fio_trylock_group should succeed");
    FIO_ASSERT(fio_trylock_group(&lock, FIO_LOCK_SUBLOCK(i)),
               "fio_trylock should fail");
    FIO_ASSERT(fio_trylock_full(&lock), "fio_trylock_full should fail");
    FIO_ASSERT(fio_is_group_locked(&lock, FIO_LOCK_SUBLOCK(i)),
               "sub-lock %zu should be engaged",
               i);
    {
      uint8_t g =
          fio_trylock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(3));
      FIO_ASSERT((i != 1 && i != 3 && !g) || ((i == 1 || i == 3) && g),
                 "fio_trylock_group should succeed / fail");
      if (!g)
        fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(3));
    }
    for (uint8_t j = 1; j < 8; ++j) {
      FIO_ASSERT(i == j || !fio_is_group_locked(&lock, FIO_LOCK_SUBLOCK(j)),
                 "another group lock was flagged, though it wasn't engaged");
    }
    FIO_ASSERT(fio_is_group_locked(&lock, FIO_LOCK_SUBLOCK(i)),
               "lock should remain engaged");
    fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(i));
    FIO_ASSERT(!fio_is_group_locked(&lock, FIO_LOCK_SUBLOCK(i)),
               "group lock should be released");
    FIO_ASSERT(!fio_trylock_full(&lock), "fio_trylock_full should succeed");
    fio_unlock_full(&lock);
    FIO_ASSERT(!lock, "fio_unlock_full should unlock all");
  }
}

FIO_SFUNC void fio___test_core_type_sizes(void) {
  switch (sizeof(void *)) {
  case 2:
    fprintf(stderr, "\t 16bit words size (unexpected, unknown effects).\n");
    break;
  case 4:
    fprintf(stderr, "\t 32bit words size (some features might be slower).\n");
    break;
  case 8: fprintf(stderr, "\t 64bit words size okay.\n"); break;
  case 16: fprintf(stderr, "\t 128bit words size... wow!\n"); break;
  default:
    fprintf(stderr, "\t Unknown words size %zubit!\n", sizeof(void *) << 3);
    break;
  }
  fprintf(stderr, "\t Using the following type sizes:\n");
  FIO_PRINT_SIZE_OF(char);
  FIO_PRINT_SIZE_OF(short);
  FIO_PRINT_SIZE_OF(int);
  FIO_PRINT_SIZE_OF(float);
  FIO_PRINT_SIZE_OF(long);
  FIO_PRINT_SIZE_OF(double);
  FIO_PRINT_SIZE_OF(size_t);
  FIO_PRINT_SIZE_OF(void *);
  FIO_PRINT_SIZE_OF(uintmax_t);
  FIO_PRINT_SIZE_OF(long double);
  FIO_PRINT_SIZE_OF(fio_u128);
  FIO_PRINT_SIZE_OF(fio_u256);
  FIO_PRINT_SIZE_OF(fio_u512);
  FIO_PRINT_SIZE_OF(fio_u1024);
  FIO_PRINT_SIZE_OF(fio_u2048);
  FIO_PRINT_SIZE_OF(fio_u4096);
#ifdef __SIZEOF_INT128__
  FIO_PRINT_SIZE_OF(__uint128_t);
#endif
  FIO_PRINT_SIZE_OF(fio_thread_t);
  FIO_PRINT_SIZE_OF(fio_thread_mutex_t);
#if FIO_OS_POSIX || defined(_SC_PAGESIZE)
  long page = sysconf(_SC_PAGESIZE);
  if (page > 0) {
    fprintf(stderr, "\t\t%-17s%ld bytes.\n", "Page", page);
    if (page != (1UL << FIO_MEM_PAGE_SIZE_LOG))
      FIO_LOG_INFO("unexpected page size != 4096\n          "
                   "facil.io could be recompiled with:\n          "
                   "`CFLAGS=\"-DFIO_MEM_PAGE_SIZE_LOG=%.0lf\"`",
                   log2(page));
  }
#endif /* FIO_OS_POSIX */
}

static size_t FIO_NAME_TEST(stl, state_task_counter) = 0;
FIO_SFUNC void FIO_NAME_TEST(stl, state_task)(void *arg) {
  size_t *i = (size_t *)arg;
  ++i[0];
}
FIO_SFUNC void FIO_NAME_TEST(stl, state_task_global)(void *arg) {
  (void)arg;
  ++FIO_NAME_TEST(stl, state_task_counter);
}

FIO_SFUNC void fio___test_core_state_callbacks(void) {
  size_t count = 0;
  for (size_t i = 0; i < 1024; ++i) {
    fio_state_callback_add(FIO_CALL_RESERVED1,
                           FIO_NAME_TEST(stl, state_task),
                           &count);
    fio_state_callback_add(FIO_CALL_RESERVED1,
                           FIO_NAME_TEST(stl, state_task_global),
                           (void *)i);
  }
  FIO_ASSERT(!count && !FIO_NAME_TEST(stl, state_task_counter),
             "callbacks should NOT have been called yet");
  fio_state_callback_force(FIO_CALL_RESERVED1);
  FIO_ASSERT(count == 1, "count error for local counter callback (%zu)", count);
  FIO_ASSERT(FIO_NAME_TEST(stl, state_task_counter) == 1024,
             "count error for global counter callback (%zu)",
             FIO_NAME_TEST(stl, state_task_counter));
  for (size_t i = 0; i < 1024; ++i) {
    fio_state_callback_remove(FIO_CALL_RESERVED1,
                              FIO_NAME_TEST(stl, state_task),
                              &count);
    fio_state_callback_remove(FIO_CALL_RESERVED1,
                              FIO_NAME_TEST(stl, state_task_global),
                              (void *)i);
  }
  fio_state_callback_force(FIO_CALL_RESERVED1);
  FIO_ASSERT(count == 1,
             "count error for local counter callback (%zu) - not removed?",
             count);
  FIO_ASSERT(FIO_NAME_TEST(stl, state_task_counter) == 1024,
             "count error for global counter callback (%zu) - not removed?",
             FIO_NAME_TEST(stl, state_task_counter));
}

FIO_SFUNC void fio___test_core_logging(void) {
  FIO_LOG_FATAL("this is a fatal error message!");
  FIO_LOG_ERROR("this is an error message!");
  FIO_LOG_WARNING("this is a warning message!");
  FIO_LOG_INFO("this is an informative message.");
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

FIO_STATIC_SAFE_ALLOC_DEF(fio___test_sa, uint64_t, 8, 4)

typedef struct FIO_ALIGN(16) {
  uint64_t v[2];
} fio___test_sa16_s;
FIO_STATIC_SAFE_ALLOC_DEF(fio___test_sa16, fio___test_sa16_s, 2, 2)

FIO_SFUNC void fio___test_core_static_safe_alloc(void) {
  fprintf(stderr, "* Testing FIO_STATIC_SAFE_ALLOC_DEF\n");
  uint64_t *p[4] = {0};
  /* exhaust all slots */
  for (int i = 0; i < 4; ++i) {
    p[i] = fio___test_sa_try();
    FIO_ASSERT(p[i] != NULL, "slot %d checkout should succeed", i);
    FIO_ASSERT(((uintptr_t)p[i] % _Alignof(uint64_t)) == 0,
               "slot %d data must be aligned for its element type",
               i);
    for (int j = 0; j < i; ++j)
      FIO_ASSERT(p[j] != p[i], "slots %d and %d alias", j, i);
    p[i][0] = (uint64_t)(0xABCD + i);
  }
  FIO_ASSERT(fio___test_sa_try() == NULL,
             "checkout past exhaustion must return NULL");
  /* free restores availability (and data survives until reuse) */
  fio___test_sa_free(p[1]);
  uint64_t *q = fio___test_sa_try();
  FIO_ASSERT(q != NULL, "checkout after free should succeed");
  FIO_ASSERT(fio___test_sa_size() == 32, "logical arena capacity mismatch");
  /* release everything; a full second cycle must work */
  fio___test_sa_free(p[0]);
  fio___test_sa_free(p[2]);
  fio___test_sa_free(p[3]);
  fio___test_sa_free(q);
  for (int i = 0; i < 4; ++i) {
    uint64_t *r = fio___test_sa_try();
    FIO_ASSERT(r != NULL, "second cycle slot %d should succeed", i);
    fio___test_sa_free(r);
  }
  /* 16-byte (or larger) element types keep 16-byte header/alignment */
  fio___test_sa16_s *a = fio___test_sa16_try();
  fio___test_sa16_s *b = fio___test_sa16_try();
  FIO_ASSERT(a != NULL && b != NULL && a != b,
             "16-byte slots should check out distinctly");
  FIO_ASSERT((((uintptr_t)a & 15) == 0) && (((uintptr_t)b & 15) == 0),
             "16-byte slot data must be 16-byte aligned");
  FIO_ASSERT(fio___test_sa16_try() == NULL,
             "16-byte slot exhaustion must return NULL");
  fio___test_sa16_free(a);
  fio___test_sa16_free(b);
}

typedef struct {
  FIO_LIST_NODE node;
  int v;
} fio___test_list_item_s;

FIO_SFUNC void fio___test_core_list_each(void) {
  fprintf(stderr, "* Testing FIO_LIST_EACH / FIO_LIST_EACH_REVERSED\n");
  /* a zeroed (never initialized) head must iterate zero times */
  FIO_LIST_HEAD zero = {0};
  size_t count = 0;
  FIO_LIST_EACH(fio___test_list_item_s, node, &zero, pos) {
    (void)pos;
    ++count;
  }
  FIO_LIST_EACH_REVERSED(fio___test_list_item_s, node, &zero, pos) {
    (void)pos;
    ++count;
  }
  FIO_ASSERT(!count, "zeroed list head should not iterate");

  fio___test_list_item_s items[5];
  for (int n = 0; n <= 5; ++n) {
    FIO_LIST_HEAD head = FIO_LIST_INIT(head);
    for (int i = 0; i < n; ++i) {
      items[i].v = i;
      FIO_LIST_PUSH(&head, &items[i].node);
    }
    int expect = 0;
    FIO_LIST_EACH(fio___test_list_item_s, node, &head, pos) {
      FIO_ASSERT(pos->v == expect, "forward order (n=%d)", n);
      ++expect;
    }
    FIO_ASSERT(expect == n, "forward count %d != %d", expect, n);
    expect = n;
    FIO_LIST_EACH_REVERSED(fio___test_list_item_s, node, &head, pos) {
      --expect;
      FIO_ASSERT(pos->v == expect, "reversed order (n=%d)", n);
    }
    FIO_ASSERT(!expect, "reversed visited %d of %d nodes", n - expect, n);
    /* removing the current node is safe in both directions */
    count = 0;
    FIO_LIST_EACH_REVERSED(fio___test_list_item_s, node, &head, pos) {
      FIO_LIST_REMOVE(&pos->node);
      ++count;
    }
    FIO_ASSERT(count == (size_t)n && FIO_LIST_IS_EMPTY(&head),
               "reversed removal (n=%d)",
               n);
  }
}

/* First-difference index, including the internal experimental implementation.
 */
FIO_SFUNC void fio___test_core_diff_index(void) {
#if FIO___DIFF_INDEX
  enum { MAX_LEN = 4096 };
  uint8_t a_store[MAX_LEN + 16], b_store[MAX_LEN + 16];
  FIO_ASSERT(fio___diff_index(NULL, NULL, 0) == 0, "empty difference");
  const size_t lengths[] = {0,  1,  2,   3,   4,   7,   8,   15,  16,   31,  32,
                            63, 64, 127, 128, 129, 255, 256, 257, 4095, 4096};
  for (size_t offset = 0; offset < 8; ++offset) {
    uint8_t *a = a_store + offset, *b = b_store + ((offset + 3) & 7);
    for (size_t i = 0; i < MAX_LEN; ++i)
      a[i] = b[i] = (uint8_t)(i * 37U + 0x80U);
    for (size_t k = 0; k < sizeof lengths / sizeof *lengths; ++k) {
      size_t len = lengths[k];
      FIO_ASSERT(fio___diff_index(a, b, len) == len, "diff equal len=%zu", len);
      FIO_ASSERT(fio___diff_index(a, a, len) == len, "diff alias len=%zu", len);
      for (size_t pos = 0; pos < len; ++pos) {
        b[pos] ^= 0x80U;
        FIO_ASSERT(fio___diff_index(a, b, len) == pos,
                   "diff index len=%zu pos=%zu offset=%zu",
                   len,
                   pos,
                   offset);
        b[pos] ^= 0x80U;
      }
    }
  }
#endif
}

/* ASCII reference independent of the optimized word-lane implementation. */
FIO_SFUNC bool fio___core_buf_ieq_reference(fio_buf_info_s a,
                                            fio_buf_info_s b) {
  if (a.len != b.len)
    return false;
  for (size_t i = 0; i < a.len; ++i) {
    uint8_t x = (uint8_t)a.buf[i], y = (uint8_t)b.buf[i];
    if (x >= 'A' && x <= 'Z')
      x = (uint8_t)(x + 32);
    if (y >= 'A' && y <= 'Z')
      y = (uint8_t)(y + 32);
    if (x != y)
      return false;
  }
  return true;
}

FIO_SFUNC void fio___core_check_info_eq(fio_buf_info_s a, fio_buf_info_s b) {
  bool exact = a.len == b.len && (!a.len || !FIO_MEMCMP(a.buf, b.buf, a.len));
  bool folded = fio___core_buf_ieq_reference(a, b);
  FIO_ASSERT(fio_buf_info_is_eq(a, b) == exact, "buf exact length=%zu", a.len);
  FIO_ASSERT(fio_buf_info_is_ieq(a, b) == folded,
             "buf ASCII length=%zu",
             a.len);
  fio_str_info_s sa = {.len = a.len, .buf = a.buf};
  fio_str_info_s sb = {.len = b.len, .buf = b.buf};
  FIO_ASSERT(fio_str_info_is_eq(sa, sb) == exact,
             "str exact length=%zu",
             a.len);
  FIO_ASSERT(fio_str_info_is_ieq(sa, sb) == folded,
             "str ASCII length=%zu",
             a.len);
}

FIO_SFUNC void fio___test_core_info_equality(void) {
  enum { MAX_LEN = 2048 };
  char a_store[MAX_LEN + 16], b_store[MAX_LEN + 16];
  fio___core_check_info_eq((fio_buf_info_s){0}, (fio_buf_info_s){0});
  for (unsigned x = 0; x < 256; ++x) {
    for (unsigned y = 0; y < 256; ++y) {
      a_store[0] = (char)x;
      b_store[0] = (char)y;
      fio___core_check_info_eq((fio_buf_info_s){1, a_store},
                               (fio_buf_info_s){1, b_store});
    }
  }
  /* Every byte position and length, including both neighbors of power-of-two
   * boundaries, with unaligned input pointers and mixed ASCII case. */
  for (size_t offset = 0; offset < 8; ++offset) {
    char *a = a_store + offset;
    char *b = b_store + ((offset + 3) & 7);
    for (size_t i = 0; i < MAX_LEN; ++i) {
      a[i] = "Az@`[_{|9!"[i % 10];
      b[i] = a[i];
      if (b[i] >= 'A' && b[i] <= 'Z')
        b[i] += 32;
    }
    const size_t lengths[] = {
        0,  1,  2,  3,  4,   5,   7,   8,   9,   15,   16,   17,   31,   32,
        33, 63, 64, 65, 127, 128, 129, 255, 256, 1023, 1024, 1025, 2047, 2048};
    for (size_t k = 0; k < sizeof lengths / sizeof *lengths; ++k) {
      size_t len = lengths[k];
      fio_buf_info_s x = {len, a}, y = {len, b};
      fio___core_check_info_eq(x, y);
      fio___core_check_info_eq(x, x);
      fio___core_check_info_eq(x, (fio_buf_info_s){len + 1, b});
      for (size_t pos = 0; pos < len; ++pos) {
        char original = b[pos];
        b[pos] = '#';
        fio___core_check_info_eq(x, y);
        b[pos] = original;
        a[pos] = (char)0xC0;
        b[pos] = (char)0xE0;
        fio___core_check_info_eq(x, y);
        a[pos] = original >= 'a' && original <= 'z' ? original - 32 : original;
        b[pos] = original;
      }
    }
  }
}

/* Security regression tests for `000 core.h` candidates (2026-10-27 audit). */
#if defined(__SIZEOF_INT128__)
/* reference (a * b) % m for 128 bit values (double-and-add, overflow safe) */
FIO_SFUNC __uint128_t fio___test_mulmod128(__uint128_t a,
                                           __uint128_t b,
                                           __uint128_t m) {
  __uint128_t r = 0;
  a %= m;
  for (; b; b >>= 1) {
    if (b & 1)
      r = (r >= m - a) ? r - (m - a) : r + a;
    a = (a >= m - a) ? a - (m - a) : a + a;
  }
  return r;
}
#endif

/* deterministic 64 bit test values */
FIO_SFUNC uint64_t fio___test_mix64(uint64_t i, uint64_t seed) {
  uint64_t x = (i + 1) * 0x9E3779B97F4A7C15ULL + seed * 0xC2B2AE3D27D4EB4FULL;
  x ^= x >> 29;
  x *= 0xBF58476D1CE4E5B9ULL;
  return x ^ (x >> 32);
}

FIO_SFUNC void fio___test_core_security_regressions(void) {
  { /* C1: signed constant-time min / max at the extrema */
    const intmax_t v[] = {INTMAX_MIN,
                          INTMAX_MIN + 1,
                          -2,
                          -1,
                          0,
                          1,
                          2,
                          INTMAX_MAX - 1,
                          INTMAX_MAX};
    for (size_t i = 0; i < sizeof(v) / sizeof(v[0]); ++i)
      for (size_t j = 0; j < sizeof(v) / sizeof(v[0]); ++j) {
        FIO_ASSERT(fio_ct_max(v[i], v[j]) == (v[i] > v[j] ? v[i] : v[j]),
                   "fio_ct_max wrong at [%zu, %zu]",
                   i,
                   j);
        FIO_ASSERT(fio_ct_min(v[i], v[j]) == (v[i] < v[j] ? v[i] : v[j]),
                   "fio_ct_min wrong at [%zu, %zu]",
                   i,
                   j);
      }
  }
  { /* C2: shift counts at / above the width are masked, not UB */
    volatile uint8_t c32 = 32, c64 = 64;
    FIO_ASSERT(fio_xor_rrot2_shr_32(0x80000001UL, 0, 0, c32) == 0x80000001UL,
               "fio_xor_rrot2_shr_32 shift not masked");
    FIO_ASSERT(fio_xor_rrot2_shr_64(0x8000000000000001ULL, 0, 0, c64) ==
                   0x8000000000000001ULL,
               "fio_xor_rrot2_shr_64 shift not masked");
  }
  { /* C3: fio_xmask_cpy tail, all lengths, matches fio_xmask */
    char src[64], dst[64], ref[64];
    for (size_t i = 0; i < sizeof(src); ++i)
      src[i] = (char)(i * 7 + 1);
    for (size_t len = 0; len < 48; ++len) {
      FIO_MEMSET(dst, 0x5A, sizeof(dst));
      FIO_MEMCPY(ref, src, sizeof(ref));
      fio_xmask(ref, len, 0x0123456789ABCDEFULL);
      fio_xmask_cpy(dst, src, len, 0x0123456789ABCDEFULL);
      FIO_ASSERT(!FIO_MEMCMP(dst, ref, len), "fio_xmask_cpy len %zu", len);
      for (size_t i = len; i < sizeof(dst); ++i)
        FIO_ASSERT(dst[i] == 0x5A, "fio_xmask_cpy overflow at len %zu", len);
    }
  }
  { /* C7: fio_has_full_byte32 detects 0xFF, not 0x00 */
    FIO_ASSERT(!fio_has_full_byte32(0x00000000UL), "full byte: zero row");
    FIO_ASSERT(!fio_has_full_byte32(0x7F80FE01UL), "full byte: false hit");
    FIO_ASSERT(fio_has_full_byte32(0x00FF0000UL) == 0x00800000UL,
               "full byte: missed 0xFF");
    FIO_ASSERT(fio_has_full_byte32(0xFFFFFFFFUL) == 0x80808080UL,
               "full byte: all set");
    for (unsigned b = 0; b < 256; ++b)
      FIO_ASSERT(!!fio_has_full_byte32((uint32_t)b << 8) == (b == 0xFF),
                 "full byte: byte %u",
                 b);
  }
  { /* C8: zero-length multiplication writes nothing */
    uint64_t one = 1;
    fio_math_mul(NULL, &one, &one, 0);
  }
  { /* C9: lengths above the Karatsuba limit still multiply correctly */
    enum { LEN = FIO___MATH_KARATSUBA_LIMIT + 2 };
    static uint64_t a[LEN], b[LEN], r1[LEN * 2], r2[LEN * 2];
    for (size_t i = 0; i < LEN; ++i) {
      a[i] = fio___test_mix64(i, 1);
      b[i] = fio___test_mix64(i, 2);
    }
    fio_math_mul(r1, a, b, LEN);
    fio___math_mul_long(r2, a, b, LEN);
    FIO_ASSERT(!FIO_MEMCMP(r1, r2, sizeof(r1)), "large fio_math_mul");
  }
  { /* C10 / C11: modular helpers edge cases */
    FIO_ASSERT(!fio_math_is_iprime(INT64_MIN), "INT64_MIN isn't prime");
    FIO_ASSERT(fio_math_is_iprime(-7), "-7 is (absolutely) prime");
    FIO_ASSERT(!fio_math_mod_mul64(3, 5, 0), "mod 0 => 0");
    FIO_ASSERT(!fio_math_mod_expo64(3, 5, 0), "expo mod 0 => 0");
    FIO_ASSERT(!fio_math_mod_expo64(3, 0, 1), "x^0 mod 1 == 0");
    FIO_ASSERT(fio_math_mod_expo64(3, 0, 7) == 1, "x^0 mod 7 == 1");
    const uint64_t m = 0xFFFFFFFFFFFFFFC5ULL; /* largest 64 bit prime */
    FIO_ASSERT(fio_math_mod_mul64(m - 1, m - 1, m) == 1,
               "(-1)*(-1) mod m == 1");
    FIO_ASSERT(fio_math_mod_mul64(m - 1, 2, m) == m - 2, "(-1)*2 mod m");
    FIO_ASSERT(fio_math_is_uprime(m), "largest 64 bit prime");
  }
  { /* C14: case-insensitive compare guards NULL buffers */
    FIO_ASSERT(!fio_buf_info_is_ieq(FIO_BUF_INFO2(NULL, 3),
                                    FIO_BUF_INFO2((char *)"abc", 3)),
               "ieq NULL a");
    FIO_ASSERT(!fio_buf_info_is_ieq(FIO_BUF_INFO2((char *)"abc", 3),
                                    FIO_BUF_INFO2(NULL, 3)),
               "ieq NULL b");
  }
#if defined(__SIZEOF_INT128__)
  { /* C16: Montgomery reduction with N close to R (carry out of t + mN) */
    const __uint128_t n_vals[] = {
        ((__uint128_t)0xFFFFFFFFFFFFFFFFULL << 64) | 0xFFFFFFFFFFFFFF61ULL,
        ((__uint128_t)0x8000000000000000ULL << 64) | 0x0000000000000001ULL,
        ((__uint128_t)0xC3A5C85C97CB3127ULL << 64) | 0xB492B66FBE98F273ULL,
    };
    for (size_t k = 0; k < sizeof(n_vals) / sizeof(n_vals[0]); ++k) {
      const __uint128_t n = n_vals[k];
      __uint128_t inv = n; /* Newton: n^-1 mod 2^128 */
      for (size_t i = 0; i < 7; ++i)
        inv *= 2 - n * inv;
      const __uint128_t n_dash_v = (__uint128_t)0 - inv;
      const __uint128_t r_mod_n = ((__uint128_t)0 - n) % n; /* 2^128 mod n */
      for (size_t i = 0; i < 64; ++i) {
        __uint128_t av = n - 1 - (__uint128_t)fio___test_mix64(i, 3) * i;
        __uint128_t bv = n - 1 - ((__uint128_t)fio___test_mix64(i, 4) << 63);
        av %= n;
        bv %= n;
        fio_u128 a, b, N, N_dash, res;
        a.u64[0] = (uint64_t)av, a.u64[1] = (uint64_t)(av >> 64);
        b.u64[0] = (uint64_t)bv, b.u64[1] = (uint64_t)(bv >> 64);
        N.u64[0] = (uint64_t)n, N.u64[1] = (uint64_t)(n >> 64);
        N_dash.u64[0] = (uint64_t)n_dash_v;
        N_dash.u64[1] = (uint64_t)(n_dash_v >> 64);
        fio_u128_montgomery_mul(&res, &a, &b, &N, &N_dash);
        __uint128_t rv = ((__uint128_t)res.u64[1] << 64) | res.u64[0];
        FIO_ASSERT(rv < n, "montgomery result not reduced [%zu:%zu]", k, i);
        FIO_ASSERT(fio___test_mulmod128(rv, r_mod_n, n) ==
                       fio___test_mulmod128(av, bv, n),
                   "montgomery result wrong [%zu:%zu]",
                   k,
                   i);
      }
    }
  }
#endif /* __SIZEOF_INT128__ */
  {    /* C17: invalid code points write nothing; ASCII writes one byte */
    uint8_t buf[8];
    FIO_MEMSET(buf, 0xAA, sizeof(buf));
    FIO_ASSERT(!fio_utf8_write(buf, 0x200000UL) && buf[0] == 0xAA,
               "fio_utf8_write wrote an invalid code point");
    FIO_ASSERT(fio_utf8_write(buf, 'A') == 1 && buf[0] == 'A' && buf[1] == 0xAA,
               "fio_utf8_write ASCII overflow");
  }
  { /* C18: strict UTF-8 validation (RFC 3629) */
    static const struct {
      const char *s;
      unsigned len;
    } t[] = {
        {"\xC2\x80", 2},         {"\xDF\xBF", 2},
        {"\xE0\xA0\x80", 3},     {"\xED\x9F\xBF", 3},
        {"\xEE\x80\x80", 3},     {"\xEF\xBF\xBF", 3},
        {"\xF0\x90\x80\x80", 4}, {"\xF4\x8F\xBF\xBF", 4},
        {"\xC0\x80", 0},         /* overlong NUL */
        {"\xC0\xAF", 0},         /* overlong '/' */
        {"\xC1\xBF", 0},         /* overlong */
        {"\xE0\x80\xAF", 0},     /* overlong '/' */
        {"\xE0\x9F\xBF", 0},     /* overlong */
        {"\xED\xA0\x80", 0},     /* surrogate U+D800 */
        {"\xED\xBF\xBF", 0},     /* surrogate U+DFFF */
        {"\xF0\x80\x80\xAF", 0}, /* overlong '/' */
        {"\xF0\x8F\xBF\xBF", 0}, /* overlong */
        {"\xF4\x90\x80\x80", 0}, /* U+110000 */
        {"\xF5\x80\x80\x80", 0}, {"\xF7\xBF\xBF\xBF", 0},
    };
    for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
      char *pos = (char *)t[i].s;
      FIO_ASSERT(fio_utf8_char_len(t[i].s) == t[i].len,
                 "fio_utf8_char_len strictness [%zu] %u != %u",
                 i,
                 fio_utf8_char_len(t[i].s),
                 t[i].len);
      uint32_t u = fio_utf8_read(&pos);
      FIO_ASSERT((size_t)(pos - t[i].s) == t[i].len && (!t[i].len || u),
                 "fio_utf8_read strictness [%zu]",
                 i);
    }
    /* exhaustive: every (c0, c1) pair x interesting c2 / c3 vs. a decoder */
    static const uint8_t tails[] = {0x00, 0x41, 0x7F, 0x80, 0xBF, 0xC0, 0xFF};
    for (unsigned c0 = 0; c0 < 256; ++c0) {
      for (unsigned c1 = 0; c1 < 256; ++c1) {
        for (size_t j = 0; j < sizeof(tails); ++j) {
          for (size_t k = 0; k < sizeof(tails); ++k) {
            const uint8_t b[4] = {(uint8_t)c0, (uint8_t)c1, tails[j], tails[k]};
            unsigned want = 0;
            uint32_t cp = 0;
            if (c0 < 0x80)
              want = 1;
            else if ((c0 & 0xE0U) == 0xC0U)
              want = 2, cp = c0 & 0x1FU;
            else if ((c0 & 0xF0U) == 0xE0U)
              want = 3, cp = c0 & 0x0FU;
            else if ((c0 & 0xF8U) == 0xF0U)
              want = 4, cp = c0 & 0x07U;
            for (unsigned n = 1; n < want; ++n) {
              if ((b[n] & 0xC0U) != 0x80U) {
                want = 0;
                break;
              }
              cp = (cp << 6) | (b[n] & 0x3FU);
            }
            static const uint32_t min_cp[5] = {0, 0, 0x80, 0x800, 0x10000};
            if (cp < min_cp[want] || (cp >= 0xD800U && cp <= 0xDFFFU) ||
                cp > 0x10FFFFU)
              want = 0;
            FIO_ASSERT(fio_utf8_char_len(b) == want,
                       "fio_utf8_char_len(%02X %02X %02X %02X) %u != %u",
                       b[0],
                       b[1],
                       b[2],
                       b[3],
                       fio_utf8_char_len(b),
                       want);
          }
        }
      }
    }
  }
  { /* C20: bounded reads never pass `len` (exact-size heap copies so ASan
     * catches overreads; also run with FIO_UTF8_ALLOW_IF=0) */
    static const struct {
      const char *s;
      size_t len;
      unsigned expect;
    } t[] = {
        {"A", 1, 1},
        {"\x80", 1, 0},
        {"\xC3\xA9", 2, 2},
        {"\xC3\xA9", 1, 0},
        {"\xE2\x82\xAC", 3, 3},
        {"\xE2\x82\xAC", 2, 0},
        {"\xF0\x9F\x98\x80", 4, 4},
        {"\xF0\x9F\x98\x80", 3, 0},
        {"\xC0\x80", 2, 0},
        {"A\x80", 2, 1}, /* both build modes agree: ASCII stands alone */
    };
    for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
      char *b = (char *)malloc(t[i].len);
      FIO_ASSERT_ALLOC(b);
      FIO_MEMCPY(b, t[i].s, t[i].len);
      FIO_ASSERT(fio_utf8_char_len_bounded(b, t[i].len) == t[i].expect,
                 "fio_utf8_char_len_bounded [%zu] %u != %u",
                 i,
                 fio_utf8_char_len_bounded(b, t[i].len),
                 t[i].expect);
      free(b);
    }
    FIO_ASSERT(!fio_utf8_char_len_bounded(NULL, 0),
               "fio_utf8_char_len_bounded must not read when len == 0");
    /* NUL terminated: reads stop at the first NUL (or ASCII) byte */
    static const char *full[] = {"\xC3\xA9",
                                 "\xE2\x82\xAC",
                                 "\xED\x9F\xBF",
                                 "\xF0\x9F\x98\x80",
                                 "\xF4\x8F\xBF\xBF"};
    for (size_t i = 0; i < sizeof(full) / sizeof(full[0]); ++i) {
      const size_t n = strlen(full[i]);
      for (size_t k = 1; k < n; ++k) {
        for (size_t ascii = 0; ascii < 2; ++ascii) {
          char *b = (char *)malloc(k + ascii + 1);
          FIO_ASSERT_ALLOC(b);
          FIO_MEMCPY(b, full[i], k);
          b[k] = 'A';
          b[k + ascii] = 0;
          FIO_ASSERT(!fio_utf8_char_len(b),
                     "fio_utf8_char_len NUL tail [%zu] %zu %zu",
                     i,
                     k,
                     ascii);
          free(b);
        }
      }
    }
  }
  { /* C19: generated shuffle indexes wrap within the vector */
    uint64_t v[4] = {10, 11, 12, 13};
    FIO_MATH_UXXX_SUFFLE(v, 64, 7, 6, 5, 4);
    FIO_ASSERT(v[0] == 13 && v[1] == 12 && v[2] == 11 && v[3] == 10,
               "FIO_MATH_UXXX_SUFFLE index masking");
  }
}

int main(void) {
  /* Run merged core-adjacent tests first. */
  fio___test_core_type_sizes();
  fio___test_core_logging();
  fio___test_core_state_callbacks();
  fio___test_core_atomics_and_locks();

  /* Run all functional tests */
  fio___test_core_memcpy_primitives();
  fio___test_memcpyXx_masking();
  fio___test_core_byte_swap();
  fio___test_core_bit_rotation();
  fio___test_core_bit_index();
  fio___test_core_buf_helpers();
  fio___test_core_info_equality();
  fio___test_core_diff_index();
  fio___test_core_ct_operations();
  fio___test_core_ct_bitwise();
  fio___test_core_bitmap();
  fio___test_core_atomic_bitmap();
  fio___test_core_popcount();
  fio___test_core_has_byte();
  fio___test_core_ptr_from_field();
  fio___test_core_xmask();
  fio___test_core_utf8();
  fio___test_core_multiprecision_basic();
  fio___test_core_vector_operations();
  fio___test_core_simd_value_ops();

  /* Rotation function tests (RROT/LROT) */
  fio___test_core_rotation_basic_correctness();
  fio___test_core_rotation_zero();
  fio___test_core_rotation_full_width();
  fio___test_core_rotation_inverse();
  fio___test_core_rotation_per_lane();
  fio___test_core_rotation_all_sizes();
  fio___test_core_rotation_macros();
  fio___test_core_static_safe_alloc();
  fio___test_core_list_each();
  fio___test_core_security_regressions();
  return 0;
}
