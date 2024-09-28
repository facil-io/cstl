## Custom JSON Parser

```c
#define FIO_JSON
#include "fio-stl.h"
```

The facil.io JSON parser is a non-strict parser, with support for trailing commas in collections, new-lines in strings, extended escape characters, comments, and common numeral formats (octal, hex and binary).

The facil.io JSON parser should be considered **unsafe** as overflow protection depends on the `NUL` character appearing at the end of the string passed to the parser.

**Note:** this module depends on the `FIO_ATOL` module which will be automatically included.

#### `FIO_JSON_MAX_DEPTH`

```c
#ifndef FIO_JSON_MAX_DEPTH
/** Maximum allowed JSON nesting level. Values above 64K might fail. */
#define FIO_JSON_MAX_DEPTH 512
#endif
```

To ensure the program's stack isn't abused, the parser will limit JSON nesting levels to a customizable `FIO_JSON_MAX_DEPTH` number of nesting levels.

### JSON parser API

#### `fio_json_parser_callbacks_s`

```c
typedef struct {
  /** NULL object was detected. Returns new object as `void *`. */
  void *(*get_null)(void);
  /** TRUE object was detected. Returns new object as `void *`. */
  void *(*get_true)(void);
  /** FALSE object was detected. Returns new object as `void *`. */
  void *(*get_false)(void);
  /** Number was detected (long long). Returns new object as `void *`. */
  void *(*get_number)(int64_t i);
  /** Float was detected (double).Returns new object as `void *`.  */
  void *(*get_float)(double f);
  /** String was detected (int / float). update `pos` to point at ending */
  void *(*get_string)(const void *start, size_t len);
  /** Dictionary was detected. Returns ctx to hash map or NULL on error. */
  void *(*get_map)(void *ctx, void *at);
  /** Array was detected. Returns ctx to array or NULL on error. */
  void *(*get_array)(void *ctx, void *at);
  /** Array was detected. Returns non-zero on error. */
  int (*map_push)(void *ctx, void *key, void *value);
  /** Array was detected. Returns non-zero on error. */
  int (*array_push)(void *ctx, void *value);
  /** Called for the `key` element in case of error or NULL value. */
  void (*free_unused_object)(void *ctx);
  /** the JSON parsing encountered an error - what to do with ctx? */
  void *(*on_error)(void *ctx);
} fio_json_parser_callbacks_s;
```

The JSON parser requires certain callbacks to create objects or perform actions based on JSON data.

The following callbacks MUST be provided to the parser:

 - `void *(*get_null)(void)` - `NULL` object was detected. Returns NULL object as `void *`.

 - `void *(*get_true)(void)` - `true` object was detected. Returns TRUE object as `void *`.

 - `void *(*get_false)(void)` - `false` object was detected. Returns FALSE object as `void *`.

 - `void *(*get_number)(int64_t i)` - Number was detected (`int64_t`). Returns new number object as `void *`.

 - `void *(*get_float)(double f)` - Float was detected (`double`). Returns new float object as `void *`. 

 - `void *(*get_string)(const void *start, size_t len)` - String was detected. `start` points to a JSON escaped String (remember to unescape). Returns a new String as `void *`.

 - `void *(*get_map)(void *ctx, void *at)` - Dictionary was detected. Returns new `ctx` to hash map or `NULL` on error.

 - `void *(*get_array)(void *ctx, void *at)` - Array was detected. Returns new `ctx` to array or `NULL` on error.

 - `int (*map_push)(void *ctx, void *key, void *value)` - Pushes data to Array. Returns non-zero on error.

 - `int (*array_push)(void *ctx, void *value)` - Pushes data to Dictionary. Returns non-zero on error.

 - `void (*free_unused_object)(void *ctx)` - Called for the `key` element in case of error that caused `key` to be unused.

 - `void *(*on_error)(void *ctx)` - the JSON parsing encountered an error - what to do with ctx?


#### `fio_json_parse`

```c
/** The JSON return type. */
typedef struct {
  void *ctx;
  size_t stop_pos;
  int err;
} fio_json_result_s;

fio_json_result_s fio_json_parse(fio_json_parser_callbacks_s *callbacks,
                                 const char *json_string,
                                 const size_t len);
```


The facil.io JSON parser is a non-strict parser, with support for trailing commas in collections, new-lines in strings, extended escape characters and octal, hex and binary numbers.

The parser allows for streaming data and decouples the parsing process from the resulting data-structure by calling the requested callbacks for JSON related events.

Returns the result object which details the number of bytes consumed (stop position index `stop_pos`), if the parsing stopped due to an error (`err`) and the top most context / object in the JSON stream.

#### `fio_json_parse_update`

```c
fio_json_result_s fio_json_parse_update(fio_json_parser_callbacks_s *s,
                                        void *ctx,
                                        const char *start,
                                        const size_t len);
```

