/* *****************************************************************************
Stress - Redis command client and Pub/Sub engine (422 redis.h)

Uses only cstl's public Redis, Pub/Sub, FIOBJ, IO, logging, random, time, and
memory APIs. The test targets the default Redis URL (localhost:6379), requires
a successful RESP3 HELLO handshake, and skips with a warning when no RESP3
server is available.

The workload covers strings, binary/large values, multi-key commands,
counters, bit operations, lists, hashes, sets, sorted sets, expiration,
transactions, scripting, streams, scanning, server introspection, command
queue pressure, exact subscriptions, pattern subscriptions, binary Pub/Sub
payloads, large push frames, and unsubscribe behavior.
***************************************************************************** */
#define FIO_LOG
#define FIO_REDIS
#include "fio-stl/include.h"

/* ****************************************************************************
Configuration
***************************************************************************** */

#define REDIS_STRESS_DEFAULT_ROUNDS       128U
#define REDIS_STRESS_MAX_ROUNDS           512U
#define REDIS_STRESS_EXPECTATION_CAPACITY 1024U
#define REDIS_STRESS_LARGE_VALUE_SIZE     (1U << 20)
#define REDIS_STRESS_LARGE_PUSH_SIZE      (256U << 10)
#define REDIS_STRESS_CONNECT_TIMEOUT_MS   3000U
#define REDIS_STRESS_MAGIC                0x4353544CU /* "CSTL" */

/* ****************************************************************************
Types and state
***************************************************************************** */

typedef struct {
  const char *buf;
  size_t len;
} redis_stress_arg_s;

typedef enum {
  REDIS_STRESS_EXPECT_BYTES,
  REDIS_STRESS_EXPECT_INTEGER,
  REDIS_STRESS_EXPECT_INTEGER_GE,
  REDIS_STRESS_EXPECT_FLOAT,
  REDIS_STRESS_EXPECT_NULL,
  REDIS_STRESS_EXPECT_ARRAY_COUNT,
  REDIS_STRESS_EXPECT_HASH_COUNT,
  REDIS_STRESS_EXPECT_TYPE,
} redis_stress_expect_kind_e;

typedef struct {
  const char *label;
  const void *data;
  size_t len;
  int64_t integer;
  double floating;
  uintptr_t type;
  redis_stress_expect_kind_e kind;
} redis_stress_expect_s;

typedef enum {
  REDIS_STRESS_CONNECTING,
  REDIS_STRESS_COMMANDS,
  REDIS_STRESS_PUBLISHING,
  REDIS_STRESS_UNSUBSCRIBING,
  REDIS_STRESS_POST_UNSUBSCRIBE,
  REDIS_STRESS_CLEANUP,
  REDIS_STRESS_DONE,
  REDIS_STRESS_SKIPPED,
} redis_stress_phase_e;

enum {
  REDIS_STRESS_KEY_STRING,
  REDIS_STRESS_KEY_MULTI_1,
  REDIS_STRESS_KEY_MULTI_2,
  REDIS_STRESS_KEY_COUNTER,
  REDIS_STRESS_KEY_QUEUE_COUNTER,
  REDIS_STRESS_KEY_BITS,
  REDIS_STRESS_KEY_LIST,
  REDIS_STRESS_KEY_HASH,
  REDIS_STRESS_KEY_SET,
  REDIS_STRESS_KEY_ZSET,
  REDIS_STRESS_KEY_EXPIRE,
  REDIS_STRESS_KEY_STREAM,
  REDIS_STRESS_KEY_TRANSACTION,
  REDIS_STRESS_KEY_BINARY,
  REDIS_STRESS_KEY_SETNX,
  REDIS_STRESS_KEY_COUNT,
};

static struct {
  fio_pubsub_engine_s *engine;
  redis_stress_phase_e phase;
  size_t rounds;
  size_t failures;
  size_t commands_sent;
  size_t commands_done;
  size_t exact_received;
  size_t pattern_received;
  size_t exact_before_unsubscribe;
  size_t pattern_before_unsubscribe;
  size_t expectation_count;
  size_t overall_timeout_ms;
  redis_stress_expect_s expectations[REDIS_STRESS_EXPECTATION_CAPACITY];
  char prefix[64];
  char keys[REDIS_STRESS_KEY_COUNT][96];
  char exact_channel[96];
  char pattern_channel[96];
  char pattern_glob[96];
  char scan_glob[96];
  char rounds_text[32];
  uint8_t *large_value;
  uint8_t *publish_buffer;
  uint8_t *exact_seen;
  uint8_t *pattern_seen;
} redis_stress = {0};

/* ****************************************************************************
Diagnostics and reply validation
***************************************************************************** */

#define REDIS_STRESS_FAIL(...)                                                 \
  do {                                                                         \
    ++redis_stress.failures;                                                   \
    FIO_LOG_ERROR(__VA_ARGS__);                                                \
  } while (0)

static int redis_stress_reply_is_pong(FIOBJ reply, const char *label) {
  if (!reply || reply == FIOBJ_INVALID || FIOBJ_TYPE(reply) != FIOBJ_T_STRING) {
    REDIS_STRESS_FAIL("%s returned an invalid/non-string reply", label);
    return 0;
  }
  fio_str_info_s s = fiobj2cstr(reply);
  if (s.len != 4 || FIO_MEMCMP(s.buf, "PONG", 4)) {
    REDIS_STRESS_FAIL("%s expected PONG, got %.*s", label, (int)s.len, s.buf);
    return 0;
  }
  return 1;
}

