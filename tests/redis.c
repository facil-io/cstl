/* *****************************************************************************
Redis Module Tests

This test requires a running Redis server on localhost:6379.
The test will be skipped if Redis is not available.
***************************************************************************** */
#include "test-helpers.h"

/* Redis module requires these dependencies */
#define FIO_SOCK
#define FIO_TIME
#define FIO_THREADS
#define FIO_FIOBJ
#define FIO_PUBSUB
#define FIO_RESP3
#define FIO_REDIS
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Redis Availability Check
***************************************************************************** */

/** Returns 1 if Redis is available on localhost:6379, 0 otherwise */
FIO_SFUNC int redis_is_available(void) {
  int fd = fio_sock_open("127.0.0.1", "6379", FIO_SOCK_TCP | FIO_SOCK_CLIENT);
  if (!FIO_SOCK_FD_ISVALID(fd))
    return 0;
  /* Connection succeeded - Redis is available */
  fio_sock_close(fd);
  return 1;
}

/* *****************************************************************************
Test State - Event-Driven Test Framework
***************************************************************************** */

typedef enum {
  TEST_PHASE_INIT = 0,
  TEST_PHASE_PING,
  TEST_PHASE_SET,
  TEST_PHASE_GET,
  TEST_PHASE_DEL,
  TEST_PHASE_INCR_DEL,
  TEST_PHASE_INCR,
  TEST_PHASE_INCR_CLEANUP,
  TEST_PHASE_PUBSUB_ROUNDTRIP,
  TEST_PHASE_PUBSUB_PATTERN_DEDUP,
  TEST_PHASE_DONE,
} test_phase_e;

typedef struct {
  fio_pubsub_engine_s *redis;
  test_phase_e phase;
  test_phase_e last_phase_seen; /* For timeout detection */
  int test_failed;
  char get_value[256];
  size_t get_value_len;
  /* Pub/Sub test state */
  uintptr_t pubsub_sub_handle;
  uintptr_t pubsub_pattern_handle;
  uintptr_t pubsub_channel_handle;
  volatile int pubsub_msg_received;
  volatile int pubsub_dedup_msg_count;
} redis_test_state_s;

static redis_test_state_s test_state = {0};

/* Forward declarations */
FIO_SFUNC void redis_run_next_test(void);

/* *****************************************************************************
Timeout Watchdog - Fails test if no progress within 3 seconds
***************************************************************************** */

FIO_SFUNC int redis_timeout_watchdog(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;

  /* Check if we're done */
  if (test_state.phase == TEST_PHASE_DONE) {
    return -1; /* Stop the timer */
  }

  /* Check if phase has progressed since last check */
  if (test_state.phase == test_state.last_phase_seen) {
    /* No progress - timeout! */
    fprintf(stderr,
            "\t  FAIL: Timeout - no progress (stuck at phase %d)\n",
            (int)test_state.phase);
    test_state.test_failed = 1;
    fio_io_stop();
    return -1; /* Stop the timer */
  }

  /* Update last seen phase */
  test_state.last_phase_seen = test_state.phase;
  return 0; /* Continue timer */
}

/* *****************************************************************************
Test Callbacks - Called when Redis replies
***************************************************************************** */

static const char *TEST_KEY = "test.--.fio_stl.test_key";
static const char *TEST_VALUE = "hello_from_fio_stl";
static const char *TEST_COUNTER_KEY = "test.--.fio_stl.test_counter";

FIO_SFUNC void redis_on_ping_reply(fio_pubsub_engine_s *e,
                                   FIOBJ reply,
                                   void *udata) {
  (void)e;
  (void)udata;

  if (FIOBJ_TYPE(reply) == FIOBJ_T_STRING) {
    fio_str_info_s s = fiobj2cstr(reply);
    if (s.len == 4 && !memcmp(s.buf, "PONG", 4)) {
      test_state.phase = TEST_PHASE_SET;
      redis_run_next_test();
      return;
    }
  }
  FIO_LOG_ERROR("  FAIL: PING did not return PONG");
  test_state.test_failed = 1;
  fio_io_stop();
}

FIO_SFUNC void redis_on_set_reply(fio_pubsub_engine_s *e,
                                  FIOBJ reply,
                                  void *udata) {
  (void)e;
  (void)udata;

  if (FIOBJ_TYPE(reply) == FIOBJ_T_STRING) {
    fio_str_info_s s = fiobj2cstr(reply);
    if (s.len == 2 && !memcmp(s.buf, "OK", 2)) {
      /* SET succeeded, now test GET */
      test_state.phase = TEST_PHASE_GET;
      redis_run_next_test();
      return;
    }
  }
  FIO_LOG_ERROR("  FAIL: SET did not return OK");
  test_state.test_failed = 1;
  fio_io_stop();
}

