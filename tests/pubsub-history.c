/* *****************************************************************************
Test - Pub/Sub History correctness (420 pubsub.h)

Fast, deterministic, single-process coverage of the pub/sub history manager
API and in-process history replay.  No performance loops, no benchmarking,
no external processes, and no long-running reactor sessions.

POSIX-only paths log FIO_LOG_WARNING("SKIPPED") and return success on
non-POSIX platforms.
***************************************************************************** */
#define FIO_PUBSUB
#include "test-helpers.h"

/* *****************************************************************************
Test State - History Manager Counters
***************************************************************************** */

static int ps_history_custom_push_count = 0;
static int ps_history_custom_replay_count = 0;
static int ps_history_custom_detached_count = 0;
static int ps_history_minimal_push_count = 0;

static volatile size_t ps_history_replay_received = 0;
static char ps_history_replay_payload[64] = {0};

/* *****************************************************************************
Unit Tests - History Cache API
***************************************************************************** */

static void test_history_cache_api(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB history cache API.\n");

  fio_pubsub_history_s const *cache = fio_pubsub_history_cache(0);
  FIO_ASSERT(cache != NULL, "fio_pubsub_history_cache should return non-NULL");
  FIO_ASSERT(cache->push != NULL, "cache history should have push callback");
  FIO_ASSERT(cache->replay != NULL,
             "cache history should have replay callback");
  FIO_ASSERT(cache->oldest != NULL,
             "cache history should have oldest callback");
  FIO_ASSERT(cache->detached != NULL,
             "cache history should have detached callback");

  fio_pubsub_history_s const *cache2 = fio_pubsub_history_cache(1024 * 1024);
  FIO_ASSERT(cache2 == cache,
             "fio_pubsub_history_cache should return the same manager");

  fprintf(stderr, "* FIO_PUBSUB history cache API tests passed.\n");
}

/* *****************************************************************************
Unit Tests - Custom History Manager Attach/Detach
***************************************************************************** */

static int ps_history_custom_push(const struct fio_pubsub_history_s *hist,
                                  fio_pubsub_msg_s *msg) {
  (void)hist;
  (void)msg;
  ps_history_custom_push_count++;
  return 0;
}

static int ps_history_custom_replay(const struct fio_pubsub_history_s *hist,
                                    fio_buf_info_s channel,
                                    int16_t filter,
                                    uint64_t since,
                                    void (*on_message)(fio_pubsub_msg_s *msg,
                                                       void *udata),
                                    void (*on_done)(void *udata),
                                    void *udata) {
  (void)hist;
  (void)channel;
  (void)filter;
  (void)since;
  (void)on_message;
  (void)on_done;
  (void)udata;
  ps_history_custom_replay_count++;
  return -1; /* let cache handle replay */
}

static uint64_t
ps_history_custom_oldest(const struct fio_pubsub_history_s *hist,
                         fio_buf_info_s channel,
                         int16_t filter) {
  (void)hist;
  (void)channel;
  (void)filter;
  return UINT64_MAX;
}

static void
ps_history_custom_detached(const struct fio_pubsub_history_s *hist) {
  (void)hist;
  ps_history_custom_detached_count++;
}

static fio_pubsub_history_s ps_history_custom = {
    .push = ps_history_custom_push,
    .replay = ps_history_custom_replay,
    .oldest = ps_history_custom_oldest,
    .detached = ps_history_custom_detached,
};

static int ps_history_minimal_push(const struct fio_pubsub_history_s *hist,
                                   fio_pubsub_msg_s *msg) {
  (void)hist;
  (void)msg;
  ps_history_minimal_push_count++;
  return 0;
}

static fio_pubsub_history_s ps_history_minimal = {
    .push = ps_history_minimal_push,
    .replay = NULL,
    .oldest = NULL,
    .detached = NULL,
};

