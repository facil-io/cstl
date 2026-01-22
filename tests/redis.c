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
      FIO_LOG_DDEBUG("  PASS: PING returned PONG");
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
    FIO_LOG_DDEBUG("  PASS: SET/GET working correctly");
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

  FIO_LOG_DDEBUG("  PASS: INCR working correctly");
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
    FIO_LOG_DDEBUG("  PASS: Pub/Sub roundtrip message received");
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
      FIO_LOG_DDEBUG("  PASS: Deduplication working (2 messages, one per "
                     "subscription)");
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
    FIO_LOG_DDEBUG("Testing Redis PING command");
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
    FIO_LOG_DDEBUG("Testing Redis SET/GET commands");
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
    FIO_LOG_DDEBUG("Testing Redis INCR command");
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
    FIO_LOG_DDEBUG("Testing Redis Pub/Sub roundtrip");
    /* Reset state */
    test_state.pubsub_msg_received = 0;

    /* Attach Redis engine to pub/sub system, keep a reference for use here */
    fio_pubsub_engine_attach(fio_redis_dup(redis));

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
    FIO_LOG_DDEBUG("Testing Redis Pub/Sub pattern deduplication");

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
    FIO_LOG_DDEBUG("Redis tests complete!");

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
On Start Callback - Called when IO reactor starts
***************************************************************************** */

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_LOG_DDEBUG("==================================");
  FIO_LOG_DDEBUG("Testing Redis Module");

  if (!redis_is_available()) {
    fprintf(stderr, "SKIPPED! no local redis database to test against\n");
    return 0;
  }

  FIO_LOG_DDEBUG("(Redis detected on localhost:6379)");

  /* Create Redis engine */
  fio_pubsub_engine_s *redis = fio_redis_new(.url = "redis://localhost:6379");
  FIO_ASSERT_ALLOC(redis);
  test_state.redis = redis;

  /* Start timeout watchdog - checks every 3 seconds for progress */
  fio_io_run_every(.fn = redis_timeout_watchdog,
                   .every = 3000,
                   .repetitions = -1); /* Run until stopped */

  /* Schedule tests to run after a short delay to allow connection */
  fio_io_run_every(.fn = redis_start_tests_timer,
                   .every = 500,
                   .repetitions = 1);

  /* Start the IO reactor (this blocks until fio_io_stop is called) */
  fio_io_start(0);
  fio_redis_free(redis);

  /* Check if tests completed successfully */
  if (test_state.test_failed || test_state.phase != TEST_PHASE_DONE) {
    FIO_LOG_ERROR("FAIL: Tests did not complete successfully");
    return 1;
  }

  return 0;
}
