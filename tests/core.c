/* *****************************************************************************
Test - Core Module
***************************************************************************** */
#undef FIO_MEMALT
#define FIO_MEMALT /* Enable fio_memcpy, fio_memset, fio_memchr, etc. */
#include "test-helpers.h"

/* *****************************************************************************
Functional Tests
***************************************************************************** */

FIO_SFUNC void fio___test_core_memcpy_primitives(void) {
  FIO_LOG_DDEBUG("Testing fio_memcpy primitives.");
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
}

FIO_SFUNC void fio___test_core_byte_swap(void) {
  FIO_LOG_DDEBUG("Testing fio_bswapX macros.");
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
  FIO_LOG_DDEBUG("Testing fio_lrotX and fio_rrotX macros.");
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
  FIO_LOG_DDEBUG("Testing bit index functions.");
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
  /* Test fio_bits_lsb and fio_bits_msb */
  FIO_ASSERT(fio_bits_lsb(0x0F00) == 0x0100, "fio_bits_lsb failed");
  FIO_ASSERT(fio_bits_msb(0x0F00) == 0x0800, "fio_bits_msb failed");
  FIO_ASSERT(fio_bits_lsb(0) == 0, "fio_bits_lsb(0) should be 0");
  FIO_ASSERT(fio_bits_msb(0) == 0, "fio_bits_msb(0) should be 0");
}

FIO_SFUNC void fio___test_core_buf_helpers(void) {
  FIO_LOG_DDEBUG("Testing fio_buf2uX and fio_u2bufX helpers.");
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
  FIO_LOG_DDEBUG("Testing constant-time helpers.");
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
  FIO_LOG_DDEBUG("Testing constant-time bitwise selection functions.");
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
  FIO_LOG_DDEBUG("Testing bitmap helpers.");
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
  FIO_LOG_DDEBUG("Testing atomic bitmap helpers.");
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
  FIO_LOG_DDEBUG("Testing popcount and hemming distance calculation.");
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
  FIO_LOG_DDEBUG("Testing fio_has_zero_byte and fio_has_byte.");
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
  FIO_LOG_DDEBUG("Testing fio_xmask.");
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
  FIO_LOG_DDEBUG("Testing Core UTF-8 Support (Macros).");
  struct {
    const char *buf;
    size_t clen;
    bool expect_fail;
  } utf8_core_tests[] = {
      {"\xf0\x9f\x92\x85", 4},
      {"\xf0\x9f\x92\x95", 4},
      {"\xe2\x9d\xa4", 3},
      {"\xE1\x9A\x80", 3},
      {"\xE2\x80\x80", 3},
      {"\xE2\x80\x81", 3},
      {"\xE2\x80\x82", 3},
      {"\xE2\x80\x83", 3},
      {"\xE2\x80\x84", 3},
      {"\xE2\x80\x85", 3},
      {"\xE2\x80\x86", 3},
      {"\xE2\x80\x87", 3},
      {"\xE2\x80\x88", 3},
      {"\xE2\x80\x89", 3},
      {"\xE2\x80\x8A", 3},
      {"\xE2\x80\xA8", 3},
      {"\xE2\x80\xA9", 3},
      {"\xE2\x80\xAF", 3},
      {"\xE2\x81\x9F", 3},
      {"\xE3\x80\x80", 3},
      {"\xEF\xBB\xBF", 3},
      {"\xc6\x92", 2},
      {"\xC2\xA0", 2},
      {"\x09", 1},
      {"\x0A", 1},
      {"\x0B", 1},
      {"\x0C", 1},
      {"\x0D", 1},
      {"\x20", 1},
      {"Z", 1},
      {"\0", 1},
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
               val_len,
               utf8_core_tests[i].clen);
    pos = output;
    validate = fio_utf8_read(&pos);
    FIO_ASSERT(validate == value && (value > 0 || !utf8_core_tests[i].buf[0]),
               "fio_utf8_read + fio_utf8_write roundtrip failed on [%zu] %s\n"
               "\t %zu != %zu",
               i,
               utf8_core_tests[i].buf,
               validate,
               value);
  }
}

