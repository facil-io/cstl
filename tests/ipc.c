/* *****************************************************************************
Test - IPC (Inter-Process Communication) correctness (404 ipc.h)

Fast, deterministic, single-process coverage of the public and internal
IPC API.  No performance loops, no benchmarking, no external processes, and
no long-running reactor sessions.

POSIX-only paths log FIO_LOG_WARNING("SKIPPED") and return success on
non-POSIX platforms.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IPC
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test State - Global Variables for Callback Tracking
***************************************************************************** */

static volatile int fio___test_ipc_call_count = 0;
static volatile int fio___test_ipc_reply_count = 0;
static volatile int fio___test_ipc_done_count = 0;

static char fio___test_ipc_received_data[1024];
static size_t fio___test_ipc_received_len = 0;
static void *fio___test_ipc_received_udata = NULL;
static uint16_t fio___test_ipc_received_flags = 0;

static void fio___test_ipc_reset_state(void) {
  fio___test_ipc_call_count = 0;
  fio___test_ipc_reply_count = 0;
  fio___test_ipc_done_count = 0;
  FIO_MEMSET(fio___test_ipc_received_data,
             0,
             sizeof(fio___test_ipc_received_data));
  fio___test_ipc_received_len = 0;
  fio___test_ipc_received_udata = NULL;
  fio___test_ipc_received_flags = 0;
}

/* *****************************************************************************
Test Callbacks
***************************************************************************** */

static void fio___test_ipc_call_simple(fio_ipc_s *msg) {
  ++fio___test_ipc_call_count;
  (void)msg;
}

static void fio___test_ipc_call_capture(fio_ipc_s *msg) {
  ++fio___test_ipc_call_count;
  if (msg->len > 0 && msg->len < sizeof(fio___test_ipc_received_data)) {
    FIO_MEMCPY(fio___test_ipc_received_data, msg->data, msg->len);
    fio___test_ipc_received_len = msg->len;
  }
  fio___test_ipc_received_udata = msg->udata;
  fio___test_ipc_received_flags = msg->flags;
}

static void fio___test_ipc_call_with_reply(fio_ipc_s *msg) {
  ++fio___test_ipc_call_count;
  const char *reply_data = "reply_data";
  fio_ipc_reply(msg,
                .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)reply_data)),
                .done = 1);
}

static void fio___test_ipc_call_streaming(fio_ipc_s *msg) {
  ++fio___test_ipc_call_count;
  const char *chunk1 = "chunk1";
  const char *chunk2 = "chunk2";
  const char *chunk3 = "done";
  fio_ipc_reply(msg,
                .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)chunk1)),
                .done = 0);
  fio_ipc_reply(msg,
                .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)chunk2)),
                .done = 0);
  fio_ipc_reply(msg,
                .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)chunk3)),
                .done = 1);
}

static void fio___test_ipc_on_reply(fio_ipc_s *msg) {
  ++fio___test_ipc_reply_count;
  if (msg->len > 0 && msg->len < sizeof(fio___test_ipc_received_data)) {
    FIO_MEMCPY(fio___test_ipc_received_data, msg->data, msg->len);
    fio___test_ipc_received_len = msg->len;
  }
}

static void fio___test_ipc_on_done(fio_ipc_s *msg) {
  ++fio___test_ipc_done_count;
  (void)msg;
}

/* *****************************************************************************
Test: Message Lifecycle - Reference Counting
***************************************************************************** */

static void test_ipc_message_lifecycle(void) {
  /* Create message with zero data */
  {
    fio_ipc_s *msg = fio___ipc_new(16); /* +16 for MAC */
    FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");
    msg->from = NULL;
    msg->len = 0;
    msg->flags = 0;
    msg->routing_flags = 0;
    msg->timestamp = 12345;
    msg->id = 67890;
    msg->call = NULL;
    msg->on_reply = NULL;
    msg->on_done = NULL;
    msg->udata = NULL;

    FIO_ASSERT(msg->timestamp == 12345, "timestamp should be preserved");
    FIO_ASSERT(msg->id == 67890, "id should be preserved");

    fio___ipc_free(msg);
  }

  /* Create message with small data */
  {
    const char *test_data = "Hello, IPC!";
    size_t data_len = FIO_STRLEN(test_data);
    fio_ipc_s *msg = fio___ipc_new(data_len + 16);
    FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");
    msg->from = NULL;

    msg->len = (uint32_t)data_len;
    FIO_MEMCPY(msg->data, test_data, data_len);

    FIO_ASSERT(msg->len == data_len, "len should match data length");
    FIO_ASSERT(FIO_MEMCMP(msg->data, test_data, data_len) == 0,
               "data should match input");

    fio___ipc_free(msg);
  }

  /* Create message with large data */
  {
    size_t large_len = 8192;
    fio_ipc_s *msg = fio___ipc_new(large_len + 16);
    FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate large message");
    msg->from = NULL;

    msg->len = (uint32_t)large_len;
    FIO_MEMSET(msg->data, 0xAB, large_len);

    for (size_t i = 0; i < large_len; ++i) {
      FIO_ASSERT((uint8_t)msg->data[i] == 0xAB,
                 "large message data should be preserved at byte %zu",
                 i);
    }

    fio___ipc_free(msg);
  }

  /* Reference counting with dup/free */
  {
    fio_ipc_s *msg = fio___ipc_new(32);
    FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");
    msg->from = NULL;
    msg->len = 5;
    FIO_MEMCPY(msg->data, "test", 5);

    fio_ipc_s *dup1 = fio_ipc_dup(msg);
    FIO_ASSERT(dup1 == msg, "fio_ipc_dup should return same pointer");

    fio_ipc_s *dup2 = fio_ipc_dup(msg);
    FIO_ASSERT(dup2 == msg, "fio_ipc_dup should return same pointer");

    fio_ipc_free(dup1);
    fio_ipc_free(dup2);

    FIO_ASSERT(msg->len == 5,
               "message should still be valid after freeing dups");
    FIO_ASSERT(FIO_MEMCMP(msg->data, "test", 5) == 0,
               "message data should be preserved");

    fio_ipc_free(msg);
  }

  /* NULL handling */
  {
    fio_ipc_s *null_dup = fio_ipc_dup(NULL);
    FIO_ASSERT(null_dup == NULL, "fio_ipc_dup(NULL) should return NULL");

    fio_ipc_free(NULL);
  }
}

/* *****************************************************************************
Test: URL Management
***************************************************************************** */

static void test_ipc_url_management(void) {
  /* Get default URL (auto-generated at init) */
  {
    const char *url = fio_ipc_url();
    FIO_ASSERT(url != NULL, "fio_ipc_url should return non-NULL URL");
    FIO_ASSERT(FIO_STRLEN(url) > 0, "IPC URL should not be empty");
    FIO_ASSERT(FIO_STRLEN(url) < FIO_IPC_URL_MAX_LENGTH,
               "IPC URL should be within max length");
  }

  /* URL starts with expected prefix */
  {
    const char *url = fio_ipc_url();
    FIO_ASSERT(url != NULL, "fio_ipc_url should return non-NULL URL");
    FIO_ASSERT(FIO_STRLEN(url) >= 7 && FIO_MEMCMP(url, "unix://", 7) == 0,
               "IPC URL should start with 'unix://'");
  }

  /* Set custom URL */
  {
    const char *custom_url = "unix://test_ipc_url.sock";
    int result = fio_ipc_url_set(custom_url);
    FIO_ASSERT(result == 0, "fio_ipc_url_set should succeed on master");
    const char *url = fio_ipc_url();
    FIO_ASSERT(url != NULL, "fio_ipc_url should return non-NULL after set");
    FIO_ASSERT(FIO_MEMCMP(url, custom_url, FIO_STRLEN(custom_url)) == 0,
               "IPC URL should match custom URL");
  }

  /* Set NULL URL (auto-generate) */
  {
    int result = fio_ipc_url_set(NULL);
    FIO_ASSERT(result == 0, "fio_ipc_url_set(NULL) should succeed");

    const char *url = fio_ipc_url();
    FIO_ASSERT(url != NULL,
               "fio_ipc_url should return non-NULL after NULL set");
    FIO_ASSERT(FIO_STRLEN(url) > 0, "Auto-generated URL should not be empty");
  }

  /* URL length limit */
  {
    char long_url[FIO_IPC_URL_MAX_LENGTH + 100];
    FIO_MEMSET(long_url, 'a', sizeof(long_url) - 1);
    long_url[sizeof(long_url) - 1] = '\0';

    int result = fio_ipc_url_set(long_url);
    FIO_ASSERT(result == -1, "fio_ipc_url_set should reject too-long URL");
  }

  fio_ipc_url_set(NULL);
}

/* *****************************************************************************
Test: Message Structure Fields
***************************************************************************** */

