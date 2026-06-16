/* *****************************************************************************
ML Entity Decoding Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_ENTITY
#include FIO_INCLUDE_FILE

#include <string.h>

static void expect_entity(const char *entity,
                          const char *expected,
                          size_t expected_len) {
  char buf[8];
  FIO_MEMSET(buf, 0xA5, sizeof(buf));

  size_t written = fio_entity(buf, entity, FIO_STRLEN(entity));
  FIO_ASSERT(written == expected_len,
             "fio_entity(%s) length: expected %zu, got %zu",
             entity,
             expected_len,
             written);
  FIO_ASSERT(!FIO_MEMCMP(buf, expected, expected_len),
             "fio_entity(%s) decoded bytes mismatch",
             entity);
  FIO_ASSERT(buf[written] == '\0',
             "fio_entity(%s) should NUL terminate short output",
             entity);
}

static void expect_invalid(const char *entity, size_t len) {
  char buf[8];
  FIO_MEMSET(buf, 0xA5, sizeof(buf));

  size_t written = fio_entity(buf, entity, len);
  FIO_ASSERT(written == 0,
             "fio_entity should reject invalid input: %.*s",
             (int)len,
             entity ? entity : "(null)");
  FIO_ASSERT(buf[0] == (char)0xA5,
             "fio_entity invalid input should not write output");
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

int main(void) {
  test_named_entities();
  test_numeric_entities();
  test_invalid_entities();
  return 0;
}
