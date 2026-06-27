# WebSocket Parser

```c
#define FIO_WEBSOCKET_PARSER
#include FIO_INCLUDE_FILE
```

Small RFC 6455 frame parser and writer. It keeps 24 bytes of parser state,
allocates nothing, unmasks incoming payload bytes in-place, and reports one
parse event at a time.

Nearby context: [IO and HTTP overview](./400 io-overview.md), the neighboring
[HTTP/1.x parser](./431 http1 parser.md), the higher-level
[HTTP module](./439 http.md), and WebSocket compression support in
[DEFLATE / Gzip](./162 deflate.md).

---

## What Gets Added

`FIO_WEBSOCKET_PARSER` exposes:

- `fio_websocket_s` — incremental parser state.
- `fio_websocket_event_s` — one parse event returned by
  `fio_websocket_parse`.
- `fio_websocket_init` / `fio_websocket_reset` — parser lifecycle helpers.
- `fio_websocket_parse` — incremental parser entry point.
- frame writer helpers for server and client data, ping, pong, and close
  frames.
- WebSocket close code, event type, opcode, RSV, parser state, flag, and error
  constants.

The implementation also defines helpers named with `fio___websocket...`; those
are private implementation details.

---

## Parser State

### `fio_websocket_s`

```c
typedef struct fio_websocket_s {
  uint64_t frame_remaining;
  uint32_t mask;
  uint32_t frame_consumed;
  uint16_t close_code;
  uint8_t state;
  uint8_t flags;
  uint8_t flags2;
  uint8_t reserved;
} fio_websocket_s;
```

The parser state is intentionally small and is asserted to fit in 24 bytes.
Allocate it wherever the connection state lives, initialize it before use, and
prefer the public helper macros below instead of editing fields by hand.

```c
FIO_WEBSOCKET_GET_FIN(p)
FIO_WEBSOCKET_GET_MASKED(p)
FIO_WEBSOCKET_GET_OPCODE(p)
FIO_WEBSOCKET_GET_MSG_OPCODE(p)
FIO_WEBSOCKET_GET_PAUSED(p)
FIO_WEBSOCKET_GET_MSG_RSV(p)
```

`GET_OPCODE` reports the current frame opcode. `GET_MSG_OPCODE` reports the
open message opcode (`FIO_WEBSOCKET_OP_TEXT`, `FIO_WEBSOCKET_OP_BINARY`, or
`0` when no message is open). `GET_MSG_RSV` reports the RSV bits from the
opening data frame in the same 3-bit format used by the writer API.

### Lifecycle

```c
FIO_IFUNC void fio_websocket_init(fio_websocket_s *p);
FIO_IFUNC void fio_websocket_reset(fio_websocket_s *p);
```

Both helpers clear the struct and set the parser to the header-reading state.
After a protocol error or after receiving a close frame, discard or reset the
parser before reusing it.

---

## Events

### `fio_websocket_event_s`

```c
typedef struct {
  uint8_t type;
  uint8_t opcode;
  uint8_t is_text;
  uint8_t rsv;
  uint8_t is_first;
  uint8_t is_last;
  fio_buf_info_s payload;
  uint16_t close_code;
} fio_websocket_event_s;
```

`payload` points into the input buffer passed to `fio_websocket_parse`. If the
incoming frame is masked, that memory is unmasked in-place before the event is
returned. Keep the input bytes writable and alive until the event payload is no
longer needed.

### Event Types

```c
FIO_WEBSOCKET_EV_NONE
FIO_WEBSOCKET_EV_DATA_CHUNK
FIO_WEBSOCKET_EV_CONTROL
FIO_WEBSOCKET_EV_MESSAGE_END
FIO_WEBSOCKET_EV_ERROR
```

The current parser reports message data with `FIO_WEBSOCKET_EV_DATA_CHUNK` and
marks message boundaries with `is_first` and `is_last`. Control frames are
reported whole with `FIO_WEBSOCKET_EV_CONTROL`. Protocol errors are reported as
`FIO_WEBSOCKET_EV_ERROR` when an event pointer is supplied.