FIO_SFUNC void fio___test_core_multiprecision_basic(void) {
  FIO_LOG_DDEBUG("Testing Basic Multi-Precision add / sub / mul for fio_uXXX "
                 "(fio_u256).");

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
  FIO_LOG_DDEBUG("Testing Basic vector operations for fio_uXXX (fio_u256).");
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
      FIO_ASSERT(fio_u512_is_eq(&result, (fio_u512 *)&expected),
                 "Multi-Precision MUL error (is_eq)");
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

FIO_SFUNC void fio___test_memcpy_overlap(void) {
  FIO_LOG_DDEBUG("Testing fio_memcpy overlap handling.");

  /* Forward overlap: dest < src < dest+len (should use buffered copy) */
  {
    char buf[256];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    /* Copy from buf+10 to buf+0 (100 bytes) - forward overlap */
    fio_memcpy(buf, buf + 10, 100);
    FIO_ASSERT(!memcmp(buf, expected + 10, 100),
               "fio_memcpy forward overlap failed");
  }

  /* Backward overlap: src < dest < src+len (should use reversed copy) */
  {
    char buf[256];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    /* Copy from buf+0 to buf+10 (100 bytes) - backward overlap */
    fio_memcpy(buf + 10, buf, 100);
    FIO_ASSERT(!memcmp(buf + 10, expected, 100),
               "fio_memcpy backward overlap failed");
  }

  /* No overlap: dest+len <= src */
  {
    char buf[512];
    fio_rand_bytes(buf + 256, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf + 256, 256);

    fio_memcpy(buf, buf + 256, 256);
    FIO_ASSERT(!memcmp(buf, expected, 256),
               "fio_memcpy non-overlapping (dest before src) failed");
  }

  /* No overlap: src+len <= dest */
  {
    char buf[512];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    fio_memcpy(buf + 256, buf, 256);
    FIO_ASSERT(!memcmp(buf + 256, expected, 256),
               "fio_memcpy non-overlapping (src before dest) failed");
  }

  /* Adjacent memory: dest = src+len (no overlap) */
  {
    char buf[256];
    fio_rand_bytes(buf, 128);
    char expected[128];
    FIO_MEMCPY(expected, buf, 128);

    fio_memcpy(buf + 128, buf, 128);
    FIO_ASSERT(!memcmp(buf + 128, expected, 128),
               "fio_memcpy adjacent memory failed");
  }

  /* Same pointer: dest == src (should be no-op) */
  {
    char buf[64];
    fio_rand_bytes(buf, 64);
    char expected[64];
    FIO_MEMCPY(expected, buf, 64);

    fio_memcpy(buf, buf, 64);
    FIO_ASSERT(!memcmp(buf, expected, 64), "fio_memcpy same pointer failed");
  }

  /* Overlap at various offsets */
  for (size_t offset = 1; offset <= 64; offset *= 2) {
    char buf[256];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    /* Forward overlap */
    fio_memcpy(buf, buf + offset, 128);
    FIO_ASSERT(!memcmp(buf, expected + offset, 128),
               "fio_memcpy forward overlap at offset %zu failed",
               offset);

    /* Reset and test backward overlap */
    FIO_MEMCPY(buf, expected, 256);
    fio_memcpy(buf + offset, buf, 128);
    FIO_ASSERT(!memcmp(buf + offset, expected, 128),
               "fio_memcpy backward overlap at offset %zu failed",
               offset);
  }
}

FIO_SFUNC void fio___test_memcpy_null_handling(void) {
  FIO_LOG_DDEBUG("Testing fio_memcpy NULL pointer handling.");

  char buf[64];
  fio_rand_bytes(buf, 64);
  char expected[64];
  FIO_MEMCPY(expected, buf, 64);

  /* Both NULL with len = 0 - should be safe no-op */
  void *result = fio_memcpy(NULL, NULL, 0);
  FIO_ASSERT(result == NULL, "fio_memcpy(NULL, NULL, 0) should return NULL");

  /* len = 0 with valid pointers - should be no-op */
  result = fio_memcpy(buf, buf + 32, 0);
  FIO_ASSERT(!memcmp(buf, expected, 64),
             "fio_memcpy with len=0 should not modify buffer");
}

FIO_SFUNC void fio___test_memcpy_size_boundaries(void) {
  FIO_LOG_DDEBUG("Testing fio_memcpy size boundaries.");

  /* Test sizes: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 31, 32, 33,
   * 63, 64, 65, 127, 128, 129, 255, 256, 257 */
  size_t test_sizes[] = {0,  1,   2,   3,   4,   5,   6,   7,   8,
                         9,  15,  16,  17,  31,  32,  33,  63,  64,
                         65, 127, 128, 129, 255, 256, 257, 511, 512};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  char src[1024];
  char dest[1024];
  char guard = (char)0xAA;

  fio_rand_bytes(src, sizeof(src));

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];
    if (len > sizeof(src))
      continue;

    /* Clear dest and set guard byte */
    FIO_MEMSET(dest, 0, sizeof(dest));
    if (len < sizeof(dest))
      dest[len] = guard;

    fio_memcpy(dest, src, len);

    FIO_ASSERT(!memcmp(dest, src, len), "fio_memcpy failed for size %zu", len);

    /* Check guard byte wasn't modified (overflow detection) */
    if (len < sizeof(dest)) {
      FIO_ASSERT(dest[len] == guard,
                 "fio_memcpy overflow detected at size %zu",
                 len);
    }
  }
}

