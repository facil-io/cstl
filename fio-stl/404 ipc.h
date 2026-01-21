/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_IPC                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                Pub/Sub Services v2 (IPC-First Architecture)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_IPC) && !defined(H___FIO_IPC___H) &&                           \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_IPC___H
#undef FIO_IPC

#ifndef FIO_IPC_URL_MAX_LENGTH
#define FIO_IPC_URL_MAX_LENGTH 1024
#endif
#ifndef FIO_IPC_MAX_LENGTH
/* maximum message length */
#define FIO_IPC_MAX_LENGTH (128ULL * 1024U * 1024U)
#endif

/* *****************************************************************************
IPC (Inter Process Communication) Settings / Controls
***************************************************************************** */

/** Returns the IPC url to listen to (for incoming connections). */
SFUNC const char *fio_ipc_url(void);

/**
 * Sets the IPC url to listen to (for incoming connections).
 *
 * Can only be called on the master process and only before the IO reactor.
 */
SFUNC int fio_ipc_url_set(const char *url);

/* *****************************************************************************
IPC (Inter Process Communication) Types
***************************************************************************** */

/** IPC message structure (reference counted) */
typedef struct fio_ipc_s {
  fio_io_s *from; /* IO to caller - set by receiver (unsent) */
  /* ----- wire format starts here ----- */
  uint32_t len;   /* Length of data[] (AAD - authenticated, unencrypted) */
  uint16_t flags; /* User settable flags (AAD - authenticated, unencrypted) */
  uint16_t routing_flags; /* Internal (AAD - authenticated, unencrypted) */
  uint64_t timestamp;     /* timestamp (unencrypted, used for nonce) */
  uint64_t id;            /* 8 random bytes (unencrypted, used for nonce) */
  union {
    void (*call)(struct fio_ipc_s *); /* function pointer to call (encrypted) */
    uint32_t opcode;                  /* op-code to execute*/
  };
  void (*on_reply)(struct fio_ipc_s *); /* run on caller (encrypted) */
  void (*on_done)(struct fio_ipc_s *);  /* run on caller (encrypted) */
  void *udata; /* opaque, valid only in caller (encrypted) */
  char data[]; /* Variable-length data + 16-byte MAC at end (encrypted) */
} fio_ipc_s;

/** IPC call arguments */
typedef struct {
  void (*call)(fio_ipc_s *);     /* function to call */
  void (*on_reply)(fio_ipc_s *); /* (optional) reply callback */
  void (*on_done)(fio_ipc_s *);  /* (optional) reply finished callback */
  uint64_t timestamp;            /* (optional) to force timestamp  */
  uint64_t id;                   /* (optional) to force an id value  */
  uint32_t opcode;               /* replaces `call` with op-code if non-zero */
  uint16_t flags;                /* (optional) user-opaque flags  */
  bool cluster; /* if set, this is intended for all machines in cluster */
  bool workers; /* if set, this is intended for master + workers */
  bool others;  /* if set, will not run on calling process */
  void *udata;  /* opaque pointer data for reply */
  fio_buf_info_s *data; /* payload (see FIO_IPC_DATA) */
} fio_ipc_args_s;

typedef struct fio_ipc_opcode_s {
  uint32_t opcode;                      /* Unique op-code value */
  void (*call)(struct fio_ipc_s *);     /* function to call */
  void (*on_reply)(struct fio_ipc_s *); /* (optional) reply callback */
  void (*on_done)(struct fio_ipc_s *);  /* (optional) reply finished callback */
  void *udata; /* opaque, valid only in caller (encrypted) */
} fio_ipc_opcode_s;

/**
 * A helper macro for composing multiple buffers into the request. Fill with
 * fio_buf_info_s - will stop when a buffer has zero length. Use:
 *
 *     .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"example:"),
 *                          FIO_BUF_INFO2(&number, sizeof(number)))
 * */
#define FIO_IPC_DATA(...)                                                      \
  (fio_buf_info_s[]) {                                                         \
    __VA_ARGS__, { 0 }                                                         \
  }

/* *****************************************************************************
IPC/RPC Op-Codes - allows cross machine operation codes for execution
***************************************************************************** */

/**
 * Registers an op-code for message routing.
 *
 * There are two types of messages:
 * 1. Fast Path - function pointers in the message payload (local IPC only).
 * 2. Safe Path - Op-Code in the `call` payload (multi-machine RCP).
 *
 * Op-Codes MUST be non-zero uint32_t values.
 * RESERVED: op-codes >= 0xFF000000 are reserved for internal use.
 *
 * Note: Thread safety requires that this be called before fio_io_start.
 *
 * Returns -1 or failure (not on master process / already registered)
 * */
SFUNC int fio_ipc_opcode_register(fio_ipc_opcode_s opcode);
#define fio_ipc_opcode_register(...)                                           \
  fio_ipc_opcode_register((fio_ipc_opcode_s){__VA_ARGS__})

/** Returns a pointer to a registered op-code, or NULL if missing. */
SFUNC const fio_ipc_opcode_s *fio_ipc_opcode(uint32_t opcode);

/* *****************************************************************************
IPC/RPC Message Exchange - Main API
***************************************************************************** */

/**
 * Call arbitrary code in master process (worker → master / master → master).
 *
 * The `call` function pointer is executed on the master process.
 *
 * Replies are sent back to the caller via `on_reply` callback.
 * When all replies are done, `on_done` is called.
 */
#define fio_ipc_call(...) fio_ipc_send(fio_ipc_new(__VA_ARGS__))

/**
 * Call arbitrary code in master process and workers (all local).
 *
 * Replies can be sent back to the caller only from its local master process.
 */
#define fio_ipc_local(...) fio_ipc_send(fio_ipc_new(.workers = 1, __VA_ARGS__))

/** Call arbitrary code in master process of every machine in cluster. */
#define fio_ipc_cluster(...)                                                   \
  fio_ipc_send(fio_ipc_new(.cluster = 1, __VA_ARGS__))

/** Call arbitrary code in master process of every machine in cluster. */
#define fio_ipc_broadcast(...)                                                 \
  fio_ipc_send(fio_ipc_new(.workers = 1, .cluster = 1, __VA_ARGS__))

/** IPC reply arguments */
typedef struct {
  fio_ipc_s *ipc;
  fio_buf_info_s *data;
  uint64_t timestamp; /* (optional) override timestamp, 0 = use current time */
  uint64_t id;        /* (optional) override id, 0 = use original request id */
  uint16_t flags;     /* (optional) override flags, 0 = use original request */
  uint8_t done;
  uint8_t flags_set; /* set to 1 if flags should be used (allows flags=0) */
} fio_ipc_reply_args_s;

/**
 * Send a response to the caller process (master → caller).
 *
 * Can be called multiple times for streaming responses.
 * Set `done = 1` on the last reply.
 */
SFUNC void fio_ipc_reply(fio_ipc_reply_args_s args);

#define fio_ipc_reply(r, ...)                                                  \
  fio_ipc_reply((fio_ipc_reply_args_s){.ipc = (r), __VA_ARGS__})

/* *****************************************************************************
IPC/RPC Message Core / Lifetime - used internally and for advance use-cases
***************************************************************************** */

/**
 * Authors a message without sending it.
 *
 * Used internally but available for "faking" IPC or when composing a unified
 * code path for local execution.
 */
SFUNC fio_ipc_s *fio_ipc_new(fio_ipc_args_s args);
#define fio_ipc_new(...) fio_ipc_new((fio_ipc_args_s){__VA_ARGS__})

/**
 * Duplicate a message (increment reference count).
 *
 * Use when storing messages for later processing.
 * Every dup() must be matched with a free().
 */
SFUNC fio_ipc_s *fio_ipc_dup(fio_ipc_s *msg);

/**
 * Free a message (decrement reference count).
 *
 * Message is destroyed when reference count reaches zero.
 */
SFUNC void fio_ipc_free(fio_ipc_s *msg);

/**
 * Detaches the IPC message from it's originating IO.
 *
 * Call if storing or performing non-IPC actions using the IPC message.
 * */
SFUNC void fio_ipc_detach(fio_ipc_s *msg);

/**
 * Encrypts the IPC message(!), sends it for execution and frees it.
 *
 * Message will be sent according to the flags set (see fio_ipc_new):
 * - Only to Master (fio_ipc_call);
 * - To Master and Workers (fio_ipc_local);
 * - To Master on Every Machine (fio_ipc_cluster);
 * - To Master and Workers on Every Machine (fio_ipc_broadcast);
 *
 * Note: Takes ownership of the message's memory.
 *
 * Note: overwrites after_send unless `ipc->from == FIO_IPC_EXCLUDE_SELF`
 *
 * Note: excludes ipc->from.
 *
 * Note: The message is encrypted and unusable once call returns - pass it the
 * last reference.
 */