static void test_ipc_message_fields(void) {
  /* All fields are properly initialized and accessible */
  {
    fio_ipc_s *msg = fio___ipc_new(64);
    FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");
    msg->from = NULL;

    msg->len = 42;
    msg->flags = 0x1234;
    msg->routing_flags = 0x5678;
    msg->timestamp = 0xDEADBEEFCAFEBABEULL;
    msg->id = 0x123456789ABCDEF0ULL;
    msg->call = fio___test_ipc_call_simple;
    msg->on_reply = fio___test_ipc_on_reply;
    msg->on_done = fio___test_ipc_on_done;
    msg->udata = (void *)0xABCDEF00;

    FIO_ASSERT(msg->len == 42, "len field error");
    FIO_ASSERT(msg->flags == 0x1234, "flags field error");
    FIO_ASSERT(msg->routing_flags == 0x5678, "routing_flags field error");
    FIO_ASSERT(msg->timestamp == 0xDEADBEEFCAFEBABEULL,
               "timestamp field error");
    FIO_ASSERT(msg->id == 0x123456789ABCDEF0ULL, "id field error");
    FIO_ASSERT(msg->call == fio___test_ipc_call_simple, "call field error");
    FIO_ASSERT(msg->on_reply == fio___test_ipc_on_reply,
               "on_reply field error");
    FIO_ASSERT(msg->on_done == fio___test_ipc_on_done, "on_done field error");
    FIO_ASSERT(msg->udata == (void *)0xABCDEF00, "udata field error");

    fio___ipc_free(msg);
  }

  /* Flexible array member (data[]) */
  {
    const char *test_data = "This is test data for the flexible array member";
    size_t data_len = FIO_STRLEN(test_data);
    fio_ipc_s *msg = fio___ipc_new(data_len + 16);
    FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");
    msg->from = NULL;

    msg->len = (uint32_t)data_len;
    FIO_MEMCPY(msg->data, test_data, data_len);

    FIO_ASSERT(FIO_MEMCMP(msg->data, test_data, data_len) == 0,
               "data[] should contain copied data");

    fio___ipc_free(msg);
  }

  /* sizeof(fio_ipc_s) is reasonable */
  {
    FIO_ASSERT(sizeof(fio_ipc_s) >= 64,
               "fio_ipc_s should be at least 64 bytes");
    FIO_ASSERT(sizeof(fio_ipc_s) <= 256,
               "fio_ipc_s should not be excessively large");
  }
}

/* *****************************************************************************
Test: Error Handling - NULL and Invalid Inputs
***************************************************************************** */

static void test_ipc_error_handling(void) {
  fio___test_ipc_reset_state();

  /* fio_ipc_call with NULL call function (should be no-op) */
  {
    fio_ipc_call(.call = NULL,
                 .on_reply = fio___test_ipc_on_reply,
                 .on_done = fio___test_ipc_on_done,
                 .udata = (void *)0x1234,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"test")));

    FIO_ASSERT(fio___test_ipc_call_count == 0,
               "NULL call should not invoke any callback");
  }

  /* fio_ipc_reply with NULL ipc (should be no-op) */
  {
    fio_ipc_reply(NULL,
                  .done = 1,
                  .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"test")));
  }

  /* fio_ipc_local with NULL call (should be no-op) */
  {
    fio_ipc_local(.call = NULL,
                  .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"test")));
  }

  /* fio_ipc_dup with NULL (should return NULL) */
  {
    fio_ipc_s *result = fio_ipc_dup(NULL);
    FIO_ASSERT(result == NULL, "fio_ipc_dup(NULL) should return NULL");
  }

  /* fio_ipc_free with NULL (should be safe no-op) */
  { fio_ipc_free(NULL); }
}

/* *****************************************************************************
Test: Data Integrity Through Call/Reply Cycle
***************************************************************************** */

static void test_ipc_data_integrity(void) {
  fio___test_ipc_reset_state();

  /* Data is correctly copied in fio_ipc_call */
  {
    const char *test_data = "integrity_test_data_12345";
    void *test_udata = (void *)0xCAFEBABE;

    fio_ipc_call(.call = fio___test_ipc_call_capture,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = test_udata,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)test_data)));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked");
    FIO_ASSERT(fio___test_ipc_received_len == FIO_STRLEN(test_data),
               "received data length should match");
    FIO_ASSERT(FIO_MEMCMP(fio___test_ipc_received_data,
                          test_data,
                          FIO_STRLEN(test_data)) == 0,
               "received data should match sent data");
    FIO_ASSERT(fio___test_ipc_received_udata == test_udata,
               "udata should be preserved");
  }

  fio___test_ipc_reset_state();

  /* Empty data */
  {
    fio_ipc_call(.call = fio___test_ipc_call_capture,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked for empty data");
    FIO_ASSERT(fio___test_ipc_received_len == 0,
               "received data length should be 0 for empty request");
  }

  fio___test_ipc_reset_state();

  /* Binary data with null bytes */
  {
    unsigned char binary_data[] =
        {0x00, 0x01, 0x02, 0x00, 0xFF, 0xFE, 0x00, 0x03};
    size_t binary_len = sizeof(binary_data);

    fio_ipc_call(.call = fio___test_ipc_call_capture,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)binary_data, binary_len)));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked for binary data");
    FIO_ASSERT(fio___test_ipc_received_len == binary_len,
               "received binary data length should match");
    FIO_ASSERT(
        FIO_MEMCMP(fio___test_ipc_received_data, binary_data, binary_len) == 0,
        "received binary data should match");
  }
}

/* *****************************************************************************
Test: Master Process Direct Execution
***************************************************************************** */

static void test_ipc_master_execution(void) {
  FIO_ASSERT(fio_io_is_master(), "Test should run as master process");

  fio___test_ipc_reset_state();

  /* fio_ipc_call executes callback on master (deferred) */
  {
    fio_ipc_call(.call = fio___test_ipc_call_simple,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    FIO_ASSERT(fio___test_ipc_call_count == 0,
               "call should be deferred, not immediate");

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked after queue processing");
  }

  fio___test_ipc_reset_state();

  /* Multiple calls are all executed */
  {
    for (int i = 0; i < 5; ++i) {
      fio_ipc_call(.call = fio___test_ipc_call_simple,
                   .on_reply = NULL,
                   .on_done = NULL,
                   .udata = NULL,
                   .data = FIO_IPC_DATA(FIO_BUF_INFO0));
    }

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 5,
               "all 5 call callbacks should be invoked");
  }
}

/* *****************************************************************************
Test: Reply Mechanism
***************************************************************************** */

static void test_ipc_reply_mechanism(void) {
  fio___test_ipc_reset_state();

  /* Reply with done=1 triggers on_done (not on_reply) */
  {
    fio_ipc_call(.call = fio___test_ipc_call_with_reply,
                 .on_reply = fio___test_ipc_on_reply,
                 .on_done = fio___test_ipc_on_done,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked");
    FIO_ASSERT(fio___test_ipc_reply_count == 0,
               "on_reply should NOT be invoked when done=1");
    FIO_ASSERT(fio___test_ipc_done_count == 1,
               "on_done callback should be invoked when done=1");
  }

  fio___test_ipc_reset_state();

  /* Streaming replies (multiple replies before done) */
  {
    fio_ipc_call(.call = fio___test_ipc_call_streaming,
                 .on_reply = fio___test_ipc_on_reply,
                 .on_done = fio___test_ipc_on_done,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked");
    FIO_ASSERT(
        fio___test_ipc_reply_count == 2,
        "on_reply should be called 2 times for intermediate streaming replies");
    FIO_ASSERT(fio___test_ipc_done_count == 1,
               "on_done should be called once at the end");
  }
}

/* *****************************************************************************
Test: Flags Preservation
***************************************************************************** */

static void test_ipc_flags_preservation(void) {
  fio___test_ipc_reset_state();

  /* User flags are preserved through message lifecycle */
  {
    fio_ipc_s *msg = fio___ipc_new(32);
    FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");

    msg->len = 4;
    msg->flags = 0xABCD;
    msg->routing_flags = 0;
    msg->timestamp = fio_time_milli();
    msg->id = fio_rand64();
    msg->from = NULL;
    msg->call = fio___test_ipc_call_capture;
    msg->on_reply = NULL;
    msg->on_done = NULL;
    msg->udata = NULL;
    FIO_MEMCPY(msg->data, "test", 4);

    FIO_ASSERT(msg->flags == 0xABCD, "flags should be set correctly");

    fio___ipc_free(msg);
  }
}

/* *****************************************************************************
Test: Broadcast Mechanism (Single Process)
***************************************************************************** */

static void test_ipc_broadcast(void) {
  fio___test_ipc_reset_state();

  /* Local broadcast with NULL call is a no-op */
  {
    fio_ipc_local(.call = NULL,
                  .on_reply = NULL,
                  .on_done = NULL,
                  .udata = NULL,
                  .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 0,
               "local broadcast with NULL call should be no-op");
  }

  fio___test_ipc_reset_state();

  /* Local broadcast creates message correctly (even with no workers) */
  {
    fio_ipc_local(.call = fio___test_ipc_call_simple,
                  .on_reply = NULL,
                  .on_done = NULL,
                  .udata = NULL,
                  .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    fio_queue_perform_all(fio_io_queue());
  }
}

/* *****************************************************************************
Test: Message Size Limits
***************************************************************************** */

static void test_ipc_message_sizes(void) {
  /* Very small message (1 byte) */
  {
    fio_ipc_s *msg = fio___ipc_new(1 + 16);
    FIO_ASSERT(msg != NULL, "should allocate 1-byte message");
    msg->from = NULL;
    msg->len = 1;
    msg->data[0] = 'X';
    FIO_ASSERT(msg->data[0] == 'X', "1-byte data should be preserved");
    fio___ipc_free(msg);
  }

  /* Medium message (4KB) */
  {
    size_t size = 4096;
    fio_ipc_s *msg = fio___ipc_new(size + 16);
    FIO_ASSERT(msg != NULL, "should allocate 4KB message");
    msg->from = NULL;
    msg->len = (uint32_t)size;
    FIO_MEMSET(msg->data, 0x55, size);

    for (size_t i = 0; i < size; ++i) {
      FIO_ASSERT((uint8_t)msg->data[i] == 0x55,
                 "4KB message data should be preserved at byte %zu",
                 i);
    }
    fio___ipc_free(msg);
  }

  /* Large message (1MB) */
  {
    size_t size = 1024 * 1024;
    fio_ipc_s *msg = fio___ipc_new(size + 16);
    FIO_ASSERT(msg != NULL, "should allocate 1MB message");
    msg->from = NULL;
    msg->len = (uint32_t)size;

    for (size_t i = 0; i < size; ++i) {
      msg->data[i] = (char)(i & 0xFF);
    }

    for (size_t i = 0; i < size; ++i) {
      FIO_ASSERT((uint8_t)msg->data[i] == (i & 0xFF),
                 "1MB message data should be preserved at byte %zu",
                 i);
    }
    fio___ipc_free(msg);
  }
}

/* *****************************************************************************
Test: Timestamp and ID Generation
***************************************************************************** */

static void test_ipc_timestamp_id(void) {
  /* Timestamps are monotonically increasing */
  {
    uint64_t prev_ts = 0;
    for (int i = 0; i < 10; ++i) {
      fio_ipc_s *msg = fio___ipc_new(16);
      FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");
      msg->from = NULL;

      msg->timestamp = fio_time_milli();
      FIO_ASSERT(msg->timestamp >= prev_ts,
                 "timestamps should be monotonically increasing");
      prev_ts = msg->timestamp;

      fio___ipc_free(msg);
    }
  }

  /* IDs are unique (random) */
  {
    uint64_t ids[100];
    for (int i = 0; i < 100; ++i) {
      ids[i] = fio_rand64();
    }

    for (int i = 0; i < 100; ++i) {
      for (int j = i + 1; j < 100; ++j) {
        FIO_ASSERT(ids[i] != ids[j],
                   "random IDs should be unique (collision at %d and %d)",
                   i,
                   j);
      }
    }
  }
}

/* *****************************************************************************
Test: Callback Function Pointers
***************************************************************************** */

static void test_ipc_callbacks(void) {
  fio___test_ipc_reset_state();

  /* All three callbacks are invoked correctly with udata preserved */
  {
    void *test_udata = (void *)0x12345678;
    fio_ipc_call(.call = fio___test_ipc_call_capture,
                 .on_reply = fio___test_ipc_on_reply,
                 .on_done = fio___test_ipc_on_done,
                 .udata = test_udata,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"callback_test")));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1, "call should be invoked once");
    FIO_ASSERT(
        fio___test_ipc_received_udata == test_udata,
        "udata should be preserved in call callback (got %p, expected %p)",
        fio___test_ipc_received_udata,
        test_udata);
  }

  fio___test_ipc_reset_state();

  /* Call and done callbacks invoked (on_reply not called when done=1) */
  {
    fio_ipc_call(.call = fio___test_ipc_call_with_reply,
                 .on_reply = fio___test_ipc_on_reply,
                 .on_done = fio___test_ipc_on_done,
                 .udata = (void *)0xABCDEF00,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"callback_test")));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1, "call should be invoked once");
    FIO_ASSERT(fio___test_ipc_reply_count == 0,
               "on_reply should NOT be invoked when done=1");
    FIO_ASSERT(fio___test_ipc_done_count == 1,
               "on_done should be invoked once");
  }

  fio___test_ipc_reset_state();

  /* NULL on_reply and on_done are handled (replaced with noop) */
  {
    fio_ipc_call(.call = fio___test_ipc_call_simple,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call should be invoked even with NULL reply/done");
  }
}

