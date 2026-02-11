/* *****************************************************************************
FIO_PUBSUB - Compilation, Unit Tests, and Multi-Process Integration Tests

Refactored to consolidate tests under fewer fio_io_start calls:
- Non-forking group: Tests that run with 0 workers (master-only)
- Forking group: Tests that require workers (uses 3 workers for all)

Each test uses unique channel names to avoid collisions.
Atomic counters track results across processes.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_PUBSUB
#define FIO_LOG
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Constants and Configuration
***************************************************************************** */

/* Worker startup can take up to 250ms */
#define WORKER_STARTUP_DELAY_MS 300
/* Additional time for message delivery */
#define MESSAGE_DELIVERY_MS 100
/* Timeout for entire test group */
#define TEST_TIMEOUT_MS 3000
/* Number of workers for forking tests (use max needed) */
#define FORKING_TEST_WORKERS 3

/* *****************************************************************************
Test Helpers
***************************************************************************** */

static volatile int test_message_received = 0;
static volatile int test_unsubscribe_called = 0;

static void test_on_message(fio_pubsub_msg_s *msg) {
  test_message_received = 1;
  (void)msg;
}

static void test_on_unsubscribe(void *udata) {
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
Unit Tests - Cluster Engine
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
Non-Forking Integration Tests - Consolidated
*****************************************************************************
All master-only tests run under a single fio_io_start(0) call.
Each test uses unique channel names to avoid collisions.
***************************************************************************** */

/* --- Test State (Atomic Counters) --- */

/* Test: Basic Subscribe/Publish */
static volatile size_t nf_basic_received = 0;
static char nf_basic_channel[64];
static char nf_basic_message[64];

/* Test: Pattern Matching */
static volatile size_t nf_pattern_received = 0;

/* Test: Filter Namespace */
static volatile size_t nf_filter0_received = 0;
static volatile size_t nf_filter1_received = 0;

/* Test: Unsubscribe */
static volatile size_t nf_unsub_received = 0;
static volatile size_t nf_unsub_callback = 0;

/* Test: Data Integrity */
static volatile size_t nf_data_verified = 0;

/* Test: Engine Comparison */
static volatile size_t nf_cmp_ipc_received = 0;
static volatile size_t nf_cmp_cluster_received = 0;

/* --- Channel Names (Unique per test) --- */
#define NF_BASIC_CHANNEL   "nf-basic-ch"
#define NF_PATTERN_CHANNEL "nf-news/*"
#define NF_FILTER_CHANNEL  "nf-filter-ch"
#define NF_UNSUB_CHANNEL   "nf-unsub-ch"
#define NF_DATA_CHANNEL    "nf-data-ch"
#define NF_CMP_CHANNEL     "nf-cmp-ch"

/* --- Callbacks --- */

FIO_SFUNC void nf_basic_on_message(fio_pubsub_msg_s *msg) {
  fio_atomic_add(&nf_basic_received, 1);
  if (msg->channel.len < 64) {
    FIO_MEMCPY((void *)nf_basic_channel, msg->channel.buf, msg->channel.len);
    nf_basic_channel[msg->channel.len] = '\0';
  }
  if (msg->message.len < 64) {
    FIO_MEMCPY((void *)nf_basic_message, msg->message.buf, msg->message.len);
    nf_basic_message[msg->message.len] = '\0';
  }
}

FIO_SFUNC void nf_pattern_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&nf_pattern_received, 1);
}

FIO_SFUNC void nf_filter0_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&nf_filter0_received, 1);
}

FIO_SFUNC void nf_filter1_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&nf_filter1_received, 1);
}

FIO_SFUNC void nf_unsub_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&nf_unsub_received, 1);
}

FIO_SFUNC void nf_unsub_on_unsub(void *udata) {
  (void)udata;
  fio_atomic_add(&nf_unsub_callback, 1);
}

FIO_SFUNC void nf_data_on_message(fio_pubsub_msg_s *msg) {
  /* Verify pattern: each byte = (index & 0xFF) */
  int valid = 1;
  for (size_t i = 0; i < msg->message.len && valid; ++i) {
    if ((uint8_t)msg->message.buf[i] != (uint8_t)(i & 0xFF))
      valid = 0;
  }
  if (valid && msg->message.len == 65536) {
    fio_atomic_add(&nf_data_verified, 1);
  }
}

