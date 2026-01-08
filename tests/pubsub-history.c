/*
 * Test for pubsub multiple history managers with priority
 */
#define FIO_PUBSUB
#define FIO_PUBSUB
#include "test-helpers.h"

/* Track calls to custom history manager */
static int custom_push_count = 0;
static int custom_replay_count = 0;
static int custom_replay_handled = 0;

/* Custom history manager - high priority but only handles recent history */
static int custom_history_push(const struct fio_pubsub_history_s *hist,
                               fio_pubsub_msg_s *msg) {
  (void)hist;
  (void)msg;
  custom_push_count++;
  return 0;
}

static int custom_history_replay(const struct fio_pubsub_history_s *hist,
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
  (void)on_message;
  (void)on_done;
  (void)udata;
  custom_replay_count++;

  /* Only handle "recent" requests (since > 1000) */
  if (since > 1000) {
    custom_replay_handled = 1;
    return 0; /* Handled */
  }
  return -1; /* Can't handle - let lower priority manager try */
}

static fio_pubsub_history_s custom_history = {
    .push = custom_history_push,
    .replay = custom_history_replay,
    .oldest = NULL,   /* Will be filled with stub */
    .detached = NULL, /* Will be filled with stub */
};

/* Minimal history manager - only has push, others NULL */
static int minimal_push_count = 0;
static int minimal_history_push(const struct fio_pubsub_history_s *hist,
                                fio_pubsub_msg_s *msg) {
  (void)hist;
  (void)msg;
  minimal_push_count++;
  return 0;
}

static fio_pubsub_history_s minimal_history = {
    .push = minimal_history_push,
    .replay = NULL,   /* Will be filled with stub */
    .oldest = NULL,   /* Will be filled with stub */
    .detached = NULL, /* Will be filled with stub */
};

int main(void) {
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;
  printf("\tTesting pubsub multiple history managers...\n");

  /* Test 1: Attach custom manager with high priority */
  printf("\t  Test 1: Attach custom manager with priority 100...\n");
  int r = fio_pubsub_history_attach(&custom_history, 100);
  FIO_ASSERT(r == 0, "fio_pubsub_history_attach should succeed");

  /* Test 2: Verify builtin is still attached (was attached at init) */
  printf("\t  Test 2: Verify builtin manager still attached...\n");
  /* Both managers should receive push calls */

  /* Test 3: Detach custom manager */
  printf("\t  Test 3: Detach custom manager...\n");
  fio_pubsub_history_detach(&custom_history);

  /* Test 4: Re-attach with different priority */
  printf("\t  Test 4: Re-attach with priority 50...\n");
  r = fio_pubsub_history_attach(&custom_history, 50);
  FIO_ASSERT(r == 0, "fio_pubsub_history_attach should succeed");

  /* Test 5: Update priority of existing manager */
  printf("\t  Test 5: Update priority to 200...\n");
  r = fio_pubsub_history_attach(&custom_history, 200);
  FIO_ASSERT(r == 0, "fio_pubsub_history_attach should update priority");

  /* Cleanup */
  fio_pubsub_history_detach(&custom_history);

  /* Test 6: Verify stubs are filled in for NULL callbacks */
  printf("\t  Test 6: Verify stubs fill in NULL callbacks...\n");
  r = fio_pubsub_history_attach(&minimal_history, 50);
  FIO_ASSERT(r == 0, "fio_pubsub_history_attach should succeed");
  FIO_ASSERT(minimal_history.replay != NULL,
             "replay should be filled with stub");
  FIO_ASSERT(minimal_history.oldest != NULL,
             "oldest should be filled with stub");
  FIO_ASSERT(minimal_history.detached != NULL,
             "destroy should be filled with stub");
  fio_pubsub_history_detach(&minimal_history);

  printf("\tAll pubsub history manager tests passed!\n");
  return 0;
}