FIO_SFUNC void fio___test_memcpy_alignment(void) {
  FIO_LOG_DDEBUG("Testing fio_memcpy alignment variations.");

  char buf[256 + 16] FIO_ALIGN(16);
  char src[256 + 16] FIO_ALIGN(16);
  fio_rand_bytes(src, sizeof(src));

  /* Test all combinations of dest and src alignment offsets 0-7 */
  for (size_t dest_off = 0; dest_off < 8; ++dest_off) {
    for (size_t src_off = 0; src_off < 8; ++src_off) {
      FIO_MEMSET(buf, 0, sizeof(buf));

      fio_memcpy(buf + dest_off, src + src_off, 64);

      FIO_ASSERT(!memcmp(buf + dest_off, src + src_off, 64),
                 "fio_memcpy alignment test failed: dest_off=%zu, src_off=%zu",
                 dest_off,
                 src_off);
    }
  }
}

FIO_SFUNC void fio___test_memset_size_boundaries(void) {
  FIO_LOG_DDEBUG("Testing fio_memset size boundaries.");

  size_t test_sizes[] = {0, 1, 7, 8, 15, 16, 31, 32, 33, 63, 64, 127, 128};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  char buf[256];
  char guard = (char)0xCC;

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];

    /* Clear buffer and set guard */
    FIO_MEMSET(buf, 0xFF, sizeof(buf));
    if (len < sizeof(buf))
      buf[len] = guard;

    fio_memset(buf, 0x42, len);

    /* Verify all bytes are set correctly */
    for (size_t j = 0; j < len; ++j) {
      FIO_ASSERT((uint8_t)buf[j] == 0x42,
                 "fio_memset failed at byte %zu for size %zu",
                 j,
                 len);
    }

    /* Check guard byte */
    if (len < sizeof(buf)) {
      FIO_ASSERT(buf[len] == guard,
                 "fio_memset overflow detected at size %zu",
                 len);
    }
  }
}

