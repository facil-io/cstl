/*
 * Test for pubsub multiple history managers with priority
 */
#define FIO_PUBSUB
#include "test-helpers.h"

/* --- Global State --- */
static volatile size_t WORKER_ID = 1; /* start from 1 and increase every fork */
#define TEST_CHANNEL           FIO_BUF_INFO1("test")
#define MESSAGES_PER_PUBLISHER 50
#define WORKERS                4
#define START_OFFSET_MILLI     150
#define SUBSCRIBE_OFFSET_MILLI 10
#define CLEANUP_MILLI          3000
#define LISTENERS              (WORKERS + 1)

static struct {
  struct {
    struct {
      size_t counter[MESSAGES_PER_PUBLISHER];
      size_t total;
    } from[LISTENERS];
  } to[LISTENERS];
} results = {0}, sent = {0}, history = {0};

/* --- Types --- */

typedef struct {
  size_t numerator;
  uint16_t from;
  uint16_t to;
} payload_s;

/* --- fio_state Callbacks --- */

static void add_one(void *ignr_) {
  (void)ignr_;
  fio_atomic_add(&WORKER_ID, 1);
}

/* --- IPC Callbacks --- */

static void inform_master_received(fio_ipc_s *ipc) {
  payload_s *data = (payload_s *)ipc->data;
  FIO_ASSERT(ipc->len == sizeof(*data), "Test code error - wrong payload size");
  results.to[data->to].from[data->from].counter[data->numerator] += 1;
  results.to[data->to].from[data->from].total += 1;
  if (ipc->from)
    FIO_LOG_INFO("(%d) Message[%zu] ACK from %zu",
                 fio_io_pid(),
                 data->numerator,
                 (size_t)data->from);
}

static void inform_master_sent(fio_ipc_s *ipc) {
  payload_s *data = (payload_s *)ipc->data;
  FIO_ASSERT(ipc->len == sizeof(*data), "Test code error - wrong payload size");
  for (size_t i = 0; i < LISTENERS; ++i) {
    sent.to[i].from[data->from].counter[data->numerator] += 1;
    sent.to[i].from[data->from].total += 1;
  }
}

/* --- Pub/Sub Callbacks --- */

static void on_message_callback(fio_pubsub_msg_s *msg) {
  payload_s data;
  FIO_ASSERT(msg->message.len == sizeof(data),
             "Test code error - wrong payload size");
  FIO_MEMCPY(&data, msg->message.buf, sizeof(data));
  data.to = WORKER_ID;
  if (fio_io_is_master()) {
    data.to = 0;
  }
  fio_ipc_call(.call = inform_master_received,
               .data =
                   FIO_IPC_DATA(FIO_BUF_INFO2((char *)&data, sizeof(data))));

  if (!fio_io_is_master())
    FIO_LOG_INFO("(%d) Sending Message[%zu] ACK from %zu, to %zu ",
                 fio_io_pid(),
                 data.numerator,
                 (size_t)data.from,
                 (size_t)data.to);
}

/* --- Timer Callbacks --- */

static int stop_io_timeout(void *ignr_, void *ignr__) {
  (void)ignr_;
  (void)ignr__;
  if (!fio_io_is_master())
    return -1;
  FIO_LOG_INFO("(%d) Timeout Detected, calling fio_io_stop", fio_io_pid());
  fio_io_stop();
  return -1;
}

static int subscribe2test(void *ignr_, void *ignr__) {
  (void)ignr_;
  (void)ignr__;
  size_t id = WORKER_ID;
  if (fio_io_is_master())
    id = 0;
  FIO_LOG_INFO("(%d) [%s][%zu] Subscribing to Test Channel: %s",
               fio_io_pid(),
               id ? "Worker" : "Master",
               id,
               TEST_CHANNEL.buf);
  fio_pubsub_subscribe(.channel = TEST_CHANNEL,
                       .on_message = on_message_callback,
                       .replay_since = 1);
  return -1;
}

static int publish_message(void *ignr_, void *ignr__) {
  (void)ignr_;
  (void)ignr__;
  static size_t numerator = 0;
  payload_s msg = {
      .numerator = numerator++,
      .from = WORKER_ID,
  };
  if (fio_io_is_master()) {
    msg.from = 0;
  }
  msg.to = msg.from;
  fio_pubsub_publish(.channel = TEST_CHANNEL,
                     .message = FIO_BUF_INFO2((char *)&msg, sizeof(msg)));
  fio_ipc_call(.call = inform_master_sent,
               .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)&msg, sizeof(msg))));
  if (numerator == MESSAGES_PER_PUBLISHER) {
    /* last message from this publisher, add some roudtrip time and stop */
    if (fio_io_is_master())
      fio_io_run_every(.fn = stop_io_timeout,
                       .every = CLEANUP_MILLI,
                       .repetitions = 0);
    return -1;
  }
  return 0;
}