/* *****************************************************************************
Test: Multi-Buffer Data Combining
***************************************************************************** */

static void test_ipc_multi_buffer_data(void) {
  fio___test_ipc_reset_state();

  /* Combine 4 buffers into a single message */
  {
    const char *header = "header:";
    uint32_t number1 = 0xDEADBEEF;
    const char *middle = ":middle:";
    uint64_t number2 = 0xCAFEBABE12345678ULL;

    size_t expected_len = FIO_STRLEN(header) + sizeof(number1) +
                          FIO_STRLEN(middle) + sizeof(number2);

    fio_ipc_call(.call = fio___test_ipc_call_capture,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = (void *)0xABCD1234,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO1((char *)header),
                     FIO_BUF_INFO2((char *)&number1, sizeof(number1)),
                     FIO_BUF_INFO1((char *)middle),
                     FIO_BUF_INFO2((char *)&number2, sizeof(number2))));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked for multi-buffer data");
    FIO_ASSERT(
        fio___test_ipc_received_len == expected_len,
        "received data length should match combined buffers (%zu vs %zu)",
        fio___test_ipc_received_len,
        expected_len);

    size_t offset = 0;

    FIO_ASSERT(FIO_MEMCMP(fio___test_ipc_received_data + offset,
                          header,
                          FIO_STRLEN(header)) == 0,
               "header should be at start of combined data");
    offset += FIO_STRLEN(header);

    uint32_t received_num1;
    FIO_MEMCPY(&received_num1,
               fio___test_ipc_received_data + offset,
               sizeof(uint32_t));
    FIO_ASSERT(received_num1 == number1,
               "number1 should be preserved (0x%X vs 0x%X)",
               received_num1,
               number1);
    offset += sizeof(number1);

    FIO_ASSERT(FIO_MEMCMP(fio___test_ipc_received_data + offset,
                          middle,
                          FIO_STRLEN(middle)) == 0,
               "middle should follow number1");
    offset += FIO_STRLEN(middle);

    uint64_t received_num2;
    FIO_MEMCPY(&received_num2,
               fio___test_ipc_received_data + offset,
               sizeof(uint64_t));
    FIO_ASSERT(received_num2 == number2,
               "number2 should be preserved (0x%llX vs 0x%llX)",
               (unsigned long long)received_num2,
               (unsigned long long)number2);

    FIO_ASSERT(fio___test_ipc_received_udata == (void *)0xABCD1234,
               "udata should be preserved");
  }

  fio___test_ipc_reset_state();

  /* 5 buffers with mixed string and binary data */
  {
    const char *prefix = "START|";
    uint8_t byte_val = 0x42;
    const char *sep1 = "|";
    uint16_t short_val = 0x1234;
    const char *suffix = "|END";

    size_t expected_len = FIO_STRLEN(prefix) + sizeof(byte_val) +
                          FIO_STRLEN(sep1) + sizeof(short_val) +
                          FIO_STRLEN(suffix);

    fio_ipc_call(.call = fio___test_ipc_call_capture,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO1((char *)prefix),
                     FIO_BUF_INFO2((char *)&byte_val, sizeof(byte_val)),
                     FIO_BUF_INFO1((char *)sep1),
                     FIO_BUF_INFO2((char *)&short_val, sizeof(short_val)),
                     FIO_BUF_INFO1((char *)suffix)));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked for 5-buffer data");
    FIO_ASSERT(fio___test_ipc_received_len == expected_len,
               "received data length should match 5 combined buffers");

    FIO_ASSERT(FIO_MEMCMP(fio___test_ipc_received_data,
                          prefix,
                          FIO_STRLEN(prefix)) == 0,
               "prefix should be at start");

    size_t suffix_offset = expected_len - FIO_STRLEN(suffix);
    FIO_ASSERT(FIO_MEMCMP(fio___test_ipc_received_data + suffix_offset,
                          suffix,
                          FIO_STRLEN(suffix)) == 0,
               "suffix should be at end");
  }

  fio___test_ipc_reset_state();

  /* Empty buffer in the middle should NOT stop combining */
  {
    const char *part1 = "part1";
    const char *part2 = "part2";

    fio_ipc_call(.call = fio___test_ipc_call_capture,
                 .on_reply = NULL,
                 .on_done = NULL,
                 .udata = NULL,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)part1),
                                      FIO_BUF_INFO0,
                                      FIO_BUF_INFO1((char *)part2)));

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_ipc_call_count == 1,
               "call callback should be invoked");
    FIO_ASSERT(fio___test_ipc_received_len ==
                   FIO_STRLEN(part1) + FIO_STRLEN(part2),
               "empty buffer should NOT terminate data");
    FIO_ASSERT(
        FIO_MEMCMP(fio___test_ipc_received_data, part1, FIO_STRLEN(part1)) == 0,
        "part1 should be received");
  }
}

/* *****************************************************************************
Test: Memory Safety
***************************************************************************** */

static void test_ipc_memory_safety(void) {
  /* Allocate and free many messages */
  {
    const int num_messages = 1000;
    fio_ipc_s *messages[1000];

    for (int i = 0; i < num_messages; ++i) {
      messages[i] = fio___ipc_new(64 + (i % 100));
      FIO_ASSERT(messages[i] != NULL, "should allocate message %d", i);
      messages[i]->from = NULL;
      messages[i]->len = (uint32_t)(i % 64);
    }

    for (int i = 0; i < num_messages; ++i) {
      fio___ipc_free(messages[i]);
    }
  }

  /* Interleaved dup/free */
  {
    fio_ipc_s *msg = fio___ipc_new(32);
    FIO_ASSERT(msg != NULL, "should allocate message");
    msg->from = NULL;

    fio_ipc_s *refs[10];
    for (int i = 0; i < 10; ++i) {
      refs[i] = fio_ipc_dup(msg);
    }

    fio___ipc_free(refs[5]);
    fio___ipc_free(refs[2]);
    fio___ipc_free(refs[8]);
    fio___ipc_free(refs[0]);
    fio___ipc_free(refs[9]);
    fio___ipc_free(refs[3]);
    fio___ipc_free(refs[7]);
    fio___ipc_free(refs[1]);
    fio___ipc_free(refs[4]);
    fio___ipc_free(refs[6]);

    fio___ipc_free(msg);
  }
}

/* *****************************************************************************
Test: Encryption Verification
***************************************************************************** */