FIO_SFUNC void redis_on_get_reply(fio_pubsub_engine_s *e,
                                  FIOBJ reply,
                                  void *udata) {
  (void)e;
  (void)udata;

  if (FIOBJ_TYPE(reply) == FIOBJ_T_STRING) {
    fio_str_info_s s = fiobj2cstr(reply);
    if (s.len == strlen(TEST_VALUE) && !memcmp(s.buf, TEST_VALUE, s.len)) {
      /* GET succeeded, now cleanup with DEL */
      test_state.phase = TEST_PHASE_DEL;
      redis_run_next_test();
      return;
    }
    FIO_LOG_ERROR("  FAIL: GET value mismatch: got '%.*s', expected '%s'",
                  (int)s.len,
                  s.buf,
                  TEST_VALUE);
  } else {
    FIO_LOG_ERROR("  FAIL: GET did not return string");
  }
  test_state.test_failed = 1;
  fio_io_stop();
}

FIO_SFUNC void redis_on_del_reply(fio_pubsub_engine_s *e,
                                  FIOBJ reply,
                                  void *udata) {
  (void)e;
  (void)udata;

  /* DEL returns integer - we don't care about the value */
  if (FIOBJ_TYPE(reply) == FIOBJ_T_NUMBER) {
    test_state.phase = TEST_PHASE_INCR_DEL;
    redis_run_next_test();
    return;
  }
  FIO_LOG_ERROR("  FAIL: DEL did not return integer");
  test_state.test_failed = 1;
  fio_io_stop();
}

FIO_SFUNC void redis_on_incr_del_reply(fio_pubsub_engine_s *e,
                                       FIOBJ reply,
                                       void *udata) {
  (void)e;
  (void)reply;
  (void)udata;

  /* DEL before INCR - we don't care about result */
  test_state.phase = TEST_PHASE_INCR;
  redis_run_next_test();
}

FIO_SFUNC void redis_on_incr_reply(fio_pubsub_engine_s *e,
                                   FIOBJ reply,
                                   void *udata) {
  (void)e;
  (void)udata;

  if (FIOBJ_TYPE(reply) == FIOBJ_T_NUMBER) {
    /* INCR succeeded, cleanup */
    test_state.phase = TEST_PHASE_INCR_CLEANUP;
    redis_run_next_test();
    return;
  }
  FIO_LOG_ERROR("  FAIL: INCR did not return integer");
  test_state.test_failed = 1;
  fio_io_stop();
}

FIO_SFUNC void redis_on_incr_cleanup_reply(fio_pubsub_engine_s *e,
                                           FIOBJ reply,
                                           void *udata) {
  (void)e;
  (void)reply;
  (void)udata;
  test_state.phase = TEST_PHASE_PUBSUB_ROUNDTRIP;
  redis_run_next_test();
}

/* *****************************************************************************
Pub/Sub Roundtrip Test Callbacks
***************************************************************************** */

static const char *PUBSUB_TEST_CHANNEL = "test.--.fio_stl.pubsub_channel";
static const char *PUBSUB_TEST_MESSAGE = "hello_from_pubsub_test";

/** Timer to start dedup test after roundtrip completes */
FIO_SFUNC int redis_start_dedup_test(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;
  test_state.phase = TEST_PHASE_PUBSUB_PATTERN_DEDUP;
  redis_run_next_test();
  return -1; /* Don't repeat */
}

/** Callback for pub/sub roundtrip test - receives the message */
FIO_SFUNC void redis_pubsub_roundtrip_on_message(fio_pubsub_msg_s *msg) {
  if (msg->message.len == strlen(PUBSUB_TEST_MESSAGE) &&
      !memcmp(msg->message.buf, PUBSUB_TEST_MESSAGE, msg->message.len)) {
    test_state.pubsub_msg_received = 1;
    /* Add a small delay before starting dedup test */
    fio_io_run_every(.fn = redis_start_dedup_test,
                     .every = 100,
                     .repetitions = 1);
  }
}

/** Timer callback to check if pub/sub message was received */
FIO_SFUNC int redis_pubsub_roundtrip_check(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;

  if (test_state.pubsub_msg_received) {
    return -1; /* Stop timer - message was received */
  }

  /* Message not received yet - this is handled by the watchdog timer */
  return 0;
}

/* *****************************************************************************
Pub/Sub Pattern Deduplication Test Callbacks
***************************************************************************** */

/* Channel and pattern for deduplication test - pattern must match channel */
static const char *DEDUP_TEST_CHANNEL = "test.--.fio_stl.dedup_channel";
static const char *DEDUP_TEST_PATTERN = "test.--.fio_stl.dedup*";
static const char *DEDUP_TEST_MESSAGE = "dedup_test_message";