SFUNC void fio_ipc_send(fio_ipc_s *ipc);

/**
 * Encrypts the IPC message(!) and sends it to target IO - frees the message.
 *
 * Note: Takes ownership of the message's memory.
 *
 * Note: The message is encrypted and unusable until the message was sent and
 * either freed or the `after_send` callback was called.
 */
SFUNC void fio_ipc_send_to(fio_io_s *to, fio_ipc_s *ipc);

/** Excludes current process from fio_ipc_local (to be set in ipc->from) */
#define FIO_IPC_EXCLUDE_SELF ((fio_io_s *)((char *)-1LL))

/* Sets callback to accept memory ownership after sending is complete. */
SFUNC void fio_ipc_after_send(fio_ipc_s *ipc,
                              void (*fn)(fio_ipc_s *, void *),
                              void *udata);
/* *****************************************************************************
IPC (Inter Process Communication) Encryption
***************************************************************************** */

/** Encrypt IPC message before sending them anywhere. */
SFUNC void fio_ipc_encrypt(fio_ipc_s *m);

/** Decrypt IPC message when received. */
FIO_IFUNC int fio_ipc_decrypt(fio_ipc_s *m);

/* *****************************************************************************
RPC (Multi-Machine Communication)
***************************************************************************** */

/**
 * Listens to cluster connections on the port listed, auto-connects to peers.
 *
 * This does NOT improve message exchange or pub/sub performance. This is
 * designed for downtime mitigation (rotating pods) / data tunneling and client
 * load balancing (without message load balancing).
 *
 * All server instances get all `cluster` messages (e.g., pub/sub cluster).
 *
 * Note: uses the environment's (shared) secret for rudimentary encryption
 * without forward secrecy. Rotate secrets when possible (requires restart).
 * Good for trusted data centers, Kubernetes pods, etc'.
 * */
SFUNC fio_io_listener_s *fio_ipc_cluster_listen(uint16_t port);

/** Manually connects to cluster peers. Usually unnecessary. */
SFUNC void fio_ipc_cluster_connect(const char *url);

/** Returns the last port number passed to fio_ipc_cluster_listen (or zero) */
SFUNC uint16_t fio_ipc_cluster_port(void);

/* *****************************************************************************
IPC Flags - Used for message routing, settable in routing_flags

These are mostly for internal use, but made available for advance usage with
`fio_ipc_send`
***************************************************************************** */

/** Set if message is in its encrypted state - do NOT edit manually */
#define FIO_IPC_FLAG_ENCRYPTED ((uint16_t)1UL << 0)
/** If set, calls `on_done` rather than `call` - requires FIO_IPC_FLAG_REPLY */
#define FIO_IPC_FLAG_DONE ((uint16_t)1UL << 1)
/** Set if message callbacks are mapped using op-codes  */
#define FIO_IPC_FLAG_OPCODE ((uint16_t)1UL << 2)
/** If set, delivered to worker processes as well as master */
#define FIO_IPC_FLAG_WORKERS ((uint16_t)1UL << 3)
/** If set, delivered to remote machines - requires FIO_IPC_FLAG_OPCODE */
#define FIO_IPC_FLAG_CLUSTER ((uint16_t)1UL << 4)
/** If set, calls `on_reply` rather than `call` */
#define FIO_IPC_FLAG_REPLY ((uint16_t)1UL << 5)
/** If flag is set == 1, otherwise zero */
#define FIO_IPC_FLAG_TEST(msg, flag) (((msg)->routing_flags & flag) == flag)
/** If flag is set == flag, otherwise zero */
#define FIO_IPC_FLAG_IF(bcond, flag) (((uint16_t)0UL - (bcond)) & flag)

/* *****************************************************************************
Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
/* we recursively include here */
#define FIO___RECURSIVE_INCLUDE 1

#ifndef FIO_IPC_BUFFER_LEN
#define FIO_IPC_BUFFER_LEN 8192
#endif

/* *****************************************************************************
IPC Message Helpers
***************************************************************************** */

typedef struct fio___ipc_metadata_s {
  void (*after_send)(fio_ipc_s *, void *udata);
  void *udata;
} fio___ipc_metadata_s;

/* Message reference counting using FIO_REF */
#define FIO_REF_NAME             fio___ipc
#define FIO_REF_TYPE             fio_ipc_s
#define FIO_REF_DESTROY(ipc)     fio_ipc_detach(&ipc)
#define FIO_REF_METADATA         fio___ipc_metadata_s
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_FLEX_TYPE        char
#include FIO_INCLUDE_FILE

FIO_IFUNC uint32_t fio___ipc_sizeof_header(void) {
  return (uint32_t)(FIO_PTR_FIELD_OFFSET(fio_ipc_s, data) -
                    FIO_PTR_FIELD_OFFSET(fio_ipc_s, len));
}
FIO_IFUNC uint32_t fio___ipc_sizeof_enc(uint32_t len) {
  return (uint32_t)(FIO_PTR_FIELD_OFFSET(fio_ipc_s, data) -
                    FIO_PTR_FIELD_OFFSET(fio_ipc_s, call)) +
         len;
}

FIO_IFUNC uint32_t fio___ipc_wire_length(uint32_t len) {
  /* includes the 16 byte MAC and data */
  return (uint32_t)(fio___ipc_sizeof_header() + len + 16);
}

/* *****************************************************************************
IPC / Remote - Op-Codes
***************************************************************************** */

#define FIO___IPC_REMOTE_OPCODE_HASH(a)                                        \
  fio_risky_num((uint64_t)(a)->opcode, (uint64_t)(uintptr_t)&fio_ipc_new)
#define FIO___IPC_REMOTE_OPCODE_CMP(a, b) ((a)->opcode == (b)->opcode)
#define FIO___IPC_REMOTE_OPCODE_VALID(a)  ((a)->opcode)
FIO_TYPEDEF_IMAP_ARRAY(fio___ipc_cluster_ops,
                       fio_ipc_opcode_s,
                       uint32_t,
                       FIO___IPC_REMOTE_OPCODE_HASH,
                       FIO___IPC_REMOTE_OPCODE_CMP,
                       FIO___IPC_REMOTE_OPCODE_VALID)
#undef FIO___IPC_REMOTE_OPCODE_HASH
#undef FIO___IPC_REMOTE_OPCODE_CMP
#undef FIO___IPC_REMOTE_OPCODE_VALID

/* *****************************************************************************
IPC / Remote - Filtering pre-existing messages
***************************************************************************** */

typedef struct {
  uint64_t timestamp; /* timestamp (unencrypted, used for nonce) */
  uint64_t id;        /* 8 random bytes (unencrypted, used for nonce) */
} fio___ipc_msg_id_s;
#define FIO___IPC_REMOTE_FILTER_HASH(a) fio_risky_num((a)->id, (a)->timestamp)
#define FIO___IPC_REMOTE_FILTER_CMP(a, b)                                      \
  ((a)->id == (b)->id && (a)->timestamp == (b)->timestamp)
#define FIO___IPC_REMOTE_FILTER_VALID(a) ((a)->id && (a)->timestamp)
FIO_TYPEDEF_IMAP_ARRAY(fio___ipc_cluster_filter,
                       fio___ipc_msg_id_s,
                       uint32_t,
                       FIO___IPC_REMOTE_FILTER_HASH,
                       FIO___IPC_REMOTE_FILTER_CMP,
                       FIO___IPC_REMOTE_FILTER_VALID)
#undef FIO___IPC_REMOTE_FILTER_HASH
#undef FIO___IPC_REMOTE_FILTER_CMP
#undef FIO___IPC_REMOTE_FILTER_VALID

/* Sort by timestamp */
#define FIO_SORT_NAME            fio___ipc_cluster_filter
#define FIO_SORT_TYPE            fio___ipc_msg_id_s
#define FIO_SORT_IS_BIGGER(a, b) ((a).timestamp > (b).timestamp)
#include FIO_INCLUDE_FILE

/* Returns 0 if message should be processed, -1 if already processed. */
FIO_IFUNC int fio___ipc_cluster_filter(fio___ipc_cluster_filter_s *filter,
                                       uint64_t timestamp,
                                       uint64_t id) {
  if (!filter)
    return -1;
  fio___ipc_msg_id_s mid = {.timestamp = timestamp, .id = id};
  const size_t limit = 65536, keep = 16384;
  if (fio___ipc_cluster_filter_get(filter, mid))
    return -1;
  if (filter->count >= limit)
    goto at_limit;
  fio___ipc_cluster_filter_set(filter, mid, 0);
  return 0;
at_limit:
  /* reduce to `keep` items: sort, save only tail, rehash */
  fio___ipc_cluster_filter_sort(filter->ary, filter->w);
  FIO_MEMMOVE(filter->ary, ((filter->ary + filter->w) - keep), keep);
  filter->ary[keep] = mid;
  filter->count = filter->w = (keep + 1);
  fio___ipc_cluster_filter_rehash(filter);
  return 0;
}

