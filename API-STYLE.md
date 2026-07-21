# fio-stl API Style Guide

**Scope:** Tranche A (`000`–`011`) patterns observed during 2026-05-07 hardening audit.  
**Purpose:** Document the conventions that *are* followed, and explicitly call out exceptions with file/line references.  
**Audience:** Contributors writing new modules or refactoring existing ones during the hardening campaign.

---

## 1. Naming Conventions

### 1.1 General Rule

Use `snake_case` for all identifiers.

**Pattern:**
```c
fio_url_parse(...)
fio_string_write(...)
fio_thread_mutex_lock(...)
```

**Exceptions:**
- Macro-template APIs require consumer-defined prefixes (`FIO_SORT_NAME`, `FIO_IMAP_*`). These are UPPER_SNAKE_CASE by necessity.
- The memory allocator uses `FIO_NAME(FIO_MEMORY_NAME, malloc)` macro namespacing in `010 mem.h`. This is intentional (supports multiple allocator instances) but is the **only** macro-namespaced public API in Tranche A.

### 1.2 Public Types

**Pattern:** `fio_xxx_s` for structs, `fio_xxx_e` for enums.

| Type | File |
|------|------|
| `fio_str_info_s` | `011 string core.h` |
| `fio_buf_info_s` | `000 core.h` |
| `fio_url_s` | `002 url.h` |
| `fio_json_parser_s` | `004 json.h` |
| `fio_cli_arg_e` | `005 cli.h` |

**Exception:** `fio_socket_i` in `004 sock.h` is a typedef (SOCKET on Windows, int on POSIX). Ends in `_i` rather than `_s` to indicate scalar identity.

### 1.3 Internal Types

**Pattern:** `fio___xxx_s` (triple underscore).

| Type | File | Line |
|------|------|------|
| `fio___bstr_meta_s` | `011 string core.h` | ~481 |
| `fio___cli_aliases_s` | `005 cli.h` | ~339 |
| `fio___url_encoded_cb_s` | `004 urlencoded.h` | ~156 |

**Exception:** None observed in Tranche A. All internal types follow this rule.

### 1.4 Callback Types

**Pattern:** `fio_xxx_fn`.

| Type | File |
|------|------|
| `fio_string_realloc_fn` | `011 string core.h` |

### 1.5 Public Functions

**Pattern:** `fio_xxx` (module prefix + verb/noun).

| Function | Module |
|----------|--------|
| `fio_url_parse` | URL |
| `fio_string_write` | String |
| `fio_thread_create` | Threads |

**Exceptions:**
- `fio_aton` in `002 atol.h` lacks module prefix. Should ideally be `fio_atol_parse` or similar. **Historical.**
- `fio_ltocstr`, `fio_cstr2u64`, `fio_u2dig` in `002 atol.h` are abbreviated. Acceptable for tight utility functions.

### 1.6 Internal Functions

**Pattern:** `fio___xxx` (triple underscore).

| Function | File | Line |
|----------|------|------|
| `fio___url_encoded_callbacks_validate` | `004 urlencoded.h` | ~161 |
| `fio___cli_data_set` | `005 cli.h` | ~412 |
| `fio___mem_block_new` | `010 mem.h` | ~1651 |

**Exception:** Some internal helpers in `010 mem.h` use `FIO_NAME(FIO_MEMORY_NAME, __mem_xxx)` macro naming. These are internal and not directly callable.

---

## 2. Function Qualifiers

### 2.1 The Four Qualifiers

| Qualifier | Linkage | Body Location | Use Case |
|-----------|---------|---------------|----------|
| `SFUNC` | External (respects `FIO_EXTERN`) | Inside `#if defined(FIO_EXTERN_COMPLETE) \|\| !defined(FIO_EXTERN)` | Public API functions |
| `IFUNC` | External (respects `FIO_EXTERN`) | Inside `FIO_EXTERN_COMPLETE` block | Public API functions (older style) |
| `FIO_IFUNC` | Always `static inline` | Anywhere | Small helpers, accessors, fast paths |
| `FIO_SFUNC` | Always `static` | Anywhere | Internal helpers |

### 2.2 Correct Placement

**SFUNC / IFUNC bodies MUST be inside the extern block:**
```c
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
SFUNC void fio_example(void) { /* body */ }
#endif
```

