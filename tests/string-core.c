/* *****************************************************************************
Test - Comprehensive FIO_STR String Core Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_STR
#include FIO_INCLUDE_FILE

/* =============================================================================
 * Test: FIO_STR_INFO and FIO_BUF_INFO macros
 * ========================================================================== */
FIO_SFUNC void test_str_info_macros(void) {
  /* FIO_STR_INFO1 - from NUL-terminated string */
  {
    char *s = "Hello";
    fio_str_info_s info = FIO_STR_INFO1(s);
    FIO_ASSERT(info.buf == s, "FIO_STR_INFO1 buf mismatch");
    FIO_ASSERT(info.len == 5, "FIO_STR_INFO1 len mismatch: %zu", info.len);
  }

  /* FIO_STR_INFO2 - from buffer and length */
  {
    char *s = "Hello World";
    fio_str_info_s info = FIO_STR_INFO2(s, 5);
    FIO_ASSERT(info.buf == s, "FIO_STR_INFO2 buf mismatch");
    FIO_ASSERT(info.len == 5, "FIO_STR_INFO2 len mismatch");
  }

  /* FIO_STR_INFO3 - from buffer, length, and capacity */
  {
    char buf[32];
    fio_str_info_s info = FIO_STR_INFO3(buf, 0, 32);
    FIO_ASSERT(info.buf == buf, "FIO_STR_INFO3 buf mismatch");
    FIO_ASSERT(info.len == 0, "FIO_STR_INFO3 len mismatch");
    FIO_ASSERT(info.capa == 32, "FIO_STR_INFO3 capa mismatch");
  }

  /* FIO_STR_INFO_IS_EQ */
  {
    fio_str_info_s a = FIO_STR_INFO1("Hello");
    fio_str_info_s b = FIO_STR_INFO2("Hello", 5);
    fio_str_info_s c = FIO_STR_INFO1("World");
    FIO_ASSERT(FIO_STR_INFO_IS_EQ(a, b), "FIO_STR_INFO_IS_EQ equal strings");
    FIO_ASSERT(!FIO_STR_INFO_IS_EQ(a, c),
               "FIO_STR_INFO_IS_EQ different strings");
  }

  /* FIO_BUF_INFO macros */
  {
    fio_buf_info_s a = FIO_BUF_INFO1("Test");
    fio_buf_info_s b = FIO_BUF_INFO2("Test", 4);
    FIO_ASSERT(a.len == 4, "FIO_BUF_INFO1 len mismatch");
    FIO_ASSERT(FIO_BUF_INFO_IS_EQ(a, b), "FIO_BUF_INFO_IS_EQ mismatch");
  }
}

/* =============================================================================
 * Test: fio_string_write - edge cases
 * ========================================================================== */
FIO_SFUNC void test_string_write(void) {
  /* Basic write */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    int r = fio_string_write(&s, NULL, "Hello", 5);
    FIO_ASSERT(r == 0, "fio_string_write should return 0 on success");
    FIO_ASSERT(s.len == 5, "len should be 5, got %zu", s.len);
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello", 6), "content mismatch");
  }

  /* Write to buffer with exact capacity */
  {
    char buf[6];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 6);
    int r = fio_string_write(&s, NULL, "Hello", 5);
    FIO_ASSERT(r == 0, "exact capacity write should succeed");
    FIO_ASSERT(s.len == 5, "len should be 5");
  }

  /* Write with truncation */
  {
    char buf[4];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 4);
    int r = fio_string_write(&s, NULL, "Hello", 5);
    FIO_ASSERT(r != 0, "truncation should return non-zero");
    FIO_ASSERT(s.len == 3, "truncated len should be 3, got %zu", s.len);
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hel", 4), "truncated content mismatch");
  }

  /* Write zero-length data */
  {
    char buf[16];
    fio_str_info_s s = FIO_STR_INFO3(buf, 5, 16);
    FIO_MEMCPY(buf, "Hello", 5);
    int r = fio_string_write(&s, NULL, "X", 0);
    FIO_ASSERT(r == 0, "zero-length write should succeed");
    FIO_ASSERT(s.len == 5, "len should remain 5");
  }

  /* Multiple writes */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write(&s, NULL, "Hello", 5);
    fio_string_write(&s, NULL, " ", 1);
    fio_string_write(&s, NULL, "World", 5);
    FIO_ASSERT(s.len == 11, "multiple writes len should be 11");
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello World", 12),
               "multiple writes content");
  }
}

/* =============================================================================
 * Test: fio_string_replace - comprehensive
 * ========================================================================== */
FIO_SFUNC void test_string_replace(void) {
  /* Insert at beginning */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write(&s, NULL, "World", 5);
    fio_string_replace(&s, NULL, 0, 0, "Hello ", 6);
    FIO_ASSERT(s.len == 11, "insert at start len");
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello World", 12),
               "insert at start content");
  }

  /* Insert in middle */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write(&s, NULL, "HelloWorld", 10);
    fio_string_replace(&s, NULL, 5, 0, " ", 1);
    FIO_ASSERT(s.len == 11, "insert in middle len");
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello World", 12),
               "insert in middle content");
  }

  /* Replace in middle */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write(&s, NULL, "Hello World", 11);
    fio_string_replace(&s, NULL, 6, 5, "Universe", 8);
    FIO_ASSERT(s.len == 14, "replace len: %zu", s.len);
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello Universe", 15), "replace content");
  }

  /* Delete (replace with empty) */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write(&s, NULL, "Hello World", 11);
    fio_string_replace(&s, NULL, 5, 6, "", 0);
    FIO_ASSERT(s.len == 5, "delete len: %zu", s.len);
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello", 5), "delete content");
  }

  /* Negative index: -1 == end of string, -6 == position of 'W' in "Hello World"
   */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write(&s, NULL, "Hello World", 11);
    fio_string_replace(&s, NULL, -6, 5, "Earth", 5);
    FIO_ASSERT(s.len == 11, "negative index len: %zu", s.len);
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello Earth", 11), "negative index content");
  }
}

/* =============================================================================
 * Test: fio_string_write2 - format specifiers
 * ========================================================================== */
FIO_SFUNC void test_string_write2(void) {
  /* String specifiers */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write2(&s,
                      NULL,
                      FIO_STRING_WRITE_STR1("Hello"),
                      FIO_STRING_WRITE_STR2(" World", 6));
    FIO_ASSERT(s.len == 11, "STR write2 len");
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Hello World", 12), "STR write2 content");
  }

  /* Number specifiers */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write2(&s,
                      NULL,
                      FIO_STRING_WRITE_STR1("Val: "),
                      FIO_STRING_WRITE_NUM(-42));
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "Val: -42", 9),
               "NUM write2 content: %s",
               s.buf);
  }

  /* Unsigned number */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write2(&s, NULL, FIO_STRING_WRITE_UNUM(42));
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "42", 3), "UNUM content: %s", s.buf);
  }

  /* Hex */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write2(&s, NULL, FIO_STRING_WRITE_HEX(255));
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "FF", 3), "HEX content: %s", s.buf);
  }

  /* Float */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write2(&s,
                      NULL,
                      FIO_STRING_WRITE_FLOAT(3.14),
                      FIO_STRING_WRITE_STR1(" "),
                      FIO_STRING_WRITE_FLOAT(42.0));
    FIO_ASSERT(s.len == 9 && !FIO_MEMCMP(s.buf, "3.14 42.0", 10),
               "FLOAT content (fio_ftoa): %s",
               s.buf);
  }
}

/* fio_string_write2 float bounds — ASan catches any heap overflow here. */
FIO_SFUNC void test_string_write2_float_overflow(void) {
  char prefix[12];
  FIO_MEMSET(prefix, 'A', sizeof(prefix));
  double v = 1.7976931348623157e+308;

  fio_str_info_s s = FIO_STR_INFO0;
  fio_string_write2(&s,
                    fio_string_sys_reallocate,
                    FIO_STRING_WRITE_STR2(prefix, sizeof(prefix)),
                    FIO_STRING_WRITE_FLOAT(v));
  FIO_ASSERT(!FIO_MEMCMP(s.buf, prefix, sizeof(prefix)),
             "float overflow test: prefix overwritten");
  FIO_ASSERT(s.buf[s.len] == 0, "float overflow test: missing NUL terminator");
  FIO_ASSERT(s.capa > s.len, "float overflow test: capa should exceed len");
  FIO_MEM_FREE(s.buf, s.capa);
}

/* =============================================================================
 * Test: Numeral functions - boundary testing
 * ========================================================================== */
