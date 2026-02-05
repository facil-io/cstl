/* *****************************************************************************
FIO_PUBSUB - Compilation, Unit Tests, and Multi-Process Integration Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_PUBSUB
#define FIO_LOG
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test Helpers
***************************************************************************** */

static volatile int test_message_received = 0;
static volatile int test_unsubscribe_called = 0;

static void test_on_message(fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2("Message received on channel: %.*s",
                 (int)msg->channel.len,
                 msg->channel.buf);
  FIO_LOG_DEBUG2("Message content: %.*s",
                 (int)msg->message.len,
                 msg->message.buf);
  test_message_received = 1;
  (void)msg;
}

static void test_on_unsubscribe(void *udata) {
  FIO_LOG_DEBUG2("Unsubscribe callback called with udata: %p", udata);
  test_unsubscribe_called = 1;
  (void)udata;
}

/* *****************************************************************************
Unit Tests - Type Sizes and Layouts
***************************************************************************** */

FIO_SFUNC void fio___pubsub_test_types(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB type sizes and layouts.\n");

  /* Test message structure */
  fio_pubsub_msg_s msg = {
      .io = NULL,
      .udata = (void *)0x1234,
      .id = 12345,
      .timestamp = 67890,
      .channel = FIO_BUF_INFO1("test-channel"),
      .message = FIO_BUF_INFO1("test-message"),
      .filter = 42,
  };
  FIO_ASSERT(msg.id == 12345, "msg.id should be 12345");
  FIO_ASSERT(msg.filter == 42, "msg.filter should be 42");
  FIO_ASSERT(msg.channel.len == 12, "channel length should be 12");
  FIO_ASSERT(msg.message.len == 12, "message length should be 12");

  /* Test subscribe args structure */
  fio_pubsub_subscribe_args_s sub_args = {
      .io = NULL,
      .channel = FIO_BUF_INFO1("my-channel"),
      .on_message = test_on_message,
      .on_unsubscribe = test_on_unsubscribe,
      .udata = (void *)0x5678,
      .queue = NULL,
      .replay_since = 0,
      .filter = 100,
      .is_pattern = 1,
  };
  FIO_ASSERT(sub_args.filter == 100, "sub_args.filter should be 100");
  FIO_ASSERT(sub_args.is_pattern == 1, "sub_args.is_pattern should be 1");
  FIO_ASSERT(sub_args.on_message == test_on_message,
             "on_message callback should match");

  /* Test publish args structure */
  fio_pubsub_publish_args_s pub_args = {
      .channel = FIO_BUF_INFO1("pub-channel"),
      .message = FIO_BUF_INFO1("pub-message"),
      .from = NULL,
      .engine = NULL,
      .id = 99999,
      .timestamp = 88888,
      .filter = 200,
  };
  FIO_ASSERT(pub_args.id == 99999, "pub_args.id should be 99999");
  FIO_ASSERT(pub_args.filter == 200, "pub_args.filter should be 200");

  fprintf(stderr, "* FIO_PUBSUB type tests passed.\n");
}

/* *****************************************************************************
Unit Tests - Environment Type Helper
***************************************************************************** */

FIO_SFUNC void fio___pubsub_test_env_type(void) {
  fprintf(stderr, "* Testing fio___pubsub_channel_env_type helper.\n");

  /* Test that different filter/pattern combinations produce different keys */
  intptr_t t1 = fio___pubsub_channel_env_type(0, 0);
  intptr_t t2 = fio___pubsub_channel_env_type(0, 1);
  intptr_t t3 = fio___pubsub_channel_env_type(1, 0);
  intptr_t t4 = fio___pubsub_channel_env_type(1, 1);
  intptr_t t5 = fio___pubsub_channel_env_type(-1, 0);

  FIO_ASSERT(t1 != t2, "Different pattern flags should produce different keys");
  FIO_ASSERT(t1 != t3, "Different filters should produce different keys");
  FIO_ASSERT(t1 != t4,
             "Different filter+pattern should produce different keys");
  FIO_ASSERT(t3 != t5, "Negative filter should produce different key");

  /* All should be negative (reserved range) */
  FIO_ASSERT(t1 < 0, "env_type should be negative");
  FIO_ASSERT(t2 < 0, "env_type should be negative");
  FIO_ASSERT(t3 < 0, "env_type should be negative");
  FIO_ASSERT(t4 < 0, "env_type should be negative");
  FIO_ASSERT(t5 < 0, "env_type should be negative");

  fprintf(stderr, "* fio___pubsub_channel_env_type tests passed.\n");
}

/* *****************************************************************************
Unit Tests - History API
***************************************************************************** */

FIO_SFUNC void fio___pubsub_test_history_api(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB history API.\n");

  /* Get built-in cache history manager with default size */
  fio_pubsub_history_s const *cache = fio_pubsub_history_cache(0);
  FIO_ASSERT(cache != NULL, "fio_pubsub_history_cache should return non-NULL");
  FIO_ASSERT(cache->push != NULL, "cache history should have push callback");
  FIO_ASSERT(cache->replay != NULL,
             "cache history should have replay callback");
  FIO_ASSERT(cache->oldest != NULL,
             "cache history should have oldest callback");
  FIO_ASSERT(cache->detached != NULL,
             "cache history should have detached callback");

  /* Get cache history again with specific size - should return same pointer */
  fio_pubsub_history_s const *cache2 = fio_pubsub_history_cache(1024 * 1024);
  FIO_ASSERT(cache2 == cache,
             "fio_pubsub_history_cache should return same manager");

  /* Test default engine getter */
  fio_pubsub_engine_s const *default_eng = fio_pubsub_engine_default();
  FIO_ASSERT(default_eng != NULL, "default engine should not be NULL");

  /* Test IPC engine getter */
  fio_pubsub_engine_s const *ipc_eng = fio_pubsub_engine_ipc();
  FIO_ASSERT(ipc_eng != NULL, "IPC engine should not be NULL");
  FIO_ASSERT(ipc_eng->publish != NULL, "IPC engine should have publish");

  fprintf(stderr, "* FIO_PUBSUB history API tests passed.\n");
}

/* *****************************************************************************
Multi-Process Integration Tests
*****************************************************************************
These tests fork worker processes and test real pub/sub over IPC.
CRITICAL: All assertions must be on master only - worker crashes don't fail
test.
***************************************************************************** */

/* *****************************************************************************
Test 1: Basic Subscribe/Publish in Single Process (Master Only)
***************************************************************************** */

static volatile int fio___test_ps2_basic_received = 0;
static char fio___test_ps2_basic_channel[64];
static char fio___test_ps2_basic_message[64];

FIO_SFUNC void fio___test_ps2_basic_on_message(fio_pubsub_msg_s *msg) {
  fio___test_ps2_basic_received++;
  if (msg->channel.len < 64) {
    FIO_MEMCPY((void *)fio___test_ps2_basic_channel,
               msg->channel.buf,
               msg->channel.len);
    fio___test_ps2_basic_channel[msg->channel.len] = '\0';
  }
  if (msg->message.len < 64) {
    FIO_MEMCPY((void *)fio___test_ps2_basic_message,
               msg->message.buf,
               msg->message.len);
    fio___test_ps2_basic_message[msg->message.len] = '\0';
  }
  FIO_LOG_DEBUG2("(%d) basic_on_message: channel=%.*s message=%.*s",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf,
                 (int)msg->message.len,
                 msg->message.buf);
}