**FIO_IFUNC bodies MAY be anywhere:**
```c
FIO_IFUNC size_t fio_helper(size_t x) { return x + 1; }
```

### 2.3 Exceptions / Observed Deviations

- `fio_string_capa4len()` in `011 string core.h` (~656) is `FIO_IFUNC` but its body appears in the "declaration" section before the `FIO_EXTERN_COMPLETE` block. This is allowed by the qualifier rules but breaks visual separation.
- `fio_bstr_info()`, `fio_bstr_buf()`, `fio_bstr_len()` in `011 string core.h` are `FIO_IFUNC` with bodies intermixed with declarations. Acceptable for accessors.
- `fio___cli_print_help()` in `005 cli.h` (~959) is `FIO_SFUNC` but called before its definition (forward declaration at ~410). This is fine because `FIO_SFUNC` is `static`.

---

## 3. Error Model

### 3.1 Default Convention

**Return `-1` on failure, `0` on success.** (Kernel style.)

**Functions following this convention:**
- `fio_json_parse()` → `int` (0 success, -1 failure)
- `fio_sock_open()` → `int` (0 success, -1 failure)
- `fio_string_write()` → `int` (0 success, -1 truncation/ENOMEM)
- `fio_string_replace()` → `int` (0 success, -1 failure)
- `fio_string_printf()` → `int` (0 success, -1 failure)
- `fio_rand_secure()` → `int` (0 success, -1 failure)
- `fio_filename_open()` → `int` (0 success, -1 failure)
- `fio_string_utf8_select()` → `int` (0 success, -1 error)

### 3.2 Justified Exceptions

**A. Struct return with `.err` field**

When a function must return multiple values, use a result struct with an `.err` field.

| Function | Return Type | Error Field | File |
|----------|-------------|-------------|------|
| `fio_aton()` | `fio_aton_s` | `.err` | `002 atol.h` |
| `fio_url_encoded_parse()` | `fio_url_encoded_result_s` | `.err` | `004 urlencoded.h` |

**B. Pointer return (NULL on failure)**

When returning an object, NULL signals failure.

| Function | Return Type | File |
|----------|-------------|------|
| `fio_malloc()` | `void *` | `010 mem.h` |
| `fio_cli_get()` | `char const *` | `005 cli.h` |
| `fio_bstr_write()` | `char *` | `011 string core.h` |

**C. Boolean return (1 true, 0 false)**

For pure predicates.

| Function | Return Type | File |
|----------|-------------|------|
| `fio_glob_match()` | `uint8_t` | `002 glob matching.h` |
| `fio_string_utf8_valid()` | `bool` | `011 string core.h` |
| `fio_state_callback_is_pending()` | `int` (1/0) | `004 state callbacks.h` |

**D. Size return (bytes written/consumed)**

For encode/decode functions where 0 is a valid result.

| Function | Return Type | File |
|----------|-------------|------|
| `fio_url_decode()` | `size_t` | `002 url.h` |
| `fio_url_encode()` | `size_t` | `002 url.h` |
| `fio_json_unescape()` | `size_t` | `004 json.h` |
| `fio_json_escape()` | `size_t` | `004 json.h` |

**E. Multi-state integer (0/1/-1)**

For streaming parsers with intermediate states.

| Function | Return Values | File |
|----------|---------------|------|
| `fio_multipart_parser_feed()` | 0=need-more, 1=done, -1=error | `004 multipart.h` |
| `fio_resp3_parser_feed()` | 0=need-more, 1=done, -1=error | `004 resp3.h` |

**F. No error path**

For pure functions or void returns.

| Function | Return Type | File |
|----------|-------------|------|
| `fio_crc32()` | `uint32_t` | `002 crc32.h` |
| `fio_rand64()` | `uint64_t` | `002 random.h` |
| `fio_time_real()` | `void` | `004 time.h` |
| `fio_cli_end()` | `void` | `005 cli.h` |

### 3.3 Problematic Exceptions

**MEDIUM priority:**
- `fio_bstr_write()` in `011 string core.h` (~810) returns `char *`. OOM behavior is not explicitly documented (does it return NULL? truncate?).

---

## 4. Type Conventions