static void redis_stress_on_reply(fio_pubsub_engine_s *engine,
                                  FIOBJ reply,
                                  void *udata) {
  redis_stress_expect_s *expect = (redis_stress_expect_s *)udata;
  (void)engine;
  ++redis_stress.commands_done;
  if (!expect) {
    REDIS_STRESS_FAIL("Redis command callback lost its expectation");
    return;
  }
  if (!reply || reply == FIOBJ_INVALID) {
    REDIS_STRESS_FAIL("%s returned FIOBJ_INVALID", expect->label);
    return;
  }

  switch (expect->kind) {
  case REDIS_STRESS_EXPECT_BYTES: {
    if (FIOBJ_TYPE(reply) != FIOBJ_T_STRING) {
      REDIS_STRESS_FAIL("%s expected string type, got %zu",
                        expect->label,
                        (size_t)FIOBJ_TYPE(reply));
      return;
    }
    fio_str_info_s s = fiobj2cstr(reply);
    if (s.len != expect->len ||
        (s.len && FIO_MEMCMP(s.buf, expect->data, s.len)))
      REDIS_STRESS_FAIL("%s returned unexpected bytes (expected %zu, got %zu)",
                        expect->label,
                        expect->len,
                        s.len);
    return;
  }
  case REDIS_STRESS_EXPECT_INTEGER:
    if (FIOBJ_TYPE(reply) != FIOBJ_T_NUMBER ||
        fiobj2i(reply) != expect->integer)
      REDIS_STRESS_FAIL("%s expected integer %lld, got type=%zu value=%lld",
                        expect->label,
                        (long long)expect->integer,
                        (size_t)FIOBJ_TYPE(reply),
                        (long long)fiobj2i(reply));
    return;
  case REDIS_STRESS_EXPECT_INTEGER_GE:
    if (FIOBJ_TYPE(reply) != FIOBJ_T_NUMBER || fiobj2i(reply) < expect->integer)
      REDIS_STRESS_FAIL("%s expected integer >= %lld, got type=%zu value=%lld",
                        expect->label,
                        (long long)expect->integer,
                        (size_t)FIOBJ_TYPE(reply),
                        (long long)fiobj2i(reply));
    return;
  case REDIS_STRESS_EXPECT_FLOAT:
    if (FIOBJ_TYPE(reply) != FIOBJ_T_FLOAT ||
        fiobj2f(reply) != expect->floating)
      REDIS_STRESS_FAIL("%s expected float %.17g, got type=%zu value=%.17g",
                        expect->label,
                        expect->floating,
                        (size_t)FIOBJ_TYPE(reply),
                        fiobj2f(reply));
    return;
  case REDIS_STRESS_EXPECT_NULL:
    if (reply != fiobj_null())
      REDIS_STRESS_FAIL("%s expected a null reply, got type=%zu",
                        expect->label,
                        (size_t)FIOBJ_TYPE(reply));
    return;
  case REDIS_STRESS_EXPECT_ARRAY_COUNT:
    if (FIOBJ_TYPE(reply) != FIOBJ_T_ARRAY ||
        fiobj_array_count(reply) != expect->len)
      REDIS_STRESS_FAIL("%s expected array[%zu], got type=%zu count=%zu",
                        expect->label,
                        expect->len,
                        (size_t)FIOBJ_TYPE(reply),
                        FIOBJ_TYPE(reply) == FIOBJ_T_ARRAY
                            ? (size_t)fiobj_array_count(reply)
                            : 0);
    return;
  case REDIS_STRESS_EXPECT_HASH_COUNT:
    if (FIOBJ_TYPE(reply) != FIOBJ_T_HASH ||
        fiobj_hash_count(reply) != expect->len)
      REDIS_STRESS_FAIL("%s expected RESP3 map[%zu], got type=%zu count=%zu",
                        expect->label,
                        expect->len,
                        (size_t)FIOBJ_TYPE(reply),
                        FIOBJ_TYPE(reply) == FIOBJ_T_HASH
                            ? (size_t)fiobj_hash_count(reply)
                            : 0);
    return;
  case REDIS_STRESS_EXPECT_TYPE:
    if (FIOBJ_TYPE(reply) != expect->type)
      REDIS_STRESS_FAIL("%s expected FIOBJ type %zu, got %zu",
                        expect->label,
                        (size_t)expect->type,
                        (size_t)FIOBJ_TYPE(reply));
    return;
  }
  REDIS_STRESS_FAIL("%s has an unknown expectation kind", expect->label);
}

/* ****************************************************************************
Public command construction helpers
***************************************************************************** */

static int redis_stress_send(redis_stress_expect_s expect,
                             size_t argc,
                             const redis_stress_arg_s *argv) {
  if (!argc || !argv ||
      redis_stress.expectation_count >= REDIS_STRESS_EXPECTATION_CAPACITY) {
    REDIS_STRESS_FAIL("cannot queue %s: invalid arguments or expectation cap",
                      expect.label ? expect.label : "command");
    return -1;
  }
  FIOBJ command = fiobj_array_new();
  if (!command || command == FIOBJ_INVALID) {
    REDIS_STRESS_FAIL("cannot allocate command array for %s", expect.label);
    return -1;
  }
  for (size_t i = 0; i < argc; ++i)
    fiobj_array_push(command, fiobj_str_new_cstr(argv[i].buf, argv[i].len));

  redis_stress_expect_s *stored =
      redis_stress.expectations + redis_stress.expectation_count++;
  *stored = expect;
  int result = fio_redis_send(redis_stress.engine,
                              command,
                              redis_stress_on_reply,
                              stored);
  fiobj_free(command);
  if (result) {
    --redis_stress.expectation_count;
    REDIS_STRESS_FAIL("fio_redis_send rejected %s", expect.label);
    return -1;
  }
  ++redis_stress.commands_sent;
  return 0;
}

static int redis_stress_send_ping(const char *label,
                                  void (*callback)(fio_pubsub_engine_s *,
                                                   FIOBJ,
                                                   void *)) {
  FIOBJ command = fiobj_array_new();
  fiobj_array_push(command, fiobj_str_new_cstr("PING", 4));
  int result = fio_redis_send(redis_stress.engine, command, callback, NULL);
  fiobj_free(command);
  if (result) {
    REDIS_STRESS_FAIL("fio_redis_send rejected %s", label);
    fio_io_stop();
    return -1;
  }
  return 0;
}

#define REDIS_STRESS_ARG_LITERAL(s)                                            \
  ((redis_stress_arg_s){.buf = (s), .len = sizeof(s) - 1})
#define REDIS_STRESS_ARG_BUFFER(p, n)                                          \
  ((redis_stress_arg_s){.buf = (const char *)(p), .len = (n)})
#define REDIS_STRESS_SEND(expect, ...)                                         \
  redis_stress_send((expect),                                                  \
                    sizeof((redis_stress_arg_s[]){__VA_ARGS__}) /              \
                        sizeof(redis_stress_arg_s),                            \
                    (redis_stress_arg_s[]){__VA_ARGS__})

/* ****************************************************************************
Pub/Sub payload generation and validation
***************************************************************************** */

static size_t redis_stress_payload_size(size_t index, uint32_t kind) {
  if (index + 1 == redis_stress.rounds)
    return REDIS_STRESS_LARGE_PUSH_SIZE;
  return 128U + ((index * 37U + kind * 11U) & 255U);
}

static void redis_stress_payload_fill(size_t index, uint32_t kind, size_t len) {
  uint32_t header[4] = {
      REDIS_STRESS_MAGIC,
      (uint32_t)index,
      kind,
      (uint32_t)len,
  };
  FIO_MEMCPY(redis_stress.publish_buffer, header, sizeof(header));
  for (size_t i = sizeof(header); i < len; ++i)
    redis_stress.publish_buffer[i] =
        (uint8_t)((index * 31U + kind * 17U + i) & 255U);
}

