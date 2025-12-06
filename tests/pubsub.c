/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_PUBSUB
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Encryption Testing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_encryption)(void) {
  fprintf(stderr, "\t* Testing pub/sub encryption / decryption.\n");
  fio_publish_args_s origin = {.channel = FIO_BUF_INFO1((char *)"my channel"),
                               .message = FIO_BUF_INFO1((char *)"my message"),
                               .filter = 0xAA,
                               .is_json =
                                   FIO___PUBSUB_JSON | FIO___PUBSUB_CLUSTER};
  fio___pubsub_message_s *enc = fio___pubsub_message_author(origin);
  fio___pubsub_message_encrypt(enc);
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(enc->data.channel, origin.channel),
             "channel info error");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(enc->data.message, origin.message),
             "message info error");
  FIO_ASSERT(enc->data.filter == origin.filter, "filter info error");
  FIO_ASSERT(enc->data.is_json == origin.is_json, "flags info error");
  FIO_MEM_STACK_WIPE(2);

  fio___pubsub_message_s *dec = fio___pubsub_message_alloc(enc->data.udata);
  FIO_MEMCPY(dec->data.udata,
             enc->data.udata,
             origin.channel.len + origin.message.len +
                 FIO___PUBSUB_MESSAGE_OVERHEAD);
  dec->data.udata = enc->data.udata;
  FIO_ASSERT(!fio___pubsub_message_decrypt(dec), "decryption failed");
  FIO_ASSERT(enc->data.filter == dec->data.filter,
             "(pubsub) filter enc/dec error");
  FIO_ASSERT(enc->data.is_json == dec->data.is_json,
             "(pubsub) is_json enc/dec error");
  FIO_ASSERT(enc->data.id == dec->data.id, "(pubsub) id enc/dec error");
  FIO_ASSERT(enc->data.published == dec->data.published,
             "(pubsub) published enc/dec error");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(enc->data.channel, dec->data.channel),
             "(pubsub) channel enc/dec error");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(enc->data.message, dec->data.message),
             "(pubsub) message enc/dec error");
  fio___pubsub_message_free(enc);
  fio___pubsub_message_free(dec);
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
  fprintf(stderr, "\t* Testing pub/sub round-trip.\n");
  uintptr_t sub_handle = 0;
  int state = 0, expected = 0, delta = 0;
  fio_buf_info_s test_channel = FIO_BUF_INFO1((char *)"pubsub_test_channel");
  fio_subscribe_args_s sub[] = {
      {
          .channel = test_channel,
          .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
          .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
          .udata = &state,
          .filter = -127,
      },
      {
          .channel = test_channel,
          .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
          .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
          .udata = &state,
          .subscription_handle_ptr = &sub_handle,
          .filter = -127,
      },
      {
          .channel = FIO_BUF_INFO1((char *)"pubsub_*"),
          .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
          .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
          .udata = &state,
          .filter = -127,
          .is_pattern = 1,
      },
  };
  const size_t sub_count = (sizeof(sub) / sizeof(sub[0]));

#define FIO___PUBLISH2TEST()                                                   \
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,                                    \
              .channel = test_channel,                                         \
              .filter = -127);                                                 \
  expected += delta;                                                           \
  fio_queue_perform_all(fio_io_queue());

  for (size_t i = 0; i < sub_count; ++i) {
    fio_subscribe FIO_NOOP(sub[i]);
    ++delta;
    FIO_ASSERT(state == expected, "subscribe shouldn't affect state (%d)", i);
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (1-%d)", i);
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (2-%d)", i);
  }
  for (size_t i = 0; i < sub_count; ++i) {
    if (fio_unsubscribe FIO_NOOP(sub[i]))
      FIO_LOG_WARNING("fio_unsubscribe returned an error value");
    --delta;
    --expected;
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(state == expected, "unsubscribe should call callback (%i)", i);
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (3-%d)", i);
    FIO___PUBLISH2TEST();
    FIO_ASSERT(state == expected, "pub/sub test state incorrect (4-%d)", i);
  }
#undef FIO___PUBLISH2TEST
}

/* *****************************************************************************

***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, pubsub_encryption)();
  FIO_NAME_TEST(stl, pubsub_roundtrip)();
  fio___io_cleanup_at_exit(NULL);
}