### 4.1 String Views

Two non-owning view types are used throughout:

```c
typedef struct {
  char *buf;
  size_t len;
} fio_str_info_s;   /* NUL-terminated assumption: buf[len] == 0 */

typedef struct {
  char *buf;
  size_t len;
} fio_buf_info_s;   /* Binary safe, no NUL guarantee */
```

**Rule:** Use `fio_str_info_s` when the data is expected to be NUL-terminated (e.g., C strings, JSON text). Use `fio_buf_info_s` when the data may contain NUL bytes (e.g., binary payloads, raw socket data).

**Exception:** Many functions accept `fio_buf_info_s` but internally assume NUL termination (e.g., `fio_string_write` uses `dest->buf[dest->len] = 0`). This is documented but requires caller discipline.

### 4.2 Owning String Types

| Type | Ownership | Cleanup | File |
|------|-----------|---------|------|
| `fio_bstr` ( `char *` ) | Reference-counted | `fio_bstr_free()` | `011 string core.h` |
| `fio_keystr_s` | Heap or embedded SSO | `fio_keystr_destroy()` with free callback | `011 string core.h` |

**Rule:** Owning types require explicit cleanup. Non-owning views (`fio_str_info_s`, `fio_buf_info_s`) do not.

### 4.3 Callback-Driven Parsers

All streaming parsers in Tranche A use the same pattern:

```c
/* 1. Define callbacks */
typedef struct {
  void *(*on_event)(void *udata, ...);
  void (*on_error)(void *udata);
} fio_xxx_parser_callbacks_s;

/* 2. Initialize parser state */
void fio_xxx_parser_init(fio_xxx_parser_s *parser, ...);

/* 3. Feed data */
int fio_xxx_parser_feed(fio_xxx_parser_s *parser, const char *data, size_t len);
```

**Modules using this pattern:**
- `004 json.h`
- `004 multipart.h`
- `004 resp3.h`
- `004 urlencoded.h`

**Exception:** None. This is a highly coherent family.

---

## 5. Memory Allocation

### 5.1 Macros

Use the `FIO_MEM_*` macros, never raw libc functions:

| Operation | Macro | File |
|-----------|-------|------|
| Allocate | `FIO_MEM_REALLOC(NULL, 0, size, 0)` | `001 header.h` |
| Reallocate | `FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)` | `001 header.h` |
| Free | `FIO_MEM_FREE(ptr, size)` | `001 header.h` |
| Copy | `FIO_MEMCPY(dst, src, len)` | `000 core.h` |
| Move | `FIO_MEMMOVE(dst, src, len)` | `000 core.h` |
| Set | `FIO_MEMSET(ptr, val, len)` | `000 core.h` |
| Compare | `FIO_MEMCMP(a, b, len)` | `000 core.h` |

**Exception:** `011 string core.h` provides `fio_string_sys_reallocate()` which uses raw `realloc()` as a fallback. This is explicitly labeled as "system realloc" and is acceptable for the fallback path.

### 5.2 Allocator API

The custom allocator in `010 mem.h` uses macro namespacing:

```c
void *fio_malloc(size_t size);
void *fio_calloc(size_t size_per_unit, size_t unit_count);
void fio_free(void *ptr);
void *fio_realloc(void *ptr, size_t new_size);
void *fio_realloc2(void *ptr, size_t new_size, size_t copy_len);
void *fio_mmap(size_t size);
```

**Rule:** `fio_realloc2()` is preferred over `fio_realloc()` when the amount of data to copy is known to be smaller than `new_size`.

---

## 6. Macro Template Hygiene

### 6.1 Pattern

Macro-template modules (sort, imap) require the consumer to define prefixes before inclusion:

```c
#define FIO_SORT_NAME my_sort
#define FIO_SORT_TYPE int
#define FIO_SORT_CMP(a, b) ((a) < (b))
#include "002 sort.h"
#undef FIO_SORT_NAME
#undef FIO_SORT_TYPE
#undef FIO_SORT_CMP
```

### 6.2 Cleanup Rule

All `#define`s introduced by a macro-template module MUST be `#undef`d at the end of the header.

