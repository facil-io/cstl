/* *****************************************************************************
Test - dynamic string object API (`200 string.h`)
***************************************************************************** */
#include "test-helpers.h"

#define FIO_STR
#define FIO_STR_NAME fio_str
#define FIO_STR_WRITE_TEST_FUNC
#include FIO_INCLUDE_FILE

#define FIO_STR
#define FIO_STR_SMALL fio_small_str
#define FIO_STR_WRITE_TEST_FUNC
#include FIO_INCLUDE_FILE

FIO_SFUNC void test_string_object_lifecycle(void) {
  fio_str_s s = FIO_STR_INIT;

  FIO_ASSERT(fio_str_len(&s) == 0, "new stack string should be empty");
  FIO_ASSERT(fio_str_ptr(&s) && fio_str_ptr(&s)[0] == 0,
             "new stack string should expose an empty C string");

  fio_str_write(&s, "Hello", 5);
  FIO_ASSERT(fio_str_len(&s) == 5, "write should update length");
  FIO_ASSERT(!FIO_MEMCMP(fio_str_ptr(&s), "Hello", 6),
             "write should preserve content");

  fio_str_replace(&s, 5, 0, " World", 6);
  FIO_ASSERT(!FIO_MEMCMP(fio_str_ptr(&s), "Hello World", 12),
             "replace should append at exact end");

  fio_str_resize(&s, 5);
  FIO_ASSERT(fio_str_len(&s) == 5, "resize should shrink length");
  FIO_ASSERT(!FIO_MEMCMP(fio_str_ptr(&s), "Hello", 6),
             "resize should NUL terminate after shrink");

  fio_str_destroy(&s);
  FIO_ASSERT(fio_str_len(&s) == 0, "destroy should reset length");
}

FIO_SFUNC void test_string_object_init_copy_concat_detach(void) {
  fio_str_s a = FIO_STR_INIT;
  fio_str_s b = FIO_STR_INIT;
  const char *long_text =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  fio_str_init_copy(&a, long_text, FIO_STRLEN(long_text));
  FIO_ASSERT(fio_str_len(&a) == FIO_STRLEN(long_text),
             "init_copy should copy full length");
  FIO_ASSERT(fio_str_ptr(&a) != long_text,
             "init_copy should not alias source memory");
  FIO_ASSERT(!FIO_MEMCMP(fio_str_ptr(&a), long_text, FIO_STRLEN(long_text)),
             "init_copy content mismatch");

  fio_str_init_copy2(&b, &a);
  FIO_ASSERT(fio_str_is_eq(&a, &b), "init_copy2 should create equal string");
  fio_str_write(&b, "!", 1);
  FIO_ASSERT(!fio_str_is_eq(&a, &b),
             "mutating copy should not mutate original string");

  fio_str_destroy(&b);
  fio_str_init_const(&b, " + suffix", 9);
  fio_str_concat(&a, &b);
  FIO_ASSERT(fio_str_len(&a) == FIO_STRLEN(long_text) + 9,
             "concat should append source length");
  FIO_ASSERT(
      !FIO_MEMCMP(fio_str_ptr(&a) + FIO_STRLEN(long_text), " + suffix", 10),
      "concat content mismatch");

  char *detached = fio_str_detach(&a);
  FIO_ASSERT(detached, "detach should return owned buffer");
  FIO_ASSERT(!FIO_MEMCMP(detached + FIO_STRLEN(long_text), " + suffix", 10),
             "detach should preserve content");
  FIO_ASSERT(fio_str_len(&a) == 0, "detach should clear source object");
  fio_str_dealloc(detached);

  fio_str_destroy(&a);
  fio_str_destroy(&b);
}

FIO_SFUNC void test_string_object_freeze_hash_file(void) {
  fio_str_s s = FIO_STR_INIT;
  fio_str_write(&s, "immutable", 9);
  uint64_t h1 = fio_str_hash(&s, 0x12345678U);
  uint64_t h2 = fio_str_hash(&s, 0x12345678U);
  FIO_ASSERT(h1 == h2, "hash should be deterministic for same seed");

  fio_str_freeze(&s);
  FIO_ASSERT(fio_str_is_frozen(&s), "freeze should set frozen flag");
  fio_str_destroy(&s);

  fio_str_s file = FIO_STR_INIT;
  fio_str_info_s state = fio_str_readfile(&file, __FILE__, 0, 0);
  FIO_ASSERT(state.buf && state.len, "readfile should read this test file");
  FIO_ASSERT(fio_str_utf8_valid(&file), "this source file should be UTF-8");
  char *needle =
      (char *)FIO_MEMCHR(fio_str_ptr(&file), '2', fio_str_len(&file));
  FIO_ASSERT(needle, "read file should contain module number marker");
  fio_str_destroy(&file);
}

FIO_SFUNC void test_small_string_flavor(void) {
  fio_small_str_s key = FIO_STR_INIT;
  fio_small_str_init_const(&key, "short", 5);
  FIO_ASSERT(fio_small_str_len(&key) == 5, "small string init length");
  FIO_ASSERT(!FIO_MEMCMP(fio_small_str_ptr(&key), "short", 6),
             "small string init content");
  FIO_ASSERT(!fio_small_str_is_allocated(&key),
             "short small string should stay embedded/static");

  fio_small_str_write(&key, "-key", 4);
  FIO_ASSERT(!FIO_MEMCMP(fio_small_str_ptr(&key), "short-key", 10),
             "small string write content");

  fio_small_str_s copy = FIO_STR_INIT;
  fio_small_str_init_copy2(&copy, &key);
  FIO_ASSERT(fio_small_str_is_eq(&key, &copy),
             "small string copy should compare equal");
  FIO_ASSERT(fio_small_str_hash(&key, 7) == fio_small_str_hash(&copy, 7),
             "equal small strings should hash equally with same seed");

  fio_small_str_destroy(&copy);
  fio_small_str_destroy(&key);
}

int main(void) {
  FIO_NAME_TEST(stl, fio_str)();
  FIO_NAME_TEST(stl, fio_small_str)();
  test_string_object_lifecycle();
  test_string_object_init_copy_concat_detach();
  test_string_object_freeze_hash_file();
  test_small_string_flavor();
  return 0;
}