FIO_SFUNC void test_string_numerals(void) {
  /* fio_string_write_i */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_i(&s, NULL, 0);
    FIO_ASSERT(s.len == 1 && buf[0] == '0', "write_i(0)");

    s.len = 0;
    fio_string_write_i(&s, NULL, -1);
    FIO_ASSERT(!FIO_MEMCMP(buf, "-1", 3), "write_i(-1)");

    s.len = 0;
    fio_string_write_i(&s, NULL, 123456789);
    FIO_ASSERT(!FIO_MEMCMP(buf, "123456789", 10), "write_i positive");
  }

  /* fio_string_write_u */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_u(&s, NULL, 0);
    FIO_ASSERT(s.len == 1 && buf[0] == '0', "write_u(0)");

    s.len = 0;
    fio_string_write_u(&s, NULL, 18446744073709551615ULL);
    FIO_ASSERT(s.len == 20, "write_u(UINT64_MAX) len: %zu", s.len);
  }

  /* fio_string_write_hex */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_hex(&s, NULL, 0);
    FIO_ASSERT(!FIO_MEMCMP(buf, "00", 3), "write_hex(0)");

    s.len = 0;
    fio_string_write_hex(&s, NULL, 0xFF);
    FIO_ASSERT(!FIO_MEMCMP(buf, "FF", 3), "write_hex(0xFF)");

    s.len = 0;
    fio_string_write_hex(&s, NULL, 0xDEADBEEF);
    FIO_ASSERT(!FIO_MEMCMP(buf, "DEADBEEF", 9), "write_hex(0xDEADBEEF)");
  }

  /* fio_string_write_bin - binary is padded to even number of digits */
  {
    char buf[128];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 128);
    fio_string_write_bin(&s, NULL, 0);
    FIO_ASSERT(s.len == 1 && buf[0] == '0', "write_bin(0)");

    s.len = 0;
    fio_string_write_bin(&s, NULL, 5);
    /* 5 = 101 binary, padded to even = 0101 */
    FIO_ASSERT(!FIO_MEMCMP(buf, "0101", 5), "write_bin(5): %s", buf);

    s.len = 0;
    fio_string_write_bin(&s, NULL, 255);
    FIO_ASSERT(!FIO_MEMCMP(buf, "11111111", 9), "write_bin(255)");
  }
}

/* =============================================================================
 * Test: printf/vprintf
 * ========================================================================== */
FIO_SFUNC void test_string_printf(void) {
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_printf(&s, NULL, "Hello %s, number %d", "World", 42);
    FIO_ASSERT(!FIO_MEMCMP(buf, "Hello World, number 42", 23),
               "printf content: %s",
               buf);
  }

  /* Truncation */
  {
    char buf[16];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 16);
    int r = fio_string_printf(&s, NULL, "This is a very long string %d", 12345);
    FIO_ASSERT(r != 0, "printf should indicate truncation");
    FIO_ASSERT(s.len <= 15, "printf should truncate");
  }
}

/* =============================================================================
 * Test: UTF-8 functions
 * ========================================================================== */
FIO_SFUNC void test_string_utf8(void) {
  /* Valid UTF-8 */
  {
    fio_str_info_s ascii = FIO_STR_INFO1("Hello");
    FIO_ASSERT(fio_string_utf8_valid(ascii), "ASCII should be valid UTF-8");
    FIO_ASSERT(fio_string_utf8_len(ascii) == 5, "ASCII UTF-8 len");
  }

  /* Multi-byte UTF-8 */
  {
    /* UTF-8: 2-byte (é), 3-byte (€), 4-byte (😀) */
    const char *utf8_2byte = "\xC3\xA9";         /* é */
    const char *utf8_3byte = "\xE2\x82\xAC";     /* € */
    const char *utf8_4byte = "\xF0\x9F\x98\x80"; /* 😀 */

    fio_str_info_s s2 = FIO_STR_INFO1((char *)utf8_2byte);
    fio_str_info_s s3 = FIO_STR_INFO1((char *)utf8_3byte);
    fio_str_info_s s4 = FIO_STR_INFO1((char *)utf8_4byte);

    FIO_ASSERT(fio_string_utf8_valid(s2), "2-byte UTF-8 valid");
    FIO_ASSERT(fio_string_utf8_valid(s3), "3-byte UTF-8 valid");
    FIO_ASSERT(fio_string_utf8_valid(s4), "4-byte UTF-8 valid");

    FIO_ASSERT(fio_string_utf8_len(s2) == 1, "2-byte is 1 char");
    FIO_ASSERT(fio_string_utf8_len(s3) == 1, "3-byte is 1 char");
    FIO_ASSERT(fio_string_utf8_len(s4) == 1, "4-byte is 1 char");
  }

  /* Invalid UTF-8 */
  {
    const char invalid[] = {(char)0xFF, (char)0xFE, 0};
    fio_str_info_s s = FIO_STR_INFO2((char *)invalid, (sizeof(invalid) - 1));
    FIO_ASSERT(!fio_string_utf8_valid(s), "Invalid bytes should fail");
  }

  /* UTF-8 select */
  {
    const char *mixed = "A\xC3\xA9Z"; /* A + é + Z = 3 chars */
    fio_str_info_s s = FIO_STR_INFO1((char *)mixed);
    intptr_t pos = 1;
    size_t len = 1;
    int r = fio_string_utf8_select(s, &pos, &len);
    FIO_ASSERT(r == 0, "utf8_select should succeed");
    FIO_ASSERT(pos == 1, "utf8_select pos should be 1");
    FIO_ASSERT(len == 2, "utf8_select len should be 2 (é is 2 bytes)");
  }

  /* Regression: a multi-byte lead truncated by the buffer's end must be
   * rejected without reading past the buffer (exact-size heap copies so
   * ASan reports any overread). */
  {
    static const struct {
      const char *s;
      size_t len;
    } tail[] = {
        {"\xF0\x9F", 2},
        {"a\xE2\x82", 3},
        {"ab\xC3", 3},
        {"\xF4", 1},
    };
    for (size_t i = 0; i < sizeof(tail) / sizeof(tail[0]); ++i) {
      char *b = (char *)malloc(tail[i].len);
      FIO_ASSERT_ALLOC(b);
      FIO_MEMCPY(b, tail[i].s, tail[i].len);
      fio_str_info_s s = FIO_STR_INFO2(b, tail[i].len);
      size_t lead = 0;
      while ((((uint8_t *)b)[lead] & 0xC0U) != 0xC0U)
        ++lead;
      FIO_ASSERT(
          !fio_string_utf8_valid_code_point(b + lead, tail[i].len - lead),
          "truncated UTF-8 tail should be invalid (%zu)",
          i);
      FIO_ASSERT(!fio_string_utf8_valid(s), "truncated tail valid (%zu)", i);
      FIO_ASSERT(!fio_string_utf8_len(s), "truncated tail has len (%zu)", i);
      intptr_t pos = 0;
      size_t len = 8;
      FIO_ASSERT(fio_string_utf8_select(s, &pos, &len) == -1 && pos == -1,
                 "truncated tail selectable (%zu)",
                 i);
      char out[64];
      fio_str_info_s dest = FIO_STR_INFO3(out, 0, sizeof(out));
      fio_string_write_html_escape(&dest, NULL, b, tail[i].len);
      free(b);
    }
    /* a complete sequence at the exact buffer end is still accepted */
    char *b = (char *)malloc(2);
    FIO_ASSERT_ALLOC(b);
    FIO_MEMCPY(b, "\xC3\xA9", 2);
    FIO_ASSERT(fio_string_utf8_valid_code_point(b, 2) == 2 &&
                   !fio_string_utf8_valid_code_point(b, 1),
               "bounded code point length");
    FIO_ASSERT(fio_string_utf8_len(FIO_STR_INFO2(b, 2)) == 1,
               "exact-end UTF-8 len");
    free(b);
  }

  /* ASCII word skipping + bounded tail: place complete / truncated / invalid
   * sequences at every offset around the 8 byte word boundary (exact-size
   * heap buffers, so ASan reports any overread). */
  {
    static const struct {
      const char *s;
      size_t len;
      size_t chars; /* 0 == invalid */
    } seq[] = {
        {"\xC3\xA9", 2, 1},
        {"\xE2\x82\xAC", 3, 1},
        {"\xF0\x9F\x98\x80", 4, 1},
        {"\xF0\x9F\x98", 3, 0},
        {"\xE2\x82", 2, 0},
        {"\xC3", 1, 0},
        {"\xED\xA0\x80", 3, 0}, /* surrogate */
        {"\x80", 1, 0},
    };
    for (size_t i = 0; i < sizeof(seq) / sizeof(seq[0]); ++i) {
      for (size_t pre = 0; pre < 18; ++pre) {
        for (size_t post = 0; post < 10; ++post) {
          const size_t total = pre + seq[i].len + post;
          char *b = (char *)malloc(total);
          FIO_ASSERT_ALLOC(b);
          FIO_MEMSET(b, 'a', total);
          FIO_MEMCPY(b + pre, seq[i].s, seq[i].len);
          fio_str_info_s s = FIO_STR_INFO2(b, total);
          const size_t expect = seq[i].chars ? (pre + 1 + post) : 0;
          FIO_ASSERT(fio_string_utf8_valid(s) == !!seq[i].chars,
                     "UTF-8 valid mismatch (seq %zu, pre %zu, post %zu)",
                     i,
                     pre,
                     post);
          FIO_ASSERT(fio_string_utf8_len(s) == expect,
                     "UTF-8 len mismatch (seq %zu, pre %zu, post %zu)",
                     i,
                     pre,
                     post);
          free(b);
        }
      }
    }
  }
}

/* =============================================================================
 * Test: Escape/Unescape
 * ========================================================================== */
