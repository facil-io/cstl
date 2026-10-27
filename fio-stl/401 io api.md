# IO Reactor API (401 io api.h)

```c
#define FIO_IO
#include FIO_INCLUDE_FILE
```

`FIO_IO` adds the facil.io evented IO reactor, connection handles, protocol
callbacks, listener/client helpers, TLS transport hooks, per-connection
lifetime storage, timers, and optional async worker queues.

See [./400 io-overview.md](./400 io-overview.md) for the full IO / IPC /
PubSub / HTTP stack. TLS backends are covered by
[./405 openssl.md](./405 openssl.md) and [./405 tls13.md](./405 tls13.md).
Socket helpers are documented in [./004 sock.md](./004 sock.md), timers and
queues in [./102 queue.md](./102 queue.md), and lifecycle callbacks in
[./004 state callbacks.md](./004 state callbacks.md).

---

## Configuration Macros

Define before including the header to override defaults.

| Macro | Default | Meaning |
|-------|---------|---------|
| `FIO_IO_BUFFER_PER_WRITE` | `65536U` | Stack buffer size used during write events. |
| `FIO_IO_THROTTLE_LIMIT` | `2097152U` | `on_data` is throttled while outgoing backlog is large. |
| `FIO_IO_TIMEOUT_MAX` | `300000` | Maximum and default connection timeout, in milliseconds. |
| `FIO_IO_SHUTDOWN_TIMEOUT` | `15000` | Hard timeout for the reactor shutdown loop, in milliseconds. |
| `FIO_IO_COUNT_STORAGE` | `1` in `DEBUG`, else `0` | Enables IO byte-count storage when compiled in. |

---

## Core Types

| Type | Role |
|------|------|
| `fio_io_s` | Opaque connection handle. Use it instead of raw file descriptors inside protocol callbacks. |
| `fio_io_protocol_s` | Shared protocol callback table for a family of connections. Usually static / global and zero-initialized. |
| `fio_io_functions_s` | Optional transport vtable used to override socket IO, mainly for TLS. |
| `fio_io_tls_s` | Reference-counted TLS settings object consumed by transport backends. |
| `fio_io_listener_s` | Listener handle returned by `fio_io_listen`. |
| `fio_pubsub_msg_s` | Message delivered to `on_pubsub` callbacks; defined by the Pub/Sub module. |
| `fio_io_async_s` | Async worker queue attached to the reactor lifecycle. |

---

## Reactor Lifecycle and State

### Start / stop

```c
void fio_io_start(int workers);
void fio_io_stop(void);
void fio_io_add_workers(int workers);
void fio_io_restart(int workers);
void fio_io_restart_on_signal(int signal);
size_t fio_io_shutdown_timeout(void);
size_t fio_io_shutdown_timeout_set(size_t milliseconds);
```

`fio_io_start(workers)` starts the reactor and blocks until shutdown. A positive
`workers` value forks worker processes. `fio_io_stop()` asks the reactor to
stop. `fio_io_add_workers` and `fio_io_restart` are cluster/process helpers;
`fio_io_restart_on_signal` binds hot restart to a signal.

SIGINT/SIGTERM handlers only publish an atomic stop request; the reactor logs
and starts graceful shutdown on its next review. A second stop signal exits
the receiving process immediately without cleanup. Required signal-monitor
registration failure is fatal rather than starting without stop protection.
SIGPIPE suppression is process-lifetime, shared with OpenSSL, and remains
active after reactor shutdown.

`fio_io_shutdown_timeout_set` changes the hard shutdown grace period and returns
the value that will be used.

### State queries

```c
int fio_io_is_running(void);
int fio_io_is_master(void);
int fio_io_is_worker(void);
uint16_t fio_io_workers(int workers_requested);
int fio_io_pid(void);
int fio_io_root_pid(void);
int64_t fio_io_last_tick(void);
int64_t fio_io_last_tick_time(void);
```