/** Callback for pattern dedup test - counts messages received */
FIO_SFUNC void redis_pubsub_dedup_on_message(fio_pubsub_msg_s *msg) {
  if (msg->message.len == strlen(DEDUP_TEST_MESSAGE) &&
      !memcmp(msg->message.buf, DEDUP_TEST_MESSAGE, msg->message.len)) {
    fio_atomic_add(&test_state.pubsub_dedup_msg_count, 1);
  }
}

/** Timer callback to verify deduplication - message should be received once per
 * subscription.
 *
 * We have 2 subscriptions (channel + pattern), so we expect 2 messages.
 * The deduplication in fio___redis_on_sub_message prevents Redis from
 * publishing the same message twice (once via "message" and once via
 * "pmessage"). But each local subscription still receives the message once.
 *
 * Without deduplication, we would receive 4 messages:
 * - "message" from Redis → delivered to channel subscription
 * - "message" from Redis → delivered to pattern subscription
 * - "pmessage" from Redis → delivered to channel subscription
 * - "pmessage" from Redis → delivered to pattern subscription
 *
 * With deduplication, we receive 2 messages:
 * - Single local publish → delivered to channel subscription
 * - Single local publish → delivered to pattern subscription
 */
FIO_SFUNC int redis_pubsub_dedup_verify(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;

  /* Check if we got messages */
  if (test_state.pubsub_dedup_msg_count > 0) {
    /* Wait a bit more for any additional messages to arrive */
    static int wait_after_msg = 0;
    wait_after_msg++;
    if (wait_after_msg < 3) {
      return 0; /* Keep waiting */
    }
    wait_after_msg = 0;

    /* We expect 2 messages - one per subscription (channel + pattern) */
    if (test_state.pubsub_dedup_msg_count == 2) {
      test_state.phase = TEST_PHASE_DONE;
      redis_run_next_test();
    } else if (test_state.pubsub_dedup_msg_count > 2) {
      FIO_LOG_ERROR(
          "  FAIL: Deduplication failed - received %d messages (expected "
          "2)",
          test_state.pubsub_dedup_msg_count);
      test_state.test_failed = 1;
      fio_io_stop();
    } else {
      /* Less than 2 - might need more time */
      return 0;
    }
    return -1; /* Stop timer */
  }

  return 0; /* Keep checking - watchdog will handle timeout */
}

/** Timer callback to publish roundtrip test message */
FIO_SFUNC int redis_pubsub_roundtrip_publish(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;
  fio_pubsub_publish(.engine = test_state.redis,
                     .channel = FIO_BUF_INFO1((char *)PUBSUB_TEST_CHANNEL),
                     .message = FIO_BUF_INFO1((char *)PUBSUB_TEST_MESSAGE));
  return -1; /* Don't repeat */
}

/** Timer callback to publish dedup test message */
FIO_SFUNC int redis_pubsub_dedup_publish(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;
  fio_pubsub_publish(.engine = test_state.redis,
                     .channel = FIO_BUF_INFO1((char *)DEDUP_TEST_CHANNEL),
                     .message = FIO_BUF_INFO1((char *)DEDUP_TEST_MESSAGE));
  return -1; /* Don't repeat */
}

/* *****************************************************************************
Test Runner - Runs tests based on current phase
***************************************************************************** */