FIO_SFUNC void nf_cmp_on_message(fio_pubsub_msg_s *msg) {
  if (msg->message.len >= 3 && FIO_MEMCMP(msg->message.buf, "ipc", 3) == 0) {
    fio_atomic_add(&nf_cmp_ipc_received, 1);
  } else if (msg->message.len >= 7 &&
             FIO_MEMCMP(msg->message.buf, "cluster", 7) == 0) {
    fio_atomic_add(&nf_cmp_cluster_received, 1);
  }
}

/* --- Timer: Subscribe all channels (50ms) --- */
FIO_SFUNC int nf_subscribe_all(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Basic */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(NF_BASIC_CHANNEL),
                       .on_message = nf_basic_on_message);

  /* Pattern */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(NF_PATTERN_CHANNEL),
                       .on_message = nf_pattern_on_message,
                       .is_pattern = 1);

  /* Filter (two subscriptions, same channel, different filters) */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(NF_FILTER_CHANNEL),
                       .on_message = nf_filter0_on_message,
                       .filter = 0);
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(NF_FILTER_CHANNEL),
                       .on_message = nf_filter1_on_message,
                       .filter = 1);

  /* Unsubscribe test - subscribe now, unsubscribe later */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(NF_UNSUB_CHANNEL),
                       .on_message = nf_unsub_on_message,
                       .on_unsubscribe = nf_unsub_on_unsub);

  /* Data integrity */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(NF_DATA_CHANNEL),
                       .on_message = nf_data_on_message);

  /* Engine comparison */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(NF_CMP_CHANNEL),
                       .on_message = nf_cmp_on_message);

  return -1; /* One-shot */
}

/* --- Timer: Unsubscribe for unsub test (100ms) --- */
FIO_SFUNC int nf_unsubscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  (void)fio_pubsub_unsubscribe(.channel = FIO_BUF_INFO1(NF_UNSUB_CHANNEL));
  return -1;
}

/* --- Timer: Publish all messages (150ms) --- */
FIO_SFUNC int nf_publish_all(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Basic */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(NF_BASIC_CHANNEL),
                     .message = FIO_BUF_INFO1("hello-world"));

  /* Pattern - publish to channels that match and don't match */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("nf-news/sports"),
                     .message = FIO_BUF_INFO1("sports-news"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("nf-news/tech"),
                     .message = FIO_BUF_INFO1("tech-news"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1("nf-weather/rain"),
                     .message = FIO_BUF_INFO1("rain-forecast")); /* No match */

  /* Filter - publish to filter 0 only */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(NF_FILTER_CHANNEL),
                     .message = FIO_BUF_INFO1("filter0-message"),
                     .filter = 0);

  /* Unsubscribe test - publish after unsubscribe (should not receive) */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(NF_UNSUB_CHANNEL),
                     .message = FIO_BUF_INFO1("after-unsub"));

  /* Data integrity - 64KB message with pattern */
  {
    size_t size = 65536;
    char *data = (char *)FIO_MEM_REALLOC(NULL, 0, size, 0);
    if (data) {
      for (size_t i = 0; i < size; ++i) {
        data[i] = (char)(i & 0xFF);
      }
      fio_pubsub_publish(.channel = FIO_BUF_INFO1(NF_DATA_CHANNEL),
                         .message = FIO_BUF_INFO2(data, size));
      FIO_MEM_FREE(data, size);
    }
  }

  /* Engine comparison - publish with both engines */
  fio_pubsub_publish(.engine = fio_pubsub_engine_ipc(),
                     .channel = FIO_BUF_INFO1(NF_CMP_CHANNEL),
                     .message = FIO_BUF_INFO1("ipc-message"));
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1(NF_CMP_CHANNEL),
                     .message = FIO_BUF_INFO1("cluster-message"));

  return -1; /* One-shot */
}