`fio_io_last_tick()` is the cached millisecond value from the last reactor poll.
`fio_io_last_tick_time()` is the cached wall-clock millisecond timestamp (ms
since epoch), useful for approximate log and HTTP date values and for
timestamps compared across machines (IPC cluster, pub/sub).

While the reactor is stopped (before `fio_io_start`, or after it returns), both
return the current time, so timers and IO timeouts scheduled between reactor
runs start from "now" rather than from the previous run's last poll.

---

## Protocols

```c
typedef struct fio_io_protocol_s fio_io_protocol_s;

struct fio_io_protocol_s {
  struct { /* reserved; initialize to zero */ } reserved;

  void (*on_attach)(fio_io_s *io);
  void (*on_data)(fio_io_s *io);
  void (*on_ready)(fio_io_s *io);
  void (*on_shutdown)(fio_io_s *io);
  void (*on_timeout)(fio_io_s *io);
  void (*on_close)(void *iobuf, void *udata);
  void (*on_pubsub)(fio_pubsub_msg_s *msg);

  void (*on_user1)(fio_io_s *io, void *user_data);
  void (*on_user2)(fio_io_s *io, void *user_data);
  void (*on_user3)(fio_io_s *io, void *user_data);
  void (*on_reserved)(fio_io_s *io, void *user_data);

  fio_io_functions_s io_functions;
  uint32_t timeout;
  uint32_t buffer_size;
};
```

Protocol structs define connection behavior. They are normally static/global and
shared by many IO handles. Initialize the full struct to zero and never mutate
`reserved`.

Callback flow:

1. `on_attach` after an IO is attached to the protocol.
2. `on_data` when incoming data can be read.
3. `on_ready` after all pending writes drain.
4. `on_timeout` when the connection timeout is reached. An IO already closing
   with `fio_io_close` (still draining output) is closed immediately on
   timeout instead, dropping unsent data.
5. `on_shutdown` immediately before reactor shutdown closes the connection.
6. `on_close(iobuf, udata)` after the connection is closed.

All protocol callbacks return `void`. Use `fio_io_close`,
`fio_io_close_now`, or protocol state to control what happens next. Set
`on_timeout = fio_io_touch` for connections where idle timeout should be
ignored.

`timeout` is in milliseconds, capped by `FIO_IO_TIMEOUT_MAX`; `0` means the
maximum/default. `buffer_size` controls the per-connection protocol buffer
returned by `fio_io_buffer`.

### Iterating protocol IOs

```c
size_t fio_io_protocol_each(fio_io_protocol_s *protocol,
                            void (*task)(fio_io_s *, void *udata2),
                            void *udata2);
```

Runs `task` for each IO using `protocol`. Call only from the main IO thread;
use `fio_io_defer` when scheduling from another thread.

---

## Transport Functions / TLS Hooks

```c
typedef struct fio_io_functions_s fio_io_functions_s;

struct fio_io_functions_s {
  void *(*build_context)(fio_io_tls_s *tls, uint8_t is_client);
  void (*free_context)(void *context);
  void *(*dup_context)(void *context);
  void (*start)(fio_io_s *io);
  ssize_t (*read)(fio_socket_i fd, void *buf, size_t len, void *context);
  ssize_t (*write)(fio_socket_i fd, const void *buf, size_t len, void *context);
  int (*flush)(fio_socket_i fd, void *context);
  void (*finish)(fio_socket_i fd, void *context);
  void (*cleanup)(void *context);
};
```

The vtable lets a protocol replace plain socket IO with a transport layer such
as TLS. Functions receive the file descriptor but must not keep it or defer fd
operations; the `fio_io_s` handle is the long-lived identity.

Return conventions:

- When `tls` is non-NULL, the functions are a TLS variant (set by the user, or
  by the reactor's protocol validation when TLS is given). `build_context`
  returning `NULL` then means the TLS context couldn't be built, and
  `fio_io_listen` / `fio_io_connect` fail rather than continue without TLS.
- `read` behaves like non-blocking `read(2)`.
- `write` returns plaintext bytes accepted (`N > 0`), `0` for zero-length
  input, or `-1` with `errno` set. If data was transformed (for example,
  encrypted), return success even if the underlying socket later blocks; use
  `flush` for pending transformed bytes.
- `flush` returns `0` only when all internal output is empty. Non-zero (`N > 0`
  or `-1`) means pending data remains and the reactor should keep watching for
  writability.
- On graceful close (`fio_io_close`) the reactor waits until `flush` returns
  `0`, calls `finish` once (for example, to queue a TLS `close_notify`), then
  waits for `flush` to return `0` again before closing. The wait is bounded by
  the connection timeout. `cleanup` releases the per-connection transport
  context after close.
- Context references: `build_context` and `dup_context` each return a
  reference, released with `free_context`. `build_context` must duplicate its
  settings reference with `fio_io_tls_dup` if it needs the settings after the
  callback returns. A listener keeps its own context reference
  and passes the `dup_context` result to every accepted connection as is.
  `dup_context` may return `NULL` (it defaults to returning `NULL`, which also
  suits plaintext's `NULL` context); `start` then sees a `NULL` context (the
  bundled TLS backends close such connections).
  Adding `dup_context` changes the struct layout; rebuild code embedding it.
- `fio_io_attach_fd` takes ownership of its `tls` reference. `start` either
  replaces it (`fio_io_tls_set`) with per-connection state that takes over
  the reference (for example, a connection that keeps its parent context
  alive) or releases it; `cleanup` releases the IO's `tls` after close.
- `free_context` may run while connections started from the context are still
  open (for example, after a listener stops). Keep any context state those
  connections use alive until their `cleanup`, for example by reference
  counting.

Set default TLS transport functions before the reactor starts:

```c
fio_io_functions_s fio_io_tls_default_functions(fio_io_functions_s *funcs);
```

Passing `NULL` returns the current default. Passing a pointer sets a new
default and returns the selected functions.

---

## Listening and Connecting

### `fio_io_listen`

```c
typedef struct fio_io_listen_args_s {
  const char *url;
  fio_io_protocol_s *protocol;
  void *udata;
  fio_io_tls_s *tls;
  void (*on_start)(fio_io_protocol_s *protocol, void *udata);
  void (*on_stop)(fio_io_protocol_s *protocol, void *udata);
  fio_io_async_s *queue_for_accept;
  uint8_t on_root;
  uint8_t hide_from_log;
} fio_io_listen_args_s;

fio_io_listener_s *fio_io_listen(fio_io_listen_args_s args);
#define fio_io_listen(...) fio_io_listen((fio_io_listen_args_s){__VA_ARGS__})
```

Creates a network listener and returns a self-destructible listener handle, or
`NULL` on error. The default URL is `tcp://0.0.0.0:3000`.

Call it before `fio_io_start()` for normal server setup. The header notes that
this schedules a task and should not be called from `PRE_START` or `ON_START`
state callbacks.

TLS can be supplied directly with `.tls` or inferred from the URL query:

```c
fio_io_listen(.url = "0.0.0.0:3000/?tls", .protocol = &MY_PROTOCOL);
fio_io_listen(.url = "0.0.0.0:3000/?tls=./certs/", .protocol = &MY_PROTOCOL);
fio_io_listen(.url = "0.0.0.0:3000/?key=./key.pem&cert=./cert.pem",
              .protocol = &MY_PROTOCOL);
```

TLS is a must when requested: if a TLS object is in effect (`.tls`, a TLS
scheme or URL hints) but the context can't be built (for example, a missing
certificate file), `fio_io_listen` returns `NULL` instead of creating a
listener that refuses every connection. As with a failed bind, `on_stop` (if
set) is called so `protocol` / `udata` state can be released.

`.tls` ownership moves to `fio_io_listen` (success or failure; the same applies
to `fio_io_connect`). If you need to share a TLS settings object, pass
`fio_io_tls_dup(tls)` and free your own reference with `fio_io_tls_free`.

Listener helpers:

