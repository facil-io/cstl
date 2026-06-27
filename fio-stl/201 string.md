# Dynamic Strings — Module 201

Binary-safe dynamic strings, generated from a name macro.

See also: [← 200 types-overview.md](./200 types-overview.md)

---

## Setup

```c
#define FIO_STR_NAME my_str   /* required — sets the type and function prefix */
#include "fio-stl.h"
```

This generates `my_str_s` and a full API prefixed `my_str_*`.

Throughout this document the placeholder `STR` stands for whatever name you chose. All examples use `fio_str` as the name.

---

## Flavors

### Default — `FIO_STR_NAME`

The default struct stores capacity, length, and a buffer pointer (or embeds the string directly when it's short enough). On a 64-bit system the struct is 32 bytes, letting strings up to **30 bytes** live entirely inside the struct with no heap allocation.

```c
#define FIO_STR_NAME fio_str
#include "fio-stl.h"
```

Best for: strings that are written to multiple times, or strings in the 15–30 byte range that fit in the embedded slot.

### Small / immutable-optimized — `FIO_STR_SMALL`

```c
#define FIO_STR_SMALL key   /* also sets FIO_STR_NAME = key */
#include "fio-stl.h"
```

Sets `FIO_STR_OPTIMIZE4IMMUTABILITY 1`, which drops the capacity field to shrink the struct to 16 bytes. Short strings (≤ 14 bytes on 64-bit) are still embedded. Editing long strings is slower because capacity must be recalculated on each write.

Best for: map keys, mostly-immutable strings, or any context where struct size matters more than write throughput.

---

## Configuration Macros

Set these **before** `#include`, after `FIO_STR_NAME`:

| Macro | Default | Effect |
|---|---|---|
| `FIO_STR_OPTIMIZE_EMBEDDED` | `0` | Each unit adds `sizeof(char*)` to the struct, growing the embedded-string capacity. Max 4 (default) or 1 (`FIO_STR_SMALL`). |
| `FIO_STR_OPTIMIZE4IMMUTABILITY` | `0` | Drops the capacity field; set automatically by `FIO_STR_SMALL`. |

---

## Initialization Macros

| Macro | Notes |
|---|---|
| `FIO_STR_INIT` | Zero-initializes the struct. Use for stack or heap allocation. |
| `FIO_STR_INIT_EXISTING(buf, len, capa)` | Wraps an already-allocated buffer. **Not valid for `FIO_STR_SMALL`**. |
| `FIO_STR_INIT_STATIC(cstr)` | Wraps a string literal; `free` won't touch the buffer. **Not valid for `FIO_STR_SMALL`**. |
| `FIO_STR_INIT_STATIC2(buf, len)` | Same as above with explicit length. **Not valid for `FIO_STR_SMALL`**. |

```c
fio_str_s s = FIO_STR_INIT;           /* stack, zero-initialized */

fio_str_s *p = malloc(sizeof(*p));
*p = (fio_str_s)FIO_STR_INIT;         /* heap, zero-initialized */

fio_str_s ro = FIO_STR_INIT_STATIC("read-only literal");
```

---

## Lifecycle

### Heap allocation

```c
fio_str_s *STR_new(void);          /* allocate + zero-initialize */
void       STR_free(fio_str_s *s); /* destroy content + free struct */
```

### Stack / embedded destruction

```c
void STR_destroy(fio_str_s *s);    /* free content, re-initialize struct */
```

### Initialization helpers

```c
/* Wrap a static/const buffer. Copies if it fits embedded, otherwise borrows. */
fio_str_info_s STR_init_const(fio_str_s *s, const char *str, size_t len);

/* Always copies src into s. */
fio_str_info_s STR_init_copy(fio_str_s *s, const char *str, size_t len);

/* Copies another STR object into dest. */
fio_str_info_s STR_init_copy2(fio_str_s *dest, fio_str_s *src);
```

### Detach / hand-off

```c
/* Remove string data from s, return a heap-allocated C string. s is reset. */
char *STR_detach(fio_str_s *s);

/* Free a pointer returned by STR_detach. */
void  STR_dealloc(void *ptr);
```

`detach` always returns a dynamically allocated buffer (never embedded data). Free it with `STR_dealloc`, not plain `free`, unless you know the allocator matches.

---

## State Inspection

```c
fio_str_info_s STR_info(const fio_str_s *s);   /* buf + len + capa */
fio_buf_info_s STR_buf (const fio_str_s *s);   /* buf + len only   */
char          *STR_ptr (fio_str_s *s);          /* char * to data   */
size_t         STR_len (fio_str_s *s);          /* byte length      */
size_t         STR_capa(fio_str_s *s);          /* allocated capacity */
```

`fio_str_info_s` is a plain struct: `{ size_t len; char *buf; size_t capa; }`. When `capa == 0` the buffer is read-only (frozen or static). Data is always NUL-terminated but may contain embedded NUL bytes — don't use `strlen` on it.

```c
void     STR_freeze    (fio_str_s *s);          /* make read-only          */
uint8_t  STR_is_frozen (fio_str_s *s);          /* 1 if frozen             */
int      STR_is_allocated(const fio_str_s *s);  /* 1 if heap buffer in use */
int      STR_is_eq     (const fio_str_s *a,
                        const fio_str_s *b);     /* binary equality         */
uint64_t STR_hash      (const fio_str_s *s,
                        uint64_t seed);          /* Risky Hash              */
```

---

## Memory Management

```c
/* Resize without reallocation (capped by current capacity).
   Returns updated state. Shrinking may lose data beyond the new length. */
fio_str_info_s STR_resize(fio_str_s *s, size_t size);

/* Try to release unused capacity. Effect depends on allocator. */
void STR_compact(fio_str_s *s);

/* Reserve at least `amount` additional bytes beyond current length.
   NOT available for FIO_STR_SMALL types. */
fio_str_info_s STR_reserve(fio_str_s *s, size_t amount);
```

`reserve` + manual write + `resize` is the fastest pattern for bulk construction:

```c
fio_str_s s = FIO_STR_INIT;
fio_str_info_s info = fio_str_reserve(&s, 128);
size_t written = my_encoder(info.buf + fio_str_len(&s), 128, src);
fio_str_resize(&s, fio_str_len(&s) + written);
```

---

## Write / Append

All write functions append to the **end** of the string and return the updated `fio_str_info_s`.

```c
fio_str_info_s STR_write(fio_str_s *s, const void *src, size_t src_len);
fio_str_info_s STR_concat(fio_str_s *dest, fio_str_s *src); /* alias: STR_join */
fio_str_info_s STR_replace(fio_str_s *s,
                            intptr_t start_pos, /* negative = from end */
                            size_t   old_len,   /* 0 = insert only     */
                            const void *src,
                            size_t   src_len);  /* 0 = erase only      */
```

### Multi-write macro

```c
FIO_STR_WRITE2(str_name, dest_ptr, ...fio_string_write_s items...);
```

Combines multiple writes in one call using `fio_string_write_s` descriptors:

```c
#define FIO_STR_NAME fio_str
#include "fio-stl.h"

fio_str_s s = FIO_STR_INIT;
FIO_STR_WRITE2(fio_str, &s,
    FIO_STRING_WRITE_STR1("Hello "),
    FIO_STRING_WRITE_STR1("World!"));
/* s now contains "Hello World!" */
fio_str_destroy(&s);
```

---

## Number Writing

```c
fio_str_info_s STR_write_i  (fio_str_s *s, int64_t num); /* decimal     */
fio_str_info_s STR_write_hex(fio_str_s *s, int64_t num); /* raw hex digits, no 0x prefix */
fio_str_info_s STR_write_bin(fio_str_s *s, int64_t num); /* binary repr */
```

---

## Formatted Writes

```c
fio_str_info_s STR_printf (fio_str_s *s, const char *fmt, ...);
fio_str_info_s STR_vprintf(fio_str_s *s, const char *fmt, va_list argv);
```

Both append to the end of `s`.

```c
fio_str_s msg = FIO_STR_INIT;
fio_str_printf(&msg, "items: %zu, rate: %.2f\n", count, rate);
puts(fio_str_ptr(&msg));
fio_str_destroy(&msg);
```

---

## UTF-8

```c
size_t STR_utf8_valid (fio_str_s *s);                          /* char count if valid, 0 if not */
size_t STR_utf8_len   (fio_str_s *s);                          /* character count   */
int    STR_utf8_select(fio_str_s *s, intptr_t *pos, size_t *len); /* char→byte slice */
```

`utf8_select` converts a UTF-8 character position and length to byte offsets in-place. Sets `*pos = -1` on invalid UTF-8 before the selection point. Returns -1 on error, 0 on success.

---

## Escaping

### JSON / C escaping

```c
fio_str_info_s STR_write_escape  (fio_str_s *s, const void *data, size_t len);
fio_str_info_s STR_write_unescape(fio_str_s *s, const void *escaped, size_t len);
```

### HTML escaping

```c
fio_str_info_s STR_write_html_escape  (fio_str_s *s, const void *raw, size_t len);
fio_str_info_s STR_write_html_unescape(fio_str_s *s, const void *escaped, size_t len);
```

`write_html_unescape` handles numeric code-points and a minimal set of named entities — it is intentionally incomplete.

---

## Base64

```c
fio_str_info_s STR_write_base64enc(fio_str_s *s,
                                    const void *data, size_t data_len,
                                    uint8_t url_encoded); /* 1 for URL-safe variant */
fio_str_info_s STR_write_base64dec(fio_str_s *s,
                                    const void *encoded, size_t encoded_len);
```

---

## File / FD Reading

```c
/* Append from open file descriptor (regular files only; fd stays open). */
fio_str_info_s STR_readfd(fio_str_s *s, int fd,
                           intptr_t start_at, intptr_t limit);

/* Open filename, append slice, close. limit==0 reads to EOF. */
fio_str_info_s STR_readfile(fio_str_s *s, const char *filename,
                             intptr_t start_at, intptr_t limit);
```

On failure `STR_readfile` returns a state with `buf == NULL`.

---

## Quick Example

```c
#define FIO_STR_NAME fio_str
#include "fio-stl.h"

void greet(void) {
    fio_str_s s = FIO_STR_INIT;

    fio_str_write(&s, "Hello", 5);
    fio_str_write(&s, ", ", 2);
    fio_str_write(&s, "world!", 6);

    printf("%.*s\n", (int)fio_str_len(&s), fio_str_ptr(&s));
    /* → Hello, world! */

    fio_str_replace(&s, 0, 5, "Goodbye", 7);
    printf("%.*s\n", (int)fio_str_len(&s), fio_str_ptr(&s));
    /* → Goodbye, world! */

    fio_str_destroy(&s);
}
```

### Heap-allocated with reference counting

```c
#define FIO_STR_NAME fio_str
#define FIO_REF_NAME fio_str
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_DESTROY(s) fio_str_destroy(&(s))
#include "fio-stl.h"

void example(void) {
    fio_str_s *s = fio_str_new();
    fio_str_write(s, "shared", 6);

    fio_str_s *ref = fio_str_dup(s); /* increment ref count */
    fio_str_free(ref);               /* decrement; destroys string at zero */
    fio_str_free(s);
}
```

---

## Embedded-string Sizes (64-bit reference)

| Variant | Struct size | Max embedded string |
|---|---|---|
| `FIO_STR_NAME` (default) | 32 bytes | 30 bytes |
| `FIO_STR_NAME` + `FIO_STR_OPTIMIZE_EMBEDDED 1` | 40 bytes | 38 bytes |
| `FIO_STR_SMALL` | 16 bytes | 14 bytes |

Two bytes are always reserved for the metadata byte and the NUL terminator.

---

See [→ 200 types-overview.md](./200 types-overview.md) for the shared define-include-use pattern and how to combine this type with arrays, maps, and reference counters.
