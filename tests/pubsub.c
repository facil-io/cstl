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
  FIO_LOG_DDEBUG("Testing pub/sub encryption / decryption.");
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
  FIO_LOG_DDEBUG("Testing pub/sub round-trip.");
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
Channel Isolation Testing - subscribers only receive their channel's messages
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_channel_isolation)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub channel isolation.");
  int state_a = 0, state_b = 0;
  fio_buf_info_s channel_a = FIO_BUF_INFO1((char *)"channel_a");
  fio_buf_info_s channel_b = FIO_BUF_INFO1((char *)"channel_b");

  /* Subscribe to different channels */
  fio_subscribe(.channel = channel_a,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state_a,
                .filter = -126);
  fio_subscribe(.channel = channel_b,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state_b,
                .filter = -126);

  /* Publish to channel_a only */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = channel_a,
              .filter = -126);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state_a == 1, "channel_a subscriber should receive message");
  FIO_ASSERT(state_b == 0, "channel_b subscriber should NOT receive message");

  /* Publish to channel_b only */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = channel_b,
              .filter = -126);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state_a == 1, "channel_a state should remain unchanged");
  FIO_ASSERT(state_b == 1, "channel_b subscriber should receive message");

  /* Cleanup */
  fio_unsubscribe(.channel = channel_a, .filter = -126);
  fio_unsubscribe(.channel = channel_b, .filter = -126);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state_a == 0, "channel_a unsubscribe callback should fire");
  FIO_ASSERT(state_b == 0, "channel_b unsubscribe callback should fire");
}

/* *****************************************************************************
Filter Namespace Testing - different filters are isolated
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_filter_isolation)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub filter namespace isolation.");
  int state_f1 = 0, state_f2 = 0;
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"filter_test_channel");

  /* Subscribe to same channel with different filters */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state_f1,
                .filter = -125);
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state_f2,
                .filter = -124);

  /* Publish with filter -125 */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER, .channel = channel, .filter = -125);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state_f1 == 1, "filter -125 subscriber should receive message");
  FIO_ASSERT(state_f2 == 0,
             "filter -124 subscriber should NOT receive message");

  /* Publish with filter -124 */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER, .channel = channel, .filter = -124);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state_f1 == 1, "filter -125 state should remain unchanged");
  FIO_ASSERT(state_f2 == 1, "filter -124 subscriber should receive message");

  /* Cleanup */
  fio_unsubscribe(.channel = channel, .filter = -125);
  fio_unsubscribe(.channel = channel, .filter = -124);
  fio_queue_perform_all(fio_io_queue());
}

/* *****************************************************************************
No Subscribers Testing - publish to empty channel should not crash
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_no_subscribers)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub publish to channel with no subscribers.");
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"empty_channel");

  /* Publish to a channel with no subscribers - should not crash */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = channel,
              .message = FIO_BUF_INFO1((char *)"test message"),
              .filter = -123);
  fio_queue_perform_all(fio_io_queue());

  /* If we get here without crashing, the test passes */
  FIO_ASSERT(1, "publish to empty channel should not crash");
}

/* *****************************************************************************
Empty Channel Name Testing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_empty_channel)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub with empty channel name.");
  int state = 0;
  fio_buf_info_s empty_channel = FIO_BUF_INFO2((char *)"", 0);

  /* Subscribe to empty channel name */
  fio_subscribe(.channel = empty_channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state,
                .filter = -122);

  /* Publish to empty channel */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = empty_channel,
              .filter = -122);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state == 1, "empty channel subscriber should receive message");

  /* Cleanup */
  fio_unsubscribe(.channel = empty_channel, .filter = -122);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state == 0, "empty channel unsubscribe callback should fire");
}

/* *****************************************************************************
Large Message Payload Testing
***************************************************************************** */