FIO_SFUNC void fio___test_ps2_basic_on_unsub(void *udata) {
  FIO_LOG_DEBUG2("(%d) basic_on_unsub called with udata=%p",
                 fio_io_pid(),
                 udata);
  (void)udata;
}

/* Timer to trigger subscribe after reactor starts */
FIO_SFUNC int fio___test_ps2_basic_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Subscribing to test-channel", fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("test-channel"),
                       .on_message = fio___test_ps2_basic_on_message,
                       .on_unsubscribe = fio___test_ps2_basic_on_unsub);
  return -1; /* One-shot */
}

/* Timer to trigger publish after subscribe */
FIO_SFUNC int fio___test_ps2_basic_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Publishing to test-channel", fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("test-channel"),
                     .message = FIO_BUF_INFO1("hello-world"));
  return -1; /* One-shot */
}

/* Check if message was received and stop */
FIO_SFUNC int fio___test_ps2_basic_check(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Check every 50ms if we received the message */
  if (fio___test_ps2_basic_received > 0) {
    FIO_LOG_DEBUG2("(%d) [Master] Message received, stopping reactor",
                   fio_io_pid());
    fio_io_stop();
    return -1; /* Stop checking */
  }
  return 0; /* Keep checking */
}

/* Timer to stop reactor (timeout) */
FIO_SFUNC int fio___test_ps2_basic_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  FIO_LOG_DEBUG2("(%d) [Master] Timeout - stopping reactor", fio_io_pid());
  fio_io_stop();
  return -1;
}

/* Verify results */
FIO_SFUNC void fio___test_ps2_basic_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  FIO_ASSERT(fio___test_ps2_basic_received >= 1,
             "[Master] Should receive at least 1 message (got %d)",
             fio___test_ps2_basic_received);
  FIO_ASSERT(FIO_STRLEN(fio___test_ps2_basic_channel) == 12 &&
                 FIO_MEMCMP(fio___test_ps2_basic_channel, "test-channel", 12) ==
                     0,
             "[Master] Channel should be 'test-channel' (got '%s')",
             fio___test_ps2_basic_channel);
  FIO_ASSERT(FIO_STRLEN(fio___test_ps2_basic_message) == 11 &&
                 FIO_MEMCMP(fio___test_ps2_basic_message, "hello-world", 11) ==
                     0,
             "[Master] Message should be 'hello-world' (got '%s')",
             fio___test_ps2_basic_message);
  FIO_LOG_DEBUG2("(%d) [Master] Basic test: received=%d channel=%s message=%s",
                 fio_io_pid(),
                 fio___test_ps2_basic_received,
                 fio___test_ps2_basic_channel,
                 fio___test_ps2_basic_message);
}