static void test_ipc_encryption_verification(void) {
  const char *test_data = "encryption_test_data";
  size_t data_len = FIO_STRLEN(test_data);
  fio_ipc_s *msg = fio___ipc_new(data_len + 16);
  FIO_ASSERT(msg != NULL, "fio___ipc_new should allocate message");

  msg->timestamp = fio_time_milli();
  msg->id = fio_rand64();
  msg->from = NULL;
  msg->call = fio___test_ipc_call_simple;
  msg->on_reply = fio___test_ipc_on_reply;
  msg->on_done = fio___test_ipc_on_done;
  msg->udata = (void *)0x12345678;
  msg->len = (uint32_t)data_len;
  msg->flags = 0;
  msg->routing_flags = 0;
  FIO_MEMCPY(msg->data, test_data, data_len);

  char original[256];
  FIO_MEMCPY(original, msg, sizeof(*msg) + data_len);

  fio_ipc_encrypt(msg);

  FIO_ASSERT(FIO_MEMCMP(((char *)msg) + offsetof(fio_ipc_s, call),
                        original + offsetof(fio_ipc_s, call),
                        sizeof(*msg) - offsetof(fio_ipc_s, call) + data_len) !=
                 0,
             "Encrypted data should differ from original");

  char *mac = msg->data + msg->len;
  int mac_nonzero = 0;
  for (int i = 0; i < 16; ++i) {
    if (mac[i] != 0)
      mac_nonzero = 1;
  }
  FIO_ASSERT(mac_nonzero, "MAC should be non-zero");

  int decrypt_result = fio_ipc_decrypt(msg);
  FIO_ASSERT(decrypt_result == 0, "Decryption should succeed");

  FIO_ASSERT(msg->len == data_len, "Length should be restored");
  FIO_ASSERT(FIO_MEMCMP(msg->data, test_data, data_len) == 0,
             "Data should be restored after decryption");
  FIO_ASSERT(msg->call == fio___test_ipc_call_simple,
             "call pointer should be restored");
  FIO_ASSERT(msg->udata == (void *)0x12345678, "udata should be restored");

  /* Tampered message detection */
  fio_ipc_s *tampered = fio___ipc_new(data_len + 16);
  FIO_ASSERT(tampered != NULL, "fio___ipc_new should allocate message");
  tampered->from = NULL;
  FIO_MEMCPY(tampered, msg, sizeof(*msg) + data_len + 16);
  tampered->from = NULL;
  tampered->timestamp = msg->timestamp;
  tampered->id = msg->id;
  tampered->len = msg->len;
  tampered->routing_flags = 0;
  FIO_MEMCPY(tampered->data, test_data, data_len);

  fio_ipc_encrypt(tampered);

  tampered->data[tampered->len] ^= 0xFF;

  fprintf(stderr, "      (expect SECURITY message about decryption failure)\n");
  int tampered_result = fio_ipc_decrypt(tampered);
  FIO_ASSERT(tampered_result != 0,
             "Decryption should fail for tampered message");
  fprintf(stderr, "      (expect SECURITY message about decryption failure)\n");
  FIO_ASSERT(fio_ipc_decrypt(tampered) != 0 &&
                 (tampered->routing_flags & 1),
             "Failed decryption is terminal: a retry fails again");

  fio___ipc_free(msg);
  fio___ipc_free(tampered);
}

/* *****************************************************************************
Test: every encryption uses a fresh nonce

Messages that share `timestamp` and `id` (forced IDs) must still use distinct
key streams; `timestamp`, `id` and `nonce` are all authenticated (they select
the key); replies default to a fresh id.
***************************************************************************** */

static void test_ipc_encryption_nonce_unique(void) {
  enum { MSG_LEN = 24 };
  const char *plain[2] = {"AAAAAAAAAAAAAAAAAAAAAAAA",
                          "BBBBBBBBBBBBBBBBBBBBBBBB"};
  fio_ipc_s *m[2];
  uint8_t pt[2][128], ct[2][128];
  const size_t enc_len =
      sizeof(fio_ipc_s) - offsetof(fio_ipc_s, call) + MSG_LEN;
  FIO_ASSERT(enc_len <= sizeof(pt[0]), "test buffer too small");
  for (size_t i = 0; i < 2; ++i) {
    m[i] = fio___ipc_new(MSG_LEN + 16);
    FIO_ASSERT(m[i], "fio___ipc_new should allocate message");
    m[i]->from = NULL;
    m[i]->len = MSG_LEN;
    m[i]->flags = 0;
    m[i]->routing_flags = 0;
    m[i]->timestamp = 1700000000123ULL; /* same tick */
    m[i]->id = 0x1122334455667788ULL;   /* same (forced) id */
    m[i]->call = fio___test_ipc_call_simple;
    m[i]->on_reply = fio___test_ipc_on_reply;
    m[i]->on_done = fio___test_ipc_on_done;
    m[i]->udata = NULL;
    FIO_MEMCPY(m[i]->data, plain[i], MSG_LEN);
    FIO_MEMCPY(pt[i], &m[i]->call, enc_len);
    fio_ipc_encrypt(m[i]);
    FIO_MEMCPY(ct[i], &m[i]->call, enc_len);
  }
  FIO_ASSERT(m[0]->nonce != m[1]->nonce, "each encryption gets a new nonce");
  size_t same_stream = 0;
  for (size_t i = 0; i < enc_len; ++i)
    same_stream += ((ct[0][i] ^ ct[1][i]) == (pt[0][i] ^ pt[1][i]));
  FIO_ASSERT(same_stream < enc_len / 2,
             "repeated timestamp / id must not share a key stream (%zu/%zu)",
             same_stream,
             enc_len);
  for (size_t i = 0; i < 2; ++i) {
    FIO_ASSERT(!fio_ipc_decrypt(m[i]), "decryption should succeed");
    FIO_ASSERT(!FIO_MEMCMP(m[i]->data, plain[i], MSG_LEN),
               "payload should round-trip");
  }
  /* the nonce fields are authenticated (they select the key) */
  for (size_t i = 0; i < 3; ++i) {
    uint64_t *field = (i == 0)   ? &m[0]->id
                      : (i == 1) ? &m[0]->timestamp
                                 : &m[0]->nonce;
    fio_ipc_encrypt(m[0]);
    *field ^= 1;
    fprintf(stderr,
            "      (expect SECURITY message about decryption failure)\n");
    FIO_ASSERT(fio_ipc_decrypt(m[0]),
               "tampered nonce field (%zu) must fail authentication",
               i);
  }
  fio___ipc_free(m[0]);
  fio___ipc_free(m[1]);
}

static uint64_t fio___test_reply_ids[3];
static size_t fio___test_reply_count;
static void fio___test_reply_id_collect(fio_ipc_s *r) {
  if (fio___test_reply_count < 3)
    fio___test_reply_ids[fio___test_reply_count++] = r->id;
}

static void test_ipc_reply_fresh_id(void) {
  fio_ipc_s *req = fio_ipc_new(.call = fio___test_ipc_call_simple,
                               .on_reply = fio___test_reply_id_collect,
                               .on_done = fio___test_reply_id_collect,
                               .id = 0x0123456789ABCDEFULL);
  FIO_ASSERT(req, "allocate request");
  req->from = NULL; /* local execution path */
  fio___test_reply_count = 0;
  fio_ipc_reply(req, .done = 0);
  fio_ipc_reply(req, .done = 0);
  fio_ipc_reply(req, .id = 77, .done = 1);
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio___test_reply_count == 3, "replies delivered locally");
  FIO_ASSERT(fio___test_reply_ids[0] != req->id &&
                 fio___test_reply_ids[1] != req->id &&
                 fio___test_reply_ids[0] != fio___test_reply_ids[1],
             "default reply ids are fresh (unique nonce per reply)");
  FIO_ASSERT(fio___test_reply_ids[2] == 77, "explicit reply id is kept");
  fio_ipc_free(req);
}

/* *****************************************************************************
Test: RPC Message Creation (fio_ipc_new)
***************************************************************************** */

static void test_ipc_rpc_message_creation(void) {
  /* Create RPC message with opcode and data */
  {
    const char *test_data = "rpc_test_data";
    uint32_t test_opcode = 0x12345678;

    fio_ipc_s *msg =
        fio_ipc_new(.cluster = 1,
                    .opcode = test_opcode,
                    .flags = 0xABCD,
                    .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)test_data)));

    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate message");

    FIO_ASSERT(msg->opcode == test_opcode,
               "opcode should be stored in opcode field (got 0x%X, expected "
               "0x%X)",
               msg->opcode,
               test_opcode);

    FIO_ASSERT(msg->len == FIO_STRLEN(test_data),
               "data length should match (got %u, expected %zu)",
               msg->len,
               FIO_STRLEN(test_data));
    FIO_ASSERT(FIO_MEMCMP(msg->data, test_data, msg->len) == 0,
               "data should match input");

    FIO_ASSERT((msg->routing_flags & FIO_IPC_FLAG_OPCODE) != 0,
               "OPCODE flag should be set in routing_flags");

    uint64_t timestamp = fio_ltole64(msg->timestamp);
    FIO_ASSERT(timestamp > 0, "timestamp should be set");

    FIO_ASSERT(msg->id != 0, "id should be set (random)");

    fio___ipc_free(msg);
  }

  /* Create RPC message with empty data */
  {
    fio_ipc_s *msg = fio_ipc_new(.cluster = 1,
                                 .opcode = 0x00000001,
                                 .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate empty message");
    FIO_ASSERT(msg->len == 0, "empty message should have len=0");
    FIO_ASSERT(msg->opcode == 0x00000001, "opcode should be stored correctly");

    fio___ipc_free(msg);
  }

  /* Create RPC message with multi-buffer data */
  {
    const char *part1 = "PART1:";
    uint32_t num = 0xDEADBEEF;
    const char *part2 = ":PART2";

    size_t expected_len = FIO_STRLEN(part1) + sizeof(num) + FIO_STRLEN(part2);

    fio_ipc_s *msg = fio_ipc_new(.cluster = 1,
                                 .opcode = 0x42,
                                 .data = FIO_IPC_DATA(
                                     FIO_BUF_INFO1((char *)part1),
                                     FIO_BUF_INFO2((char *)&num, sizeof(num)),
                                     FIO_BUF_INFO1((char *)part2)));

    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate multi-buffer");
    FIO_ASSERT(msg->len == expected_len,
               "multi-buffer length should match (got %u, expected %zu)",
               msg->len,
               expected_len);

    FIO_ASSERT(FIO_MEMCMP(msg->data, part1, FIO_STRLEN(part1)) == 0,
               "part1 should be at start");

    uint32_t received_num;
    FIO_MEMCPY(&received_num, msg->data + FIO_STRLEN(part1), sizeof(uint32_t));
    FIO_ASSERT(received_num == num,
               "number should be preserved (got 0x%X)",
               received_num);

    fio___ipc_free(msg);
  }

  /* Create RPC message with forced timestamp and id */
  {
    uint64_t forced_ts = 0x123456789ABCDEF0ULL;
    uint64_t forced_id = 0xFEDCBA9876543210ULL;

    fio_ipc_s *msg = fio_ipc_new(.cluster = 1,
                                 .opcode = 0x99,
                                 .timestamp = forced_ts,
                                 .id = forced_id,
                                 .data = FIO_IPC_DATA(FIO_BUF_INFO0));

    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate message");

    FIO_ASSERT(fio_ltole64(msg->timestamp) == forced_ts,
               "forced timestamp should be preserved");
    FIO_ASSERT(msg->id == forced_id, "forced id should be preserved");

    fio___ipc_free(msg);
  }
}