FIO_SFUNC int FIO_NAME_TEST(stl, pubsub_large_msg_received) = 0;
FIO_SFUNC size_t FIO_NAME_TEST(stl, pubsub_large_msg_len) = 0;

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_large_on_message)(fio_msg_s *msg) {
  FIO_NAME_TEST(stl, pubsub_large_msg_received) = 1;
  FIO_NAME_TEST(stl, pubsub_large_msg_len) = msg->message.len;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_large_message)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub with large message payload.");
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"large_msg_channel");

  /* Create a large message (64KB) */
  size_t large_size = 64 * 1024;
  char *large_msg = (char *)FIO_MEM_REALLOC(NULL, 0, large_size, 0);
  FIO_ASSERT_ALLOC(large_msg);
  FIO_MEMSET(large_msg, 'X', large_size);

  FIO_NAME_TEST(stl, pubsub_large_msg_received) = 0;
  FIO_NAME_TEST(stl, pubsub_large_msg_len) = 0;

  /* Subscribe */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_large_on_message),
                .filter = -121);

  /* Publish large message */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = channel,
              .message = FIO_BUF_INFO2(large_msg, large_size),
              .filter = -121);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_large_msg_received) == 1,
             "large message should be received");
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_large_msg_len) == large_size,
             "large message length should match (%zu != %zu)",
             FIO_NAME_TEST(stl, pubsub_large_msg_len),
             large_size);

  /* Cleanup */
  fio_unsubscribe(.channel = channel, .filter = -121);
  fio_queue_perform_all(fio_io_queue());
  FIO_MEM_FREE(large_msg, large_size);
}

/* *****************************************************************************
Rapid Subscribe/Unsubscribe Cycles Testing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_rapid_cycles)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub rapid subscribe/unsubscribe cycles.");
  int state = 0;
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"rapid_cycle_channel");
  const int cycles = 100;

  for (int i = 0; i < cycles; ++i) {
    fio_subscribe(.channel = channel,
                  .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                  .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                  .udata = &state,
                  .filter = -120);

    fio_publish(.engine = FIO_PUBSUB_CLUSTER,
                .channel = channel,
                .filter = -120);
    fio_queue_perform_all(fio_io_queue());

    fio_unsubscribe(.channel = channel, .filter = -120);
    fio_queue_perform_all(fio_io_queue());
  }

  /* Each cycle: +1 from message, -1 from unsubscribe = net 0 */
  FIO_ASSERT(state == 0,
             "rapid cycles should balance out (state=%d, expected=0)",
             state);
}

/* *****************************************************************************
Subscription Handle Testing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_subscription_handle)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub subscription handle management.");
  int state = 0;
  uintptr_t handle = 0;
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"handle_test_channel");

  /* Subscribe with handle */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state,
                .subscription_handle_ptr = &handle,
                .filter = -119);

  FIO_ASSERT(handle != 0, "subscription handle should be non-zero");

  /* Publish and verify receipt */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER, .channel = channel, .filter = -119);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state == 1, "handle subscription should receive message");

  /* Unsubscribe using handle */
  fio_unsubscribe(.channel = channel,
                  .subscription_handle_ptr = &handle,
                  .filter = -119);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state == 0, "handle unsubscribe callback should fire");

  /* Verify no more messages received */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER, .channel = channel, .filter = -119);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state == 0, "unsubscribed handle should not receive messages");
}

/* *****************************************************************************
Engine Attachment/Detachment Testing
***************************************************************************** */

FIO_SFUNC int FIO_NAME_TEST(stl, engine_subscribe_called) = 0;
FIO_SFUNC int FIO_NAME_TEST(stl, engine_unsubscribe_called) = 0;
FIO_SFUNC int FIO_NAME_TEST(stl, engine_publish_called) = 0;
FIO_SFUNC int FIO_NAME_TEST(stl, engine_detached_called) = 0;

FIO_SFUNC void FIO_NAME_TEST(stl,
                             engine_subscribe)(const fio_pubsub_engine_s *eng,
                                               fio_buf_info_s channel,
                                               int16_t filter) {
  FIO_NAME_TEST(stl, engine_subscribe_called) += 1;
  (void)eng;
  (void)channel;
  (void)filter;
}

FIO_SFUNC void FIO_NAME_TEST(stl,
                             engine_unsubscribe)(const fio_pubsub_engine_s *eng,
                                                 fio_buf_info_s channel,
                                                 int16_t filter) {
  FIO_NAME_TEST(stl, engine_unsubscribe_called) += 1;
  (void)eng;
  (void)channel;
  (void)filter;
}

FIO_SFUNC void FIO_NAME_TEST(stl,
                             engine_publish)(const fio_pubsub_engine_s *eng,
                                             fio_msg_s *msg) {
  FIO_NAME_TEST(stl, engine_publish_called) += 1;
  (void)eng;
  (void)msg;
}

