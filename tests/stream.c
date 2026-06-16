/* *****************************************************************************
Test - Stream buffering module (102 stream.h)

Correctness coverage for fio_stream_init/destroy/add/read/advance/length/any
and packing helpers (fio_stream_pack_data / fio_stream_pack_fd).
***************************************************************************** */
#include "test-helpers.h"

#define FIO_MALLOC
#define FIO_STREAM
#include FIO_INCLUDE_FILE

FIO_SFUNC size_t FIO_NAME_TEST(stl, stream___noop_dealloc_count) = 0;
FIO_SFUNC void FIO_NAME_TEST(stl, stream___noop_dealloc)(void *ignr_) {
  fio_atomic_add(&FIO_NAME_TEST(stl, stream___noop_dealloc_count), 1);
  (void)ignr_;
}

static void test_stream_embedded_copy(void) {
  const char *const str =
      "My Hello World string should be long enough so it can be used "
      "for testing the stream functionality in the facil.io stream "
      "module. The stream module takes strings and files and places "
      "them (by reference / copy) into a linked list of objects.";
  fio_stream_s s = FIO_STREAM_INIT(s);
  char mem[4096];
  char *buf = mem;
  size_t len = sizeof(mem);
  size_t expect_dealloc = FIO_NAME_TEST(stl, stream___noop_dealloc_count);

  fio_stream_add(
      &s,
      fio_stream_pack_data((void *)str,
                           11,
                           3,
                           1,
                           FIO_NAME_TEST(stl, stream___noop_dealloc)));
  ++expect_dealloc;
  FIO_ASSERT(fio_stream_any(&s),
             "stream is empty after fio_stream_add (data, copy)");
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "copying a packet should deallocate the original");

  for (size_t i = 0; i < 3; ++i) {
    buf = mem;
    len = sizeof(mem);
    fio_stream_read(&s, &buf, &len);
    FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) ==
                   expect_dealloc,
               "reading a packet shouldn't deallocate anything");
    FIO_ASSERT(len == 11,
               "fio_stream_read didn't read all data from stream? (%zu)",
               len);
    FIO_ASSERT(!memcmp(str + 3, buf, len),
               "fio_stream_read data error? (%.*s)",
               (int)len,
               buf);
  }

  fio_stream_advance(&s, len);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "advancing an embedded packet shouldn't deallocate anything");
  FIO_ASSERT(!fio_stream_any(&s),
             "after advance, the stream should have been consumed");

  buf = mem;
  len = sizeof(mem);
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(!buf && !len,
             "reading from an empty stream should set buf and len to zero");

  fio_stream_destroy(&s);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "destroying an empty stream shouldn't deallocate anything");
  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty");
  fprintf(stderr, "* embedded copy packet: OK\n");
}

static void test_stream_external_reference(void) {
  const char *const str =
      "My Hello World string should be long enough so it can be used "
      "for testing the stream functionality in the facil.io stream "
      "module. The stream module takes strings and files and places "
      "them (by reference / copy) into a linked list of objects.";
  fio_stream_s s = FIO_STREAM_INIT(s);
  char mem[4096];
  char *buf = mem;
  size_t len = sizeof(mem);
  size_t expect_dealloc = FIO_NAME_TEST(stl, stream___noop_dealloc_count);

  fio_stream_add(
      &s,
      fio_stream_pack_data((void *)str,
                           49,
                           11,
                           0,
                           FIO_NAME_TEST(stl, stream___noop_dealloc)));
  expect_dealloc += (49 < FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN);
  FIO_ASSERT(fio_stream_any(&s), "stream with data shouldn't be empty");
  FIO_ASSERT(fio_stream_length(&s) == 49, "stream length error");
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "adding a stream shouldn't deallocate it");

  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 49,
             "fio_stream_read didn't read all data from stream? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str + 11, buf, len),
             "fio_stream_read data error? (%.*s)",
             (int)len,
             buf);

  fio_stream_advance(&s, 80);
  expect_dealloc +=
      (49 >= FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "advance past packet should deallocate external packet");
  FIO_ASSERT(!fio_stream_any(&s), "stream should be empty at this point");
  FIO_ASSERT(!fio_stream_length(&s), "stream length should be zero");

  fio_stream_destroy(&s);
  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty");
  fprintf(stderr, "* external reference packet: OK\n");
}

static void test_stream_partial_read_and_advance(void) {
  const char *const str =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  fio_stream_s s = FIO_STREAM_INIT(s);
  char mem[4096];
  char *buf = mem;
  size_t len;

  /* First packet is short (copied) so a read larger than it forces a
   * fragmented copy and exercises partial read behavior. */
  fio_stream_add(
      &s, fio_stream_pack_data((void *)str, 5, 0, 1, NULL));
  fio_stream_add(
      &s, fio_stream_pack_data((void *)str, 20, 5, 0, NULL));
  fio_stream_add(
      &s, fio_stream_pack_data((void *)str, 15, 25, 1, NULL));
  FIO_ASSERT(fio_stream_length(&s) == 40, "stream length error");

  buf = mem;
  len = 8;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 8, "partial read length error (%zu)", len);
  FIO_ASSERT(!memcmp(str, buf, len),
             "partial read data error");

  fio_stream_advance(&s, 10);
  FIO_ASSERT(fio_stream_length(&s) == 30, "stream length after advance");

  buf = mem;
  len = sizeof(mem);
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 30,
             "fio_stream_read didn't read remaining data? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str + 10, buf, len),
             "remaining data mismatch");

  fio_stream_destroy(&s);
  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty");
  fprintf(stderr, "* partial read/advance: OK\n");
}

static void test_stream_file_packet(void) {
  fio_stream_s s = FIO_STREAM_INIT(s);
  char mem[4096];
  char *buf = mem;
  size_t len = sizeof(mem);

  int fd = open(__FILE__, O_RDONLY);
  FIO_ASSERT(fd >= 0, "failed to open %s", __FILE__);

  fio_stream_add(&s, fio_stream_pack_fd(fd, 20, 0, 0));
  FIO_ASSERT(fio_stream_length(&s) == 20, "stream length error (file)");

  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 20,
             "fio_stream_read didn't read file data from stream? (%zu)",
             len);
  FIO_ASSERT(!memcmp("/* *****************", buf, 20),
             "fio_stream_read file read data error?\n%.*s",
             (int)len,
             buf);

  buf = mem;
  len = sizeof(mem);
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 20,
             "fio_stream_read didn't re-read file data? (%zu)",
             len);

  fio_stream_destroy(&s);
  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty");
  fprintf(stderr, "* file descriptor packet: OK\n");
}

static void test_stream_pack_free(void) {
  const char *const str = "unused packet";
  size_t expect_dealloc = FIO_NAME_TEST(stl, stream___noop_dealloc_count);
  fio_stream_packet_s *p = fio_stream_pack_data(
      (void *)str,
      13,
      0,
      0,
      FIO_NAME_TEST(stl, stream___noop_dealloc));
  /* If copied, pack_data already called dealloc; if external, pack_free
   * will call dealloc. Either way the count increases by one. */
  ++expect_dealloc;
  FIO_ASSERT(p, "fio_stream_pack_data returned NULL");
  fio_stream_pack_free(p);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "fio_stream_pack_free should deallocate packet");
  fprintf(stderr, "* pack_free: OK\n");
}

int main(void) {
  test_stream_embedded_copy();
  test_stream_external_reference();
  test_stream_partial_read_and_advance();
  test_stream_file_packet();
  test_stream_pack_free();
  return 0;
}