```c
void fio_io_listen_stop(fio_io_listener_s *l);
fio_io_protocol_s *fio_io_listener_protocol(fio_io_listener_s *l);
void *fio_io_listener_udata(fio_io_listener_s *l);
void *fio_io_listener_udata_set(fio_io_listener_s *l, void *new_udata);
fio_buf_info_s fio_io_listener_url(fio_io_listener_s *l);
int fio_io_listener_is_tls(fio_io_listener_s *l);
```

### `fio_io_connect`

```c
typedef struct {
  const char *url;
  fio_io_protocol_s *protocol;
  void (*on_failed)(fio_io_protocol_s *protocol, void *udata);
  void *udata;
  fio_io_tls_s *tls;
  uint32_t timeout;
  uint8_t verify;
} fio_io_connect_args_s;

fio_io_s *fio_io_connect(fio_io_connect_args_s args);
#define fio_io_connect(url_, ...) \
  fio_io_connect((fio_io_connect_args_s){.url = url_, __VA_ARGS__})
```

Connects to `url` as a client and returns the IO handle or `NULL`. The URL may
contain TLS hints. `timeout` defaults to 30 seconds. `on_failed` is the cleanup
hook for failed connection attempts; established connections use the protocol
callbacks.

If TLS is requested but the TLS context can't be built, no socket is opened
and `NULL` is returned; `on_failed` still runs once on the IO thread (possibly
after `fio_io_connect` returns) and remains the only place to release
`protocol` / `udata`.

For TLS URLs (`https`, `wss`, `tls`, or explicit TLS hints), the URL's host
becomes the expected server identity, replacing any name previously stored in
the TLS object. When trust is configured, the server certificate must match it
(DNS name or IP address SAN). DNS host names are also sent as the SNI (Server
Name Indication) host name; IP literals are never sent as SNI (RFC 6066).

**Verifying the server (opt-in).** A TLS client without trust entries does
**not** verify the server certificate (a warning is logged). To verify against
the system trust store, use either the `trust=sys` URL query or `.verify`:

```c
fio_io_connect("tls://example.com:443/?trust=sys", .protocol = &MY_PROTOCOL);
fio_io_connect("tls://example.com:443", .protocol = &MY_PROTOCOL, .verify = 1);
/* custom CA file (`.pem` is appended when missing) */
fio_io_connect("tls://example.com:443/?trust=./my-ca", .protocol = &MY_PROTOCOL);
```

When `.verify` is set and the connection uses TLS, a TLS object without trust
entries gets the system trust store (same as `fio_io_tls_trust_add(tls, NULL)`).
Explicit trust entries (`.tls` or `trust=<file>`) are kept as is. Like URL TLS
hints, this updates the `.tls` object, so later connections using a
`fio_io_tls_dup` reference to it also verify. For non-TLS connections `.verify`
is ignored; it never changes the URL scheme. Once trust is set, the server certificate's DNS name /
IP address SAN must also match the URL host.

---

## IO Handles and Operations

```c
fio_io_s *fio_io_attach_fd(fio_socket_i fd,
                           fio_io_protocol_s *protocol,
                           void *udata,
                           void *tls);
fio_io_protocol_s *fio_io_protocol_set(fio_io_s *io,
                                       fio_io_protocol_s *protocol);
fio_io_protocol_s *fio_io_protocol(fio_io_s *io);
void *fio_io_buffer(fio_io_s *io);
size_t fio_io_buffer_len(fio_io_s *io);
void *fio_io_udata_set(fio_io_s *io, void *udata);
void *fio_io_udata(fio_io_s *io);
void *fio_io_tls_set(fio_io_s *io, void *tls);
void *fio_io_tls(fio_io_s *io);
fio_socket_i fio_io_fd(fio_io_s *io);
void fio_io_touch(fio_io_s *io);
size_t fio_io_read(fio_io_s *io, void *buf, size_t len);
void fio_io_close(fio_io_s *io);
void fio_io_close_now(fio_io_s *io);
fio_io_s *fio_io_dup(fio_io_s *io);
void fio_io_free(fio_io_s *io);
void fio_io_suspend(fio_io_s *io);
void fio_io_unsuspend(fio_io_s *io);
int fio_io_is_suspended(fio_io_s *io);
int fio_io_is_open(fio_io_s *io);
size_t fio_io_backlog(fio_io_s *io);
void fio_io_noop(fio_io_s *io);
```