FIO_SFUNC void FIO_NAME_TEST(stl,
                             engine_detached)(const fio_pubsub_engine_s *eng) {
  FIO_NAME_TEST(stl, engine_detached_called) += 1;
  (void)eng;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_engine)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub engine attachment/detachment.");

  /* Reset counters */
  FIO_NAME_TEST(stl, engine_subscribe_called) = 0;
  FIO_NAME_TEST(stl, engine_unsubscribe_called) = 0;
  FIO_NAME_TEST(stl, engine_publish_called) = 0;
  FIO_NAME_TEST(stl, engine_detached_called) = 0;

  fio_pubsub_engine_s test_engine = {
      .subscribe = FIO_NAME_TEST(stl, engine_subscribe),
      .unsubscribe = FIO_NAME_TEST(stl, engine_unsubscribe),
      .publish = FIO_NAME_TEST(stl, engine_publish),
      .detached = FIO_NAME_TEST(stl, engine_detached),
  };

  int state = 0;
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"engine_test_channel");

  /* First subscribe to create a channel */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state,
                .filter = -118);
  fio_queue_perform_all(fio_io_queue());

  /* Attach engine - should call subscribe for existing channels */
  fio_pubsub_attach(&test_engine);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(FIO_NAME_TEST(stl, engine_subscribe_called) >= 1,
             "engine subscribe should be called for existing channels");

  /* Publish through the engine */
  fio_publish(.engine = &test_engine, .channel = channel, .filter = -118);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(FIO_NAME_TEST(stl, engine_publish_called) == 1,
             "engine publish should be called");

  /* Detach engine */
  fio_pubsub_detach(&test_engine);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(FIO_NAME_TEST(stl, engine_detached_called) == 1,
             "engine detached callback should be called");

  /* Cleanup subscription */
  fio_unsubscribe(.channel = channel, .filter = -118);
  fio_queue_perform_all(fio_io_queue());
}

/* *****************************************************************************
Message Content Verification Testing
***************************************************************************** */

FIO_SFUNC char FIO_NAME_TEST(stl, pubsub_received_channel_buf)[256] = {0};
FIO_SFUNC size_t FIO_NAME_TEST(stl, pubsub_received_channel_len) = 0;
FIO_SFUNC char FIO_NAME_TEST(stl, pubsub_received_message_buf)[256] = {0};
FIO_SFUNC size_t FIO_NAME_TEST(stl, pubsub_received_message_len) = 0;
FIO_SFUNC int16_t FIO_NAME_TEST(stl, pubsub_received_filter) = 0;
FIO_SFUNC uint8_t FIO_NAME_TEST(stl, pubsub_received_is_json) = 0;

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_content_on_message)(fio_msg_s *msg) {
  /* Copy channel content */
  FIO_NAME_TEST(stl, pubsub_received_channel_len) = msg->channel.len;
  if (msg->channel.len && msg->channel.len < 256) {
    FIO_MEMCPY(FIO_NAME_TEST(stl, pubsub_received_channel_buf),
               msg->channel.buf,
               msg->channel.len);
  }
  /* Copy message content */
  FIO_NAME_TEST(stl, pubsub_received_message_len) = msg->message.len;
  if (msg->message.len && msg->message.len < 256) {
    FIO_MEMCPY(FIO_NAME_TEST(stl, pubsub_received_message_buf),
               msg->message.buf,
               msg->message.len);
  }
  FIO_NAME_TEST(stl, pubsub_received_filter) = msg->filter;
  FIO_NAME_TEST(stl, pubsub_received_is_json) = msg->is_json;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_message_content)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub message content verification.");
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"content_test_channel");
  fio_buf_info_s message = FIO_BUF_INFO1((char *)"Hello, Pub/Sub World!");

  /* Reset received data */
  FIO_MEMSET(FIO_NAME_TEST(stl, pubsub_received_channel_buf), 0, 256);
  FIO_NAME_TEST(stl, pubsub_received_channel_len) = 0;
  FIO_MEMSET(FIO_NAME_TEST(stl, pubsub_received_message_buf), 0, 256);
  FIO_NAME_TEST(stl, pubsub_received_message_len) = 0;
  FIO_NAME_TEST(stl, pubsub_received_filter) = 0;
  FIO_NAME_TEST(stl, pubsub_received_is_json) = 0;

  /* Subscribe */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_content_on_message),
                .filter = -117);

  /* Publish with JSON flag */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = channel,
              .message = message,
              .filter = -117,
              .is_json = 1);
  fio_queue_perform_all(fio_io_queue());

  /* Verify content */
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_channel_len) == channel.len,
             "received channel length should match (%zu != %zu)",
             FIO_NAME_TEST(stl, pubsub_received_channel_len),
             channel.len);
  FIO_ASSERT(FIO_MEMCMP(FIO_NAME_TEST(stl, pubsub_received_channel_buf),
                        channel.buf,
                        channel.len) == 0,
             "received channel content should match");
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_message_len) == message.len,
             "received message length should match (%zu != %zu)",
             FIO_NAME_TEST(stl, pubsub_received_message_len),
             message.len);
  FIO_ASSERT(FIO_MEMCMP(FIO_NAME_TEST(stl, pubsub_received_message_buf),
                        message.buf,
                        message.len) == 0,
             "received message content should match");
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_filter) == -117,
             "received filter should match");
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_is_json) == 1,
             "received is_json flag should match");

  /* Cleanup */
  fio_unsubscribe(.channel = channel, .filter = -117);
  fio_queue_perform_all(fio_io_queue());
}

