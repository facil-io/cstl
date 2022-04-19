## Binary Safe String Core Helpers

```c
#define FIO_STR
#include "fio-stl.h"
```

The following helpers are part of the String core library and they become available whenever [a String type was defined](#dynamic-strings) or when the `FIO_STR` is defined before any inclusion of the C STL header.

The main difference between using the String Core API directly and defining a String type is that String types provide a few additional optimizations, such as embedding short strings (embedded within the type data rather than allocated), optional reference counting and pointer tagging features.

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
                                         size_t new_capa) {
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

#### `fio_string_insert`

```c
int fio_string_insert(fio_str_info_s *dest,
                      void (*reallocate)(fio_str_info_s *,
                                         size_t new_capa),
                      intptr_t start_pos,
                      size_t overwrite_len,
                      const void *src,
                      size_t len);
```

Similar to `fio_string_write`, only replacing a sub-string or inserting a string in a specific location.

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
void fio_string_default_free_noop(fio_str_info_s str);
```

Does nothing.

### Core String Comparison

#### `fio_string_is_bigger`

```c
int fio_string_is_bigger(fio_str_info_s a, fio_str_info_s b);
```

Compares two strings, returning 1 if string `a` is bigger than string `b`.

**Note**: returns 0 if string `b` is bigger than string `a` or if strings are equal.

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

Returns the String's length in UTF-8 characters.

#### `fio_string_utf8_select`

```c
int fio_string_utf8_select(fio_str_info_s str, intptr_t *pos, size_t *len);
```

Takes a UTF-8 character selection information (UTF-8 position and length) and updates the same variables so they reference the raw byte slice information.

If the String isn't UTF-8 valid up to the requested selection, than `pos` will be updated to `-1` otherwise values are always positive.

The returned `len` value may be shorter than the original if there wasn't enough data left to accommodate the requested length. When a `len` value of `0` is returned, this means that `pos` marks the end of the String.

Returns -1 on error and 0 on success.

-------------------------------------------------------------------------------
