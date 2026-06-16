/* *****************************************************************************
IPC Stress Test - Multi-Process / Long-Running Behavior (404 ipc.h)

Covers the long-running multi-process IPC behavior from tests-old/ipc.c.
This test starts the IO reactor with worker processes and exercises
worker->master calls, broadcasts, RPC op-codes, and large message transfer.

Guarded with #ifdef _WIN32 to log FIO_LOG_WARNING("SKIPPED") and return
success on Windows, because the POSIX fork()-based worker model is not
available there.
***************************************************************************** */
#include "tests/test-helpers.h"

#define FIO_IPC
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Configuration
***************************************************************************** */

#define F_TEST_WORKERS 2
#define F_BCAST_DELAY_MS 1000
#define F_BCAST2_DELAY_MS 1500
#define F_TIMEOUT_MS 4000

/* *****************************************************************************
Test State - Atomic Counters for Cross-Process Result Tracking
***************************************************************************** */

/* --- Test: Worker->Master Call (f_call_) --- */
static volatile size_t f_call_master_received = 0;

static void f_call_master_handler(fio_ipc_s *msg) {
  fio_atomic_add(&f_call_master_received, 1);
  const char *expected = "worker_request";
  if (msg->len == FIO_STRLEN(expected) &&
      FIO_MEMCMP(msg->data, expected, msg->len) == 0) {
    const char *reply = "master_reply";
    fio_ipc_reply(
        msg,
        .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)reply, FIO_STRLEN(reply))),
        .done = 1);
  }
}

/* --- Test: udata Preservation (f_udata_) --- */
#define F_UDATA_MARKER ((void *)0xDEADBEEFCAFEBABEULL)
static volatile size_t f_udata_master_received = 0;
static volatile size_t f_udata_verified = 0;
static volatile size_t f_udata_failed = 0;

static void f_udata_master_handler(fio_ipc_s *msg) {
  fio_atomic_add(&f_udata_master_received, 1);
  if (msg->udata == F_UDATA_MARKER) {
    fio_atomic_add(&f_udata_verified, 1);
  } else {
    fio_atomic_add(&f_udata_failed, 1);
  }
  const char *reply =
      (msg->udata == F_UDATA_MARKER) ? "udata_ok" : "udata_fail";
  fio_ipc_reply(
      msg,
      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)reply, FIO_STRLEN(reply))),
      .done = 1);
}

/* --- Test: Streaming Replies (f_stream_) --- */
static volatile size_t f_stream_master_received = 0;

static void f_stream_master_handler(fio_ipc_s *msg) {
  fio_atomic_add(&f_stream_master_received, 1);
  const char *reply = "stream_reply_1";
  fio_ipc_reply(
      msg,
      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)reply, FIO_STRLEN(reply))),
      .done = 1);
}

/* --- Test: Broadcast Master->Workers (f_bcast_) --- */
static volatile size_t f_bcast_confirmations = 0;

static void f_bcast_confirm_handler(fio_ipc_s *ipc) {
  (void)ipc;
  fio_atomic_add(&f_bcast_confirmations, 1);
}

static void f_bcast_worker_handler(fio_ipc_s *msg) {
  const char *expected = "broadcast_data";
  if (msg->len == FIO_STRLEN(expected) &&
      FIO_MEMCMP(msg->data, expected, msg->len) == 0) {
    const char *confirm = "success";
    fio_ipc_call(.call = f_bcast_confirm_handler,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)confirm, FIO_STRLEN(confirm))));
  }
}

/* --- Test: Binary Data (f_binary_) --- */
static volatile size_t f_binary_master_verified = 0;

static void f_binary_master_handler(fio_ipc_s *msg) {
  uint8_t expected[] = {0x00, 0x01, 0x02, 0x00, 0xFF, 0xFE, 0x00, 0x03};
  if (msg->len == sizeof(expected) &&
      FIO_MEMCMP(msg->data, expected, sizeof(expected)) == 0) {
    fio_atomic_add(&f_binary_master_verified, 1);
  }
  const char *reply = "binary_ok";
  fio_ipc_reply(
      msg,
      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)reply, FIO_STRLEN(reply))),
      .done = 1);
}

/* --- Test: Concurrent Workers (f_conc_) --- */
static volatile size_t f_conc_master_calls = 0;