static void redis_stress_validate_message(fio_pubsub_msg_s *msg,
                                          uint32_t expected_kind,
                                          const char *expected_channel,
                                          uint8_t *seen,
                                          size_t *received) {
  if (msg->channel.len != FIO_STRLEN(expected_channel) ||
      FIO_MEMCMP(msg->channel.buf, expected_channel, msg->channel.len)) {
    REDIS_STRESS_FAIL("Pub/Sub kind %u arrived on unexpected channel %.*s",
                      expected_kind,
                      (int)msg->channel.len,
                      msg->channel.buf);
    return;
  }
  if (msg->message.len < 16) {
    REDIS_STRESS_FAIL("Pub/Sub kind %u payload too short (%zu)",
                      expected_kind,
                      msg->message.len);
    return;
  }
  uint32_t header[4];
  FIO_MEMCPY(header, msg->message.buf, sizeof(header));
  size_t index = header[1];
  if (header[0] != REDIS_STRESS_MAGIC || header[2] != expected_kind ||
      header[3] != msg->message.len || index >= redis_stress.rounds) {
    REDIS_STRESS_FAIL("Pub/Sub kind %u has corrupt header "
                      "(magic=%08x index=%zu kind=%u len=%u/%zu)",
                      expected_kind,
                      header[0],
                      index,
                      header[2],
                      header[3],
                      msg->message.len);
    return;
  }
  size_t expected_len = redis_stress_payload_size(index, expected_kind);
  if (msg->message.len != expected_len) {
    REDIS_STRESS_FAIL("Pub/Sub kind %u index %zu expected %zu bytes, got %zu",
                      expected_kind,
                      index,
                      expected_len,
                      msg->message.len);
    return;
  }
  const uint8_t *bytes = (const uint8_t *)msg->message.buf;
  for (size_t i = sizeof(header); i < msg->message.len; ++i) {
    uint8_t expected =
        (uint8_t)((index * 31U + expected_kind * 17U + i) & 255U);
    if (bytes[i] != expected) {
      REDIS_STRESS_FAIL("Pub/Sub kind %u index %zu corrupt at byte %zu",
                        expected_kind,
                        index,
                        i);
      return;
    }
  }
  if (seen[index]) {
    REDIS_STRESS_FAIL("Pub/Sub kind %u index %zu was delivered more than once",
                      expected_kind,
                      index);
    return;
  }
  seen[index] = 1;
  ++*received;
}

static void redis_stress_on_exact_message(fio_pubsub_msg_s *msg) {
  redis_stress_validate_message(msg,
                                1,
                                redis_stress.exact_channel,
                                redis_stress.exact_seen,
                                &redis_stress.exact_received);
}

static void redis_stress_on_pattern_message(fio_pubsub_msg_s *msg) {
  redis_stress_validate_message(msg,
                                2,
                                redis_stress.pattern_channel,
                                redis_stress.pattern_seen,
                                &redis_stress.pattern_received);
}

static void redis_stress_publish_round(size_t index) {
  size_t len = redis_stress_payload_size(index, 1);
  redis_stress_payload_fill(index, 1, len);
  fio_pubsub_publish(.engine = redis_stress.engine,
                     .channel =
                         FIO_BUF_INFO2(redis_stress.exact_channel,
                                       FIO_STRLEN(redis_stress.exact_channel)),
                     .message =
                         FIO_BUF_INFO2((char *)redis_stress.publish_buffer,
                                       len));

  len = redis_stress_payload_size(index, 2);
  redis_stress_payload_fill(index, 2, len);
  fio_pubsub_publish(
          .engine = redis_stress.engine,
          .channel = FIO_BUF_INFO2(redis_stress.pattern_channel,
                                   FIO_STRLEN(redis_stress.pattern_channel)),
          .message = FIO_BUF_INFO2((char *)redis_stress.publish_buffer, len));
}

/* ****************************************************************************
Command workload
***************************************************************************** */

static void redis_stress_on_work_barrier(fio_pubsub_engine_s *, FIOBJ, void *);