FIO_SFUNC void fio___test_memset_data_patterns(void) {
  FIO_LOG_DDEBUG("Testing fio_memset data patterns.");

  char buf[128];

  /* data = 0 (all zeros) */
  {
    FIO_MEMSET(buf, 0xFF, sizeof(buf));
    fio_memset(buf, 0, 64);
    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT(buf[i] == 0, "fio_memset with data=0 failed at byte %zu", i);
    }
  }

  /* data = 0xFF (single byte, should expand to 0xFFFFFFFFFFFFFFFF) */
  {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf, 0xFF, 64);
    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT((uint8_t)buf[i] == 0xFF,
                 "fio_memset with data=0xFF failed at byte %zu",
                 i);
    }
  }

  /* data = 1 (single byte, should expand to 0x0101010101010101) */
  {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf, 1, 64);
    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT((uint8_t)buf[i] == 0x01,
                 "fio_memset with data=1 failed at byte %zu",
                 i);
    }
  }

  /* data = 0x0100 (two-byte pattern, NOT expanded - only low byte used) */
  {
    FIO_MEMSET(buf, 0xFF, sizeof(buf));
    fio_memset(buf, 0x0100, 64);
    /* When data >= 0x100, fio_memset uses the full 64-bit pattern */
    /* Check that pattern is applied correctly */
    uint64_t expected = 0x0100;
    for (size_t i = 0; i + 8 <= 64; i += 8) {
      uint64_t val;
      fio_memcpy8(&val, buf + i);
      FIO_ASSERT(val == expected,
                 "fio_memset with data=0x0100 failed at offset %zu",
                 i);
    }
  }

  /* data = 0xDEADBEEFCAFEBABE (8-byte pattern) */
  {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf, 0xDEADBEEFCAFEBABEULL, 64);
    uint64_t expected = 0xDEADBEEFCAFEBABEULL;
    for (size_t i = 0; i + 8 <= 64; i += 8) {
      uint64_t val;
      fio_memcpy8(&val, buf + i);
      FIO_ASSERT(val == expected,
                 "fio_memset with 8-byte pattern failed at offset %zu",
                 i);
    }
  }
}

FIO_SFUNC void fio___test_memset_alignment(void) {
  FIO_LOG_DDEBUG("Testing fio_memset alignment variations.");

  char buf[128 + 16] FIO_ALIGN(16);

  /* Test dest unaligned by 1-7 bytes */
  for (size_t offset = 0; offset < 8; ++offset) {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf + offset, 0xAB, 64);

    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT((uint8_t)buf[offset + i] == 0xAB,
                 "fio_memset alignment test failed: offset=%zu, byte=%zu",
                 offset,
                 i);
    }

    /* Verify bytes before offset weren't touched */
    for (size_t i = 0; i < offset; ++i) {
      FIO_ASSERT(buf[i] == 0,
                 "fio_memset wrote before dest at offset=%zu",
                 offset);
    }
  }
}

FIO_SFUNC void fio___test_memchr_token_position(void) {
  FIO_LOG_DDEBUG("Testing fio_memchr token position.");

  char buf[256];
  FIO_MEMSET(buf, 'X', sizeof(buf));

  /* Token at position 0 (first byte) */
  {
    buf[0] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf, "fio_memchr failed to find token at position 0");
    buf[0] = 'X';
  }

  /* Token at position 1 */
  {
    buf[1] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + 1,
               "fio_memchr failed to find token at position 1");
    buf[1] = 'X';
  }

  /* Token at position len-1 (last byte) */
  {
    buf[sizeof(buf) - 1] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + sizeof(buf) - 1,
               "fio_memchr failed to find token at last position");
    buf[sizeof(buf) - 1] = 'X';
  }

  /* Token not present (return NULL) */
  {
    void *result = fio_memchr(buf, 'Z', sizeof(buf));
    FIO_ASSERT(result == NULL,
               "fio_memchr should return NULL when token not found");
  }

  /* Multiple occurrences (find first) */
  {
    buf[10] = 'A';
    buf[50] = 'A';
    buf[100] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + 10, "fio_memchr should find first occurrence");
    buf[10] = buf[50] = buf[100] = 'X';
  }
}