FIO_SFUNC void redis_run_next_test(void) {
  fio_pubsub_engine_s *redis = test_state.redis;
  FIOBJ cmd;
  int r;

  switch (test_state.phase) {
  case TEST_PHASE_INIT:
    /* Wait for connection - will be called again after delay */
    break;

  case TEST_PHASE_PING:
    cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("PING", 4));
    r = fio_redis_send(redis, cmd, redis_on_ping_reply, NULL);
    fiobj_free(cmd);
    if (r != 0) {
      FIO_LOG_ERROR("  FAIL: fio_redis_send returned %d", r);
      test_state.test_failed = 1;
      fio_io_stop();
    }
    break;

  case TEST_PHASE_SET:
    cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("SET", 3));
    fiobj_array_push(cmd, fiobj_str_new_cstr(TEST_KEY, strlen(TEST_KEY)));
    fiobj_array_push(cmd, fiobj_str_new_cstr(TEST_VALUE, strlen(TEST_VALUE)));
    r = fio_redis_send(redis, cmd, redis_on_set_reply, NULL);
    fiobj_free(cmd);
    if (r != 0) {
      FIO_LOG_ERROR("  FAIL: SET send failed");
      test_state.test_failed = 1;
      fio_io_stop();
    }
    break;

  case TEST_PHASE_GET:
    cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("GET", 3));
    fiobj_array_push(cmd, fiobj_str_new_cstr(TEST_KEY, strlen(TEST_KEY)));
    r = fio_redis_send(redis, cmd, redis_on_get_reply, NULL);
    fiobj_free(cmd);
    if (r != 0) {
      FIO_LOG_ERROR("  FAIL: GET send failed");
      test_state.test_failed = 1;
      fio_io_stop();
    }
    break;

  case TEST_PHASE_DEL:
    cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("DEL", 3));
    fiobj_array_push(cmd, fiobj_str_new_cstr(TEST_KEY, strlen(TEST_KEY)));
    r = fio_redis_send(redis, cmd, redis_on_del_reply, NULL);
    fiobj_free(cmd);
    if (r != 0) {
      FIO_LOG_ERROR("  FAIL: DEL send failed");
      test_state.test_failed = 1;
      fio_io_stop();
    }
    break;

  case TEST_PHASE_INCR_DEL:
    cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("DEL", 3));
    fiobj_array_push(
        cmd,
        fiobj_str_new_cstr(TEST_COUNTER_KEY, strlen(TEST_COUNTER_KEY)));
    r = fio_redis_send(redis, cmd, redis_on_incr_del_reply, NULL);
    fiobj_free(cmd);
    if (r != 0) {
      FIO_LOG_ERROR("  FAIL: DEL (before INCR) send failed");
      test_state.test_failed = 1;
      fio_io_stop();
    }
    break;

  case TEST_PHASE_INCR:
    cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("INCR", 4));
    fiobj_array_push(
        cmd,
        fiobj_str_new_cstr(TEST_COUNTER_KEY, strlen(TEST_COUNTER_KEY)));
    r = fio_redis_send(redis, cmd, redis_on_incr_reply, NULL);
    fiobj_free(cmd);
    if (r != 0) {
      FIO_LOG_ERROR("  FAIL: INCR send failed");
      test_state.test_failed = 1;
      fio_io_stop();
    }
    break;

  case TEST_PHASE_INCR_CLEANUP:
    cmd = fiobj_array_new();
    fiobj_array_push(cmd, fiobj_str_new_cstr("DEL", 3));
    fiobj_array_push(
        cmd,
        fiobj_str_new_cstr(TEST_COUNTER_KEY, strlen(TEST_COUNTER_KEY)));
    r = fio_redis_send(redis, cmd, redis_on_incr_cleanup_reply, NULL);
    fiobj_free(cmd);
    if (r != 0) {
      FIO_LOG_ERROR("  FAIL: DEL (cleanup) send failed");
      test_state.test_failed = 1;
      fio_io_stop();
    }
    break;

  case TEST_PHASE_PUBSUB_ROUNDTRIP:
    /* Reset state */
    test_state.pubsub_msg_received = 0;

    /* Attach Redis engine to pub/sub system.
     * Attach/detach do NOT affect reference counts - the caller owns the ref.
     */
    fio_pubsub_engine_attach(redis);

    /* Subscribe to test channel with our callback */
    fio_pubsub_subscribe(.channel = FIO_BUF_INFO1((char *)PUBSUB_TEST_CHANNEL),
                         .on_message = redis_pubsub_roundtrip_on_message,
                         .subscription_handle_ptr =
                             &test_state.pubsub_sub_handle);

    /* Schedule publish after a short delay to allow subscription to propagate
     */
    fio_io_run_every(.fn = redis_pubsub_roundtrip_check,
                     .every = 100,
                     .repetitions = -1);

    /* Publish message through Redis engine after a delay */
    fio_io_run_every(.fn = redis_pubsub_roundtrip_publish,
                     .every = 500,
                     .repetitions = 1);
    break;

  case TEST_PHASE_PUBSUB_PATTERN_DEDUP:
    /* Reset state */
    test_state.pubsub_dedup_msg_count = 0;

    /* Subscribe to both a pattern and a channel that matches the pattern.
     * When we publish to the channel, Redis will send both a "message" and
     * a "pmessage". The deduplication logic should prevent duplicate local
     * publishes, so we expect exactly 2 messages (one per subscription). */
    fio_pubsub_subscribe(.channel = FIO_BUF_INFO1((char *)DEDUP_TEST_PATTERN),
                         .on_message = redis_pubsub_dedup_on_message,
                         .is_pattern = 1,
                         .subscription_handle_ptr =
                             &test_state.pubsub_pattern_handle);

    fio_pubsub_subscribe(.channel = FIO_BUF_INFO1((char *)DEDUP_TEST_CHANNEL),
                         .on_message = redis_pubsub_dedup_on_message,
                         .subscription_handle_ptr =
                             &test_state.pubsub_channel_handle);

    /* Schedule verification timer */
    fio_io_run_every(.fn = redis_pubsub_dedup_verify,
                     .every = 200,
                     .repetitions = -1);

    /* Publish message through Redis after subscriptions propagate
     * Use longer delay to ensure PSUBSCRIBE is processed */
    fio_io_run_every(.fn = redis_pubsub_dedup_publish,
                     .every = 1500,
                     .repetitions = 1);
    break;

  case TEST_PHASE_DONE:
    /* Cleanup pub/sub subscriptions */
    if (test_state.pubsub_sub_handle) {
      fio_pubsub_unsubscribe(.subscription_handle_ptr =
                                 &test_state.pubsub_sub_handle);
      test_state.pubsub_sub_handle = 0;
    }
    if (test_state.pubsub_channel_handle) {
      fio_pubsub_unsubscribe(.subscription_handle_ptr =
                                 &test_state.pubsub_channel_handle);
      test_state.pubsub_channel_handle = 0;
    }
    if (test_state.pubsub_pattern_handle) {
      fio_pubsub_unsubscribe(.subscription_handle_ptr =
                                 &test_state.pubsub_pattern_handle,
                             .is_pattern = 1);
      test_state.pubsub_pattern_handle = 0;
    }

    /* Detach Redis from pub/sub before cleanup */
    fio_pubsub_engine_detach(redis);

    fio_io_stop();
    break;
  }
}