`FIO_WEBSOCKET_EV_MESSAGE_END` is defined for API completeness, but the current
parse loop uses the `is_last` flag on data events instead of emitting a separate
message-end event.

### Event Fields

For data events:

- `opcode` is the current frame opcode. Continuation frames report
  `FIO_WEBSOCKET_OP_CONT`.
- `is_text` is true when the open message started as a text message.
- `rsv` is copied from the opening data frame.
- `is_first` is true for the first chunk of the opening data frame.
- `is_last` is true for the final chunk of the message.
- `payload` is the available payload chunk, possibly length `0`.

For control events:

- `opcode` is `FIO_WEBSOCKET_OP_CLOSE`, `FIO_WEBSOCKET_OP_PING`, or
  `FIO_WEBSOCKET_OP_PONG`.
- `payload` is the complete control payload, never a partial control frame.
- close frames set `close_code` to the on-wire close code, or
  `FIO_WEBSOCKET_CLOSE_NO_STATUS` for an empty close payload.

---

## Parsing API

```c
FIO_SFUNC size_t fio_websocket_parse(fio_websocket_s *p,
                                     fio_buf_info_s buf,
                                     fio_websocket_event_s *ev);
```

Parses WebSocket bytes from `buf` and returns the number of bytes consumed, or
`FIO_WEBSOCKET_PARSE_ERROR` (`(size_t)-1`) on protocol error.

A successful call may consume fewer bytes than supplied. Feed the remaining
bytes to the next call, usually in a loop. If more bytes are needed before an
event can be produced, the function returns the bytes consumed so far, which can
be `0`, and leaves `ev->type` as `FIO_WEBSOCKET_EV_NONE`.

The parser is pure state plus the caller-provided buffer:

- no heap allocation;
- no callbacks;
- no retained payload pointers;
- no internal message accumulator;
- no control-frame buffering beyond waiting until a complete control payload is
  available in the supplied input.

`ev` may be `NULL` if the caller only wants to advance or validate input, but
then event details and close/error codes must be read from parser state where
available.

### Parser Flow

```text
header
  ├─ incomplete header       -> wait for more bytes
  ├─ control frame           -> wait for full payload -> CONTROL event
  └─ data / continuation     -> unmask available bytes -> DATA_CHUNK event
                                  └─ is_last marks message completion
```

The parser returns after at most one event. For fragmented messages, each data
frame may produce one or more data chunks depending on the input buffer splits.
Control frames may appear between fragmented data frames and are reported as
control events without closing the open message.

### Protocol Checks Performed

The parser rejects:

- unknown opcodes;
- fragmented control frames;
- continuation frames without an open message;
- nested text/binary messages while another data message is open;
- control payloads larger than 125 bytes;
- 64-bit lengths with the high bit set;
- frames larger than `FIO_WEBSOCKET_DEFAULT_MAX_FRAME`;
- close frames with a 1-byte payload;
- close frames carrying invalid on-wire close codes.

On rejection, the parser state becomes `FIO_WEBSOCKET_STATE_ERROR`,
`p->close_code` is set, and `fio_websocket_parse` returns
`FIO_WEBSOCKET_PARSE_ERROR`.

Caller policy still includes:

- enforcing client/server masking rules;
- interpreting RSV bits and applying extension transforms such as
  `permessage-deflate`;
- enforcing total message size limits;
- validating UTF-8 for text messages;
- accumulating message chunks if whole-message delivery is desired.

---

## Frame Writers

Server writers produce unmasked frames. Client writers produce masked frames;
passing `mask == 0` asks the writer to generate a non-zero PRNG mask.

### Sizing

```c
FIO_IFUNC uint64_t fio_websocket_write_len(uint64_t payload_len, _Bool masked);
```

Returns the number of bytes required for one complete frame with the requested
payload length and masking mode. Allocate at least this many bytes before
calling a writer.

