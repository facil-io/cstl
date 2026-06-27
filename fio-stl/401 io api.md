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
`fio_io_last_tick_time()` is the cached wall-clock millisecond timestamp,
useful for approximate log and HTTP date values.

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
4. `on_timeout` when the connection timeout is reached.
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

- `read` behaves like non-blocking `read(2)`.
- `write` returns plaintext bytes accepted (`N > 0`), `0` for zero-length
  input, or `-1` with `errno` set. If data was transformed (for example,
  encrypted), return success even if the underlying socket later blocks; use
  `flush` for pending transformed bytes.
- `flush` returns `0` only when all internal output is empty. Non-zero (`N > 0`
  or `-1`) means pending data remains and the reactor should keep watching for
  writability.
- `finish` runs before closing after output is sent; `cleanup` releases the
  per-connection transport context after close.

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

`.tls` ownership is moved to the listener. If you need to share a TLS settings
object, duplicate it first with `fio_io_tls_dup` and free your own references
with `fio_io_tls_free`.

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
} fio_io_connect_args_s;

fio_io_s *fio_io_connect(fio_io_connect_args_s args);
#define fio_io_connect(url_, ...) \
  fio_io_connect((fio_io_connect_args_s){.url = url_, __VA_ARGS__})
```

Connects to `url` as a client and returns the IO handle or `NULL`. The URL may
contain TLS hints. `timeout` defaults to 30 seconds. `on_failed` is the cleanup
hook for failed connection attempts; established connections use the protocol
callbacks.

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
error. The returned pointer must not be used arbitrarily; IO handles are valid
inside proper callbacks and scheduled tasks. If code must keep a handle outside
that context, call `fio_io_dup` and later `fio_io_free`. These two functions are
thread-safe.

`fio_io_protocol_set` installs a new protocol. `NULL` is a valid "only-write"
protocol. The accessor may temporarily return the old protocol while the change
is being attached.

`fio_io_read` returns bytes read. `0` is not EOF by itself; it can mean no data
was available on the non-blocking socket. Use close callbacks for final cleanup.

`fio_io_close` closes after scheduled data is sent. `fio_io_close_now` closes as
soon as possible. `fio_io_suspend` / `fio_io_unsuspend` control future
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
uintptr_t fio_io_tls_cert_count(fio_io_tls_s *tls);
uintptr_t fio_io_tls_alpn_count(fio_io_tls_s *tls);
uintptr_t fio_io_tls_trust_count(fio_io_tls_s *tls);
```

`fio_io_tls_s` stores backend-neutral TLS instructions. Add certificates for
SNI, ALPN protocol callbacks, and trusted certificates / CA stores. Passing
`NULL` certificate paths can request a backend-provided self-signed certificate;
passing `NULL` to `fio_io_tls_trust_add` asks the backend to use system trust.
Backend behavior is documented in the TLS backend docs.

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

---

## Async Worker Queues

```c
typedef struct fio_io_async_s fio_io_async_s;
#define FIO_IO_ASYN_INIT ((fio_io_async_s){0})

fio_queue_s *fio_io_async_queue(fio_io_async_s *q);
void fio_io_async_attach(fio_io_async_s *q, uint32_t threads);
#define fio_io_async(q_, ...) fio_queue_push((q_)->q, __VA_ARGS__)
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

The queue starts and stops with the IO reactor. Use `fio_io_async_queue` when a
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