FIO_SFUNC void test_string_escape(void) {
  /* Escape special characters */
  {
    char buf[128];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 128);
    const char *input = "Hello\nWorld\t\"Test\"\\End";
    fio_string_write_escape(&s, NULL, input, FIO_STRLEN(input));
    FIO_ASSERT(s.len > FIO_STRLEN(input), "escaped should be longer");
  }

  /* Truncated UTF-8 tail: escape byte-wise, deterministically (no OOB read) */
  {
    char buf[128];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 128);
    const char input[] = {'a', 'b', 'c', (char)0xC3}; /* lone 2-byte lead */
    fio_string_write_escape(&s, NULL, input, sizeof(input));
    FIO_ASSERT(s.len == 7, "truncated UTF-8 lead should be escaped as \\xXX");
    FIO_ASSERT(!FIO_MEMCMP(s.buf, "abc\\xC3", 7),
               "truncated UTF-8 lead should escape to \\xC3");
    /* valid UTF-8 passes through untouched */
    s.len = 0;
    const char valid[] = {'x', (char)0xC3, (char)0xA9, 'y'}; /* xé y */
    fio_string_write_escape(&s, NULL, valid, sizeof(valid));
    FIO_ASSERT(s.len == sizeof(valid) &&
                   !FIO_MEMCMP(s.buf, valid, sizeof(valid)),
               "valid UTF-8 should pass through");
    /* 3-byte lead truncated to 2 bytes: escape both bytes */
    s.len = 0;
    const char trunc3[] = {'z', (char)0xE1, (char)0x80};
    fio_string_write_escape(&s, NULL, trunc3, sizeof(trunc3));
    FIO_ASSERT(s.len == 9 && !FIO_MEMCMP(s.buf, "z\\xE1\\x80", 9),
               "truncated 3-byte UTF-8 tail should escape byte-wise");
  }

  /* Roundtrip */
  {
    char buf1[128], buf2[128];
    fio_str_info_s escaped = FIO_STR_INFO3(buf1, 0, 128);
    fio_str_info_s unescaped = FIO_STR_INFO3(buf2, 0, 128);
    const char *original = "Test\n\t\r\"\\";

    fio_string_write_escape(&escaped, NULL, original, FIO_STRLEN(original));
    fio_string_write_unescape(&unescaped, NULL, escaped.buf, escaped.len);

    FIO_ASSERT(unescaped.len == FIO_STRLEN(original), "roundtrip len");
    FIO_ASSERT(!FIO_MEMCMP(unescaped.buf, original, unescaped.len),
               "roundtrip content");
  }

  /* Unicode escape */
  {
    char buf[128];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 128);
    fio_string_write_unescape(&s, NULL, "\\u0041", 6); /* A */
    FIO_ASSERT(s.len == 1 && buf[0] == 'A', "unicode escape \\u0041 = A");
  }

  /* Hex escape */
  {
    char buf[128];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 128);
    fio_string_write_unescape(&s, NULL, "\\x41", 4); /* A */
    FIO_ASSERT(s.len == 1 && buf[0] == 'A', "hex escape \\x41 = A");
  }
}

/* =============================================================================
 * Test: Base64 encoding/decoding
 * ========================================================================== */
FIO_SFUNC void test_string_base64(void) {
  /* Known vectors */
  struct {
    const char *plain;
    const char *encoded;
  } vectors[] = {
      {"", ""},
      {"f", "Zg=="},
      {"fo", "Zm8="},
      {"foo", "Zm9v"},
      {"foob", "Zm9vYg=="},
      {"fooba", "Zm9vYmE="},
      {"foobar", "Zm9vYmFy"},
  };

  for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
    if (!vectors[i].plain[0])
      continue; /* skip empty */

    char enc_buf[64], dec_buf[64];
    fio_str_info_s enc = FIO_STR_INFO3(enc_buf, 0, 64);
    fio_str_info_s dec = FIO_STR_INFO3(dec_buf, 0, 64);

    fio_string_write_base64enc(&enc,
                               NULL,
                               vectors[i].plain,
                               FIO_STRLEN(vectors[i].plain),
                               0);
    FIO_ASSERT(!FIO_MEMCMP(enc.buf, vectors[i].encoded, enc.len),
               "base64 encode '%s' -> '%s' (got '%.*s')",
               vectors[i].plain,
               vectors[i].encoded,
               (int)enc.len,
               enc.buf);

    fio_string_write_base64dec(&dec,
                               NULL,
                               vectors[i].encoded,
                               FIO_STRLEN(vectors[i].encoded));
    FIO_ASSERT(dec.len == FIO_STRLEN(vectors[i].plain), "base64 decode len");
    FIO_ASSERT(!FIO_MEMCMP(dec.buf, vectors[i].plain, dec.len),
               "base64 decode content");
  }

  /* Binary data roundtrip */
  {
    char enc_buf[256], dec_buf[256];
    char original[128];
    for (int i = 0; i < 128; ++i)
      original[i] = (char)i;

    fio_str_info_s enc = FIO_STR_INFO3(enc_buf, 0, 256);
    fio_str_info_s dec = FIO_STR_INFO3(dec_buf, 0, 256);

    fio_string_write_base64enc(&enc, NULL, original, 128, 0);
    fio_string_write_base64dec(&dec, NULL, enc.buf, enc.len);

    FIO_ASSERT(dec.len == 128, "binary roundtrip len: %zu", dec.len);
    FIO_ASSERT(!FIO_MEMCMP(dec.buf, original, 128), "binary roundtrip content");
  }

  /* decoder: accepted forms (RFC 4648 vectors, unpadded, whitespace, URL) */
  {
    static const struct {
      const char *in;
      const char *out;
      size_t out_len;
    } ok[] = {
        {"", "", 0},
        {"   \r\n\t ", "", 0},
        {"Zg==", "f", 1},
        {"Zm8=", "fo", 2},
        {"Zm9v", "foo", 3},
        {"Zm9vYg==", "foob", 4},
        {"Zm9vYmE=", "fooba", 5},
        {"Zm9vYmFy", "foobar", 6},
        {"Zg", "f", 1},
        {"Zm8", "fo", 2},
        {"Zm9vYmE", "fooba", 5},
        {" Z m 9 v\r\nY g = = \n", "foob", 4},
        {"Zm9v\vYmFy\f", "foobar", 6},
        {"-_-_", "\xFB\xFF\xBF", 3},
        {"+/+/", "\xFB\xFF\xBF", 3},
        {"+_-/", "\xFB\xFF\xBF", 3},
        /* lenient: padding variants, padding only, \b, non-zero tail bits */
        {"Zg=", "f", 1},
        {"Zg===", "f", 1},
        {"Zm8==", "fo", 2},
        {"Zm9v=", "foo", 3},
        {"Zm9v====", "foo", 3},
        {"====", "", 0},
        {"\bZm9v\b", "foo", 3},
        {"Zh==", "f", 1},
        {"Zm9=", "fo", 2},
    };
    for (size_t i = 0; i < sizeof(ok) / sizeof(ok[0]); ++i) {
      char db[16];
      fio_str_info_s dec = FIO_STR_INFO3(db, 0, sizeof(db));
      FIO_ASSERT(!fio_string_write_base64dec(&dec,
                                             NULL,
                                             ok[i].in,
                                             FIO_STRLEN(ok[i].in)) &&
                     dec.len == ok[i].out_len &&
                     !FIO_MEMCMP(db, ok[i].out, dec.len),
                 "base64 decode should accept \"%s\"",
                 ok[i].in);
    }
    fio_str_info_s empty = {0};
    FIO_ASSERT(!fio_string_write_base64dec(&empty, NULL, NULL, 0) &&
                   !empty.buf && !empty.len,
               "base64 NULL/0 decode should succeed without changes");
    /* whitespace heavy input must not fail an exact capacity check */
    char spaced[256];
    size_t sl = 0;
    for (const char *c = "Zm9vYmFy"; *c; ++c) {
      for (size_t k = 0; k < 20; ++k)
        spaced[sl++] = ' ';
      spaced[sl++] = *c;
    }
    char db[7]; /* exactly "foobar" + NUL */
    fio_str_info_s dec = FIO_STR_INFO3(db, 0, sizeof(db));
    FIO_ASSERT(!fio_string_write_base64dec(&dec, NULL, spaced, sl) &&
                   dec.len == 6 && !FIO_MEMCMP(db, "foobar", 7),
               "base64 whitespace heavy decode should fit an exact buffer");
  }
  /* decoder: rejected forms leave `dest` unchanged */
  {
    static const struct {
      const char *in;
      size_t len;
    } bad[] = {
        {"Zm9v!YmFy", 9},     /* invalid character mid-stream */
        {"Zm9vYmFy!", 9},     /* trailing garbage */
        {"!", 1},             /* invalid only */
        {"Zm9v\0Ym", 7},      /* embedded NUL */
        {"Zm9vYmFy\x80", 9},  /* non-ASCII */
        {"Zg==Zg==", 8},      /* data after padding */
        {"Zg=a", 4},          /* data after padding */
        {"Z", 1},             /* impossible length */
        {"Zm9vY", 5},         /* impossible length */
        {"Zm9vY===", 8},      /* impossible length + bad padding */
        {"Zg=\x01", 4},       /* invalid character after padding */
        {"TQ!===", 6},        /* historical partial-success case */
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
      char db[32];
      FIO_MEMCPY(db, "keep", 5);
      fio_str_info_s dec = FIO_STR_INFO3(db, 4, sizeof(db));
      FIO_ASSERT(fio_string_write_base64dec(&dec, NULL, bad[i].in, bad[i].len) ==
                         -1 &&
                     dec.len == 4 && dec.buf == db && dec.capa == sizeof(db) &&
                     !FIO_MEMCMP(db, "keep", 5),
                 "base64 decode should reject case %zu without changes",
                 i);
      fio_str_info_s dyn = {0};
      FIO_ASSERT(fio_string_write_base64dec(&dyn,
                                            FIO_STRING_ALLOC_COPY,
                                            bad[i].in,
                                            bad[i].len) == -1 &&
                     !dyn.buf && !dyn.len,
                 "base64 decode should reject case %zu (no allocation)",
                 i);
    }
  }
}