static void redis_stress_queue_work(void) {
#define K(n)       redis_stress.keys[(n)]
#define KEY_ARG(n) REDIS_STRESS_ARG_BUFFER(K(n), FIO_STRLEN(K(n)))
#define EXPECT_BYTES(label_, bytes_, len_)                                     \
  ((redis_stress_expect_s){.label = (label_),                                  \
                           .data = (bytes_),                                   \
                           .len = (len_),                                      \
                           .kind = REDIS_STRESS_EXPECT_BYTES})
#define EXPECT_LITERAL(label_, literal_)                                       \
  EXPECT_BYTES((label_), (literal_), sizeof(literal_) - 1)
#define EXPECT_INT(label_, value_)                                             \
  ((redis_stress_expect_s){.label = (label_),                                  \
                           .integer = (value_),                                \
                           .kind = REDIS_STRESS_EXPECT_INTEGER})
#define EXPECT_INT_GE(label_, value_)                                          \
  ((redis_stress_expect_s){.label = (label_),                                  \
                           .integer = (value_),                                \
                           .kind = REDIS_STRESS_EXPECT_INTEGER_GE})
#define EXPECT_FLOAT(label_, value_)                                           \
  ((redis_stress_expect_s){.label = (label_),                                  \
                           .floating = (value_),                               \
                           .kind = REDIS_STRESS_EXPECT_FLOAT})
#define EXPECT_NULL(label_)                                                    \
  ((redis_stress_expect_s){.label = (label_), .kind = REDIS_STRESS_EXPECT_NULL})
#define EXPECT_ARRAY(label_, count_)                                           \
  ((redis_stress_expect_s){.label = (label_),                                  \
                           .len = (count_),                                    \
                           .kind = REDIS_STRESS_EXPECT_ARRAY_COUNT})
#define EXPECT_HASH(label_, count_)                                            \
  ((redis_stress_expect_s){.label = (label_),                                  \
                           .len = (count_),                                    \
                           .kind = REDIS_STRESS_EXPECT_HASH_COUNT})
#define EXPECT_TYPE(label_, type_)                                             \
  ((redis_stress_expect_s){.label = (label_),                                  \
                           .type = (type_),                                    \
                           .kind = REDIS_STRESS_EXPECT_TYPE})

  /* Start from a clean, unique namespace without touching unrelated data. */
  REDIS_STRESS_SEND(EXPECT_INT_GE("initial DEL", 0),
                    REDIS_STRESS_ARG_LITERAL("DEL"),
                    KEY_ARG(REDIS_STRESS_KEY_STRING),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_1),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_2),
                    KEY_ARG(REDIS_STRESS_KEY_COUNTER),
                    KEY_ARG(REDIS_STRESS_KEY_QUEUE_COUNTER),
                    KEY_ARG(REDIS_STRESS_KEY_BITS),
                    KEY_ARG(REDIS_STRESS_KEY_LIST),
                    KEY_ARG(REDIS_STRESS_KEY_HASH),
                    KEY_ARG(REDIS_STRESS_KEY_SET),
                    KEY_ARG(REDIS_STRESS_KEY_ZSET),
                    KEY_ARG(REDIS_STRESS_KEY_EXPIRE),
                    KEY_ARG(REDIS_STRESS_KEY_STREAM),
                    KEY_ARG(REDIS_STRESS_KEY_TRANSACTION),
                    KEY_ARG(REDIS_STRESS_KEY_BINARY),
                    KEY_ARG(REDIS_STRESS_KEY_SETNX));

  REDIS_STRESS_SEND(EXPECT_LITERAL("PING", "PONG"),
                    REDIS_STRESS_ARG_LITERAL("PING"));
  REDIS_STRESS_SEND(EXPECT_LITERAL("ECHO", "redis-stress"),
                    REDIS_STRESS_ARG_LITERAL("ECHO"),
                    REDIS_STRESS_ARG_LITERAL("redis-stress"));

  /* Strings, multi-key commands, conditional writes, and binary data. */
  REDIS_STRESS_SEND(EXPECT_LITERAL("SET string", "OK"),
                    REDIS_STRESS_ARG_LITERAL("SET"),
                    KEY_ARG(REDIS_STRESS_KEY_STRING),
                    REDIS_STRESS_ARG_LITERAL("alpha"));
  REDIS_STRESS_SEND(EXPECT_INT("APPEND", 10),
                    REDIS_STRESS_ARG_LITERAL("APPEND"),
                    KEY_ARG(REDIS_STRESS_KEY_STRING),
                    REDIS_STRESS_ARG_LITERAL("-beta"));
  REDIS_STRESS_SEND(EXPECT_LITERAL("GET string", "alpha-beta"),
                    REDIS_STRESS_ARG_LITERAL("GET"),
                    KEY_ARG(REDIS_STRESS_KEY_STRING));
  REDIS_STRESS_SEND(EXPECT_INT("STRLEN", 10),
                    REDIS_STRESS_ARG_LITERAL("STRLEN"),
                    KEY_ARG(REDIS_STRESS_KEY_STRING));
  REDIS_STRESS_SEND(EXPECT_LITERAL("TYPE string", "string"),
                    REDIS_STRESS_ARG_LITERAL("TYPE"),
                    KEY_ARG(REDIS_STRESS_KEY_STRING));
  REDIS_STRESS_SEND(EXPECT_LITERAL("MSET", "OK"),
                    REDIS_STRESS_ARG_LITERAL("MSET"),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_1),
                    REDIS_STRESS_ARG_LITERAL("one"),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_2),
                    REDIS_STRESS_ARG_LITERAL("two"));
  REDIS_STRESS_SEND(EXPECT_ARRAY("MGET", 2),
                    REDIS_STRESS_ARG_LITERAL("MGET"),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_1),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_2));
  REDIS_STRESS_SEND(EXPECT_INT("EXISTS", 2),
                    REDIS_STRESS_ARG_LITERAL("EXISTS"),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_1),
                    KEY_ARG(REDIS_STRESS_KEY_MULTI_2));
  REDIS_STRESS_SEND(EXPECT_INT("SETNX first", 1),
                    REDIS_STRESS_ARG_LITERAL("SETNX"),
                    KEY_ARG(REDIS_STRESS_KEY_SETNX),
                    REDIS_STRESS_ARG_LITERAL("first"));
  REDIS_STRESS_SEND(EXPECT_INT("SETNX second", 0),
                    REDIS_STRESS_ARG_LITERAL("SETNX"),
                    KEY_ARG(REDIS_STRESS_KEY_SETNX),
                    REDIS_STRESS_ARG_LITERAL("second"));
  REDIS_STRESS_SEND(EXPECT_LITERAL("SET binary", "OK"),
                    REDIS_STRESS_ARG_LITERAL("SET"),
                    KEY_ARG(REDIS_STRESS_KEY_BINARY),
                    REDIS_STRESS_ARG_BUFFER(redis_stress.large_value,
                                            REDIS_STRESS_LARGE_VALUE_SIZE));
  REDIS_STRESS_SEND(EXPECT_BYTES("GET binary",
                                 redis_stress.large_value,
                                 REDIS_STRESS_LARGE_VALUE_SIZE),
                    REDIS_STRESS_ARG_LITERAL("GET"),
                    KEY_ARG(REDIS_STRESS_KEY_BINARY));

  /* Numbers, bit operations, and a deliberately deep command queue. */
  REDIS_STRESS_SEND(EXPECT_LITERAL("SET counter", "OK"),
                    REDIS_STRESS_ARG_LITERAL("SET"),
                    KEY_ARG(REDIS_STRESS_KEY_COUNTER),
                    REDIS_STRESS_ARG_LITERAL("0"));
  REDIS_STRESS_SEND(EXPECT_INT("INCR", 1),
                    REDIS_STRESS_ARG_LITERAL("INCR"),
                    KEY_ARG(REDIS_STRESS_KEY_COUNTER));
  REDIS_STRESS_SEND(EXPECT_INT("INCRBY", 42),
                    REDIS_STRESS_ARG_LITERAL("INCRBY"),
                    KEY_ARG(REDIS_STRESS_KEY_COUNTER),
                    REDIS_STRESS_ARG_LITERAL("41"));
  REDIS_STRESS_SEND(EXPECT_INT("DECR", 41),
                    REDIS_STRESS_ARG_LITERAL("DECR"),
                    KEY_ARG(REDIS_STRESS_KEY_COUNTER));
  REDIS_STRESS_SEND(EXPECT_LITERAL("SET queue counter", "OK"),
                    REDIS_STRESS_ARG_LITERAL("SET"),
                    KEY_ARG(REDIS_STRESS_KEY_QUEUE_COUNTER),
                    REDIS_STRESS_ARG_LITERAL("0"));
  for (size_t i = 1; i <= redis_stress.rounds; ++i)
    REDIS_STRESS_SEND(EXPECT_INT("queued INCR", (int64_t)i),
                      REDIS_STRESS_ARG_LITERAL("INCR"),
                      KEY_ARG(REDIS_STRESS_KEY_QUEUE_COUNTER));
  REDIS_STRESS_SEND(EXPECT_BYTES("GET queue counter",
                                 redis_stress.rounds_text,
                                 FIO_STRLEN(redis_stress.rounds_text)),
                    REDIS_STRESS_ARG_LITERAL("GET"),
                    KEY_ARG(REDIS_STRESS_KEY_QUEUE_COUNTER));
  REDIS_STRESS_SEND(EXPECT_INT("SETBIT", 0),
                    REDIS_STRESS_ARG_LITERAL("SETBIT"),
                    KEY_ARG(REDIS_STRESS_KEY_BITS),
                    REDIS_STRESS_ARG_LITERAL("7"),
                    REDIS_STRESS_ARG_LITERAL("1"));
  REDIS_STRESS_SEND(EXPECT_INT("GETBIT", 1),
                    REDIS_STRESS_ARG_LITERAL("GETBIT"),
                    KEY_ARG(REDIS_STRESS_KEY_BITS),
                    REDIS_STRESS_ARG_LITERAL("7"));

  /* Lists: push from both ends, inspect, and pop from both ends. */
  REDIS_STRESS_SEND(EXPECT_INT("LPUSH", 2),
                    REDIS_STRESS_ARG_LITERAL("LPUSH"),
                    KEY_ARG(REDIS_STRESS_KEY_LIST),
                    REDIS_STRESS_ARG_LITERAL("b"),
                    REDIS_STRESS_ARG_LITERAL("a"));
  REDIS_STRESS_SEND(EXPECT_INT("RPUSH", 3),
                    REDIS_STRESS_ARG_LITERAL("RPUSH"),
                    KEY_ARG(REDIS_STRESS_KEY_LIST),
                    REDIS_STRESS_ARG_LITERAL("c"));
  REDIS_STRESS_SEND(EXPECT_INT("LLEN", 3),
                    REDIS_STRESS_ARG_LITERAL("LLEN"),
                    KEY_ARG(REDIS_STRESS_KEY_LIST));
  REDIS_STRESS_SEND(EXPECT_LITERAL("LPOP", "a"),
                    REDIS_STRESS_ARG_LITERAL("LPOP"),
                    KEY_ARG(REDIS_STRESS_KEY_LIST));
  REDIS_STRESS_SEND(EXPECT_LITERAL("RPOP", "c"),
                    REDIS_STRESS_ARG_LITERAL("RPOP"),
                    KEY_ARG(REDIS_STRESS_KEY_LIST));
  REDIS_STRESS_SEND(EXPECT_ARRAY("LRANGE", 1),
                    REDIS_STRESS_ARG_LITERAL("LRANGE"),
                    KEY_ARG(REDIS_STRESS_KEY_LIST),
                    REDIS_STRESS_ARG_LITERAL("0"),
                    REDIS_STRESS_ARG_LITERAL("-1"));

  /* Hashes exercise RESP3 map replies. */
  REDIS_STRESS_SEND(EXPECT_INT("HSET", 2),
                    REDIS_STRESS_ARG_LITERAL("HSET"),
                    KEY_ARG(REDIS_STRESS_KEY_HASH),
                    REDIS_STRESS_ARG_LITERAL("field1"),
                    REDIS_STRESS_ARG_LITERAL("value1"),
                    REDIS_STRESS_ARG_LITERAL("field2"),
                    REDIS_STRESS_ARG_LITERAL("value2"));
  REDIS_STRESS_SEND(EXPECT_LITERAL("HGET", "value1"),
                    REDIS_STRESS_ARG_LITERAL("HGET"),
                    KEY_ARG(REDIS_STRESS_KEY_HASH),
                    REDIS_STRESS_ARG_LITERAL("field1"));
  REDIS_STRESS_SEND(EXPECT_INT("HEXISTS", 1),
                    REDIS_STRESS_ARG_LITERAL("HEXISTS"),
                    KEY_ARG(REDIS_STRESS_KEY_HASH),
                    REDIS_STRESS_ARG_LITERAL("field2"));
  REDIS_STRESS_SEND(EXPECT_HASH("HGETALL RESP3 map", 2),
                    REDIS_STRESS_ARG_LITERAL("HGETALL"),
                    KEY_ARG(REDIS_STRESS_KEY_HASH));
  REDIS_STRESS_SEND(EXPECT_INT("HDEL", 1),
                    REDIS_STRESS_ARG_LITERAL("HDEL"),
                    KEY_ARG(REDIS_STRESS_KEY_HASH),
                    REDIS_STRESS_ARG_LITERAL("field2"));
  REDIS_STRESS_SEND(EXPECT_INT("HLEN", 1),
                    REDIS_STRESS_ARG_LITERAL("HLEN"),
                    KEY_ARG(REDIS_STRESS_KEY_HASH));

  /* Unordered and sorted sets, including pop operations. */
  REDIS_STRESS_SEND(EXPECT_INT("SADD", 3),
                    REDIS_STRESS_ARG_LITERAL("SADD"),
                    KEY_ARG(REDIS_STRESS_KEY_SET),
                    REDIS_STRESS_ARG_LITERAL("a"),
                    REDIS_STRESS_ARG_LITERAL("b"),
                    REDIS_STRESS_ARG_LITERAL("c"));
  REDIS_STRESS_SEND(EXPECT_INT("SISMEMBER", 1),
                    REDIS_STRESS_ARG_LITERAL("SISMEMBER"),
                    KEY_ARG(REDIS_STRESS_KEY_SET),
                    REDIS_STRESS_ARG_LITERAL("b"));
  REDIS_STRESS_SEND(EXPECT_HASH("SMEMBERS RESP3 set", 3),
                    REDIS_STRESS_ARG_LITERAL("SMEMBERS"),
                    KEY_ARG(REDIS_STRESS_KEY_SET));
  REDIS_STRESS_SEND(EXPECT_TYPE("SPOP", FIOBJ_T_STRING),
                    REDIS_STRESS_ARG_LITERAL("SPOP"),
                    KEY_ARG(REDIS_STRESS_KEY_SET));
  REDIS_STRESS_SEND(EXPECT_INT("SCARD after SPOP", 2),
                    REDIS_STRESS_ARG_LITERAL("SCARD"),
                    KEY_ARG(REDIS_STRESS_KEY_SET));
  REDIS_STRESS_SEND(EXPECT_INT("ZADD", 3),
                    REDIS_STRESS_ARG_LITERAL("ZADD"),
                    KEY_ARG(REDIS_STRESS_KEY_ZSET),
                    REDIS_STRESS_ARG_LITERAL("1"),
                    REDIS_STRESS_ARG_LITERAL("one"),
                    REDIS_STRESS_ARG_LITERAL("2"),
                    REDIS_STRESS_ARG_LITERAL("two"),
                    REDIS_STRESS_ARG_LITERAL("3"),
                    REDIS_STRESS_ARG_LITERAL("three"));
  REDIS_STRESS_SEND(EXPECT_INT("ZCARD", 3),
                    REDIS_STRESS_ARG_LITERAL("ZCARD"),
                    KEY_ARG(REDIS_STRESS_KEY_ZSET));
  REDIS_STRESS_SEND(EXPECT_FLOAT("ZSCORE", 2.0),
                    REDIS_STRESS_ARG_LITERAL("ZSCORE"),
                    KEY_ARG(REDIS_STRESS_KEY_ZSET),
                    REDIS_STRESS_ARG_LITERAL("two"));
  REDIS_STRESS_SEND(EXPECT_TYPE("ZPOPMIN", FIOBJ_T_ARRAY),
                    REDIS_STRESS_ARG_LITERAL("ZPOPMIN"),
                    KEY_ARG(REDIS_STRESS_KEY_ZSET),
                    REDIS_STRESS_ARG_LITERAL("1"));
  REDIS_STRESS_SEND(EXPECT_INT("ZCARD after ZPOPMIN", 2),
                    REDIS_STRESS_ARG_LITERAL("ZCARD"),
                    KEY_ARG(REDIS_STRESS_KEY_ZSET));

  /* Expiration, transactions, scripting, streams, and server metadata. */
  REDIS_STRESS_SEND(EXPECT_LITERAL("SET expiring", "OK"),
                    REDIS_STRESS_ARG_LITERAL("SET"),
                    KEY_ARG(REDIS_STRESS_KEY_EXPIRE),
                    REDIS_STRESS_ARG_LITERAL("temporary"));
  REDIS_STRESS_SEND(EXPECT_INT("EXPIRE", 1),
                    REDIS_STRESS_ARG_LITERAL("EXPIRE"),
                    KEY_ARG(REDIS_STRESS_KEY_EXPIRE),
                    REDIS_STRESS_ARG_LITERAL("60"));
  REDIS_STRESS_SEND(EXPECT_INT_GE("TTL", 0),
                    REDIS_STRESS_ARG_LITERAL("TTL"),
                    KEY_ARG(REDIS_STRESS_KEY_EXPIRE));
  REDIS_STRESS_SEND(EXPECT_INT("PERSIST", 1),
                    REDIS_STRESS_ARG_LITERAL("PERSIST"),
                    KEY_ARG(REDIS_STRESS_KEY_EXPIRE));
  REDIS_STRESS_SEND(EXPECT_INT("TTL persistent", -1),
                    REDIS_STRESS_ARG_LITERAL("TTL"),
                    KEY_ARG(REDIS_STRESS_KEY_EXPIRE));

  REDIS_STRESS_SEND(EXPECT_LITERAL("MULTI", "OK"),
                    REDIS_STRESS_ARG_LITERAL("MULTI"));
  REDIS_STRESS_SEND(EXPECT_LITERAL("MULTI SET queued", "QUEUED"),
                    REDIS_STRESS_ARG_LITERAL("SET"),
                    KEY_ARG(REDIS_STRESS_KEY_TRANSACTION),
                    REDIS_STRESS_ARG_LITERAL("0"));
  REDIS_STRESS_SEND(EXPECT_LITERAL("MULTI INCR queued", "QUEUED"),
                    REDIS_STRESS_ARG_LITERAL("INCR"),
                    KEY_ARG(REDIS_STRESS_KEY_TRANSACTION));
  REDIS_STRESS_SEND(EXPECT_LITERAL("MULTI GET queued", "QUEUED"),
                    REDIS_STRESS_ARG_LITERAL("GET"),
                    KEY_ARG(REDIS_STRESS_KEY_TRANSACTION));
  REDIS_STRESS_SEND(EXPECT_ARRAY("EXEC", 3), REDIS_STRESS_ARG_LITERAL("EXEC"));
  REDIS_STRESS_SEND(EXPECT_ARRAY("EVAL", 2),
                    REDIS_STRESS_ARG_LITERAL("EVAL"),
                    REDIS_STRESS_ARG_LITERAL("return {ARGV[1], 42}"),
                    REDIS_STRESS_ARG_LITERAL("0"),
                    REDIS_STRESS_ARG_LITERAL("lua-ok"));
  REDIS_STRESS_SEND(EXPECT_TYPE("XADD", FIOBJ_T_STRING),
                    REDIS_STRESS_ARG_LITERAL("XADD"),
                    KEY_ARG(REDIS_STRESS_KEY_STREAM),
                    REDIS_STRESS_ARG_LITERAL("*"),
                    REDIS_STRESS_ARG_LITERAL("field"),
                    REDIS_STRESS_ARG_LITERAL("value"));
  REDIS_STRESS_SEND(EXPECT_INT("XLEN", 1),
                    REDIS_STRESS_ARG_LITERAL("XLEN"),
                    KEY_ARG(REDIS_STRESS_KEY_STREAM));
  REDIS_STRESS_SEND(EXPECT_ARRAY("XRANGE", 1),
                    REDIS_STRESS_ARG_LITERAL("XRANGE"),
                    KEY_ARG(REDIS_STRESS_KEY_STREAM),
                    REDIS_STRESS_ARG_LITERAL("-"),
                    REDIS_STRESS_ARG_LITERAL("+"));
  REDIS_STRESS_SEND(EXPECT_ARRAY("SCAN", 2),
                    REDIS_STRESS_ARG_LITERAL("SCAN"),
                    REDIS_STRESS_ARG_LITERAL("0"),
                    REDIS_STRESS_ARG_LITERAL("MATCH"),
                    REDIS_STRESS_ARG_BUFFER(redis_stress.scan_glob,
                                            FIO_STRLEN(redis_stress.scan_glob)),
                    REDIS_STRESS_ARG_LITERAL("COUNT"),
                    REDIS_STRESS_ARG_LITERAL("1000"));
  REDIS_STRESS_SEND(EXPECT_INT_GE("DBSIZE", 0),
                    REDIS_STRESS_ARG_LITERAL("DBSIZE"));
  REDIS_STRESS_SEND(EXPECT_ARRAY("TIME", 2), REDIS_STRESS_ARG_LITERAL("TIME"));
  REDIS_STRESS_SEND(EXPECT_TYPE("INFO SERVER", FIOBJ_T_STRING),
                    REDIS_STRESS_ARG_LITERAL("INFO"),
                    REDIS_STRESS_ARG_LITERAL("SERVER"));
  REDIS_STRESS_SEND(EXPECT_INT_GE("COMMAND COUNT", 1),
                    REDIS_STRESS_ARG_LITERAL("COMMAND"),
                    REDIS_STRESS_ARG_LITERAL("COUNT"));
  REDIS_STRESS_SEND(EXPECT_INT("PUBLISH without subscribers", 0),
                    REDIS_STRESS_ARG_LITERAL("PUBLISH"),
                    REDIS_STRESS_ARG_LITERAL("cstl:redis:stress:unused"),
                    REDIS_STRESS_ARG_LITERAL("unused"));

  /* Bound residue if a later stress phase exposes a disconnect or crash. */
  for (size_t i = 0; i < REDIS_STRESS_KEY_COUNT; ++i)
    REDIS_STRESS_SEND(EXPECT_INT("safety EXPIRE", 1),
                      REDIS_STRESS_ARG_LITERAL("EXPIRE"),
                      KEY_ARG(i),
                      REDIS_STRESS_ARG_LITERAL("300"));

  redis_stress_send_ping("command workload barrier",
                         redis_stress_on_work_barrier);