FIO_SFUNC void fio___pubsub_test_basic(void) {
  fprintf(stderr,
          "* Testing FIO_PUBSUB basic subscribe/publish (master only).\n");

  /* Reset state */
  fio___test_ps2_basic_received = 0;
  FIO_MEMSET((void *)fio___test_ps2_basic_channel,
             0,
             sizeof(fio___test_ps2_basic_channel));
  FIO_MEMSET((void *)fio___test_ps2_basic_message,
             0,
             sizeof(fio___test_ps2_basic_message));

  /* Schedule: subscribe at 50ms, publish at 100ms, check every 50ms, timeout at
   * 2s */
  fio_io_run_every(.fn = fio___test_ps2_basic_subscribe,
                   .every = 50,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_basic_publish,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_basic_check,
                   .every = 50,
                   .repetitions = 0); /* Repeat until stopped */
  fio_io_run_every(.fn = fio___test_ps2_basic_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start reactor with 0 workers (master only) */
  fio_io_start(0);
  /* Test Assertions */
  fio___test_ps2_basic_on_finish(NULL);

  fprintf(stderr, "* FIO_PUBSUB basic test passed.\n");
}

/* *****************************************************************************
Test 2: Worker Subscribes, Master Publishes, Worker Receives
***************************************************************************** */

static volatile int fio___test_ps2_w2m_master_confirms = 0;
static volatile int fio___test_ps2_w2m_worker_received = 0;

/* Master handler for worker confirmation */
FIO_SFUNC void fio___test_ps2_w2m_confirm_handler(fio_ipc_s *ipc) {
  fio___test_ps2_w2m_master_confirms++;
  FIO_LOG_DEBUG2("(%d) [Master] Worker confirmed receipt: %.*s",
                 fio_io_pid(),
                 (int)ipc->len,
                 ipc->data);
}

/* Worker message callback - reports to master */
FIO_SFUNC void fio___test_ps2_w2m_on_message(fio_pubsub_msg_s *msg) {
  fio___test_ps2_w2m_worker_received++;
  FIO_LOG_DEBUG2("(%d) [Worker] Received message: channel=%.*s message=%.*s",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf,
                 (int)msg->message.len,
                 msg->message.buf);

  /* Report to master via IPC */
  const char *confirm = "received";
  fio_ipc_call(.call = fio___test_ps2_w2m_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

/* Worker startup - subscribes to channel */
FIO_SFUNC void fio___test_ps2_w2m_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  FIO_LOG_DEBUG2("(%d) [Worker] Subscribing to worker-channel", fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("worker-channel"),
                       .on_message = fio___test_ps2_w2m_on_message);
}

/* Master publishes after workers connect */
FIO_SFUNC int fio___test_ps2_w2m_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Publishing to worker-channel", fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("worker-channel"),
                     .message = FIO_BUF_INFO1("master-message"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_w2m_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_w2m_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  FIO_ASSERT(
      fio___test_ps2_w2m_master_confirms >= 1,
      "[Master] Should receive at least 1 confirmation from worker (got %d)",
      fio___test_ps2_w2m_master_confirms);
  FIO_LOG_DEBUG2("(%d) [Master] W2M test: confirms=%d",
                 fio_io_pid(),
                 fio___test_ps2_w2m_master_confirms);
}

FIO_SFUNC void fio___pubsub_test_worker_subscribe(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB worker subscribe, master publish.\n");

  /* Reset */
  fio___test_ps2_w2m_master_confirms = 0;
  fio___test_ps2_w2m_worker_received = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_w2m_worker_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP, fio___test_ps2_w2m_on_finish, NULL);

  /* Publish after 200ms (give workers time to subscribe) */
  fio_io_run_every(.fn = fio___test_ps2_w2m_publish,
                   .every = 200,
                   .repetitions = 1);

  /* Timeout after 2 seconds */
  fio_io_run_every(.fn = fio___test_ps2_w2m_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start with 1 worker */
  fio_io_start(1);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_w2m_worker_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_w2m_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB worker subscribe test passed.\n");
}

/* *****************************************************************************
Test 3: Worker Publishes, Master and Other Workers Receive
***************************************************************************** */

static volatile int fio___test_ps2_wpub_master_received = 0;
static volatile int fio___test_ps2_wpub_worker_confirms = 0;

/* Master handler for worker confirmations */
FIO_SFUNC void fio___test_ps2_wpub_confirm_handler(fio_ipc_s *ipc) {
  fio___test_ps2_wpub_worker_confirms++;
  FIO_LOG_DEBUG2("(%d) [Master] Worker confirmed: %.*s",
                 fio_io_pid(),
                 (int)ipc->len,
                 ipc->data);
}

/* Master message callback */
FIO_SFUNC void fio___test_ps2_wpub_master_on_message(fio_pubsub_msg_s *msg) {
  fio___test_ps2_wpub_master_received++;
  FIO_LOG_DEBUG2("(%d) [Master] Received message: channel=%.*s message=%.*s",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf,
                 (int)msg->message.len,
                 msg->message.buf);
}

/* Worker message callback - reports to master */
FIO_SFUNC void fio___test_ps2_wpub_worker_on_message(fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2("(%d) [Worker] Received message: channel=%.*s message=%.*s",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf,
                 (int)msg->message.len,
                 msg->message.buf);

  /* Report to master */
  const char *confirm = "worker_received";
  fio_ipc_call(.call = fio___test_ps2_wpub_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

/* Worker startup - subscribes to channel (same pattern as Test 2) */
FIO_SFUNC void fio___test_ps2_wpub_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  FIO_LOG_DEBUG2("(%d) [Worker] Subscribing to pub-channel", fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("pub-channel"),
                       .on_message = fio___test_ps2_wpub_worker_on_message);
}

/* Master subscribes via timer (50ms after start) */
FIO_SFUNC int fio___test_ps2_wpub_master_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  FIO_LOG_DEBUG2("(%d) [Master] Subscribing to pub-channel", fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("pub-channel"),
                       .on_message = fio___test_ps2_wpub_master_on_message);
  return -1;
}

/* First worker publishes */
static volatile int fio___test_ps2_wpub_published = 0;
FIO_SFUNC int fio___test_ps2_wpub_worker_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_worker())
    return -1;

  /* Only one worker should publish */
  if (fio___test_ps2_wpub_published)
    return -1;
  fio___test_ps2_wpub_published = 1;

  FIO_LOG_DEBUG2("(%d) [Worker] Publishing to pub-channel", fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("pub-channel"),
                     .message = FIO_BUF_INFO1("worker-message"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_wpub_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_wpub_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* Master should receive the message */
  FIO_ASSERT(fio___test_ps2_wpub_master_received >= 1,
             "[Master] Should receive at least 1 message (got %d)",
             fio___test_ps2_wpub_master_received);

  /* Other worker should also receive (2 workers total, 1 publishes, 1 receives)
   */
  /* Note: The publishing worker also receives its own message via local
   * delivery */
  FIO_ASSERT(fio___test_ps2_wpub_worker_confirms >= 1,
             "[Master] Should receive worker confirmation (got %d)",
             fio___test_ps2_wpub_worker_confirms);

  FIO_LOG_DEBUG2(
      "(%d) [Master] WPUB test: master_received=%d worker_confirms=%d",
      fio_io_pid(),
      fio___test_ps2_wpub_master_received,
      fio___test_ps2_wpub_worker_confirms);
}

FIO_SFUNC void fio___pubsub_test_worker_publish(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB worker publishes, master receives.\n");

  /* Reset */
  fio___test_ps2_wpub_master_received = 0;
  fio___test_ps2_wpub_worker_confirms = 0;
  fio___test_ps2_wpub_published = 0;

  /* Register callbacks - workers subscribe via FIO_CALL_ON_START (same as Test
   * 2) */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_wpub_worker_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP, fio___test_ps2_wpub_on_finish, NULL);

  /* Master subscribes at 50ms (give workers time to subscribe first) */
  fio_io_run_every(.fn = fio___test_ps2_wpub_master_subscribe,
                   .every = 50,
                   .repetitions = 1);

  /* Worker publishes at 200ms (gives time for subscriptions to be active) */
  fio_io_run_every(.fn = fio___test_ps2_wpub_worker_publish,
                   .every = 200,
                   .repetitions = 1);

  /* Timeout at 2 seconds */
  fio_io_run_every(.fn = fio___test_ps2_wpub_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start with 2 workers */
  fio_io_start(2);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_wpub_worker_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_wpub_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB worker publish test passed.\n");
}

/* *****************************************************************************
Test 4: Pattern Matching (Glob)
***************************************************************************** */

static volatile int fio___test_ps2_pattern_received = 0;
static char fio___test_ps2_pattern_channels[4][64];

/* Message callback for pattern subscription */
FIO_SFUNC void fio___test_ps2_pattern_on_message(fio_pubsub_msg_s *msg) {
  int idx = fio___test_ps2_pattern_received++;
  if (idx < 4 && msg->channel.len < 64) {
    FIO_MEMCPY((void *)fio___test_ps2_pattern_channels[idx],
               msg->channel.buf,
               msg->channel.len);
    fio___test_ps2_pattern_channels[idx][msg->channel.len] = '\0';
  }
  FIO_LOG_DEBUG2("(%d) Pattern received: channel=%.*s",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf);
}

/* Subscribe with pattern */
FIO_SFUNC int fio___test_ps2_pattern_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Subscribing to pattern: news/*", fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("news/*"),
                       .on_message = fio___test_ps2_pattern_on_message,
                       .is_pattern = 1);
  return -1;
}

/* Publish to various channels */
FIO_SFUNC int fio___test_ps2_pattern_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2(
      "(%d) [Master] Publishing to news/sports, news/tech, weather/rain",
      fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("news/sports"),
                     .message = FIO_BUF_INFO1("sports-news"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("news/tech"),
                     .message = FIO_BUF_INFO1("tech-news"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("weather/rain"),
                     .message =
                         FIO_BUF_INFO1("rain-forecast")); /* Should NOT match */
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_pattern_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_pattern_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* Should receive exactly 2 messages (news/sports and news/tech) */
  FIO_ASSERT(fio___test_ps2_pattern_received == 2,
             "[Master] Pattern should match exactly 2 messages (got %d)",
             fio___test_ps2_pattern_received);

  FIO_LOG_DEBUG2("(%d) [Master] Pattern test: received=%d",
                 fio_io_pid(),
                 fio___test_ps2_pattern_received);
}

FIO_SFUNC void fio___pubsub_test_pattern(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB pattern matching (glob).\n");

  /* Reset */
  fio___test_ps2_pattern_received = 0;
  FIO_MEMSET((void *)fio___test_ps2_pattern_channels,
             0,
             sizeof(fio___test_ps2_pattern_channels));

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___test_ps2_pattern_on_finish,
                         NULL);

  /* Subscribe at 50ms, publish at 100ms, stop at 500ms */
  fio_io_run_every(.fn = fio___test_ps2_pattern_subscribe,
                   .every = 50,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_pattern_publish,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_pattern_timeout,
                   .every = 500,
                   .repetitions = 1);

  /* Master only */
  fio_io_start(0);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_pattern_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB pattern test passed.\n");
}

/* *****************************************************************************
Test 5: Filter Namespace Separation
***************************************************************************** */

static volatile int fio___test_ps2_filter_received_0 = 0;
static volatile int fio___test_ps2_filter_received_1 = 0;

/* Message callback for filter 0 */
FIO_SFUNC void fio___test_ps2_filter0_on_message(fio_pubsub_msg_s *msg) {
  fio___test_ps2_filter_received_0++;
  FIO_LOG_DEBUG2("(%d) Filter 0 received: channel=%.*s filter=%d",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf,
                 (int)msg->filter);
}

/* Message callback for filter 1 */
FIO_SFUNC void fio___test_ps2_filter1_on_message(fio_pubsub_msg_s *msg) {
  fio___test_ps2_filter_received_1++;
  FIO_LOG_DEBUG2("(%d) Filter 1 received: channel=%.*s filter=%d",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf,
                 (int)msg->filter);
}

/* Subscribe to same channel with different filters */
FIO_SFUNC int fio___test_ps2_filter_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2(
      "(%d) [Master] Subscribing to 'data' with filter=0 and filter=1",
      fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("data"),
                       .on_message = fio___test_ps2_filter0_on_message,
                       .filter = 0);
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("data"),
                       .on_message = fio___test_ps2_filter1_on_message,
                       .filter = 1);
  return -1;
}