FIO_SFUNC void fio___test_memchr_token_values(void) {
  FIO_LOG_DDEBUG("Testing fio_memchr token values.");

  char buf[128];
  FIO_MEMSET(buf, 0x55, sizeof(buf));

  /* token = 0 (NUL byte) - critical for fio_has_zero_byte64 */
  {
    buf[32] = '\0';
    void *result = fio_memchr(buf, '\0', sizeof(buf));
    FIO_ASSERT(result == buf + 32, "fio_memchr failed to find NUL byte");
    buf[32] = 0x55;
  }

  /* token = 0xFF */
  {
    buf[48] = (char)0xFF;
    void *result = fio_memchr(buf, (char)0xFF, sizeof(buf));
    FIO_ASSERT(result == buf + 48, "fio_memchr failed to find 0xFF byte");
    buf[48] = 0x55;
  }

  /* token = 0x80 (high bit set) */
  {
    buf[64] = (char)0x80;
    void *result = fio_memchr(buf, (char)0x80, sizeof(buf));
    FIO_ASSERT(result == buf + 64, "fio_memchr failed to find 0x80 byte");
    buf[64] = 0x55;
  }

  /* token = 'A' (normal ASCII) */
  {
    buf[16] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + 16, "fio_memchr failed to find 'A'");
    buf[16] = 0x55;
  }
}

FIO_SFUNC void fio___test_memchr_size_boundaries(void) {
  FIO_LOG_DDEBUG("Testing fio_memchr size boundaries.");

  char buf[256];
  FIO_MEMSET(buf, 'X', sizeof(buf));

  /* len = 0 (return NULL) */
  {
    buf[0] = 'A';
    void *result = fio_memchr(buf, 'A', 0);
    FIO_ASSERT(result == NULL, "fio_memchr with len=0 should return NULL");
    buf[0] = 'X';
  }

  /* Test various sizes with token at end */
  size_t test_sizes[] = {1, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];
    buf[len - 1] = 'A';
    void *result = fio_memchr(buf, 'A', len);
    FIO_ASSERT(result == buf + len - 1, "fio_memchr failed for size %zu", len);
    buf[len - 1] = 'X';
  }
}

FIO_SFUNC void fio___test_memchr_simd_boundaries(void) {
  FIO_LOG_DDEBUG("Testing fio_memchr SIMD boundary cases.");

  char buf[256] FIO_ALIGN(32);
  FIO_MEMSET(buf, 'X', sizeof(buf));

  /* Token at SIMD vector boundaries: 15, 16, 31, 32, 63, 64 */
  size_t boundary_positions[] = {15, 16, 31, 32, 63, 64, 127, 128};
  size_t num_positions =
      sizeof(boundary_positions) / sizeof(boundary_positions[0]);

  for (size_t i = 0; i < num_positions; ++i) {
    size_t pos = boundary_positions[i];
    if (pos >= sizeof(buf))
      continue;

    buf[pos] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + pos,
               "fio_memchr failed at SIMD boundary position %zu",
               pos);
    buf[pos] = 'X';
  }

  /* Test with buffer unaligned by 1-15 bytes */
  for (size_t offset = 1; offset < 16; ++offset) {
    char *unaligned = buf + offset;
    size_t len = sizeof(buf) - offset - 1;
    FIO_MEMSET(unaligned, 'Y', len);
    unaligned[32] = 'B';

    void *result = fio_memchr(unaligned, 'B', len);
    FIO_ASSERT(result == unaligned + 32,
               "fio_memchr failed with unaligned buffer offset=%zu",
               offset);
  }
}

FIO_SFUNC void fio___test_memcmp_equal_buffers(void) {
  FIO_LOG_DDEBUG("Testing fio_memcmp equal buffers.");

  char a[128], b[128];
  fio_rand_bytes(a, sizeof(a));
  FIO_MEMCPY(b, a, sizeof(b));

  /* Test various sizes */
  size_t test_sizes[] = {0, 1, 7, 8, 63, 64, 65, 127, 128};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];
    int result = fio_memcmp(a, b, len);
    FIO_ASSERT(result == 0,
               "fio_memcmp should return 0 for equal buffers, size=%zu",
               len);
  }
}