/* *****************************************************************************
Timer Callback - Starts tests after connection delay
***************************************************************************** */

FIO_SFUNC int redis_start_tests_timer(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;

  /* Start with PING test */
  test_state.phase = TEST_PHASE_PING;
  test_state.last_phase_seen = TEST_PHASE_INIT; /* Different from current */
  redis_run_next_test();
  return 0; /* Don't repeat */
}

/* *****************************************************************************
Round 1 Runner - Single-process mode
***************************************************************************** */

/** Runs Round 1 tests in single-process mode. Returns 0 on success. */
FIO_SFUNC int redis_run_round1(void) {
  fprintf(stderr, "\t* Round 1: single-process mode (master-direct paths)\n");

  /* Create Redis engine */
  fio_pubsub_engine_s *redis = fio_redis_new(.url = "redis://localhost:6379");
  FIO_ASSERT_ALLOC(redis);

  FIO_MEMSET(&test_state, 0, sizeof(test_state));
  test_state.redis = redis;

  /* Start timeout watchdog - checks every 3 seconds for progress */
  fio_io_run_every(.fn = redis_timeout_watchdog,
                   .every = 3000,
                   .repetitions = -1);

  /* Schedule tests to run after a short delay to allow connection */
  fio_io_run_every(.fn = redis_start_tests_timer,
                   .every = 500,
                   .repetitions = 1);

  /* Start the IO reactor (this blocks until fio_io_stop is called) */
  fio_io_start(0);
  fio_redis_free(redis);

  /* Check if tests completed successfully */
  if (test_state.test_failed || test_state.phase != TEST_PHASE_DONE) {
    FIO_LOG_ERROR("FAIL: Round 1 tests did not complete successfully");
    return 1;
  }
  fprintf(stderr, "\t  Round 1 PASS\n");
  return 0;
}

/* *****************************************************************************




Round 2: Multi-Process Mode Tests (1 worker)

Exercises the IPC worker path that is the core architectural feature:
- fio_redis_send from worker: worker→IPC→master→Redis→reply→IPC→worker callback
- fio_pubsub_publish from worker: vtable-swapped publish via IPC to master
- fio_redis_send from master: direct path still works in multi-process mode
- Pub/sub receive on master: messages published by worker reach master
subscriber

Result reporting strategy:
  Worker fio_redis_send callbacks report results to master via IPC
  (fio_ipc_call with a master-side handler that increments atomic counters).
  This is reliable and has no timing dependencies.

  Worker pub/sub publish is tested separately: the master subscribes from
  a timer (after the IO reactor and Redis connections are established),
  then the worker publishes with a delay to ensure the master is subscribed.

Timeline:
  0ms:      fio_io_start(1) - master forks 1 worker
  ~100ms:   Worker IPC connection established
  ON_START: Worker fires fio_redis_send commands (PING, SET, GET, DEL)
  ~300ms:   Worker callbacks fire, report results via IPC
  1000ms:   Master setup timer: attach pubsub, subscribe, send master PING
  2000ms:   Worker pub/sub publish timer fires (master is subscribed by now)
  5000ms:   Timeout → fio_io_stop()

All verification happens AFTER fio_io_start(1) returns (master only).




***************************************************************************** */

/* --- Configuration --- */
#define MP_TIMEOUT_MS 5000

/* --- Redis keys and channels (distinct from Round 1) --- */
static const char *MP_TEST_KEY = "test.--.fio_stl.mp_test_key";
static const char *MP_TEST_VALUE = "mp_hello_from_worker";
static const char *MP_PUBSUB_CHANNEL = "test.--.fio_stl.mp_pubsub_channel";
static const char *MP_PUBSUB_MESSAGE = "mp_hello_from_worker_pubsub";

/* --- Multi-process test state (master-side atomic counters) --- */
static volatile size_t mp_ack_ping = 0;
static volatile size_t mp_ack_set = 0;
static volatile size_t mp_ack_get = 0;
static volatile size_t mp_ack_del = 0;
static volatile size_t mp_pubsub_received = 0;
static volatile size_t mp_master_ping = 0;