#undef EXPECT_TYPE
#undef EXPECT_HASH
#undef EXPECT_ARRAY
#undef EXPECT_NULL
#undef EXPECT_FLOAT
#undef EXPECT_INT_GE
#undef EXPECT_INT
#undef EXPECT_LITERAL
#undef EXPECT_BYTES
#undef KEY_ARG
#undef K
}

/* ****************************************************************************
Pub/Sub and cleanup state machine
***************************************************************************** */

static void redis_stress_on_publish_barrier(fio_pubsub_engine_s *,
                                            FIOBJ,
                                            void *);
static void redis_stress_on_unsubscribe_barrier(fio_pubsub_engine_s *,
                                                FIOBJ,
                                                void *);
static void redis_stress_on_post_unsubscribe_barrier(fio_pubsub_engine_s *,
                                                     FIOBJ,
                                                     void *);

static void redis_stress_on_work_barrier(fio_pubsub_engine_s *engine,
                                         FIOBJ reply,
                                         void *udata) {
  (void)engine;
  (void)udata;
  redis_stress_reply_is_pong(reply, "command workload barrier");
  if (redis_stress.commands_done != redis_stress.commands_sent)
    REDIS_STRESS_FAIL("command barrier reached with %zu/%zu callbacks",
                      redis_stress.commands_done,
                      redis_stress.commands_sent);
  redis_stress.phase = REDIS_STRESS_PUBLISHING;
  for (size_t i = 0; i < redis_stress.rounds; ++i)
    redis_stress_publish_round(i);
  redis_stress_send_ping("publish barrier", redis_stress_on_publish_barrier);
}

