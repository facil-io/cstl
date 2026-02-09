## SIMD Types and Vector Operations

The facil.io C STL provides a comprehensive set of aligned union types, SIMD-aware vector operations, and multi-precision math primitives. These are always available as part of the core library -- no `#define` or `#include` is needed beyond the initial inclusion of the STL.

**Note**: the [Hex Read/Write](#hex-readwrite) functions are an exception -- they require `#define FIO_ATOL` before inclusion.

The library abstracts over three SIMD backends:

1. **ARM NEON** -- uses `uint64x2_t`, `uint32x4_t`, `uint16x8_t`, `uint8x16_t` intrinsics (128-bit vectors).
2. **GCC/Clang `vector_size`** -- uses compiler vector extensions for full-width vectors.
3. **Scalar fallback** -- uses plain C arrays, relying on compiler auto-vectorization.

The same source code compiles to efficient SIMD on all platforms. The backend is selected automatically at compile time based on available intrinsics and compiler features.

### SIMD Backend Selection

The SIMD backend is selected automatically at compile time. The library detects available intrinsics and chooses the best option:

| Platform | Detection condition | Effect |
|----------|-------------------|--------|
| ARM NEON + crypto | `__ARM_FEATURE_CRYPTO` and `__ARM_NEON` defined | Includes `arm_neon.h` and `arm_acle.h` |
| ARMv8.2-A SHA3 | `__ARM_FEATURE_SHA3` defined | Enables EOR3/RAX1/BCAX instructions |
| x86_64 | `__x86_64` and `immintrin.h` available | Includes `immintrin.h` |
| x86 SHA-NI | `__SHA__` defined | Enables SHA-NI instructions |

All intrinsic detection is guarded by `!defined(DEBUG) && !defined(NO_INTRIN)` -- intrinsics are disabled in debug builds and when `NO_INTRIN` is defined.

### SIMD Loop Helper

#### `FIO_FOR_UNROLL`

```c
#define FIO_FOR_UNROLL(iterations, size_of_loop, i, action)
```

An unrolled `for` loop macro that splits iteration into a remainder loop followed by fixed-stride batches (256 bytes / `size_of_loop` iterations per batch). This structure enables compiler auto-vectorization by presenting predictable, aligned loop bodies.

**Parameters:**
- `iterations` - total number of iterations to perform
- `size_of_loop` - number of bytes consumed by each `action` (used to compute batch size)
- `i` - the loop index variable name (accessible within `action`)
- `action` - an expression or statement to execute each iteration (may reference `i`)

Example:

```c
/* Zero out an array of 1000 uint32_t values with auto-vectorization hints */
uint32_t arr[1000];
FIO_FOR_UNROLL(1000, sizeof(uint32_t), i, arr[i] = 0);
```

**Note**: the remainder loop handles `iterations % (256 / size_of_loop)` elements first, then the main loop processes full 256-byte batches. This ensures correct behavior for any iteration count.

### Aligned Union Types (`fio_uXXX`)

The library defines six aligned union types for SIMD and multi-precision math: `fio_u128`, `fio_u256`, `fio_u512`, `fio_u1024`, `fio_u2048`, and `fio_u4096`.

Each type is a `union` containing multiple views of the same memory:

#### `fio_u128`

```c
typedef union fio_u128 {
  /* Unsigned integers */
  size_t   uz[16 / sizeof(size_t)];  /* native word array, system-dependent length */
  uint64_t u64[2];
  uint32_t u32[4];
  uint16_t u16[8];
  uint8_t  u8[16];
  /* Signed integers */
  ssize_t  iz[16 / sizeof(ssize_t)]; /* signed native word array */
  int64_t  i64[2];
  int32_t  i32[4];
  int16_t  i16[8];
  int8_t   i8[16];
  /* Floating point */
  float    f[16 / sizeof(float)];
  double   d[16 / sizeof(double)];
  long double ld[16 / sizeof(long double)];
  /* SIMD vector members - platform dependent */
  /* ARM NEON:    uint64x2_t x64[1]; uint32x4_t x32[1]; ... */
  /* GCC vector:  uint64_t __attribute__((vector_size(16))) x64[1]; ... */
  /* Scalar:      uint64_t x64[2]; uint32_t x32[4]; ... */
#if defined(__SIZEOF_INT128__)
  __uint128_t alignment_for_u128_[1]; /* forces 128-bit alignment on supported compilers */
#endif
} fio_u128 FIO_ALIGN(16);
```

**Note**: on compilers that support `__uint128_t` (detected via `__SIZEOF_INT128__`), an additional `__uint128_t alignment_for_u128_[1]` member is included to guarantee proper 128-bit alignment of the union.

#### `fio_u256`

```c
typedef union fio_u256 {
  fio_u128 u128[2];
  /* ... all unsigned, signed, float, and SIMD members for 256 bits ... */
#if defined(__SIZEOF_INT256__)
  __uint256_t alignment_for_u256_[1]; /* forces 256-bit alignment on supported compilers */
#endif
} fio_u256 FIO_ALIGN(32);
```

**Note**: on compilers that support `__uint256_t` (detected via `__SIZEOF_INT256__`), an additional alignment member is included. In practice, very few compilers define this type.

#### `fio_u512`

```c
typedef union fio_u512 {
  fio_u128 u128[4];
  fio_u256 u256[2];
  /* ... all data and SIMD members for 512 bits ... */
} fio_u512 FIO_ALIGN(64);
```

#### `fio_u1024`

```c
typedef union fio_u1024 {
  fio_u128 u128[8];
  fio_u256 u256[4];
  fio_u512 u512[2];
  /* ... all data and SIMD members for 1024 bits ... */
} fio_u1024 FIO_ALIGN(64);
```

#### `fio_u2048`

```c
typedef union fio_u2048 {
  fio_u128 u128[16];
  fio_u256 u256[8];
  fio_u512 u512[4];
  fio_u1024 u1024[2];
  /* ... all data and SIMD members for 2048 bits ... */
} fio_u2048 FIO_ALIGN(64);
```

#### `fio_u4096`

```c
typedef union fio_u4096 {
  fio_u128 u128[32];
  fio_u256 u256[16];
  fio_u512 u512[8];
  fio_u1024 u1024[4];
  fio_u2048 u2048[2];
  /* ... all data and SIMD members for 4096 bits ... */
} fio_u4096 FIO_ALIGN(64);
```

#### Data Member Array Lengths

Each union contains the following data member arrays. The array length depends on the total bit width:

| Member | Type | 128 | 256 | 512 | 1024 | 2048 | 4096 |
|--------|------|-----|-----|-----|------|------|------|
| `uz[]` / `iz[]` | `size_t` / `ssize_t` | 2* | 4* | 8* | 16* | 32* | 64* |
| `u64[]` / `i64[]` | `uint64_t` / `int64_t` | 2 | 4 | 8 | 16 | 32 | 64 |
| `u32[]` / `i32[]` | `uint32_t` / `int32_t` | 4 | 8 | 16 | 32 | 64 | 128 |
| `u16[]` / `i16[]` | `uint16_t` / `int16_t` | 8 | 16 | 32 | 64 | 128 | 256 |
| `u8[]` / `i8[]` | `uint8_t` / `int8_t` | 16 | 32 | 64 | 128 | 256 | 512 |
| `f[]` | `float` | 4 | 8 | 16 | 32 | 64 | 128 |
| `d[]` | `double` | 2 | 4 | 8 | 16 | 32 | 64 |
| `ld[]` | `long double` | * | * | * | * | * | * |

\* `uz[]`/`iz[]` and `ld[]` lengths are system-dependent (`sizeof(size_t)` and `sizeof(long double)` vary by platform).

#### SIMD Vector Members

The `x64[]`, `x32[]`, `x16[]`, `x8[]` members provide platform-specific SIMD vector access:

| Platform | `x64` type | `x32` type | `x16` type | `x8` type | Array length |
|----------|-----------|-----------|-----------|----------|-------------|
| ARM NEON | `uint64x2_t` | `uint32x4_t` | `uint16x8_t` | `uint8x16_t` | `bits / 128` |
| GCC `vector_size` | full-width vector | full-width vector | full-width vector | full-width vector | `1` |
| Scalar fallback | `uint64_t` | `uint32_t` | `uint16_t` | `uint8_t` | `bits / 64`, `bits / 32`, etc. |

#### Nesting

Larger types nest smaller ones for convenient sub-vector access:

| Type | Nested members |
|------|---------------|
| `fio_u256` | `u128[2]` |
| `fio_u512` | `u128[4]`, `u256[2]` |
| `fio_u1024` | `u128[8]`, `u256[4]`, `u512[2]` |
| `fio_u2048` | `u128[16]`, `u256[8]`, `u512[4]`, `u1024[2]` |
| `fio_u4096` | `u128[32]`, `u256[16]`, `u512[8]`, `u1024[4]`, `u2048[2]` |

#### Alignment

| Type | Alignment |
|------|-----------|
| `fio_u128` | 16 bytes |
| `fio_u256` | 32 bytes |
| `fio_u512` | 64 bytes |
| `fio_u1024` | 64 bytes |
| `fio_u2048` | 64 bytes |
| `fio_u4096` | 64 bytes |

### Init Macros

Each `fio_uXXX` type has four initialization macros that initialize via the corresponding unsigned integer array member:

```c
#define fio_uXXX_init8(...)  ((fio_uXXX){.u8 = {__VA_ARGS__}})
#define fio_uXXX_init16(...) ((fio_uXXX){.u16 = {__VA_ARGS__}})
#define fio_uXXX_init32(...) ((fio_uXXX){.u32 = {__VA_ARGS__}})
#define fio_uXXX_init64(...) ((fio_uXXX){.u64 = {__VA_ARGS__}})
```

Example:

```c
/* Initialize a 256-bit value with four 64-bit words */
fio_u256 val = fio_u256_init64(0x0123456789ABCDEF,
                                0xFEDCBA9876543210,
                                0x1111111111111111,
                                0x2222222222222222);

/* Initialize a 128-bit value with 16 bytes */
fio_u128 key = fio_u128_init8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
```

**Complete list of init macros:**

| 128-bit | 256-bit | 512-bit | 1024-bit | 2048-bit | 4096-bit |
|---------|---------|---------|----------|----------|----------|
| `fio_u128_init8` | `fio_u256_init8` | `fio_u512_init8` | `fio_u1024_init8` | `fio_u2048_init8` | `fio_u4096_init8` |
| `fio_u128_init16` | `fio_u256_init16` | `fio_u512_init16` | `fio_u1024_init16` | `fio_u2048_init16` | `fio_u4096_init16` |
| `fio_u128_init32` | `fio_u256_init32` | `fio_u512_init32` | `fio_u1024_init32` | `fio_u2048_init32` | `fio_u4096_init32` |
| `fio_u128_init64` | `fio_u256_init64` | `fio_u512_init64` | `fio_u1024_init64` | `fio_u2048_init64` | `fio_u4096_init64` |

### Load / Store / Endian Operations

For each `fio_uXXX` type (128, 256, 512, 1024, 2048, 4096), the library generates 11 functions for loading from memory, storing to memory, and byte-swapping with endian conversion.

#### Representative Signatures

```c
/* Load from memory in local (native) endian order */
fio_uXXX fio_uXXX_load(const void *buf);

/* Store to memory in local (native) endian order */
void fio_uXXX_store(void *buf, const fio_uXXX a);

/* Load, converting 16/32/64-bit words from little-endian */
fio_uXXX fio_uXXX_load_le16(const void *buf);
fio_uXXX fio_uXXX_load_le32(const void *buf);
fio_uXXX fio_uXXX_load_le64(const void *buf);

/* Load, converting 16/32/64-bit words from big-endian */
fio_uXXX fio_uXXX_load_be16(const void *buf);
fio_uXXX fio_uXXX_load_be32(const void *buf);
fio_uXXX fio_uXXX_load_be64(const void *buf);

/* Byte-swap each 16/32/64-bit word in place */
fio_uXXX fio_uXXX_bswap16(fio_uXXX a);
fio_uXXX fio_uXXX_bswap32(fio_uXXX a);
fio_uXXX fio_uXXX_bswap64(fio_uXXX a);
```

Example:

```c
/* Load 256 bits from a buffer, treating each 64-bit word as big-endian */
uint8_t wire_data[32] = { /* ... network data ... */ };
fio_u256 val = fio_u256_load_be64(wire_data);

/* Store back to a buffer in local endian */
uint8_t local_buf[32];
fio_u256_store(local_buf, val);
```

#### Complete Function List

| Function pattern | 128 | 256 | 512 | 1024 | 2048 | 4096 |
|-----------------|-----|-----|-----|------|------|------|
| `fio_uXXX_load` | `fio_u128_load` | `fio_u256_load` | `fio_u512_load` | `fio_u1024_load` | `fio_u2048_load` | `fio_u4096_load` |
| `fio_uXXX_store` | `fio_u128_store` | `fio_u256_store` | `fio_u512_store` | `fio_u1024_store` | `fio_u2048_store` | `fio_u4096_store` |
| `fio_uXXX_load_le16` | `fio_u128_load_le16` | `fio_u256_load_le16` | `fio_u512_load_le16` | `fio_u1024_load_le16` | `fio_u2048_load_le16` | `fio_u4096_load_le16` |
| `fio_uXXX_load_le32` | `fio_u128_load_le32` | `fio_u256_load_le32` | `fio_u512_load_le32` | `fio_u1024_load_le32` | `fio_u2048_load_le32` | `fio_u4096_load_le32` |
| `fio_uXXX_load_le64` | `fio_u128_load_le64` | `fio_u256_load_le64` | `fio_u512_load_le64` | `fio_u1024_load_le64` | `fio_u2048_load_le64` | `fio_u4096_load_le64` |
| `fio_uXXX_load_be16` | `fio_u128_load_be16` | `fio_u256_load_be16` | `fio_u512_load_be16` | `fio_u1024_load_be16` | `fio_u2048_load_be16` | `fio_u4096_load_be16` |
| `fio_uXXX_load_be32` | `fio_u128_load_be32` | `fio_u256_load_be32` | `fio_u512_load_be32` | `fio_u1024_load_be32` | `fio_u2048_load_be32` | `fio_u4096_load_be32` |
| `fio_uXXX_load_be64` | `fio_u128_load_be64` | `fio_u256_load_be64` | `fio_u512_load_be64` | `fio_u1024_load_be64` | `fio_u2048_load_be64` | `fio_u4096_load_be64` |
| `fio_uXXX_bswap16` | `fio_u128_bswap16` | `fio_u256_bswap16` | `fio_u512_bswap16` | `fio_u1024_bswap16` | `fio_u2048_bswap16` | `fio_u4096_bswap16` |
| `fio_uXXX_bswap32` | `fio_u128_bswap32` | `fio_u256_bswap32` | `fio_u512_bswap32` | `fio_u1024_bswap32` | `fio_u2048_bswap32` | `fio_u4096_bswap32` |
| `fio_uXXX_bswap64` | `fio_u128_bswap64` | `fio_u256_bswap64` | `fio_u512_bswap64` | `fio_u1024_bswap64` | `fio_u2048_bswap64` | `fio_u4096_bswap64` |

That is 11 functions x 6 sizes = **66 functions** total.

### Low-Level Vector Helper Macros

These macros form the building blocks for all higher-level `fio_uXXX` vector operations. `FIO_MATH_UXXX_OP`, `COP`, `SOP`, and `TOP` operate on the `.xN[]` SIMD vector members (benefiting from hardware SIMD when available). `FIO_MATH_UXXX_REDUCE` and `FIO_MATH_UXXX_SUFFLE` operate on the `.uN[]` unsigned integer array members instead.

#### `FIO_MATH_UXXX_OP`

```c
#define FIO_MATH_UXXX_OP(t, a, b, bits, op)
```

Lane-wise binary operation: `t.xN[i] = a.xN[i] op b.xN[i]` for each lane `i`.

- `t` - target `fio_uXXX` value (modified in place)
- `a`, `b` - source `fio_uXXX` values
- `bits` - selects the union member (64 for `.x64`, 32 for `.x32`, etc.)
- `op` - C binary operator (`+`, `-`, `*`, `&`, `|`, `^`)

#### `FIO_MATH_UXXX_COP`

```c
#define FIO_MATH_UXXX_COP(t, a, b, bits, op)
```

Lane-wise operation with a scalar constant: `t.xN[i] = a.xN[i] op b` for each lane `i`.

- `b` - a scalar constant (not a `fio_uXXX` value)
- All other parameters are the same as `FIO_MATH_UXXX_OP`.

#### `FIO_MATH_UXXX_SOP`

```c
#define FIO_MATH_UXXX_SOP(t, a, bits, op)
```

Lane-wise unary operation: `t.xN[i] = op a.xN[i]` for each lane `i`.

- `op` - C unary operator (e.g., `~` for bitwise NOT)

#### `FIO_MATH_UXXX_TOP`

```c
#define FIO_MATH_UXXX_TOP(t, a, b, c, bits, expr)
```

Lane-wise ternary operation: `t.xN[i] = expr(a.xN[i], b.xN[i], c.xN[i])` for each lane `i`.

- `expr` - a function-like macro taking three arguments (e.g., a macro expanding to `(z) ^ ((x) & ((y) ^ (z)))` for mux/choose)

#### `FIO_MATH_UXXX_REDUCE`

```c
#define FIO_MATH_UXXX_REDUCE(t, a, bits, op)
```

Horizontal reduction across all lanes: `t = a.uN[0] op a.uN[1] op ... op a.uN[last]`.

- `t` - scalar result variable
- Uses `.uN[]` (unsigned integer array), not `.xN[]` (vector array)

#### `FIO_MATH_UXXX_SUFFLE`

```c
#define FIO_MATH_UXXX_SUFFLE(var, bits, ...)
```

Reorders lanes by index array: `var.uN[i] = var.uN[index[i]]` for each lane `i`. Uses the `.u##bits[]` unsigned integer array member (not the `.x##bits[]` vector member).

- `var` - the `fio_uXXX` value to shuffle (modified in place)
- `bits` - selects the word width for shuffling (accesses `.u##bits[]`)
- `...` - comma-separated index values

**Note**: the macro name is intentionally misspelled as "SUFFLE" (not "SHUFFLE"). When searching the codebase for shuffle operations, look for `SUFFLE`.

**Note**: for `FIO_MATH_UXXX_OP`, `COP`, `SOP`, and `TOP`, the `bits` parameter selects the `.x##bits[]` vector union member, **NOT** the element size. The loop count is `sizeof(t.xN) / sizeof(t.xN[0])`, which varies by platform: 1 for GCC `vector_size`, `bits/128` for NEON, `bits/64` (or `bits/32`, etc.) for scalar fallback. For `REDUCE` and `SUFFLE`, the `bits` parameter selects the `.u##bits[]` scalar array member instead.

### Lane-Wise Operations (Pointer-Based)

These functions perform lane-wise binary operations on `fio_uXXX` types through pointers. They are generated for all 6 sizes (128, 256, 512, 1024, 2048, 4096), 4 word widths (8, 16, 32, 64), and 6 operations.

#### Signature Pattern

```c
void fio_uXXX_opN(fio_uXXX *target, const fio_uXXX *a, const fio_uXXX *b);
```

Where `op` is one of `add`, `sub`, `mul`, `and`, `or`, `xor` and `N` is one of `8`, `16`, `32`, `64`.

#### Operations

| Operation | Operator | Description |
|-----------|----------|-------------|
| `add` | `+` | Lane-wise addition |
| `sub` | `-` | Lane-wise subtraction |
| `mul` | `*` | Lane-wise multiplication |
| `and` | `&` | Lane-wise bitwise AND |
| `or` | `\|` | Lane-wise bitwise OR |
| `xor` | `^` | Lane-wise bitwise XOR |

#### Complete Function List (6 ops x 4 widths x 6 sizes = 144 functions)

| Pattern | Word widths | Sizes |
|---------|-------------|-------|
| `fio_uXXX_addN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_subN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_mulN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_andN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_orN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_xorN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |

For example: `fio_u256_add32`, `fio_u512_xor64`, `fio_u1024_mul8`, etc.

#### Untyped (64-bit Lane) Pointer-Based Operations

Additionally, untyped convenience functions operate on 64-bit lanes without a width suffix:

```c
void fio_uXXX_and(fio_uXXX *target, const fio_uXXX *a, const fio_uXXX *b);
void fio_uXXX_or(fio_uXXX *target, const fio_uXXX *a, const fio_uXXX *b);
void fio_uXXX_xor(fio_uXXX *target, const fio_uXXX *a, const fio_uXXX *b);
```

Available for all 6 sizes: 3 ops x 6 sizes = **18 functions**.

### Constant Operations (Pointer-Based)

These functions perform lane-wise operations between a `fio_uXXX` vector and a scalar constant.

#### Signature Pattern

```c
void fio_uXXX_copN(fio_uXXX *target, const fio_uXXX *a, uintN_t b);
```

Where `cop` is one of `cadd`, `csub`, `cmul`, `cand`, `cor`, `cxor` and `N` is one of `8`, `16`, `32`, `64`.

#### Complete Function List (6 ops x 4 widths x 6 sizes = 144 functions)

| Pattern | Word widths | Sizes |
|---------|-------------|-------|
| `fio_uXXX_caddN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_csubN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_cmulN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_candN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_corN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_cxorN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |

For example: `fio_u256_cadd32(&target, &a, 42)`, `fio_u512_cxor64(&target, &a, 0xFF)`.

### Reduce Operations (Pointer-Based)

These functions perform horizontal reduction across all lanes of a `fio_uXXX` type, returning a single scalar value.

#### Signature Pattern

```c
uintN_t fio_uXXX_reduce_opN(const fio_uXXX *a);
```

Where `op` is one of `add`, `sub`, `mul`, `and`, `or`, `xor` and `N` is one of `8`, `16`, `32`, `64`.

#### Complete Function List (6 ops x 4 widths x 6 sizes = 144 functions)

| Pattern | Word widths | Sizes |
|---------|-------------|-------|
| `fio_uXXX_reduce_addN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_reduce_subN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_reduce_mulN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_reduce_andN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_reduce_orN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_reduce_xorN` | 8, 16, 32, 64 | 128, 256, 512, 1024, 2048, 4096 |

For example: `uint64_t sum = fio_u256_reduce_add64(&val)`.

### Value-Returning SIMD Operations

These functions return `fio_uXXX` by value (not through pointers). They use the `.x64` vector member for maximum SIMD width.

#### Signature Pattern

```c
fio_uXXX fio_uXXX_xorv(fio_uXXX a, fio_uXXX b);   /* XOR */
fio_uXXX fio_uXXX_andv(fio_uXXX a, fio_uXXX b);   /* AND */
fio_uXXX fio_uXXX_orv(fio_uXXX a, fio_uXXX b);    /* OR */
fio_uXXX fio_uXXX_addv64(fio_uXXX a, fio_uXXX b); /* ADD (64-bit lanes) */
```

#### Complete Function List (4 ops x 6 sizes = 24 functions)

| Operation | 128 | 256 | 512 | 1024 | 2048 | 4096 |
|-----------|-----|-----|-----|------|------|------|
| XOR | `fio_u128_xorv` | `fio_u256_xorv` | `fio_u512_xorv` | `fio_u1024_xorv` | `fio_u2048_xorv` | `fio_u4096_xorv` |
| AND | `fio_u128_andv` | `fio_u256_andv` | `fio_u512_andv` | `fio_u1024_andv` | `fio_u2048_andv` | `fio_u4096_andv` |
| OR | `fio_u128_orv` | `fio_u256_orv` | `fio_u512_orv` | `fio_u1024_orv` | `fio_u2048_orv` | `fio_u4096_orv` |
| ADD (64-bit) | `fio_u128_addv64` | `fio_u256_addv64` | `fio_u512_addv64` | `fio_u1024_addv64` | `fio_u2048_addv64` | `fio_u4096_addv64` |

**Note**: the `v` suffix distinguishes these value-returning functions from the pointer-based variants. The return value carries the `warn_unused_result` attribute (`FIO_MIFN`).

Example:

```c
fio_u256 a = fio_u256_init64(1, 2, 3, 4);
fio_u256 b = fio_u256_init64(5, 6, 7, 8);
fio_u256 result = fio_u256_xorv(a, b);
/* result.u64[0] == 1^5, result.u64[1] == 2^6, ... */
```

### Comparison and Utility (Pointer-Based)

For each size XXX (128, 256, 512, 1024, 2048, 4096):

#### `fio_uXXX_is_eq`

```c
bool fio_uXXX_is_eq(const fio_uXXX *a, const fio_uXXX *b);
```

Returns `true` if `a` and `b` are equal (all bits identical). Implemented as XOR + OR reduction -- constant time.

#### `fio_uXXX_inv`

```c
void fio_uXXX_inv(fio_uXXX *target, const fio_uXXX *a);
```

Bitwise NOT (inversion): `target = ~a`. Operates on 64-bit lanes.

#### `fio_uXXX_ct_swap_if`

```c
void fio_uXXX_ct_swap_if(bool cond, fio_uXXX *restrict a, fio_uXXX *restrict b);
```

Constant-time conditional swap. If `cond` is true, swaps the contents of `a` and `b`. If `cond` is false, `a` and `b` are unchanged. Uses XOR-mask technique to avoid branching.

**Note**: this is safe for use in cryptographic code where branch timing must not leak information.

#### Complete Function List (3 ops x 6 sizes = 18 functions)

| Operation | 128 | 256 | 512 | 1024 | 2048 | 4096 |
|-----------|-----|-----|-----|------|------|------|
| Equality | `fio_u128_is_eq` | `fio_u256_is_eq` | `fio_u512_is_eq` | `fio_u1024_is_eq` | `fio_u2048_is_eq` | `fio_u4096_is_eq` |
| Invert | `fio_u128_inv` | `fio_u256_inv` | `fio_u512_inv` | `fio_u1024_inv` | `fio_u2048_inv` | `fio_u4096_inv` |
| CT Swap | `fio_u128_ct_swap_if` | `fio_u256_ct_swap_if` | `fio_u512_ct_swap_if` | `fio_u1024_ct_swap_if` | `fio_u2048_ct_swap_if` | `fio_u4096_ct_swap_if` |

### Ternary Operations

These functions perform lane-wise ternary bitwise operations commonly used in cryptographic hash functions (SHA-1, SHA-2, etc.).

#### Formulas

| Operation | Formula | Cryptographic name |
|-----------|---------|-------------------|
| **mux** (choose) | `z ^ (x & (y ^ z))` | Ch (SHA-1/2) |
| **maj** (majority) | `(x & y) \| (z & (x \| y))` | Maj (SHA-1/2) |
| **3xor** (parity) | `x ^ y ^ z` | Parity (SHA-1) |

#### Signature Patterns

For each operation, three variants exist: 32-bit lanes, 64-bit lanes, and untyped (64-bit):

```c
/* 32-bit lane variant */
void fio_uXXX_mux32(fio_uXXX *t, const fio_uXXX *a, const fio_uXXX *b, const fio_uXXX *c);
/* 64-bit lane variant */
void fio_uXXX_mux64(fio_uXXX *t, const fio_uXXX *a, const fio_uXXX *b, const fio_uXXX *c);
/* Untyped (64-bit lane) variant */
void fio_uXXX_mux(fio_uXXX *t, const fio_uXXX *a, const fio_uXXX *b, const fio_uXXX *c);
```

#### Complete Function List (3 ops x 3 variants x 6 sizes = 54 functions)

| Operation | Variants | Sizes |
|-----------|----------|-------|
| `fio_uXXX_mux32`, `fio_uXXX_mux64`, `fio_uXXX_mux` | 32-bit, 64-bit, untyped | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_maj32`, `fio_uXXX_maj64`, `fio_uXXX_maj` | 32-bit, 64-bit, untyped | 128, 256, 512, 1024, 2048, 4096 |
| `fio_uXXX_3xor32`, `fio_uXXX_3xor64`, `fio_uXXX_3xor` | 32-bit, 64-bit, untyped | 128, 256, 512, 1024, 2048, 4096 |

### Multi-Precision Math

These functions provide arbitrary-precision integer arithmetic on `fio_uXXX` types. All operations assume LSW (Least Significant Word) ordering within the `.u64[]` array.

**Note**: `fio_uXXX_add`, `fio_uXXX_sub`, and `fio_uXXX_cmp` use `FIO_MIFN` (static inline, warn-unused-result). `fio_uXXX_mul` and `fio_uXXX_montgomery_mul` use `FIO_SFUNC` (static, **non-inline**), unlike the lane-wise operations which use `FIO_IFUNC` (always-inline). This means multiply and Montgomery multiply are compiled as regular function calls, which affects code generation and may prevent inlining in hot loops.

#### Addition and Subtraction

```c
uint64_t fio_uXXX_add(fio_uXXX *result, const fio_uXXX *a, const fio_uXXX *b);
uint64_t fio_uXXX_sub(fio_uXXX *result, const fio_uXXX *a, const fio_uXXX *b);
```

Performs multi-precision addition or subtraction. Returns the carry (for `add`) or borrow (for `sub`) as 1 or 0.

#### Comparison

```c
int fio_uXXX_cmp(const fio_uXXX *a, const fio_uXXX *b);
```

Returns `-1` if `a < b`, `0` if `a == b`, or `1` if `a > b`. Constant-time implementation.

#### Complete Add/Sub/Cmp List (3 ops x 6 sizes = 18 functions)

| Operation | 128 | 256 | 512 | 1024 | 2048 | 4096 |
|-----------|-----|-----|-----|------|------|------|
| Add | `fio_u128_add` | `fio_u256_add` | `fio_u512_add` | `fio_u1024_add` | `fio_u2048_add` | `fio_u4096_add` |
| Sub | `fio_u128_sub` | `fio_u256_sub` | `fio_u512_sub` | `fio_u1024_sub` | `fio_u2048_sub` | `fio_u4096_sub` |
| Cmp | `fio_u128_cmp` | `fio_u256_cmp` | `fio_u512_cmp` | `fio_u1024_cmp` | `fio_u2048_cmp` | `fio_u4096_cmp` |

#### Widening Multiply

```c
void fio_uXXX_mul(fio_uYYY *result, const fio_uXXX *a, const fio_uXXX *b);
```

Multiplies two `fio_uXXX` values and stores the full double-width result in a `fio_uYYY` (where `YYY = 2 * XXX`).

Available for sizes 128 through 2048 (5 functions):

| Function | Input type | Result type |
|----------|-----------|-------------|
| `fio_u128_mul` | `fio_u128` | `fio_u256` |
| `fio_u256_mul` | `fio_u256` | `fio_u512` |
| `fio_u512_mul` | `fio_u512` | `fio_u1024` |
| `fio_u1024_mul` | `fio_u1024` | `fio_u2048` |
| `fio_u2048_mul` | `fio_u2048` | `fio_u4096` |

**Note**: `fio_u4096_mul` is not available because there is no `fio_u8192` type to hold the result.

#### Montgomery Multiplication

```c
void fio_uXXX_montgomery_mul(fio_uXXX *result,
                              const fio_uXXX *a,
                              const fio_uXXX *b,
                              const fio_uXXX *N,
                              const fio_uXXX *N_dash);
```

Performs Montgomery modular multiplication: `result = (a * b * R^-1) mod N`, where `R = 2^bits` and `N_dash` is the Montgomery constant `N' = -N^-1 mod R`.

Available for the same sizes as widening multiply (5 functions):

| Function | Size |
|----------|------|
| `fio_u128_montgomery_mul` | 128-bit |
| `fio_u256_montgomery_mul` | 256-bit |
| `fio_u512_montgomery_mul` | 512-bit |
| `fio_u1024_montgomery_mul` | 1024-bit |
| `fio_u2048_montgomery_mul` | 2048-bit |

Example:

```c
/* Multi-precision addition with carry detection */
fio_u256 a = fio_u256_init64(0xFFFFFFFFFFFFFFFF, 0, 0, 0);
fio_u256 b = fio_u256_init64(1, 0, 0, 0);
fio_u256 result;
uint64_t carry = fio_u256_add(&result, &a, &b);
/* result.u64[0] == 0, result.u64[1] == 1, carry == 0 */

/* Widening multiply: 128-bit x 128-bit -> 256-bit */
fio_u128 x = fio_u128_init64(0x123456789ABCDEF0, 0);
fio_u128 y = fio_u128_init64(0xFEDCBA9876543210, 0);
fio_u256 product;
fio_u128_mul(&product, &x, &y);
```

### Native Numeral Vector Operations (Array-Based)

These operations work on plain C arrays of native types (not `fio_uXXX` unions). They are written with the hope that the compiler will replace the naive implementations with SIMD instructions where possible.

#### Integer Types and Lengths

| Type prefix | C type | Available lengths |
|-------------|--------|-------------------|
| `u8` | `uint8_t` | 4, 8, 16, 32, 64, 128, 256 |
| `u16` | `uint16_t` | 2, 4, 8, 16, 32, 64, 128 |
| `u32` | `uint32_t` | 2, 4, 8, 16, 32, 64 |
| `u64` | `uint64_t` | 2, 4, 8, 16, 32 |

#### Float Types and Lengths

| Type prefix | C type | Available lengths |
|-------------|--------|-------------------|
| `float` | `float` | 2, 4, 8, 16, 32, 64 |
| `dbl` | `double` | 2, 4, 8, 16, 32 |

#### Integer Operations

For each integer type/length combination, the following operations are available:

**Binary operations** (signature: `void fio_{type}x{len}_{op}({type} *dest, {type} *a, {type} *b)`):

| Operation | Operator | Description |
|-----------|----------|-------------|
| `add` | `+` | Element-wise addition |
| `sub` | `-` | Element-wise subtraction |
| `mul` | `*` | Element-wise multiplication |
| `and` | `&` | Element-wise bitwise AND |
| `or` | `\|` | Element-wise bitwise OR |
| `xor` | `^` | Element-wise bitwise XOR |

**Reduce operations** (signature: `{type} fio_{type}x{len}_reduce_{op}({type} *v)`):

| Operation | Description |
|-----------|-------------|
| `reduce_add` | Sum of all elements |
| `reduce_sub` | Sequential subtraction of all elements |
| `reduce_mul` | Product of all elements |
| `reduce_and` | Bitwise AND of all elements |
| `reduce_or` | Bitwise OR of all elements |
| `reduce_xor` | Bitwise XOR of all elements |
| `reduce_max` | Maximum element |
| `reduce_min` | Minimum element |

**Reshuffle** (macro: `fio_{type}x{len}_reshuffle(v, ...)`):

Reorders elements by index. The variadic arguments specify the source index for each destination position.

#### Float Operations

Float types have a reduced operation set:

**Binary operations**: `add`, `mul`

**Reduce operations**: `reduce_add`, `reduce_mul`, `reduce_max`, `reduce_min`

**Reshuffle**: `reshuffle`

**Note**: float types do NOT support `sub`, `and`, `or`, `xor`, `reduce_sub`, `reduce_and`, `reduce_or`, `reduce_xor`.

#### Naming Convention

All functions follow the pattern `fio_{type}x{len}_{op}(...)`.

Examples:

```c
uint32_t a[4] = {1, 2, 3, 4};
uint32_t b[4] = {5, 6, 7, 8};
uint32_t dest[4];

/* Element-wise addition */
fio_u32x4_add(dest, a, b);
/* dest = {6, 8, 10, 12} */

/* Horizontal sum */
uint32_t sum = fio_u32x4_reduce_add(a);
/* sum = 10 */

/* Reshuffle: reverse order */
fio_u32x4_reshuffle(a, 3, 2, 1, 0);
/* a = {4, 3, 2, 1} */
```

#### Reshuffle Macros

The reshuffle functions are shadowed by macros that convert variadic arguments into the required index array:

```c
#define fio_u8x4_reshuffle(v, ...)     fio_u8x4_reshuffle(v,     (uint8_t[4]){__VA_ARGS__})
#define fio_u16x4_reshuffle(v, ...)    fio_u16x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_u32x4_reshuffle(v, ...)    fio_u32x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
/* ... etc. for all type/length combinations */
```

Complete list of reshuffle macros:

| Type | Lengths |
|------|---------|
| `fio_u8xN_reshuffle` | 4, 8, 16, 32, 64, 128, 256 |
| `fio_u16xN_reshuffle` | 2, 4, 8, 16, 32, 64, 128 |
| `fio_u32xN_reshuffle` | 2, 4, 8, 16, 32, 64 |
| `fio_u64xN_reshuffle` | 2, 4, 8, 16, 32 |
| `fio_floatxN_reshuffle` | 2, 4, 8, 16, 32, 64 |
| `fio_dblxN_reshuffle` | 2, 4, 8, 16, 32 |

### FIO_VEC_* Macros

Generic vector operation macros that work with any array type. These use `FIO_FOR_UNROLL` internally for auto-vectorization.

#### Building Block Macros

#### `FIO_VEC_SIMPLE_OP`

```c
#define FIO_VEC_SIMPLE_OP(result, a, b, len, op)
```

Performs `result[i] = a[i] op b[i]` for `len` elements using `FIO_FOR_UNROLL`.

#### `FIO_VEC_SCALAR_OP`

```c
#define FIO_VEC_SCALAR_OP(result, v, sclr, len, op)
```

Performs `result[i] = v[i] op sclr` for `len` elements using `FIO_FOR_UNROLL`.

#### `FIO_VEC_REDUCE_OP`

```c
#define FIO_VEC_REDUCE_OP(result, vec, len, op)
```

Performs `result = result op vec[i]` for `len` elements using `FIO_FOR_UNROLL`.

**Note**: `result` should be initialized before calling (e.g., to 0 for addition, 1 for multiplication).

#### High-Level Vector Macros

#### `FIO_VEC_ADD`

```c
#define FIO_VEC_ADD(result, a, b, len)
```

Adds two vectors element-wise: `result[i] = a[i] + b[i]`.

#### `FIO_VEC_SUB`

```c
#define FIO_VEC_SUB(result, a, b, len)
```

Subtracts two vectors element-wise: `result[i] = a[i] - b[i]`.

#### `FIO_VEC_MUL`

```c
#define FIO_VEC_MUL(result, a, b, len)
```

Multiplies two vectors element-wise: `result[i] = a[i] * b[i]`.

#### `FIO_VEC_DOT`

```c
#define FIO_VEC_DOT(result, a, b, len)
```

Computes the dot product: `result += a[0]*b[0] + a[1]*b[1] + ... + a[len-1]*b[len-1]`.

**Note**: `result` should be initialized to 0 before calling.

#### `FIO_VEC_SCALAR_ADD`

```c
#define FIO_VEC_SCALAR_ADD(result, vec, scalar, vlen)
```

Adds a scalar to each element: `result[i] = vec[i] + scalar`.

#### `FIO_VEC_SCALAR_SUB`

```c
#define FIO_VEC_SCALAR_SUB(result, vec, scalar, vlen)
```

Subtracts a scalar from each element: `result[i] = vec[i] - scalar`.

#### `FIO_VEC_SCALAR_MUL`

```c
#define FIO_VEC_SCALAR_MUL(result, vec, scalar, vlen)
```

Multiplies each element by a scalar: `result[i] = vec[i] * scalar`.

#### `FIO_VEC_REDUCE_ADD`

```c
#define FIO_VEC_REDUCE_ADD(result, vec, vlen)
```

Sums all elements: `result = result + vec[0] + vec[1] + ...`.

#### `FIO_VEC_REDUCE_SUB`

```c
#define FIO_VEC_REDUCE_SUB(result, vec, vlen)
```

Subtracts all elements: `result = result - vec[0] - vec[1] - ...`.

#### `FIO_VEC_REDUCE_MUL`

```c
#define FIO_VEC_REDUCE_MUL(result, vec, vlen)
```

Multiplies all elements: `result = result * vec[0] * vec[1] * ...`.

Example:

```c
float a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
float b[8] = {8, 7, 6, 5, 4, 3, 2, 1};
float result[8];
float dot = 0;

/* Element-wise addition */
FIO_VEC_ADD(result, a, b, 8);

/* Dot product */
FIO_VEC_DOT(dot, a, b, 8);
/* dot = 1*8 + 2*7 + 3*6 + 4*5 + 5*4 + 6*3 + 7*2 + 8*1 = 120 */

/* Scale vector by 2.0 */
FIO_VEC_SCALAR_MUL(result, a, 2.0f, 8);
```

### Hex Read/Write

These functions convert between `fio_uXXX` types and hexadecimal string representations. They require `FIO_ATOL` to be defined before inclusion.

```c
#define FIO_ATOL
#include "fio-stl/include.h"
```

#### `fio_uXXX_hex_read`

```c
fio_uXXX fio_uXXX_hex_read(char **pstr);
```

Reads a hexadecimal numeral string and returns the parsed value as a `fio_uXXX`. Advances `*pstr` past the consumed hex characters.

#### `fio_uXXX_hex_write`

```c
size_t fio_uXXX_hex_write(char *dest, const fio_uXXX *u);
```

Writes the `fio_uXXX` value as a hexadecimal string to `dest`. Returns the number of characters written. Prints out the underlying 64-bit array representation.

**Note**: `dest` must have enough space for the full hex representation (XXX/4 hex digits plus potential prefix).

**Note**: this function is primarily intended for **debugging** purposes (the source describes it as "Prints out the underlying 64 bit array (for debugging)").

#### Complete Function List (2 ops x 6 sizes = 12 functions)

| Size | Read | Write |
|------|------|-------|
| 128 | `fio_u128_hex_read` | `fio_u128_hex_write` |
| 256 | `fio_u256_hex_read` | `fio_u256_hex_write` |
| 512 | `fio_u512_hex_read` | `fio_u512_hex_write` |
| 1024 | `fio_u1024_hex_read` | `fio_u1024_hex_write` |
| 2048 | `fio_u2048_hex_read` | `fio_u2048_hex_write` |
| 4096 | `fio_u4096_hex_read` | `fio_u4096_hex_write` |

### SIMD Backend Abstraction

The `fio_uXXX` types and their operations provide a portable SIMD abstraction. The same application code compiles to efficient platform-specific instructions without `#ifdef` guards.

#### How It Works

The `.xN[]` union members (`.x64`, `.x32`, `.x16`, `.x8`) are defined differently depending on the platform:

**ARM NEON** (when NEON + crypto intrinsics are available):
```c
uint64x2_t x64[bits / 128];  /* array of 128-bit NEON vectors */
uint32x4_t x32[bits / 128];
uint16x8_t x16[bits / 128];
uint8x16_t x8[bits / 128];
```

**GCC/Clang `vector_size`** (`__has_attribute(vector_size)`):
```c
uint64_t __attribute__((vector_size(bits / 8))) x64[1];  /* single full-width vector */
uint32_t __attribute__((vector_size(bits / 8))) x32[1];
uint16_t __attribute__((vector_size(bits / 8))) x16[1];
uint8_t  __attribute__((vector_size(bits / 8))) x8[1];
```

**Scalar fallback**:
```c
uint64_t x64[bits / 64];  /* plain arrays */
uint32_t x32[bits / 32];
uint16_t x16[bits / 16];
uint8_t  x8[bits / 8];
```

The `FIO_MATH_UXXX_OP` macro (and friends) iterate over `sizeof(t.xN) / sizeof(t.xN[0])` elements. This yields:

| Platform | Loop iterations for `fio_u256` with `.x64` |
|----------|---------------------------------------------|
| GCC `vector_size` | **1** (single 256-bit vector operation) |
| ARM NEON | **2** (two 128-bit NEON operations) |
| Scalar | **4** (four 64-bit scalar operations) |

This means the same `FIO_MATH_UXXX_OP(r, a, b, 64, ^)` call compiles to a single vector XOR on GCC, two `veorq_u64` on NEON, or four scalar XORs on the fallback -- all from the same source code.

### Examples

#### Initializing and Using `fio_u256`

```c
#include "fio-stl/include.h"

void example_init(void) {
  /* Initialize from 64-bit words (LSW first) */
  fio_u256 a = fio_u256_init64(0x0011223344556677,
                                0x8899AABBCCDDEEFF,
                                0x0000000000000001,
                                0x0000000000000000);

  /* Access individual elements */
  printf("First 64-bit word: 0x%016llx\n", (unsigned long long)a.u64[0]);
  printf("First byte: 0x%02x\n", a.u8[0]);

  /* Access as 128-bit halves */
  fio_u128 low_half = a.u128[0];
  fio_u128 high_half = a.u128[1];
}
```

#### Loading/Storing with Endian Conversion

```c
void example_endian(void) {
  /* Simulate receiving big-endian data from network */
  uint8_t network_data[32];
  /* ... fill from network ... */

  /* Load, converting each 64-bit word from big-endian to local */
  fio_u256 val = fio_u256_load_be64(network_data);

  /* Process in local endian... */
  fio_u256 mask = fio_u256_init64(0xFF, 0xFF, 0xFF, 0xFF);
  fio_u256 masked;
  fio_u256_and(&masked, &val, &mask);

  /* Store back to buffer in local endian */
  uint8_t local_buf[32];
  fio_u256_store(local_buf, masked);
}
```

#### Lane-Wise Operations and Multi-Precision Math

```c
void example_operations(void) {
  fio_u256 a = fio_u256_init64(0xFFFFFFFFFFFFFFFF, 0, 0, 0);
  fio_u256 b = fio_u256_init64(1, 0, 0, 0);
  fio_u256 result;

  /* Multi-precision addition (propagates carry across 64-bit words) */
  uint64_t carry = fio_u256_add(&result, &a, &b);
  /* result.u64 = {0, 1, 0, 0}, carry = 0 */

  /* Lane-wise XOR (no carry propagation, operates per-lane) */
  fio_u256 x = fio_u256_init64(0xAA, 0xBB, 0xCC, 0xDD);
  fio_u256 y = fio_u256_init64(0xFF, 0xFF, 0xFF, 0xFF);
  fio_u256_xor64(&result, &x, &y);
  /* result.u64 = {0x55, 0x44, 0x33, 0x22} */

  /* Value-returning XOR (same operation, different calling convention) */
  fio_u256 r = fio_u256_xorv(x, y);

  /* Comparison */
  if (fio_u256_is_eq(&result, &r)) {
    printf("Results are equal\n");
  }

  /* Constant-time conditional swap */
  bool should_swap = (fio_u256_cmp(&x, &y) > 0);
  fio_u256_ct_swap_if(should_swap, &x, &y);
  /* Now x <= y (constant-time, no branch) */
}
```

------------------------------------------------------------