FIO_IFUNC int fio___ipc_cluster_filter_window(uint64_t timestamp) {
  const uint64_t tick = fio_io_last_tick();
  const uint64_t window = 30000;
  const uint64_t min = tick - window;
  const uint64_t max = tick + window;
  return (int)0 - (int)(timestamp > max || timestamp < min);
}
/* *****************************************************************************
IPC Global State
***************************************************************************** */

/* Global state */
static struct {
  size_t max_length;
  fio_io_s *listener;                   /* Needs to be reset sometimes? */
  fio_io_s *worker_connection;          /* Worker's connection to master */
  fio_io_protocol_s protocol_ipc;       /* IPC protocol */
  fio_io_protocol_s protocol_rpc;       /* RPC protocol */
  fio_io_protocol_s protocol_rpc_hello; /* RPC handshake protocol */
  fio_io_protocol_s protocol_udp;       /* Peer Discovery protocol */
  fio___ipc_cluster_ops_s opcodes;      /* Registered op-codes */
  fio___ipc_cluster_filter_s received;  /* messages recently received */
  fio___ipc_cluster_filter_s peers;     /* connected peers */
  fio_u128 uuid;                        /* instance ID peers */
  uint16_t cluster_port;                /* last port number in cluster_listen */
  char ipc_url[FIO_IPC_URL_MAX_LENGTH]; /* IPC socket path */
} FIO___IPC;

/* *****************************************************************************
IPC Settings
***************************************************************************** */

/** Returns the IPC url to listen to (for incoming connections). */
SFUNC const char *fio_ipc_url(void) { return FIO___IPC.ipc_url; }

/**
 * Sets the IPC url to listen to (for incoming connections).
 *
 * Can only be called on the master process and only before the IO reactor.
 */
SFUNC int fio_ipc_url_set(const char *url) {
  /* Build IPC URL: unix://fio_tmp_<rand>.sock  - URL inherited by worker */
  if (!fio_io_is_master() || fio_io_is_running())
    return -1;
  size_t len = url ? FIO_STRLEN(url) : 0;
  if (len > 3) {
    if (len >= FIO_IPC_URL_MAX_LENGTH)
      return -1;
    FIO_MEMCPY(FIO___IPC.ipc_url, url, len);
    FIO___IPC.ipc_url[len] = 0;
  } else {
    fio_str_info_s str =
        FIO_STR_INFO3(FIO___IPC.ipc_url, 0, FIO_IPC_URL_MAX_LENGTH);
    const char *options[] = {"TMPDIR", "TMP", "TEMP", NULL};
    const char *tmpdir = NULL;
    for (size_t i = 0; !tmpdir && options[i]; ++i) {
      tmpdir = getenv(options[i]);
    }
    size_t tmplen = tmpdir ? FIO_STRLEN(tmpdir) : 0;
    if (!tmpdir || tmplen > 128) {
      tmpdir = "/tmp/";
      tmplen = FIO_STRLEN(tmpdir);
    }

    fio_string_write2(&str,
                      NULL,
                      FIO_STRING_WRITE_STR1("unix://"),
                      FIO_STRING_WRITE_STR1(tmpdir),
                      FIO_STRING_WRITE_STR2("/", (tmpdir[tmplen - 1] != '/')),
                      FIO_STRING_WRITE_STR1("fio_tmp_"),
                      FIO_STRING_WRITE_HEX((fio_rand64() >> 32)),
                      FIO_STRING_WRITE_STR1(".sock"));
  }

  if (FIO___IPC.listener) {
    fio_queue_perform_all(fio_io_queue());
    fio_io_close(FIO___IPC.listener);
    FIO___IPC.listener = NULL;
  }

  return 0;
}

/* *****************************************************************************
IPC Lifetime Management
***************************************************************************** */

FIO_SFUNC void fio___ipc_free_task(void *ipc_, void *ignr_) {
  (void)ignr_;
  void (*fn)(fio_ipc_s *, void *) = NULL;
  void *udata = NULL;
  fio_ipc_s *ipc = (fio_ipc_s *)ipc_;
  if (!ipc)
    return;
  /* avoid encryption race conditions, only decrypt and execute after sending */
  if (fio___ipc_references(ipc) == 1 && fio___ipc_metadata(ipc)->after_send)
    goto after_send;
  fio___ipc_free(ipc);
  return;

after_send:
  /* all access SHOULD BE from the IO thread, so we are the last owner */
  fio_ipc_decrypt(ipc);
  fn = fio___ipc_metadata(ipc)->after_send;
  udata = fio___ipc_metadata(ipc)->udata;
  fio___ipc_metadata(ipc)->after_send = NULL;
  fio___ipc_metadata(ipc)->udata = NULL;
  fn(ipc, udata);
  return;
}

/** Free message (decrement ref count) */
SFUNC void fio_ipc_free(fio_ipc_s *msg) {
  if (!msg)
    return;
  fio_io_defer(fio___ipc_free_task, msg, NULL);
}

/** Free message (decrement ref count) */
FIO_SFUNC void fio___ipc_free_in_io_thread(fio_ipc_s *msg) {
  fio___ipc_free_task(msg, NULL);
}

/** Duplicate message (increment ref count) */
SFUNC fio_ipc_s *fio_ipc_dup(fio_ipc_s *msg) { return fio___ipc_dup(msg); }

/** Detaches the IPC message from it's originating IO. */
SFUNC void fio_ipc_detach(fio_ipc_s *ipc) {
  fio_io_s *io = ipc->from;
  if (io && io != FIO_IPC_EXCLUDE_SELF)
    fio_io_free(io);
  ipc->from = NULL;
}

/* *****************************************************************************
IPC Composition
***************************************************************************** */

FIO_SFUNC void fio___ipc_callback_noop(fio_ipc_s *m) { (void)m; }

FIO_IFUNC size_t fio___ipc_data_len(const fio_buf_info_s *data) {
  size_t all = 0;
  if (!data)
    return all;
  for (; data->len || data->buf; ++data)
    all += data->len;
  return all;
}
FIO_IFUNC void fio___ipc_data_write(fio_ipc_s *m, const fio_buf_info_s *data) {
  size_t pos = 0;
  if (!data)
    return;
  for (; data->len || data->buf; ++data) {
    if (!data->len)
      continue;
    FIO_MEMCPY(m->data + pos, data->buf, data->len);
    pos += data->len;
  }
}

FIO_IFUNC fio_ipc_s *fio___ipc_copy(const fio_ipc_s *ipc) {
  fio_ipc_s *cpy = fio___ipc_new(ipc->len + 16);
  FIO_MEMCPY(cpy, ipc, sizeof(*cpy) + ipc->len);
  if (cpy->from != FIO_IPC_EXCLUDE_SELF)
    cpy->from = fio_io_dup(cpy->from);
  return cpy;
}

FIO_IFUNC fio_ipc_s *fio___ipc_new_author(const fio_ipc_args_s *args,
                                          uint16_t routing_flags) {

  /* Create IPC message (allocate extra 16 bytes for MAC) */
  const size_t data_len = fio___ipc_data_len(args->data);
  if (data_len >= FIO___IPC.max_length) {
    FIO_LOG_WARNING("(%d) IPC message too long to author", fio_io_pid());
    return NULL;
  }
  fio_ipc_s *m = fio___ipc_new(data_len + 16);
  if (!m)
    return NULL;

  /* Fill message */
  if (args->cluster)
    routing_flags |= FIO_IPC_FLAG_CLUSTER | FIO_IPC_FLAG_OPCODE;
  if (args->workers)
    routing_flags |= FIO_IPC_FLAG_WORKERS;
  m->from = NULL;
  if (args->others)
    m->from = FIO_IPC_EXCLUDE_SELF;
  m->len = (uint32_t)(data_len);
  m->flags = args->flags;
  m->timestamp = (args->timestamp ? args->timestamp : fio_io_last_tick());
  m->id = (args->id ? args->id : fio_rand64());
  if (args->opcode) {
    routing_flags |= FIO_IPC_FLAG_OPCODE;
    m->opcode = args->opcode;
  } else
    m->call = args->call ? args->call : fio___ipc_callback_noop;
  m->on_reply = args->on_reply ? args->on_reply : fio___ipc_callback_noop;
  m->on_done = args->on_done ? args->on_done : fio___ipc_callback_noop;
  m->udata = args->udata;
  m->routing_flags = routing_flags;

  fio___ipc_data_write(m, args->data);
  return m;
}

void fio_ipc_new___(void);
/**
 * Authors a message without sending it.
 *
 * Used internally but available for "faking" IPC or when composing a unified
 * code path for local execution.
 */
