# String and Buffer Information

Lightweight descriptors for byte ranges. They are defined in [`./000 core.h`](./000%20core.h) and are used throughout the library to pass around strings and buffers without taking ownership.

## Descriptor Types

### `fio_str_info_s`

A string descriptor that carries length, buffer pointer, and capacity.

```c
typedef struct fio_str_info_s {
  size_t len;   /* string length, in bytes */
  char *buf;    /* pointer to the first byte, or NULL on error */
  size_t capa;  /* buffer capacity; 0 means read-only */
} fio_str_info_s;
```

`capa` tells callers how much room is available in the underlying buffer. When `capa` is `0`, the buffer is considered read-only and must not be written to through the descriptor.

### `fio_buf_info_s`

A lighter buffer descriptor with no capacity field.

```c
typedef struct fio_buf_info_s {
  size_t len;   /* buffer length, in bytes */
  char *buf;    /* pointer to the first byte, may be NULL */
} fio_buf_info_s;
```

Use this when only a length/buffer pair is needed, such as for input data or temporary views.

## Construction Macros

### `FIO_STR_INFO0`

An all-zero `fio_str_info_s`.

```c
#define FIO_STR_INFO0 ((fio_str_info_s){0})
```

### `FIO_STR_INFO1(str)`

Build an `fio_str_info_s` from a NUL-terminated C string. Computes the length with `FIO_STRLEN`. If `str` is `NULL`, the length is `0`.

```c
#define FIO_STR_INFO1(str) \
  ((fio_str_info_s){.len = ((str) ? FIO_STRLEN((str)) : 0), .buf = (str)})
```

### `FIO_STR_INFO2(str, length)`

Build an `fio_str_info_s` from a buffer and a known length.

```c
#define FIO_STR_INFO2(str, length) \
  ((fio_str_info_s){.len = (length), .buf = (str)})
```

### `FIO_STR_INFO3(str, length, capacity)`

Build a fully initialized `fio_str_info_s`.

```c
#define FIO_STR_INFO3(str, length, capacity) \
  ((fio_str_info_s){.len = (length), .buf = (str), .capa = (capacity)})
```

### `FIO_BUF_INFO0`

An all-zero `fio_buf_info_s`.

```c
#define FIO_BUF_INFO0 ((fio_buf_info_s){0})
```

### `FIO_BUF_INFO1(str)`

Build an `fio_buf_info_s` from a NUL-terminated C string. If `str` is `NULL`, the length is `0`.

```c
#define FIO_BUF_INFO1(str) \
  ((fio_buf_info_s){.len = ((str) ? FIO_STRLEN((str)) : 0), .buf = (str)})
```

### `FIO_BUF_INFO2(str, length)`

Build an `fio_buf_info_s` from a buffer and a known length.

```c
#define FIO_BUF_INFO2(str, length) \
  ((fio_buf_info_s){.len = (length), .buf = (str)})
```

## Equality

### `FIO_STR_INFO_IS_EQ(s1, s2)`

Returns a true value if two `fio_str_info_s` objects have the same content.

The comparison is a fast-path short-circuit: equal length and either both empty, the same pointer, or matching first byte and `FIO_MEMCMP` equal.

```c
#define FIO_STR_INFO_IS_EQ(s1, s2) \
  ((s1).len == (s2).len && \
   (!(s1).len || (s1).buf == (s2).buf || \
    ((s1).buf && (s2).buf && (s1).buf[0] == (s2).buf[0] && \
     !FIO_MEMCMP((s1).buf, (s2).buf, (s1).len))))
```

### `FIO_BUF_INFO_IS_EQ(s1, s2)`

Same comparison, routed to `FIO_STR_INFO_IS_EQ`.

```c
#define FIO_BUF_INFO_IS_EQ(s1, s2) FIO_STR_INFO_IS_EQ((s1), (s2))
```

## Conversion

### `FIO_BUF2STR_INFO(buf_info)`

Convert an `fio_buf_info_s` to an `fio_str_info_s`. The resulting descriptor has the same `len` and `buf`; `capa` is left as `0`.

```c
#define FIO_BUF2STR_INFO(buf_info) \
  ((fio_str_info_s){.len = (buf_info).len, .buf = (buf_info).buf})
```

### `FIO_STR2BUF_INFO(str_info)`

Convert an `fio_str_info_s` to an `fio_buf_info_s`, dropping the capacity.

```c
#define FIO_STR2BUF_INFO(str_info) \
  ((fio_buf_info_s){.len = (str_info).len, .buf = (str_info).buf})
```

## Temporary Stack Variables

### `FIO_STR_INFO_TMP_VAR(name, capacity)`

Creates a stack-allocated `fio_str_info_s` named `name` with `capacity` writable bytes. The macro allocates `capacity + 1` bytes so a trailing NUL guard is always present, and initializes `name.buf` and `name.capa`.

```c
#define FIO_STR_INFO_TMP_VAR(name, capacity) \
  char fio___stack_mem___##name[(capacity) + 1]; \
  fio___stack_mem___##name[0] = 0; /* guard */ \
  fio_str_info_s name = (fio_str_info_s) { \
    .buf = fio___stack_mem___##name, .capa = (capacity) \
  }
```

Use this when a function needs a writable working buffer that normally fits on the stack but may be reallocated by downstream helpers.

### `FIO_STR_INFO_TMP_IS_REALLOCATED(name)`

Returns a true value if the temporary variable `name` no longer points to its original stack buffer, which indicates that a helper reallocated the memory onto the heap.

```c
#define FIO_STR_INFO_TMP_IS_REALLOCATED(name) \
  (fio___stack_mem___##name != name.buf)
```

If this returns true, the buffer must eventually be freed by whatever allocator the helper used (usually the facil.io allocator or the override set via `FIO_MEM_REALLOC`).