static void f_conc_master_handler(fio_ipc_s *msg) {
  fio_atomic_add(&f_conc_master_calls, 1);
  char reply[64];
  int len = snprintf(reply, sizeof(reply), "ack_%u", msg->len);
  fio_ipc_reply(msg,
                .data = FIO_IPC_DATA(FIO_BUF_INFO2(reply, (size_t)len)),
                .done = 1);
}

/* --- Test: Data Integrity 64B/4KB/64KB (f_data_) --- */
static volatile size_t f_data_64_verified = 0;
static volatile size_t f_data_4k_verified = 0;
static volatile size_t f_data_64k_verified = 0;

static void f_data_master_handler(fio_ipc_s *msg) {
  int valid = 1;
  for (size_t i = 0; i < msg->len && valid; ++i) {
    if ((uint8_t)msg->data[i] != (uint8_t)(i & 0xFF))
      valid = 0;
  }
  if (valid) {
    if (msg->len == 64)
      fio_atomic_add(&f_data_64_verified, 1);
    else if (msg->len == 4096)
      fio_atomic_add(&f_data_4k_verified, 1);
    else if (msg->len == 65536)
      fio_atomic_add(&f_data_64k_verified, 1);
  }
  const char *reply = valid ? "verified" : "failed";
  fio_ipc_reply(
      msg,
      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)reply, FIO_STRLEN(reply))),
      .done = 1);
}

/* --- Test: Multi-Buffer IPC (f_mbuf_) --- */
#define F_MBUF_HEADER "HDR:"
#define F_MBUF_NUM1 0xDEADBEEFU
#define F_MBUF_SEP ":SEP:"
#define F_MBUF_NUM2 0xCAFEBABE12345678ULL
#define F_MBUF_FOOTER ":FTR"

static volatile size_t f_mbuf_master_verified = 0;
static volatile size_t f_mbuf_data_fail = 0;

static void f_mbuf_master_handler(fio_ipc_s *msg) {
  size_t expected_len = FIO_STRLEN(F_MBUF_HEADER) + sizeof(uint32_t) +
                        FIO_STRLEN(F_MBUF_SEP) + sizeof(uint64_t) +
                        FIO_STRLEN(F_MBUF_FOOTER);
  int valid = 1;
  if (msg->len != expected_len) {
    valid = 0;
  }
  if (valid) {
    size_t offset = 0;
    if (FIO_MEMCMP(msg->data + offset,
                   F_MBUF_HEADER,
                   FIO_STRLEN(F_MBUF_HEADER)) != 0)
      valid = 0;
    offset += FIO_STRLEN(F_MBUF_HEADER);
    if (valid) {
      uint32_t num1;
      FIO_MEMCPY(&num1, msg->data + offset, sizeof(uint32_t));
      if (num1 != F_MBUF_NUM1)
        valid = 0;
      offset += sizeof(uint32_t);
    }
    if (valid) {
      if (FIO_MEMCMP(msg->data + offset, F_MBUF_SEP, FIO_STRLEN(F_MBUF_SEP)) !=
          0)
        valid = 0;
      offset += FIO_STRLEN(F_MBUF_SEP);
    }
    if (valid) {
      uint64_t num2;
      FIO_MEMCPY(&num2, msg->data + offset, sizeof(uint64_t));
      if (num2 != F_MBUF_NUM2)
        valid = 0;
      offset += sizeof(uint64_t);
    }
    if (valid) {
      if (FIO_MEMCMP(msg->data + offset,
                     F_MBUF_FOOTER,
                     FIO_STRLEN(F_MBUF_FOOTER)) != 0)
        valid = 0;
    }
  }
  if (valid)
    fio_atomic_add(&f_mbuf_master_verified, 1);
  else
    fio_atomic_add(&f_mbuf_data_fail, 1);

  const char *reply_prefix = "REPLY:";
  uint16_t reply_num = 0xABCD;
  const char *reply_suffix = ":OK";
  fio_ipc_reply(
      msg,
      .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)reply_prefix),
                           FIO_BUF_INFO2((char *)&reply_num, sizeof(reply_num)),
                           FIO_BUF_INFO1((char *)reply_suffix)),
      .done = 1);
}

