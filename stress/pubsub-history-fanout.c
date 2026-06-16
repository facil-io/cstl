/* *****************************************************************************
Stress - Pub/Sub History Multi-Process Fanout (420 pubsub.h)

Multi-process fanout behavior moved from tests-old/pubsub-history2.c.
Starts worker processes and validates that every process receives every
message published by every process, including replay from history.

Guarded with #ifdef _WIN32 to log FIO_LOG_WARNING("SKIPPED") and return
success on Windows, because the POSIX fork()-based worker model is not
available there.
***************************************************************************** */
#include "tests/test-helpers.h"

#define FIO_PUBSUB
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Configuration
***************************************************************************** */

#define PSHF_TEST_CHANNEL           FIO_BUF_INFO1("pshf-test")
#define PSHF_MESSAGES_PER_PUBLISHER 50
#define PSHF_WORKERS                4
#define PSHF_START_OFFSET_MILLI     150
#define PSHF_SUBSCRIBE_OFFSET_MILLI 20
#define PSHF_LISTENERS              (PSHF_WORKERS + 1)

#ifdef DEBUG
#define PSHF_CLEANUP_MILLI 5000
#else
#define PSHF_CLEANUP_MILLI 3000
#endif

/* *****************************************************************************
Global State
***************************************************************************** */

static volatile size_t pshf_worker_id = 1;

static struct {
  struct {
    struct {
      size_t counter[PSHF_MESSAGES_PER_PUBLISHER];
      size_t total;
    } from[PSHF_LISTENERS];
  } to[PSHF_LISTENERS];
} pshf_results = {0}, pshf_sent = {0}, pshf_history = {0};

/* *****************************************************************************
Types and Payload Helpers
***************************************************************************** */

typedef struct {
  size_t numerator;
  uint16_t from;
  uint16_t to;
} pshf_payload_s;

static void pshf_add_worker_id(void *ignr_) {
  (void)ignr_;
  fio_atomic_add(&pshf_worker_id, 1);
}

/* *****************************************************************************
IPC Confirmation Handlers
***************************************************************************** */

static void pshf_inform_master_received(fio_ipc_s *ipc) {
  pshf_payload_s *data = (pshf_payload_s *)ipc->data;
  FIO_ASSERT(ipc->len == sizeof(*data), "Test code error - wrong payload size");
  FIO_ASSERT(data->to < PSHF_LISTENERS && data->from < PSHF_LISTENERS &&
                 data->numerator < PSHF_MESSAGES_PER_PUBLISHER,
             "Corrupt message - invalid indices");
  pshf_results.to[data->to].from[data->from].counter[data->numerator] += 1;
  pshf_results.to[data->to].from[data->from].total += 1;
}

static void pshf_inform_master_sent(fio_ipc_s *ipc) {
  pshf_payload_s *data = (pshf_payload_s *)ipc->data;
  FIO_ASSERT(ipc->len == sizeof(*data), "Test code error - wrong payload size");
  FIO_ASSERT(data->from < PSHF_LISTENERS &&
                 data->numerator < PSHF_MESSAGES_PER_PUBLISHER,
             "Corrupt message - invalid indices");
  for (size_t i = 0; i < PSHF_LISTENERS; ++i) {
    pshf_sent.to[i].from[data->from].counter[data->numerator] += 1;
    pshf_sent.to[i].from[data->from].total += 1;
  }
}

/* *****************************************************************************
Pub/Sub Message Callback
***************************************************************************** */

static void pshf_on_message_callback(fio_pubsub_msg_s *msg) {
  pshf_payload_s data = {0};
  FIO_ASSERT(msg->message.len == sizeof(data),
             "Test code error - wrong payload size");
  FIO_MEMCPY(&data, msg->message.buf, sizeof(data));
  data.to = pshf_worker_id;
  if (fio_io_is_master()) {
    data.to = 0;
  }
  fio_ipc_call(.call = pshf_inform_master_received,
               .data =
                   FIO_IPC_DATA(FIO_BUF_INFO2((char *)&data, sizeof(data))));
}

/* *****************************************************************************
Timer Callbacks
***************************************************************************** */