static int redis_stress_wait_for_messages(void *u1, void *u2) {
  (void)u1;
  (void)u2;
  if (redis_stress.exact_received < redis_stress.rounds ||
      redis_stress.pattern_received < redis_stress.rounds)
    return 0;
  if (redis_stress.exact_received != redis_stress.rounds ||
      redis_stress.pattern_received != redis_stress.rounds)
    REDIS_STRESS_FAIL("Pub/Sub delivery count mismatch (exact=%zu pattern=%zu "
                      "expected=%zu)",
                      redis_stress.exact_received,
                      redis_stress.pattern_received,
                      redis_stress.rounds);

  redis_stress.phase = REDIS_STRESS_UNSUBSCRIBING;
  if (fio_pubsub_unsubscribe(.channel = FIO_BUF_INFO2(
                                 redis_stress.exact_channel,
                                 FIO_STRLEN(redis_stress.exact_channel)),
                             .on_message = redis_stress_on_exact_message))
    REDIS_STRESS_FAIL("exact-channel unsubscribe was rejected");
  if (fio_pubsub_unsubscribe(.channel = FIO_BUF_INFO2(
                                 redis_stress.pattern_glob,
                                 FIO_STRLEN(redis_stress.pattern_glob)),
                             .on_message = redis_stress_on_pattern_message,
                             .is_pattern = 1))
    REDIS_STRESS_FAIL("pattern unsubscribe was rejected");
  redis_stress_send_ping("unsubscribe barrier",
                         redis_stress_on_unsubscribe_barrier);
  return -1;
}