SFUNC fio_ipc_s *fio_ipc_new FIO_NOOP(fio_ipc_args_s args) {
  return fio___ipc_new_author(&args, 0);
}

/* Set what to do after sending is complete */
SFUNC void fio_ipc_after_send(fio_ipc_s *ipc,
                              void (*fn)(fio_ipc_s *, void *),
                              void *udata) {
  fio___ipc_metadata(ipc)->after_send = fn;
  fio___ipc_metadata(ipc)->udata = udata;
}

/* *****************************************************************************
IPC Core - Encryption/Decryption
***************************************************************************** */

FIO_IFUNC fio_u256 fio___ipc_get_encryption_key(fio_ipc_s *m) {
  fio_u512 secret = fio_secret();
  fio_u256 key;
  /* folding - mix / encrypt secret using secret offset as key */
  /* this creates a safe ephemeral key */
  fio_chacha20(secret.u8,
               sizeof(secret),
               (secret.u8 + (m->id & 31) + 1),
               ((char *)&m->timestamp + 4),
               (m->id & 1023) + m->len);
  /* use an impossible to predict key offset to derive final key */
  fio_memcpy32(key.u8, secret.u8 + (secret.u8[2] & 31));
  // FIO_LOG_DDEBUG2("(%d) IPC key: %p...%p",
  //                 fio_io_pid(),
  //                 (void *)(uintptr_t)key.u64[0],
  //                 (void *)(uintptr_t)key.u64[3]);
  return key;
}

FIO_IFUNC fio_u128 fio___ipc_get_encryption_nonce(fio_ipc_s *m) {
  fio_u128 nonce = fio_u128_init64(m->timestamp, m->id);
  // FIO_LOG_DDEBUG2("(%d) IPC nonce: %p-%p",
  //                 fio_io_pid(),
  //                 (void *)(uintptr_t)nonce.u64[0],
  //                 (void *)(uintptr_t)nonce.u64[1]);
  return nonce;
}

/** Encrypt IPC message before sending */
SFUNC void fio_ipc_encrypt(fio_ipc_s *m) {
  if ((m->routing_flags & FIO_IPC_FLAG_ENCRYPTED)) /* runs on IO thread */
    return;
  fio_u256 key_buf = fio___ipc_get_encryption_key(m);
  fio_u128 nonce = fio___ipc_get_encryption_nonce(m);
  m->routing_flags |= FIO_IPC_FLAG_ENCRYPTED; /* runs on IO thread */

  FIO_LOG_DDEBUG2("(%d) IPC sizes \t enc: %zu \t wire %zu",
                  fio_io_pid(),
                  fio___ipc_sizeof_enc(m->len),
                  fio___ipc_wire_length(m->len));

  /* Encrypt in-place (everything after first 16 bytes) */
  fio_chacha20_poly1305_enc((m->data + m->len),           /* MAC output */
                            ((char *)&m->call),           /* Data to encrypt */
                            fio___ipc_sizeof_enc(m->len), /* Data length */
                            (char *)(&m->len),            /* AAD */
                            8,                            /* AAD length */
                            key_buf.u8,                   /* 32-byte key */
                            nonce.u8                      /* 12-byte nonce */
  );
  m->len = fio_ltole32(m->len);
}

/** Decrypt IPC message on receive */
FIO_IFUNC int fio_ipc_decrypt(fio_ipc_s *m) {
  int r = 0;
  if (!(m->routing_flags & FIO_IPC_FLAG_ENCRYPTED)) /* runs on IO thread */
    return r;
  m->len = fio_ltole32(m->len);
  fio_u256 key_buf = fio___ipc_get_encryption_key(m);
  fio_u128 nonce = fio___ipc_get_encryption_nonce(m);
  FIO_LOG_DDEBUG2("(%d) IPC sizes \t enc: %zu \t wire %zu",
                  fio_io_pid(),
                  fio___ipc_sizeof_enc(m->len),
                  fio___ipc_wire_length(m->len));
  /* Decrypt and verify MAC */
  r = fio_chacha20_poly1305_dec((m->data + m->len), /* MAC output */
                                ((char *)&m->call), /* Data to decrypt */
                                fio___ipc_sizeof_enc(m->len), /* Data length */
                                (char *)(&m->len),            /* AAD */
                                8,                            /* AAD length */
                                key_buf.u8,                   /* 32-byte key */
                                nonce.u8 /* 12-byte nonce */
  );
  m->routing_flags &= ~FIO_IPC_FLAG_ENCRYPTED; /* runs on IO thread */
  if (r) {
    FIO_LOG_SECURITY("IPC message decryption failed (possible attack)");
  }
  return r;
}

#undef FIO_IPC_FLAG_ENCRYPTED
/* *****************************************************************************
IPC / RPC - Op-Codes
***************************************************************************** */

void fio_ipc_opcode_register____(void); /* IDE Marker */
/* Registers an op-code for message routing. */
SFUNC int fio_ipc_opcode_register FIO_NOOP(fio_ipc_opcode_s o) {
  if (fio_io_is_running()) {
    FIO_LOG_ERROR("(%d) opcodes MUST be registered BEFORE IO reactor starts",
                  fio_io_pid());
    return -1;
  }
  if (!o.opcode) {
    FIO_LOG_ERROR("(%d) opcode MUST be non-zero", fio_io_pid());
    return -1;
  }
  if (!o.call) {
    fio___ipc_cluster_ops_remove(&FIO___IPC.opcodes, o);
    return 0;
  }
  if (!o.on_reply)
    o.on_reply = fio___ipc_callback_noop;
  if (!o.on_done)
    o.on_done = fio___ipc_callback_noop;
  fio___ipc_cluster_ops_set(&FIO___IPC.opcodes, o, 1);
  return 0;
}

/** Returns a pointer to a registered op-code, or NULL if missing. */
SFUNC const fio_ipc_opcode_s *fio_ipc_opcode(uint32_t opcode) {
  fio_ipc_opcode_s *r;
  fio_ipc_opcode_s find = {.opcode = opcode};
  r = fio___ipc_cluster_ops_get(&FIO___IPC.opcodes, find);
  return r;
}

/* *****************************************************************************
IPC / RPC - Execution (fast path / slow path)
***************************************************************************** */

SFUNC void fio___ipc_opcode_task(fio_ipc_s *ipc, void *ignr_) {
  const fio_ipc_opcode_s *op = fio_ipc_opcode(ipc->opcode);
  void (*const *call)(struct fio_ipc_s *);
  if (op) {
    call = &op->call;
    call += FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_REPLY);
    call += FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_DONE);
    ipc->udata = op->udata;
    call[0](ipc);
  } else {
    FIO_LOG_WARNING("(%d) ignoring IPC cluster with illegal op-code: %u",
                    fio_io_pid(),
                    (unsigned)ipc->opcode);
  }
  fio___ipc_free_in_io_thread(ipc);
  (void)ignr_;
}

SFUNC void fio___ipc_direct_task(fio_ipc_s *ipc, void *ignr_) {
  void (**call)(struct fio_ipc_s *) = &ipc->call;
  call += FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_REPLY);
  call += FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_DONE);
  call[0](ipc);
  fio___ipc_free_in_io_thread(ipc);
  (void)ignr_;
}

SFUNC void fio___ipc_execute_task(fio_ipc_s *ipc, void *ignr_) {
  void (*task[])(fio_ipc_s * ipc, void *ignr_) = {fio___ipc_direct_task,
                                                  fio___ipc_opcode_task};
  task[FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_OPCODE)](ipc, ignr_);
}

/* *****************************************************************************
IPC Core - Sending
***************************************************************************** */

FIO_SFUNC void fio___ipc_send_each_task(fio_io_s *to, void *ipc_) {
  fio_ipc_s *ipc = (fio_ipc_s *)ipc_;
  if (to == ipc->from)
    return;
  fio_io_write2(to,
                .buf = fio_ipc_dup(ipc),
                .offset = FIO_PTR_FIELD_OFFSET(fio_ipc_s, len),
                .len = fio___ipc_wire_length(ipc->len),
                .dealloc = (void (*)(void *))fio___ipc_free_in_io_thread);
}

FIO_SFUNC void fio___ipc_send_master_task(void *ipc_, void *ignr_) {
  fio_ipc_s *ipc = (fio_ipc_s *)ipc_;
  size_t count = 0;

  if (FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_WORKERS))
    count += fio_io_protocol_each(&FIO___IPC.protocol_ipc,
                                  fio___ipc_send_each_task,
                                  ipc);
  if (FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_CLUSTER))
    count += fio_io_protocol_each(&FIO___IPC.protocol_rpc,
                                  fio___ipc_send_each_task,
                                  ipc);
  FIO_LOG_DDEBUG2("(%d) [%s] sent IPC/RPC to %zu peers",
                  fio_io_pid(),
                  (fio_io_is_master() ? "Master" : "Worker"),
                  count);
  fio___ipc_free_task(ipc, NULL);
  (void)count; /* if unused by logger */
  (void)ignr_;
}