/* =============================================================================
 * Regression test: V2 — fio_string_write_base64dec OOB read (CWE-125)
 *
 * After decoding, the old padding-detection code read encoded[-1] and
 * encoded[-2] unconditionally. For a single-character input this read one byte
 * before the start of the provided buffer. The input is heap-allocated at the
 * exact size so AddressSanitizer can detect the under-read on unpatched code.
 * ========================================================================== */
FIO_SFUNC void test_string_base64_short_input(void) {
  char *in = (char *)FIO_MEM_REALLOC(NULL, 0, 1, 0);
  FIO_ASSERT_ALLOC(in);
  in[0] = 'A';

  char out[64];
  fio_str_info_s s = FIO_STR_INFO3(out, 0, sizeof(out));
  out[0] = 0;
  int r = fio_string_write_base64dec(&s, NULL, in, 1);
  /* a single symbol can't encode a byte: rejected, `dest` unchanged */
  FIO_ASSERT(r == -1, "base64 short input: should report an error");
  FIO_ASSERT(s.len == 0 && !out[0], "base64 short input: dest changed");
  in[0] = '=';
  r = fio_string_write_base64dec(&s, NULL, in, 1);
  FIO_ASSERT(r == 0 && !s.len && !out[0],
             "base64 short input '=': padding only is an empty decode");

  FIO_MEM_FREE(in, 1);
}

/* =============================================================================
 * Test: Base32 encoding/decoding
 * ========================================================================== */
FIO_SFUNC void test_string_base32(void) {
  /* Roundtrip with simple data */
  {
    char enc_buf[256], dec_buf[256];
    const char *original = "Hello";

    fio_str_info_s enc = FIO_STR_INFO3(enc_buf, 0, 256);
    fio_str_info_s dec = FIO_STR_INFO3(dec_buf, 0, 256);

    int r1 =
        fio_string_write_base32enc(&enc, NULL, original, FIO_STRLEN(original));
    FIO_ASSERT(r1 == 0, "base32 encode should succeed");
    FIO_ASSERT(enc.len > 0, "base32 encoded len should be > 0");

    int r2 = fio_string_write_base32dec(&dec, NULL, enc.buf, enc.len);
    FIO_ASSERT(r2 == 0, "base32 decode should succeed");
    FIO_ASSERT(dec.len == FIO_STRLEN(original),
               "base32 roundtrip len: got %zu, expected %zu",
               dec.len,
               FIO_STRLEN(original));
    FIO_ASSERT(!FIO_MEMCMP(dec.buf, original, dec.len),
               "base32 roundtrip content");
  }
  /* RFC 4648 section 10 test vectors (encode: canonical, padded) */
  {
    static const char *vec[][2] = {{"", ""},
                                   {"f", "MY======"},
                                   {"fo", "MZXQ===="},
                                   {"foo", "MZXW6==="},
                                   {"foob", "MZXW6YQ="},
                                   {"fooba", "MZXW6YTB"},
                                   {"foobar", "MZXW6YTBOI======"}};
    for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); ++i) {
      char eb[64], db[64];
      fio_str_info_s enc = FIO_STR_INFO3(eb, 0, sizeof(eb));
      fio_str_info_s dec = FIO_STR_INFO3(db, 0, sizeof(db));
      FIO_ASSERT(!fio_string_write_base32enc(&enc,
                                             NULL,
                                             vec[i][0],
                                             FIO_STRLEN(vec[i][0])) &&
                     enc.len == FIO_STRLEN(vec[i][1]) &&
                     !FIO_MEMCMP(enc.buf, vec[i][1], enc.len) && !eb[enc.len],
                 "base32 RFC 4648 encode \"%s\" -> %s",
                 vec[i][0],
                 enc.buf);
      FIO_ASSERT(!fio_string_write_base32dec(&dec,
                                             NULL,
                                             vec[i][1],
                                             FIO_STRLEN(vec[i][1])) &&
                     dec.len == FIO_STRLEN(vec[i][0]) &&
                     !FIO_MEMCMP(dec.buf, vec[i][0], dec.len),
                 "base32 RFC 4648 decode %s",
                 vec[i][1]);
    }
  }
  /* decoder: accepted forms (unpadded, lower case, whitespace heavy) */
  {
    static const char *ok[] = {"MZXW6",
                               "mzxw6===",
                               " M Z\tX\r\nW 6 = = = ",
                               "MZXW6=",
                               "   \n\t  ",
                               "\v\fMZXW6\b",
                               "MZXW7",
                               "MZXW6==========",
                               "========"};
    static const size_t ok_len[] = {3, 3, 3, 3, 0, 3, 3, 3, 0};
    for (size_t i = 0; i < sizeof(ok) / sizeof(ok[0]); ++i) {
      char db[8];
      fio_str_info_s dec = FIO_STR_INFO3(db, 0, sizeof(db));
      FIO_ASSERT(!fio_string_write_base32dec(&dec, NULL, ok[i], FIO_STRLEN(ok[i])) &&
                     dec.len == ok_len[i] &&
                     (!dec.len || !FIO_MEMCMP(db, "foo", 3)),
                 "base32 decode should accept \"%s\"",
                 ok[i]);
    }
    /* empty input: success, no buffer required */
    fio_str_info_s empty = {0};
    FIO_ASSERT(!fio_string_write_base32dec(&empty, NULL, "", 0) && !empty.len,
               "base32 empty decode should succeed");
    FIO_ASSERT(!fio_string_write_base32dec(&empty, NULL, NULL, 0),
               "base32 NULL/0 decode should succeed");
    /* whitespace heavy input must not fail an exact capacity check */
    char spaced[256];
    size_t sl = 0;
    for (const char *c = "MZXW6YTBOI======"; *c; ++c) {
      for (size_t k = 0; k < 15; ++k)
        spaced[sl++] = ' ';
      spaced[sl++] = *c;
    }
    char db[7]; /* exactly "foobar" + NUL */
    fio_str_info_s dec = FIO_STR_INFO3(db, 0, sizeof(db));
    FIO_ASSERT(!fio_string_write_base32dec(&dec, NULL, spaced, sl) &&
                   dec.len == 6 && !FIO_MEMCMP(db, "foobar", 6) && !db[6],
               "base32 whitespace heavy decode should fit an exact buffer");
  }
  /* decoder: rejected forms leave `dest` unchanged */
  {
    static const char *bad[] = {"MZXW6!AAAA", "MZ!XW6", "!", "MZXW6=A",
                                "MY======MZXQ====", "M", "MZX", "MZXW6Y",
                                "MZXW6YTB1", "MZXW6\0AA"};
    static const size_t bad_len[] = {10, 6, 1, 7, 16, 1, 3, 6, 9, 8};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
      char db[32];
      FIO_MEMCPY(db, "keep", 5);
      fio_str_info_s dec = FIO_STR_INFO3(db, 4, sizeof(db));
      FIO_ASSERT(fio_string_write_base32dec(&dec, NULL, bad[i], bad_len[i]) ==
                         -1 &&
                     dec.len == 4 && dec.buf == db &&
                     !FIO_MEMCMP(db, "keep", 5),
                 "base32 decode should reject \"%s\" without changes",
                 bad[i]);
      /* also with a reallocating destination (no allocation on error) */
      fio_str_info_s dyn = {0};
      FIO_ASSERT(fio_string_write_base32dec(&dyn,
                                            FIO_STRING_ALLOC_COPY,
                                            bad[i],
                                            bad_len[i]) == -1 &&
                     !dyn.buf && !dyn.len,
                 "base32 decode should reject \"%s\" (no allocation)",
                 bad[i]);
    }
  }
}

/* =============================================================================
 * Test: URL encoding/decoding
 * ========================================================================== */
FIO_SFUNC void test_string_url(void) {
  /* Encode special chars */
  {
    char buf[256];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 256);
    fio_string_write_url_enc(&s, NULL, "hello world", 11);
    FIO_ASSERT(s.len > 11, "URL encoding should expand space");
  }

  /* Roundtrip */
  {
    char enc_buf[256], dec_buf[256];
    const char *original = "Hello World! @#$%^&*()";

    fio_str_info_s enc = FIO_STR_INFO3(enc_buf, 0, 256);
    fio_str_info_s dec = FIO_STR_INFO3(dec_buf, 0, 256);

    fio_string_write_url_enc(&enc, NULL, original, FIO_STRLEN(original));
    fio_string_write_url_dec(&dec, NULL, enc.buf, enc.len);

    FIO_ASSERT(dec.len == FIO_STRLEN(original),
               "URL roundtrip len: %zu vs %zu",
               dec.len,
               FIO_STRLEN(original));
    FIO_ASSERT(!FIO_MEMCMP(dec.buf, original, dec.len),
               "URL roundtrip content");
  }

  /* Plus to space */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_url_dec(&s, NULL, "hello+world", 11);
    FIO_ASSERT(!FIO_MEMCMP(buf, "hello world", 12), "plus to space");
  }

  /* Path decoding (plus stays plus) */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_path_dec(&s, NULL, "hello+world", 11);
    FIO_ASSERT(!FIO_MEMCMP(buf, "hello+world", 12), "path: plus stays plus");
  }
}

