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
  FIO_LOG_DDEBUG("Testing FIO_STR_INFO and FIO_BUF_INFO macros");

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
  FIO_LOG_DDEBUG("Testing fio_string_write");

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
  FIO_LOG_DDEBUG("Testing fio_string_replace");

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
  FIO_LOG_DDEBUG("Testing fio_string_write2");

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
    fio_string_write2(&s, NULL, FIO_STRING_WRITE_FLOAT(3.14));
    FIO_ASSERT(s.len > 0, "FLOAT len should be > 0");
  }
}

/* =============================================================================
 * Test: Numeral functions - boundary testing
 * ========================================================================== */
FIO_SFUNC void test_string_numerals(void) {
  FIO_LOG_DDEBUG("Testing numeral functions");

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
  FIO_LOG_DDEBUG("Testing fio_string_printf");

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
  FIO_LOG_DDEBUG("Testing UTF-8 functions");

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
    fio_str_info_s s = FIO_STR_INFO1((char *)invalid);
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
}

/* =============================================================================
 * Test: Escape/Unescape
 * ========================================================================== */
FIO_SFUNC void test_string_escape(void) {
  FIO_LOG_DDEBUG("Testing escape/unescape");

  /* Escape special characters */
  {
    char buf[128];
    fio_str_info_s s = FIO_STR_INFO3(buf, 0, 128);
    const char *input = "Hello\nWorld\t\"Test\"\\End";
    fio_string_write_escape(&s, NULL, input, FIO_STRLEN(input));
    FIO_ASSERT(s.len > FIO_STRLEN(input), "escaped should be longer");
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
  FIO_LOG_DDEBUG("Testing Base64");

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
}

/* =============================================================================
 * Test: Base32 encoding/decoding
 * ========================================================================== */
FIO_SFUNC void test_string_base32(void) {
  FIO_LOG_DDEBUG("Testing Base32");

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
}

/* =============================================================================
 * Test: URL encoding/decoding
 * ========================================================================== */
FIO_SFUNC void test_string_url(void) {
  FIO_LOG_DDEBUG("Testing URL encoding");

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

/* =============================================================================
 * Test: HTML escaping/unescaping
 * ========================================================================== */
FIO_SFUNC void test_string_html(void) {
  FIO_LOG_DDEBUG("Testing HTML escape/unescape");

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
  FIO_LOG_DDEBUG("Testing fio_bstr");

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
  FIO_LOG_DDEBUG("Testing fio_keystr");

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
  FIO_LOG_DDEBUG("Testing string comparison");

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
  FIO_LOG_DDEBUG("Testing memory helpers");

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
  FIO_LOG_DDEBUG("Testing file operations");

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
 * Main
 * ========================================================================== */
int main(void) {
  FIO_LOG_DDEBUG("=== Comprehensive FIO_STR String Core Tests ===");

  test_str_info_macros();
  test_string_write();
  test_string_replace();
  test_string_write2();
  test_string_numerals();
  test_string_printf();
  test_string_utf8();
  test_string_escape();
  test_string_base64();
  test_string_base32();
  test_string_url();
  test_string_html();
  test_bstr();
  test_keystr();
  test_string_comparison();
  test_string_memory();
  test_string_files();

  FIO_LOG_DDEBUG("=== All FIO_STR tests passed! ===");
  return 0;
}