/* Publish to filter 0 only */
FIO_SFUNC int fio___test_ps2_filter_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Publishing to 'data' with filter=0",
                 fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("data"),
                     .message = FIO_BUF_INFO1("filter0-message"),
                     .filter = 0);
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_filter_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_filter_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* Only filter 0 subscription should receive */
  FIO_ASSERT(fio___test_ps2_filter_received_0 == 1,
             "[Master] Filter 0 should receive exactly 1 message (got %d)",
             fio___test_ps2_filter_received_0);
  FIO_ASSERT(fio___test_ps2_filter_received_1 == 0,
             "[Master] Filter 1 should receive 0 messages (got %d)",
             fio___test_ps2_filter_received_1);

  FIO_LOG_DEBUG2("(%d) [Master] Filter test: filter0=%d filter1=%d",
                 fio_io_pid(),
                 fio___test_ps2_filter_received_0,
                 fio___test_ps2_filter_received_1);
}

FIO_SFUNC void fio___pubsub_test_filter(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB filter namespace separation.\n");

  /* Reset */
  fio___test_ps2_filter_received_0 = 0;
  fio___test_ps2_filter_received_1 = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___test_ps2_filter_on_finish,
                         NULL);

  /* Subscribe at 50ms, publish at 100ms, stop at 500ms */
  fio_io_run_every(.fn = fio___test_ps2_filter_subscribe,
                   .every = 50,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_filter_publish,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_filter_timeout,
                   .every = 500,
                   .repetitions = 1);

  /* Master only */
  fio_io_start(0);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_filter_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB filter test passed.\n");
}

/* *****************************************************************************
Test 6: Unsubscribe
***************************************************************************** */

static volatile int fio___test_ps2_unsub_received = 0;
static volatile int fio___test_ps2_unsub_callback_called = 0;

/* Message callback */
FIO_SFUNC void fio___test_ps2_unsub_on_message(fio_pubsub_msg_s *msg) {
  fio___test_ps2_unsub_received++;
  FIO_LOG_DEBUG2("(%d) Unsub test received message", fio_io_pid());
  (void)msg;
}

/* Unsubscribe callback */
FIO_SFUNC void fio___test_ps2_unsub_on_unsub(void *udata) {
  fio___test_ps2_unsub_callback_called++;
  FIO_LOG_DEBUG2("(%d) Unsubscribe callback called", fio_io_pid());
  (void)udata;
}

/* Subscribe */
FIO_SFUNC int fio___test_ps2_unsub_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Subscribing to unsub-channel", fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("unsub-channel"),
                       .on_message = fio___test_ps2_unsub_on_message,
                       .on_unsubscribe = fio___test_ps2_unsub_on_unsub);
  return -1;
}

/* Unsubscribe */
FIO_SFUNC int fio___test_ps2_unsub_unsubscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Unsubscribing from unsub-channel",
                 fio_io_pid());
  int r = fio_pubsub_unsubscribe(.channel = FIO_BUF_INFO1("unsub-channel"));
  FIO_LOG_DEBUG2("(%d) [Master] Unsubscribe returned: %d", fio_io_pid(), r);
  return -1;
}

/* Publish after unsubscribe */
FIO_SFUNC int fio___test_ps2_unsub_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2(
      "(%d) [Master] Publishing to unsub-channel (should not be received)",
      fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("unsub-channel"),
                     .message = FIO_BUF_INFO1("after-unsub"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_unsub_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_unsub_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* Should not receive any messages (unsubscribed before publish) */
  FIO_ASSERT(fio___test_ps2_unsub_received == 0,
             "[Master] Should not receive messages after unsubscribe (got %d)",
             fio___test_ps2_unsub_received);

  /* Unsubscribe callback should have been called */
  FIO_ASSERT(fio___test_ps2_unsub_callback_called == 1,
             "[Master] Unsubscribe callback should be called once (got %d)",
             fio___test_ps2_unsub_callback_called);

  FIO_LOG_DEBUG2("(%d) [Master] Unsub test: received=%d callback=%d",
                 fio_io_pid(),
                 fio___test_ps2_unsub_received,
                 fio___test_ps2_unsub_callback_called);
}

/* Subscribe on startup (runs in IO thread context) */
FIO_SFUNC void fio___test_ps2_unsub_on_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  FIO_LOG_DEBUG2("(%d) [Master] Subscribing to unsub-channel (on_start)",
                 fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("unsub-channel"),
                       .on_message = fio___test_ps2_unsub_on_message,
                       .on_unsubscribe = fio___test_ps2_unsub_on_unsub);
}

FIO_SFUNC void fio___pubsub_test_unsubscribe(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB unsubscribe.\n");

  /* Reset */
  fio___test_ps2_unsub_received = 0;
  fio___test_ps2_unsub_callback_called = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_unsub_on_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___test_ps2_unsub_on_finish,
                         NULL);

  /* Subscribe happens on startup. Then:
   * unsubscribe at 100ms, publish at 200ms, stop at 500ms */
  fio_io_run_every(.fn = fio___test_ps2_unsub_unsubscribe,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_unsub_publish,
                   .every = 200,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_unsub_timeout,
                   .every = 500,
                   .repetitions = 1);

  /* Master only */
  fio_io_start(0);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_unsub_on_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_unsub_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB unsubscribe test passed.\n");
}

/* *****************************************************************************
Test 7: Broadcast to Multiple Workers
***************************************************************************** */

static volatile int fio___test_ps2_bcast_master_confirms = 0;

/* Master handler for worker confirmations */
FIO_SFUNC void fio___test_ps2_bcast_confirm_handler(fio_ipc_s *ipc) {
  fio___test_ps2_bcast_master_confirms++;
  FIO_LOG_DEBUG2("(%d) [Master] Broadcast confirm from worker: %.*s",
                 fio_io_pid(),
                 (int)ipc->len,
                 ipc->data);
}

/* Worker message callback */
FIO_SFUNC void fio___test_ps2_bcast_on_message(fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2("(%d) [Worker] Broadcast received: channel=%.*s",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf);

  /* Confirm to master */
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "worker_%d_received", fio_io_pid());
  fio_ipc_call(.call = fio___test_ps2_bcast_confirm_handler,
               .data = FIO_IPC_DATA(FIO_BUF_INFO2(buf, (size_t)len)));
}

