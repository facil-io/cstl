# WebSocket Parser

Parse WebSocket frames (RFC 6455). 24 bytes of state. Zero allocation. Bytes in, events out.

## Quick Start

```c
#define FIO_WEBSOCKET_PARSER
#include FIO_INCLUDE_FILE

fio_websocket_s parser;
fio_websocket_init(&parser);

// In your read loop:
fio_websocket_event_s ev;
size_t consumed = fio_websocket_parse(&parser, input_buffer, &ev);

switch (ev.type) {
  case FIO_WEBSOCKET_EV_DATA_CHUNK:
    if (ev.is_first) { /* start new message */ }
    /* ev.payload points into input_buffer, already unmasked */
    if (ev.is_last)  { /* message complete */ }
    break;
  case FIO_WEBSOCKET_EV_CONTROL:
    if (ev.opcode == FIO_WEBSOCKET_OP_PING) { /* send pong */ }
    break;
  case FIO_WEBSOCKET_EV_ERROR:
    /* protocol error—ev.close_code is set */
    break;
}
```

## What It Does

This parser reads raw WebSocket bytes and produces structured events. It:

- Parses frame headers (opcode, length, mask, FIN, RSV)
- Unmasks payload **in place** in your buffer
- Validates protocol invariants (opcode whitelist, fragmentation rules, close codes)
- Reports data chunks and control frames as events

It does **not**:

- Allocate memory
- Store callbacks
- Buffer control frames internally
- Accumulate message data
- Validate masking policy (client vs server)
- Run extension transforms (e.g., permessage-deflate)

Those concerns belong in the caller.

## Parser State

```c
typedef struct fio_websocket_s {
  uint64_t frame_remaining;  // bytes left in current frame
  uint32_t mask;             // frame mask (0 = unmasked)
  uint32_t frame_consumed;   // bytes consumed (for mask rotation)
  uint16_t close_code;       // last error/close code
  uint8_t state;             // HEADER, PAYLOAD, CLOSED, ERROR
  uint8_t flags;             // fin, masked, opcode, msg_opcode
  uint8_t flags2;            // paused, msg_rsv
  uint8_t reserved;
} fio_websocket_s;
```

**Size: 24 bytes.** Verified at compile time.

### Flags

Access via macros—do not touch fields directly:

```c
FIO_WEBSOCKET_GET_FIN(p)        // current frame FIN bit
FIO_WEBSOCKET_GET_MASKED(p)     // current frame MASK bit
FIO_WEBSOCKET_GET_OPCODE(p)     // current frame opcode (0,1,2,8,9,10)
FIO_WEBSOCKET_GET_MSG_OPCODE(p) // message opcode (1=text, 2=binary, 0=none)
FIO_WEBSOCKET_GET_PAUSED(p)     // one-message-per-parse gate
FIO_WEBSOCKET_GET_MSG_RSV(p)    // RSV bits from opening frame (3-bit value)
```

## Events

```c
typedef struct {
  uint8_t type;            // NONE, DATA_CHUNK, CONTROL, ERROR
  uint8_t opcode;          // frame opcode (control frames)
  uint8_t is_text;         // 1=text, 0=binary (data chunks)
  uint8_t rsv;             // RSV bits from opening frame
  uint8_t is_first;        // first chunk of a new message
  uint8_t is_last;         // last chunk of message
  fio_buf_info_s payload;  // points INTO your input buffer
  uint16_t close_code;     // for CLOSE and ERROR
} fio_websocket_event_s;
```

### Event Types

| Type | When It Fires |
|------|---------------|
| `FIO_WEBSOCKET_EV_NONE` | No complete frame yet—feed more bytes |
| `FIO_WEBSOCKET_EV_DATA_CHUNK` | Data payload available. `payload` points into your buffer. `is_first`/`is_last` mark message boundaries. |
| `FIO_WEBSOCKET_EV_CONTROL` | Complete control frame. `opcode` is PING, PONG, or CLOSE. `payload` is the full control payload (≤125 bytes). |
| `FIO_WEBSOCKET_EV_ERROR` | Protocol violation. `close_code` set. Parser enters ERROR state. |

### Buffer Lifetime

**Critical:** `ev.payload.buf` points **into the input buffer you passed to `fio_websocket_parse()`**. The parser unmasks in place. Keep the input buffer valid until you are done with the payload.

## API Reference

### Lifecycle

```c
void fio_websocket_init(fio_websocket_s *p);   // zero, set state = HEADER
void fio_websocket_reset(fio_websocket_s *p);  // same as init
```

### Parsing

```c
size_t fio_websocket_parse(fio_websocket_s *p,
                           fio_buf_info_s buf,
                           fio_websocket_event_s *ev);
```

**Returns:** bytes consumed (≤ `buf.len`), or `FIO_WEBSOCKET_PARSE_ERROR` on protocol error.

**Parameters:**
- `p` — parser state (must be initialized)
- `buf` — `{buf, len}` pointing to incoming bytes
- `ev` — out-parameter receiving the event (may be NULL, but then errors are silent)

### Writing Frames

Server functions produce **unmasked** frames. Client functions produce **masked** frames (PRNG mask when `mask=0`).