/* Encrypts the IPC message(!), sends it for execution and frees it. */
SFUNC void fio_ipc_send(fio_ipc_s *ipc) {
  if (!ipc)
    return;

  /* Master */
  if (fio_io_is_master()) {
    if (ipc->from != FIO_IPC_EXCLUDE_SELF) {
      /* on master we can't risk slow clients preventing execution */
      fio_io_defer((void (*)(void *, void *))fio___ipc_execute_task,
                   fio___ipc_copy(ipc),
                   NULL);
    }
    if (!FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_WORKERS) &&
        !FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_CLUSTER))
      goto master_only;
    fio_ipc_encrypt(ipc);
    /* to loop on protocol IO we must run in the IO thread */
    fio_io_defer(fio___ipc_send_master_task, ipc, NULL);
    return;
  }
  /* Worker */
  if (FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_OPCODE)) {
    ipc->udata = ipc->from;
  }
  if (FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_WORKERS) &&
      ipc->from != FIO_IPC_EXCLUDE_SELF) {
    fio_ipc_after_send(ipc, fio___ipc_execute_task, NULL);
  }
  fio_ipc_send_to(FIO___IPC.worker_connection, ipc);
  FIO_LOG_DDEBUG2("(%d) [Worker] sent IPC/RPC to 1 Master", fio_io_pid());
  return;
master_only:
  fio_ipc_free(ipc); /* we might not be in the IO thread */
}

/** Send IPC message to target IO - frees the message after it's sent */
SFUNC void fio_ipc_send_to(fio_io_s *to, fio_ipc_s *m) {
  if (!m)
    return;
  if (!to)
    goto free_send;

  FIO_LOG_DDEBUG2("(%d) IPC sending %zu (data length %zu) bytes (offset %zu)",
                  fio_io_pid(),
                  fio___ipc_wire_length(m->len),
                  (size_t)m->len,
                  (size_t)FIO_PTR_FIELD_OFFSET(fio_ipc_s, len));

  /* Encrypt message */
  fio_ipc_encrypt(m);
  FIO_LOG_DDEBUG2("(%d) IPC wire bytes: %02x %02x %02x %02x (len=%u)",
                  fio_io_pid(),
                  ((uint8_t *)&m->len)[0],
                  ((uint8_t *)&m->len)[1],
                  ((uint8_t *)&m->len)[2],
                  ((uint8_t *)&m->len)[3],
                  m->len);
  fio_io_write2(to,
                .buf = m,
                .offset = FIO_PTR_FIELD_OFFSET(fio_ipc_s, len),
                .len = fio___ipc_wire_length(m->len),
                .dealloc = (void (*)(void *))fio___ipc_free_in_io_thread);
  return;

free_send:
  fio_ipc_free(m);
}

/* *****************************************************************************
IPC Core - Call and Reply
***************************************************************************** */

void fio_ipc_reply___(void); /* IDE Marker */
/** Send reply to caller */
SFUNC void fio_ipc_reply FIO_NOOP(fio_ipc_reply_args_s args) {
  if (!args.ipc)
    return;
  fio_ipc_args_s ar = {
      .timestamp = args.timestamp,
      .id = (args.id ? args.id : args.ipc->id),
      .call = args.ipc->call,
      .on_reply = args.ipc->on_reply,
      .on_done = args.ipc->on_done,
      .udata = args.ipc->udata,
      .flags = (args.flags_set ? args.flags : args.ipc->flags),
      .data = args.data,
  };

  /* Author IPC message */
  fio_ipc_s *reply = fio___ipc_new_author(
      &ar,
      (args.ipc->routing_flags & (FIO_IPC_FLAG_CLUSTER | FIO_IPC_FLAG_OPCODE)) |
          FIO_IPC_FLAG_REPLY | FIO_IPC_FLAG_IF(args.done, FIO_IPC_FLAG_DONE));
  if (!reply)
    return;

  /* Send to caller */
  if (args.ipc->from && args.ipc->from != FIO_IPC_EXCLUDE_SELF)
    fio_ipc_send_to(args.ipc->from, reply);
  else
    fio___ipc_execute_task(reply, NULL);
}

/* *****************************************************************************
IPC Core - executing IPC calls
***************************************************************************** */

/** Wrapper that manages request vs response states */
FIO_SFUNC void fio___ipc_on_ipc_master(void *ipc_, void *io_) {
  fio_ipc_s *ipc = (fio_ipc_s *)ipc_;
  if (fio_ipc_decrypt(ipc))
    goto decrypt_error;
  if (FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_WORKERS) ||
      FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_CLUSTER)) {
    /* this message expects to be forwarded */
    if (ipc->udata == FIO_IPC_EXCLUDE_SELF &&
        FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_CLUSTER)) {
      /* cluster messages with FIO_IPC_EXCLUDE_SELF are excluded from master */
      fio_ipc_encrypt(ipc);
      fio___ipc_send_master_task(ipc, NULL);
      return;
    }
    fio_ipc_s *cpy = fio___ipc_copy(ipc);
    fio_ipc_encrypt(cpy);
    fio___ipc_send_master_task(cpy, NULL);
  }
  fio___ipc_execute_task(ipc, io_);
  return;
decrypt_error:
  fio_io_close(io_);
  fio___ipc_free(ipc);
}

/** Wrapper that manages request vs response states */
FIO_SFUNC void fio___ipc_on_ipc_worker(void *ipc_, void *io_) {
  fio_ipc_s *ipc = (fio_ipc_s *)ipc_;
  if (fio_ipc_decrypt(ipc))
    goto decrypt_error;
  fio___ipc_execute_task(ipc, io_);
  return;
decrypt_error:
  fio_io_close(io_);
  fio___ipc_free(ipc);
}

/** Wrapper that manages request vs response states */
FIO_SFUNC void fio___ipc_on_rpc_master(void *ipc_, void *io_) {
  fio_ipc_s *ipc = (fio_ipc_s *)ipc_;
  ipc->from = io_;
  fio_u128 *uuid = fio_io_buffer(io_);
  fio_u128 peer_uuid;

  if (fio_ipc_decrypt(ipc) || !FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_CLUSTER) ||
      !FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_OPCODE))
    goto peer_error;

  if (fio___ipc_cluster_filter_window(fio_ltole64(ipc->timestamp)) ||
      fio___ipc_cluster_filter(&FIO___IPC.received,
                               fio_ltole64(ipc->timestamp),
                               ipc->id))
    goto filtered;

  if (ipc->opcode == (uint32_t)0xFFFFFFFF)
    goto uuid_msg;

  if (FIO_IPC_FLAG_TEST(ipc, FIO_IPC_FLAG_WORKERS)) {
    fio_ipc_send(ipc);
    return;
  }
  fio___ipc_execute_task(ipc, io_);
  return;

uuid_msg:
  if (uuid->u64[0] || uuid->u64[1])
    goto peer_error; /* Already received UUID from this connection */
  fio_memcpy16(&peer_uuid, &ipc->on_reply);
  /* test if we already connected to peer previously */
  if (fio___ipc_cluster_filter(&FIO___IPC.peers,
                               peer_uuid.u64[0],
                               peer_uuid.u64[1]))
    goto peer_error;
  /* set peer uuid */
  *uuid = peer_uuid;
  fio___ipc_free(ipc);
  return;
peer_error:
  fio_io_close(io_);
filtered:
  fio___ipc_free(ipc);
  return;
}

/* *****************************************************************************
IPC Core - IPC Socket Management
***************************************************************************** */

/** Parser state for reading IPC messages from socket */
typedef struct {
  fio_ipc_s *msg;        /* Current message being parsed */
  uint32_t expected_len; /* Expected total length */
  uint32_t msg_received; /* Bytes received for current message */
  size_t buf_len;        /* Valid bytes in buffer */
  char buffer[];         /* FIO_IPC_BUFFER_LEN for small messages */
} fio___ipc_parser_s;

FIO_IFUNC fio_u128 *fio___ipc_cluster_uuid(fio_io_s *io) {
  return (fio_u128 *)fio_io_buffer(io);
}
FIO_IFUNC fio___ipc_parser_s *fio___ipc_cluster_parser(fio_io_s *io) {
  return (fio___ipc_parser_s *)(fio___ipc_cluster_uuid(io) + 1);
}

/** Get parser from IO buffer */
FIO_IFUNC fio___ipc_parser_s *fio___ipc_parser(fio_io_s *io) {
  return (fio___ipc_parser_s *)fio_io_buffer(io);
}

/** Initialize parser */
FIO_IFUNC void fio___ipc_parser_init(fio___ipc_parser_s *p) {
  *p = (fio___ipc_parser_s){0};
}