/* Worker startup */
FIO_SFUNC void fio___test_ps2_bcast_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  FIO_LOG_DEBUG2("(%d) [Worker] Subscribing to broadcast-channel",
                 fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("broadcast-channel"),
                       .on_message = fio___test_ps2_bcast_on_message);
}

/* Master publishes */
FIO_SFUNC int fio___test_ps2_bcast_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Publishing to broadcast-channel", fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("broadcast-channel"),
                     .message = FIO_BUF_INFO1("broadcast-message"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_bcast_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_bcast_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* All 3 workers should confirm receipt */
  FIO_ASSERT(fio___test_ps2_bcast_master_confirms >= 3,
             "[Master] Should receive 3 confirmations (got %d)",
             fio___test_ps2_bcast_master_confirms);

  FIO_LOG_DEBUG2("(%d) [Master] Broadcast test: confirms=%d",
                 fio_io_pid(),
                 fio___test_ps2_bcast_master_confirms);
}

FIO_SFUNC void fio___pubsub_test_broadcast(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB broadcast to multiple workers.\n");

  /* Reset */
  fio___test_ps2_bcast_master_confirms = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_bcast_worker_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___test_ps2_bcast_on_finish,
                         NULL);

  /* Publish after 300ms (give workers time to subscribe and IPC to complete) */
  fio_io_run_every(.fn = fio___test_ps2_bcast_publish,
                   .every = 300,
                   .repetitions = 1);

  /* Timeout at 2 seconds */
  fio_io_run_every(.fn = fio___test_ps2_bcast_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start with 3 workers */
  fio_io_start(3);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_bcast_worker_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_bcast_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB broadcast test passed.\n");
}

/* *****************************************************************************
Test 8: Message Data Integrity (Large Messages)
***************************************************************************** */

static volatile int fio___test_ps2_data_verified = 0;

/* Message callback - verifies data integrity */
FIO_SFUNC void fio___test_ps2_data_on_message(fio_pubsub_msg_s *msg) {
  /* Verify pattern: each byte = (index & 0xFF) */
  int valid = 1;
  for (size_t i = 0; i < msg->message.len && valid; ++i) {
    if ((uint8_t)msg->message.buf[i] != (uint8_t)(i & 0xFF))
      valid = 0;
  }

  if (valid && msg->message.len == 65536) {
    fio___test_ps2_data_verified = 1;
    FIO_LOG_DEBUG2("(%d) Data integrity verified for 64KB message",
                   fio_io_pid());
  } else {
    FIO_LOG_ERROR("(%d) Data integrity FAILED! len=%zu valid=%d",
                  fio_io_pid(),
                  msg->message.len,
                  valid);
  }
}

/* Subscribe */
FIO_SFUNC int fio___test_ps2_data_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("data-channel"),
                       .on_message = fio___test_ps2_data_on_message);
  return -1;
}

/* Publish large message */
FIO_SFUNC int fio___test_ps2_data_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Create 64KB message with pattern */
  size_t size = 65536;
  char *data = (char *)FIO_MEM_REALLOC(NULL, 0, size, 0);
  if (!data)
    return -1;

  for (size_t i = 0; i < size; ++i) {
    data[i] = (char)(i & 0xFF);
  }

  FIO_LOG_DEBUG2("(%d) [Master] Publishing 64KB message", fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("data-channel"),
                     .message = FIO_BUF_INFO2(data, size));

  FIO_MEM_FREE(data, size);
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_data_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_data_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  FIO_ASSERT(fio___test_ps2_data_verified == 1,
             "[Master] 64KB message data integrity should be verified");

  FIO_LOG_DEBUG2("(%d) [Master] Data integrity test: verified=%d",
                 fio_io_pid(),
                 fio___test_ps2_data_verified);
}

FIO_SFUNC void fio___pubsub_test_data_integrity(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB data integrity (64KB message).\n");

  /* Reset */
  fio___test_ps2_data_verified = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_STOP, fio___test_ps2_data_on_finish, NULL);

  /* Subscribe at 50ms, publish at 100ms, stop at 1 second */
  fio_io_run_every(.fn = fio___test_ps2_data_subscribe,
                   .every = 50,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_data_publish,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_data_timeout,
                   .every = 1000,
                   .repetitions = 1);

  /* Master only */
  fio_io_start(0);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_data_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB data integrity test passed.\n");
}

/* *****************************************************************************
Test 9: Worker History Replay via IPC
*****************************************************************************
Master publishes messages to history, then worker subscribes with replay_since.
Worker should receive historical messages via IPC from master.
***************************************************************************** */

static volatile int fio___test_ps2_hist_master_confirms = 0;
static volatile int fio___test_ps2_hist_messages_published = 0;

/* Master handler for worker confirmation */
FIO_SFUNC void fio___test_ps2_hist_confirm_handler(fio_ipc_s *ipc) {
  fio___test_ps2_hist_master_confirms++;
  FIO_LOG_DEBUG2("(%d) [Master] Worker confirmed history receipt: count=%d",
                 fio_io_pid(),
                 fio___test_ps2_hist_master_confirms);
  (void)ipc;
}

/* Worker message callback - reports to master */
FIO_SFUNC void fio___test_ps2_hist_on_message(fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2(
      "(%d) [Worker] Received history message: channel=%.*s msg=%.*s",
      fio_io_pid(),
      (int)msg->channel.len,
      msg->channel.buf,
      (int)msg->message.len,
      msg->message.buf);

  /* Report each message to master via IPC */
  const char *confirm = "history_received";
  fio_ipc_call(.call = fio___test_ps2_hist_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

/* Master publishes messages to history */
FIO_SFUNC int fio___test_ps2_hist_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Publish 3 messages to history */
  FIO_LOG_DEBUG2("(%d) [Master] Publishing 3 messages to history-channel",
                 fio_io_pid());
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("history-channel"),
                     .message = FIO_BUF_INFO1("history-msg-1"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("history-channel"),
                     .message = FIO_BUF_INFO1("history-msg-2"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("history-channel"),
                     .message = FIO_BUF_INFO1("history-msg-3"));
  fio___test_ps2_hist_messages_published = 3;
  return -1;
}

/* Worker subscribes with replay_since - called from timer after worker starts
 */
FIO_SFUNC int fio___test_ps2_hist_worker_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_worker())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Worker] Subscribing to history-channel with replay",
                 fio_io_pid());
  /* Subscribe with replay_since = 1 (replay all history) */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("history-channel"),
                       .on_message = fio___test_ps2_hist_on_message,
                       .replay_since = 1);
  return -1;
}

/* Worker startup - schedules subscription after a delay to ensure master has
 * published */
FIO_SFUNC void fio___test_ps2_hist_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  /* Schedule subscription 200ms after worker starts - this ensures:
   * 1. Worker is fully initialized
   * 2. Master has had time to publish (master publishes at 100ms from start)
   */
  fio_io_run_every(.fn = fio___test_ps2_hist_worker_subscribe,
                   .every = 200,
                   .repetitions = 1);
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_hist_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_hist_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* Worker should have received all 3 historical messages */
  FIO_ASSERT(
      fio___test_ps2_hist_master_confirms >= 3,
      "[Master] Worker should receive at least 3 history messages (got %d)",
      fio___test_ps2_hist_master_confirms);

  FIO_LOG_DEBUG2("(%d) [Master] History replay test: confirms=%d",
                 fio_io_pid(),
                 fio___test_ps2_hist_master_confirms);
}