/* *****************************************************************************
Test: RPC Op-Code Registration and Dispatch
***************************************************************************** */

static volatile int fio___test_rpc_opcode_called = 0;
static volatile uint32_t fio___test_rpc_opcode_received = 0;
static void *fio___test_rpc_opcode_udata = NULL;

static void fio___test_rpc_opcode_handler(fio_ipc_s *msg) {
  fio___test_rpc_opcode_called++;
  fio___test_rpc_opcode_udata = msg->udata;
  (void)msg;
}

static void test_ipc_rpc_opcode_registration(void) {
  fio___test_rpc_opcode_called = 0;
  fio___test_rpc_opcode_received = 0;
  fio___test_rpc_opcode_udata = NULL;

  /* Register a valid op-code and verify dispatch works */
  {
    uint32_t test_opcode = 0x00001234;
    void *test_udata = (void *)0xABCDEF00;

    fio_ipc_opcode_register(.opcode = test_opcode,
                            .call = fio___test_rpc_opcode_handler,
                            .udata = test_udata);

    fio_ipc_s *msg = fio_ipc_new(.cluster = 1,
                                 .opcode = test_opcode,
                                 .data = FIO_IPC_DATA(
                                     FIO_BUF_INFO1((char *)"test_dispatch")));
    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate message");

    FIO_ASSERT(msg->opcode == test_opcode,
               "opcode should be stored in opcode field");

    fio___ipc_opcode_task(msg, NULL);

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_rpc_opcode_called == 1,
               "op-code handler should be called once (called %d times)",
               fio___test_rpc_opcode_called);
    FIO_ASSERT(fio___test_rpc_opcode_udata == test_udata,
               "op-code handler should receive correct udata");
  }

  fio___test_rpc_opcode_called = 0;
  fio___test_rpc_opcode_udata = NULL;

  /* Register another op-code with NULL udata */
  {
    uint32_t test_opcode2 = 0x00005678;
    fio_ipc_opcode_register(.opcode = test_opcode2,
                            .call = fio___test_rpc_opcode_handler,
                            .udata = NULL);

    fio_ipc_s *msg =
        fio_ipc_new(.cluster = 1,
                    .opcode = test_opcode2,
                    .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"test2")));
    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate message");

    fio___ipc_opcode_task(msg, NULL);

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_rpc_opcode_called == 1,
               "second op-code handler should be called");
    FIO_ASSERT(fio___test_rpc_opcode_udata == NULL,
               "second op-code should have NULL udata");
  }

  fio___test_rpc_opcode_called = 0;

  /* Remove an op-code by passing NULL function */
  {
    uint32_t test_opcode = 0x00001234;
    fio_ipc_opcode_register(.opcode = test_opcode, .call = NULL);

    fio_ipc_s *msg =
        fio_ipc_new(.cluster = 1,
                    .opcode = test_opcode,
                    .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"removed")));
    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate message");

    fprintf(stderr,
            "      (expect WARNING about illegal op-code %u)\n",
            test_opcode);
    fio___ipc_opcode_task(msg, NULL);

    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(fio___test_rpc_opcode_called == 0,
               "removed op-code should not call handler");
  }

  fio_ipc_opcode_register(.opcode = 0x00005678, .call = NULL);
}

/* *****************************************************************************
Test: RPC Listen/Connect Validation
***************************************************************************** */

static void test_ipc_rpc_listen_connect_validation(void) {
  {
    fprintf(stderr, "      (expect WARNING about NULL or empty URL)\n");
    fio_ipc_cluster_connect(NULL);
  }

  {
    fprintf(stderr, "      (expect WARNING about NULL or empty URL)\n");
    fio_ipc_cluster_connect("");
  }

  {
    if (fio_secret_is_random()) {
      fprintf(stderr, "      (expect WARNING about RPC disabled)\n");
      fio_io_listener_s *listener = fio_ipc_cluster_listen(12345);
      FIO_ASSERT(
          listener == NULL,
          "fio_ipc_cluster_listen should return NULL with random secret");
    }
  }
}

/* *****************************************************************************
Test: RPC Message Encryption/Decryption
***************************************************************************** */

static void test_ipc_rpc_encryption(void) {
  /* RPC message can be encrypted and decrypted */
  {
    const char *test_data = "rpc_encryption_test";
    uint32_t test_opcode = 0xABCD1234;

    fio_ipc_s *msg =
        fio_ipc_new(.cluster = 1,
                    .opcode = test_opcode,
                    .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)test_data)));

    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate message");

    uint32_t original_len = msg->len;
    char original_data[64];
    FIO_MEMCPY(original_data, msg->data, msg->len);

    fio_ipc_encrypt(msg);

    FIO_ASSERT((msg->routing_flags & FIO_IPC_FLAG_OPCODE) != 0,
               "OPCODE flag should be preserved after encryption");

    FIO_ASSERT(FIO_MEMCMP(msg->data, original_data, original_len) != 0,
               "Data should be encrypted (different from original)");

    int result = fio_ipc_decrypt(msg);
    FIO_ASSERT(result == 0, "Decryption should succeed");

    FIO_ASSERT(msg->len == original_len, "Length should be restored");
    FIO_ASSERT(FIO_MEMCMP(msg->data, original_data, original_len) == 0,
               "Data should be restored after decryption");

    FIO_ASSERT(msg->opcode == test_opcode,
               "opcode should be restored after decryption");

    fio___ipc_free(msg);
  }

  /* Tampered RPC message fails decryption */
  {
    fio_ipc_s *msg =
        fio_ipc_new(.cluster = 1,
                    .opcode = 0x42,
                    .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"test")));

    FIO_ASSERT(msg != NULL, "fio_ipc_new should allocate message");

    fio_ipc_encrypt(msg);

    msg->data[msg->len] ^= 0xFF;

    fprintf(stderr,
            "      (expect SECURITY message about decryption failure)\n");
    int result = fio_ipc_decrypt(msg);
    FIO_ASSERT(result != 0, "Decryption should fail for tampered RPC message");

    fio___ipc_free(msg);
  }
}

/* *****************************************************************************
Test: RPC Filter (Duplicate Message Detection)
***************************************************************************** */

static void test_ipc_rpc_filter(void) {
  /* Filter rejects duplicate messages */
  {
    fio___ipc_cluster_filter_s filter = {0};
    uint64_t timestamp = fio_io_last_tick_time();
    uint64_t id = fio_rand64();

    int result1 = fio___ipc_cluster_filter(&filter, timestamp, id);
    FIO_ASSERT(result1 == 0, "First message should pass filter");

    int result2 = fio___ipc_cluster_filter(&filter, timestamp, id);
    FIO_ASSERT(result2 == -1, "Duplicate message should be rejected");

    int result3 = fio___ipc_cluster_filter(&filter, timestamp, id + 1);
    FIO_ASSERT(result3 == 0, "Different message should pass filter");

    int result4 = fio___ipc_cluster_filter(&filter, timestamp + 1, id);
    FIO_ASSERT(result4 == 0, "Different timestamp should pass filter");

    fio___ipc_cluster_filter_destroy(&filter);
  }

  /* Filter handles many messages */
  {
    fio___ipc_cluster_filter_s filter = {0};
    uint64_t base_timestamp = fio_io_last_tick_time();

    for (int i = 0; i < 1000; ++i) {
      int result =
          fio___ipc_cluster_filter(&filter, base_timestamp, (uint64_t)i + 1);
      FIO_ASSERT(result == 0, "Unique message %d should pass filter", i);
    }

    int dup_result = fio___ipc_cluster_filter(&filter, base_timestamp, 500);
    FIO_ASSERT(dup_result == -1, "Duplicate message 500 should be rejected");

    fio___ipc_cluster_filter_destroy(&filter);
  }

  /* Filter truncation (65536 limit) keeps the newest 16384 IDs */
  {
    fio___ipc_cluster_filter_s filter = {0};
    const uint64_t base = 1000;
    for (uint64_t i = 1; i <= 65537; ++i)
      FIO_ASSERT(!fio___ipc_cluster_filter(&filter, base + i, i),
                 "Unique message %zu should pass filter",
                 (size_t)i);
    FIO_ASSERT(filter.count == 16385, "Filter should truncate to keep + 1");
    for (uint64_t i = 65537 - 16384; i <= 65537; ++i)
      FIO_ASSERT(fio___ipc_cluster_filter(&filter, base + i, i) == -1,
                 "Recent message %zu should remain filtered",
                 (size_t)i);
    fio___ipc_cluster_filter_destroy(&filter);
  }

  /* Filter time window validation */
  {
    uint64_t current_tick = fio_io_last_tick_time();

    int result1 = fio___ipc_cluster_filter_window(current_tick);
    FIO_ASSERT(result1 == 0, "Current timestamp should pass window check");

    int result2 = fio___ipc_cluster_filter_window(current_tick - 29000);
    FIO_ASSERT(result2 == 0, "29s old timestamp should pass window check");

    int result3 = fio___ipc_cluster_filter_window(current_tick - 31000);
    FIO_ASSERT(result3 == -1, "31s old timestamp should fail window check");

    int result4 = fio___ipc_cluster_filter_window(current_tick + 31000);
    FIO_ASSERT(result4 == -1, "31s future timestamp should fail window check");

    /* the window is wall-clock: a monotonic (since boot) stamp is foreign */
    FIO_ASSERT(fio___ipc_cluster_filter_window(fio___ipc_now()) == -1,
               "monotonic timestamp should fail the wall-clock window");
  }
}