```c
// Data messages
uint64_t fio_websocket_write_message_server(void *dst, fio_buf_info_s msg,
                                            _Bool is_text, uint8_t rsv);
uint64_t fio_websocket_write_message_client(void *dst, fio_buf_info_s msg,
                                            _Bool is_text, uint32_t mask,
                                            uint8_t rsv);

// Control frames
uint64_t fio_websocket_write_ping_server(void *dst, fio_buf_info_s p);
uint64_t fio_websocket_write_ping_client(void *dst, fio_buf_info_s p,
                                         uint32_t mask);
uint64_t fio_websocket_write_pong_server(void *dst, fio_buf_info_s p);
uint64_t fio_websocket_write_pong_client(void *dst, fio_buf_info_s p,
                                         uint32_t mask);
uint64_t fio_websocket_write_close_server(void *dst, uint16_t code,
                                          fio_buf_info_s reason);
uint64_t fio_websocket_write_close_client(void *dst, uint16_t code,
                                          fio_buf_info_s reason,
                                          uint32_t mask);

// Utility
uint64_t fio_websocket_write_len(uint64_t payload_len, _Bool masked);
```

**RSV bits:** Pass `FIO_WEBSOCKET_RSV1` (0x4) to mark permessage-deflate compression (RFC 7692). The parser exposes RSV in `ev.rsv`; the caller interprets them.

## Complete Example

```c
#include "fio-stl.h"

typedef struct {
  fio_websocket_s parser;
  fio_bstr_s *accumulator;   // message accumulator
  void (*on_message)(fio_buf_info_s msg, _Bool is_text);
} connection_t;

void on_data(connection_t *c, fio_websocket_event_s *ev) {
  if (ev->is_first) {
    fio_bstr_free(c->accumulator);
    c->accumulator = NULL;
  }
  if (ev->payload.len) {
    c->accumulator = fio_bstr_write(c->accumulator,
                                    ev->payload.buf,
                                    ev->payload.len);
  }
  if (ev->is_last) {
    fio_buf_info_s msg = c->accumulator
        ? fio_bstr_buf(c->accumulator)
        : FIO_BUF_INFO0;
    c->on_message(msg, ev->is_text);
    fio_bstr_free(c->accumulator);
    c->accumulator = NULL;
  }
}

void on_control(connection_t *c, fio_websocket_event_s *ev) {
  switch (ev->opcode) {
  case FIO_WEBSOCKET_OP_PING: {
    char buf[140];
    size_t n = fio_websocket_write_pong_server(buf, ev->payload);
    send(c->fd, buf, n, 0);
    break;
  }
  case FIO_WEBSOCKET_OP_CLOSE: {
    uint16_t code = ev->close_code;
    fio_buf_info_s reason = (ev->payload.len > 2)
        ? FIO_BUF_INFO2(ev->payload.buf + 2, ev->payload.len - 2)
        : FIO_BUF_INFO0;
    printf("Close: %d %.*s\n", code, (int)reason.len, reason.buf);
    break;
  }
  }
}

void process_bytes(connection_t *c, char *data, size_t len) {
  fio_buf_info_s buf = FIO_BUF_INFO2(data, len);
  while (buf.len) {
    fio_websocket_event_s ev = {0};
    size_t n = fio_websocket_parse(&c->parser, buf, &ev);
    if (n == FIO_WEBSOCKET_PARSE_ERROR) {
      fprintf(stderr, "WebSocket error: %d\n", c->parser.close_code);
      return;
    }
    buf.buf += n;
    buf.len -= n;
    switch (ev.type) {
    case FIO_WEBSOCKET_EV_DATA_CHUNK:  on_data(c, &ev); break;
    case FIO_WEBSOCKET_EV_CONTROL:     on_control(c, &ev); break;
    case FIO_WEBSOCKET_EV_ERROR:       return;
    }
  }
}
```

## Validation

The parser enforces:

- Opcode whitelist: 0, 1, 2, 8, 9, 10 only
- Control frames: FIN=1, payload ≤ 125 bytes
- Fragmentation: continuations follow openings; no nested messages
- Extended length: MSB must be 0
- Frame size: ≤ `FIO_WEBSOCKET_DEFAULT_MAX_FRAME` (1 GiB)
- Close codes: validated per RFC §7.4.1

**Caller must enforce:**

- Masking policy (client masks, server does not)
- RSV semantics and extension transforms
- Message size limits
- UTF-8 validation for text frames

## Thread Safety

The parser is fully thread-safe. It stores no shared state, no callbacks, and no pointers to external data. Multiple threads may parse concurrently with independent `fio_websocket_s` instances. Synchronize access to shared buffers and connection state in the caller.

## Performance

- **24 bytes**: fits in one cache line
- **Zero allocation**: no malloc/free in parse path
- **In-place unmasking**: no payload copies
- **No callback indirection**: events returned by value
- **Straight-line hot path**: header parse → chunk emit, minimal branches

## Constants

```c
// Close codes (RFC 6455 §7.4.1)
FIO_WEBSOCKET_CLOSE_OK               = 1000
FIO_WEBSOCKET_CLOSE_GOING_AWAY       = 1001
FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR   = 1002
FIO_WEBSOCKET_CLOSE_UNSUPPORTED_DATA = 1003
FIO_WEBSOCKET_CLOSE_NO_STATUS        = 1005
FIO_WEBSOCKET_CLOSE_INVALID_PAYLOAD  = 1007
FIO_WEBSOCKET_CLOSE_POLICY_VIOLATION = 1008
FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG  = 1009
FIO_WEBSOCKET_CLOSE_MANDATORY_EXT    = 1010
FIO_WEBSOCKET_CLOSE_INTERNAL_ERROR   = 1011

// RSV bits
FIO_WEBSOCKET_RSV1 = 0x4   // permessage-deflate
FIO_WEBSOCKET_RSV2 = 0x2
FIO_WEBSOCKET_RSV3 = 0x1

// Limits
FIO_WEBSOCKET_DEFAULT_MAX_FRAME = (1ULL << 30)  // 1 GiB
FIO_WEBSOCKET_PARSE_ERROR       = ((size_t)-1)
```

## License

See `000 copyright.h` or the top of `fio-stl.h`.