/* --- Test: Broadcast Exclude (f_bexcl_) --- */
static volatile size_t f_bexcl_data_fail = 0;

static void f_bexcl_handler(fio_ipc_s *msg) {
  const char *expected = "exclude_test_data";
  if (msg->len != FIO_STRLEN(expected) ||
      FIO_MEMCMP(msg->data, expected, msg->len) != 0) {
    fio_atomic_add(&f_bexcl_data_fail, 1);
  }
}

/* --- Test: Broadcast Verify (f_bver_) --- */
static volatile size_t f_bver_data_fail = 0;

static void f_bver_handler(fio_ipc_s *msg) {
  const char *expected = "verify_data";
  if (msg->len != FIO_STRLEN(expected) ||
      FIO_MEMCMP(msg->data, expected, msg->len) != 0) {
    fio_atomic_add(&f_bver_data_fail, 1);
  }
}

/* --- Test: Child Broadcast (f_cbcast_) --- */
static volatile size_t f_cbcast_data_fail = 0;
static volatile size_t f_cbcast_started = 0;

static void f_cbcast_handler(fio_ipc_s *msg) {
  const char *expected = "child_broadcast_data";
  if (msg->len != FIO_STRLEN(expected) ||
      FIO_MEMCMP(msg->data, expected, msg->len) != 0) {
    fio_atomic_add(&f_cbcast_data_fail, 1);
  }
}

/* --- Test: Child Broadcast Exclude (f_cexcl_) --- */
static volatile size_t f_cexcl_master_received = 0;
static volatile size_t f_cexcl_data_fail = 0;
static volatile size_t f_cexcl_started = 0;

static void f_cexcl_handler(fio_ipc_s *msg) {
  if (fio_io_is_master()) {
    fio_atomic_add(&f_cexcl_master_received, 1);
    const char *expected = "child_excl_data";
    if (msg->len != FIO_STRLEN(expected) ||
        FIO_MEMCMP(msg->data, expected, msg->len) != 0) {
      fio_atomic_add(&f_cexcl_data_fail, 1);
    }
  }
}

/* --- Test: Redistribution (f_redist_) --- */
static volatile size_t f_redist_master_received = 0;
static volatile size_t f_redist_data_fail = 0;
static volatile size_t f_redist_started = 0;

static void f_redist_handler(fio_ipc_s *msg) {
  if (fio_io_is_master()) {
    fio_atomic_add(&f_redist_master_received, 1);
    const char *expected = "redistribution_test";
    if (msg->len != FIO_STRLEN(expected) ||
        FIO_MEMCMP(msg->data, expected, msg->len) != 0) {
      fio_atomic_add(&f_redist_data_fail, 1);
    }
  }
}

/* --- Test: Worker Broadcast (f_wbcast_) --- */
static volatile size_t f_wbcast_master_received = 0;
static volatile size_t f_wbcast_data_fail = 0;
static volatile size_t f_wbcast_started = 0;

static void f_wbcast_handler(fio_ipc_s *msg) {
  if (fio_io_is_master()) {
    fio_atomic_add(&f_wbcast_master_received, 1);
    const char *expected = "worker_mistaken_broadcast";
    if (msg->len != FIO_STRLEN(expected) ||
        FIO_MEMCMP(msg->data, expected, msg->len) != 0) {
      fio_atomic_add(&f_wbcast_data_fail, 1);
    }
  }
}

/* --- Test: Large Message 1MB (f_large_) --- */
static volatile size_t f_large_master_verified = 0;

static void f_large_master_handler(fio_ipc_s *msg) {
  int valid = 1;
  size_t expected_size = 1024 * 1024;
  if (msg->len != expected_size) {
    valid = 0;
  } else {
    for (size_t i = 0; i < msg->len && valid; ++i) {
      if ((uint8_t)msg->data[i] != (uint8_t)(i & 0xFF))
        valid = 0;
    }
  }
  if (valid)
    fio_atomic_add(&f_large_master_verified, 1);

  const char *reply = valid ? "large_verified" : "large_failed";
  fio_ipc_reply(
      msg,
      .data = FIO_IPC_DATA(FIO_BUF_INFO2((char *)reply, FIO_STRLEN(reply))),
      .done = 1);
}