/* Boundaries at which the URL decoder's output-length estimate must include
 * every decoded or literal percent byte and leave room for the terminator. */
FIO_SFUNC void test_string_url_decode_boundaries(void) {
  static const struct {
    const char *input;
    const char *output;
  } cases[] = {
      {"%41", "A"}, {"%zz", "%zz"}, {"%", "%"},
      {"%u0041", "A"}, {"%u00E9", "\xC3\xA9"},
      {"%uD83D%uDE00", "\xF0\x9F\x98\x80"},
      {"a%41", "aA"}, {"a%zz", "a%zz"},
  };
  static const size_t tails[] = {
      0, 1, 15, 16, 1022, 1023, 1024, 2046, 2047, 2048, 4094, 4095, 4096};
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    const size_t prefix_len = strlen(cases[i].input);
    const size_t decoded_len = strlen(cases[i].output);
    for (size_t j = 0; j < sizeof(tails) / sizeof(*tails); ++j) {
      const size_t input_len = prefix_len + tails[j];
      char *input = (char *)malloc(input_len + 1);
      FIO_ASSERT(input, "URL boundary input allocation failed");
      FIO_MEMCPY(input, cases[i].input, prefix_len);
      FIO_MEMSET(input + prefix_len, 'z', tails[j]);
      input[input_len] = 0;
      fio_str_info_s decoded = {0};
      int r = fio_string_write_url_dec(
          &decoded, FIO_STRING_ALLOC_COPY, input, input_len);
      FIO_ASSERT(!r && decoded.buf && decoded.len == decoded_len + tails[j] &&
                     decoded.len < decoded.capa && !decoded.buf[decoded.len] &&
                     !FIO_MEMCMP(decoded.buf, cases[i].output, decoded_len) &&
                     !FIO_MEMCMP(decoded.buf + decoded_len,
                                 input + prefix_len,
                                 tails[j]),
                 "URL boundary case %zu/%zu (rc=%d, len=%zu, capacity=%zu)",
                 i, j, r, decoded.len, decoded.capa);
      FIO_STRING_FREE(decoded.buf);
      free(input);
    }
  }
}

/* Check decoder estimates at allocation boundaries, using owned buffers so
 * ASan can detect an off-by-one NUL write rather than stack spare capacity. */
FIO_SFUNC void test_string_other_decode_boundaries(void) {
  static const size_t sizes[] = {1, 2, 3, 15, 16, 17, 1023, 1024, 1025,
                                 2047, 2048, 2049, 4095, 4096};
  for (size_t i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
    const size_t n = sizes[i];
    char *input = (char *)malloc(n);
    FIO_ASSERT(input, "codec boundary input allocation failed");
    for (size_t k = 0; k < n; ++k)
      input[k] = (char)((k * 73U + 13U) & 255U);
    fio_str_info_s encoded = {0}, decoded = {0};
    int r = fio_string_write_base64enc(
        &encoded, FIO_STRING_ALLOC_COPY, input, n, 0);
    FIO_ASSERT(!r && encoded.buf && encoded.len < encoded.capa &&
                   !encoded.buf[encoded.len],
               "base64 encode boundary %zu", n);
    r = fio_string_write_base64dec(
        &decoded, FIO_STRING_ALLOC_COPY, encoded.buf, encoded.len);
    FIO_ASSERT(!r && decoded.buf && decoded.len == n &&
                   decoded.len < decoded.capa && !decoded.buf[decoded.len] &&
                   !FIO_MEMCMP(decoded.buf, input, n),
               "base64 decode boundary %zu (rc=%d len=%zu capa=%zu)",
               n, r, decoded.len, decoded.capa);
    FIO_STRING_FREE(encoded.buf);
    FIO_STRING_FREE(decoded.buf);
    free(input);
  }
  static const struct {
    const char *entity;
    const char *decoded;
  } entities[] = {{"&#65;", "A"}, {"&#x41;", "A"}, {"&amp;", "&"},
                  {"&lt;", "<"}, {"&nbsp;", "\xC2\xA0"}};
  static const size_t tails[] = {0, 1, 15, 16, 1023, 1024, 2047, 2048, 4095};
  for (size_t i = 0; i < sizeof(entities) / sizeof(*entities); ++i)
    for (size_t j = 0; j < sizeof(tails) / sizeof(*tails); ++j) {
      const size_t prefix = strlen(entities[i].entity);
      const size_t expected = strlen(entities[i].decoded);
      const size_t input_len = prefix + tails[j];
      char *input = (char *)malloc(input_len + 1);
      FIO_ASSERT(input, "HTML boundary input allocation failed");
      FIO_MEMCPY(input, entities[i].entity, prefix);
      FIO_MEMSET(input + prefix, 'z', tails[j]);
      input[input_len] = 0;
      fio_str_info_s out = {0};
      int r = fio_string_write_html_unescape(
          &out, FIO_STRING_ALLOC_COPY, input, input_len);
      FIO_ASSERT(!r && out.buf && out.len == expected + tails[j] &&
                     out.len < out.capa && !out.buf[out.len] &&
                     !FIO_MEMCMP(out.buf, entities[i].decoded, expected) &&
                     !FIO_MEMCMP(out.buf + expected, input + prefix, tails[j]),
                 "HTML decode boundary %zu/%zu (rc=%d len=%zu capa=%zu)",
                 i, j, r, out.len, out.capa);
      FIO_STRING_FREE(out.buf);
      free(input);
    }
}

/* =============================================================================
 * Test: HTML escaping/unescaping
 * ========================================================================== */
FIO_SFUNC void test_string_html(void) {
  /* Escape */
  {
    char buf[256];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 256);
    fio_string_write_html_escape(&s, NULL, "<script>alert('xss')</script>", 29);
    FIO_ASSERT(s.len > 29, "HTML escape should expand");
    FIO_ASSERT(!strstr(buf, "<script>"), "script tag should be escaped");
  }

  /* Unescape named entities */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_html_unescape(&s, NULL, "&lt;test&gt;", 12);
    FIO_ASSERT(!FIO_MEMCMP(buf, "<test>", 7),
               "named entities unescape: %s",
               buf);
  }

  /* Numeric entity */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_html_unescape(&s, NULL, "&#65;", 5); /* A */
    FIO_ASSERT(buf[0] == 'A', "numeric entity &#65; = A");
  }

  /* Hex entity */
  {
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_html_unescape(&s, NULL, "&#x41;", 6); /* A */
    FIO_ASSERT(buf[0] == 'A', "hex entity &#x41; = A");
  }
}

/* =============================================================================
 * Test: fio_bstr - binary string type
 * ========================================================================== */
FIO_SFUNC void test_bstr(void) {
  /* Basic write */
  {
    char *s = fio_bstr_write(NULL, "Hello", 5);
    FIO_ASSERT(s, "fio_bstr_write should return non-NULL");
    FIO_ASSERT(fio_bstr_len(s) == 5, "bstr len");
    FIO_ASSERT(!FIO_MEMCMP(s, "Hello", 6), "bstr content");
    fio_bstr_free(s);
  }

  /* Multiple writes */
  {
    char *s = fio_bstr_write(NULL, "Hello", 5);
    s = fio_bstr_write(s, " World", 6);
    FIO_ASSERT(fio_bstr_len(s) == 11, "bstr multiple writes len");
    FIO_ASSERT(!FIO_MEMCMP(s, "Hello World", 12),
               "bstr multiple writes content");
    fio_bstr_free(s);
  }

  /* fio_bstr_write2 */
  {
    char *s = fio_bstr_write2(NULL,
                              FIO_STRING_WRITE_STR1("Hello"),
                              FIO_STRING_WRITE_STR2(" ", 1),
                              FIO_STRING_WRITE_NUM(42));
    FIO_ASSERT(s, "bstr_write2 should return non-NULL");
    FIO_ASSERT(!FIO_MEMCMP(s, "Hello 42", 9), "bstr_write2 content: %s", s);
    fio_bstr_free(s);
  }

  /* fio_bstr_replace */
  {
    char *s = fio_bstr_write(NULL, "Hello World", 11);
    s = fio_bstr_replace(s, 6, 5, "Universe", 8);
    FIO_ASSERT(!FIO_MEMCMP(s, "Hello Universe", 15), "bstr_replace content");
    fio_bstr_free(s);
  }

  /* Copy-on-write */
  {
    char *s1 = fio_bstr_write(NULL, "Hello", 5);
    char *s2 = fio_bstr_copy(s1);
    FIO_ASSERT(s1 == s2, "copy should share pointer initially");
    s1 = fio_bstr_write(s1, "!", 1);
    FIO_ASSERT(s1 != s2, "write after copy should create new allocation");
    FIO_ASSERT(fio_bstr_len(s1) == 6, "s1 len after write");
    FIO_ASSERT(fio_bstr_len(s2) == 5, "s2 len unchanged");
    fio_bstr_free(s1);
    fio_bstr_free(s2);
  }

  /* Numerals */
  {
    char *s = fio_bstr_write_i(NULL, -42);
    FIO_ASSERT(!FIO_MEMCMP(s, "-42", 4), "bstr_write_i");
    fio_bstr_free(s);

    s = fio_bstr_write_u(NULL, 42);
    FIO_ASSERT(!FIO_MEMCMP(s, "42", 3), "bstr_write_u");
    fio_bstr_free(s);

    s = fio_bstr_write_hex(NULL, 0xDEAD);
    FIO_ASSERT(!FIO_MEMCMP(s, "DEAD", 5), "bstr_write_hex");
    fio_bstr_free(s);
  }

  /* Comparisons */
  {
    char *a = fio_bstr_write(NULL, "apple", 5);
    char *b = fio_bstr_write(NULL, "banana", 6);
    char *c = fio_bstr_write(NULL, "apple", 5);

    FIO_ASSERT(!fio_bstr_is_greater(a, b), "apple < banana");
    FIO_ASSERT(fio_bstr_is_greater(b, a), "banana > apple");
    FIO_ASSERT(fio_bstr_is_eq(a, c), "apple == apple");
    FIO_ASSERT(!fio_bstr_is_eq(a, b), "apple != banana");

    fio_bstr_free(a);
    fio_bstr_free(b);
    fio_bstr_free(c);
  }

  /* fio_bstr_info, fio_bstr_buf */
  {
    char *s = fio_bstr_write(NULL, "Test", 4);
    fio_str_info_s info = fio_bstr_info(s);
    fio_buf_info_s buf = fio_bstr_buf(s);

    FIO_ASSERT(info.len == 4, "bstr_info len");
    FIO_ASSERT(buf.len == 4, "bstr_buf len");
    FIO_ASSERT(info.buf == s, "bstr_info buf");
    FIO_ASSERT(buf.buf == s, "bstr_buf buf");

    fio_bstr_free(s);
  }

  /* NULL handling */
  {
    FIO_ASSERT(fio_bstr_len(NULL) == 0, "bstr_len(NULL) == 0");
    fio_bstr_free(NULL); /* should not crash */
  }
}