/** Destroy parser */
FIO_IFUNC void fio___ipc_parser_destroy(fio___ipc_parser_s *p) {
  if (p->msg) {
    p->msg->from = NULL; /* avoid double free, fio_io_dup wasn't called yet */
    fio___ipc_free(p->msg);
  }
  *p = (fio___ipc_parser_s){0};
}

FIO_SFUNC void fio___ipc_on_attach(fio_io_s *io) {
  fio___ipc_parser_s *p = fio___ipc_parser(io);
  fio___ipc_parser_init(p);
}

/** Called when data is received on IPC socket */
FIO_IFUNC void fio___ipc_on_data_internal(fio_io_s *io,
                                          fio___ipc_parser_s *p,
                                          void (*fn)(void *ipc, void *io)) {
  fio_ipc_s *msg;
  if (!p)
    return;
  bool had_messages = 0; /* if messages were sent, we stop reading */
  for (;;) {
    if ((msg = p->msg)) { /* read directly to message object */
      p->msg_received += fio_io_read(io,
                                     (char *)&msg->len + p->msg_received,
                                     p->expected_len - p->msg_received);
      if (p->expected_len != p->msg_received)
        return;
      fio_io_defer(fn, msg, fio_io_dup(io));
      fio___ipc_parser_init(p);
      return; /* don't read more messages for now */
    }
    /* read into IO buffer */
    p->buf_len += fio_io_read(io,
                              p->buffer + p->buf_len,
                              FIO_IPC_BUFFER_LEN - p->buf_len);
    size_t consumed = 0;
    for (;;) { /* consume from IO buffer */
      if (p->buf_len < 4 + consumed)
        break;
      p->expected_len =
          fio___ipc_wire_length(fio_buf2u32_le(p->buffer + consumed));
      FIO_LOG_DEBUG2("(%d) incoming IPC message: %u/%zu (len=%u, bytes: %02x "
                     "%02x %02x %02x)",
                     fio_io_pid(),
                     p->expected_len,
                     p->buf_len,
                     fio_buf2u32u(p->buffer + consumed),
                     (uint8_t)p->buffer[consumed],
                     (uint8_t)p->buffer[consumed + 1],
                     (uint8_t)p->buffer[consumed + 2],
                     (uint8_t)p->buffer[consumed + 3]);
      if (p->expected_len > FIO___IPC.max_length) { /* oversized? */
        FIO_LOG_SECURITY("(%d) Invalid IPC message length: %u (buf: %zu)",
                         fio_io_pid(),
                         p->expected_len,
                         p->buf_len);
        fio_io_close(io);
        return;
      }

      if (p->buf_len >= consumed + p->expected_len) {
        FIO_LOG_DEBUG2("(%d) routing small IPC message:%u/%u",
                       fio_io_pid(),
                       p->expected_len,
                       p->buf_len);
        msg = fio___ipc_new(p->expected_len - fio___ipc_sizeof_header());
        msg->from = io;
        FIO_MEMCPY(&msg->len, p->buffer + consumed, p->expected_len);
        consumed += p->expected_len;
        fio_io_defer(fn, msg, fio_io_dup(io));
        had_messages |= 1;
        continue;
      }
      p->msg = msg = fio___ipc_new(p->expected_len - fio___ipc_sizeof_header());
      p->msg_received = p->buf_len - consumed;
      msg->from = io;
      FIO_MEMCPY(&msg->len, p->buffer + consumed, p->msg_received);
      consumed = p->buf_len = 0;
      break;
    }
    if (consumed == p->buf_len)
      consumed = p->buf_len = 0;
    if (consumed) { /* partial message length 4 byte block */
      FIO_MEMMOVE(p->buffer, p->buffer + consumed, p->buf_len - consumed);
      p->buf_len -= consumed;
    }
    if (had_messages || !p->msg)
      return;
  }
}

/** Called when data is received on IPC socket */
FIO_SFUNC void fio___ipc_on_data_master(fio_io_s *io) {
  fio___ipc_on_data_internal(io, fio___ipc_parser(io), fio___ipc_on_ipc_master);
}
/** Called when data is received on IPC socket */
FIO_SFUNC void fio___ipc_on_data_worker(fio_io_s *io) {
  fio___ipc_on_data_internal(io, fio___ipc_parser(io), fio___ipc_on_ipc_worker);
}

/** Called when data is received on a cluster connection */
FIO_SFUNC void fio___ipc_on_data_cluster(fio_io_s *io) {
  fio___ipc_on_data_internal(io,
                             fio___ipc_cluster_parser(io),
                             fio___ipc_on_rpc_master);
}

/** Called when data is received on a cluster connection */
FIO_SFUNC void fio___ipc_on_shutdown_worker(fio_io_s *io) { (void)io; }

FIO_SFUNC void fio___ipc_on_shutdown_worker_callback(fio_ipc_s *ipc) {
  fio_io_stop();
  (void)ipc;
}
FIO_SFUNC void fio___ipc_on_shutdown_master(fio_io_s *io) {
  fio_ipc_local(.others = 1, .call = fio___ipc_on_shutdown_worker_callback);
  (void)io;
}

/** Called when IPC socket is closed */
FIO_SFUNC void fio___ipc_on_close(void *buffer, void *udata) {
  fio___ipc_parser_s *p = (fio___ipc_parser_s *)buffer;
  fio___ipc_parser_destroy(p);
  if (!fio_io_is_master()) {
    FIO_LOG_DEBUG2("(%d) lost ICP connection", fio_io_pid());
    fio_io_stop();
  }
  (void)udata;
}

/* *****************************************************************************
IPC - Listening / Connecting
***************************************************************************** */

FIO_SFUNC void fio___ipc_listen_on_stop(void *iobuf, void *udata) {
  (void)iobuf;
  (void)udata;
  FIO___IPC.listener = NULL;
}

FIO_SFUNC void fio___ipc_connect_on_failed(fio_io_protocol_s *protocol,
                                           void *udata) {
  (void)protocol;
  (void)udata;
  FIO_LOG_FATAL("Couldn't connect to IPC @ %s", FIO___IPC.ipc_url);
  fio_io_stop();
}

static void fio___ipc_listen_on_data(fio_io_s *io) {
  int fd;
  fio_io_protocol_s *protocol = (fio_io_protocol_s *)fio_io_udata(io);
  while (FIO_SOCK_FD_ISVALID(fd = fio_sock_accept(fio_io_fd(io), NULL, NULL))) {
    FIO_LOG_DDEBUG2("(%d) accepted new IPC connection with fd %d",
                    fio_io_pid(),
                    fd);
    fio_io_attach_fd(fd, protocol, NULL, NULL);
  }
}

FIO_SFUNC void fio___ipc_listen(void *ignr_) {
  if (FIO___IPC.listener)
    return;
  static fio_io_protocol_s listening_protocol = {
      .on_data = fio___ipc_listen_on_data,
      .on_timeout = fio_io_touch,
      .on_close = fio___ipc_listen_on_stop,
  };
  fio_url_s url =
      fio_url_parse(FIO___IPC.ipc_url, FIO_STRLEN(FIO___IPC.ipc_url));
  fio_url_tls_info_s tls_info = fio_url_is_tls(url);
  if (tls_info.tls) {
    FIO_LOG_SECURITY(
        "IPC URL error - ignoring TLS (using internal encryption): %s",
        FIO___IPC.ipc_url);
  }

  int fd = fio_sock_open2(FIO___IPC.ipc_url,
                          FIO_SOCK_SERVER | FIO_SOCK_TCP | FIO_SOCK_NONBLOCK);

  FIO_ASSERT(fd != -1,
             "(%d) failed to start IPC listening @ %s",
             fio_io_pid(),
             FIO___IPC.ipc_url);
  FIO_LOG_DDEBUG2("(%d) starting IPC listening socket at: %s",
                  fio_io_pid(),
                  FIO___IPC.ipc_url);
  fio_io_attach_fd(fd, &listening_protocol, &FIO___IPC.protocol_ipc, NULL);
  (void)ignr_;
}

/* *****************************************************************************
IPC - Remote - Peer 2 Peer Negotiations (UDP broadcast & connect)
***************************************************************************** */
/** Get parser from IO buffer */

/* *****************************************************************************
IPC - Remote - UDP Discovery Message Format
*****************************************************************************
UDP discovery message (48 bytes):
  - uuid[16]:      Instance UUID (16 bytes)
  - timestamp[8]:  Current timestamp in milliseconds (8 bytes)
  - random[8]:     Random nonce for replay protection (8 bytes)
  - mac[16]:       Poly1305 MAC over uuid+timestamp+random (16 bytes)
***************************************************************************** */