/* --- Timer: Timeout (500ms) --- */
FIO_SFUNC int nf_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* --- Run Non-Forking Tests --- */
FIO_SFUNC void fio___pubsub_test_nonforking_group(void) {
  fprintf(stderr,
          "* Testing FIO_PUBSUB non-forking tests (consolidated group).\n");

  /* Reset all state */
  nf_basic_received = 0;
  FIO_MEMSET((void *)nf_basic_channel, 0, sizeof(nf_basic_channel));
  FIO_MEMSET((void *)nf_basic_message, 0, sizeof(nf_basic_message));
  nf_pattern_received = 0;
  nf_filter0_received = 0;
  nf_filter1_received = 0;
  nf_unsub_received = 0;
  nf_unsub_callback = 0;
  nf_data_verified = 0;
  nf_cmp_ipc_received = 0;
  nf_cmp_cluster_received = 0;

  /* Schedule timers:
   * 50ms: Subscribe all
   * 100ms: Unsubscribe (for unsub test)
   * 150ms: Publish all
   * 500ms: Timeout
   */
  fio_io_run_every(.fn = nf_subscribe_all, .every = 50, .repetitions = 1);
  fio_io_run_every(.fn = nf_unsubscribe, .every = 100, .repetitions = 1);
  fio_io_run_every(.fn = nf_publish_all, .every = 150, .repetitions = 1);
  fio_io_run_every(.fn = nf_timeout, .every = 500, .repetitions = 1);

  /* Start reactor with 0 workers (master only) */
  fio_io_start(0);

  /* Verify results */
  fprintf(stderr, "  - Basic subscribe/publish: ");
  FIO_ASSERT(nf_basic_received >= 1,
             "Should receive at least 1 message (got %zu)",
             nf_basic_received);
  FIO_ASSERT(FIO_STRLEN(nf_basic_channel) == 11 &&
                 FIO_MEMCMP(nf_basic_channel, NF_BASIC_CHANNEL, 11) == 0,
             "Channel should be '%s' (got '%s')",
             NF_BASIC_CHANNEL,
             nf_basic_channel);
  FIO_ASSERT(FIO_STRLEN(nf_basic_message) == 11 &&
                 FIO_MEMCMP(nf_basic_message, "hello-world", 11) == 0,
             "Message should be 'hello-world' (got '%s')",
             nf_basic_message);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Pattern matching: ");
  FIO_ASSERT(nf_pattern_received == 2,
             "Pattern should match exactly 2 messages (got %zu)",
             nf_pattern_received);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Filter namespace: ");
  FIO_ASSERT(nf_filter0_received == 1,
             "Filter 0 should receive exactly 1 message (got %zu)",
             nf_filter0_received);
  FIO_ASSERT(nf_filter1_received == 0,
             "Filter 1 should receive 0 messages (got %zu)",
             nf_filter1_received);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Unsubscribe: ");
  FIO_ASSERT(nf_unsub_received == 0,
             "Should not receive messages after unsubscribe (got %zu)",
             nf_unsub_received);
  FIO_ASSERT(nf_unsub_callback == 1,
             "Unsubscribe callback should be called once (got %zu)",
             nf_unsub_callback);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Data integrity (64KB): ");
  FIO_ASSERT(nf_data_verified == 1,
             "64KB message data integrity should be verified (got %zu)",
             nf_data_verified);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Engine comparison: ");
  FIO_ASSERT(nf_cmp_ipc_received >= 1,
             "IPC engine should deliver message (got %zu)",
             nf_cmp_ipc_received);
  FIO_ASSERT(nf_cmp_cluster_received >= 1,
             "Cluster engine should deliver message (got %zu)",
             nf_cmp_cluster_received);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "* FIO_PUBSUB non-forking tests passed.\n");
}

/* *****************************************************************************
Forking Integration Tests - Consolidated
*****************************************************************************
All multi-process tests run under a single fio_io_start(3) call.
Each test uses unique channel names to avoid collisions.
Uses atomic counters for cross-process result tracking via IPC.
Accounts for 250ms+ worker startup delay.
***************************************************************************** */

/* --- Test State (Atomic Counters) --- */

/* Test: Worker Subscribe (master publishes, worker receives) */
static volatile size_t f_wsub_confirms = 0;

/* Test: Worker Publish (worker publishes, master receives) */
static volatile size_t f_wpub_master_received = 0;
static volatile size_t f_wpub_worker_confirms = 0;

/* Test: Broadcast (master publishes, all workers receive) */
static volatile size_t f_bcast_confirms = 0;

/* Test: History Replay (worker subscribes with replay_since) */
static volatile size_t f_hist_confirms = 0;

/* Test: Cluster Engine Publish (master publishes via cluster engine) */
static volatile size_t f_cluster_confirms = 0;