/* =============================================================================
 * Test: fio_keystr - key string type
 * ========================================================================== */
FIO_SFUNC void test_keystr(void) {
  /* Small string (embedded) - fio_keystr_s is ~16 bytes, can embed ~11 chars */
  {
    fio_keystr_s k = fio_keystr_tmp("Hi", 2);
    fio_buf_info_s b = fio_keystr_buf(&k);
    FIO_ASSERT(b.len == 2, "keystr tmp len");
    FIO_ASSERT(!FIO_MEMCMP(b.buf, "Hi", 2), "keystr tmp content");
  }

  /* Medium string (still embeddable) */
  {
    const char *str = "Hello123"; /* 8 chars - should embed */
    fio_keystr_s k = fio_keystr_tmp(str, (uint32_t)FIO_STRLEN(str));
    fio_buf_info_s b = fio_keystr_buf(&k);
    FIO_ASSERT(b.len == FIO_STRLEN(str),
               "keystr medium len: got %zu, expected %zu",
               b.len,
               FIO_STRLEN(str));
  }

  /* Equality */
  {
    fio_keystr_s a = fio_keystr_tmp("test", 4);
    fio_keystr_s b = fio_keystr_tmp("test", 4);
    fio_keystr_s c = fio_keystr_tmp("Test", 4);

    FIO_ASSERT(fio_keystr_is_eq(a, b), "keystr equal");
    FIO_ASSERT(!fio_keystr_is_eq(a, c), "keystr not equal (case)");
  }

  /* Hash */
  {
    fio_keystr_s a = fio_keystr_tmp("test", 4);
    fio_keystr_s b = fio_keystr_tmp("test", 4);
    FIO_ASSERT(fio_keystr_hash(a) == fio_keystr_hash(b),
               "equal strings same hash");
  }
}

/* =============================================================================
 * Test: String comparison
 * ========================================================================== */
FIO_SFUNC void test_string_comparison(void) {
  /* Equal strings */
  {
    fio_buf_info_s a = FIO_BUF_INFO1("hello");
    fio_buf_info_s b = FIO_BUF_INFO1("hello");
    FIO_ASSERT(!fio_string_is_greater_buf(a, b), "equal strings: a not > b");
    FIO_ASSERT(!fio_string_is_greater_buf(b, a), "equal strings: b not > a");
  }

  /* Different lengths */
  {
    fio_buf_info_s a = FIO_BUF_INFO1("hello");
    fio_buf_info_s b = FIO_BUF_INFO1("hell");
    FIO_ASSERT(fio_string_is_greater_buf(a, b), "longer string is greater");
    FIO_ASSERT(!fio_string_is_greater_buf(b, a),
               "shorter string is not greater");
  }

  /* Different content */
  {
    fio_buf_info_s a = FIO_BUF_INFO1("b");
    fio_buf_info_s b = FIO_BUF_INFO1("a");
    FIO_ASSERT(fio_string_is_greater_buf(a, b), "b > a");
    FIO_ASSERT(!fio_string_is_greater_buf(b, a), "a not > b");
  }

  /* Empty strings */
  {
    fio_buf_info_s a = FIO_BUF_INFO1("a");
    fio_buf_info_s b = FIO_BUF_INFO2("", 0);
    FIO_ASSERT(fio_string_is_greater_buf(a, b), "non-empty > empty");
    FIO_ASSERT(!fio_string_is_greater_buf(b, a), "empty not > non-empty");
  }
}

/* =============================================================================
 * Test: Memory helpers
 * ========================================================================== */
FIO_SFUNC void test_string_memory(void) {
  /* fio_string_capa4len - 16-byte alignment */
  {
    FIO_ASSERT(fio_string_capa4len(0) >= 1, "capa4len(0) >= 1");
    FIO_ASSERT(fio_string_capa4len(1) >= 2, "capa4len(1) >= 2");
    FIO_ASSERT((fio_string_capa4len(15) & 15) == 0, "capa4len aligned");
    FIO_ASSERT((fio_string_capa4len(100) & 15) == 0, "capa4len(100) aligned");
  }
}

/* =============================================================================
 * Test: File operations
 * ========================================================================== */
FIO_SFUNC void test_string_files(void) {
  /* Read this file */
  {
    char *s = fio_bstr_readfile(NULL, __FILE__, 0, 0);
    FIO_ASSERT(s && fio_bstr_len(s) > 0,
               "fio_bstr_readfile should read this file");
    FIO_ASSERT(strstr(s, "test_string_files"), "should contain function name");
    fio_bstr_free(s);
  }

  /* Read with limit */
  {
    char *s = fio_bstr_readfile(NULL, __FILE__, 0, 100);
    FIO_ASSERT(s, "readfile with limit");
    FIO_ASSERT(fio_bstr_len(s) <= 100, "readfile respects limit");
    fio_bstr_free(s);
  }

  /* Read with offset */
  {
    char *s = fio_bstr_readfile(NULL, __FILE__, 10, 50);
    FIO_ASSERT(s, "readfile with offset");
    fio_bstr_free(s);
  }

  /* fio_bstr_getdelim_file */
  {
    char *s = fio_bstr_getdelim_file(NULL, __FILE__, 0, '\n', 0);
    FIO_ASSERT(s, "getdelim_file");
    FIO_ASSERT(s[fio_bstr_len(s) - 1] == '\n' || fio_bstr_len(s) > 0,
               "should read to newline");
    fio_bstr_free(s);
  }
}

/* =============================================================================
 * Test: archived exhaustive roundtrips
 * ========================================================================== */