static int pshf_stop_io_timeout(void *ignr_, void *ignr__) {
  (void)ignr_;
  (void)ignr__;
  if (!fio_io_is_master() || !fio_io_is_running())
    return -1;
  fio_io_stop();
  return -1;
}

static int pshf_subscribe(void *ignr_, void *ignr__) {
  (void)ignr_;
  (void)ignr__;
  fio_pubsub_subscribe(.channel = PSHF_TEST_CHANNEL,
                       .on_message = pshf_on_message_callback,
                       .replay_since = 1);
  return -1;
}

static int pshf_publish_message(void *ignr_, void *ignr__) {
  (void)ignr_;
  (void)ignr__;
  static size_t numerator = 0;
  pshf_payload_s msg = {
      .numerator = numerator++,
      .from = pshf_worker_id,
  };
  if (fio_io_is_master()) {
    msg.from = 0;
  }
  msg.to = msg.from;
  fio_pubsub_publish(.channel = PSHF_TEST_CHANNEL,
                     .message = FIO_BUF_INFO2((char *)&msg, sizeof(msg)));
  fio_ipc_call(.call = pshf_inform_master_sent,
               .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)&msg, sizeof(msg))));
  if (numerator == PSHF_MESSAGES_PER_PUBLISHER) {
    if (fio_io_is_master())
      fio_io_run_every(.fn = pshf_stop_io_timeout,
                       .every = PSHF_CLEANUP_MILLI,
                       .repetitions = 0);
    return -1;
  }
  return 0;
}

static int pshf_subscribe_timer_setup(void *ignr_, void *ignr__) {
  size_t offset =
      PSHF_SUBSCRIBE_OFFSET_MILLI * (pshf_worker_id * !fio_io_is_master());
  if (!offset)
    pshf_subscribe(NULL, NULL);
  else
    fio_io_run_every(.fn = pshf_subscribe,
                     .every = offset,
                     .repetitions = 0);
  return -1;
  (void)ignr_;
  (void)ignr__;
}

/* *****************************************************************************
History Validation Callbacks
***************************************************************************** */

static void pshf_history_replay_on_message(fio_pubsub_msg_s *msg, void *udata) {
  pshf_payload_s data = {0};
  FIO_ASSERT(msg->message.len == sizeof(data),
             "Test code error - wrong payload size");
  FIO_MEMCPY(&data, msg->message.buf, sizeof(data));
  FIO_ASSERT(data.from < PSHF_LISTENERS &&
                 data.numerator < PSHF_MESSAGES_PER_PUBLISHER,
             "Corrupt message - invalid indices");
  for (size_t i = 0; i < PSHF_LISTENERS; ++i) {
    pshf_history.to[i].from[data.from].counter[data.numerator] += 1;
    pshf_history.to[i].from[data.from].total += 1;
  }
  (void)udata;
}

/* *****************************************************************************
Run and Verify
***************************************************************************** */