/* *****************************************************************************
Test: UDP Discovery Message Composition and Validation
***************************************************************************** */

static void test_ipc_udp_discovery_message(void) {
  /* Compose and validate a discovery message */
  {
    fio___ipc_udp_discovery_s msg;
    fio___ipc_udp_discovery_compose(&msg);

    FIO_ASSERT(FIO_MEMCMP(msg.uuid, FIO___IPC.uuid.u8, 16) == 0,
               "Discovery message should contain our UUID");

    uint64_t timestamp = fio_ltole64(msg.timestamp);
    uint64_t now = (uint64_t)fio_io_last_tick_time();
    FIO_ASSERT(timestamp <= now + 1000 && timestamp >= now - 1000,
               "Discovery message timestamp should be recent (wall-clock)");

    int result = fio___ipc_udp_discovery_validate(&msg);
    FIO_ASSERT(result == -1,
               "Self-generated discovery message should fail validation");

    /* the same message, seen by another instance, must validate */
    fio_u128 saved = FIO___IPC.uuid;
    FIO___IPC.uuid.u8[0] ^= 0xFF;
    result = fio___ipc_udp_discovery_validate(&msg);
    FIO___IPC.uuid = saved;
    FIO_ASSERT(result == 0,
               "Discovery message from another instance should validate");
  }

  /* Tampered message should fail validation */
  {
    fio___ipc_udp_discovery_s msg;
    fio___ipc_udp_discovery_compose(&msg);

    msg.uuid[0] ^= 0xFF;

    int result = fio___ipc_udp_discovery_validate(&msg);
    FIO_ASSERT(result == -1,
               "Tampered discovery message should fail validation");
  }

  /* Tampered MAC (first / last byte) must be rejected (constant-time check) */
  for (size_t i = 0; i < 16; i += 15) {
    fio___ipc_udp_discovery_s msg;
    fio___ipc_udp_discovery_compose(&msg);
    msg.mac[i] ^= 0x01;
    fio_u128 saved = FIO___IPC.uuid; /* pretend message came from a peer */
    FIO___IPC.uuid.u8[0] ^= 0xFF;
    int result = fio___ipc_udp_discovery_validate(&msg);
    msg.mac[i] ^= 0x01; /* restore: the untampered tag must validate */
    int restored = fio___ipc_udp_discovery_validate(&msg);
    FIO___IPC.uuid = saved;
    FIO_ASSERT(result == -1,
               "Discovery message with tampered MAC byte %zu should fail",
               i);
    FIO_ASSERT(restored == 0,
               "Discovery message with restored MAC should validate");
  }

  /* Message with old timestamp should fail */
  {
    fio___ipc_udp_discovery_s msg;
    fio___ipc_udp_discovery_compose(&msg);

    msg.timestamp = fio_ltole64((uint64_t)fio_io_last_tick_time() - 60000);

    int result = fio___ipc_udp_discovery_validate(&msg);
    FIO_ASSERT(result == -1,
               "Old discovery message should fail time window validation");
  }

  /* Rejection logs are rate limited (unauthenticated floods) */
  {
    FIO___IPC.udp_log_time = 0;
    FIO___IPC.udp_log_skipped = 0;
    fprintf(stderr, "      (expect one SECURITY message about UDP discovery)\n");
    for (size_t i = 0; i < 16; ++i)
      fio___ipc_udp_log_rejected();
    FIO_ASSERT(FIO___IPC.udp_log_skipped == 15,
               "UDP rejection log should report once per second");
    FIO___IPC.udp_log_time = 0;
    FIO___IPC.udp_log_skipped = 0;
  }
}

/* *****************************************************************************
Main entry point
***************************************************************************** */

/* Peer lifecycle: exercise authenticated RPC handshake and connection close. */
static size_t fio___test_lifecycle_attached[2];
static size_t fio___test_lifecycle_detached[2];
static fio_u128 fio___test_lifecycle_uuid;
static fio_ipc_s *fio___test_lifecycle_saved;
static fio_io_s *fio___test_lifecycle_io;
static size_t fio___test_lifecycle_opcode_calls;

static void fio___test_lifecycle_opcode(fio_ipc_s *ipc) {
  ++fio___test_lifecycle_opcode_calls;
  FIO_ASSERT(ipc->opcode == 0xCC11U && ipc->len == 3 &&
                 FIO_MEMCMP(ipc->data, "ack", 3) == 0,
             "ordinary opcode request reaches application handler");
}
static void fio___test_lifecycle_on_attached(fio_ipc_s *event) {
  size_t *count = (size_t *)event->udata;
  ++*count;
  FIO_ASSERT(event->len == 16 && event->from == fio___test_lifecycle_io,
             "attached event must contain UUID and originating IO");
  FIO_ASSERT(!FIO_IPC_FLAG_TEST(event, FIO_IPC_FLAG_OPCODE) &&
                 !FIO_IPC_FLAG_TEST(event, FIO_IPC_FLAG_REPLY) &&
                 FIO_MEMCMP(event->data, fio___test_lifecycle_uuid.u8, 16) == 0,
             "synthetic event must not advertise an opcode or reply");
  if (count == &fio___test_lifecycle_attached[0]) {
    fio___test_lifecycle_saved = fio_ipc_dup(event);
    fio_ipc_s *msg =
        fio_ipc_new(.cluster = 1,
                    .opcode = 0xCC11U,
                    .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)"ack", 3)));
    FIO_ASSERT(msg && FIO_IPC_FLAG_TEST(msg, FIO_IPC_FLAG_OPCODE) &&
                   !FIO_IPC_FLAG_TEST(msg, FIO_IPC_FLAG_REPLY),
               "attached callback authors normal opcode request");
    fio_ipc_send_to(event->from, msg);
  }
}
static void fio___test_lifecycle_on_detached(fio_ipc_s *event) {
  size_t *counter = (size_t *)event->udata;
  size_t index = (counter == &fio___test_lifecycle_attached[0]) ? 0 : 1;
  ++fio___test_lifecycle_detached[index];
  FIO_ASSERT(event->len == 16 && event->from == NULL &&
                 FIO_MEMCMP(event->data, fio___test_lifecycle_uuid.u8, 16) == 0,
             "detached event contains UUID but no connection");
}

/* authors an encrypted UUID declaration, as a peer would */
static fio_ipc_s *fio___test_hello_frame(const fio_u128 *uuid,
                                         uint8_t role,
                                         uint64_t timestamp) {
  const uint32_t len = (role == FIO___IPC_ROLE_CLUSTER) ? 25 : 17;
  fio_ipc_s *m = fio___ipc_new(len + 16);
  FIO_ASSERT(m != NULL, "allocate declaration");
  m->from = NULL;
  m->len = len;
  m->flags = 0;
  m->routing_flags = FIO_IPC_FLAG_HELLO;
  m->timestamp = timestamp;
  m->id = fio_rand64();
  m->call = NULL;
  m->on_reply = NULL;
  m->on_done = NULL;
  m->udata = NULL;
  FIO_MEMCPY(m->data, uuid->u8, 16);
  m->data[16] = (char)role;
  if (len == 25) { /* cluster: the peer's challenge */
    uint64_t challenge = fio_rand64();
    FIO_MEMCPY(m->data + 17, &challenge, 8);
  }
  fio_ipc_encrypt(m);
  return m;
}

static fio_ipc_s *fio___test_lifecycle_hello(const fio_u128 *uuid) {
  return fio___test_hello_frame(uuid,
                                FIO___IPC_ROLE_CLUSTER,
                                (uint64_t)fio_io_last_tick_time());
}

/* authors an encrypted cluster challenge proof, as a peer would */
static fio_ipc_s *fio___test_proof_frame(const fio_u128 *uuid,
                                         uint64_t challenge) {
  fio_ipc_s *m = fio___ipc_new(24 + 16);
  FIO_ASSERT(m != NULL, "allocate challenge proof");
  FIO_MEMSET(m, 0, sizeof(*m));
  m->len = 24;
  m->routing_flags = FIO_IPC_FLAG_HELLO | FIO_IPC_FLAG_REPLY;
  m->timestamp = (uint64_t)fio_io_last_tick_time();
  m->id = fio_rand64();
  FIO_MEMCPY(m->data, uuid->u8, 16);
  FIO_MEMCPY(m->data + 16, &challenge, 8);
  fio_ipc_encrypt(m);
  return m;
}

/* declares `uuid` on `io` and answers its challenge (full handshake) */
static void fio___test_lifecycle_declare(fio_io_s *io, const fio_u128 *uuid) {
  fio___ipc_on_rpc_master(fio___test_lifecycle_hello(uuid), fio_io_dup(io));
  FIO_ASSERT(fio___ipc_cluster_parser(io)->auth == FIO___IPC_AUTH_PROOF,
             "declaration awaits the challenge proof");
  fio___ipc_on_rpc_master(
      fio___test_proof_frame(uuid, fio___ipc_cluster_parser(io)->challenge),
      fio_io_dup(io));
}

/* attaches a fresh cluster connection (sends our declaration on attach) */
static fio_io_s *fio___test_lifecycle_attach(fio_socket_i fds[2]) {
  FIO_ASSERT(!fio_sock_socketpair(fds), "create peer sockets");
  fio___test_lifecycle_io =
      fio_io_attach_fd(fds[0], &FIO___IPC.protocol_rpc_hello, NULL, NULL);
  FIO_ASSERT(fio___test_lifecycle_io != NULL, "attach RPC peer");
  fio_queue_perform_all(fio_io_queue());
  return fio___test_lifecycle_io;
}