`fio_io_attach_fd` adopts a valid socket into the reactor. It returns `NULL` on
error. Ownership of `fd` and `tls` (a `build_context` / `dup_context`
reference) transfers on success and on failure; call `dup_context` first to
keep a reference. A valid `fd` that can't be attached (allocation failure, or
the socket can't be made non-blocking) is closed, `tls` is released with
`free_context` and the protocol's `on_close` is scheduled. The returned
pointer must not be used arbitrarily; IO handles are valid
inside proper callbacks and scheduled tasks. With a `NULL` protocol, any
`tls` reference must come from the default TLS backend. If code must keep a handle outside
that context, call `fio_io_dup` and later `fio_io_free`. These two functions are
thread-safe. A write whose packet or task can't be allocated requests closure
of the IO (`fio_io_close`). Closure itself is scheduled through the IO queue
and can be lost while that queue is saturated; no completion guarantee applies
in that case.

`fio_io_protocol_set` installs a new protocol. `NULL` is a valid "only-write"
protocol. The accessor may temporarily return the old protocol while the change
is being attached. If the state keeping a custom transport (`NULL` over a
custom transport) can't be allocated, the switch fails closed: the change isn't
applied and the IO is closed immediately (like `fio_io_close_now`). This is
distinct from a change whose queued task can't be allocated: that change is
dropped without closing the IO.

`fio_io_read` returns bytes read. `0` is not EOF by itself; it can mean no data
was available on the non-blocking socket. Use close callbacks for final cleanup.

`fio_io_close` closes after scheduled data is sent. `fio_io_close_now` closes as
soon as possible. `fio_io_is_open` returns 1 only while open and not marked for
closure: it returns 0 immediately after `fio_io_close`, even while previously
scheduled output is draining. `fio_io_suspend` / `fio_io_unsuspend` control future
`on_data` delivery, and `fio_io_backlog` reports the approximate outgoing byte
count.

### Writing

```c
typedef struct {
  void *buf;
  intptr_t fd;
  size_t len;
  size_t offset;
  void (*dealloc)(void *);
  uint8_t copy;
} fio_io_write_args_s;

void fio_io_write2(fio_io_s *io, fio_io_write_args_s args);
#define fio_io_write2(io, ...) \
  fio_io_write2(io, (fio_io_write_args_s){__VA_ARGS__})
#define fio_io_write(io, buf_, len_) \
  fio_io_write2(io, .buf = (buf_), .len = (len_), .copy = 1)
#define fio_io_sendfile(io, source_fd, offset_, bytes) \
  fio_io_write2((io), .fd = (source_fd), .offset = (size_t)(offset_), .len = (bytes))
```

`fio_io_write2` schedules buffered output. If `.buf` is used and `.copy` is
non-zero, the data is copied immediately. If `.copy == 0` and `.dealloc` is set,
the IO layer takes ownership of the buffer and calls `dealloc` later. If
`.dealloc == NULL`, the buffer is not freed by the IO layer.

For file output, pass `.fd`; `.len == 0` means send the whole file. The
`fio_io_sendfile` helper closes `source_fd` after sending or on error.

---

## Scheduling Tasks and Timers

```c
void fio_io_defer(void (*task)(void *, void *), void *udata1, void *udata2);
void fio_io_run_every(fio_timer_schedule_args_s args);
#define fio_io_run_every(...) \
  fio_io_run_every((fio_timer_schedule_args_s){__VA_ARGS__})
fio_queue_s *fio_io_queue(void);
```

`fio_io_defer` schedules a task on the IO reactor queue and is thread-safe.
Use it to move work from other threads back to the IO thread.

`fio_io_run_every` schedules a timer. It uses `fio_timer_schedule_args_s` from
the queue/timer API:

- `fn` returns non-zero to stop the timer.
- `udata1` and `udata2` are passed to callbacks.
- `on_stop` runs when the timer ends.
- `every` is the interval in milliseconds.
- `repetitions` is the number of runs; `-1` means indefinitely.

`fio_io_queue()` returns the reactor queue.

---

## Connection Environment

The IO environment links named objects to a connection lifetime. When the IO is
closed, stored `on_close` callbacks run automatically.

```c
typedef struct {
  intptr_t type;
  fio_buf_info_s name;
  void *udata;
  void (*on_close)(void *data);
  uint8_t const_name;
} fio_io_env_set_args_s;

typedef struct {
  intptr_t type;
  fio_buf_info_s name;
} fio_io_env_get_args_s;

void *fio_io_env_get(fio_io_s *io, fio_io_env_get_args_s args);
void fio_io_env_set(fio_io_s *io, fio_io_env_set_args_s args);
int fio_io_env_unset(fio_io_s *io, fio_io_env_get_args_s args);
int fio_io_env_remove(fio_io_s *io, fio_io_env_get_args_s args);

#define fio_io_env_get(io, ...) \
  fio_io_env_get(io, (fio_io_env_get_args_s){__VA_ARGS__})
#define fio_io_env_set(io, ...) \
  fio_io_env_set(io, (fio_io_env_set_args_s){__VA_ARGS__})
#define fio_io_env_unset(io, ...) \
  fio_io_env_unset(io, (fio_io_env_get_args_s){__VA_ARGS__})
#define fio_io_env_remove(io, ...) \
  fio_io_env_remove(io, (fio_io_env_get_args_s){__VA_ARGS__})
```

`type` and `name` together identify an entry. Negative `type` values are
reserved. If `const_name` is set, the name string must outlive the environment.

If `io == NULL`, entries are stored in the global environment; their `on_close`
callbacks run when the process exits. `unset` detaches without calling
`on_close`; `remove` detaches and calls `on_close` as if the connection closed.

---

## TLS Settings Helpers

```c
fio_io_tls_s *fio_io_tls_new(void);
fio_io_tls_s *fio_io_tls_from_url(fio_io_tls_s *target_or_null, fio_url_s url);
fio_io_tls_s *fio_io_tls_dup(fio_io_tls_s *tls);
void fio_io_tls_free(fio_io_tls_s *tls);

fio_io_tls_s *fio_io_tls_cert_add(fio_io_tls_s *tls,
                                  const char *server_name,
                                  const char *public_cert_file,
                                  const char *private_key_file,
                                  const char *pk_password);
fio_io_tls_s *fio_io_tls_alpn_add(fio_io_tls_s *tls,
                                  const char *protocol_name,
                                  void (*on_selected)(fio_io_s *));
int fio_io_tls_alpn_select(fio_io_tls_s *tls,
                           const char *protocol_name,
                           size_t name_length,
                           fio_io_s *io);
fio_io_tls_s *fio_io_tls_trust_add(fio_io_tls_s *tls,
                                   const char *public_cert_file);
fio_io_tls_s *fio_io_tls_sni_set(fio_io_tls_s *tls,
                                 const char *hostname,
                                 size_t hostname_len);
uintptr_t fio_io_tls_cert_count(fio_io_tls_s *tls);
uintptr_t fio_io_tls_alpn_count(fio_io_tls_s *tls);
uintptr_t fio_io_tls_trust_count(fio_io_tls_s *tls);
```

`fio_io_tls_s` stores backend-neutral TLS instructions. Add certificates for
SNI, ALPN protocol callbacks, and trusted certificates / CA stores. Passing
`NULL` certificate paths can request a backend-provided self-signed certificate;
passing `NULL` to `fio_io_tls_trust_add` asks the backend to use system trust.
Backend behavior is documented in the TLS backend docs.

A server may hold several certificates in one `fio_io_tls_s` object; the
context builder loads each one once and every TLS backend selects one per
handshake from the ClientHello SNI host name (before any application data):