static void redis_stress_on_publish_barrier(fio_pubsub_engine_s *engine,
                                            FIOBJ reply,
                                            void *udata) {
  (void)engine;
  (void)udata;
  redis_stress_reply_is_pong(reply, "publish barrier");
  fio_io_run_every(.fn = redis_stress_wait_for_messages,
                   .every = 10,
                   .repetitions = -1);
}

static void redis_stress_on_unsubscribe_barrier(fio_pubsub_engine_s *engine,
                                                FIOBJ reply,
                                                void *udata) {
  (void)engine;
  (void)udata;
  redis_stress_reply_is_pong(reply, "unsubscribe barrier");
  redis_stress.exact_before_unsubscribe = redis_stress.exact_received;
  redis_stress.pattern_before_unsubscribe = redis_stress.pattern_received;
  redis_stress.phase = REDIS_STRESS_POST_UNSUBSCRIBE;
  redis_stress_publish_round(0);
  redis_stress_send_ping("post-unsubscribe publish barrier",
                         redis_stress_on_post_unsubscribe_barrier);
}

static void redis_stress_on_cleanup(fio_pubsub_engine_s *engine,
                                    FIOBJ reply,
                                    void *udata) {
  (void)engine;
  (void)udata;
  if (!reply || reply == FIOBJ_INVALID || FIOBJ_TYPE(reply) != FIOBJ_T_NUMBER ||
      fiobj2i(reply) < 0)
    REDIS_STRESS_FAIL("cleanup DEL returned an invalid reply");
  redis_stress.phase = REDIS_STRESS_DONE;
  fio_io_stop();
}

static void redis_stress_queue_cleanup(void) {
  redis_stress.phase = REDIS_STRESS_CLEANUP;
  FIOBJ command = fiobj_array_new();
  fiobj_array_push(command, fiobj_str_new_cstr("DEL", 3));
  for (size_t i = 0; i < REDIS_STRESS_KEY_COUNT; ++i)
    fiobj_array_push(command,
                     fiobj_str_new_cstr(redis_stress.keys[i],
                                        FIO_STRLEN(redis_stress.keys[i])));
  int result = fio_redis_send(redis_stress.engine,
                              command,
                              redis_stress_on_cleanup,
                              NULL);
  fiobj_free(command);
  if (result) {
    REDIS_STRESS_FAIL("fio_redis_send rejected cleanup DEL");
    fio_io_stop();
  }
}

static int redis_stress_finish_post_unsubscribe(void *u1, void *u2) {
  (void)u1;
  (void)u2;
  if (redis_stress.exact_received != redis_stress.exact_before_unsubscribe ||
      redis_stress.pattern_received != redis_stress.pattern_before_unsubscribe)
    REDIS_STRESS_FAIL("messages arrived after unsubscribe "
                      "(exact %zu->%zu, pattern %zu->%zu)",
                      redis_stress.exact_before_unsubscribe,
                      redis_stress.exact_received,
                      redis_stress.pattern_before_unsubscribe,
                      redis_stress.pattern_received);
  redis_stress_queue_cleanup();
  return -1;
}

static void redis_stress_on_post_unsubscribe_barrier(
    fio_pubsub_engine_s *engine,
    FIOBJ reply,
    void *udata) {
  (void)engine;
  (void)udata;
  redis_stress_reply_is_pong(reply, "post-unsubscribe publish barrier");
  fio_io_run_every(.fn = redis_stress_finish_post_unsubscribe,
                   .every = 100,
                   .repetitions = 1);
}

/* ****************************************************************************
Connection probing and reactor lifecycle
***************************************************************************** */

static void redis_stress_skip(const char *reason) {
  redis_stress.phase = REDIS_STRESS_SKIPPED;
  FIO_LOG_WARNING("SKIPPED - no RESP3 database available at the default "
                  "Redis URL redis://localhost:6379 (%s)",
                  reason);
  fio_io_stop();
}

static int redis_stress_poll_connection(void *u1, void *u2) {
  (void)u1;
  (void)u2;
  fio_redis_state_e state = fio_redis_state(redis_stress.engine);
  if (state == FIO_REDIS_STATE_ERROR) {
    redis_stress_skip("HELLO 3 failed");
    return -1;
  }
  if (state != FIO_REDIS_STATE_CONNECTED)
    return 0;
  redis_stress.phase = REDIS_STRESS_COMMANDS;
  redis_stress_queue_work();
  return -1;
}

static int redis_stress_connect_timeout(void *u1, void *u2) {
  (void)u1;
  (void)u2;
  if (redis_stress.phase == REDIS_STRESS_CONNECTING)
    redis_stress_skip("connection timed out");
  return -1;
}

static int redis_stress_overall_timeout(void *u1, void *u2) {
  (void)u1;
  (void)u2;
  if (redis_stress.phase == REDIS_STRESS_DONE ||
      redis_stress.phase == REDIS_STRESS_SKIPPED)
    return -1;
  REDIS_STRESS_FAIL("Redis stress test timed out in phase %d "
                    "(commands=%zu/%zu exact=%zu/%zu pattern=%zu/%zu)",
                    (int)redis_stress.phase,
                    redis_stress.commands_done,
                    redis_stress.commands_sent,
                    redis_stress.exact_received,
                    redis_stress.rounds,
                    redis_stress.pattern_received,
                    redis_stress.rounds);
  fio_io_stop();
  return -1;
}

