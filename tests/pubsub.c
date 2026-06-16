/* *****************************************************************************
Test - Pub/Sub correctness (420 pubsub.h)

Fast, deterministic, single-process coverage of the public and internal
pub/sub API.  No performance loops, no benchmarking, no external processes,
and no long-running reactor sessions.

POSIX-only paths log FIO_LOG_WARNING("SKIPPED") and return success on
non-POSIX platforms.
***************************************************************************** */
#define FIO_PUBSUB
#include "test-helpers.h"

/* *****************************************************************************
Test State - Global Variables for Callback Tracking
***************************************************************************** */

static volatile size_t ps_basic_received = 0;
static char ps_basic_channel[64] = {0};
static char ps_basic_message[64] = {0};

static volatile size_t ps_pattern_received = 0;
static volatile size_t ps_filter0_received = 0;
static volatile size_t ps_filter1_received = 0;
static volatile size_t ps_unsub_received = 0;
static volatile size_t ps_unsub_callback = 0;
static volatile size_t ps_data_verified = 0;
static volatile size_t ps_ipc_received = 0;
static volatile size_t ps_cluster_received = 0;
static volatile size_t ps_handle_received = 0;
static volatile size_t ps_handle_unsub = 0;

/* *****************************************************************************
Unit Tests - Type Sizes and Layouts
***************************************************************************** */

static void test_pubsub_types(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB type sizes and layouts.\n");

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

  fio_pubsub_subscribe_args_s sub_args = {
      .io = NULL,
      .channel = FIO_BUF_INFO1("my-channel"),
      .udata = (void *)0x5678,
      .replay_since = 0,
      .filter = 100,
      .is_pattern = 1,
      .master_only = 0,
  };
  FIO_ASSERT(sub_args.filter == 100, "sub_args.filter should be 100");
  FIO_ASSERT(sub_args.is_pattern == 1, "sub_args.is_pattern should be 1");

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
Unit Tests - Engine API
***************************************************************************** */

static void test_pubsub_engines(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB engine API.\n");

  fio_pubsub_engine_s const *ipc = fio_pubsub_engine_ipc();
  FIO_ASSERT(ipc != NULL, "IPC engine should not be NULL");
  FIO_ASSERT(ipc->publish != NULL, "IPC engine should have publish");

  fio_pubsub_engine_s const *cluster = fio_pubsub_engine_cluster();
  FIO_ASSERT(cluster != NULL, "cluster engine should not be NULL");
  FIO_ASSERT(cluster->publish != NULL, "cluster engine should have publish");
  FIO_ASSERT(cluster != ipc, "cluster and IPC engines should differ");

  fio_pubsub_engine_s const *old_default = fio_pubsub_engine_default();
  FIO_ASSERT(old_default != NULL, "default engine should not be NULL");

  fio_pubsub_engine_s const *set = fio_pubsub_engine_default_set(cluster);
  FIO_ASSERT(set == cluster, "default_set should return the engine set");
  FIO_ASSERT(fio_pubsub_engine_default() == cluster,
             "default engine should now be cluster");

  fio_pubsub_engine_default_set(ipc);
  FIO_ASSERT(fio_pubsub_engine_default() == ipc,
             "default engine should be restored to IPC");

  fio_pubsub_engine_default_set(NULL);
  FIO_ASSERT(fio_pubsub_engine_default() == ipc,
             "setting NULL should restore IPC default");

  fprintf(stderr, "* FIO_PUBSUB engine tests passed.\n");
}

/* *****************************************************************************
Unit Tests - Match Function Set
***************************************************************************** */

static uint8_t ps_match_always(fio_str_info_s pattern, fio_str_info_s name) {
  (void)pattern;
  (void)name;
  return 1;
}

static uint8_t ps_match_never(fio_str_info_s pattern, fio_str_info_s name) {
  (void)pattern;
  (void)name;
  return 0;
}

static void test_pubsub_match_fn(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB match function setter.\n");

  fio_pubsub_match_fn_set(ps_match_always);
  fio_pubsub_match_fn_set(ps_match_never);
  fio_pubsub_match_fn_set(NULL); /* restore default glob matcher */

  fprintf(stderr, "* FIO_PUBSUB match function tests passed.\n");
}

/* *****************************************************************************
Integration Tests - Single Process
***************************************************************************** */

#define PS_BASIC_CHANNEL   "ps-basic-ch"
#define PS_PATTERN_CHANNEL "ps-news/*"
#define PS_FILTER_CHANNEL  "ps-filter-ch"
#define PS_UNSUB_CHANNEL   "ps-unsub-ch"
#define PS_DATA_CHANNEL    "ps-data-ch"
#define PS_CMP_CHANNEL     "ps-cmp-ch"
#define PS_HANDLE_CHANNEL  "ps-handle-ch"

FIO_SFUNC void ps_basic_on_message(fio_pubsub_msg_s *msg) {
  fio_atomic_add(&ps_basic_received, 1);
  if (msg->channel.len < sizeof(ps_basic_channel)) {
    FIO_MEMCPY((void *)ps_basic_channel, msg->channel.buf, msg->channel.len);
    ps_basic_channel[msg->channel.len] = '\0';
  }
  if (msg->message.len < sizeof(ps_basic_message)) {
    FIO_MEMCPY((void *)ps_basic_message, msg->message.buf, msg->message.len);
    ps_basic_message[msg->message.len] = '\0';
  }
}

FIO_SFUNC void ps_pattern_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&ps_pattern_received, 1);
}