static int pshf_run_and_verify(void) {
  FIO_MEMSET(&pshf_results, 0, sizeof(pshf_results));
  FIO_MEMSET(&pshf_sent, 0, sizeof(pshf_sent));
  FIO_MEMSET(&pshf_history, 0, sizeof(pshf_history));

  size_t sent_count = 0;
  size_t received_count = 0;

  fio_io_run_every(.fn = pshf_stop_io_timeout,
                   .every = (PSHF_CLEANUP_MILLI + 7000),
                   .repetitions = 0);

  fio_pubsub_history_attach(fio_pubsub_history_cache(0), 100);

  fio_state_callback_add(FIO_CALL_IN_MASTER, pshf_add_worker_id, NULL);

  fio_io_run_every(.fn = pshf_subscribe_timer_setup,
                   .every = PSHF_START_OFFSET_MILLI,
                   .repetitions = 0);

  fio_io_run_every(.fn = pshf_publish_message,
                   .every = (PSHF_SUBSCRIBE_OFFSET_MILLI / 2),
                   .repetitions = PSHF_MESSAGES_PER_PUBLISHER,
                   .start_at = fio_io_last_tick() + PSHF_START_OFFSET_MILLI +
                               (PSHF_SUBSCRIBE_OFFSET_MILLI / PSHF_LISTENERS));

  fio_io_start(PSHF_WORKERS);

  int r = 0;
  for (size_t to = 0; to < PSHF_LISTENERS; ++to) {
    bool eol = 0;
    for (size_t from = 0; from < PSHF_LISTENERS; ++from) {
      for (size_t n = 0; n < PSHF_MESSAGES_PER_PUBLISHER; ++n) {
        if (!pshf_sent.to[to].from[from].counter[n])
          continue;
        sent_count += (pshf_sent.to[to].from[from].counter[n]) * (to == 0);
        received_count += pshf_results.to[to].from[from].counter[n];
        r -= !pshf_results.to[to].from[from].counter[n];
        if (!pshf_results.to[to].from[from].counter[n]) {
          eol |= 1;
          FIO_LOG_ERROR("%s[%zu] missing %s[%zu].Message[%zu]",
                        (to ? "Worker" : "Master"),
                        to,
                        (from ? "Worker" : "Master"),
                        from,
                        n);
        } else if (pshf_results.to[to].from[from].counter[n] > 1) {
          eol |= 1;
          FIO_LOG_INFO("%s[%zu] duplicate %s[%zu].Message[%zu]: %zu times",
                       (to ? "Worker" : "Master"),
                       to,
                       (from ? "Worker" : "Master"),
                       from,
                       n,
                       pshf_results.to[to].from[from].counter[n]);
        }
      }
    }
    if (eol)
      fprintf(stderr, "\n");
  }

  fio_pubsub_history_cache(0)->replay(fio_pubsub_history_cache(0),
                                      PSHF_TEST_CHANNEL,
                                      0,
                                      1,
                                      pshf_history_replay_on_message,
                                      NULL,
                                      NULL);

  size_t history_count = 0;
  if (FIO_MEMCMP(pshf_sent.to, pshf_history.to, sizeof(pshf_sent.to[0]))) {
    FIO_LOG_FATAL("History doesn't match sent message registry!");
    for (size_t from = 0; from < PSHF_LISTENERS; ++from) {
      for (size_t n = 0; n < PSHF_MESSAGES_PER_PUBLISHER; ++n) {
        history_count += pshf_history.to[0].from[from].counter[n];
        if (pshf_history.to[0].from[from].counter[n] > 1) {
          r |= 1;
          FIO_LOG_ERROR("History message duplicate! %s[%zu].Message[%zu] x %zu",
                        (from ? "Worker" : "Master"),
                        from,
                        n,
                        pshf_history.to[0].from[from].counter[n]);
        }
        if (pshf_sent.to[0].from[from].counter[n] ==
            pshf_history.to[0].from[from].counter[n])
          continue;
        r |= 1;
        FIO_LOG_ERROR("%s/%s %s[%zu].Message[%zu]",
                      (pshf_history.to[0].from[from].counter[n] ? "✅" : "❌"),
                      (pshf_sent.to[0].from[from].counter[n] ? "✅" : "❌"),
                      (from ? "Worker" : "Master"),
                      from,
                      n);
      }
    }
  }
  r |= !!history_count;
  if (r) {
    FIO_LOG_WARNING("Received: %zu    /    Sent: %zu    /    History: %zu",
                    received_count,
                    sent_count,
                    history_count);
  } else {
    FIO_LOG_INFO("Received: %zu    /    Sent: %zu", received_count, sent_count);
  }

  return r;
}

/* *****************************************************************************
Main entry point
***************************************************************************** */

int main(void) {
#ifdef _WIN32
  FIO_LOG_WARNING("SKIPPED");
  fprintf(stderr, "=== Pub/Sub history fanout stress tests skipped on Windows ===\n");
  return 0;
#else
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  fprintf(stderr, "=== Pub/Sub history fanout stress tests ===\n");

  int r = pshf_run_and_verify();

  if (r) {
    fprintf(stderr, "=== Pub/Sub history fanout stress tests FAILED ===\n");
  } else {
    fprintf(stderr, "=== Pub/Sub history fanout stress tests passed ===\n");
  }

  return r;
#endif
}