/* Redis engine pointer for Round 2 (shared between master setup and worker) */
static fio_pubsub_engine_s *mp_redis = NULL;

/* Subscription handles for cleanup */
static uintptr_t mp_pubsub_sub = 0;

/* *****************************************************************************
Round 2: IPC result reporting (worker → master)
*****************************************************************************
Worker fio_redis_send callbacks validate replies and report results to master
via fio_ipc_call. The master-side handler increments atomic counters.
This is reliable and has no timing dependencies (unlike pub/sub acks).
***************************************************************************** */

/** Master-side IPC handler: increments the counter pointed to by ipc->udata */
FIO_SFUNC void mp_ack_handler(fio_ipc_s *ipc) {
  if (ipc->udata)
    fio_atomic_add((volatile size_t *)ipc->udata, 1);
}

/** Helper: send an IPC ack from worker to master */
FIO_SFUNC void mp_worker_ack(volatile size_t *counter) {
  fio_ipc_call(.call = mp_ack_handler, .udata = (void *)counter);
}

/* *****************************************************************************
Round 2: Worker-side callbacks (run in worker process)
*****************************************************************************
These are fio_redis_send callbacks. They fire in the WORKER process after
the full IPC round-trip: worker→IPC→master→Redis→reply→IPC→worker.

Each callback validates the reply and sends an IPC ack to master.
***************************************************************************** */

/** Worker PING callback: validate "PONG", IPC ack to master */
FIO_SFUNC void mp_worker_on_ping(fio_pubsub_engine_s *e,
                                 FIOBJ reply,
                                 void *udata) {
  (void)e;
  (void)udata;
  if (FIOBJ_TYPE(reply) == FIOBJ_T_STRING) {
    fio_str_info_s s = fiobj2cstr(reply);
    if (s.len == 4 && FIO_MEMCMP(s.buf, "PONG", 4) == 0) {
      mp_worker_ack(&mp_ack_ping);
      return;
    }
  }
  FIO_LOG_ERROR("(mp worker) PING did not return PONG");
}

/** Worker SET callback: validate "OK", IPC ack to master */
FIO_SFUNC void mp_worker_on_set(fio_pubsub_engine_s *e,
                                FIOBJ reply,
                                void *udata) {
  (void)e;
  (void)udata;
  if (FIOBJ_TYPE(reply) == FIOBJ_T_STRING) {
    fio_str_info_s s = fiobj2cstr(reply);
    if (s.len == 2 && FIO_MEMCMP(s.buf, "OK", 2) == 0) {
      mp_worker_ack(&mp_ack_set);
      return;
    }
  }
  FIO_LOG_ERROR("(mp worker) SET did not return OK");
}

/** Worker GET callback: validate value matches, IPC ack to master */
FIO_SFUNC void mp_worker_on_get(fio_pubsub_engine_s *e,
                                FIOBJ reply,
                                void *udata) {
  (void)e;
  (void)udata;
  if (FIOBJ_TYPE(reply) == FIOBJ_T_STRING) {
    fio_str_info_s s = fiobj2cstr(reply);
    size_t expected_len = FIO_STRLEN(MP_TEST_VALUE);
    if (s.len == expected_len &&
        FIO_MEMCMP(s.buf, MP_TEST_VALUE, expected_len) == 0) {
      mp_worker_ack(&mp_ack_get);
      return;
    }
    FIO_LOG_ERROR("(mp worker) GET mismatch: got '%.*s'", (int)s.len, s.buf);
  } else {
    FIO_LOG_ERROR("(mp worker) GET did not return string");
  }
}

/** Worker DEL callback: validate integer reply, IPC ack to master */
FIO_SFUNC void mp_worker_on_del(fio_pubsub_engine_s *e,
                                FIOBJ reply,
                                void *udata) {
  (void)e;
  (void)udata;
  if (FIOBJ_TYPE(reply) == FIOBJ_T_NUMBER) {
    mp_worker_ack(&mp_ack_del);
    return;
  }
  FIO_LOG_ERROR("(mp worker) DEL did not return integer");
}

/* *****************************************************************************
Round 2: Master-side callbacks
***************************************************************************** */

/**
 * Master subscription callback for the pub/sub test channel.
 * Receives the message that the worker published via fio_pubsub_publish.
 */
FIO_SFUNC void mp_master_on_pubsub(fio_pubsub_msg_s *msg) {
  size_t expected_len = FIO_STRLEN(MP_PUBSUB_MESSAGE);
  if (msg->message.len == expected_len &&
      FIO_MEMCMP(msg->message.buf, MP_PUBSUB_MESSAGE, expected_len) == 0)
    fio_atomic_add(&mp_pubsub_received, 1);
}