FIO_SFUNC void fio___pubsub_test_worker_history_replay(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB worker history replay via IPC.\n");

  /* Reset */
  fio___test_ps2_hist_master_confirms = 0;
  fio___test_ps2_hist_messages_published = 0;

  /* Attach history manager BEFORE starting reactor - this ensures it's ready
   * before any workers start or timers fire */
  fio_pubsub_history_attach((fio_pubsub_history_s *)fio_pubsub_history_cache(0),
                            100);

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_hist_worker_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP, fio___test_ps2_hist_on_finish, NULL);

  /* Timeline:
   * BEFORE fio_io_start: History manager attached
   * 100ms: Master publishes 3 messages to history
   * ON_START (worker): Worker schedules subscription for 200ms later
   * ~200ms after worker start: Worker subscribes with replay_since
   * 2000ms: Timeout
   */
  fio_io_run_every(.fn = fio___test_ps2_hist_publish,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_hist_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start with 1 worker */
  fio_io_start(1);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_hist_worker_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_hist_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB worker history replay test passed.\n");
}

/* *****************************************************************************
Test 10: Cluster Engine Unit Tests
*****************************************************************************
Tests that the cluster engine getter returns a valid engine with proper
callbacks.
***************************************************************************** */

FIO_SFUNC void fio___pubsub_test_cluster_engine_unit(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB cluster engine unit tests.\n");

  /* Test cluster engine getter */
  fio_pubsub_engine_s const *cluster_eng = fio_pubsub_engine_cluster();
  FIO_ASSERT(cluster_eng != NULL,
             "fio_pubsub_engine_cluster() should return "
             "non-NULL");
  FIO_ASSERT(cluster_eng->publish != NULL,
             "cluster engine should have publish callback");
  FIO_ASSERT(cluster_eng->subscribe != NULL,
             "cluster engine should have subscribe callback");
  FIO_ASSERT(cluster_eng->psubscribe != NULL,
             "cluster engine should have psubscribe callback");
  FIO_ASSERT(cluster_eng->unsubscribe != NULL,
             "cluster engine should have unsubscribe callback");
  FIO_ASSERT(cluster_eng->punsubscribe != NULL,
             "cluster engine should have punsubscribe callback");
  FIO_ASSERT(cluster_eng->detached != NULL,
             "cluster engine should have detached callback");

  /* Test IPC engine getter for comparison */
  fio_pubsub_engine_s const *ipc_eng = fio_pubsub_engine_ipc();
  FIO_ASSERT(ipc_eng != NULL, "fio_pubsub_engine_ipc() should return non-NULL");
  FIO_ASSERT(ipc_eng->publish != NULL,
             "IPC engine should have publish callback");

  /* Cluster and IPC engines should be different */
  FIO_ASSERT(cluster_eng != ipc_eng,
             "cluster and IPC engines should be different pointers");
  FIO_ASSERT(cluster_eng->publish != ipc_eng->publish,
             "cluster and IPC engines should have different publish callbacks");

  /* Test default engine getter */
  fio_pubsub_engine_s const *default_eng = fio_pubsub_engine_default();
  FIO_ASSERT(default_eng != NULL, "default engine should not be NULL");
  /* Default should be IPC when not in cluster mode */
  FIO_ASSERT(default_eng == ipc_eng,
             "default engine should be IPC when not in cluster mode");

  /* Test setting default engine */
  fio_pubsub_engine_s const *old_default =
      fio_pubsub_engine_default_set(cluster_eng);
  FIO_ASSERT(old_default == cluster_eng,
             "engine_default_set should return the engine that was set");
  FIO_ASSERT(fio_pubsub_engine_default() == cluster_eng,
             "default engine should now be cluster engine");

  /* Restore default to IPC */
  fio_pubsub_engine_default_set(ipc_eng);
  FIO_ASSERT(fio_pubsub_engine_default() == ipc_eng,
             "default engine should be restored to IPC");

  /* Test setting NULL restores to IPC */
  fio_pubsub_engine_default_set(cluster_eng);
  fio_pubsub_engine_default_set(NULL);
  FIO_ASSERT(fio_pubsub_engine_default() == ipc_eng,
             "setting NULL should restore default to IPC engine");

  fprintf(stderr, "* FIO_PUBSUB cluster engine unit tests passed.\n");
}

/* *****************************************************************************
Test 11: Cluster Engine Explicit Publish
*****************************************************************************
Tests publishing with the cluster engine explicitly specified.
Uses workers to verify message delivery via opcode-based routing.
***************************************************************************** */

static volatile int fio___test_ps2_cluster_master_confirms = 0;
static volatile int fio___test_ps2_cluster_worker_received = 0;

/* Master handler for worker confirmation */
FIO_SFUNC void fio___test_ps2_cluster_confirm_handler(fio_ipc_s *ipc) {
  fio___test_ps2_cluster_master_confirms++;
  FIO_LOG_DEBUG2("(%d) [Master] Cluster test: worker confirmed receipt: %.*s",
                 fio_io_pid(),
                 (int)ipc->len,
                 ipc->data);
}

/* Worker message callback - reports to master */
FIO_SFUNC void fio___test_ps2_cluster_on_message(fio_pubsub_msg_s *msg) {
  fio___test_ps2_cluster_worker_received++;
  FIO_LOG_DEBUG2(
      "(%d) [Worker] Cluster test: received message: channel=%.*s message=%.*s",
      fio_io_pid(),
      (int)msg->channel.len,
      msg->channel.buf,
      (int)msg->message.len,
      msg->message.buf);

  /* Report to master via IPC */
  const char *confirm = "cluster_received";
  fio_ipc_call(.call = fio___test_ps2_cluster_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

/* Worker startup - subscribes to channel */
FIO_SFUNC void fio___test_ps2_cluster_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  FIO_LOG_DEBUG2("(%d) [Worker] Subscribing to cluster-test-channel",
                 fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("cluster-test-channel"),
                       .on_message = fio___test_ps2_cluster_on_message);
}

/* Master publishes using cluster engine explicitly */
FIO_SFUNC int fio___test_ps2_cluster_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2(
      "(%d) [Master] Publishing to cluster-test-channel via cluster engine",
      fio_io_pid());
  /* Explicitly use cluster engine */
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1("cluster-test-channel"),
                     .message = FIO_BUF_INFO1("cluster-message"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_cluster_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_cluster_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  FIO_ASSERT(fio___test_ps2_cluster_master_confirms >= 1,
             "[Master] Cluster test: should receive at least 1 confirmation "
             "from worker (got %d)",
             fio___test_ps2_cluster_master_confirms);
  FIO_LOG_DEBUG2("(%d) [Master] Cluster engine test: confirms=%d",
                 fio_io_pid(),
                 fio___test_ps2_cluster_master_confirms);
}

FIO_SFUNC void fio___pubsub_test_cluster_engine_publish(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB cluster engine explicit publish.\n");

  /* Reset */
  fio___test_ps2_cluster_master_confirms = 0;
  fio___test_ps2_cluster_worker_received = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_cluster_worker_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___test_ps2_cluster_on_finish,
                         NULL);

  /* Publish after 200ms (give workers time to subscribe) */
  fio_io_run_every(.fn = fio___test_ps2_cluster_publish,
                   .every = 200,
                   .repetitions = 1);

  /* Timeout after 2 seconds */
  fio_io_run_every(.fn = fio___test_ps2_cluster_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start with 1 worker */
  fio_io_start(1);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_cluster_worker_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_cluster_on_finish,
                            NULL);

  fprintf(stderr,
          "* FIO_PUBSUB cluster engine explicit publish test passed.\n");
}