### Data Messages

```c
FIO_IFUNC uint64_t fio_websocket_write_message_server(void *target,
                                                      fio_buf_info_s msg,
                                                      _Bool is_text,
                                                      uint8_t rsv);
FIO_IFUNC uint64_t fio_websocket_write_message_client(void *target,
                                                      fio_buf_info_s msg,
                                                      _Bool is_text,
                                                      uint32_t mask,
                                                      uint8_t rsv);
```

Writes one complete FIN data message to `target` and returns the bytes written.
`is_text` selects text (`1`) or binary (`0`). `rsv` is the 3-bit RSV value;
usually pass `0`, or `FIO_WEBSOCKET_RSV1` for a compressed
`permessage-deflate` message after applying the extension transform.

### Ping / Pong

```c
FIO_IFUNC uint64_t fio_websocket_write_ping_server(void *target,
                                                   fio_buf_info_s payload);
FIO_IFUNC uint64_t fio_websocket_write_ping_client(void *target,
                                                   fio_buf_info_s payload,
                                                   uint32_t mask);
FIO_IFUNC uint64_t fio_websocket_write_pong_server(void *target,
                                                   fio_buf_info_s payload);
FIO_IFUNC uint64_t fio_websocket_write_pong_client(void *target,
                                                   fio_buf_info_s payload,
                                                   uint32_t mask);
```

Writes one FIN control frame. Keep ping and pong payloads at 125 bytes or less;
the writer does not add a separate policy check for that RFC limit.

### Close

```c
FIO_IFUNC uint64_t fio_websocket_write_close_server(void *target,
                                                    uint16_t code,
                                                    fio_buf_info_s reason);
FIO_IFUNC uint64_t fio_websocket_write_close_client(void *target,
                                                    uint16_t code,
                                                    fio_buf_info_s reason,
                                                    uint32_t mask);
```

Writes a close frame containing the 2-byte close code followed by the optional
reason. The reason is truncated so the whole close payload fits in the 125-byte
control-frame limit. Choose a close code that is valid to send on the wire.

---

## Constants

### Opcodes

```c
FIO_WEBSOCKET_OP_CONT
FIO_WEBSOCKET_OP_TEXT
FIO_WEBSOCKET_OP_BINARY
FIO_WEBSOCKET_OP_CLOSE
FIO_WEBSOCKET_OP_PING
FIO_WEBSOCKET_OP_PONG
```

### Close Codes

```c
FIO_WEBSOCKET_CLOSE_OK
FIO_WEBSOCKET_CLOSE_GOING_AWAY
FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR
FIO_WEBSOCKET_CLOSE_UNSUPPORTED_DATA
FIO_WEBSOCKET_CLOSE_NO_STATUS
FIO_WEBSOCKET_CLOSE_INVALID_PAYLOAD
FIO_WEBSOCKET_CLOSE_POLICY_VIOLATION
FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG
FIO_WEBSOCKET_CLOSE_MANDATORY_EXT
FIO_WEBSOCKET_CLOSE_INTERNAL_ERROR
```

`FIO_WEBSOCKET_CLOSE_NO_STATUS` is synthesized for an empty received close
payload and must not be sent as an on-wire status code. The parser also accepts
valid registered wire codes such as 1012-1014 and application/library codes in
the 3000-4999 range.

### RSV Bits

```c
FIO_WEBSOCKET_RSV1
FIO_WEBSOCKET_RSV2
FIO_WEBSOCKET_RSV3
```

These are 3-bit values for the writer API and event `rsv` field. The writer
shifts them into byte-0 bits 4..6 on the wire.

### Parser States and Limits

```c
FIO_WEBSOCKET_STATE_HEADER
FIO_WEBSOCKET_STATE_PAYLOAD
FIO_WEBSOCKET_STATE_CLOSED
FIO_WEBSOCKET_STATE_ERROR

FIO_WEBSOCKET_DEFAULT_MAX_FRAME
FIO_WEBSOCKET_PARSE_ERROR
```