FIO_SFUNC void test_archive_roundtrips(void) {
  { /* C / JSON style escaping over UTF-8 plus byte range 1..255. */
    char mem[2048];
    fio_str_info_s unescaped = FIO_STR_INFO3(mem, 0, 512);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 512, 0, 512);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 1024, 0, 1024);
    const char *utf8_sample =
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95\xc6\x92Z";
    FIO_ASSERT(!fio_string_write(&unescaped,
                                 NULL,
                                 utf8_sample,
                                 FIO_STRLEN(utf8_sample)),
               "couldn't write UTF-8 example");
    for (size_t i = 1; i < 256; ++i) {
      uint8_t c = (uint8_t)i;
      FIO_ASSERT(!fio_string_write(&unescaped, NULL, &c, 1),
                 "write returned an error");
    }
    FIO_ASSERT(
        !fio_string_write_escape(&encoded, NULL, unescaped.buf, unescaped.len),
        "write escape returned an error");
    FIO_ASSERT(
        !fio_string_write_unescape(&decoded, NULL, encoded.buf, encoded.len),
        "write unescape returned an error");
    FIO_ASSERT(unescaped.len == decoded.len,
               "escape roundtrip length error: %zu != %zu",
               unescaped.len,
               decoded.len);
    FIO_ASSERT(!FIO_MEMCMP(unescaped.buf, decoded.buf, unescaped.len),
               "escape roundtrip content mismatch");
  }

  { /* Base64 and Base32 over a mixed printable/binary payload. */
    char mem[2048];
    fio_str_info_s original = FIO_STR_INFO3(mem, 0, 512);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 512, 0, 512);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 1024, 0, 512);
    fio_string_write(&original,
                     NULL,
                     "Hello World, this is the voice of peace:)",
                     41);
    for (size_t i = 0; i < 256; ++i) {
      uint8_t c = (uint8_t)i;
      FIO_ASSERT(!fio_string_write(&original, NULL, &c, 1),
                 "write returned an error");
    }

    FIO_ASSERT(!fio_string_write_base64enc(&encoded,
                                           NULL,
                                           original.buf,
                                           original.len,
                                           1),
               "base64 encode returned an error");
    FIO_ASSERT(
        !fio_string_write_base64dec(&decoded, NULL, encoded.buf, encoded.len),
        "base64 decode returned an error");
    FIO_ASSERT(FIO_STR_INFO_IS_EQ(original, decoded),
               "base64 exhaustive roundtrip failed");

    encoded.len = decoded.len = 0;
    FIO_ASSERT(
        !fio_string_write_base32enc(&encoded, NULL, original.buf, original.len),
        "base32 encode returned an error");
    FIO_ASSERT(
        !fio_string_write_base32dec(&decoded, NULL, encoded.buf, encoded.len),
        "base32 decode returned an error");
    FIO_ASSERT(FIO_STR_INFO_IS_EQ(original, decoded),
               "base32 exhaustive roundtrip failed");
  }

  { /* URL encode/decode every byte value. */
    char mem[2048];
    for (size_t i = 0; i < 256; ++i)
      mem[i] = (char)i;
    fio_str_info_s original = FIO_STR_INFO3(mem, 256, 256);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 256, 0, 1024);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 1280, 0, 512);
    FIO_ASSERT(!fio_string_write_url_enc(&encoded, NULL, mem, 256),
               "url encode returned an error");
    FIO_ASSERT(
        !fio_string_write_url_dec(&decoded, NULL, encoded.buf, encoded.len),
        "url decode returned an error");
    FIO_ASSERT(FIO_STR_INFO_IS_EQ(original, decoded),
               "url exhaustive roundtrip failed");
  }

  { /* HTML escaping and named/numeric entity unescaping coverage. */
    char mem[3072];
    fio_str_info_s original = FIO_STR_INFO3(mem, 127, 256);
    fio_str_info_s escaped = FIO_STR_INFO3(mem + 256, 0, 2048);
    fio_str_info_s unescaped = FIO_STR_INFO3(mem + 2560, 0, 512);
    for (size_t i = 0; i < 127; ++i)
      mem[i] = (char)i;
    FIO_ASSERT(!fio_string_write_html_escape(&escaped,
                                             NULL,
                                             original.buf,
                                             original.len),
               "html escape returned an error");
    FIO_ASSERT(!fio_string_write_html_unescape(&unescaped,
                                               NULL,
                                               escaped.buf,
                                               escaped.len),
               "html unescape returned an error");
    FIO_ASSERT(FIO_STR_INFO_IS_EQ(original, unescaped),
               "html byte-range roundtrip failed");

    original.len = 0;
    fio_string_write(&original, NULL, "ÿ", FIO_STRLEN("ÿ"));
    original.buf[original.len++] = (char)0xE2;
    original.buf[original.len++] = (char)0x82;
    original.buf[original.len++] = (char)0xAC;
    original.buf[original.len++] = (char)0xC2;
    original.buf[original.len++] = (char)0xA3;
    original.buf[original.len++] = (char)0xC2;
    original.buf[original.len++] = (char)0xA2;
    original.buf[original.len++] = (char)0xC2;
    original.buf[original.len++] = (char)0xA9;
    original.buf[original.len++] = (char)0xC2;
    original.buf[original.len++] = (char)0xAE;
    original.buf[original.len++] = (char)0xC2;
    original.buf[original.len++] = (char)0xA0;
    original.buf[original.len++] = (char)0x26;
    original.buf[original.len++] = (char)0x27;
    original.buf[original.len++] = (char)0x22;
    original.buf[original.len] = 0;
    escaped.len = unescaped.len = 0;
    fio_string_write(&escaped,
                     NULL,
                     "&#255;&eUro;&pound;&cenT&Copy;&reg&nbsp;&amp;&apos;&quot",
                     56);
    FIO_ASSERT(!fio_string_write_html_unescape(&unescaped,
                                               NULL,
                                               escaped.buf,
                                               escaped.len),
               "html named entity unescape returned an error");
    FIO_ASSERT(FIO_STR_INFO_IS_EQ(original, unescaped),
               "html named entity decoding failed");
  }
}

/* =============================================================================
 * Main
 * ========================================================================== */
/* =============================================================================
 * Test: fio_string2primitive
 * ========================================================================== */
FIO_SFUNC void test_string2primitive(void) {
  fprintf(stderr, "* Testing fio_string2primitive\n");
  struct {
    const char *s;
    fio_string_primitive_e type;
    uint8_t base;
    int64_t i;
    double f;
  } t[] = {
      {"", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"hello", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"null", FIO_STRING_PRIMITIVE_NULL, 0, 0, 0},
      {"NuLL", FIO_STRING_PRIMITIVE_NULL, 0, 0, 0},
      {"nIL", FIO_STRING_PRIMITIVE_NULL, 0, 0, 0},
      {"TRUE", FIO_STRING_PRIMITIVE_TRUE, 0, 0, 0},
      {"fAlSe", FIO_STRING_PRIMITIVE_FALSE, 0, 0, 0},
      {"nulls", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"fals", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"tru", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"42", FIO_STRING_PRIMITIVE_NUMBER, 10, 42, 0},
      {"-42", FIO_STRING_PRIMITIVE_NUMBER, 10, -42, 0},
      {"+5", FIO_STRING_PRIMITIVE_NUMBER, 10, 5, 0},
      {" 7", FIO_STRING_PRIMITIVE_NUMBER, 10, 7, 0},
      {"0", FIO_STRING_PRIMITIVE_NUMBER, 10, 0, 0},
      {"1_000", FIO_STRING_PRIMITIVE_NUMBER, 10, 1000, 0},
      {"0x1F", FIO_STRING_PRIMITIVE_NUMBER, 16, 31, 0},
      {"-0X10", FIO_STRING_PRIMITIVE_NUMBER, 16, -16, 0},
      {"x1F", FIO_STRING_PRIMITIVE_NUMBER, 16, 31, 0},
      {"0b101", FIO_STRING_PRIMITIVE_NUMBER, 2, 5, 0},
      {"b101", FIO_STRING_PRIMITIVE_NUMBER, 2, 5, 0},
      {"0123", FIO_STRING_PRIMITIVE_NUMBER, 8, 83, 0},
      {"007", FIO_STRING_PRIMITIVE_NUMBER, 8, 7, 0},
      {"1.5", FIO_STRING_PRIMITIVE_FLOAT, 10, 0, 1.5},
      {"-1.5e3", FIO_STRING_PRIMITIVE_FLOAT, 10, 0, -1500},
      {".5", FIO_STRING_PRIMITIVE_FLOAT, 10, 0, 0.5},
      {"012.5", FIO_STRING_PRIMITIVE_FLOAT, 10, 0, 12.5},
      {"0x1.8p1", FIO_STRING_PRIMITIVE_FLOAT, 16, 0, 3},
      {"12abc", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"1e", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"08", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"7 ", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
      {"-", FIO_STRING_PRIMITIVE_STRING, 0, 0, 0},
  };
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    fio_string_primitive_s r =
        fio_string2primitive(FIO_STR_INFO1((char *)t[i].s));
    FIO_ASSERT(r.type == t[i].type && r.base == t[i].base,
               "fio_string2primitive(\"%s\") type %d base %d, expected %d / %d",
               t[i].s,
               (int)r.type,
               (int)r.base,
               (int)t[i].type,
               (int)t[i].base);
    FIO_ASSERT(r.type != FIO_STRING_PRIMITIVE_NUMBER || r.i == t[i].i,
               "fio_string2primitive(\"%s\") = %lld",
               t[i].s,
               (long long)r.i);
    FIO_ASSERT(r.type != FIO_STRING_PRIMITIVE_FLOAT || r.f == t[i].f,
               "fio_string2primitive(\"%s\") = %g",
               t[i].s,
               r.f);
  }
  /* special floats and length-delimited input (NUL guard inside buffer) */
  fio_string_primitive_s r = fio_string2primitive(FIO_STR_INFO1((char *)"inf"));
  FIO_ASSERT(r.type == FIO_STRING_PRIMITIVE_FLOAT && isinf(r.f) && r.base == 10,
             "fio_string2primitive(inf)");
  r = fio_string2primitive(FIO_STR_INFO1((char *)"-nan"));
  FIO_ASSERT(r.type == FIO_STRING_PRIMITIVE_FLOAT && isnan(r.f),
             "fio_string2primitive(-nan)");
  r = fio_string2primitive(FIO_STR_INFO2((char *)"12\0" "34", 3));
  FIO_ASSERT(r.type == FIO_STRING_PRIMITIVE_STRING,
             "embedded NUL should not be a number");
  r = fio_string2primitive(FIO_STR_INFO2(NULL, 0));
  FIO_ASSERT(r.type == FIO_STRING_PRIMITIVE_STRING, "NULL should be a String");
}

/* =============================================================================
 * Test: string core security regressions (bounds / truncation / COW)
 * ========================================================================== */
FIO_SFUNC char *test___exact_copy(const char *s, size_t len) {
  char *p = (char *)malloc(len ? len : 1); /* exact size: ASan catches reads */
  FIO_ASSERT(p, "allocation failed");
  FIO_MEMCPY(p, s, len);
  return p;
}