/** Master PING callback: increments counter directly (runs in master) */
FIO_SFUNC void mp_master_on_ping(fio_pubsub_engine_s *e,
                                 FIOBJ reply,
                                 void *udata) {
  (void)e;
  (void)udata;
  if (FIOBJ_TYPE(reply) == FIOBJ_T_STRING) {
    fio_str_info_s s = fiobj2cstr(reply);
    if (s.len == 4 && FIO_MEMCMP(s.buf, "PONG", 4) == 0) {
      fio_atomic_add(&mp_master_ping, 1);
      return;
    }
  }
  FIO_LOG_ERROR("(mp master) PING did not return PONG");
}

/* *****************************************************************************
Round 2: Worker pub/sub publish timer
***************************************************************************** */

/** Timer: Worker publishes a message via the vtable-swapped publish path.
 * Delayed to allow master to establish subscription connection first. */
FIO_SFUNC int mp_worker_pubsub_timer(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;
  fio_pubsub_publish(.engine = mp_redis,
                     .channel = FIO_BUF_INFO1((char *)MP_PUBSUB_CHANNEL),
                     .message = FIO_BUF_INFO1((char *)MP_PUBSUB_MESSAGE));
  return -1; /* One-shot */
}

/* *****************************************************************************
Round 2: Worker startup (FIO_CALL_ON_START)
*****************************************************************************
Fires in the worker process after the IO loop and IPC connection are ready.
Sends all Redis commands and schedules a delayed pub/sub publish.
Commands are queued in order: SET → GET → DEL (Redis processes them FIFO).
PING is independent.
***************************************************************************** */

FIO_SFUNC void mp_worker_start(void *ignr_) {
  (void)ignr_;
  /* Only run in worker processes (not master) */
  if (fio_io_is_master())
    return;

  fio_pubsub_engine_s *redis = mp_redis;
  FIOBJ cmd;

  /* --- PING via fio_redis_send (IPC round-trip) --- */
  cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("PING", 4));
  fio_redis_send(redis, cmd, mp_worker_on_ping, NULL);
  fiobj_free(cmd);

  /* --- SET via fio_redis_send (IPC round-trip) --- */
  cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("SET", 3));
  fiobj_array_push(cmd,
                   fiobj_str_new_cstr(MP_TEST_KEY, FIO_STRLEN(MP_TEST_KEY)));
  fiobj_array_push(
      cmd,
      fiobj_str_new_cstr(MP_TEST_VALUE, FIO_STRLEN(MP_TEST_VALUE)));
  fio_redis_send(redis, cmd, mp_worker_on_set, NULL);
  fiobj_free(cmd);

  /* --- GET via fio_redis_send (IPC round-trip, reads SET's value) --- */
  cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("GET", 3));
  fiobj_array_push(cmd,
                   fiobj_str_new_cstr(MP_TEST_KEY, FIO_STRLEN(MP_TEST_KEY)));
  fio_redis_send(redis, cmd, mp_worker_on_get, NULL);
  fiobj_free(cmd);

  /* --- DEL via fio_redis_send (IPC round-trip, cleanup) --- */
  cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("DEL", 3));
  fiobj_array_push(cmd,
                   fiobj_str_new_cstr(MP_TEST_KEY, FIO_STRLEN(MP_TEST_KEY)));
  fio_redis_send(redis, cmd, mp_worker_on_del, NULL);
  fiobj_free(cmd);

  /* --- Pub/sub publish from worker (vtable-swapped path) ---
   * This exercises fio___redis_publish_worker which was installed by
   * FIO_CALL_IN_CHILD. The publish goes: worker→IPC→master→Redis PUBLISH.
   * The master's subscription on MP_PUBSUB_CHANNEL receives it.
   *
   * Delayed to 2000ms so the master has time to:
   * 1. Establish the Redis subscription connection (master setup at 1000ms)
   * 2. Send SUBSCRIBE to Redis (~100ms after attach)
   * 3. Be ready to receive the published message */
  fio_io_run_every(.fn = mp_worker_pubsub_timer,
                   .every = 2000,
                   .repetitions = 1);
}

/* *****************************************************************************
Round 2: Master timers
***************************************************************************** */

/**
 * Timer: Master-side setup after Redis connection is established (~500ms).
 * Attaches the Redis engine to pub/sub, subscribes to test channels,
 * and sends a PING from master.
 *
 * This MUST happen inside the IO loop (not before fio_io_start) because
 * fio_pubsub_engine_attach triggers the subscription connection, which
 * requires the IO reactor to be running.
 */
FIO_SFUNC int mp_master_setup_timer(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;
  if (!fio_io_is_master())
    return -1;

  /* Attach to pub/sub system (starts subscription connection to Redis) */
  fio_pubsub_engine_attach(mp_redis);

  /* Subscribe to pub/sub test channel (receives worker's publish) */
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO1((char *)MP_PUBSUB_CHANNEL),
                       .on_message = mp_master_on_pubsub,
                       .subscription_handle_ptr = &mp_pubsub_sub);

  /* Send PING from master (direct path in multi-process mode) */
  FIOBJ cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("PING", 4));
  fio_redis_send(mp_redis, cmd, mp_master_on_ping, NULL);
  fiobj_free(cmd);

  return -1; /* One-shot */
}