/* Test: Cluster Broadcast (master publishes via cluster, all workers receive)
 */
static volatile size_t f_cluster_bcast_confirms = 0;

/* Test: Worker Cluster Publish (worker publishes via cluster engine) */
static volatile size_t f_wcluster_master_received = 0;
static volatile size_t f_wcluster_worker_confirms = 0;

/* --- Channel Names (Unique per test) --- */
#define F_WSUB_CHANNEL          "f-wsub-ch"
#define F_WPUB_CHANNEL          "f-wpub-ch"
#define F_BCAST_CHANNEL         "f-bcast-ch"
#define F_HIST_CHANNEL          "f-hist-ch"
#define F_CLUSTER_CHANNEL       "f-cluster-ch"
#define F_CLUSTER_BCAST_CHANNEL "f-cluster-bcast-ch"
#define F_WCLUSTER_CHANNEL      "f-wcluster-ch"

/* --- IPC Handlers (run on master) --- */

FIO_SFUNC void f_wsub_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_wsub_confirms, 1);
}

FIO_SFUNC void f_wpub_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_wpub_worker_confirms, 1);
}

FIO_SFUNC void f_bcast_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_bcast_confirms, 1);
}

FIO_SFUNC void f_hist_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_hist_confirms, 1);
}

FIO_SFUNC void f_cluster_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_cluster_confirms, 1);
}

FIO_SFUNC void f_cluster_bcast_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_cluster_bcast_confirms, 1);
}

FIO_SFUNC void f_wcluster_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_wcluster_worker_confirms, 1);
}

/* --- Worker Message Callbacks --- */

FIO_SFUNC void f_wsub_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  const char *confirm = "wsub_received";
  fio_ipc_call(.call = f_wsub_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

FIO_SFUNC void f_wpub_worker_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  const char *confirm = "wpub_worker_received";
  fio_ipc_call(.call = f_wpub_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

FIO_SFUNC void f_wpub_master_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&f_wpub_master_received, 1);
}

FIO_SFUNC void f_bcast_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "bcast_worker_%d", fio_io_pid());
  fio_ipc_call(.call = f_bcast_confirm_handler,
               .data = FIO_IPC_DATA(FIO_BUF_INFO2(buf, (size_t)len)));
}

FIO_SFUNC void f_hist_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  const char *confirm = "hist_received";
  fio_ipc_call(.call = f_hist_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

FIO_SFUNC void f_cluster_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  const char *confirm = "cluster_received";
  fio_ipc_call(.call = f_cluster_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

FIO_SFUNC void f_cluster_bcast_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "cluster_bcast_worker_%d", fio_io_pid());
  fio_ipc_call(.call = f_cluster_bcast_confirm_handler,
               .data = FIO_IPC_DATA(FIO_BUF_INFO2(buf, (size_t)len)));
}

FIO_SFUNC void f_wcluster_worker_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  const char *confirm = "wcluster_worker_received";
  fio_ipc_call(.call = f_wcluster_confirm_handler,
               .data = FIO_IPC_DATA(
                   FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
}

FIO_SFUNC void f_wcluster_master_on_message(fio_pubsub_msg_s *msg) {
  (void)msg;
  fio_atomic_add(&f_wcluster_master_received, 1);
}

/* --- Worker Startup (FIO_CALL_ON_START) --- */
FIO_SFUNC void f_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  /* Worker Subscribe test */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_WSUB_CHANNEL),
                       .on_message = f_wsub_on_message);

  /* Worker Publish test */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_WPUB_CHANNEL),
                       .on_message = f_wpub_worker_on_message);

  /* Broadcast test */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_BCAST_CHANNEL),
                       .on_message = f_bcast_on_message);

  /* Cluster engine test */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_CLUSTER_CHANNEL),
                       .on_message = f_cluster_on_message);

  /* Cluster broadcast test */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_CLUSTER_BCAST_CHANNEL),
                       .on_message = f_cluster_bcast_on_message);

  /* Worker cluster publish test */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_WCLUSTER_CHANNEL),
                       .on_message = f_wcluster_worker_on_message);
}

/* --- Timer: Worker subscribes to history channel (after master publishes) ---
 */
FIO_SFUNC int f_hist_worker_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_worker())
    return -1;
  /* Subscribe with replay_since = 1 (replay all history) */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_HIST_CHANNEL),
                       .on_message = f_hist_on_message,
                       .replay_since = 1);
  return -1;
}

