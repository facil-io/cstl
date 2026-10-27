/* *****************************************************************************
ML Entity Decoding Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_ENTITY
#include FIO_INCLUDE_FILE

#include <stdlib.h>
#include <string.h>

/* Exact-size input allocations make every logical-end overread visible to
 * ASan. The decoder gets exactly eight writable destination bytes. */
static void expect_window(const char *entity,
                          size_t len,
                          const char *expected,
                          size_t expected_len) {
  char *exact = NULL;
  if (entity && len) {
    exact = malloc(len);
    FIO_ASSERT(exact, "input allocation failed");
    FIO_MEMCPY(exact, entity, len);
  }
  unsigned char guarded[10];
  FIO_MEMSET(guarded, 0xA5, sizeof(guarded));
  char *buf = (char *)guarded + 1;
  size_t written = fio_entity(buf, exact, len);
  FIO_ASSERT(written == expected_len,
             "entity window (%zu bytes): expected %zu, got %zu",
             len,
             expected_len,
             written);
  FIO_ASSERT(guarded[0] == 0xA5 && guarded[9] == 0xA5,
             "entity destination canaries changed");
  if (written) {
    FIO_ASSERT(!FIO_MEMCMP(buf, expected, written), "decoded bytes mismatch");
    FIO_ASSERT(buf[written] == '\0', "missing output NUL");
    for (size_t pos = 0; pos < written;) {
      unsigned n = fio_utf8_char_len_bounded(buf + pos, written - pos);
      FIO_ASSERT(n, "entity output is not valid scalar UTF-8");
      pos += n;
    }
  }
  for (size_t i = written ? written + 1 : 0; i < 8; ++i)
    FIO_ASSERT((unsigned char)buf[i] == 0xA5,
               "entity wrote beyond output or modified rejected output");
  free(exact);
}

static void expect_entity(const char *entity,
                          const char *expected,
                          size_t expected_len) {
  expect_window(entity, FIO_STRLEN(entity), expected, expected_len);
}

static void expect_invalid(const char *entity, size_t len) {
  expect_window(entity, len, NULL, 0);
}

static void test_named_entities(void) {
  fprintf(stderr, "* Testing named ML entities...\n");

  expect_entity("&lt;", "<", 1);
  expect_entity("&GT;", ">", 1);
  expect_entity("&Amp;", "&", 1);
  expect_entity("&apos;", "'", 1);
  expect_entity("&quot;", "\"", 1);
  expect_entity("&nbsp;", "\xC2\xA0", 2);
  expect_entity("&copy;", "\xC2\xA9", 2);
  expect_entity("&euro;", "\xE2\x82\xAC", 3);
  expect_entity("&fjlig;", "fj", 2);
  expect_entity("&HilbertSpace;", "\xE2\x84\x8B", 3);
  expect_entity("&ClockwiseContourIntegral;", "\xE2\x88\xB2", 3);
  expect_entity("&nge;", "\xE2\x89\xA7\xCC\xB8", 5);
  expect_entity("&frac14;", "\xC2\xBC", 2);
  expect_entity("&FRAC12;", "\xC2\xBD", 2);
  expect_entity("&FrAc34;", "\xC2\xBE", 2);
}

static void test_numeric_entities(void) {
  fprintf(stderr, "* Testing numeric ML entities...\n");

  expect_entity("&#65;", "A", 1);
  expect_entity("&#x41;", "A", 1);
  expect_entity("&#X41;", "A", 1);
  expect_entity("&#169;", "\xC2\xA9", 2);
  expect_entity("&#x20AC;", "\xE2\x82\xAC", 3);
  expect_entity("&#x1F600;", "\xF0\x9F\x98\x80", 4);
  expect_entity("&#0;", "\xEF\xBF\xBD", 3);
  expect_entity("&#1114111;", "\xF4\x8F\xBF\xBF", 4);
  expect_entity("&#55295;", "\xED\x9F\xBF", 3);
  expect_entity("&#55296;", "\xEF\xBF\xBD", 3);
  expect_entity("&#57343;", "\xEF\xBF\xBD", 3);
  expect_entity("&#57344;", "\xEE\x80\x80", 3);
  expect_entity("&#xD800;", "\xEF\xBF\xBD", 3);
  expect_entity("&#xDFFF;", "\xEF\xBF\xBD", 3);
  expect_entity("&#x10FFFF;", "\xF4\x8F\xBF\xBF", 4);
  expect_entity("&#00000000000000000000000000000065;", "A", 1);
  expect_entity("&#x000000000000000000000000000041;", "A", 1);
}