Use only when `ctx` is an object and JSON data is wrapped in an object (of the same type).

i.e., update an array or hash map.

### JSON Parsing Example - a JSON minifier

The biggest question about parsing JSON is - where do we store the resulting data?

Different parsers solve this question in different ways.

The `FIOBJ` soft-typed object system offers a very effective solution for data manipulation, as it creates a separate object for every JSON element.

However, many parsers store the result in an internal data structure that can't be separated into different elements. These parser appear faster while actually deferring a lot of the heavy lifting to a later stage.

Here is a short example that parses the data and writes it to a new minifed (compact) JSON String result.

```c
#define FIO_JSON
#define FIO_STR_NAME fio_str
#define FIO_LOG
#include "fio-stl.h"

#define FIO_CLI
#include "fio-stl.h"

typedef struct {
  fio_json_parser_s p;
  fio_str_s out;
  uint8_t counter;
  uint8_t done;
} my_json_parser_s;

#define JSON_PARSER_CAST(ptr) FIO_PTR_FROM_FIELD(my_json_parser_s, p, ptr)
#define JSON_PARSER2OUTPUT(p) (&JSON_PARSER_CAST(p)->out)

FIO_IFUNC void my_json_write_seperator(fio_json_parser_s *p) {
  my_json_parser_s *j = JSON_PARSER_CAST(p);
  if (j->counter) {
    switch (fio_json_parser_is_in_object(p)) {
    case 0: /* array */
      if (fio_json_parser_is_in_array(p))
        fio_str_write(&j->out, ",", 1);
      break;
    case 1: /* object */
      // note the reverse `if` statement due to operation ordering
      fio_str_write(&j->out, (fio_json_parser_is_key(p) ? "," : ":"), 1);
      break;
    }
  }
  j->counter |= 1;
}

/** a NULL object was detected */
FIO_JSON_CB void fio_json_on_null(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "null", 4);
}
/** a TRUE object was detected */
static inline void fio_json_on_true(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "true", 4);
}
/** a FALSE object was detected */
FIO_JSON_CB void fio_json_on_false(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "false", 4);
}
/** a Number was detected (long long). */
FIO_JSON_CB void fio_json_on_number(fio_json_parser_s *p, long long i) {
  my_json_write_seperator(p);
  fio_str_write_i(JSON_PARSER2OUTPUT(p), i);
}
/** a Float was detected (double). */
FIO_JSON_CB void fio_json_on_float(fio_json_parser_s *p, double f) {
  my_json_write_seperator(p);
  char buffer[256];
  size_t len = fio_ftoa(buffer, f, 10);
  fio_str_write(JSON_PARSER2OUTPUT(p), buffer, len);
}
/** a String was detected (int / float). update `pos` to point at ending */
FIO_JSON_CB void
fio_json_on_string(fio_json_parser_s *p, const void *start, size_t len) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "\"", 1);
  fio_str_write(JSON_PARSER2OUTPUT(p), start, len);
  fio_str_write(JSON_PARSER2OUTPUT(p), "\"", 1);
}
/** a dictionary object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_object(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "{", 1);
  JSON_PARSER_CAST(p)->counter = 0;
  return 0;
}
/** a dictionary object closure detected */
FIO_JSON_CB void fio_json_on_end_object(fio_json_parser_s *p) {
  fio_str_write(JSON_PARSER2OUTPUT(p), "}", 1);
  JSON_PARSER_CAST(p)->counter = 1;
}
/** an array object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_array(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "[", 1);
  JSON_PARSER_CAST(p)->counter = 0;
  return 0;
}
/** an array closure was detected */
FIO_JSON_CB void fio_json_on_end_array(fio_json_parser_s *p) {
  fio_str_write(JSON_PARSER2OUTPUT(p), "]", 1);
  JSON_PARSER_CAST(p)->counter = 1;
}
/** the JSON parsing is complete */
FIO_JSON_CB void fio_json_on_json(fio_json_parser_s *p) {
  JSON_PARSER_CAST(p)->done = 1;
  (void)p;
}
/** the JSON parsing encountered an error */
FIO_JSON_CB void fio_json_on_error(fio_json_parser_s *p) {
  fio_str_write(
      JSON_PARSER2OUTPUT(p), "--- ERROR, invalid JSON after this point.\0", 42);
}

void run_my_json_minifier(char *json, size_t len) {
  my_json_parser_s p = {{0}};
  fio_json_parse(&p.p, json, len);
  if (!p.done)
    FIO_LOG_WARNING(
        "JSON parsing was incomplete, minification output is partial");
  fprintf(stderr, "%s\n", fio_str2ptr(&p.out));
  fio_str_destroy(&p.out);
}
```

-------------------------------------------------------------------------------