typedef struct {
  uint8_t uuid[16];   /* Instance UUID */
  uint64_t timestamp; /* Timestamp in milliseconds */
  uint64_t random;    /* Random nonce */
  uint8_t mac[16];    /* Poly1305 MAC */
} fio___ipc_udp_discovery_s;

/** Compose a UDP discovery message */
FIO_SFUNC void fio___ipc_udp_discovery_compose(fio___ipc_udp_discovery_s *msg) {
  fio_u256 key_buf;
  uint64_t mac[2];
  /* Fill message fields */
  fio_memcpy16(msg->uuid, FIO___IPC.uuid.u8);
  msg->timestamp = fio_lton64(fio_io_last_tick());
  msg->random = fio_rand64();
  /* Derive key from secret + random */
  fio_u512 secret = fio_secret();
  fio_chacha20(secret.u8,
               sizeof(secret),
               (secret.u8 + (msg->random & 31) + 1),
               ((char *)&msg->timestamp + 4),
               (msg->random & 1023));
  fio_memcpy32(key_buf.u8, secret.u8 + (secret.u8[2] & 31));
  /* Compute MAC over uuid + timestamp + random (32 bytes) */
  fio_poly1305_auth(mac, msg, 32, NULL, 0, key_buf.u8);
  fio_memcpy16(msg->mac, mac);
}

/** Validate a UDP discovery message. Returns 0 on success, -1 on failure. */
FIO_SFUNC int fio___ipc_udp_discovery_validate(fio___ipc_udp_discovery_s *msg) {
  fio_u256 key_buf;
  uint64_t mac[2];
  uint64_t timestamp = fio_ltole64(msg->timestamp);
  /* Test for self-generated messages */
  if (FIO_MEMCMP(msg->uuid, FIO___IPC.uuid.u8, 16) == 0)
    return -1;
  /* Test time window (30 second window) */
  if (fio___ipc_cluster_filter_window(timestamp))
    return -1;
  /* Derive key from secret + random */
  fio_u512 secret = fio_secret();
  fio_chacha20(secret.u8,
               sizeof(secret),
               (secret.u8 + (msg->random & 31) + 1),
               ((char *)&msg->timestamp + 4),
               (msg->random & 1023));
  fio_memcpy32(key_buf.u8, secret.u8 + (secret.u8[2] & 31));
  /* Verify MAC */
  fio_poly1305_auth(mac, msg, 32, NULL, 0, key_buf.u8);
  if (FIO_MEMCMP(mac, msg->mac, 16) != 0) {
    FIO_LOG_SECURITY("(%d) UDP discovery MAC failure - possible attack?",
                     fio_io_pid());
    return -1;
  }
  return 0;
}

/* *****************************************************************************
IPC - Remote - UDP Discovery Protocol
***************************************************************************** */

/** Send UDP broadcast discovery message */
FIO_SFUNC void fio___ipc_udp_broadcast_hello(fio_io_s *io) {
  if (!fio_io_is_running() || !io)
    return;
  fio___ipc_udp_discovery_s msg;
  fio___ipc_udp_discovery_compose(&msg);
  uint16_t port = (uint16_t)(uintptr_t)fio_io_udata(io);
  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = fio_lton16(port),
      .sin_addr.s_addr = INADDR_BROADCAST,
  };
  FIO_LOG_DDEBUG2("(%d) IPC/RPC sending UDP discovery broadcast on port %u",
                  fio_io_pid(),
                  (unsigned)port);
  sendto(fio_io_fd(io),
         (const char *)&msg,
         sizeof(msg),
         0,
         (struct sockaddr *)&addr,
         sizeof(addr));
}

/** Periodic task to send UDP discovery broadcasts */
FIO_SFUNC int fio___ipc_udp_broadcast_task(void *io_, void *ignr_) {
  (void)ignr_;
  fio_io_s *io = (fio_io_s *)io_;
  if (fio_io_is_open(io)) {
    fio___ipc_udp_broadcast_hello(io);
    return 0;
  }
  return -1;
}
FIO_SFUNC void fio___ipc_udp_broadcast_task_done(void *io_, void *ignr_) {
  fio_io_s *io = (fio_io_s *)io_;
  fio_io_free(io);
  (void)ignr_;
}
/** Called when UDP socket is attached */
FIO_SFUNC void fio___ipc_udp_on_attach(fio_io_s *io) {
  /* Send initial broadcast */
  fio___ipc_udp_broadcast_hello(io);
  /* Schedule periodic broadcasts (every 1-2 seconds with jitter) */
  fio_io_run_every(.fn = fio___ipc_udp_broadcast_task,
                   .udata1 = fio_io_dup(io),
                   .every = (uint32_t)(2048 | (1023 & FIO___IPC.uuid.u64[0])),
                   .on_finish = fio___ipc_udp_broadcast_task_done,
                   .repetitions = 0); /* Repeat indefinitely */
}

/** Called when UDP socket is closed */
FIO_SFUNC void fio___ipc_udp_on_close(void *ignr1_, void *ignr2_) {
  (void)ignr1_, (void)ignr2_;
}

/** Called when UDP data is received */
FIO_SFUNC void fio___ipc_udp_on_data(fio_io_s *io) {
  fio___ipc_udp_discovery_s buf;
  struct sockaddr_storage from_storage;
  struct sockaddr *from = (struct sockaddr *)&from_storage;
  socklen_t from_len = sizeof(from_storage);
  ssize_t len;
  uint16_t port = (uint16_t)(uintptr_t)fio_io_udata(io);

  while ((len = recvfrom(fio_io_fd(io),
                         (char *)&buf,
                         sizeof(buf),
                         0,
                         from,
                         &from_len)) > 0) {
    /* Validate message size */
    if (len != sizeof(fio___ipc_udp_discovery_s)) {
      FIO_LOG_SECURITY("(%d) IPC/RPC UDP received invalid packet (%zd bytes)",
                       fio_io_pid(),
                       len);
      continue;
    }
    /* Validate message content */
    if (fio___ipc_udp_discovery_validate(&buf)) {
      FIO_LOG_SECURITY("(%d) IPC/RPC UDP discovery message validation failed",
                       fio_io_pid());
      continue;
    }
    /* Check if already connected to this peer */
    fio_u128 peer_uuid;
    fio_memcpy16(peer_uuid.u8, buf.uuid);
    if (fio___ipc_cluster_filter_get(
            &FIO___IPC.peers,
            (fio___ipc_msg_id_s){peer_uuid.u64[0], peer_uuid.u64[1]})) {
      FIO_LOG_DDEBUG2("(%d) IPC/RPC UDP skipping already connected peer",
                      fio_io_pid());
      continue;
    }
    /* Extract peer address and connect */
    char addr_buf[128];
    char port_buf[16];
    if (getnameinfo(from,
                    from_len,
                    addr_buf,
                    sizeof(addr_buf),
                    port_buf,
                    sizeof(port_buf),
                    (NI_NUMERICHOST | NI_NUMERICSERV))) {
      FIO_LOG_ERROR("(%d) IPC/RPC UDP couldn't resolve peer address",
                    fio_io_pid());
      continue;
    }
    /* Build TCP URL using the same port as UDP */
    FIO_STR_INFO_TMP_VAR(url, 256);
    fio_string_write2(&url,
                      NULL,
                      FIO_STRING_WRITE_STR1("tcp://"),
                      FIO_STRING_WRITE_STR1(addr_buf),
                      FIO_STRING_WRITE_STR1(":"),
                      FIO_STRING_WRITE_UNUM(port));
    FIO_LOG_INFO("(%d) IPC/RPC UDP discovered peer at %s",
                 fio_io_pid(),
                 url.buf);
    /* Connect to peer via TCP */
    fio_ipc_cluster_connect(url.buf);
    /* Send another broadcast to help peer discover us */
    fio___ipc_udp_broadcast_hello(io);
  }
}

/** Start UDP discovery on the specified port */
FIO_SFUNC void fio___ipc_cluster_udp_start(uint16_t port) {
  /* Initialize UDP protocol */
  FIO___IPC.protocol_udp = (fio_io_protocol_s){
      .on_attach = fio___ipc_udp_on_attach,
      .on_data = fio___ipc_udp_on_data,
      .on_close = fio___ipc_udp_on_close,
      .on_timeout = fio_io_touch, /* UDP doesn't need timeout handling */
  };
  /* Open UDP socket */
  FIO_STR_INFO_TMP_VAR(port_str, 16);
  fio_string_write_u(&port_str, NULL, (uint64_t)port);
  int fd_udp =
      fio_sock_open(NULL,
                    port_str.buf,
                    FIO_SOCK_UDP | FIO_SOCK_NONBLOCK | FIO_SOCK_SERVER);
  if (fd_udp == -1) {
    FIO_LOG_ERROR("(%d) IPC/RPC couldn't open UDP discovery socket on port %u",
                  fio_io_pid(),
                  (unsigned)port);
    return;
  }
  /* Enable broadcast */
  {
#if FIO_OS_WIN
    char enabled = 1;
#else
    int enabled = 1;
#endif
    setsockopt(fd_udp, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));
    enabled = 1;
    setsockopt(fd_udp, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
#ifdef SO_REUSEPORT
    enabled = 1;
    setsockopt(fd_udp, SOL_SOCKET, SO_REUSEPORT, &enabled, sizeof(enabled));
#endif
  }
  /* Attach to IO reactor with port as udata */
  fio_io_s *io = fio_io_attach_fd(fd_udp,
                                  &FIO___IPC.protocol_udp,
                                  (void *)(uintptr_t)port,
                                  NULL);
  FIO_LOG_INFO("(%d) IPC/RPC UDP discovery broadcasting on port %u",
               fio_io_pid(),
               (unsigned)port);
  fio___ipc_udp_broadcast_hello(io);
}