/* *****************************************************************************
Test 12: Cluster Engine Broadcast to Multiple Workers
*****************************************************************************
Tests that cluster engine broadcasts messages to all workers correctly.
Similar to Test 7 but explicitly uses cluster engine.
***************************************************************************** */

static volatile int fio___test_ps2_cluster_bcast_confirms = 0;

/* Master handler for worker confirmations */
FIO_SFUNC void fio___test_ps2_cluster_bcast_confirm_handler(fio_ipc_s *ipc) {
  fio___test_ps2_cluster_bcast_confirms++;
  FIO_LOG_DEBUG2("(%d) [Master] Cluster broadcast confirm from worker: %.*s",
                 fio_io_pid(),
                 (int)ipc->len,
                 ipc->data);
}

/* Worker message callback */
FIO_SFUNC void fio___test_ps2_cluster_bcast_on_message(fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2("(%d) [Worker] Cluster broadcast received: channel=%.*s",
                 fio_io_pid(),
                 (int)msg->channel.len,
                 msg->channel.buf);

  /* Confirm to master */
  char buf[64];
  int len =
      snprintf(buf, sizeof(buf), "cluster_worker_%d_received", fio_io_pid());
  fio_ipc_call(.call = fio___test_ps2_cluster_bcast_confirm_handler,
               .data = FIO_IPC_DATA(FIO_BUF_INFO2(buf, (size_t)len)));
}

/* Worker startup */
FIO_SFUNC void fio___test_ps2_cluster_bcast_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  FIO_LOG_DEBUG2("(%d) [Worker] Subscribing to cluster-broadcast-channel",
                 fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("cluster-broadcast-channel"),
                       .on_message = fio___test_ps2_cluster_bcast_on_message);
}

/* Master publishes using cluster engine */
FIO_SFUNC int fio___test_ps2_cluster_bcast_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Broadcasting to cluster-broadcast-channel via "
                 "cluster engine",
                 fio_io_pid());
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1("cluster-broadcast-channel"),
                     .message = FIO_BUF_INFO1("cluster-broadcast-message"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_cluster_bcast_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_cluster_bcast_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* All 3 workers should confirm receipt */
  FIO_ASSERT(fio___test_ps2_cluster_bcast_confirms >= 3,
             "[Master] Cluster broadcast: should receive 3 confirmations (got "
             "%d)",
             fio___test_ps2_cluster_bcast_confirms);

  FIO_LOG_DEBUG2("(%d) [Master] Cluster broadcast test: confirms=%d",
                 fio_io_pid(),
                 fio___test_ps2_cluster_bcast_confirms);
}

FIO_SFUNC void fio___pubsub_test_cluster_engine_broadcast(void) {
  fprintf(stderr,
          "* Testing FIO_PUBSUB cluster engine broadcast to multiple "
          "workers.\n");

  /* Reset */
  fio___test_ps2_cluster_bcast_confirms = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_cluster_bcast_worker_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___test_ps2_cluster_bcast_on_finish,
                         NULL);

  /* Publish after 300ms (give workers time to subscribe and IPC to complete) */
  fio_io_run_every(.fn = fio___test_ps2_cluster_bcast_publish,
                   .every = 300,
                   .repetitions = 1);

  /* Timeout at 2 seconds */
  fio_io_run_every(.fn = fio___test_ps2_cluster_bcast_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start with 3 workers */
  fio_io_start(3);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_cluster_bcast_worker_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_cluster_bcast_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB cluster engine broadcast test passed.\n");
}

/* *****************************************************************************
Test 13: Cluster vs IPC Engine Comparison
*****************************************************************************
Tests that both cluster and IPC engines deliver messages correctly.
Publishes with both engines and verifies both are received.
***************************************************************************** */

static volatile int fio___test_ps2_cmp_ipc_received = 0;
static volatile int fio___test_ps2_cmp_cluster_received = 0;

/* Message callback - distinguishes by message content */
FIO_SFUNC void fio___test_ps2_cmp_on_message(fio_pubsub_msg_s *msg) {
  if (msg->message.len >= 3 && FIO_MEMCMP(msg->message.buf, "ipc", 3) == 0) {
    fio___test_ps2_cmp_ipc_received++;
    FIO_LOG_DEBUG2("(%d) Comparison test: IPC message received", fio_io_pid());
  } else if (msg->message.len >= 7 &&
             FIO_MEMCMP(msg->message.buf, "cluster", 7) == 0) {
    fio___test_ps2_cmp_cluster_received++;
    FIO_LOG_DEBUG2("(%d) Comparison test: Cluster message received",
                   fio_io_pid());
  }
}

/* Subscribe */
FIO_SFUNC int fio___test_ps2_cmp_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Subscribing to comparison-channel",
                 fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("comparison-channel"),
                       .on_message = fio___test_ps2_cmp_on_message);
  return -1;
}

/* Publish with both engines */
FIO_SFUNC int fio___test_ps2_cmp_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  FIO_LOG_DEBUG2("(%d) [Master] Publishing with IPC engine", fio_io_pid());
  fio_pubsub_publish(.engine = fio_pubsub_engine_ipc(),
                     .channel = FIO_BUF_INFO1("comparison-channel"),
                     .message = FIO_BUF_INFO1("ipc-message"));

  FIO_LOG_DEBUG2("(%d) [Master] Publishing with cluster engine", fio_io_pid());
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1("comparison-channel"),
                     .message = FIO_BUF_INFO1("cluster-message"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_cmp_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_cmp_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* Both engines should deliver messages */
  FIO_ASSERT(fio___test_ps2_cmp_ipc_received >= 1,
             "[Master] IPC engine should deliver message (got %d)",
             fio___test_ps2_cmp_ipc_received);
  FIO_ASSERT(fio___test_ps2_cmp_cluster_received >= 1,
             "[Master] Cluster engine should deliver message (got %d)",
             fio___test_ps2_cmp_cluster_received);

  FIO_LOG_DEBUG2("(%d) [Master] Engine comparison test: ipc=%d cluster=%d",
                 fio_io_pid(),
                 fio___test_ps2_cmp_ipc_received,
                 fio___test_ps2_cmp_cluster_received);
}