FIO_SFUNC void ps_filter0_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&ps_filter0_received, 1);
}

FIO_SFUNC void ps_filter1_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&ps_filter1_received, 1);
}

FIO_SFUNC void ps_unsub_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&ps_unsub_received, 1);
}

FIO_SFUNC void ps_unsub_on_unsubscribe(void *udata) {
  (void)udata;
  fio_atomic_add(&ps_unsub_callback, 1);
}

FIO_SFUNC void ps_data_on_message(fio_pubsub_msg_s *msg) {
  int valid = 1;
  for (size_t i = 0; i < msg->message.len && valid; ++i) {
    if ((uint8_t)msg->message.buf[i] != (uint8_t)(i & 0xFF))
      valid = 0;
  }
  if (valid && msg->message.len == 1024) {
    fio_atomic_add(&ps_data_verified, 1);
  }
}

FIO_SFUNC void ps_cmp_on_message(fio_pubsub_msg_s *msg) {
  if (msg->message.len >= 3 && FIO_MEMCMP(msg->message.buf, "ipc", 3) == 0) {
    fio_atomic_add(&ps_ipc_received, 1);
  } else if (msg->message.len >= 7 &&
             FIO_MEMCMP(msg->message.buf, "cluster", 7) == 0) {
    fio_atomic_add(&ps_cluster_received, 1);
  }
}

FIO_SFUNC void ps_handle_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&ps_handle_received, 1);
}

FIO_SFUNC void ps_handle_on_unsubscribe(void *udata) {
  (void)udata;
  fio_atomic_add(&ps_handle_unsub, 1);
}

static uintptr_t ps_handle = 0;

FIO_SFUNC int ps_subscribe_all(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_BASIC_CHANNEL),
                       .on_message = ps_basic_on_message);

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_PATTERN_CHANNEL),
                       .on_message = ps_pattern_on_message,
                       .is_pattern = 1);

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_FILTER_CHANNEL),
                       .on_message = ps_filter0_on_message,
                       .filter = 0);
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_FILTER_CHANNEL),
                       .on_message = ps_filter1_on_message,
                       .filter = 1);

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_UNSUB_CHANNEL),
                       .on_message = ps_unsub_on_message,
                       .on_unsubscribe = ps_unsub_on_unsubscribe);

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_DATA_CHANNEL),
                       .on_message = ps_data_on_message);

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_CMP_CHANNEL),
                       .on_message = ps_cmp_on_message);

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PS_HANDLE_CHANNEL),
                       .on_message = ps_handle_on_message,
                       .on_unsubscribe = ps_handle_on_unsubscribe,
                       .subscription_handle_ptr = &ps_handle);

  return -1;
}

FIO_SFUNC int ps_unsubscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  (void)fio_pubsub_unsubscribe(.channel = FIO_BUF_INFO1(PS_UNSUB_CHANNEL));
  return -1;
}

FIO_SFUNC int ps_unsubscribe_handle(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  (void)fio_pubsub_unsubscribe(.subscription_handle_ptr = &ps_handle);
  return -1;
}

FIO_SFUNC int ps_publish_all(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PS_BASIC_CHANNEL),
                     .message = FIO_BUF_INFO1("hello-world"));

  fio_pubsub_publish(.channel = FIO_BUF_INFO1("ps-news/sports"),
                     .message = FIO_BUF_INFO1("sports-news"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("ps-news/tech"),
                     .message = FIO_BUF_INFO1("tech-news"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("ps-weather/rain"),
                     .message = FIO_BUF_INFO1("rain-forecast"));

  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PS_FILTER_CHANNEL),
                     .message = FIO_BUF_INFO1("filter0-message"),
                     .filter = 0);

  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PS_UNSUB_CHANNEL),
                     .message = FIO_BUF_INFO1("after-unsub"));

  {
    size_t size = 1024;
    char *data = (char *)FIO_MEM_REALLOC(NULL, 0, size, 0);
    if (data) {
      for (size_t i = 0; i < size; ++i) {
        data[i] = (char)(i & 0xFF);
      }
      fio_pubsub_publish(.channel = FIO_BUF_INFO1(PS_DATA_CHANNEL),
                         .message = FIO_BUF_INFO2(data, size));
      FIO_MEM_FREE(data, size);
    }
  }

  fio_pubsub_publish(.engine = fio_pubsub_engine_ipc(),
                     .channel = FIO_BUF_INFO1(PS_CMP_CHANNEL),
                     .message = FIO_BUF_INFO1("ipc-message"));
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1(PS_CMP_CHANNEL),
                     .message = FIO_BUF_INFO1("cluster-message"));

  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PS_HANDLE_CHANNEL),
                     .message = FIO_BUF_INFO1("handle-message-1"));

  return -1;
}

