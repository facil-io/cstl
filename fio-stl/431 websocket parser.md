## WebSocket Parser

```c
#define FIO_WEBSOCKET_PARSER
#include FIO_INCLUDE_FILE
```

By defining `FIO_WEBSOCKET_PARSER`, a WebSocket frame parser and formatter are defined and made available. This module implements the WebSocket protocol framing layer as specified in [RFC 6455](https://tools.ietf.org/html/rfc6455).

The module provides:

- **Parsing (unwrapping)** - a callback-driven streaming parser that consumes raw WebSocket frames, handles masking/unmasking, fragmentation, and dispatches complete messages to user-defined callbacks
- **Formatting (wrapping)** - functions to wrap payload data into properly framed WebSocket messages for both server-side (unmasked) and client-side (masked) connections
- **Control frame handling** - automatic dispatch of ping, pong, and close control frames
- **permessage-deflate** - support for the compression extension via a decompression callback (RSV1 flag)

The parser is designed as a set of static functions suitable for embedding directly in protocol implementations. It uses a state-machine approach with function-pointer dispatch for efficient incremental parsing.

### Configuration Macros

#### `FIO_WEBSOCKET_MAX_PAYLOAD`

```c
#ifndef FIO_WEBSOCKET_MAX_PAYLOAD
#define FIO_WEBSOCKET_MAX_PAYLOAD ((uint64_t)(1ULL << 30))
#endif
```

Maximum allowed WebSocket frame payload length. Defaults to 1 GB (`1 << 30`).

Override this macro before including the header to change the limit. Setting a lower value helps prevent denial-of-service attacks via memory exhaustion. Frames with a payload length exceeding this value cause a parser error.

#### `FIO_WEBSOCKET_PARSER_ERROR`

```c
#define FIO_WEBSOCKET_PARSER_ERROR ((size_t)-1)
```

The sentinel value returned by `fio_websocket_parse` on error. Equivalent to `(size_t)-1`.

### Types

#### `fio_websocket_parser_s`

```c
struct fio_websocket_parser_s {
  int (*fn)(fio_websocket_parser_s *, fio_buf_info_s *, void *);
  uint64_t start_at;
  uint64_t expect;
  uint32_t mask;
  uint8_t first;
  uint8_t current;
  uint8_t must_mask;
};
```

The WebSocket parser state machine context.

**Members:**
- `fn` - internal state function pointer (drives the parsing state machine)
- `start_at` - tracks the unmasking offset within a multi-frame message
- `expect` - number of payload bytes remaining in the current frame
- `mask` - the 32-bit masking key for the current frame (0 if unmasked)
- `first` - the first byte (opcode + flags) of the first frame in the current message
- `current` - the first byte (opcode + flags) of the current frame being parsed
- `must_mask` - set to 1 to require masking on all incoming frames (server mode); causes a parser error if an unmasked frame is received

**Note**: initialize this struct to zero before first use. The parser automatically sets up its internal state on the first call to `fio_websocket_parse`. Set `.must_mask = 1` for server-side parsers (RFC 6455 requires clients to mask all frames).

### Parsing API

#### `fio_websocket_parse`

```c
size_t fio_websocket_parse(fio_websocket_parser_s *p,
                           fio_buf_info_s buf,
                           void *udata);
```

Parses WebSocket data from `buf`, calling the appropriate callbacks as frames are consumed.

The parser is incremental: it can be called with partial data and will resume from where it left off on the next call. Internally it drives a state machine that consumes frame headers, payload data, handles unmasking, reassembles fragmented messages, and dispatches complete messages via callbacks.

**Parameters:**
- `p` - pointer to an initialized (zeroed) `fio_websocket_parser_s` context
- `buf` - the raw data buffer to parse (a `fio_buf_info_s` with `.buf` and `.len`)
- `udata` - opaque user data pointer passed through to all callbacks

**Returns:** the number of bytes consumed from `buf`, or `FIO_WEBSOCKET_PARSER_ERROR` (`(size_t)-1`) on protocol error.

**Note**: a return value less than `buf.len` means there is unconsumed data remaining (typically a partial frame). The caller should buffer the remainder and call again when more data arrives.

### Parsing Callbacks

These functions must be implemented by the user. The parser calls them during parsing to deliver events and request buffer management.

#### `fio_websocket_on_message`

```c
void fio_websocket_on_message(void *udata,
                              fio_buf_info_s msg,
                              unsigned char is_text);
```

Called when a complete data message (text or binary) has been fully received and reassembled.

**Parameters:**
- `udata` - the opaque user data pointer from `fio_websocket_parse`
- `msg` - the complete message payload (already unmasked and, if RSV1 was set, decompressed)
- `is_text` - `1` if the message is a UTF-8 text frame (opcode 0x1), `0` if binary (opcode 0x2)

#### `fio_websocket_write_partial`

```c
fio_buf_info_s fio_websocket_write_partial(void *udata,
                                           fio_buf_info_s partial,
                                           size_t more_expected);
```

Called when the parser needs to copy incoming frame payload to an external buffer. This callback is responsible for accumulating partial frame data into a contiguous message buffer.

The returned buffer **must** point to the accumulated message data, as the parser needs it for unmasking.

**Parameters:**
- `udata` - the opaque user data pointer from `fio_websocket_parse`
- `partial` - the partial payload data to append (`partial.len` may be 0)
- `more_expected` - the number of additional payload bytes expected in the current frame (0 when the frame is complete)

**Returns:** a `fio_buf_info_s` pointing to the full accumulated message buffer so far. Must return a valid buffer (non-NULL `.buf`); returning a buffer with `.buf == NULL` signals a protocol error and aborts parsing.

**Note**: when `more_expected` is 0, the current frame's payload is complete. The parser will then unmask the data in-place starting from the appropriate offset.

#### `fio_websocket_decompress`

```c
fio_buf_info_s fio_websocket_decompress(void *udata,
                                        fio_buf_info_s msg);
```

Called when the permessage-deflate extension requires decompression (RSV1 bit set on the first frame of the message).

**Parameters:**
- `udata` - the opaque user data pointer from `fio_websocket_parse`
- `msg` - the complete (unmasked) compressed message payload

**Returns:** a `fio_buf_info_s` pointing to the decompressed message data. Returning a buffer with `.buf == NULL` signals an error and aborts parsing.

#### `fio_websocket_on_protocol_ping`

```c
void fio_websocket_on_protocol_ping(void *udata, fio_buf_info_s msg);
```

Called when a WebSocket ping control frame (opcode 0x9) is received.

**Parameters:**
- `udata` - the opaque user data pointer from `fio_websocket_parse`
- `msg` - the ping payload (may be empty; up to 125 bytes per RFC 6455)

**Note**: per RFC 6455, the application should respond with a pong frame containing the same payload.

#### `fio_websocket_on_protocol_pong`

```c
void fio_websocket_on_protocol_pong(void *udata, fio_buf_info_s msg);
```

Called when a WebSocket pong control frame (opcode 0xA) is received.

**Parameters:**
- `udata` - the opaque user data pointer from `fio_websocket_parse`
- `msg` - the pong payload (may be empty)

#### `fio_websocket_on_protocol_close`

```c
void fio_websocket_on_protocol_close(void *udata, fio_buf_info_s msg);
```

Called when a WebSocket close control frame (opcode 0x8) is received.

**Parameters:**
- `udata` - the opaque user data pointer from `fio_websocket_parse`
- `msg` - the close payload. If non-empty, the first 2 bytes contain the close status code (big-endian) and the remainder is an optional UTF-8 reason string.

**Note**: per RFC 6455, the application should respond with a close frame and then close the connection.

### Formatting API

#### `fio_websocket_wrapped_len`

```c
uint64_t fio_websocket_wrapped_len(uint64_t len);
```

Returns the number of bytes required for the WebSocket frame header plus payload for a server (unmasked) message of `len` bytes.

The header size varies depending on the payload length:
- 0-125 bytes: 2-byte header
- 126-65535 bytes: 4-byte header
- 65536+ bytes: 10-byte header

**Parameters:**
- `len` - the payload length in bytes

**Returns:** the total framed message size (header + payload) for an unmasked (server) frame.

**Note**: for client (masked) frames, add 4 to the returned value to account for the 32-bit masking key.

#### `fio_websocket_server_wrap`

```c
uint64_t fio_websocket_server_wrap(void *target,
                                   const void *msg,
                                   uint64_t len,
                                   unsigned char opcode,
                                   unsigned char first,
                                   unsigned char last,
                                   unsigned char rsv);
```

Wraps a WebSocket server message and writes the framed data to `target`. Server frames are unmasked per RFC 6455.

The `first` and `last` flags support message fragmentation. When sending a complete message in a single frame, set both to 1.

**Parameters:**
- `target` - destination buffer (must have capacity for at least `fio_websocket_wrapped_len(len)` bytes)
- `msg` - pointer to the payload data
- `len` - payload length in bytes
- `opcode` - the WebSocket opcode (see table below)
- `first` - set to 1 for the first (or only) frame of a message
- `last` - set to 1 for the last (or only) frame of a message (sets the FIN bit)
- `rsv` - reserved bits (3 bits); set bit 0 (value 1) for permessage-deflate compressed data

**Returns:** the number of bytes written to `target`. Always equal to `fio_websocket_wrapped_len(len)`.

**Opcode values (RFC 6455):**

| Opcode | Meaning |
|--------|---------|
| `0x0` | Continuation frame |
| `0x1` | Text frame (UTF-8) |
| `0x2` | Binary frame |
| `0x3`-`0x7` | Reserved (non-control) |
| `0x8` | Connection close |
| `0x9` | Ping |
| `0xA` | Pong |
| `0xB`-`0xF` | Reserved (control) |

#### `fio_websocket_client_wrap`

```c
uint64_t fio_websocket_client_wrap(void *target,
                                   const void *msg,
                                   uint64_t len,
                                   unsigned char opcode,
                                   unsigned char first,
                                   unsigned char last,
                                   unsigned char rsv);
```

Wraps a WebSocket client message and writes the framed data to `target`. Client frames are masked with a random 32-bit key per RFC 6455.

The masking key is generated using `fio_rand64()` and is guaranteed to be non-zero. Masking prevents proxy cache poisoning attacks and is required for all client-to-server frames.

The `first` and `last` flags support message fragmentation, identical to `fio_websocket_server_wrap`.

**Parameters:**
- `target` - destination buffer (must have capacity for at least `fio_websocket_wrapped_len(len) + 4` bytes)
- `msg` - pointer to the payload data
- `len` - payload length in bytes
- `opcode` - the WebSocket opcode (see opcode table in `fio_websocket_server_wrap`)
- `first` - set to 1 for the first (or only) frame of a message
- `last` - set to 1 for the last (or only) frame of a message (sets the FIN bit)
- `rsv` - reserved bits (3 bits); set bit 0 (value 1) for permessage-deflate compressed data

**Returns:** the number of bytes written to `target`. Always equal to `fio_websocket_wrapped_len(len) + 4`.

### Examples

#### Server: Sending a Text Message

```c
#define FIO_WEBSOCKET_PARSER
#include FIO_INCLUDE_FILE

void example_server_send(int fd) {
  const char *msg = "Hello, WebSocket!";
  uint64_t len = strlen(msg);
  uint64_t frame_len = fio_websocket_wrapped_len(len);

  uint8_t buf[128]; /* ensure sufficient capacity */
  fio_websocket_server_wrap(buf, msg, len,
                            1,  /* opcode: text */
                            1,  /* first frame */
                            1,  /* last frame (FIN) */
                            0); /* no RSV bits */
  /* write buf[0..frame_len-1] to the connection */
}
```

#### Client: Sending a Binary Message

```c
void example_client_send(int fd) {
  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  uint64_t len = sizeof(data);
  uint64_t frame_len = fio_websocket_wrapped_len(len) + 4;

  uint8_t buf[128];
  fio_websocket_client_wrap(buf, data, len,
                            2,  /* opcode: binary */
                            1,  /* first frame */
                            1,  /* last frame (FIN) */
                            0); /* no RSV bits */
  /* write buf[0..frame_len-1] to the connection */
}
```

#### Parsing Incoming Data

```c
/* Callback implementations (must be provided by the user) */
FIO_SFUNC void fio_websocket_on_message(void *udata,
                                        fio_buf_info_s msg,
                                        unsigned char is_text) {
  printf("Received %s message (%zu bytes): %.*s\n",
         is_text ? "text" : "binary",
         msg.len, (int)msg.len, msg.buf);
}

FIO_SFUNC fio_buf_info_s fio_websocket_write_partial(void *udata,
                                                     fio_buf_info_s partial,
                                                     size_t more_expected) {
  /* Simple example: accumulate into a dynamic buffer (pseudo-code) */
  my_buffer_s *b = (my_buffer_s *)udata;
  if (partial.len)
    my_buffer_append(b, partial.buf, partial.len);
  return FIO_BUF_INFO2(b->data, b->len);
}

FIO_SFUNC fio_buf_info_s fio_websocket_decompress(void *udata,
                                                  fio_buf_info_s msg) {
  /* Implement permessage-deflate decompression here */
  return msg; /* pass-through if compression not supported */
}

FIO_SFUNC void fio_websocket_on_protocol_ping(void *udata,
                                              fio_buf_info_s msg) {
  /* Respond with a pong frame containing the same payload */
}

FIO_SFUNC void fio_websocket_on_protocol_pong(void *udata,
                                              fio_buf_info_s msg) {
  /* Handle pong (e.g., update keep-alive timer) */
}

FIO_SFUNC void fio_websocket_on_protocol_close(void *udata,
                                               fio_buf_info_s msg) {
  /* Send a close frame in response, then close the connection */
}

void example_parse(void *raw_data, size_t raw_len) {
  static fio_websocket_parser_s parser = {0};
  parser.must_mask = 1; /* server-side: require client masking */

  fio_buf_info_s buf = FIO_BUF_INFO2((char *)raw_data, raw_len);
  my_buffer_s my_buf = {0};

  size_t consumed = fio_websocket_parse(&parser, buf, &my_buf);
  if (consumed == FIO_WEBSOCKET_PARSER_ERROR) {
    /* Protocol error - close the connection */
    return;
  }
  /* If consumed < raw_len, buffer the remainder for next read */
}
```

------------------------------------------------------------