FIO_SFUNC void fio___test_memcmp_difference_position(void) {
  FIO_LOG_DDEBUG("Testing fio_memcmp difference position.");

  char a[128], b[128];

  /* First byte differs (a > b) */
  {
    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x40, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: first byte a > b should return positive");
  }

  /* First byte differs (a < b) */
  {
    FIO_MEMSET(a, 0x40, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: first byte a < b should return negative");
  }

  /* Last byte differs (a > b) */
  {
    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    a[sizeof(a) - 1] = 0x60;
    b[sizeof(b) - 1] = 0x40;
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: last byte a > b should return positive");
  }

  /* Last byte differs (a < b) */
  {
    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    a[sizeof(a) - 1] = 0x40;
    b[sizeof(b) - 1] = 0x60;
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: last byte a < b should return negative");
  }

  /* Difference at boundary positions: 7, 8, 63, 64 */
  size_t boundary_positions[] = {7, 8, 63, 64};
  size_t num_positions =
      sizeof(boundary_positions) / sizeof(boundary_positions[0]);

  for (size_t i = 0; i < num_positions; ++i) {
    size_t pos = boundary_positions[i];
    if (pos >= sizeof(a))
      continue;

    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    a[pos] = 0x60;
    b[pos] = 0x40;

    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: difference at position %zu (a > b) failed",
               pos);

    a[pos] = 0x40;
    b[pos] = 0x60;
    result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: difference at position %zu (a < b) failed",
               pos);
  }
}

FIO_SFUNC void fio___test_memcmp_same_pointer(void) {
  FIO_LOG_DDEBUG("Testing fio_memcmp same pointer.");

  char buf[64];
  fio_rand_bytes(buf, sizeof(buf));

  int result = fio_memcmp(buf, buf, sizeof(buf));
  FIO_ASSERT(result == 0, "fio_memcmp(a, a, len) should return 0");
}