1. a case-insensitive exact match of the `server_name`;
2. else a single-label wildcard entry (`*.example.com` matches
   `www.example.com`, but neither `example.com` nor `a.b.example.com`);
3. else (no SNI, unknown or malformed name — empty, over 255 bytes, holding
   spaces, control, `*` or non-ASCII bytes) the **default** certificate: the
   one added with an empty / `NULL` `server_name`, else the first one added.

`server_name` is stored lowercased (names over 255 bytes are rejected) and
re-adding a name replaces its entry. Certificates added without files are
self-signed for their name (`"localhost"` for the default). A certificate or
key file that fails to load (missing file, bad PEM, wrong password) fails the
context build in every backend; it never falls back to a self-signed
certificate.

`fio_io_tls_sni_set` sets the SNI (Server Name Indication) host name a TLS
client sends during the handshake. `fio_io_connect` sets it automatically from
the URL's host name, so manual calls are rarely needed. `NULL` / empty host
names and IP literals (IPv4 / IPv6) are silently ignored, as SNI carries DNS
host names only (RFC 6066).

`fio_io_tls_alpn_add` silently ignores a `NULL` protocol name and replaces a
`NULL` callback with a no-op. The first ALPN protocol added is the default.

### Iterating TLS settings

```c
typedef struct fio_io_tls_each_s {
  fio_io_tls_s *tls;
  void *udata;
  void *udata2;
  int (*each_cert)(struct fio_io_tls_each_s *,
                   const char *server_name,
                   const char *public_cert_file,
                   const char *private_key_file,
                   const char *pk_password);
  int (*each_alpn)(struct fio_io_tls_each_s *,
                   const char *protocol_name,
                   void (*on_selected)(fio_io_s *));
  int (*each_trust)(struct fio_io_tls_each_s *, const char *public_cert_file);
} fio_io_tls_each_s;

int fio_io_tls_each(fio_io_tls_each_s args);
#define fio_io_tls_each(tls_, ...) \
  fio_io_tls_each(((fio_io_tls_each_s){.tls = tls_, __VA_ARGS__}))
```

Transport backends use `fio_io_tls_each` to consume the stored instructions.
Iterator callbacks return `int`; check the backend using them for any additional
meaning assigned to non-zero return values.

### Peer certificate inspection (mTLS)

```c
int fio_io_peer_info_next(fio_io_s *io, fio_x509_cert_s *dest);
```

Trust configured with `fio_io_tls_trust_add` always refers to **peer**
verification: a server context verifies (and requires) client certificates,
a client context verifies the server certificate. An empty trust list means
no peer verification; passing `NULL` to `fio_io_tls_trust_add` selects the
system trust store. A trust instruction that can't be stored (allocation
failure, or an overlong `tls=` URL trust value) fails closed: it still counts
in `fio_io_tls_trust_count`, and `fio_io_tls_each` fails for trust iteration,
so the backend refuses to build the context rather than skip verification.

Once the handshake completed, iterate the peer's certificate chain (leaf
first) to implement application level client certificate authentication and
authorization:

```c
fio_x509_cert_s cert = {0}; /* zeroed = new loop */
while (fio_io_peer_info_next(io, &cert) == 0) {
  if (!cert.verified)
    continue; /* chain failed TLS-level verification (or was skipped) */
  /* authorize by identity, e.g.: cert.cn,
   * cert.subject, or pin cert.fingerprint (32-byte SHA-256) */
}
```

The iterator is stateless — the position is identified from `dest` alone: a
zeroed `dest` (`der.buf == NULL`) starts a new loop; otherwise iteration
continues at `dest->chain_index + 1`. To restart a loop, zero the struct.
Multiple loops may iterate the same connection concurrently.
Iteration is capped at 128 certificates (`chain_index` 0..127) as a
deep-nesting / DoS guard — longer chains end the loop with -1.

Passing a `NULL` `dest` returns -1. The function returns
-1 when the iteration is done or when peer information is unavailable (no
TLS, handshake incomplete, peer sent no certificate, or the X509 module
missing).