/* *****************************************************************************
Multiple Subscribers Same Channel Testing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_multiple_subscribers)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub multiple subscribers to same channel.");
  int state1 = 0, state2 = 0, state3 = 0;
  uintptr_t handle1 = 0, handle2 = 0, handle3 = 0;
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"multi_sub_channel");

  /* Subscribe multiple times with handles (to allow multiple subs) */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state1,
                .subscription_handle_ptr = &handle1,
                .filter = -116);
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state2,
                .subscription_handle_ptr = &handle2,
                .filter = -116);
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_on_message),
                .on_unsubscribe = FIO_NAME_TEST(stl, pubsub_on_unsubscribe),
                .udata = &state3,
                .subscription_handle_ptr = &handle3,
                .filter = -116);

  /* Publish once */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER, .channel = channel, .filter = -116);
  fio_queue_perform_all(fio_io_queue());

  /* All three should receive */
  FIO_ASSERT(state1 == 1, "subscriber 1 should receive message");
  FIO_ASSERT(state2 == 1, "subscriber 2 should receive message");
  FIO_ASSERT(state3 == 1, "subscriber 3 should receive message");

  /* Unsubscribe one */
  fio_unsubscribe(.channel = channel,
                  .subscription_handle_ptr = &handle2,
                  .filter = -116);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(state2 == 0, "subscriber 2 unsubscribe callback should fire");

  /* Publish again */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER, .channel = channel, .filter = -116);
  fio_queue_perform_all(fio_io_queue());

  /* Only 1 and 3 should receive */
  FIO_ASSERT(state1 == 2, "subscriber 1 should receive second message");
  FIO_ASSERT(state2 == 0, "subscriber 2 should NOT receive (unsubscribed)");
  FIO_ASSERT(state3 == 2, "subscriber 3 should receive second message");

  /* Cleanup */
  fio_unsubscribe(.channel = channel,
                  .subscription_handle_ptr = &handle1,
                  .filter = -116);
  fio_unsubscribe(.channel = channel,
                  .subscription_handle_ptr = &handle3,
                  .filter = -116);
  fio_queue_perform_all(fio_io_queue());
}

/* *****************************************************************************
Pattern Matching Testing
***************************************************************************** */

FIO_SFUNC int FIO_NAME_TEST(stl, pubsub_pattern_count) = 0;

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_pattern_on_message)(fio_msg_s *msg) {
  FIO_NAME_TEST(stl, pubsub_pattern_count) += 1;
  (void)msg;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_pattern_matching)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub pattern matching.");

  FIO_NAME_TEST(stl, pubsub_pattern_count) = 0;

  /* Subscribe to pattern */
  fio_subscribe(.channel = FIO_BUF_INFO1((char *)"test_*"),
                .on_message = FIO_NAME_TEST(stl, pubsub_pattern_on_message),
                .filter = -115,
                .is_pattern = 1);

  /* Publish to matching channels */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = FIO_BUF_INFO1((char *)"test_foo"),
              .filter = -115);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_pattern_count) == 1,
             "pattern should match test_foo");

  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = FIO_BUF_INFO1((char *)"test_bar"),
              .filter = -115);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_pattern_count) == 2,
             "pattern should match test_bar");

  /* Publish to non-matching channel */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = FIO_BUF_INFO1((char *)"other_channel"),
              .filter = -115);
  fio_queue_perform_all(fio_io_queue());

  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_pattern_count) == 2,
             "pattern should NOT match other_channel");

  /* Cleanup */
  fio_unsubscribe(.channel = FIO_BUF_INFO1((char *)"test_*"),
                  .filter = -115,
                  .is_pattern = 1);
  fio_queue_perform_all(fio_io_queue());
}