FIO_SFUNC void fio___test_memcmp_byte_value_edge_cases(void) {
  FIO_LOG_DDEBUG("Testing fio_memcmp byte value edge cases.");

  char a[8], b[8];

  /* 0x7F vs 0x80 (sign bit boundary - must use unsigned comparison) */
  {
    FIO_MEMSET(a, 0x7F, sizeof(a));
    FIO_MEMSET(b, 0x80, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: 0x7F < 0x80 (unsigned comparison required)");

    result = fio_memcmp(b, a, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: 0x80 > 0x7F (unsigned comparison required)");
  }

  /* 0x00 vs 0x01 */
  {
    FIO_MEMSET(a, 0x00, sizeof(a));
    FIO_MEMSET(b, 0x01, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0, "fio_memcmp: 0x00 < 0x01 failed");
  }

  /* 0xFE vs 0xFF */
  {
    FIO_MEMSET(a, 0xFE, sizeof(a));
    FIO_MEMSET(b, 0xFF, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0, "fio_memcmp: 0xFE < 0xFF failed");
  }
}

FIO_SFUNC void fio___test_strlen_lengths(void) {
  FIO_LOG_DDEBUG("Testing fio_strlen string lengths.");

  /* Empty string "" (return 0) */
  FIO_ASSERT(fio_strlen("") == 0, "fio_strlen(\"\") should return 0");

  /* Single char "a" (return 1) */
  FIO_ASSERT(fio_strlen("a") == 1, "fio_strlen(\"a\") should return 1");

  /* Test with string literals of known lengths */
  FIO_ASSERT(fio_strlen("1234567") == 7, "fio_strlen 7-char string failed");
  FIO_ASSERT(fio_strlen("12345678") == 8, "fio_strlen 8-char string failed");
  FIO_ASSERT(fio_strlen("123456789012345") == 15,
             "fio_strlen 15-char string failed");
  FIO_ASSERT(fio_strlen("1234567890123456") == 16,
             "fio_strlen 16-char string failed");

  /* Test longer strings with dynamically created buffers */
  char buf[512] FIO_ALIGN(64);
  size_t test_lengths[] = {31, 32, 63, 64, 127, 128, 255, 256};
  size_t num_lengths = sizeof(test_lengths) / sizeof(test_lengths[0]);

  for (size_t i = 0; i < num_lengths; ++i) {
    size_t len = test_lengths[i];
    if (len >= sizeof(buf) - 1)
      continue;

    /* Reset buffer and set NUL at desired position */
    FIO_MEMSET(buf, 'A', sizeof(buf));
    buf[len] = '\0';

    size_t result = fio_strlen(buf);
    FIO_ASSERT(result == len,
               "fio_strlen failed for length %zu (got %zu)",
               len,
               result);
  }
}

FIO_SFUNC void fio___test_strlen_null_pointer(void) {
  FIO_LOG_DDEBUG("Testing fio_strlen NULL pointer.");

  size_t result = fio_strlen(NULL);
  FIO_ASSERT(result == 0, "fio_strlen(NULL) should return 0");
}

FIO_SFUNC void fio___test_strlen_alignment(void) {
  FIO_LOG_DDEBUG("Testing fio_strlen alignment variations.");

  char buf[128 + 16] FIO_ALIGN(16);
  FIO_MEMSET(buf, 'B', sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  /* Test string starting at various alignment offsets */
  for (size_t offset = 0; offset < 8; ++offset) {
    char *str = buf + offset;
    size_t expected_len = sizeof(buf) - 1 - offset;

    size_t result = fio_strlen(str);
    FIO_ASSERT(result == expected_len,
               "fio_strlen alignment test failed: offset=%zu, expected=%zu, "
               "got=%zu",
               offset,
               expected_len,
               result);
  }
}

FIO_SFUNC void fio___test_strlen_simd_boundaries(void) {
  FIO_LOG_DDEBUG("Testing fio_strlen SIMD boundary cases.");

  char buf[256] FIO_ALIGN(32);

  /* NUL at SIMD vector boundaries: 15, 16, 31, 32, 63, 64 */
  size_t boundary_positions[] = {15, 16, 31, 32, 63, 64, 127, 128};
  size_t num_positions =
      sizeof(boundary_positions) / sizeof(boundary_positions[0]);

  for (size_t i = 0; i < num_positions; ++i) {
    size_t pos = boundary_positions[i];
    if (pos >= sizeof(buf))
      continue;

    FIO_MEMSET(buf, 'C', sizeof(buf));
    buf[pos] = '\0';

    size_t result = fio_strlen(buf);
    FIO_ASSERT(result == pos,
               "fio_strlen failed at SIMD boundary position %zu (got %zu)",
               pos,
               result);
  }
}

FIO_SFUNC void fio___test_memcpyXx_masking(void) {
  FIO_LOG_DDEBUG("Testing fio_memcpyXx masking behavior.");

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
  FIO_LOG_DDEBUG("Testing value-returning SIMD operations (_xorv, _andv, _orv, "
                 "_addv64).");
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
Main Test Entry Point
***************************************************************************** */

int main(void) {
  FIO_LOG_DDEBUG("Testing Core Module");

  /* Run all functional tests */
  fio___test_core_memcpy_primitives();
  fio___test_core_byte_swap();
  fio___test_core_bit_rotation();
  fio___test_core_bit_index();
  fio___test_core_buf_helpers();
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

  /* Memory function edge case tests (FIO_MEMALT) */
  fio___test_memcpy_overlap();
  fio___test_memcpy_null_handling();
  fio___test_memcpy_size_boundaries();
  fio___test_memcpy_alignment();
  fio___test_memset_size_boundaries();
  fio___test_memset_data_patterns();
  fio___test_memset_alignment();
  fio___test_memchr_token_position();
  fio___test_memchr_token_values();
  fio___test_memchr_size_boundaries();
  fio___test_memchr_simd_boundaries();
  fio___test_memcmp_equal_buffers();
  fio___test_memcmp_difference_position();
  fio___test_memcmp_same_pointer();
  fio___test_memcmp_byte_value_edge_cases();
  fio___test_strlen_lengths();
  fio___test_strlen_null_pointer();
  fio___test_strlen_alignment();
  fio___test_strlen_simd_boundaries();
  fio___test_memcpyXx_masking();

  FIO_LOG_DDEBUG("Core Module tests complete.");
  FIO_LOG_DDEBUG("(Performance tests moved to tests/performance-core.c)");
  return 0;
}