/* --- Timer: Master subscribes (WORKER_STARTUP_DELAY_MS) --- */
FIO_SFUNC int f_master_subscribe(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Worker Publish test - master subscribes */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_WPUB_CHANNEL),
                       .on_message = f_wpub_master_on_message);

  /* Worker Cluster Publish test - master subscribes */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(F_WCLUSTER_CHANNEL),
                       .on_message = f_wcluster_master_on_message);

  return -1;
}

/* --- Timer: Master publishes history messages (100ms - before workers
 * subscribe) --- */
FIO_SFUNC int f_hist_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Publish 3 messages to history */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(F_HIST_CHANNEL),
                     .message = FIO_BUF_INFO1("history-msg-1"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(F_HIST_CHANNEL),
                     .message = FIO_BUF_INFO1("history-msg-2"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(F_HIST_CHANNEL),
                     .message = FIO_BUF_INFO1("history-msg-3"));
  return -1;
}

/* --- Timer: Master publishes to workers (WORKER_STARTUP_DELAY_MS +
 * MESSAGE_DELIVERY_MS) --- */
FIO_SFUNC int f_master_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  /* Worker Subscribe test */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(F_WSUB_CHANNEL),
                     .message = FIO_BUF_INFO1("master-message"));

  /* Broadcast test */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(F_BCAST_CHANNEL),
                     .message = FIO_BUF_INFO1("broadcast-message"));

  /* Cluster engine test */
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1(F_CLUSTER_CHANNEL),
                     .message = FIO_BUF_INFO1("cluster-message"));

  /* Cluster broadcast test */
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1(F_CLUSTER_BCAST_CHANNEL),
                     .message = FIO_BUF_INFO1("cluster-broadcast-message"));

  return -1;
}

/* --- Timer: Worker publishes (WORKER_STARTUP_DELAY_MS + 2*MESSAGE_DELIVERY_MS)
 * --- */
FIO_SFUNC int f_worker_publish(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_worker())
    return -1;

  /* Worker Publish test */
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(F_WPUB_CHANNEL),
                     .message = FIO_BUF_INFO1("worker-message"));

  /* Worker Cluster Publish test */
  fio_pubsub_publish(.engine = fio_pubsub_engine_cluster(),
                     .channel = FIO_BUF_INFO1(F_WCLUSTER_CHANNEL),
                     .message = FIO_BUF_INFO1("worker-cluster-message"));

  return -1;
}

