## URL-Encoded Parser

```c
#define FIO_URL_ENCODED
#include FIO_INCLUDE_FILE
```

By defining `FIO_URL_ENCODED`, a non-allocating, callback-based URL-encoded (`application/x-www-form-urlencoded`) parser is defined and made available.

The parser finds boundaries between `name=value` pairs without decoding the data. Decoding is the caller's responsibility (use `fio_string_write_url_dec`).

URL-encoded format:
- Pairs separated by `&`
- Name and value separated by `=`
- Special characters are percent-encoded (`%XX`)

### URL-Encoded Parser Types

#### `fio_url_encoded_parser_callbacks_s`

```c
typedef struct {
  void *(*on_pair)(void *udata, fio_buf_info_s name, fio_buf_info_s value);
  void (*on_error)(void *udata);
} fio_url_encoded_parser_callbacks_s;
```

The URL-encoded parser callbacks. Callbacks receive `udata` as their first argument.

**Members:**
- `on_pair` - called for each `name=value` pair found. `name` and `value` point directly into the original input data (NOT decoded). Returns the (possibly updated) `udata`, or NULL to stop parsing.
- `on_error` - called on parsing error (optional). Currently reserved for future use since URL-encoded parsing is very permissive.

**Note**: this struct should typically be declared as `static const`.

#### `fio_url_encoded_result_s`

```c
typedef struct {
  size_t consumed; /* Number of bytes consumed from the buffer. */
  size_t count;    /* Number of name=value pairs found. */
  int err;         /* Non-zero if an error occurred (callback returned NULL). */
} fio_url_encoded_result_s;
```

The URL-encoded parse result type.

**Members:**
- `consumed` - number of bytes consumed from the input buffer
- `count` - number of `name=value` pairs found
- `err` - non-zero if parsing was stopped early (the `on_pair` callback returned NULL)

### URL-Encoded Parser API

#### `fio_url_encoded_parse`

```c
fio_url_encoded_result_s
fio_url_encoded_parse(const fio_url_encoded_parser_callbacks_s *callbacks,
                      void *udata,
                      const char *data,
                      size_t len);
```

Parses URL-encoded data from a buffer.

Iterates over the input, splitting it into `name=value` pairs delimited by `&`, and invokes the `on_pair` callback for each pair found. The parser does NOT decode percent-encoded characters.

**Parameters:**
- `callbacks` - pointer to the callback struct (should be `static const`)
- `udata` - user data passed to callbacks; also receives the return value of `on_pair`
- `data` - pointer to the URL-encoded data to parse
- `len` - length of the data in bytes

**Returns:** a `fio_url_encoded_result_s` containing:
- `consumed` - number of bytes consumed from the buffer
- `count` - number of `name=value` pairs found
- `err` - non-zero if parsing was stopped (callback returned NULL)

Parsing rules:
- Pairs are separated by `&`
- Name and value are separated by `=`
- Empty value is valid: `name=` produces `value.len = 0`
- Missing `=` means value is empty: `name` produces `name="name"`, `value.len = 0`
- Empty name with value: `=value` produces `name.len = 0`, `value="value"`
- Empty pairs (`&&`) are skipped

**Note**: the parser does NOT decode percent-encoded characters. Use `fio_string_write_url_dec` to decode name and value buffers if needed.

### URL-Encoded Parser Examples

#### Basic Parsing

```c
#define FIO_URL_ENCODED
#include FIO_INCLUDE_FILE

static void *my_on_pair(void *udata,
                        fio_buf_info_s name,
                        fio_buf_info_s value) {
  printf("  %.*s = %.*s\n",
         (int)name.len, name.buf,
         (int)value.len, value.buf);
  return udata;
}

void example_basic(void) {
  static const fio_url_encoded_parser_callbacks_s callbacks = {
    .on_pair = my_on_pair,
  };

  const char *data = "foo=bar&baz=42&key=hello%20world";
  size_t len = strlen(data);

  fio_url_encoded_result_s result =
      fio_url_encoded_parse(&callbacks, (void *)1, data, len);

  printf("Consumed: %zu bytes, Pairs: %zu, Error: %d\n",
         result.consumed, result.count, result.err);
  /* Output:
   *   foo = bar
   *   baz = 42
   *   key = hello%20world
   * Consumed: 31 bytes, Pairs: 3, Error: 0
   */
}
```

#### Early Termination

```c
static void *stop_after_two(void *udata,
                            fio_buf_info_s name,
                            fio_buf_info_s value) {
  size_t *count = (size_t *)udata;
  ++(*count);
  if (*count >= 2)
    return NULL; /* Stop parsing */
  return udata;
  (void)name;
  (void)value;
}

void example_early_stop(void) {
  static const fio_url_encoded_parser_callbacks_s callbacks = {
    .on_pair = stop_after_two,
  };

  const char *data = "a=1&b=2&c=3&d=4";
  size_t count = 0;

  fio_url_encoded_result_s result =
      fio_url_encoded_parse(&callbacks, &count, data, strlen(data));

  /* result.count == 2, result.err == 1 */
}
```

#### Edge Cases

```c
void example_edges(void) {
  static const fio_url_encoded_parser_callbacks_s callbacks = {
    .on_pair = my_on_pair,
  };

  /* Missing '=' - entire segment is the name */
  fio_url_encoded_parse(&callbacks, (void *)1, "justname", 8);
  /* on_pair: name="justname", value="" */

  /* Empty value */
  fio_url_encoded_parse(&callbacks, (void *)1, "key=", 4);
  /* on_pair: name="key", value="" */

  /* Empty name with value */
  fio_url_encoded_parse(&callbacks, (void *)1, "=value", 6);
  /* on_pair: name="", value="value" */

  /* Empty pairs are skipped */
  fio_url_encoded_parse(&callbacks, (void *)1, "a=1&&b=2", 8);
  /* on_pair called twice: "a"="1" and "b"="2" */
}
```

-------------------------------------------------------------------------------