/* *****************************************************************************
Message ID and Timestamp Testing
***************************************************************************** */

FIO_SFUNC uint64_t FIO_NAME_TEST(stl, pubsub_received_id) = 0;
FIO_SFUNC uint64_t FIO_NAME_TEST(stl, pubsub_received_published) = 0;

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_id_on_message)(fio_msg_s *msg) {
  FIO_NAME_TEST(stl, pubsub_received_id) = msg->id;
  FIO_NAME_TEST(stl, pubsub_received_published) = msg->published;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_message_id)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub message ID and timestamp.");
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"id_test_channel");

  FIO_NAME_TEST(stl, pubsub_received_id) = 0;
  FIO_NAME_TEST(stl, pubsub_received_published) = 0;

  /* Subscribe */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_id_on_message),
                .filter = -114);

  /* Get current time for comparison */
  uint64_t before_publish = (uint64_t)fio_time2milli(fio_time_real());

  /* Publish */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER, .channel = channel, .filter = -114);
  fio_queue_perform_all(fio_io_queue());

  uint64_t after_publish = (uint64_t)fio_time2milli(fio_time_real());

  /* Verify ID is non-zero */
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_id) != 0,
             "message ID should be non-zero");

  /* Verify timestamp is reasonable */
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_published) >= before_publish,
             "published timestamp should be >= before_publish");
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_published) <= after_publish,
             "published timestamp should be <= after_publish");

  /* Cleanup */
  fio_unsubscribe(.channel = channel, .filter = -114);
  fio_queue_perform_all(fio_io_queue());
}

/* *****************************************************************************
Custom ID and Timestamp Testing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pubsub_custom_id)(void) {
  FIO_LOG_DDEBUG("Testing pub/sub custom message ID and timestamp.");
  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"custom_id_channel");
  uint64_t custom_id = 0xDEADBEEF12345678ULL;
  uint64_t custom_time = 1234567890123ULL;

  FIO_NAME_TEST(stl, pubsub_received_id) = 0;
  FIO_NAME_TEST(stl, pubsub_received_published) = 0;

  /* Subscribe */
  fio_subscribe(.channel = channel,
                .on_message = FIO_NAME_TEST(stl, pubsub_id_on_message),
                .filter = -113);

  /* Publish with custom ID and timestamp */
  fio_publish(.engine = FIO_PUBSUB_CLUSTER,
              .channel = channel,
              .id = custom_id,
              .published = custom_time,
              .filter = -113);
  fio_queue_perform_all(fio_io_queue());

  /* Verify custom values */
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_id) == custom_id,
             "custom message ID should be preserved");
  FIO_ASSERT(FIO_NAME_TEST(stl, pubsub_received_published) == custom_time,
             "custom published timestamp should be preserved");

  /* Cleanup */
  fio_unsubscribe(.channel = channel, .filter = -113);
  fio_queue_perform_all(fio_io_queue());
}

/* *****************************************************************************

***************************************************************************** */

int main(void) {
  FIO_LOG_DDEBUG("Testing Pub/Sub module.");
  FIO_NAME_TEST(stl, pubsub_encryption)();
  FIO_NAME_TEST(stl, pubsub_roundtrip)();
  FIO_NAME_TEST(stl, pubsub_channel_isolation)();
  FIO_NAME_TEST(stl, pubsub_filter_isolation)();
  FIO_NAME_TEST(stl, pubsub_no_subscribers)();
  FIO_NAME_TEST(stl, pubsub_empty_channel)();
  FIO_NAME_TEST(stl, pubsub_large_message)();
  FIO_NAME_TEST(stl, pubsub_rapid_cycles)();
  FIO_NAME_TEST(stl, pubsub_subscription_handle)();
  FIO_NAME_TEST(stl, pubsub_engine)();
  FIO_NAME_TEST(stl, pubsub_message_content)();
  FIO_NAME_TEST(stl, pubsub_multiple_subscribers)();
  FIO_NAME_TEST(stl, pubsub_pattern_matching)();
  FIO_NAME_TEST(stl, pubsub_message_id)();
  FIO_NAME_TEST(stl, pubsub_custom_id)();
  FIO_LOG_DDEBUG("Pub/Sub tests complete.");
  fio___io_cleanup_at_exit(NULL);
}