static void test_ipc_cluster_lifecycle(void) {
  FIO_ASSERT(fio_ipc_cluster_on_peer(.udata = NULL) == -1,
             "empty subscription rejected");
  FIO_ASSERT(!FIO___IPC.on_peer_count,
             "invalid subscription leaves registry unchanged");
  for (size_t i = 0; i < 2; ++i)
    FIO_ASSERT(!fio_ipc_cluster_on_peer(
                       .on_attached = fio___test_lifecycle_on_attached,
                       .on_detached = fio___test_lifecycle_on_detached,
                       .udata = &fio___test_lifecycle_attached[i]),
               "register independent subscriber");
  for (size_t i = 2; i < FIO_IPC_CLUSTER_PEER_MAX_SUBSCRIBERS; ++i)
    FIO_ASSERT(
        !fio_ipc_cluster_on_peer(.on_detached =
                                     fio___test_lifecycle_on_detached,
                                 .udata = &fio___test_lifecycle_detached[1]),
        "append subscriber until capacity");
  FIO_ASSERT(fio_ipc_cluster_on_peer(.on_attached =
                                         fio___test_lifecycle_on_attached) ==
                 -1,
             "full subscriber list rejected without replacement");
  FIO___IPC.on_peer_count = 2; /* discard test-only subscribers */
  FIO_ASSERT(!fio_ipc_opcode_register(.opcode = 0xCC11U,
                                      .call = fio___test_lifecycle_opcode),
             "register ordinary application opcode");

  fio___test_lifecycle_uuid =
      fio_u128_init64(0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL);
  fio_socket_i fds[2];
  fio___test_lifecycle_attach(fds);
  fio_ipc_s *declaration =
      fio___test_lifecycle_hello(&fio___test_lifecycle_uuid);
  fio_ipc_s *replay = fio___ipc_copy(declaration); /* same ciphertext */
  fio___ipc_on_rpc_master(declaration, fio_io_dup(fio___test_lifecycle_io));
  FIO_ASSERT(!fio___test_lifecycle_attached[0] &&
                 fio_io_protocol(fio___test_lifecycle_io) ==
                     &FIO___IPC.protocol_rpc_hello &&
                 fio___ipc_cluster_parser(fio___test_lifecycle_io)->auth ==
                     FIO___IPC_AUTH_PROOF,
             "a (transferable) declaration alone doesn't authenticate");
  fio___ipc_on_rpc_master(
      fio___test_proof_frame(
          &fio___test_lifecycle_uuid,
          fio___ipc_cluster_parser(fio___test_lifecycle_io)->challenge),
      fio_io_dup(fio___test_lifecycle_io));
  FIO_ASSERT(fio___test_lifecycle_attached[0] == 1 &&
                 fio___test_lifecycle_attached[1] == 1 &&
                 !fio___test_lifecycle_detached[0],
             "both subscribers receive a valid declaration");
  FIO_ASSERT(fio___test_lifecycle_saved &&
                 fio___test_lifecycle_saved->from == fio___test_lifecycle_io &&
                 FIO_MEMCMP(fio___test_lifecycle_saved->data,
                            fio___test_lifecycle_uuid.u8,
                            16) == 0,
             "dup retains UUID and IO, not an outgoing message");
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio_io_protocol(fio___test_lifecycle_io) ==
                 &FIO___IPC.protocol_rpc,
             "declared peer moves to the broadcast protocol");
  fio_ipc_s *request =
      fio_ipc_new(.cluster = 1,
                  .opcode = 0xCC11U,
                  .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)"ack", 3)));
  FIO_ASSERT(request != NULL, "allocate ordinary request");
  fio_ipc_encrypt(request);
  fio___ipc_on_rpc_master(request, fio_io_dup(fio___test_lifecycle_io));
  FIO_ASSERT(fio___test_lifecycle_opcode_calls == 1,
             "normal opcode request dispatches to application handler");
  fio_queue_perform_all(fio_io_queue());

  /* A replayed declaration (new connection, within the window) is dropped. */
  {
    fio_io_s *first = fio___test_lifecycle_io;
    fio_socket_i rfds[2];
    fio_io_s *rio = fio___test_lifecycle_attach(rfds);
    fio___ipc_on_rpc_master(replay, fio_io_dup(rio));
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(fio___test_lifecycle_attached[0] == 1 &&
                   fio_io_protocol(rio) == &FIO___IPC.protocol_rpc_hello,
               "replayed declaration is filtered");
    fio_io_close_now(rio);
    fio_queue_perform_all(fio_io_queue());
    fio_sock_close(rfds[1]);
    fio___test_lifecycle_io = first;
  }

  /* A fresh declaration replayed by a third party (unseen here) can't answer
   * this connection's challenge: a proof for another challenge is refused. */
  {
    fio_io_s *first = fio___test_lifecycle_io;
    const fio_u128 other = fio_u128_init64(0x0102030405060708ULL, 99);
    fio_socket_i rfds[2];
    fio_io_s *rio = fio___test_lifecycle_attach(rfds);
    fio_io_s *hold = fio_io_dup(rio);
    fio___ipc_on_rpc_master(fio___test_lifecycle_hello(&other),
                            fio_io_dup(rio));
    fio___ipc_on_rpc_master(
        fio___test_proof_frame(&other,
                               fio___ipc_cluster_parser(rio)->challenge + 1),
        fio_io_dup(rio));
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(fio___test_lifecycle_attached[0] == 1 &&
                   fio_io_protocol(hold) == &FIO___IPC.protocol_rpc_hello &&
                   !fio_io_is_open(hold) &&
                   !fio___ipc_cluster_filter_get(
                       &FIO___IPC.peers,
                       (fio___ipc_msg_id_s){other.u64[0], other.u64[1]}),
               "wrong challenge proof is rejected (UUID not registered)");
    fio_io_free(hold);
    fio_queue_perform_all(fio_io_queue());
    fio_sock_close(rfds[1]);
    fio___test_lifecycle_io = first;
  }

  /* A second declaration closes the IO; retained event delays detach. */
  fio___ipc_on_rpc_master(
      fio___test_lifecycle_hello(&fio___test_lifecycle_uuid),
      fio_io_dup(fio___test_lifecycle_io));
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio___test_lifecycle_attached[0] == 1 &&
                 fio___test_lifecycle_attached[1] == 1 &&
                 !fio___test_lifecycle_detached[0] &&
                 !fio___test_lifecycle_detached[1],
             "retained event pins IO and delays both detach notifications");
  fio_ipc_free(fio___test_lifecycle_saved);
  fio___test_lifecycle_saved = NULL;
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio___test_lifecycle_detached[0] == 1 &&
                 fio___test_lifecycle_detached[1] == 1,
             "both detached callbacks arrive after retained event freed");
  fio_sock_close(fds[1]);

  /* Rejected declarations (self, wrong role, ordinary message first, stale)
   * neither announce nor detach. Rejections close the connection; stale
   * declarations are dropped (the connection times out). */
  for (size_t i = 0; i < 4; ++i) {
    fio_io_s *io = fio___test_lifecycle_attach(fds);
    fio_io_s *hold = fio_io_dup(io);
    fio_ipc_s *m;
    if (i == 0) {
      m = fio___test_lifecycle_hello(&FIO___IPC.uuid);
    } else if (i == 1) {
      m = fio___test_hello_frame(&fio___test_lifecycle_uuid,
                                 FIO___IPC_ROLE_WORKER,
                                 (uint64_t)fio_io_last_tick_time());
    } else if (i == 2) {
      m = fio_ipc_new(.cluster = 1, .opcode = 0xCC11U);
      FIO_ASSERT(m != NULL, "allocate premature RPC");
      fio_ipc_encrypt(m);
    } else {
      m = fio___test_hello_frame(&fio___test_lifecycle_uuid,
                                 FIO___IPC_ROLE_CLUSTER,
                                 (uint64_t)fio_io_last_tick_time() - 60000);
    }
    fio___ipc_on_rpc_master(m, fio_io_dup(io));
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(fio___test_lifecycle_attached[0] == 1 &&
                   fio___test_lifecycle_detached[1] == 1 &&
                   fio___test_lifecycle_opcode_calls == 1 &&
                   fio_io_protocol(hold) == &FIO___IPC.protocol_rpc_hello &&
                   (i == 3 || !fio_io_is_open(hold)),
               "invalid declaration (%zu) is rejected",
               i);
    if (i == 3)
      fio_io_close_now(hold);
    fio_io_free(hold);
    fio_queue_perform_all(fio_io_queue());
    fio_sock_close(fds[1]);
  }

  /* Reconnect with same UUID gets a fresh pair of notifications. */
  fio___test_lifecycle_attach(fds);
  fio___test_lifecycle_declare(fio___test_lifecycle_io,
                               &fio___test_lifecycle_uuid);
  FIO_ASSERT(fio___test_lifecycle_attached[0] == 2 &&
                 fio___test_lifecycle_attached[1] == 2,
             "reconnected peer announces to both subscribers");
  fio_ipc_free(fio___test_lifecycle_saved);
  fio___test_lifecycle_saved = NULL;
  fio_queue_perform_all(fio_io_queue());
  fio_io_close_now(fio___test_lifecycle_io);
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio___test_lifecycle_detached[0] == 2 &&
                 fio___test_lifecycle_detached[1] == 2,
             "reconnected peer detaches from both subscribers");
  fio_sock_close(fds[1]);
  FIO_MEMSET(FIO___IPC.on_peer, 0, sizeof(FIO___IPC.on_peer));
  FIO___IPC.on_peer_count = 0; /* test isolation */
  fio_ipc_opcode_register(.opcode = 0xCC11U, .call = NULL);
}