static void redis_stress_on_start(void *udata) {
  (void)udata;
  fio_pubsub_subscribe(.channel = FIO_BUF_INFO2(
                           redis_stress.exact_channel,
                           FIO_STRLEN(redis_stress.exact_channel)),
                       .on_message = redis_stress_on_exact_message);
  fio_pubsub_subscribe(.channel =
                           FIO_BUF_INFO2(redis_stress.pattern_glob,
                                         FIO_STRLEN(redis_stress.pattern_glob)),
                       .on_message = redis_stress_on_pattern_message,
                       .is_pattern = 1);
  fio_io_run_every(.fn = redis_stress_poll_connection,
                   .every = 10,
                   .repetitions = -1);
  fio_io_run_every(.fn = redis_stress_connect_timeout,
                   .every = REDIS_STRESS_CONNECT_TIMEOUT_MS,
                   .repetitions = 1);
  fio_io_run_every(.fn = redis_stress_overall_timeout,
                   .every = redis_stress.overall_timeout_ms,
                   .repetitions = 1);
}

/* ****************************************************************************
Setup, teardown, and main
***************************************************************************** */

static void redis_stress_init(void) {
  redis_stress.phase = REDIS_STRESS_CONNECTING;
  redis_stress.rounds = REDIS_STRESS_DEFAULT_ROUNDS;
  const char *rounds_env = getenv("FIO_REDIS_STRESS_ROUNDS");
  if (rounds_env && rounds_env[0]) {
    unsigned long parsed = strtoul(rounds_env, NULL, 10);
    if (parsed)
      redis_stress.rounds = parsed;
  }
  if (redis_stress.rounds > REDIS_STRESS_MAX_ROUNDS)
    redis_stress.rounds = REDIS_STRESS_MAX_ROUNDS;
  redis_stress.overall_timeout_ms = 15000U + redis_stress.rounds * 100U;

  snprintf(redis_stress.prefix,
           sizeof(redis_stress.prefix),
           "cstl:redis:stress:%016llx",
           (unsigned long long)fio_rand64());
  static const char *suffixes[REDIS_STRESS_KEY_COUNT] = {
      "string",
      "multi1",
      "multi2",
      "counter",
      "queue-counter",
      "bits",
      "list",
      "hash",
      "set",
      "zset",
      "expire",
      "stream",
      "tx",
      "binary",
      "setnx",
  };
  for (size_t i = 0; i < REDIS_STRESS_KEY_COUNT; ++i)
    snprintf(redis_stress.keys[i],
             sizeof(redis_stress.keys[i]),
             "%s:%s",
             redis_stress.prefix,
             suffixes[i]);
  snprintf(redis_stress.exact_channel,
           sizeof(redis_stress.exact_channel),
           "%s:exact",
           redis_stress.prefix);
  snprintf(redis_stress.pattern_channel,
           sizeof(redis_stress.pattern_channel),
           "%s:pattern:one",
           redis_stress.prefix);
  snprintf(redis_stress.pattern_glob,
           sizeof(redis_stress.pattern_glob),
           "%s:pattern:*",
           redis_stress.prefix);
  snprintf(redis_stress.scan_glob,
           sizeof(redis_stress.scan_glob),
           "%s:*",
           redis_stress.prefix);
  snprintf(redis_stress.rounds_text,
           sizeof(redis_stress.rounds_text),
           "%zu",
           redis_stress.rounds);

  redis_stress.large_value =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, REDIS_STRESS_LARGE_VALUE_SIZE, 0);
  redis_stress.publish_buffer =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, REDIS_STRESS_LARGE_PUSH_SIZE, 0);
  redis_stress.exact_seen =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, redis_stress.rounds, 0);
  redis_stress.pattern_seen =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, redis_stress.rounds, 0);
  FIO_ASSERT(redis_stress.large_value && redis_stress.publish_buffer &&
                 redis_stress.exact_seen && redis_stress.pattern_seen,
             "Redis stress test allocation failed");
  FIO_MEMSET(redis_stress.exact_seen, 0, redis_stress.rounds);
  FIO_MEMSET(redis_stress.pattern_seen, 0, redis_stress.rounds);
  for (size_t i = 0; i < REDIS_STRESS_LARGE_VALUE_SIZE; ++i)
    redis_stress.large_value[i] = (uint8_t)((i * 29U + 7U) & 255U);
}

static void redis_stress_destroy(void) {
  if (redis_stress.large_value)
    FIO_MEM_FREE(redis_stress.large_value, REDIS_STRESS_LARGE_VALUE_SIZE);
  if (redis_stress.publish_buffer)
    FIO_MEM_FREE(redis_stress.publish_buffer, REDIS_STRESS_LARGE_PUSH_SIZE);
  if (redis_stress.exact_seen)
    FIO_MEM_FREE(redis_stress.exact_seen, redis_stress.rounds);
  if (redis_stress.pattern_seen)
    FIO_MEM_FREE(redis_stress.pattern_seen, redis_stress.rounds);
}

int main(void) {
  fprintf(stderr,
          "=== Redis command/Pub-Sub stress test (default RESP3 URL) ===\n");
  redis_stress_init();

  redis_stress.engine =
      fio_redis_new(.url = NULL, .ping_interval = 5, .payload_limit = 0);
  FIO_ASSERT(redis_stress.engine, "fio_redis_new failed");
  fio_redis_dup(redis_stress.engine);            /* keep caller reference */
  fio_pubsub_engine_attach(redis_stress.engine); /* system takes original */

  fio_state_callback_add(FIO_CALL_ON_START, redis_stress_on_start, NULL);
  fio_io_start(0);
  fio_state_callback_remove(FIO_CALL_ON_START, redis_stress_on_start, NULL);

  /* Safe even when the state machine already unsubscribed or the run skipped.
   */
  (void)fio_pubsub_unsubscribe(.channel = FIO_BUF_INFO2(
                                   redis_stress.exact_channel,
                                   FIO_STRLEN(redis_stress.exact_channel)),
                               .on_message = redis_stress_on_exact_message);
  (void)fio_pubsub_unsubscribe(.channel = FIO_BUF_INFO2(
                                   redis_stress.pattern_glob,
                                   FIO_STRLEN(redis_stress.pattern_glob)),
                               .on_message = redis_stress_on_pattern_message,
                               .is_pattern = 1);
  fio_pubsub_engine_detach(redis_stress.engine);
  fio_redis_free(redis_stress.engine);
  redis_stress.engine = NULL;

  int result = redis_stress.failures ? 1 : 0;
  if (redis_stress.phase == REDIS_STRESS_SKIPPED) {
    fprintf(stderr, "=== Redis stress test SKIPPED ===\n");
    result = 0;
  } else if (result) {
    fprintf(stderr,
            "=== Redis stress test FAILED (%zu issue%s) ===\n",
            redis_stress.failures,
            redis_stress.failures == 1 ? "" : "s");
  } else {
    fprintf(stderr,
            "=== Redis stress test passed: %zu command callbacks, "
            "%zu exact + %zu pattern messages ===\n",
            redis_stress.commands_done,
            redis_stress.exact_received,
            redis_stress.pattern_received);
  }
  redis_stress_destroy();
  return result;
}