**Exception:** `010 mem.h` intentionally leaves `FIO_MEMORY_NAME` defined (line 2668: "don't undefine FIO_MEMORY_NAME due to possible use in allocation macros"). This is acceptable because the allocator is typically the last module configured.

---

## 7. File Naming

### 7.1 Pattern

Files are numbered `NNN description.h`. Descriptions use spaces, not dashes.

**Examples:**
- `000 core.h`
- `002 glob matching.h`
- `004 state callbacks.h`
- `011 string core.h`

**Exception:** `004 urlencoded.h` has no space. Minor inconsistency.

---

## 8. Documentation Style

### 8.1 Pattern

Use Doxygen-style `/** */` blocks for public APIs:

```c
/**
 * Brief description.
 *
 * Detailed description.
 *
 * * `arg` description.
 * * `arg2` description.
 */
SFUNC int fio_example(int arg, int arg2);
```

### 8.2 Exception

Some internal helpers have minimal or no comments. Acceptable for `FIO_SFUNC` static helpers.

---

## 9. Summary Table: Patterns vs. Exceptions

| Pattern | Coverage | Key Exceptions |
|---------|----------|----------------|
| `snake_case` | ~99% | Macro-template prefixes (UPPER_SNAKE_CASE) |
| Public types `fio_xxx_s` | ~100% | `fio_socket_i` (scalar typedef) |
| Internal types `fio___xxx_s` | ~100% | None |
| Public funcs `fio_xxx` | ~95% | `fio_aton` (missing module prefix) |
| Internal funcs `fio___xxx` | ~100% | Macro-namespaced allocator internals |
| Error: `-1` fail, `0` success | ~60% | Struct returns, pointer returns, boolean returns, size returns |
| `SFUNC` bodies in extern block | ~100% | `FIO_IFUNC` bodies anywhere |
| Callback-driven parsers | 4/4 modules | None |
| `FIO_MEM_*` macros | ~100% | `fio_string_sys_reallocate` fallback |

---

## 10. Tranche B Patterns (100–199)

### 10.1 Function Pointer Type Suffix: `_fn`

**Pattern:** Function pointer types use `_fn` suffix consistently across all modules.

| Type | Signature | File |
|------|-----------|------|
| `fio_crypto_enc_fn` | `void (*)(void *restrict mac, ...)` | `150 crypto core.h` |
| `fio_crypto_dec_fn` | `int (*)(void *restrict mac, ...)` | `150 crypto core.h` |

**Note:** The `_fn` suffix is used for all function pointer types, including both crypto operation vtables and user-provided callbacks (e.g., `fio_string_realloc_fn` in `011 string core.h`). There is no `_f` suffix in the public API.

### 10.2 Opaque Handle Pattern

**Pattern:** Forward-declare structs to hide implementation details.

```c
typedef struct fio_stream_packet_s fio_stream_packet_s;
typedef struct fio_deflate_s fio_deflate_s;
```

The actual struct definition appears only in the implementation section (`#if defined(FIO_EXTERN_COMPLETE) ...`).

**Modules using this pattern:**
- `102 stream.h` (`fio_stream_packet_s`)
- `162 deflate.h` (`fio_deflate_s`)

### 10.3 Named Argument Macros

**Pattern:** Use compound literals to provide named arguments for complex structs.

```c
#define fio_argon2(...) \
  fio_argon2((fio_argon2_args_s){__VA_ARGS__})

/* Usage: */
fio_argon2(.password = pwd, .salt = salt, .t_cost = 3);
```

**Modules using this pattern:**
- `159 argon2.h`
- `159 lyra2.h`
- `160 otp.h`

**Rule:** The macro must exactly match the function name. The struct type must be `{function_name}_args_s`.

### 10.4 Crypto One-Shot Hash Pattern

**Pattern:** One-shot hash functions return the digest struct by value. No error path.

```c
fio_u256 fio_sha256(const void *data, uint64_t len);
fio_u512 fio_sha512(const void *data, uint64_t len);
fio_u512 fio_sha1(const void *data, uint64_t len);
```

**Incremental variant:**
```c
fio_sha256_s fio_sha256_init(void);           /* returns state */
void fio_sha256_consume(fio_sha256_s *h, ...); /* feeds data */
fio_u256 fio_sha256_finalize(fio_sha256_s *h); /* returns digest */
```