/* *****************************************************************************
Test: local connection authentication (UUID declaration)
***************************************************************************** */

static size_t fio___test_auth_count;
static void fio___test_auth_call(fio_ipc_s *m) {
  ++fio___test_auth_count;
  (void)m;
}

/* delivers an encrypted local message as if read from `io` */
static void fio___test_auth_deliver(fio_io_s *io, fio_ipc_s *m) {
  m->from = fio_io_dup(io);
  fio___ipc_on_ipc_master(m, m->from);
}
static fio_ipc_s *fio___test_auth_msg(void) {
  fio_ipc_s *m = fio_ipc_new(.call = fio___test_auth_call);
  FIO_ASSERT(m != NULL, "allocate local message");
  fio_ipc_encrypt(m);
  return m;
}

static void test_ipc_local_auth(void) {
  fio_socket_i fds[2];
  const fio_u128 other = fio_u128_init64(1, 2);
  fio___test_auth_count = 0;
  /* a valid declaration authenticates; later messages execute in order */
  FIO_ASSERT(!fio_sock_socketpair(fds), "create local auth sockets");
  fio_io_s *io =
      fio_io_attach_fd(fds[0], &FIO___IPC.protocol_ipc_hello, NULL, NULL);
  FIO_ASSERT(io, "attach local hello connection");
  fio_queue_perform_all(fio_io_queue());
  fio___test_auth_deliver(io,
                          fio___test_hello_frame(&FIO___IPC.uuid,
                                                 FIO___IPC_ROLE_WORKER,
                                                 fio___ipc_now()));
  fio___test_auth_deliver(io, fio___test_auth_msg());
  fio_queue_perform_all(fio_io_queue());
  FIO_ASSERT(fio_io_protocol(io) == &FIO___IPC.protocol_ipc,
             "declared connection moves to the broadcast protocol");
  FIO_ASSERT(fio___test_auth_count == 1, "post-declaration message executes");
  fio_io_close_now(io);
  fio_queue_perform_all(fio_io_queue());
  fio_sock_close(fds[1]);

  /* message first, reflected role, foreign instance, stale, future: closed */
  for (size_t i = 0; i < 5; ++i) {
    FIO_ASSERT(!fio_sock_socketpair(fds), "create rejection sockets");
    io = fio_io_attach_fd(fds[0], &FIO___IPC.protocol_ipc_hello, NULL, NULL);
    fio_queue_perform_all(fio_io_queue());
    const uint64_t now = fio___ipc_parser(io)->opened;
    fio_io_s *hold = fio_io_dup(io);
    fio_ipc_s *m =
        (i == 0)   ? fio___test_auth_msg()
        : (i == 1) ? fio___test_hello_frame(&FIO___IPC.uuid,
                                            FIO___IPC_ROLE_MASTER,
                                            now)
        : (i == 2) ? fio___test_hello_frame(&other, FIO___IPC_ROLE_WORKER, now)
        : (i == 3) ? fio___test_hello_frame(&FIO___IPC.uuid,
                                            FIO___IPC_ROLE_WORKER,
                                            now - FIO___IPC_LOCAL_DRIFT)
                   : fio___test_hello_frame(&FIO___IPC.uuid,
                                            FIO___IPC_ROLE_WORKER,
                                            now + FIO___IPC_LOCAL_DRIFT);
    fio___test_auth_deliver(io, m);
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(!fio_io_is_open(hold) && fio___test_auth_count == 1 &&
                   fio_io_protocol(hold) != &FIO___IPC.protocol_ipc,
               "invalid first message closes the connection (%zu)",
               i);
    fio_io_free(hold);
    fio_queue_perform_all(fio_io_queue());
    fio_sock_close(fds[1]);
  }
}

/* *****************************************************************************
Test: security regressions (replay window, pre-auth limits, NULL payloads)
***************************************************************************** */

static void test_ipc_security_regressions(void) {
  /* NULL buffers with a length publish zeros, never uninitialized memory */
  {
    fio_ipc_s *m = fio_ipc_new(.data = FIO_IPC_DATA(FIO_BUF_INFO2(NULL, 16)));
    FIO_ASSERT(m && m->len == 16, "NULL buffer payload is authored");
    for (size_t i = 0; i < 16; ++i)
      FIO_ASSERT(!m->data[i], "NULL buffer payload is zeroed");
    fio___ipc_free(m);
  }
  /* Replay filter: in-window IDs survive compaction; expired IDs are dropped */
  {
    fio___ipc_cluster_filter_destroy(&FIO___IPC.received);
    FIO___IPC.received_floor = 0;
    FIO___IPC.received_limit = 0;
    const uint64_t now = (uint64_t)fio_io_last_tick_time();
    FIO_ASSERT(!fio___ipc_replay_record(now - 1000, 7), "record target ID");
    for (uint64_t i = 0; i <= FIO___IPC_REPLAY_MIN; ++i)
      FIO_ASSERT(!fio___ipc_replay_record(now + 1 + (i & 1023), i + 100),
                 "record newer ID %zu",
                 (size_t)i);
    FIO_ASSERT(fio___ipc_replay_seen(now - 1000, 7) &&
                   fio___ipc_replay_record(now - 1000, 7) == -1,
               "an in-window ID can't be evicted and replayed");
    fio___ipc_cluster_filter_destroy(&FIO___IPC.received);
    FIO___IPC.received_limit = 0;
    for (uint64_t i = 0; i < FIO___IPC_REPLAY_MIN; ++i)
      FIO_ASSERT(!fio___ipc_replay_record(now - 60000, i + 1),
                 "record expired ID");
    FIO_ASSERT(!fio___ipc_replay_record(now, 1) &&
                   FIO___IPC.received.count == 1 &&
                   !FIO___IPC.received_floor,
               "compaction drops expired IDs only");
    /* overflow fails closed: older timestamps are refused */
    fio___ipc_cluster_filter_destroy(&FIO___IPC.received);
    FIO___IPC.received_limit = 0;
    for (uint64_t i = 0; i <= FIO___IPC_REPLAY_MAX; ++i)
      FIO_ASSERT(!fio___ipc_replay_record(now - 20000 + (i >> 5), i + 1),
                 "record in-window ID %zu",
                 (size_t)i);
    FIO_ASSERT(FIO___IPC.received_floor &&
                   fio___ipc_replay_seen(now - 20000, 1) &&
                   fio___ipc_replay_record(now - 20000, 0xFFFFFFFFULL) == -1,
               "replay filter overflow refuses older timestamps");
    fio___ipc_cluster_filter_destroy(&FIO___IPC.received);
    FIO___IPC.received_floor = 0;
    FIO___IPC.received_limit = 0;
  }
  /* Unauthenticated peers can't make us allocate large frames */
  for (size_t i = 0; i < 2; ++i) {
    fio_socket_i fds[2];
    FIO_ASSERT(!fio_sock_socketpair(fds), "create pre-auth sockets");
    fio_io_s *io =
        fio_io_attach_fd(fds[0], &FIO___IPC.protocol_ipc_hello, NULL, NULL);
    fio_queue_perform_all(fio_io_queue());
    fio_io_s *hold = fio_io_dup(io);
    fio___ipc_parser_s *p = fio___ipc_parser(io);
    char hdr[8] = {0};
    fio_u2buf32_le(hdr, (1UL << 20)); /* published length: 1 MiB */
    if (i) /* pretend a (pending) declaration was already queued */
      p->first = 1;
    else
      fprintf(stderr, "      (expect SECURITY message about IPC length)\n");
    FIO_ASSERT(fio_sock_write(fds[1], hdr, 8) == 8, "write large header");
    fio___ipc_on_data_master(io);
    FIO_ASSERT(!p->msg, "no pre-auth allocation for a large frame (%zu)", i);
    FIO_ASSERT(i ? (fio_io_is_open(hold) && fio_io_is_suspended(hold))
                 : !fio_io_is_open(hold),
               "large first frame closes, later ones wait (%zu)",
               i);
    fio_io_close_now(hold);
    fio_io_free(hold);
    fio_queue_perform_all(fio_io_queue());
    fio_sock_close(fds[1]);
  }
}

int main(void) {
  fprintf(stderr, "=== IPC correctness tests ===\n");

  fprintf(stderr, "\t* IPC structure size:   \t%zu bytes\n", sizeof(fio_ipc_s));
  fprintf(stderr,
          "\t* IPC Wire Header + MAC:\t%zu bytes\n",
          ((size_t)fio___ipc_sizeof_header() + 16));
  fprintf(stderr,
          "\t* IPC Encrypted size:   \t%zu bytes\n",
          (FIO_PTR_FIELD_OFFSET(fio_ipc_s, data) -
           FIO_PTR_FIELD_OFFSET(fio_ipc_s, call)));
  fprintf(stderr, "\t* IPC URL max length:   \t%d\n", FIO_IPC_URL_MAX_LENGTH);

  test_ipc_message_lifecycle();
  test_ipc_url_management();
  test_ipc_message_fields();
  test_ipc_error_handling();
  test_ipc_data_integrity();
  test_ipc_master_execution();
  test_ipc_reply_mechanism();
  test_ipc_flags_preservation();
  test_ipc_broadcast();
  test_ipc_message_sizes();
  test_ipc_timestamp_id();
  test_ipc_callbacks();
  test_ipc_memory_safety();
  test_ipc_multi_buffer_data();
  test_ipc_encryption_verification();
  test_ipc_encryption_nonce_unique();
  test_ipc_reply_fresh_id();
  test_ipc_local_auth();
  test_ipc_rpc_message_creation();
  test_ipc_rpc_opcode_registration();
  test_ipc_rpc_listen_connect_validation();
  test_ipc_rpc_encryption();
  test_ipc_rpc_filter();
  test_ipc_udp_discovery_message();
  test_ipc_cluster_lifecycle();
  test_ipc_security_regressions();

  fprintf(stderr, "=== IPC correctness tests passed ===\n");
  return 0;
}