/* --- Test: Worker-Initiated RPC (f_wrpc_) --- */
#define F_WRPC_OPCODE 0x00ABCDEF
#define F_WRPC_DATA "worker_rpc_test_data"
#define F_WRPC_UDATA ((void *)0xDEADC0DE)

static volatile size_t f_wrpc_master_received = 0;
static volatile size_t f_wrpc_master_verified = 0;
static volatile size_t f_wrpc_data_fail = 0;
static volatile void *f_wrpc_udata_received = NULL;

static void f_wrpc_handler(fio_ipc_s *msg) {
  if (fio_io_is_master()) {
    fio_atomic_add(&f_wrpc_master_received, 1);
    f_wrpc_udata_received = msg->udata;
  }
  const char *expected = F_WRPC_DATA;
  if (msg->len == FIO_STRLEN(expected) &&
      FIO_MEMCMP(msg->data, expected, msg->len) == 0) {
    if (fio_io_is_master())
      fio_atomic_add(&f_wrpc_master_verified, 1);
  } else {
    if (fio_io_is_master())
      fio_atomic_add(&f_wrpc_data_fail, 1);
  }
}

/* --- Test: Worker-Initiated RPC Multi-Buffer (f_wrpcmb_) --- */
#define F_WRPCMB_OPCODE 0x00FEDCBA
#define F_WRPCMB_PREFIX "RPC:"
#define F_WRPCMB_NUM1 0xCAFEBABEU
#define F_WRPCMB_SEP ":DATA:"
#define F_WRPCMB_NUM2 0x123456789ABCDEF0ULL
#define F_WRPCMB_SUFFIX ":END"

static volatile size_t f_wrpcmb_master_received = 0;
static volatile size_t f_wrpcmb_master_verified = 0;
static volatile size_t f_wrpcmb_data_fail = 0;

static void f_wrpcmb_handler(fio_ipc_s *msg) {
  if (fio_io_is_master())
    fio_atomic_add(&f_wrpcmb_master_received, 1);
  size_t expected_len = FIO_STRLEN(F_WRPCMB_PREFIX) + sizeof(uint32_t) +
                        FIO_STRLEN(F_WRPCMB_SEP) + sizeof(uint64_t) +
                        FIO_STRLEN(F_WRPCMB_SUFFIX);
  int valid = 1;
  if (msg->len != expected_len)
    valid = 0;
  if (valid) {
    size_t offset = 0;
    if (FIO_MEMCMP(msg->data + offset,
                   F_WRPCMB_PREFIX,
                   FIO_STRLEN(F_WRPCMB_PREFIX)) != 0)
      valid = 0;
    offset += FIO_STRLEN(F_WRPCMB_PREFIX);
    if (valid) {
      uint32_t num1;
      FIO_MEMCPY(&num1, msg->data + offset, sizeof(uint32_t));
      if (num1 != F_WRPCMB_NUM1)
        valid = 0;
      offset += sizeof(uint32_t);
    }
    if (valid) {
      if (FIO_MEMCMP(msg->data + offset,
                     F_WRPCMB_SEP,
                     FIO_STRLEN(F_WRPCMB_SEP)) != 0)
        valid = 0;
      offset += FIO_STRLEN(F_WRPCMB_SEP);
    }
    if (valid) {
      uint64_t num2;
      FIO_MEMCPY(&num2, msg->data + offset, sizeof(uint64_t));
      if (num2 != F_WRPCMB_NUM2)
        valid = 0;
      offset += sizeof(uint64_t);
    }
    if (valid) {
      if (FIO_MEMCMP(msg->data + offset,
                     F_WRPCMB_SUFFIX,
                     FIO_STRLEN(F_WRPCMB_SUFFIX)) != 0)
        valid = 0;
    }
  }
  if (valid) {
    if (fio_io_is_master())
      fio_atomic_add(&f_wrpcmb_master_verified, 1);
  } else {
    if (fio_io_is_master())
      fio_atomic_add(&f_wrpcmb_data_fail, 1);
  }
}

/* *****************************************************************************
Worker Startup (FIO_CALL_ON_START)
***************************************************************************** */