Each call parses the next certificate into a `fio_x509_cert_s` (defined in
the X509 module, `156 x509.h`). All certificate fields point directly into
memory owned by the TLS backend — nothing is copied or allocated. The views
stay valid until the next `fio_io_peer_info_next` call (on ANY connection)
or connection close, whichever comes first; copy anything that must outlive
the loop.

The `verified` flag reflects the TLS backend's verification of the whole
chain and is identical for every certificate in the chain. Connections that
skipped verification (empty trust list) report `verified == 0`.

---

## Async Worker Queues

```c
typedef struct fio_io_async_s fio_io_async_s;
#define FIO_IO_ASYN_INIT ((fio_io_async_s){0})

fio_queue_s *fio_io_async_queue(fio_io_async_s *q);
void fio_io_async_attach(fio_io_async_s *q, uint32_t threads);
#define fio_io_async(q_, ...) \
  fio_queue_push(fio_io_async_queue(q_), __VA_ARGS__)
void fio_io_async_every(fio_io_async_s *q, fio_timer_schedule_args_s args);
#define fio_io_async_every(async, ...) \
  fio_io_async_every(async, (fio_timer_schedule_args_s){__VA_ARGS__})
```

Async queues are for non-IO work that must not block the reactor. Allocate the
`fio_io_async_s` object with static or otherwise reactor-long lifetime and
initialize it with `FIO_IO_ASYN_INIT` or zeroes before attaching it.

```c
static fio_io_async_s SLOW_TASKS = FIO_IO_ASYN_INIT;

int main(void) {
  fio_io_async_attach(&SLOW_TASKS, 32);
  fio_io_start(0);
}
```

The queue starts and stops with the IO reactor. While the reactor isn't running
(or if worker threads can't be spawned), tasks run on the IO queue. Stopping the
reactor (or spawning worker processes) routes new tasks to the IO queue, joins
the worker threads and performs pending tasks, so no async task is running once
`fio_io_start` returns or a worker process is forked; the queue restarts once
forking is done. A task pushed by another thread while the queue stops is never
lost: it runs on the next start or at exit. At exit, the IO queue and every
async queue are drained until all are empty (tasks may push more tasks, as may
timer `on_finish` and global environment `on_close` callbacks) before an async
queue's memory is released. A forked worker inherits the async queues stopped
(the parent's threads and their locks are gone) and runs inherited pending
tasks when its reactor starts, or at exit. Calling `fio_io_async_attach` again
while running replaces the worker threads; the old threads finish their current
task and exit.

Use `fio_io_async_queue` when a
raw `fio_queue_s *` is needed, `fio_io_async` to push tasks, and
`fio_io_async_every` for timers on that async queue.

---

## Minimal Echo Server

```c
#define FIO_IO
#include "fio-stl/include.h"

static void echo_on_data(fio_io_s *io) {
  char buf[4096];
  size_t len;

  while ((len = fio_io_read(io, buf, sizeof(buf))))
    fio_io_write(io, buf, len); /* copies stack buffer */
}

static void echo_on_timeout(fio_io_s *io) { fio_io_close(io); }

static fio_io_protocol_s ECHO = {
    .on_data = echo_on_data,
    .on_timeout = echo_on_timeout,
    .timeout = 30000,
};

int main(void) {
  fio_io_listen(.url = "tcp://0.0.0.0:3000", .protocol = &ECHO);
  fio_io_start(0);
}
```

---

## Common Lifetime Rules

- Treat `fio_io_s *` as callback/task scoped unless you hold a reference with
  `fio_io_dup`; release it with `fio_io_free`.
- Do not keep raw file descriptors received by transport callbacks.
- Protocol objects should be zero-initialized and live at least as long as any
  IO that uses them.
- Listener `.tls` is moved to the listener; duplicate TLS settings before
  sharing them elsewhere.
- `fio_io_write` copies the buffer; `fio_io_write2` can copy, borrow, or take
  ownership depending on `.copy` and `.dealloc`.
- Environment `on_close` callbacks are the preferred cleanup point for objects
  tied to a connection lifetime.