static int subscribe_timer_setup(void *ignr_, void *ignr__) {
  size_t offset = SUBSCRIBE_OFFSET_MILLI * (WORKER_ID * !fio_io_is_master());
  if (!offset)
    subscribe2test(NULL, NULL);
  else
    fio_io_run_every(.fn = subscribe2test, .every = offset, .repetitions = 0);
  return -1;
  (void)ignr_;
  (void)ignr__;
}

/* --- History Validation Callbacks --- */

static void history_replay_on_message(fio_pubsub_msg_s *msg, void *udata) {
  payload_s data;
  FIO_ASSERT(msg->message.len == sizeof(data),
             "Test code error - wrong payload size");
  FIO_MEMCPY(&data, msg->message.buf, sizeof(data));
  for (size_t i = 0; i < LISTENERS; ++i) {
    history.to[i].from[data.from].counter[data.numerator] += 1;
    history.to[i].from[data.from].total += 1;
  }
  (void)udata;
}

/* --- Main --- */

int main(void) {
  int r = 0;
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;
  size_t sent_count = 0;
  size_t received_count = 0;
  /* setup timeout */
  fio_io_run_every(.fn = stop_io_timeout,
                   .every = (CLEANUP_MILLI + 7000),
                   .repetitions = 0);
  /* attach history manager for replay_since to work */
  fio_pubsub_history_attach(fio_pubsub_history_cache(0), 100);
  /* setup callbacks for unique worker ID */
  fio_state_callback_add(FIO_CALL_IN_MASTER, add_one, (void *)&WORKER_ID);
  /* setup timers for subscriptions */
  fio_io_run_every(.fn = subscribe_timer_setup,
                   .every = START_OFFSET_MILLI,
                   .repetitions = 0);
  /* setup timers for publications */
  fio_io_run_every(.fn = publish_message,
                   .every = (SUBSCRIBE_OFFSET_MILLI / 2),
                   .repetitions = MESSAGES_PER_PUBLISHER,
                   .start_at = fio_time_milli() + START_OFFSET_MILLI +
                               (LISTENERS / SUBSCRIBE_OFFSET_MILLI));
  /* run IO and wait for timeout */
  fio_io_start(WORKERS);

  /* Review results */
  for (size_t to = 0; to < LISTENERS; ++to) {
    bool eol = 0;
    for (size_t from = 0; from < LISTENERS; ++from) {
      for (size_t n = 0; n < MESSAGES_PER_PUBLISHER; ++n) {
        if (!sent.to[to].from[from].counter[n])
          continue;
        sent_count += (sent.to[to].from[from].counter[n]) * (to == 0);
        received_count += results.to[to].from[from].counter[n];
        r -= !results.to[to].from[from].counter[n];
        if (!results.to[to].from[from].counter[n]) {
          eol |= 1;
          FIO_LOG_ERROR("%s[%zu] missing %s[%zu].Message[%zu]",
                        (to ? "Worker" : "Master"),
                        to,
                        (from ? "Worker" : "Master"),
                        from,
                        n);
        } else if (results.to[to].from[from].counter[n] > 1) {
          eol |= 1;
          FIO_LOG_INFO("%s[%zu] duplicate %s[%zu].Message[%zu]: %zu times",
                       (to ? "Worker" : "Master"),
                       to,
                       (from ? "Worker" : "Master"),
                       from,
                       n,
                       results.to[to].from[from].counter[n]);
        }
      }
    }
    if (eol)
      fprintf(stderr, "\n");
  }
  /* make sure all sent messages are in the history */
  fio_pubsub_history_cache(0)->replay(fio_pubsub_history_cache(0),
                                      TEST_CHANNEL,
                                      0,
                                      1,
                                      history_replay_on_message,
                                      NULL,
                                      NULL);
  size_t history_count = 0;
  if (FIO_MEMCMP(sent.to, history.to, sizeof(sent.to[0]))) {
    FIO_LOG_FATAL("History doesn't match sent message registry!");
    for (size_t from = 0; from < LISTENERS; ++from) {
      for (size_t n = 0; n < MESSAGES_PER_PUBLISHER; ++n) {
        history_count += history.to[0].from[from].counter[n];
        if (history.to[0].from[from].counter[n] > 1) {
          r |= 1;
          FIO_LOG_ERROR("History message duplicate! %s[%zu].Message[%zu] x %zu",
                        (from ? "Worker" : "Master"),
                        from,
                        n,
                        history.to[0].from[from].counter[n]);
        }
        if (sent.to[0].from[from].counter[n] ==
            history.to[0].from[from].counter[n])
          continue;
        r |= 1;
        FIO_LOG_ERROR("%s/%s %s[%zu].Message[%zu]",
                      (history.to[0].from[from].counter[n] ? "✅" : "❌"),
                      (sent.to[0].from[from].counter[n] ? "✅" : "❌"),
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
  } else
    FIO_LOG_INFO("Received: %zu    /    Sent: %zu", received_count, sent_count);
  /* Fail process on error */
  return r;
}