`FIO_WEBSOCKET_DEFAULT_MAX_FRAME` defaults to 1 GiB and may be overridden before
including the header. `FIO_WEBSOCKET_PARSE_ERROR` is the parse error sentinel.

The header also exposes flag bit masks used by the accessor macros:
`FIO_WEBSOCKET_FLAG_FIN`, `FIO_WEBSOCKET_FLAG_MASKED`,
`FIO_WEBSOCKET_FLAG_OPCODE_MASK`, `FIO_WEBSOCKET_FLAG_OPCODE_SHIFT`,
`FIO_WEBSOCKET_FLAG_MSG_OPCODE_MASK`, `FIO_WEBSOCKET_FLAG2_PAUSED`,
`FIO_WEBSOCKET_FLAG2_MSG_RSV_MASK`, and
`FIO_WEBSOCKET_FLAG2_MSG_RSV_SHIFT`.

---

## Minimal Parse Loop

```c
#define FIO_WEBSOCKET_PARSER
#include FIO_INCLUDE_FILE

typedef struct {
  fio_websocket_s ws;
} connection_s;

void websocket_consume(connection_s *c, char *data, size_t len) {
  fio_buf_info_s buf = FIO_BUF_INFO2(data, len);

  while (buf.len) {
    fio_websocket_event_s ev;
    size_t n = fio_websocket_parse(&c->ws, buf, &ev);

    if (n == FIO_WEBSOCKET_PARSE_ERROR) {
      /* Send/record c->ws.close_code and close the connection. */
      return;
    }

    buf.buf += n;
    buf.len -= n;

    if (!n && ev.type == FIO_WEBSOCKET_EV_NONE)
      break; /* wait for more network bytes */

    switch (ev.type) {
    case FIO_WEBSOCKET_EV_DATA_CHUNK:
      /* ev.payload is already unmasked and points into data. */
      if (ev.is_first) {
        /* start a message accumulator, if needed */
      }
      if (ev.payload.len) {
        /* copy/process this chunk before reusing data */
      }
      if (ev.is_last) {
        /* finish the message */
      }
      break;

    case FIO_WEBSOCKET_EV_CONTROL:
      if (ev.opcode == FIO_WEBSOCKET_OP_PING) {
        char out[128 + 14];
        uint64_t written = fio_websocket_write_pong_server(out, ev.payload);
        (void)written; /* write out to the socket */
      } else if (ev.opcode == FIO_WEBSOCKET_OP_CLOSE) {
        /* Echo/close according to local policy. */
        return;
      }
      break;

    default:
      break;
    }
  }
}

void websocket_open(connection_s *c) {
  fio_websocket_init(&c->ws);
}
```

For real IO, preserve any unconsumed bytes and retry when more data arrives.
Copy payload bytes before recycling the read buffer or applying asynchronous
message handling.

---

## Minimal Write Example

```c
char out[14 + 1024];
fio_buf_info_s msg = FIO_BUF_INFO2("hello", 5);

uint64_t len = fio_websocket_write_message_server(out, msg, 1, 0);
/* write `len` bytes from out */
```

For client frames:

```c
uint64_t len = fio_websocket_write_message_client(out, msg, 1, 0, 0);
```

The fourth argument is an explicit mask; `0` lets the writer choose one.

---

## Ownership and Lifetime

- The parser owns only `fio_websocket_s` state supplied by the caller.
- The parser never allocates, frees, stores callbacks, or retains payload
  pointers after returning.
- Input buffers passed to `fio_websocket_parse` must be writable because masked
  payloads are modified in-place.
- Event payload slices are valid only while the input buffer remains valid and
  unchanged.
- Writer targets must be large enough for `fio_websocket_write_len(payload.len,
  masked)` bytes.
- Independent parser instances can be used concurrently by different threads;
  synchronize shared buffers and connection state in the caller.