FIO_SFUNC void fio___pubsub_test_engine_comparison(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB cluster vs IPC engine comparison.\n");

  /* Reset */
  fio___test_ps2_cmp_ipc_received = 0;
  fio___test_ps2_cmp_cluster_received = 0;

  /* Register callbacks */
  fio_state_callback_add(FIO_CALL_ON_STOP, fio___test_ps2_cmp_on_finish, NULL);

  /* Subscribe at 50ms, publish at 100ms, stop at 500ms */
  fio_io_run_every(.fn = fio___test_ps2_cmp_subscribe,
                   .every = 50,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_cmp_publish,
                   .every = 100,
                   .repetitions = 1);
  fio_io_run_every(.fn = fio___test_ps2_cmp_timeout,
                   .every = 500,
                   .repetitions = 1);

  /* Master only */
  fio_io_start(0);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_cmp_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB engine comparison test passed.\n");
}

/* *****************************************************************************
Test 14: Worker Publishes via Cluster Engine
*****************************************************************************
Tests that workers can publish using the cluster engine and messages
are delivered to master and other workers.
***************************************************************************** */

static volatile int fio___test_ps2_wcluster_master_received = 0;
static volatile int fio___test_ps2_wcluster_worker_confirms = 0;

/* Master handler for worker confirmations */
FIO_SFUNC void fio___test_ps2_wcluster_confirm_handler(fio_ipc_s *ipc) {
  fio___test_ps2_wcluster_worker_confirms++;
  FIO_LOG_DEBUG2("(%d) [Master] Worker cluster publish confirmed: %.*s",
                 fio_io_pid(),
                 (int)ipc->len,
                 ipc->data);
}

/* Master message callback */
FIO_SFUNC void fio___test_ps2_wcluster_master_on_message(
    fio_pubsub_msg_s *msg) {
  fio___test_ps2_wcluster_master_received++;
  FIO_LOG_DEBUG2(
      "(%d) [Master] Worker cluster publish: received message: channel=%.*s",
      fio_io_pid(),
      (int)msg->channel.len,
      msg->channel.buf);
}

/* Worker message callback - reports to master */
FIO_SFUNC void fio___test_ps2_wcluster_worker_on_message(
    fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2(
      "(%d) [Worker] Worker cluster publish: received message: channel=%.*s",
      fio_io_pid(),
      (int)msg->channel.len,
      msg->channel.buf);

  /* Report to master */
  const char *confirm = "worker_cluster_received";
  fio_ipc_call(.call = fio___test_ps2_wcluster_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

/* Worker startup - subscribes to channel (same pattern as Test 3) */
FIO_SFUNC void fio___test_ps2_wcluster_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  FIO_LOG_DEBUG2("(%d) [Worker] Subscribing to worker-cluster-channel",
                 fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("worker-cluster-channel"),
                       .on_message = fio___test_ps2_wcluster_worker_on_message);
}

/* Master subscribes via timer (50ms after start) */
FIO_SFUNC int fio___test_ps2_wcluster_master_subscribe(void *ignr_1,
                                                       void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  FIO_LOG_DEBUG2("(%d) [Master] Subscribing to worker-cluster-channel",
                 fio_io_pid());
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1("worker-cluster-channel"),
                       .on_message = fio___test_ps2_wcluster_master_on_message);
  return -1;
}

/* First worker publishes using cluster engine */
static volatile int fio___test_ps2_wcluster_published = 0;
FIO_SFUNC int fio___test_ps2_wcluster_worker_publish(void *ignr_1,
                                                     void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_worker())
    return -1;

  /* Only one worker should publish */
  if (fio___test_ps2_wcluster_published)
    return -1;
  fio___test_ps2_wcluster_published = 1;

  FIO_LOG_DEBUG2(
      "(%d) [Worker] Publishing to worker-cluster-channel via cluster engine",
      fio_io_pid());
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1("worker-cluster-channel"),
                     .message = FIO_BUF_INFO1("worker-cluster-message"));
  return -1;
}

/* Timeout */
FIO_SFUNC int fio___test_ps2_wcluster_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* Verify */
FIO_SFUNC void fio___test_ps2_wcluster_on_finish(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_master())
    return;

  /* Master should receive the message */
  FIO_ASSERT(fio___test_ps2_wcluster_master_received >= 1,
             "[Master] Worker cluster publish: should receive at least 1 "
             "message (got %d)",
             fio___test_ps2_wcluster_master_received);

  /* Other worker should also receive */
  FIO_ASSERT(fio___test_ps2_wcluster_worker_confirms >= 1,
             "[Master] Worker cluster publish: should receive worker "
             "confirmation (got %d)",
             fio___test_ps2_wcluster_worker_confirms);

  FIO_LOG_DEBUG2(
      "(%d) [Master] Worker cluster publish test: master_received=%d "
      "worker_confirms=%d",
      fio_io_pid(),
      fio___test_ps2_wcluster_master_received,
      fio___test_ps2_wcluster_worker_confirms);
}

FIO_SFUNC void fio___pubsub_test_worker_cluster_publish(void) {
  fprintf(stderr,
          "* Testing FIO_PUBSUB worker publishes via cluster engine.\n");

  /* Reset */
  fio___test_ps2_wcluster_master_received = 0;
  fio___test_ps2_wcluster_worker_confirms = 0;
  fio___test_ps2_wcluster_published = 0;

  /* Register callbacks - workers subscribe via FIO_CALL_ON_START (same as Test
   * 3) */
  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___test_ps2_wcluster_worker_start,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___test_ps2_wcluster_on_finish,
                         NULL);

  /* Master subscribes at 150ms (give workers time to subscribe first) */
  fio_io_run_every(.fn = fio___test_ps2_wcluster_master_subscribe,
                   .every = 150,
                   .repetitions = 1);

  /* Worker publishes at 500ms (gives time for subscriptions to be active) */
  fio_io_run_every(.fn = fio___test_ps2_wcluster_worker_publish,
                   .every = 500,
                   .repetitions = 1);

  /* Timeout at 2 seconds */
  fio_io_run_every(.fn = fio___test_ps2_wcluster_timeout,
                   .every = 2000,
                   .repetitions = 1);

  /* Start with 2 workers */
  fio_io_start(2);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___test_ps2_wcluster_worker_start,
                            NULL);
  fio_state_callback_remove(FIO_CALL_ON_STOP,
                            fio___test_ps2_wcluster_on_finish,
                            NULL);

  fprintf(stderr, "* FIO_PUBSUB worker cluster publish test passed.\n");
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(int argc, char const *argv[]) {
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  /* Unit tests */
  fio___pubsub_test_types();
  fio___pubsub_test_env_type();
  fio___pubsub_test_history_api();
  fio___pubsub_test_cluster_engine_unit();

  /* Integration tests (master only) */
  fio___pubsub_test_basic();
  fio___pubsub_test_pattern();
  fio___pubsub_test_filter();
  fio___pubsub_test_unsubscribe();
  fio___pubsub_test_data_integrity();
  fio___pubsub_test_engine_comparison();

  /* Multi-process tests */
  fio___pubsub_test_worker_subscribe();
  fio___pubsub_test_worker_publish();
  fio___pubsub_test_broadcast();
  fio___pubsub_test_worker_history_replay();

  /* Cluster engine tests */
  fio___pubsub_test_cluster_engine_publish();
  fio___pubsub_test_cluster_engine_broadcast();
  fio___pubsub_test_worker_cluster_publish();

  (void)argc;
  (void)argv;
  return 0;
}