static void f_worker_start(void *ignr_) {
  (void)ignr_;
  if (!fio_io_is_worker())
    return;

  /* Worker->Master Call */
  {
    const char *request = "worker_request";
    fio_ipc_call(.call = f_call_master_handler,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)request, FIO_STRLEN(request))));
  }

  /* udata Preservation */
  {
    const char *request = "udata_test_request";
    fio_ipc_call(.call = f_udata_master_handler,
                 .udata = F_UDATA_MARKER,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)request, FIO_STRLEN(request))));
  }

  /* Streaming Replies */
  {
    fio_ipc_call(.call = f_stream_master_handler,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"stream_request")));
  }

  /* Binary Data */
  {
    unsigned char binary_data[] =
        {0x00, 0x01, 0x02, 0x00, 0xFF, 0xFE, 0x00, 0x03};
    fio_ipc_call(.call = f_binary_master_handler,
                 .data = FIO_IPC_DATA(
                     FIO_BUF_INFO2((char *)binary_data, sizeof(binary_data))));
  }

  /* Concurrent Workers */
  {
    char request[32];
    int len = snprintf(request, sizeof(request), "worker_%d", fio_io_pid());
    fio_ipc_call(.call = f_conc_master_handler,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO2(request, (size_t)len)));
  }

  /* Data Integrity 64B */
  {
    char data[64];
    for (size_t i = 0; i < 64; ++i)
      data[i] = (char)(i & 0xFF);
    fio_ipc_call(.call = f_data_master_handler,
                 .data = FIO_IPC_DATA(FIO_BUF_INFO2(data, 64)));
  }

  /* Data Integrity 4KB */
  {
    char *data = (char *)FIO_MEM_REALLOC(NULL, 0, 4096, 0);
    if (data) {
      for (size_t i = 0; i < 4096; ++i)
        data[i] = (char)(i & 0xFF);
      fio_ipc_call(.call = f_data_master_handler,
                   .data = FIO_IPC_DATA(FIO_BUF_INFO2(data, 4096)));
      FIO_MEM_FREE(data, 4096);
    }
  }

  /* Data Integrity 64KB */
  {
    size_t size = 65536;
    char *data = (char *)FIO_MEM_REALLOC(NULL, 0, size, 0);
    if (data) {
      for (size_t i = 0; i < size; ++i)
        data[i] = (char)(i & 0xFF);
      fio_ipc_call(.call = f_data_master_handler,
                   .data = FIO_IPC_DATA(FIO_BUF_INFO2(data, size)));
      FIO_MEM_FREE(data, size);
    }
  }

  /* Multi-Buffer IPC */
  {
    const char *header = F_MBUF_HEADER;
    uint32_t num1 = F_MBUF_NUM1;
    const char *sep = F_MBUF_SEP;
    uint64_t num2 = F_MBUF_NUM2;
    const char *footer = F_MBUF_FOOTER;

    fio_ipc_call(.call = f_mbuf_master_handler,
                 .data =
                     FIO_IPC_DATA(FIO_BUF_INFO1((char *)header),
                                  FIO_BUF_INFO2((char *)&num1, sizeof(num1)),
                                  FIO_BUF_INFO1((char *)sep),
                                  FIO_BUF_INFO2((char *)&num2, sizeof(num2)),
                                  FIO_BUF_INFO1((char *)footer)));
  }

  /* Child Broadcast (first worker only) */
  if (fio_atomic_add(&f_cbcast_started, 1) == 0) {
    const char *data = "child_broadcast_data";
    fio_ipc_local(.call = f_cbcast_handler,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  /* Child Broadcast Exclude (first worker only) */
  if (fio_atomic_add(&f_cexcl_started, 1) == 0) {
    const char *data = "child_excl_data";
    fio_ipc_local(.call = f_cexcl_handler,
                  .exclude = FIO_IPC_EXCLUDE_SELF,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  /* Redistribution (first worker only) */
  if (fio_atomic_add(&f_redist_started, 1) == 0) {
    const char *data = "redistribution_test";
    fio_ipc_local(.call = f_redist_handler,
                  .exclude = FIO_IPC_EXCLUDE_SELF,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  /* Worker Broadcast (first worker only) */
  if (fio_atomic_add(&f_wbcast_started, 1) == 0) {
    const char *data = "worker_mistaken_broadcast";
    fio_ipc_local(.call = f_wbcast_handler,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  /* Large Message 1MB */
  {
    size_t size = 1024 * 1024;
    char *data = (char *)FIO_MEM_REALLOC(NULL, 0, size, 0);
    if (data) {
      for (size_t i = 0; i < size; ++i)
        data[i] = (char)(i & 0xFF);
      fio_ipc_call(.call = f_large_master_handler,
                   .data = FIO_IPC_DATA(FIO_BUF_INFO2(data, size)));
      FIO_MEM_FREE(data, size);
    }
  }

  /* Worker-Initiated RPC */
  {
    const char *data = F_WRPC_DATA;
    fio_ipc_broadcast(.opcode = F_WRPC_OPCODE,
                      .data = FIO_IPC_DATA(
                          FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  /* Worker-Initiated RPC Multi-Buffer */
  {
    const char *prefix = F_WRPCMB_PREFIX;
    uint32_t num1 = F_WRPCMB_NUM1;
    const char *sep = F_WRPCMB_SEP;
    uint64_t num2 = F_WRPCMB_NUM2;
    const char *suffix = F_WRPCMB_SUFFIX;

    fio_ipc_broadcast(.opcode = F_WRPCMB_OPCODE,
                      .data = FIO_IPC_DATA(
                          FIO_BUF_INFO1((char *)prefix),
                          FIO_BUF_INFO2((char *)&num1, sizeof(num1)),
                          FIO_BUF_INFO1((char *)sep),
                          FIO_BUF_INFO2((char *)&num2, sizeof(num2)),
                          FIO_BUF_INFO1((char *)suffix)));
  }
}

/* *****************************************************************************
Master Timers
***************************************************************************** */

static int f_master_bcast_trigger(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  {
    const char *data = "broadcast_data";
    fio_ipc_local(.call = f_bcast_worker_handler,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  {
    const char *data = "exclude_test_data";
    fio_ipc_local(.call = f_bexcl_handler,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  return -1;
}

static int f_master_bcast2_trigger(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;

  {
    const char *data = "verify_data";
    fio_ipc_local(.call = f_bver_handler,
                  .data = FIO_IPC_DATA(
                      FIO_BUF_INFO2((char *)data, FIO_STRLEN(data))));
  }

  return -1;
}

static int f_timeout(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  if (!fio_io_is_master())
    return -1;
  fio_io_stop();
  return -1;
}

/* *****************************************************************************
Run and Verify
***************************************************************************** */

static void fio___test_ipc_stress_run(void) {
  fprintf(stderr,
          "* Testing IPC stress (multi-process, %d workers).\n",
          F_TEST_WORKERS);

  f_call_master_received = 0;
  f_udata_master_received = 0;
  f_udata_verified = 0;
  f_udata_failed = 0;
  f_stream_master_received = 0;
  f_bcast_confirmations = 0;
  f_binary_master_verified = 0;
  f_conc_master_calls = 0;
  f_data_64_verified = 0;
  f_data_4k_verified = 0;
  f_data_64k_verified = 0;
  f_mbuf_master_verified = 0;
  f_mbuf_data_fail = 0;
  f_bexcl_data_fail = 0;
  f_bver_data_fail = 0;
  f_cbcast_data_fail = 0;
  f_cbcast_started = 0;
  f_cexcl_master_received = 0;
  f_cexcl_data_fail = 0;
  f_cexcl_started = 0;
  f_redist_master_received = 0;
  f_redist_data_fail = 0;
  f_redist_started = 0;
  f_wbcast_master_received = 0;
  f_wbcast_data_fail = 0;
  f_wbcast_started = 0;
  f_large_master_verified = 0;
  f_wrpc_master_received = 0;
  f_wrpc_master_verified = 0;
  f_wrpc_data_fail = 0;
  f_wrpc_udata_received = NULL;
  f_wrpcmb_master_received = 0;
  f_wrpcmb_master_verified = 0;
  f_wrpcmb_data_fail = 0;

  /* Register RPC opcodes BEFORE fio_io_start() */
  fio_ipc_opcode_register(.opcode = F_WRPC_OPCODE,
                          .call = f_wrpc_handler,
                          .udata = F_WRPC_UDATA);
  fio_ipc_opcode_register(.opcode = F_WRPCMB_OPCODE,
                          .call = f_wrpcmb_handler,
                          .udata = NULL);

  fio_state_callback_add(FIO_CALL_ON_START, f_worker_start, NULL);

  fio_io_run_every(.fn = f_master_bcast_trigger,
                   .every = F_BCAST_DELAY_MS,
                   .repetitions = 1);
  fio_io_run_every(.fn = f_master_bcast2_trigger,
                   .every = F_BCAST2_DELAY_MS,
                   .repetitions = 1);
  fio_io_run_every(.fn = f_timeout, .every = F_TIMEOUT_MS, .repetitions = 1);

  fio_io_start(F_TEST_WORKERS);

  fio_state_callback_remove(FIO_CALL_ON_START, f_worker_start, NULL);
  fio_ipc_opcode_register(.opcode = F_WRPC_OPCODE, .call = NULL);
  fio_ipc_opcode_register(.opcode = F_WRPCMB_OPCODE, .call = NULL);

  fprintf(stderr, "  - Worker->Master call: ");
  FIO_ASSERT(f_call_master_received >= 1,
             "Should receive at least 1 call from worker (got %zu)",
             f_call_master_received);
  fprintf(stderr, "PASS (got %zu)\n", f_call_master_received);

  fprintf(stderr, "  - udata preservation: ");
  FIO_ASSERT(f_udata_master_received >= 1,
             "Should receive at least 1 IPC call with udata (got %zu)",
             f_udata_master_received);
  FIO_ASSERT(f_udata_verified >= 1,
             "udata should be verified at least once (got %zu)",
             f_udata_verified);
  FIO_ASSERT(f_udata_failed == 0,
             "udata verification should not fail (got %zu failures)",
             f_udata_failed);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Streaming replies: ");
  FIO_ASSERT(f_stream_master_received >= 1,
             "Should receive at least 1 stream request (got %zu)",
             f_stream_master_received);
  fprintf(stderr, "PASS (got %zu)\n", f_stream_master_received);

  fprintf(stderr, "  - Broadcast master->workers: ");
  FIO_ASSERT(f_bcast_confirmations >= 1,
             "Should receive at least 1 confirmation (got %zu)",
             f_bcast_confirmations);
  fprintf(stderr, "PASS (got %zu)\n", f_bcast_confirmations);

  fprintf(stderr, "  - Binary data: ");
  FIO_ASSERT(f_binary_master_verified >= 1,
             "Should verify at least 1 binary message (got %zu)",
             f_binary_master_verified);
  fprintf(stderr, "PASS (got %zu)\n", f_binary_master_verified);

  fprintf(stderr, "  - Concurrent workers: ");
  FIO_ASSERT(f_conc_master_calls >= F_TEST_WORKERS,
             "Should receive calls from %d workers (got %zu)",
             F_TEST_WORKERS,
             f_conc_master_calls);
  fprintf(stderr, "PASS (got %zu)\n", f_conc_master_calls);

  fprintf(stderr, "  - Data integrity 64B: ");
  FIO_ASSERT(f_data_64_verified >= 1,
             "Should verify at least 1 64B message (got %zu)",
             f_data_64_verified);
  fprintf(stderr, "PASS (got %zu)\n", f_data_64_verified);

  fprintf(stderr, "  - Data integrity 4KB: ");
  FIO_ASSERT(f_data_4k_verified >= 1,
             "Should verify at least 1 4KB message (got %zu)",
             f_data_4k_verified);
  fprintf(stderr, "PASS (got %zu)\n", f_data_4k_verified);

  fprintf(stderr, "  - Data integrity 64KB: ");
  FIO_ASSERT(f_data_64k_verified >= 1,
             "Should verify at least 1 64KB message (got %zu)",
             f_data_64k_verified);
  fprintf(stderr, "PASS (got %zu)\n", f_data_64k_verified);

  fprintf(stderr, "  - Multi-buffer IPC: ");
  FIO_ASSERT(f_mbuf_master_verified >= 1,
             "Should verify at least 1 multi-buffer message (got %zu)",
             f_mbuf_master_verified);
  FIO_ASSERT(f_mbuf_data_fail == 0,
             "Multi-buffer data failures: %zu",
             f_mbuf_data_fail);
  fprintf(stderr, "PASS (got %zu)\n", f_mbuf_master_verified);

  fprintf(stderr, "  - Broadcast exclude: ");
  FIO_ASSERT(f_bexcl_data_fail == 0,
             "Broadcast exclude data failures: %zu",
             f_bexcl_data_fail);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Broadcast verify: ");
  FIO_ASSERT(f_bver_data_fail == 0,
             "Broadcast verify data failures: %zu",
             f_bver_data_fail);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Child broadcast: ");
  FIO_ASSERT(f_cbcast_data_fail == 0,
             "Child broadcast data failures: %zu",
             f_cbcast_data_fail);
  fprintf(stderr, "PASS\n");

  fprintf(stderr, "  - Child broadcast exclude: ");
  FIO_ASSERT(f_cexcl_master_received >= 1,
             "Should receive child broadcast (got %zu)",
             f_cexcl_master_received);
  FIO_ASSERT(f_cexcl_data_fail == 0,
             "Child excl broadcast data failures: %zu",
             f_cexcl_data_fail);
  fprintf(stderr, "PASS (got %zu)\n", f_cexcl_master_received);

  fprintf(stderr, "  - Redistribution: ");
  FIO_ASSERT(f_redist_master_received >= 1,
             "Should receive redistribution message (got %zu)",
             f_redist_master_received);
  FIO_ASSERT(f_redist_data_fail == 0,
             "Redistribution data failures: %zu",
             f_redist_data_fail);
  fprintf(stderr, "PASS (got %zu)\n", f_redist_master_received);

  fprintf(stderr, "  - Worker broadcast: ");
  FIO_ASSERT(f_wbcast_master_received >= 1,
             "Should receive worker broadcast (got %zu)",
             f_wbcast_master_received);
  FIO_ASSERT(f_wbcast_data_fail == 0,
             "Worker broadcast data failures: %zu",
             f_wbcast_data_fail);
  fprintf(stderr, "PASS (got %zu)\n", f_wbcast_master_received);

  fprintf(stderr, "  - Large message (1MB): ");
  FIO_ASSERT(f_large_master_verified >= 1,
             "Should verify at least 1 large (1MB) message (got %zu)",
             f_large_master_verified);
  fprintf(stderr, "PASS (got %zu)\n", f_large_master_verified);

  fprintf(stderr, "  - Worker-initiated RPC: ");
  FIO_ASSERT(f_wrpc_master_received >= 1,
             "Should receive at least 1 worker-initiated RPC (got %zu)",
             f_wrpc_master_received);
  FIO_ASSERT(f_wrpc_master_verified >= 1,
             "Should verify at least 1 worker RPC message (got %zu)",
             f_wrpc_master_verified);
  FIO_ASSERT(f_wrpc_data_fail == 0,
             "Worker RPC data failures: %zu",
             f_wrpc_data_fail);
  FIO_ASSERT(f_wrpc_udata_received == F_WRPC_UDATA,
             "Handler should receive local_udata from opcode registration "
             "(got %p, expected %p)",
             f_wrpc_udata_received,
             F_WRPC_UDATA);
  fprintf(stderr, "PASS (got %zu)\n", f_wrpc_master_received);

  fprintf(stderr, "  - Worker-initiated RPC multi-buffer: ");
  FIO_ASSERT(f_wrpcmb_master_received >= 1,
             "Should receive at least 1 multi-buffer worker RPC (got %zu)",
             f_wrpcmb_master_received);
  FIO_ASSERT(f_wrpcmb_master_verified >= 1,
             "Should verify at least 1 multi-buffer worker RPC (got %zu)",
             f_wrpcmb_master_verified);
  FIO_ASSERT(f_wrpcmb_data_fail == 0,
             "Multi-buffer worker RPC data failures: %zu",
             f_wrpcmb_data_fail);
  fprintf(stderr, "PASS (got %zu)\n", f_wrpcmb_master_received);

  fprintf(stderr, "* IPC stress tests passed.\n");
}

/* *****************************************************************************
Main entry point
***************************************************************************** */

int main(void) {
#ifdef _WIN32
  FIO_LOG_WARNING("SKIPPED");
  fprintf(stderr, "=== IPC stress tests skipped on Windows ===\n");
  return 0;
#else
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  fprintf(stderr, "=== IPC stress tests ===\n");
  fio___test_ipc_stress_run();
  fprintf(stderr, "=== IPC stress tests passed ===\n");
  return 0;
#endif
}
