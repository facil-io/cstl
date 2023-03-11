/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_PUBSUB Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_PUBSUB_TEST___H)
#define H___FIO_PUBSUB_TEST___H
#ifndef H___FIO_PUBSUB___H
#define FIO_PUBSUB
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

/* *****************************************************************************
Letter Testing
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, letter)(void) {
  fprintf(stderr,
          "* Testing letter format (pub/sub "
          "message exchange)\n");
  struct test_info {
    char *channel;
    char *msg;
    int16_t filter;
    uint8_t flags;
  } test_info[] = {
      {(char *)"My Channel", (char *)"My channel Message", 0, 0},
      {NULL, (char *)"My filter Message", 1, 255},
      {(char *)"My Channel and Filter",
       (char *)"My channel -filter Message",
       257,
       4},
      {(char *)"My Channel and negative Filter",
       (char *)"My channel - filter Message",
       -3,
       8},
      {0},
  };
  for (int i = 0;
       test_info[i].msg || test_info[i].channel || test_info[i].filter;
       ++i) {
    fio_letter_s *l = fio_letter_new_compose(
        FIO_BUF_INFO2(
            test_info[i].channel,
            (test_info[i].channel ? FIO_STRLEN(test_info[i].channel) : 0)),
        FIO_BUF_INFO2(test_info[i].msg,
                      (test_info[i].msg ? FIO_STRLEN(test_info[i].msg) : 0)),
        test_info[i].filter,
        test_info[i].flags);
    FIO_ASSERT(fio_letter_filter(l) == test_info[i].filter,
               "letter filter identity error");
    FIO_ASSERT(fio_letter_flags(l) == test_info[i].flags,
               "letter flag identity error");
    if (test_info[i].msg) {
      FIO_ASSERT(fio_letter_message_len(l) == FIO_STRLEN(test_info[i].msg),
                 "letter message length error");
      FIO_ASSERT(!memcmp(fio_letter_message(l).buf,
                         test_info[i].msg,
                         fio_letter_message_len(l)),
                 "message identity error (%s != %.*s)",
                 test_info[i].msg,
                 (int)fio_letter_message_len(l),
                 fio_letter_message(l).buf);
    } else {
      FIO_ASSERT(!fio_letter_message_len(l),
                 "letter message length error %d != 0",
                 fio_letter_message_len(l));
    }
    if (test_info[i].channel) {
      FIO_ASSERT(fio_letter_channel_len(l) == FIO_STRLEN(test_info[i].channel),
                 "letter channel length error");
      FIO_ASSERT(fio_letter_channel(l).buf &&
                     !memcmp(fio_letter_channel(l).buf,
                             test_info[i].channel,
                             fio_letter_channel_len(l)),
                 "channel identity error (%s != %.*s)",
                 test_info[i].channel,
                 (int)fio_letter_channel_len(l),
                 fio_letter_channel(l).buf);
    } else {
      FIO_ASSERT(!fio_letter_channel_len(l), "letter channel length error");
    }

    fio_letter_free(l);
  }
}

/* *****************************************************************************
Round Trip Testing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_on_message)(fio_msg_s *msg) {
  ((int *)(msg->udata))[0] += 1;
}
FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_on_unsubscribe)(void *udata) {
  ((int *)(udata))[0] -= 1;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_roundtrip)(void) {
  fprintf(stderr, "* Testing pub/sub round-trip.\n");
  uintptr_t sub_handle = 0;
  int state = 0, expected = 0, delta = 0;
  fio_buf_info_s test_channel = FIO_BUF_INFO1((char *)"pubsub_test_channel");
  subscribe_args_s sub[] = {
      {
          .channel = test_channel,
          .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
          .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
          .filter = -127,
          .udata = &state,
      },
      {
          .channel = test_channel,
          .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
          .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
          .subscription_handle_ptr = &sub_handle,
          .udata = &state,
          .filter = -127,
      },
      {
          .channel = FIO_BUF_INFO1((char *)"pubsub_*"),
          .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
          .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
          .filter = -127,
          .udata = &state,
          .is_pattern = 1,
      },
  };
  const int sub_count = (sizeof(sub) / sizeof(sub[0]));
#define FIO___PUBLISH2TEST()                                                   \
  fio_publish(.channel = test_channel,                                         \
              .filter = -127,                                                  \
              .engine = FIO_PUBSUB_CLUSTER);                                   \
  expected += delta;                                                           \
  fio_queue_perform_all(fio___srv_tasks);
  for (int i = 0; i < sub_count; ++i) {
    fio_subscribe FIO_NOOP(sub[i]);
    ++delta;
    FIO_ASSERT(state == expected,
               "subscribe shouldn't have "
               "affected state");
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (1-%d)", i);
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (2-%d)", i);
  }
  for (int i = 0; i < sub_count; ++i) {
    fio_unsubscribe FIO_NOOP(sub[i]);
    --delta;
    --expected;
    fio_queue_perform_all(fio___srv_tasks);
    FIO_ASSERT(state == expected, "unsubscribe should call callback");
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (3-%d)", i);
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (4-%d)", i);
  }
#undef FIO___PUBLISH2TEST
}

/* *****************************************************************************

***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub)(void) {
  FIO_NAME_TEST(stl, letter)();
  FIO_NAME_TEST(stl, pubsub_roundtrip)();
  fio___srv_cleanup_at_exit(NULL);
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
