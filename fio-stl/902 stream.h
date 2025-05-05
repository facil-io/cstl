/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_STREAM Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_STREAM_TEST___H)
#define H___FIO_STREAM_TEST___H
#ifndef H___FIO_STREAM___H
#define FIO_STREAM
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

FIO_SFUNC size_t FIO_NAME_TEST(stl, stream___noop_dealloc_count) = 0;
FIO_SFUNC void FIO_NAME_TEST(stl, stream___noop_dealloc)(void *ignr_) {
  fio_atomic_add(&FIO_NAME_TEST(stl, stream___noop_dealloc_count), 1);
  (void)ignr_;
}

FIO_SFUNC void FIO_NAME_TEST(stl, stream)(void) {
  char *const str =
      (char *)"My Hello World string should be long enough so it can be used "
              "for testing the stream functionality in the facil.io stream "
              "module. The stream moduule takes strings and failes and places "
              "them (by reference / copy) into a linked list of objects. When "
              "data is requested from the stream, the stream will either copy "
              "the data to a pre-allocated buffer or it may update the link to "
              "it points to its own internal buffer (avoiding a copy when "
              "possible).";
  fio_stream_s s = FIO_STREAM_INIT(s);
  char mem[4000];
  char *buf = mem;
  size_t len = 4000;
  size_t expect_dealloc = FIO_NAME_TEST(stl, stream___noop_dealloc_count);

  fprintf(stderr, "* Testing fio_stream for streaming buffer storage.\n");
  fio_stream_add(
      &s,
      fio_stream_pack_data(str,
                           11,
                           3,
                           1,
                           FIO_NAME_TEST(stl, stream___noop_dealloc)));
  ++expect_dealloc;
  FIO_ASSERT(fio_stream_any(&s),
             "stream is empty after `fio_stream_add` (data, copy)");
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "copying a packet should deallocate the original");
  for (size_t i = 0; i < 3; ++i) {
    /* test that read operrations are immutable */
    buf = mem;
    len = 4000;

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
    FIO_ASSERT_DEBUG(
        buf != mem,
        "fio_stream_read should have been performed with zero-copy");
  }
  fio_stream_advance(&s, len);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "advancing an embedded packet shouldn't deallocate anything");
  FIO_ASSERT(
      !fio_stream_any(&s),
      "after advance, at this point, the stream should have been consumed.");
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(
      !buf && !len,
      "reading from an empty stream should set buf and len to NULL and zero.");
  fio_stream_destroy(&s);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "destroying an empty stream shouldn't deallocate anything");
  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty.");

  fio_stream_add(&s, fio_stream_pack_data(str, 11, 0, 1, NULL));
  fio_stream_add(
      &s,
      fio_stream_pack_data(str,
                           49,
                           11,
                           0,
                           FIO_NAME_TEST(stl, stream___noop_dealloc)));
  fio_stream_add(&s, fio_stream_pack_data(str, 20, 60, 0, NULL));
  expect_dealloc += (49 < FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN);
  FIO_ASSERT(fio_stream_any(&s), "stream with data shouldn't be empty.");
  FIO_ASSERT(fio_stream_length(&s) == 80, "stream length error.");
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "adding a stream shouldn't deallocate it.");

  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);

  FIO_ASSERT(len == 80,
             "fio_stream_read didn't read all data from stream(2)? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str, buf, len),
             "fio_stream_read data error? (%.*s)",
             (int)len,
             buf);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "reading a stream shouldn't deallocate any packets.");

  buf = mem;
  len = 8;
  fio_stream_read(&s, &buf, &len);

  FIO_ASSERT(len < 80,
             "fio_stream_read didn't perform a partial read? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str, buf, len),
             "fio_stream_read partial read data error? (%.*s)",
             (int)len,
             buf);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "failing to read a stream shouldn't deallocate any packets.");

  fio_stream_advance(&s, 20);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "partial advancing shouldn't deallocate any packets.");
  FIO_ASSERT(fio_stream_length(&s) == 60, "stream length error (2).");
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 60,
             "fio_stream_read didn't read all data from stream(3)? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str + 20, buf, len),
             "fio_stream_read data error? (%.*s)",
             (int)len,
             buf);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "reading shouldn't deallocate packets the head packet.");

  fio_stream_add(&s, fio_stream_pack_fd(open(__FILE__, O_RDONLY), 20, 0, 0));
  FIO_ASSERT(fio_stream_length(&s) == 80, "stream length error (3).");
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 80,
             "fio_stream_read didn't read all data from stream(4)? (%zu)",
             len);
  FIO_ASSERT(!memcmp("/* *****************", buf + 60, 20),
             "fio_stream_read file read data error?\n%.*s",
             (int)len,
             buf);
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "reading more than one packet shouldn't deallocate anything.");
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 80,
             "fio_stream_read didn't (re)read all data from stream(5)? (%zu)",
             len);
  FIO_ASSERT(!memcmp("/* *****************", buf + 60, 20),
             "fio_stream_read file (re)read data error? (%.*s)",
             (int)len,
             buf);

  fio_stream_destroy(&s);
  expect_dealloc += (49 >= FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN);

  FIO_ASSERT(!fio_stream_any(&s), "destroyed stream should be empty.");
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "destroying a stream should deallocate it's packets.");
  fio_stream_add(
      &s,
      fio_stream_pack_data(str,
                           49,
                           11,
                           0,
                           FIO_NAME_TEST(stl, stream___noop_dealloc)));
  buf = mem;
  len = 4000;
  fio_stream_read(&s, &buf, &len);
  FIO_ASSERT(len == 49,
             "fio_stream_read didn't read all data from stream? (%zu)",
             len);
  FIO_ASSERT(!memcmp(str + 11, buf, len),
             "fio_stream_read data error? (%.*s)",
             (int)len,
             buf);
  fio_stream_advance(&s, 80);
  ++expect_dealloc;
  FIO_ASSERT(FIO_NAME_TEST(stl, stream___noop_dealloc_count) == expect_dealloc,
             "partial advancing shouldn't deallocate any packets.");
  FIO_ASSERT(!fio_stream_any(&s), "stream should be empty at this point.");
  FIO_ASSERT(!fio_stream_length(&s),
             "stream length should be zero at this point.");
  fio_stream_destroy(&s);
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
