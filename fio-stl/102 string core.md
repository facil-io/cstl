## Binary Safe Core String Helpers

```c
#define FIO_STR
#include "fio-stl.h"
```

The following helpers are part of the String core library and they become available whenever [a String type was defined](#dynamic-strings) or when the `FIO_STR` is defined before any inclusion of the C STL header.

The main difference between using the Core String API directly and defining a String type is that String types provide a few additional optimizations, such as embedding short strings (embedded within the type data rather than allocated), optional reference counting and pointer tagging features.

**Note**: the `fio_string` functions might fail or truncate data if memory allocation fails. Test the returned value for failure (success returns `0`, failure returns `-1`).


**Note:** this module depends on the  `FIO_ATOL`,  `FIO_ATOMIC`, `FIO_RAND`, and `FIO_FILES` modules which will be automatically included.

### Core String Authorship

#### `fio_string_write`

```c
static inline int fio_string_write(fio_str_info_s *dest,
                               void (*reallocate)(fio_str_info_s *,
                                                  size_t new_capa),
                               const void *src,
                               size_t len);
```

Writes data to the end of the string in the `fio_string_s` struct, returning an updated `fio_string_s` struct.

The returned string is NUL terminated if edited.

* `dest` an `fio_string_s` struct containing the destination string.

* `reallocate` is a callback that attempts to reallocate more memory (i.e., using realloc) and returns an updated `fio_string_s` struct containing the updated capacity and buffer pointer (as well as the original length).

    On failure the original `fio_string_s` should be returned. if `reallocate` is NULL or fails, the data copied will be truncated.

* `src` is the data to be written to the end of `dest`.

* `len` is the length of the data to be written to the end of `dest`.

**Note**: this function performs only minimal checks and assumes that `dest` is fully valid - i.e., that `dest.capa >= dest.len`, that `dest.buf` is valid, etc'.

**Note**: `reallocate`, if called, will be called only once.

An example for a `reallocate` callback using the system's `realloc` function (or use `FIO_STRING_REALLOC` / `FIO_STRING_FREE`):

```c
fio_str_info_s fio_string_realloc_system(fio_str_info_s dest,
                                         size_t len) {
  /* must allocate at least len + 1 bytes. */
  const size_t new_capa = fio_string_capa4len(len);
  void *tmp = realloc(dest.buf, new_capa);
  if (!tmp)
    return dest;
  dest.capa = new_capa;
  dest.buf = (char *)tmp;
  return dest;
}
```

An example for using the function:

```c
void example(void) {
  char buf[32];
  fio_str_info_s str = FIO_STR_INFO3(buf, 0, 32);
  fio_string_write(&str, NULL, "The answer is: 0x", 17);
  str.len += fio_ltoa(str.buf + str.len, 42, 16);
  fio_string_write(&str, NULL, "!\n", 2);
  printf("%s", str.buf);
}
```

#### `fio_string_replace`

```c
int fio_string_replace(fio_str_info_s *dest,
                      void (*reallocate)(fio_str_info_s *,
                                         size_t new_capa),
                      intptr_t start_pos,
                      size_t overwrite_len,
                      const void *src,
                      size_t len);
```

Similar to `fio_string_write`, only replacing/inserting a sub-string in a specific location.

Negative `start_pos` values are calculated backwards, `-1` == end of String.

When `overwrite_len` is zero, the function will insert the data at `start_pos`, pushing existing data until after the inserted data.

If `overwrite_len` is non-zero, than `overwrite_len` bytes will be overwritten (or deleted).

If `len == 0` than `src` will be ignored and the data marked for replacement will be erased.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write2`

```c
int fio_string_write2(fio_str_info_s *restrict dest,
                      void (*reallocate)(fio_str_info_s *,
                                         size_t new_capa),
                      const fio_string_write_s sources[]);
/* Helper macro for fio_string_write2 */
#define fio_string_write2(dest, reallocate, ...)                               \
  fio_string_write2((dest),                                                    \
                    (reallocate),                                              \
                    (fio_string_write_s[]){__VA_ARGS__, {0}})
```

Writes a group of objects (strings, numbers, etc') to `dest`.

`dest` and `reallocate` are similar to `fio_string_write`.

**Note**: `reallocate`, if called, will be called only once.

`sources` is an array of `fio_string_write_s` structs, ending with a struct that's all set to 0. This array is usually populated using the following macros:

```c
/** Used to write raw string data to the string. */
#define FIO_STRING_WRITE_STR1(str_)                                            \
  ((fio_string_write_s){.klass = 1,                                            \
                        .info.str = {.len = strlen((str_)), .buf = (str_)}})
/** Used to write raw (possibly binary) string data to the string. */
#define FIO_STRING_WRITE_STR2(str_, len_)                                      \
  ((fio_string_write_s){.klass = 1, .info.str = {.len = (len_), .buf = (str_)}})
/** Used to write a signed number to the string. */
#define FIO_STRING_WRITE_NUM(num)                                              \
  ((fio_string_write_s){.klass = 2, .info.i = (int64_t)(num)})
/** Used to write an unsigned number to the string. */
#define FIO_STRING_WRITE_UNUM(num)                                             \
  ((fio_string_write_s){.klass = 3, .info.u = (uint64_t)(num)})
/** Used to write a hex representation of a number to the string. */
#define FIO_STRING_WRITE_HEX(num)                                              \
  ((fio_string_write_s){.klass = 4, .info.u = (uint64_t)(num)})
/** Used to write a binary representation of a number to the string. */
#define FIO_STRING_WRITE_BIN(num)                                              \
  ((fio_string_write_s){.klass = 5, .info.u = (uint64_t)(num)})
/** Used to write a double(!) to the string. */
#define FIO_STRING_WRITE_FLOAT(num)                                            \
  ((fio_string_write_s){.klass = 6, .info.f = (double)(num)})
```

Use the `fio_string_write2` macro for ease, i.e.:

```c
fio_str_info_s str = {0};
fio_string_write2(&str, my_reallocate,
                   FIO_STRING_WRITE_STR1("The answer is: "),
                   FIO_STRING_WRITE_NUM(42),
                   FIO_STRING_WRITE_STR2("(0x", 3),
                   FIO_STRING_WRITE_HEX(42),
                   FIO_STRING_WRITE_STR2(")", 1));
```

**Note**: this function might end up allocating more memory than absolutely required as it favors speed over memory savings.

For this function, the facil.io C STL reserves and defines the following type:

```c
/** Argument type used by fio_string_write2. */
typedef struct {
  size_t klass;
  union {
    fio_str_info_s str;
    double f;
    int64_t i;
    uint64_t u;
  } info;
} fio_string_write_s;
```

### Core String Numeral Helpers

#### `fio_string_write_i`

```c
static inline int fio_string_write_i(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 int64_t i);
```

Writes a signed number `i` to the String.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write_u`

```c
static inline int fio_string_write_u(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 uint64_t u);
```

Writes an unsigned number `u` to the String.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write_hex`

```c
static inline int fio_string_write_hex(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
                                   uint64_t i);
```

Writes a hex representation of `i` to the String.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write_bin`

```c
static inline int fio_string_write_bin(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
                                   uint64_t i);
```

Writes a binary representation of `i` to the String.

**Note**: `reallocate`, if called, will be called only once.

### Core String `printf` Helpers

#### `fio_string_printf`

```c
static int fio_string_printf(fio_str_info_s *dest,
                                void (*reallocate)(fio_str_info_s *,
                                                   size_t new_capa),
                                const char *format,
                                ...);
```

Similar to fio_string_write, only using printf semantics.

#### `fio_string_vprintf`

```c
inline int fio_string_vprintf(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 const char *format,
                                 va_list argv);
```

Similar to fio_string_write, only using vprintf semantics.

### Core String Authorship Memory Helpers

#### `fio_string_capa4len`

```c
size_t fio_string_capa4len(size_t new_len);
```

Calculates a 16 bytes boundary aligned capacity for `new_len`.

The Core String API always allocates 16 byte aligned memory blocks, since most memory allocators will only allocate memory in multiples of 16 or more. By requesting the full 16 byte allocation, future allocations could be avoided without increasing memory usage.

#### `FIO_STRING_REALLOC`

```c
#define FIO_STRING_REALLOC fio_string_default_reallocate
void fio_string_default_reallocate(fio_str_info_s *dest, size_t new_capa);
```

Default reallocation callback implementation

#### `FIO_STRING_ALLOC_COPY`

```c
#define FIO_STRING_ALLOC_COPY fio_string_default_copy_and_reallocate
void fio_string_default_copy_and_reallocate(fio_str_info_s *dest, size_t new_capa);
```

Default reallocation callback for memory that mustn't be freed.

#### `FIO_STRING_FREE`

```c
#define FIO_STRING_FREE fio_string_default_free
void fio_string_default_free(void *);
```

Frees memory that was allocated with the default callbacks.

#### `FIO_STRING_FREE2`

```c
#define FIO_STRING_FREE2 fio_string_default_free2
void fio_string_default_free2(fio_str_info_s str);
```

Frees memory that was allocated with the default callbacks.

#### `FIO_STRING_FREE_NOOP`

```c
#define FIO_STRING_FREE_NOOP fio_string_default_free_noop
void fio_string_default_free_noop(void * str);
```

Does nothing. Made available for APIs that require a callback for memory management.

#### `FIO_STRING_FREE_NOOP2`

```c
#define FIO_STRING_FREE_NOOP2 fio_string_default_free_noop2
void fio_string_default_free_noop2(fio_str_info_s str);
```

Does nothing. Made available for APIs that require a callback for memory management.

### Core String Comparison

In addition to [`FIO_STR_INFO_IS_EQ(a,b)`](#fio_str_info_is_eq) and [`FIO_BUF_INFO_IS_EQ(a,b)`](#fio_buf_info_is_eq) MACROs, the following comparisons helpers are available:

#### `fio_string_is_greater`

```c
int fio_string_is_greater(fio_str_info_s a, fio_str_info_s b);
```
Equivalent to: `memcmp(a.buf, b.buf, min(a.len, b.len)) > 0 || (!memcmp(a.buf, b.buf, min(a.len, b,len)) && a.len > b.len)`

Compares two strings, returning 1 if the data in string `a` is greater in value than the data in string `b`.

**Note**: returns 0 if string `b` is bigger than string `a` or if strings are equal, designed to be used with `FIO_SORT_IS_BIGGER(a,b)`.

**Note**: it is often faster to define `FIO_SORT_IS_BIGGER` using a `memcmp` wrapper, however the speed depends on the `clib` implementation and this function provides a good enough fallback that should be very portable.

#### `fio_string_is_greater_buf`

```c
int fio_string_is_greater_buf(fio_buf_info_s a, fio_buf_info_s b);
```

Equivalent to: `memcmp(a.buf, b.buf, min(a.len, b.len)) > 0 || (!memcmp(a.buf, b.buf, min(a.len, b,len)) && a.len > b.len)`

Compares two `fio_buf_info_s`, returning 1 if the data in buffer `a` is greater in value than the data in buffer `b`.

**Note**: returns 0 if data in `b` is greater than **or equal** to `a`, designed to be used with `FIO_SORT_IS_BIGGER(a,b)`.

**Note**: it is often faster to define `FIO_SORT_IS_BIGGER` using a `memcmp` wrapper, however the speed depends on the `clib` implementation and this function provides a good enough fallback that should be very portable.

### Core String UTF-8 Support

#### `fio_string_utf8_valid`

```c
size_t fio_string_utf8_valid(fio_str_info_s str);
```

Returns 1 if the String is UTF-8 valid and 0 if not.

#### `fio_string_utf8_len`

```c
size_t fio_string_utf8_len(fio_str_info_s str);
```

Returns the String's length in UTF-8 characters or 0 on either an error or an empty string.

#### `fio_string_utf8_select`

```c
int fio_string_utf8_select(fio_str_info_s str, intptr_t *pos, size_t *len);
```

Takes a UTF-8 character selection information (UTF-8 position and length) and updates the same variables so they reference the raw byte slice information.

If the String isn't UTF-8 valid up to the requested selection, than `pos` will be updated to `-1` otherwise values are always positive.

The returned `len` value may be shorter than the original if there wasn't enough data left to accommodate the requested length. When a `len` value of `0` is returned, this means that `pos` marks the end of the String.

Returns -1 on error and 0 on success.

### Core String C / JSON escaping

#### `fio_string_write_escape`

```c
int fio_string_write_escape(fio_str_info_s *restrict dest,
                            fio_string_realloc_fn reallocate,
                            const void *src,
                            size_t len);
```

Writes data at the end of the String, escaping the data using JSON semantics.

The JSON semantic are common to many programming languages, promising a UTF-8
String while making it easy to read and copy the string during debugging.

#### `fio_string_write_unescape`

```c
int fio_string_write_unescape(fio_str_info_s *dest,
                              fio_string_realloc_fn reallocate,
                              const void *src,
                              size_t len);
```

Writes an escaped data into the string after un-escaping the data.

### Core String Base64 support

#### `fio_string_write_base64enc`

```c
SFUNC int fio_string_write_base64enc(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *data,
                                     size_t data_len,
                                     uint8_t url_encoded);
```

Writes data to String using Base64 encoding.

#### `fio_string_write_base64dec`

```c
SFUNC int fio_string_write_base64dec(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *encoded,
                                     size_t encoded_len);
```

Writes decoded base64 data to String.

### Core String URL escaping support

#### `fio_string_write_url_enc`

```c
int fio_string_write_url_enc(fio_str_info_s *restrict dest,
                             fio_string_realloc_fn reallocate,
                             const void *raw,
                             size_t len);
```

Writes data to String using URL encoding (a.k.a., percent encoding). Always encodes spaces as `%20` rather than `+`.

#### `fio_string_write_url_dec`

```c
int fio_string_write_url_dec(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             const void *encoded,
                             size_t len);
```

Writes decoded URL data to String. Decodes "percent encoding" as well as spaces encoded using `+`.

**Note**: the decoding function reads the non-standard `"%uXXXX"` as UTF-8 encoded data.

#### `fio_string_write_path_dec`

```c
int fio_string_write_url_dec(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             const void *encoded,
                             size_t len);
```

Writes decoded URL data to String. Decodes "percent encoding" without converting `+` to spaces.

**Note**: the decoding function reads the non-standard `"%uXXXX"` as UTF-8 encoded data.

### Core String HTML escaping support

#### `fio_string_write_html_escape`

```c
int fio_string_write_html_escape(fio_str_info_s *restrict dest,
                                 fio_string_realloc_fn reallocate,
                                 const void *raw,
                                 size_t len);
```

Writes HTML escaped data to a String.

#### `fio_string_write_html_unescape`

```c
int fio_string_write_html_unescape(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *escaped,
                                   size_t len);
```

Writes HTML (mostly) un-escaped data to a String.

**Note**:

The un-escaping of HTML content includes a long list of named code-point. This list isn't handled here, instead only numerical and super-basic named code-points are supported.

The supported named code-points include a small group, among them: `&lt`, `&gt`, `&amp`, `&tab`, `&quot`, `&apos`, `&nbsp`, `&copy` (with or without a trailing `;`).

### Core String File Reading support

#### `fio_string_readfd`

```c
int fio_string_readfd(fio_str_info_s *dest,
                      fio_string_realloc_fn reallocate,
                      int fd,
                      intptr_t start_at,
                      intptr_t limit);
```

Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at`.

If `limit` is 0 (or less than 0) data will be written until EOF.

If `start_at` is negative, position will be calculated from the end of the file where `-1 == EOF`.

Note: this will fail unless used on actual files (not sockets, not pipes).

#### `fio_string_readfile`

```c
int fio_string_readfile(fio_str_info_s *dest,
                        fio_string_realloc_fn reallocate,
                        const char *filename,
                        intptr_t start_at,
                        intptr_t limit);
```

Opens the file `filename` and pastes it's contents (or a slice ot it) at the end of the String. If `limit == 0`, than the data will be read until EOF.

If the file can't be located, opened or read, or if `start_at` is beyond the EOF position, NULL is returned in the state's `data` field.


#### `fio_string_getdelim_fd`

```c
int fio_string_getdelim_fd(fio_str_info_s *dest,
                          fio_string_realloc_fn reallocate,
                          int fd,
                          intptr_t start_at,
                          char delim,
                          size_t limit);
```

Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at` and ending either at the first occurrence of `delim` or at EOF.

If `limit` is 0 (or less than 0) as much data as may be required will be written.

If `start_at` is negative, position will be calculated from the end of the file where `-1 == EOF`.

**Note**: this will fail unless used on actual seekable files (not sockets, not pipes).

#### `fio_string_getdelim_file`

```c
int fio_string_getdelim_file(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            const char *filename,
                            intptr_t start_at,
                            char delim,
                            size_t limit);
```

Opens the file `filename`, calls `fio_string_getdelim_fd` and closes the file.

-------------------------------------------------------------------------------

## C Strings with Binary Data

The facil.io C STL provides a very simple String library (`fio_bstr`) that wraps around the *Binary Safe Core String Helpers*, emulating (to some effect and degree) the behavior of the famous [Simple Dynamic Strings library](https://github.com/antirez/sds) while providing copy-on-write reference counting.

This String storage paradigm can be very effective and it is used as the default String key implementation in Maps when `FIO_MAP_KEY` is undefined.

To create a new String simply write to `NULL` and a new `char *` pointer will be returned, pointing to the first byte of the new string.

All `fio_bstr` functions that mutate the string return a pointer to the new string (**make sure to update the pointer!**).

The pointer should be freed using `fio_bstr_free`. i.e.:

```c
char * str = fio_bstr_write(NULL, "Hello World!", 12);
fprintf(stdout, "%s\n", str);
fio_bstr_free(str);
```

To copy a `fio_bstr` String use `fio_bstr_copy` - this uses a *copy-on-write* approach which can increase performance:

```c
char * str_org = fio_bstr_write(NULL, "Hello World", 11);
char * str_cpy = fio_bstr_copy(str_org);   /* str_cpy == str_org : only a reference count increase. */
str_cpy = fio_bstr_write(str_cpy, "!", 1); /* str_cpy != str_org : copy-on-write, data copied here. */
fprintf(stdout, "Original:    %s\nEdited Copy: %s\n", str_org, str_cpy);
fio_bstr_free(str_org);
fio_bstr_free(str_cpy);
```

The `fio_bstr` functions wrap all `fio_string` core API, resulting in the following available functions:

* `fio_bstr_write` - see [`fio_string_write`](#fio_string_write) for details.
* `fio_bstr_write2` (macro) - see [`fio_string_write2`](#fio_string_write2) for details.
* `fio_bstr_printf` - see [`fio_string_printf`](#fio_string_printf) for details.
* `fio_bstr_replace` - see [`fio_string_replace`](#fio_string_replace) for details.

* `fio_bstr_write_i` - see [`fio_string_write_i`](#fio_string_write_i) for details.
* `fio_bstr_write_u` - see [`fio_string_write_u`](#fio_string_write_u) for details.
* `fio_bstr_write_hex` - see [`fio_string_write_hex`](#fio_string_write_hex) for details.
* `fio_bstr_write_bin` - see [`fio_string_write_bin`](#fio_string_write_bin) for details.

* `fio_bstr_write_escape` - see [`fio_string_write_escape`](#fio_string_write_escape) for details.
* `fio_bstr_write_unescape` - see [`fio_string_write_unescape`](#fio_string_write_unescape) for details.

* `fio_bstr_write_base64enc` - see [`fio_string_write_base64enc`](#fio_string_write_base64enc) for details.
* `fio_bstr_write_base64dec` - see [`fio_string_write_base64dec`](#fio_string_write_base64dec) for details.

* `fio_bstr_write_html_escape` - see [`fio_string_write_html_escape`](#fio_string_write_html_escape) for details.
* `fio_bstr_write_html_unescape` - see [`fio_string_write_html_unescape`](#fio_string_write_html_unescape) for details.


* `fio_bstr_readfd` - see [`fio_string_readfd`](#fio_string_readfd) for details.
* `fio_bstr_readfile` - see [`fio_string_readfile`](#fio_string_readfile) for details.
* `fio_bstr_getdelim_fd` - see [`fio_string_getdelim_fd`](#fio_string_getdelim_fd) for details.
* `fio_bstr_getdelim_file` - see [`fio_string_getdelim_file`](#fio_string_getdelim_file) for details.

* `fio_bstr_is_greater` - see [`fio_string_is_greater`](#fio_string_is_greater) for details.

**Note**: the `fio_bstr` functions do not take a `reallocate` argument and their `dest` argument should be the existing `fio_bstr` pointer (`char *`).

**Note**: the `fio_bstr` functions might fail quietly if memory allocation fails. For better error handling use the `fio_bstr_info`, `fio_bstr_reallocate` and `fio_bstr_len_set` functions with the Core String API (the `.buf` in the `fio_str_info_s` struct is the `fio_bstr` pointer).

In addition, the following helpers are provided:

#### `fio_bstr_copy`

```c
char *fio_bstr_copy(char *bstr);
```

Returns a Copy-on-Write copy of the original `bstr`, increasing the original's reference count.

This approach to Copy-on-Write is **not** completely thread-safe, as a data race exists when an original string (without additional `copy`s) is being actively edited by another thread while `copy` is being called (in which case the still-in-process `write` will apply to the new copy, which may result in a re-allocation and the pointer `copy` is using being invalidated).

However, once the copy operation had completed in a thread-safe manner, further operations between the two instances do not need to be synchronized.

**Note**: This reference counter will automatically make a copy if more than 2 billion (2,147,483,648) references are counted.

**Note**: To avoid the Copy-on-Write logic, use:

```c
char *copy = fio_bstr_write(NULL, original, fio_bstr_len(original));
```

#### `fio_bstr_free`

```c
void fio_bstr_free(char *bstr);
```

Frees a binary string allocated by a `fio_bstr` function (or decreases its reference count).

#### `fio_bstr_info`

```c
fio_str_info_s fio_bstr_info(char *bstr);
```

Returns information about the `fio_bstr` using the `fio_str_info_s` struct.

#### `fio_bstr_buf`

```c
fio_buf_info_s fio_bstr_buf(char *bstr);
```

Returns information about the `fio_bstr` using the `fio_buf_info_s` struct.

#### `fio_bstr_len`

```c
size_t fio_bstr_len(char *bstr);
```

Gets the length of the `fio_bstr`.

#### `fio_bstr_len_set`

```c
char *fio_bstr_len_set(char *bstr, size_t len);
```

Sets the length of the `fio_bstr`.

**Note**: `len` **must** be less then the capacity of the `bstr`, or the function call will quietly fail.

Returns `bstr`.

#### `fio_bstr_reallocate` - for internal use

```c
int fio_bstr_reallocate(fio_str_info_s *dest, size_t len);
```

Default reallocation callback implementation. The new `fio_bstr` pointer will replace the old one in `dest->buf`.

-------------------------------------------------------------------------------

## Small Key Strings for Maps - Binary Safe

It is very common for Hash Maps to contain String keys. When the String keys are usually short, than it could be more efficient to embed the Key String data into the map itself (improve cache locality) rather than allocate memory for each separate Key String.

The `fio_keystr_s` type included with the String Core performs exactly this optimization. When the majority of the Strings are short (`len <= 14` on 64 bit machines or `len <= 10` on 32 bit machines) than the strings are stored inside the Map's memory rather than allocated separately.

See example at the end of this section. The example shows how to use the `fio_keystr_s` type and the `FIO_MAP_KEY_KSTR` MACRO.

#### `FIO_MAP_KEY_STR`

A helper macro for defining Key String keys in Hash Maps.

#### `fio_keystr_s`

```c
typedef struct fio_keystr_s fio_keystr_s;
```

a semi-opaque type used for the `fio_keystr` functions

#### `fio_keystr_buf`

```c
fio_buf_info_s fio_keystr_buf(fio_keystr_s *str);
```

Returns the Key String.

#### `fio_keystr`

```c
fio_keystr_s fio_keystr(const char *buf, uint32_t len);
```

Returns a **temporary** `fio_keystr_s` to be used as a key for a hash map.

Do **not** `fio_keystr_destroy` this key.

#### `fio_keystr_copy`

```c
fio_keystr_s fio_keystr_copy(fio_str_info_s str, void *(*alloc_func)(size_t len)) 
```

Returns a copy of `fio_keystr_s` - used internally by the hash map.

**Note**: when `.capa == FIO_KEYSTR_CONST` then the new `fio_keystr_s` will most likely point to the original pointer (which much remain valid in memory for the lifetime of the key string). Short enough strings are always copied to allow for improved cache locality.

#### `fio_keystr_destroy`

```c
void fio_keystr_destroy(fio_keystr_s *key, void (*free_func)(void *, size_t));
```

Destroys a copy of `fio_keystr_s` - used internally by the hash map.

#### `fio_keystr_is_eq`

```c
int fio_keystr_is_eq(fio_keystr_s a, fio_keystr_s b);
```

Compares two Key Strings - used internally by the hash map.

#### `FIO_KEYSTR_CONST`

```c
#define FIO_KEYSTR_CONST ((size_t)-1LL)
```

### `fio_keystr` Example

This example maps words to numbers. Note that this will work also with binary data and dynamic strings.

```c
/* map words to numbers. */
#define FIO_MAP_KEY_KSTR
#define FIO_UMAP_NAME umap
#define FIO_MAP_VALUE uintptr_t
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)&umap_destroy)
#include "fio-stl/include.h" /* or "fio-stl.h" */

/* example adding strings to map and printing data. */
void map_keystr_example(void) {
  umap_s map = FIO_MAP_INIT;
  /* FIO_KEYSTR_CONST prevents copying of longer constant strings */
  umap_set(&map, FIO_STR_INFO3("One", 3, FIO_KEYSTR_CONST), 1, NULL);
  umap_set(&map, FIO_STR_INFO3("Two", 3, FIO_KEYSTR_CONST), 2, NULL);
  umap_set(&map, FIO_STR_INFO3("Three", 5, FIO_KEYSTR_CONST), 3, NULL);
  FIO_MAP_EACH(umap, &map, pos) {
    uintptr_t value = pos.value;
    printf("%.*s: %llu\n",
           (int)pos.key.len,
           pos.key.buf,
           (unsigned long long)value);
  }
  umap_destroy(&map);
}
```
-------------------------------------------------------------------------------