/* *****************************************************************************
IPC - Remote - New Connection Attachments
***************************************************************************** */
SFUNC void fio___ipc_on_attach_cluster(fio_io_s *io) {
  /* set peer UUID to zero and initialize parser */
  *fio___ipc_cluster_uuid(io) = fio_u128_init64(0, 0);
  fio___ipc_parser_init(fio___ipc_cluster_parser(io));
  /* inform peer about our UUID */
  fio_ipc_s *ipc = fio_ipc_new(0);
  FIO_ASSERT_ALLOC(ipc);
  ipc->opcode = (uint32_t)0xFFFFFFFF;
  ipc->udata = FIO_IPC_EXCLUDE_SELF;
  ipc->routing_flags |= (FIO_IPC_FLAG_CLUSTER | FIO_IPC_FLAG_OPCODE);
  fio_memcpy16(&ipc->on_reply, FIO___IPC.uuid.u8);
  fio_ipc_send_to(io, ipc);
}

/** Called when IPC socket is closed */
FIO_SFUNC void fio___ipc_on_close_cluster(void *buffer, void *udata) {
  fio___ipc_parser_s *p =
      (fio___ipc_parser_s *)((char *)buffer + sizeof(fio_u128));
  fio___ipc_parser_destroy(p);
  fio___ipc_cluster_filter_remove(
      &FIO___IPC.peers,
      (fio___ipc_msg_id_s){fio_buf2u64u(buffer),
                           fio_buf2u64u(((char *)buffer + sizeof(uint64_t)))});
  (void)udata;
}

/* *****************************************************************************
IPC - Remote - Listen
***************************************************************************** */

/** Listens to cluster connections on the port listed, auto-connects to peers.
 */
SFUNC fio_io_listener_s *fio_ipc_cluster_listen(uint16_t port) {
  if (!fio_io_is_master()) {
    FIO_LOG_WARNING("fio_ipc_cluster_listen should only be called on master");
    return NULL;
  }
  if (fio_secret_is_random()) {
    FIO_LOG_WARNING("(%d) RPC disabled - using random secret."
                    "\n\tSet a shared secret using SECRET environment "
                    "variable for peer discovery.",
                    fio_io_pid());
    return NULL;
  }

  FIO_STR_INFO_TMP_VAR(url, 128);
  fio_string_write2(&url,
                    NULL,
                    FIO_STRING_WRITE_STR1("tcp://0.0.0.0:"),
                    FIO_STRING_WRITE_UNUM(port));

  fio_io_listener_s *listener =
      fio_io_listen(.url = url.buf,
                    .protocol = &FIO___IPC.protocol_rpc,
                    .on_root = 1,
                    .hide_from_log = 1);

  if (!listener) {
    FIO_LOG_ERROR("Failed to listen for cluster IPC on port %u",
                  (unsigned)port);
    return NULL;
  }

  FIO___IPC.cluster_port = port;

  FIO_LOG_INFO("Remote IPC listening on port %u", (unsigned)port);

  /* Start UDP discovery on the same port */
  fio___ipc_cluster_udp_start(port);

  return listener;
}

/* Returns the last port number passed to fio_ipc_cluster_listen (or zero) */
SFUNC uint16_t fio_ipc_cluster_port(void) { return FIO___IPC.cluster_port; }

/* *****************************************************************************
IPC - Remote - Connect
***************************************************************************** */

/** Manually connects to cluster peers. Usually unnecessary. */
SFUNC void fio_ipc_cluster_connect(const char *url) {
  if (!url || !*url) {
    FIO_LOG_WARNING("(%d) fio_ipc_cluster_connect: NULL or empty URL",
                    fio_io_pid());
    return;
  }

  if (!fio_io_is_master()) {
    // Workers should use fio_ipc_call to ask master to connect
    FIO_LOG_ERROR("(%d) fio_ipc_cluster_connect should be called on master",
                  fio_io_pid());
    return;
  }

  fio_io_s *io = fio_io_connect(url, .protocol = &FIO___IPC.protocol_rpc);

  if (!io) {
    FIO_LOG_ERROR("(%d) Failed to connect to cluster IPC peer: %s",
                  fio_io_pid(),
                  url);
    return;
  }

  FIO_LOG_INFO("(%d) Connecting to cluster IPC peer: %s", fio_io_pid(), url);
}

/* *****************************************************************************
IPC - Initialization & Destruction
***************************************************************************** */

/** Cleanup IPC system */
FIO_SFUNC void fio___ipc_destroy(void *ignr_) {
  fio___ipc_cluster_filter_destroy(&FIO___IPC.received);
  fio___ipc_cluster_filter_destroy(&FIO___IPC.peers);
  fio___ipc_cluster_ops_destroy(&FIO___IPC.opcodes);
  (void)ignr_;
}

/* Worker initialization - called after fork */
FIO_SFUNC void fio___ipc_on_fork(void *ignr_) {
  (void)ignr_;
  /* Cleanup master's resources - except opcodes (clients may use) */
  fio___ipc_cluster_filter_destroy(&FIO___IPC.received);
  fio___ipc_cluster_filter_destroy(&FIO___IPC.peers);

  FIO___IPC.protocol_ipc.on_data = fio___ipc_on_data_worker;
  FIO___IPC.protocol_ipc.on_shutdown = fio___ipc_on_shutdown_worker;

  /* Connect to master's IPC socket */
  FIO___IPC.worker_connection =
      fio_io_connect(FIO___IPC.ipc_url,
                     .protocol = &FIO___IPC.protocol_ipc,
                     .on_failed = fio___ipc_connect_on_failed);

  if (!FIO___IPC.worker_connection) {
    FIO_LOG_ERROR("Failed to connect to master IPC socket: %s",
                  FIO___IPC.ipc_url);
  }
}
void fio___ipc_init____(void); /* IDE Marker */
/* Master initialization - runs automatically at startup */
FIO_CONSTRUCTOR(fio___ipc_init) {
  FIO___IPC.max_length = FIO_IPC_MAX_LENGTH;
  FIO___IPC.protocol_ipc = (fio_io_protocol_s){
      .on_attach = fio___ipc_on_attach,
      .on_data = fio___ipc_on_data_master,
      .on_shutdown = fio___ipc_on_shutdown_master,
      .on_close = fio___ipc_on_close,
      .on_timeout = fio_io_touch, /* TODO Fix: add pings? */
      .buffer_size = sizeof(fio___ipc_parser_s) + FIO_IPC_BUFFER_LEN,
  };
  FIO___IPC.protocol_rpc = (fio_io_protocol_s){
      .on_attach = fio___ipc_on_attach_cluster,
      .on_data = fio___ipc_on_data_cluster,
      .on_close = fio___ipc_on_close_cluster,
      .on_timeout = fio_io_touch, /* TODO Fix: add pings? */
      .buffer_size =
          sizeof(fio_u128) + sizeof(fio___ipc_parser_s) + FIO_IPC_BUFFER_LEN,
  };
  fio_io_protocol_set(NULL, &FIO___IPC.protocol_ipc); /* initialize - safety */
  fio_io_protocol_set(NULL, &FIO___IPC.protocol_rpc); /* initialize - safety */
  FIO___IPC.uuid = fio_u128_init64(fio_rand64(), fio_rand64());
  fio_ipc_url_set(NULL);
  fio_state_callback_add(FIO_CALL_BEFORE_FORK, fio___ipc_listen, NULL);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___ipc_on_fork, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___ipc_destroy, NULL);
}

/* *****************************************************************************
IPC - Extern Cleanup
***************************************************************************** */
#undef FIO_IPC_BUFFER_LEN
#undef FIO___RECURSIVE_INCLUDE /* Done with the recursive inclusion */
#endif                         /* FIO_EXTERN_COMPLETE */

/* *****************************************************************************
IPC - Cleanup
***************************************************************************** */

#endif /* FIO_IPC */