/* --- Timer: Timeout --- */
FIO_SFUNC int f_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* --- Run Forking Tests --- */
FIO_SFUNC void fio___pubsub_test_forking_group(void) {
  fprintf(stderr,
          "* Testing FIO_PUBSUB forking tests (consolidated group, %d "
          "workers).\n",
          FORKING_TEST_WORKERS);

  /* Reset all state */
  f_wsub_confirms = 0;
  f_wpub_master_received = 0;
  f_wpub_worker_confirms = 0;
  f_bcast_confirms = 0;
  f_hist_confirms = 0;
  f_cluster_confirms = 0;
  f_cluster_bcast_confirms = 0;
  f_wcluster_master_received = 0;
  f_wcluster_worker_confirms = 0;

  /* Attach history manager for replay_since to work */
  fio_pubsub_history_attach((fio_pubsub_history_s *)fio_pubsub_history_cache(0),
                            100);

  /* Register worker startup callback */
  fio_state_callback_add(FIO_CALL_ON_START, f_worker_start, NULL);

  /* Schedule timers:
   * Timeline accounts for 250ms+ worker startup delay
   *
   * 100ms: Master publishes history messages (before workers subscribe)
   * 300ms (WORKER_STARTUP_DELAY_MS): Master subscribes, workers ready
   * 400ms: Master publishes to workers
   * 500ms: Workers subscribe to history (with replay_since)
   * 600ms: Workers publish
   * 3000ms: Timeout
   */
  fio_io_run_every(.fn = f_hist_publish, .every = 100, .repetitions = 1);

  fio_io_run_every(.fn = f_master_subscribe,
                   .every = WORKER_STARTUP_DELAY_MS,
                   .repetitions = 1);

  fio_io_run_every(.fn = f_master_publish,
                   .every = WORKER_STARTUP_DELAY_MS + MESSAGE_DELIVERY_MS,
                   .repetitions = 1);

  fio_io_run_every(.fn = f_hist_worker_subscribe,
                   .every = WORKER_STARTUP_DELAY_MS + MESSAGE_DELIVERY_MS + 100,
                   .repetitions = 1);

  fio_io_run_every(.fn = f_worker_publish,
                   .every = WORKER_STARTUP_DELAY_MS + 2 * MESSAGE_DELIVERY_MS,
                   .repetitions = 1);

  fio_io_run_every(.fn = f_timeout, .every = TEST_TIMEOUT_MS, .repetitions = 1);

  /* Start reactor with workers */
  fio_io_start(FORKING_TEST_WORKERS);

  /* Cleanup */
  fio_state_callback_remove(FIO_CALL_ON_START, f_worker_start, NULL);

  /* Verify results */
  fprintf(stderr, "  - Worker subscribe (master publishes): ");
  FIO_ASSERT(f_wsub_confirms >= 1,
             "Should receive at least 1 confirmation from worker (got %zu)",
             f_wsub_confirms);
  fprintf(stderr, "PASS (got %zu confirms)\n", f_wsub_confirms);

  fprintf(stderr, "  - Worker publish (worker publishes, master receives): ");
  FIO_ASSERT(f_wpub_master_received >= 1,
             "Master should receive at least 1 message (got %zu)",
             f_wpub_master_received);
  FIO_ASSERT(f_wpub_worker_confirms >= 1,
             "Should receive worker confirmation (got %zu)",
             f_wpub_worker_confirms);
  fprintf(stderr,
          "PASS (master: %zu, workers: %zu)\n",
          f_wpub_master_received,
          f_wpub_worker_confirms);

  fprintf(stderr, "  - Broadcast (master publishes, all workers receive): ");
  FIO_ASSERT(f_bcast_confirms >= FORKING_TEST_WORKERS,
             "Should receive %d confirmations (got %zu)",
             FORKING_TEST_WORKERS,
             f_bcast_confirms);
  fprintf(stderr, "PASS (got %zu confirms)\n", f_bcast_confirms);

  fprintf(stderr, "  - History replay (worker subscribes with replay_since): ");
  FIO_ASSERT(f_hist_confirms >= 3,
             "Worker should receive at least 3 history messages (got %zu)",
             f_hist_confirms);
  fprintf(stderr, "PASS (got %zu confirms)\n", f_hist_confirms);

  fprintf(stderr, "  - Cluster engine publish: ");
  FIO_ASSERT(f_cluster_confirms >= 1,
             "Should receive at least 1 confirmation from worker (got %zu)",
             f_cluster_confirms);
  fprintf(stderr, "PASS (got %zu confirms)\n", f_cluster_confirms);

  fprintf(stderr, "  - Cluster broadcast: ");
  FIO_ASSERT(f_cluster_bcast_confirms >= FORKING_TEST_WORKERS,
             "Should receive %d confirmations (got %zu)",
             FORKING_TEST_WORKERS,
             f_cluster_bcast_confirms);
  fprintf(stderr, "PASS (got %zu confirms)\n", f_cluster_bcast_confirms);

  fprintf(stderr, "  - Worker cluster publish: ");
  FIO_ASSERT(f_wcluster_master_received >= 1,
             "Master should receive at least 1 message (got %zu)",
             f_wcluster_master_received);
  FIO_ASSERT(f_wcluster_worker_confirms >= 1,
             "Should receive worker confirmation (got %zu)",
             f_wcluster_worker_confirms);
  fprintf(stderr,
          "PASS (master: %zu, workers: %zu)\n",
          f_wcluster_master_received,
          f_wcluster_worker_confirms);

  fprintf(stderr, "* FIO_PUBSUB forking tests passed.\n");
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(int argc, char const *argv[]) {
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  /* Unit tests (no fio_io_start) */
  fio___pubsub_test_types();
  fio___pubsub_test_env_type();
  fio___pubsub_test_history_api();
  fio___pubsub_test_cluster_engine_unit();

  /* Non-forking integration tests (single fio_io_start(0)) */
  fio___pubsub_test_nonforking_group();

  /* Forking integration tests (single fio_io_start(3)) */
  fio___pubsub_test_forking_group();

  (void)argc;
  (void)argv;
  return 0;
}