static void test_history_manager_api(void) {
  fprintf(stderr, "* Testing FIO_PUBSUB history manager attach/detach.\n");

  ps_history_custom_push_count = 0;
  ps_history_custom_replay_count = 0;
  ps_history_custom_detached_count = 0;
  ps_history_minimal_push_count = 0;

  FIO_ASSERT(fio_pubsub_history_attach(NULL, 100) == -1,
             "attaching NULL manager should fail");

  FIO_ASSERT(fio_pubsub_history_attach(&ps_history_custom, 100) == 0,
             "attaching custom manager should succeed");
  FIO_ASSERT(fio_pubsub_history_attach(&ps_history_custom, 50) == 0,
             "re-attaching custom manager should succeed");
  FIO_ASSERT(fio_pubsub_history_attach(&ps_history_custom, 200) == 0,
             "updating custom manager priority should succeed");

  fio_pubsub_history_detach(&ps_history_custom);
  FIO_ASSERT(ps_history_custom_detached_count == 1,
             "detached callback should be called once (got %d)",
             ps_history_custom_detached_count);

  FIO_ASSERT(fio_pubsub_history_attach(&ps_history_minimal, 50) == 0,
             "attaching minimal manager should succeed");
  FIO_ASSERT(ps_history_minimal.replay != NULL,
             "replay should be filled with stub");
  FIO_ASSERT(ps_history_minimal.oldest != NULL,
             "oldest should be filled with stub");
  FIO_ASSERT(ps_history_minimal.detached != NULL,
             "detached should be filled with stub");
  fio_pubsub_history_detach(&ps_history_minimal);

  fprintf(stderr, "* FIO_PUBSUB history manager API tests passed.\n");
}

/* *****************************************************************************
Integration Tests - Single Process History Replay
***************************************************************************** */

#define PSH_CHANNEL "psh-replay-ch"

FIO_SFUNC void psh_replay_on_message(fio_pubsub_msg_s *msg) {
  fio_atomic_add(&ps_history_replay_received, 1);
  if (msg->message.len < sizeof(ps_history_replay_payload)) {
    FIO_MEMCPY(ps_history_replay_payload, msg->message.buf, msg->message.len);
    ps_history_replay_payload[msg->message.len] = '\0';
  }
}

FIO_SFUNC int psh_publish_history(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PSH_CHANNEL),
                     .message = FIO_BUF_INFO1("history-msg-1"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PSH_CHANNEL),
                     .message = FIO_BUF_INFO1("history-msg-2"));
  fio_pubsub_publish(.channel = FIO_BUF_INFO1(PSH_CHANNEL),
                     .message = FIO_BUF_INFO1("history-msg-3"));

  return -1;
}

FIO_SFUNC int psh_subscribe_history(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1(PSH_CHANNEL),
                       .on_message = psh_replay_on_message,
                       .replay_since = 1);

  return -1;
}

FIO_SFUNC int psh_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

static void test_history_replay_integration(void) {
#if !FIO_OS_POSIX
  FIO_LOG_WARNING("SKIPPED");
  fprintf(stderr, "* history replay integration skipped on non-POSIX platform\n");
  return;
#else
  fprintf(stderr, "* Testing FIO_PUBSUB history replay integration.\n");

  ps_history_custom_push_count = 0;
  ps_history_replay_received = 0;
  FIO_MEMSET(ps_history_replay_payload, 0, sizeof(ps_history_replay_payload));

  /* attach both custom and cache managers */
  FIO_ASSERT(fio_pubsub_history_attach(&ps_history_custom, 200) == 0,
             "attaching custom manager should succeed");
  FIO_ASSERT(fio_pubsub_history_attach(fio_pubsub_history_cache(0), 100) == 0,
             "attaching cache manager should succeed");

  fio_io_run_every(.fn = psh_publish_history, .every = 50, .repetitions = 1);
  fio_io_run_every(.fn = psh_subscribe_history, .every = 100, .repetitions = 1);
  fio_io_run_every(.fn = psh_timeout, .every = 500, .repetitions = 1);

  fio_io_start(0);

  fio_pubsub_history_detach(fio_pubsub_history_cache(0));
  fio_pubsub_history_detach(&ps_history_custom);

  fprintf(stderr, "  - History replay: ");
  FIO_ASSERT(ps_history_replay_received == 3,
             "Should receive exactly 3 replayed messages (got %zu)",
             ps_history_replay_received);
  FIO_ASSERT(FIO_MEMCMP(ps_history_replay_payload, "history-msg-3", 13) == 0,
             "Last replayed message should be 'history-msg-3' (got '%s')",
             ps_history_replay_payload);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Custom manager push: ");
  FIO_ASSERT(ps_history_custom_push_count >= 3,
             "Custom manager should receive at least 3 push calls (got %d)",
             ps_history_custom_push_count);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "* FIO_PUBSUB history replay integration tests passed.\n");
#endif
}

/* *****************************************************************************
Main entry point
***************************************************************************** */

int main(void) {
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  fprintf(stderr, "=== Pub/Sub history correctness tests ===\n");

  test_history_cache_api();
  test_history_manager_api();
  test_history_replay_integration();

  fprintf(stderr, "=== Pub/Sub history correctness tests passed ===\n");
  return 0;
}