static void test_invalid_entities(void) {
  fprintf(stderr, "* Testing invalid ML entities...\n");

  expect_invalid(NULL, 0);
  expect_invalid("", 0);
  expect_invalid("&", 1);
  expect_invalid("&;", 2);
  expect_invalid("lt;", 3);
  expect_invalid("&lt", 3);
  expect_invalid("&unknown;", 9);
  expect_invalid("&#;", 3);
  expect_invalid("&#x;", 4);
  expect_invalid("&#xZZ;", 6);
  expect_invalid("&#65", 4);
  expect_invalid("&#1114112;", 10);
  expect_invalid("&ClockwiseContourIntegral;", 8);
}

static void test_bounded_grammar(void) {
  fprintf(stderr, "* Testing bounded entity grammar...\n");
  static const char *invalid[] = {"&#1",
                                  "&#x",
                                  "&#X",
                                  "&#x0",
                                  "&#x0x",
                                  "&#12",
                                  "&#x41",
                                  "&#1_",
                                  "&#6_0;",
                                  "&#60_;",
                                  "&#x3_c;",
                                  "&#x3c_;",
                                  "&#x0x3c;",
                                  "&#x0X3C;",
                                  "&#+60;",
                                  "&#-60;",
                                  "&# 60;",
                                  "&#x+3c;",
                                  "&#x 3c;",
                                  "&#18446744073709551615;",
                                  "&#18446744073709551616;",
                                  "&#18446744073709551681;",
                                  "&#xFFFFFFFFFFFFFFFF;",
                                  "&#x10000000000000041;",
                                  "&#x110000;",
                                  "&frac\x11\x14;",
                                  "&frac\x11\x12;",
                                  "&frac\x13\x14;",
                                  "&frac1;",
                                  "&abcdefghijklmnopqrstuvwxyzabcdef;",
                                  "&abcdefghijklmnopqrstuvwxyzabcdefg;"};
  for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i)
    expect_invalid(invalid[i], FIO_STRLEN(invalid[i]));

  /* Even a readable suffix must not be used outside the declared window. */
  char out[8];
  FIO_MEMSET(out, 0xA5, sizeof(out));
  FIO_ASSERT(!fio_entity(out, "&#65;", 4), "read past logical decimal end");
  FIO_ASSERT(!fio_entity(out, "&#x41;", 5), "read past logical hex end");
  FIO_ASSERT(!fio_entity(out, "&amp;", 4), "read past logical named end");
  for (size_t i = 0; i < sizeof(out); ++i)
    FIO_ASSERT((unsigned char)out[i] == 0xA5, "truncated input wrote output");
  FIO_ASSERT(!fio_entity(NULL, "&amp;", 5), "NULL destination accepted");

  static const char *valid[] = {"&#1114111;",
                                "&#x10FFFF;",
                                "&ClockwiseContourIntegral;"};
  for (size_t i = 0; i < sizeof(valid) / sizeof(*valid); ++i)
    for (size_t len = 0; len < FIO_STRLEN(valid[i]); ++len)
      expect_invalid(valid[i], len);

  /* Exhaust every byte in one-digit numeric references and each position of
   * a digit-bearing named entity, including NUL, controls and high bytes. */
  for (unsigned byte = 0; byte < 256; ++byte) {
    char dec[] = {'&', '#', (char)byte, ';'};
    char hex[] = {'&', '#', 'x', (char)byte, ';'};
    unsigned value = byte - '0';
    char decoded = value < 10 ? (char)value : 0;
    if (value < 10)
      expect_window(dec,
                    sizeof(dec),
                    decoded ? &decoded : "\xEF\xBF\xBD",
                    decoded ? 1 : 3);
    else
      expect_invalid(dec, sizeof(dec));
    value = value < 10 ? value : 256;
    if (byte >= 'A' && byte <= 'F')
      value = byte - 'A' + 10;
    else if (byte >= 'a' && byte <= 'f')
      value = byte - 'a' + 10;
    if (value < 16) {
      decoded = (char)value;
      expect_window(hex,
                    sizeof(hex),
                    decoded ? &decoded : "\xEF\xBF\xBD",
                    decoded ? 1 : 3);
    } else
      expect_invalid(hex, sizeof(hex));
    for (size_t pos = 1; pos < 7; ++pos) {
      char name[] = "&frac14;";
      name[pos] = (char)byte;
      static const char *names[] = {"&frac14;", "&frac12;", "&frac34;"};
      static const char *results[] = {"\xC2\xBC", "\xC2\xBD", "\xC2\xBE"};
      const char *expected = NULL;
      for (size_t i = 0; i < 3; ++i) {
        int match = 1;
        for (size_t j = 1; j < 7; ++j) {
          unsigned c = (unsigned char)name[j];
          c += (c >= 'A' && c <= 'Z') ? 32 : 0;
          match &= c == (unsigned char)names[i][j];
        }
        if (match)
          expected = results[i];
      }
      expect_window(name, sizeof(name) - 1, expected, expected ? 2 : 0);
    }
  }
}

int main(void) {
  test_named_entities();
  test_numeric_entities();
  test_invalid_entities();
  test_bounded_grammar();
  return 0;
}