FIO_SFUNC int ps_publish_after_handle_unsub(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PS_HANDLE_CHANNEL),
                     .message = FIO_BUF_INFO1("handle-message-2"));
  return -1;
}

FIO_SFUNC int ps_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

static void test_pubsub_integration(void) {
#if !FIO_OS_POSIX
  FIO_LOG_WARNING("SKIPPED");
  fprintf(stderr, "* pubsub integration skipped on non-POSIX platform\n");
  return;
#else
  fprintf(stderr, "* Testing FIO_PUBSUB single-process integration.\n");

  ps_basic_received = 0;
  FIO_MEMSET((void *)ps_basic_channel, 0, sizeof(ps_basic_channel));
  FIO_MEMSET((void *)ps_basic_message, 0, sizeof(ps_basic_message));
  ps_pattern_received = 0;
  ps_filter0_received = 0;
  ps_filter1_received = 0;
  ps_unsub_received = 0;
  ps_unsub_callback = 0;
  ps_data_verified = 0;
  ps_ipc_received = 0;
  ps_cluster_received = 0;
  ps_handle_received = 0;
  ps_handle_unsub = 0;
  ps_handle = 0;

  fio_io_run_every(.fn = ps_subscribe_all, .every = 50, .repetitions = 1);
  fio_io_run_every(.fn = ps_unsubscribe, .every = 100, .repetitions = 1);
  fio_io_run_every(.fn = ps_publish_all, .every = 150, .repetitions = 1);
  fio_io_run_every(.fn = ps_unsubscribe_handle, .every = 200, .repetitions = 1);
  fio_io_run_every(.fn = ps_publish_after_handle_unsub,
                   .every = 250,
                   .repetitions = 1);
  fio_io_run_every(.fn = ps_timeout, .every = 500, .repetitions = 1);

  fio_io_start(0);

  fprintf(stderr, "  - Basic subscribe/publish: ");
  FIO_ASSERT(ps_basic_received >= 1,
             "Should receive at least 1 message (got %zu)",
             ps_basic_received);
  FIO_ASSERT(FIO_STRLEN(ps_basic_channel) == 11 &&
                 FIO_MEMCMP(ps_basic_channel, PS_BASIC_CHANNEL, 11) == 0,
             "Channel should be '%s' (got '%s')",
             PS_BASIC_CHANNEL,
             ps_basic_channel);
  FIO_ASSERT(FIO_STRLEN(ps_basic_message) == 11 &&
                 FIO_MEMCMP(ps_basic_message, "hello-world", 11) == 0,
             "Message should be 'hello-world' (got '%s')",
             ps_basic_message);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Pattern matching: ");
  FIO_ASSERT(ps_pattern_received == 2,
             "Pattern should match exactly 2 messages (got %zu)",
             ps_pattern_received);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Filter namespace: ");
  FIO_ASSERT(ps_filter0_received == 1,
             "Filter 0 should receive exactly 1 message (got %zu)",
             ps_filter0_received);
  FIO_ASSERT(ps_filter1_received == 0,
             "Filter 1 should receive 0 messages (got %zu)",
             ps_filter1_received);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Unsubscribe: ");
  FIO_ASSERT(ps_unsub_received == 0,
             "Should not receive messages after unsubscribe (got %zu)",
             ps_unsub_received);
  FIO_ASSERT(ps_unsub_callback == 1,
             "Unsubscribe callback should be called once (got %zu)",
             ps_unsub_callback);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Data integrity: ");
  FIO_ASSERT(ps_data_verified == 1,
             "Data integrity should be verified (got %zu)",
             ps_data_verified);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Engine comparison: ");
  FIO_ASSERT(ps_ipc_received >= 1,
             "IPC engine should deliver message (got %zu)",
             ps_ipc_received);
  FIO_ASSERT(ps_cluster_received >= 1,
             "Cluster engine should deliver message (got %zu)",
             ps_cluster_received);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Manual subscription handle: ");
  FIO_ASSERT(ps_handle_received == 1,
             "Should receive exactly 1 message via handle (got %zu)",
             ps_handle_received);
  FIO_ASSERT(ps_handle_unsub == 1,
             "Handle unsubscribe callback should be called once (got %zu)",
             ps_handle_unsub);
  FIO_ASSERT(ps_handle == 0,
             "Handle should be cleared after unsubscribe (got %zu)",
             (size_t)ps_handle);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "* FIO_PUBSUB integration tests passed.\n");
#endif
}

/* *****************************************************************************
Main entry point
***************************************************************************** */

int main(void) {
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  fprintf(stderr, "=== Pub/Sub correctness tests ===\n");

  test_pubsub_types();
  test_pubsub_engines();
  test_pubsub_match_fn();
  test_pubsub_integration();

  fprintf(stderr, "=== Pub/Sub correctness tests passed ===\n");
  return 0;
}