/** Timer: Timeout → stop reactor */
FIO_SFUNC int mp_timeout(void *udata1, void *udata2) {
  (void)udata1;
  (void)udata2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* *****************************************************************************
Round 2 Runner
***************************************************************************** */

/** Runs Round 2 tests in multi-process mode. Returns 0 on success. */
FIO_SFUNC int redis_run_round2(void) {
  fprintf(stderr, "\t* Round 2: multi-process mode (1 worker, IPC paths)\n");

  /* Reset all state */
  mp_ack_ping = 0;
  mp_ack_set = 0;
  mp_ack_get = 0;
  mp_ack_del = 0;
  mp_pubsub_received = 0;
  mp_master_ping = 0;
  mp_pubsub_sub = 0;

  /* Create a fresh Redis engine for Round 2.
   * fio_redis_new defers the pub_conn connect to the IO queue.
   * Do NOT call fio_pubsub_engine_attach here — the subscription connection
   * requires the IO reactor to be running. Attach from a master-side timer. */
  mp_redis = fio_redis_new(.url = "redis://localhost:6379");
  FIO_ASSERT_ALLOC(mp_redis);

  /* Register worker startup callback */
  fio_state_callback_add(FIO_CALL_ON_START, mp_worker_start, NULL);

  /* Schedule master-side setup after Redis pub_conn is established.
   * This attaches to pub/sub, subscribes, and sends master PING. */
  fio_io_run_every(.fn = mp_master_setup_timer,
                   .every = 1000,
                   .repetitions = 1);

  /* Schedule timeout */
  fio_io_run_every(.fn = mp_timeout, .every = MP_TIMEOUT_MS, .repetitions = 1);

  /* Start reactor with 1 worker (this blocks until fio_io_stop) */
  fio_io_start(1);

  /* === Cleanup (master only, after reactor stops) === */
  fio_state_callback_remove(FIO_CALL_ON_START, mp_worker_start, NULL);

  if (mp_pubsub_sub) {
    fio_pubsub_unsubscribe(.subscription_handle_ptr = &mp_pubsub_sub);
    mp_pubsub_sub = 0;
  }
  fio_pubsub_engine_detach(mp_redis);
  fio_redis_free(mp_redis);
  mp_redis = NULL;

  /* === Verify results === */
  fprintf(stderr, "\t  - Worker PING (IPC round-trip): ");
  FIO_ASSERT(mp_ack_ping >= 1,
             "Worker PING ack not received (got %zu)",
             mp_ack_ping);
  fprintf(stderr, "PASS (got %zu)\n", mp_ack_ping);

  fprintf(stderr, "\t  - Worker SET (IPC round-trip): ");
  FIO_ASSERT(mp_ack_set >= 1,
             "Worker SET ack not received (got %zu)",
             mp_ack_set);
  fprintf(stderr, "PASS (got %zu)\n", mp_ack_set);

  fprintf(stderr, "\t  - Worker GET (IPC round-trip + data integrity): ");
  FIO_ASSERT(mp_ack_get >= 1,
             "Worker GET ack not received (got %zu)",
             mp_ack_get);
  fprintf(stderr, "PASS (got %zu)\n", mp_ack_get);

  fprintf(stderr, "\t  - Worker DEL (IPC round-trip): ");
  FIO_ASSERT(mp_ack_del >= 1,
             "Worker DEL ack not received (got %zu)",
             mp_ack_del);
  fprintf(stderr, "PASS (got %zu)\n", mp_ack_del);

  fprintf(stderr, "\t  - Worker pub/sub publish (vtable-swapped path): ");
  FIO_ASSERT(mp_pubsub_received >= 1,
             "Worker pubsub message not received (got %zu)",
             mp_pubsub_received);
  fprintf(stderr, "PASS (got %zu)\n", mp_pubsub_received);

  fprintf(stderr, "\t  - Master PING (direct path in multi-process): ");
  FIO_ASSERT(mp_master_ping >= 1,
             "Master PING not received (got %zu)",
             mp_master_ping);
  fprintf(stderr, "PASS (got %zu)\n", mp_master_ping);

  fprintf(stderr, "\t  Round 2 PASS\n");
  return 0;
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  if (!redis_is_available()) {
    fprintf(stderr, "SKIPPED! no local redis database to test against\n");
    return 0;
  }

  /* Round 1: Single-process mode (master-direct paths) */
  if (redis_run_round1())
    return 1;

  /* Round 2: Multi-process mode (IPC worker paths) */
  if (redis_run_round2())
    return 1;

  return 0;
}