FIO_SFUNC void test_string_security_regressions(void) {
  { /* write2 float: fixed tiny buffer must truncate, not overflow */
    char out[4];
    fio_str_info_s s = FIO_STR_INFO3(out, 0, 4);
    int r = fio_string_write2(&s, NULL, FIO_STRING_WRITE_FLOAT(-1.0e308));
    FIO_ASSERT(r == -1 && s.len < 4 && !out[s.len], "write2 float truncate");
  }
  { /* write2 float: length matches the stored text (no stale bytes) */
    char out[64];
    FIO_MEMSET(out, 'X', sizeof(out));
    fio_str_info_s s = FIO_STR_INFO3(out, 0, 64);
    fio_string_write2(&s, NULL, FIO_STRING_WRITE_FLOAT(-1.79769313486231e308));
    char expect[64];
    const size_t elen = fio_ftoa(expect, -1.79769313486231e308, 10);
    FIO_ASSERT(s.len == strlen(out) && s.len == elen &&
                   !FIO_MEMCMP(out, expect, elen + 1),
               "write2 float length %zu != %zu (%s)",
               s.len,
               strlen(out),
               out);
  }
  { /* escape: tiny fixed buffer truncates on whole escape sequences */
    char *out = (char *)malloc(3);
    fio_str_info_s s = FIO_STR_INFO3(out, 0, 3);
    int r = fio_string_write_escape(&s, NULL, "\n\n", 2);
    FIO_ASSERT(r == -1 && s.len == 2 && !FIO_MEMCMP(out, "\\n", 3),
               "escape truncation");
    free(out);
    char buf[8];
    s = FIO_STR_INFO3(buf, 0, 8);
    r = fio_string_write_escape(&s, NULL, "ab\x01"
                                          "cd",
                                5);
    FIO_ASSERT(r == -1 && s.len == 2 && !FIO_MEMCMP(buf, "ab", 3),
               "escape truncation before u0001 (%zu)",
               s.len);
  }
  { /* escape: overlong / surrogate UTF-8 is escaped, valid UTF-8 kept */
    char buf[64];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 64);
    fio_string_write_escape(&s, NULL, "\xC0\xAF\xED\xA0\x80\xC3\xA9", 7);
    FIO_ASSERT(s.len == 4 * 5 + 2 && !FIO_MEMCMP(buf + 20, "\xC3\xA9", 2),
               "escape strict UTF-8 (%zu: %s)",
               s.len,
               buf);
  }
  { /* shared (COW) string shrink must not overflow the detached copy */
    char *a = fio_bstr_write(NULL, "Hello", 5);
    char *b = fio_bstr_copy(a);
    b = fio_bstr_len_set(b, 0);
    FIO_ASSERT(a != b && fio_bstr_len(a) == 5 && !fio_bstr_len(b) && !b[0],
               "COW shrink");
    fio_bstr_free(b);
    b = fio_bstr_copy(a);
    b = fio_bstr_len_set(b, 2);
    FIO_ASSERT(fio_bstr_len(b) == 2 && !FIO_MEMCMP(b, "He", 3) &&
                   !FIO_MEMCMP(a, "Hello", 6),
               "COW partial shrink");
    fio_bstr_free(a);
    fio_bstr_free(b);
  }
  { /* replace: huge overwrite length must not wrap */
    char buf[16] = "abc";
    fio_str_info_s s = FIO_STR_INFO3(buf, 3, 16);
    fio_string_replace(&s, NULL, 0, SIZE_MAX, NULL, 0);
    FIO_ASSERT(s.len == 0 && !buf[0], "replace SIZE_MAX overwrite");
    s = FIO_STR_INFO3(buf, 0, 16);
    fio_string_write(&s, NULL, "abcdef", 6);
    fio_string_replace(&s, NULL, 2, SIZE_MAX - 1, "Z", 1);
    FIO_ASSERT(s.len == 3 && !FIO_MEMCMP(buf, "abZ", 4), "replace wrap");
  }
  { /* NULL destination without allocator: no NUL write through NULL */
    fio_str_info_s s = {0};
    FIO_ASSERT(fio_string_write(&s, NULL, "abc", 3) == -1 && !s.buf &&
                   !s.len,
               "write to NULL destination");
    FIO_ASSERT(fio_string_write_unescape(&s, NULL, "a\\nb", 4) == -1 && !s.buf,
               "unescape to NULL destination");
  }
  { /* truncated C / JSON escapes never read past the input */
    static const char *const vec[] = {"\\x", "\\x4", "\\0", "\\u12", "\\"};
    static const char *const expect[] = {"x", "x4", "0", "u12", "\\"};
    for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); ++i) {
      char out[16];
      char *in = test___exact_copy(vec[i], strlen(vec[i]));
      fio_str_info_s s = FIO_STR_INFO3(out, 0, 16);
      fio_string_write_unescape(&s, NULL, in, strlen(vec[i]));
      FIO_ASSERT(s.len == strlen(expect[i]) && !strcmp(out, expect[i]),
                 "truncated unescape %zu: %s",
                 i,
                 out);
      free(in);
    }
    char tmp[200]; /* long input takes the sizing path */
    FIO_MEMSET(tmp, 'a', sizeof(tmp));
    tmp[199] = '\\';
    char *in = test___exact_copy(tmp, sizeof(tmp));
    char *b = fio_bstr_write_unescape(NULL, in, sizeof(tmp));
    FIO_ASSERT(fio_bstr_len(b) == 200 && b[199] == '\\', "terminal backslash");
    fio_bstr_free(b);
    free(in);
  }
  { /* UTF-16 surrogates: full high range pairs, lone halves => U+FFFD */
    char out[32];
    fio_str_info_s s = FIO_STR_INFO3(out, 0, 32);
    fio_string_write_unescape(&s, NULL, "\\uDBFF\\uDFFF", 12);
    FIO_ASSERT(s.len == 4 && !FIO_MEMCMP(out, "\xF4\x8F\xBF\xBF", 4),
               "unescape surrogate pair U+10FFFF");
    s = FIO_STR_INFO3(out, 0, 32);
    fio_string_write_unescape(&s, NULL, "\\uD800\\u0041", 12);
    FIO_ASSERT(s.len == 4 && !FIO_MEMCMP(out,
                                         "\xEF\xBF\xBD"
                                         "A",
                                         4),
               "unescape lone surrogate");
    s = FIO_STR_INFO3(out, 0, 32);
    fio_string_write_url_dec(&s, NULL, "%uDBFF%uDFFF", 12);
    FIO_ASSERT(s.len == 4 && !FIO_MEMCMP(out, "\xF4\x8F\xBF\xBF", 4),
               "url decode surrogate pair U+10FFFF");
    char *in = test___exact_copy("%uD800%uDC0", 11);
    s = FIO_STR_INFO3(out, 0, 32);
    fio_string_write_url_dec(&s, NULL, in, 11);
    FIO_ASSERT(s.len == 8 && !FIO_MEMCMP(out, "\xEF\xBF\xBD%uDC0", 8),
               "url decode truncated pair");
    free(in);
  }
  { /* HTML unescape: truncated entities stay in bounds */
    static const char *const vec[] = {"&", "&a", "&#", "&#12", "&#x", "&l"};
    for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); ++i) {
      char out[16];
      const size_t l = strlen(vec[i]);
      char *in = test___exact_copy(vec[i], l);
      fio_str_info_s s = FIO_STR_INFO3(out, 0, 16);
      fio_string_write_html_unescape(&s, NULL, in, l);
      FIO_ASSERT(s.len == l && !FIO_MEMCMP(out, vec[i], l + 1),
                 "truncated HTML entity %zu",
                 i);
      free(in);
    }
    char *in = test___exact_copy("&lt", 3);
    char out[32];
    fio_str_info_s s = FIO_STR_INFO3(out, 0, 32);
    fio_string_write_html_unescape(&s, NULL, in, 3);
    FIO_ASSERT(s.len == 1 && out[0] == '<', "HTML entity at end of input");
    free(in);
    /* numeric entities without ';' are copied, never under-allocated */
    char tmp[4095];
    for (size_t i = 0; i + 5 <= sizeof(tmp); i += 5)
      FIO_MEMCPY(tmp + i, "&#60 ", 5);
    char *b = fio_bstr_write_html_unescape(NULL, tmp, sizeof(tmp));
    FIO_ASSERT(fio_bstr_len(b) == sizeof(tmp) && !FIO_MEMCMP(b, tmp, 4095),
               "HTML numeric without ';'");
    fio_bstr_free(b);
    s = FIO_STR_INFO3(out, 0, 32);
    fio_string_write_html_unescape(&s, NULL, "&#x1F600;&#xD800;", 17);
    FIO_ASSERT(s.len == 12 && !FIO_MEMCMP(out, "\xF0\x9F\x98\x80&#xD800;", 12),
               "HTML numeric range");
  }
  { /* utf8_select: INTPTR_MIN must not overflow */
    intptr_t pos = INTPTR_MIN;
    size_t len = 1;
    fio_string_utf8_select(FIO_STR_INFO2((char *)"abc", 3), &pos, &len);
    FIO_ASSERT(pos == 0 && len == 1, "utf8_select INTPTR_MIN");
  }
}

int main(void) {
  test_str_info_macros();
  test_string2primitive();
  test_string_write();
  test_string_replace();
  test_string_write2();
  test_string_write2_float_overflow();
  test_string_numerals();
  test_string_printf();
  test_string_utf8();
  test_string_escape();
  test_string_base64();
  test_string_base64_short_input();
  test_string_base32();
  test_string_url();
  test_string_url_decode_boundaries();
  test_string_other_decode_boundaries();
  test_string_html();
  test_bstr();
  test_keystr();
  test_string_comparison();
  test_string_memory();
  test_string_files();
  test_archive_roundtrips();
  test_string_security_regressions();
  return 0;
}