**Rule:** One-shots are infallible (assuming valid pointers). Incremental APIs use `void` for consume operations.

### 10.5 AEAD Encrypt/Decrypt Asymmetry

**Pattern:** AEAD encryption returns `void`; decryption returns `int`.

```c
/* Encryption: void return — cannot fail with valid key/IV */
void fio_aes128_gcm_enc(void *restrict mac, size_t mac_len,
                        void *restrict ciphertext,
                        const void *restrict plaintext, size_t pt_len,
                        const void *restrict aad, size_t aad_len,
                        const uint8_t key[16], const uint8_t iv[12]);

/* Decryption: int return — authentication can fail */
int fio_aes128_gcm_dec(void *restrict mac, size_t mac_len,
                       void *restrict plaintext,
                       const void *restrict ciphertext, size_t ct_len,
                       const void *restrict aad, size_t aad_len,
                       const uint8_t key[16], const uint8_t iv[12]);
```

**Justification:** Encryption is a deterministic transform with no failure mode other than invalid inputs (programmer error). Decryption must authenticate the tag, which can fail.

**Exception:** This asymmetry is intentional but must be explicitly documented in header comments.

### 10.6 TLS Key Schedule Void-Return Family

**Pattern:** Pure key-derivation functions return `void`.

```c
void fio_tls13_hkdf_expand_label(void *restrict out, size_t out_len, ...);
void fio_tls13_derive_secret(void *restrict out, ...);
void fio_tls13_derive_early_secret(void *restrict early_secret, ...);
```

**Justification:** These are deterministic transforms with no allocation and no cryptographic failure mode. Invalid inputs (NULL, out-of-range lengths) are clamped or silently ignored.

**Exception:** Callers cannot distinguish success from silently-ignored invalid input. Use `FIO_ASSERT` in debug builds to catch programmer errors.

### 10.7 Compression Triple-State `size_t` Return

**Pattern:** Decompression functions return `size_t` with three states:

| Return Value | Meaning |
|--------------|---------|
| `> 0 && <= out_len` | Success — bytes written |
| `> out_len` | Buffer too small — required size |
| `== 0` | Corrupt input or error |

```c
size_t fio_brotli_decompress(void *out, size_t out_len,
                             const void *in, size_t in_len);
```

**Query mode:** When `out == NULL || out_len == 0`, returns required decompressed size (or 0 if corrupt).

**Exception:** `0` is ambiguous between "corrupt input" and "error". Document this explicitly. Consider using `SIZE_MAX` for error if the distinction matters to callers.

### 10.8 Queue Pop Convention

**Pattern:** `fio_queue_pop()` returns `fio_queue_task_s` by value. Error is signaled by a NULL function pointer inside the struct.

```c
fio_queue_task_s task = fio_queue_pop(q);
if (!task.fn) {
  /* queue empty or error */
}
```

**Exception:** The header comment says "Returns a NULL task on error", but structs cannot be NULL. The convention is implicit and should be documented explicitly.

---

## 11. Updated Summary Table: Patterns vs. Exceptions

| Pattern | Coverage | Key Exceptions |
|---------|----------|----------------|
| `snake_case` | ~99% | Macro-template prefixes (UPPER_SNAKE_CASE) |
| Public types `fio_xxx_s` | ~100% | `fio_socket_i` (scalar typedef) |
| Internal types `fio___xxx_s` | ~100% | None |
| Public funcs `fio_xxx` | ~95% | `fio_aton` (missing module prefix) |
| Internal funcs `fio___xxx` | ~100% | Macro-namespaced allocator internals |
| Error: `-1` fail, `0` success | ~55% | Void returns (crypto, TLS), struct returns, pointer returns |
| `SFUNC` bodies in extern block | ~100% | `FIO_IFUNC` bodies anywhere |
| Callback-driven parsers | 5/5 modules | None |
| `FIO_MEM_*` macros | ~100% | `fio_string_sys_reallocate` fallback |
| AEAD encrypt void-return | 3/3 ciphers | Intentional asymmetry with decrypt |
| TLS key schedule void-return | 14/14 functions | No error signaling |
| Compression triple-state | 4/4 functions | 0 is ambiguous |

---

*Document version: 2026-05-07 Tranche A+B audit.*
