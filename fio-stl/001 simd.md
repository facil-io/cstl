# Fake x86 SIMD

```c
#define FIO_FAKE_X86 /* or FIO_FAKE_X86_SHADOW */
#include "fio-stl.h"
```

Portable x86 SIMD intrinsics for code that wants the x86 spell book even when the CPU does not. The implementation lives in [`./001 fx86.h`](./001%20fx86.h) and uses the core wide unions from [`./000 core.h`](./000%20core.h), especially `fio_u128` and `fio_u256`.

The module is compiled only when `FIO_FAKE_X86` or `FIO_FAKE_X86_SHADOW` is defined before including the STL.

### Configuration Macros

#### `FIO_FAKE_X86`

```c
#define FIO_FAKE_X86
```

Enables the namespaced fake-intrinsic API: `fio_fx86_*` and `fio_fx86_256_*`.

On real x86 with `FIO___HAS_X86_INTRIN`, supported functions pass through to native intrinsics when the matching feature macro exists (`__SSE2__`, `__AES__`, `__AVX2__`, and friends). Otherwise the fallback code copies through `fio_u128` / `fio_u256` and performs lane-wise C operations.

#### `FIO_FAKE_X86_SHADOW`

```c
#define FIO_FAKE_X86_SHADOW
```

Enables the same API as `FIO_FAKE_X86`, then shadows real intrinsic names with macros:

```c
#define __m128i fio_fx86_m128i
#define __m256i fio_fx86_m256i
#define _mm_xor_si128(a, b) fio_fx86_xor_si128((a), (b))
#define _mm256_xor_si256(a, b) fio_fx86_256_xor_si256((a), (b))
```

Shadow mode is meant for compiling x86 code paths on non-x86 machines. It also undefines ARM intrinsic guard macros such as `FIO___HAS_ARM_INTRIN`, `FIO___HAS_ARM_SHA3_INTRIN`, `__ARM_NEON`, and `__ARM_FEATURE_CRYPTO`, then defines the x86 feature guards used by downstream code (`FIO___HAS_X86_INTRIN`, `__SSE2__`, `__SSSE3__`, `__SSE4_1__`, `__AES__`, `__PCLMUL__`, `__SHA__`, `__SHA512__`, `__AVX2__`, etc.).

Shadowed names are macros. Keep arguments simple and avoid depending on compiler-specific intrinsic types outside the code path being shadowed.

#### `FIO___HAS_X86_INTRIN`

```c
#define FIO___HAS_X86_INTRIN 1
#include <immintrin.h>
```

Core detection flag from `000 core.h`. It is defined on x86_64 when intrinsics are allowed and `immintrin.h` is available.

Intrinsic detection is disabled when `DEBUG` or `NO_INTRIN` is defined.

#### `FIO___HAS_ARM_INTRIN`

```c
#define FIO___HAS_ARM_INTRIN 1
#include <arm_acle.h>
#include <arm_neon.h>
```

Core detection flag from `000 core.h`. It is defined when ARM crypto + NEON headers are available. `FIO_FAKE_X86_SHADOW` undefines it so fake x86 code paths can be selected instead of ARM paths.

#### `FIO_FOR_UNROLL_GROUP_SIZE`

```c
#define FIO_FOR_UNROLL_GROUP_SIZE ((size_t)256U)
```

The byte stride used by `FIO_FOR_UNROLL` for compiler-friendly batches.

Most CPUs should be able to hold at least 512 bytes in their registers. The default value assumes half that is available for the optimized loop, as the action within each loop will likely require some available registers.

#### `FIO_FOR_UNROLL`

```c
#define FIO_FOR_UNROLL(iterations, size_of_loop, i, action)                    \
  do {                                                                         \
    size_t i = 0;                                                              \
    const size_t fio___unroll_remainder__ =                                    \
        ((iterations) & ((FIO_FOR_UNROLL_GROUP_SIZE / (size_of_loop)) - 1));            \
    if (fio___unroll_remainder__)                                              \
      for (; i < fio___unroll_remainder__; ++i)                                \
        action;                                                                \
    if (iterations)                                                            \
      for (; !((iterations) + 1) || (i < (iterations));)                       \
        for (size_t j__loop__ = 0;                                             \
             j__loop__ < (FIO_FOR_UNROLL_GROUP_SIZE / (size_of_loop));                  \
             ++j__loop__, ++i)                                                 \
          action;                                                              \
  } while (0)
```

Runs a remainder loop, then fixed 256-byte batches. The macro declares `size_t i` inside the expansion; `action` may use that name.

#### `_MM_HINT_T0`, `_MM_HINT_T1`, `_MM_HINT_T2`, `_MM_HINT_NTA`

```c
#define _MM_HINT_T0  3
#define _MM_HINT_T1  2
#define _MM_HINT_T2  1
#define _MM_HINT_NTA 0
```

Defined in fake mode when native x86 types are not present, so `fio_fx86_prefetch` and shadowed `_mm_prefetch` calls can compile.

### Types

#### `fio_u128`

```c
typedef union fio_u128 {
  FIO___UXXX_UGRP_DEF(128);
#if defined(__SIZEOF_INT128__)
  __uint128_t alignment_for_u128_[1];
#endif
} fio_u128 FIO_ALIGN(16);
```

The 128-bit storage union used by the fallback implementation. It exposes unsigned, signed, floating-point, and vector views (`u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `x8`, `x16`, `x32`, `x64`, etc.).

#### `fio_u256`

```c
typedef union fio_u256 {
  fio_u128 u128[2];
  FIO___UXXX_UGRP_DEF(256);
#if defined(__SIZEOF_INT256__)
  __uint256_t alignment_for_u256_[1];
#endif
} fio_u256 FIO_ALIGN(32);
```

The 256-bit storage union used by AVX2 and SHA-512 fallbacks. It includes all `fio_u128`-style views plus `u128[2]`.

#### `FIO_UXXX_X64_DEF`, `FIO_UXXX_X32_DEF`, `FIO_UXXX_X16_DEF`, `FIO_UXXX_X8_DEF`

```c
#define FIO_UXXX_X64_DEF(name, bits) /* platform-specific */
#define FIO_UXXX_X32_DEF(name, bits) /* platform-specific */
#define FIO_UXXX_X16_DEF(name, bits) /* platform-specific */
#define FIO_UXXX_X8_DEF(name, bits)  /* platform-specific */
```

Core vector-member definition macros. On ARM they use NEON vector arrays. On GCC/Clang with `vector_size`, they use a single full-width vector. Otherwise they are plain C arrays. The fake x86 API itself uses `fio_u128` / `fio_u256` conversions instead of assuming a specific vector layout.

#### `fio_fx86_m128i`

```c
typedef __m128i fio_fx86_m128i;
/* or */
typedef fio_u128 fio_fx86_m128i;
```

The canonical 128-bit integer SIMD type for this module. It is native `__m128i` when available, otherwise `fio_u128`.

#### `fio_fx86_m256i`

```c
typedef __m256i fio_fx86_m256i;
/* or */
typedef fio_u256 fio_fx86_m256i;
```

The canonical 256-bit integer SIMD type for this module. It is native `__m256i` when available, otherwise `fio_u256`.

#### `FIO___FX86_HAS_NATIVE_M128I`, `FIO___FX86_HAS_NATIVE_M256I`, `FIO___FX86_HAS_NATIVE_TYPES`

```c
#define FIO___FX86_HAS_NATIVE_M128I 1 /* or 0 */
#define FIO___FX86_HAS_NATIVE_M256I 1 /* or 0 */
#define FIO___FX86_HAS_NATIVE_TYPES                                            \
  (FIO___FX86_HAS_NATIVE_M128I && FIO___FX86_HAS_NATIVE_M256I)
```

Internal feature flags used to choose native x86 types vs. `fio_uXXX` unions.

### API Functions

#### `fio___fx86_to_u128`

```c
FIO_IFUNC fio_u128 fio___fx86_to_u128(fio_fx86_m128i v);
```

Copies a `fio_fx86_m128i` into a `fio_u128` using `fio_memcpy16`. Use this when you need portable lane access and the type might be native `__m128i`.

#### `fio___fx86_from_u128`

```c
FIO_IFUNC fio_fx86_m128i fio___fx86_from_u128(fio_u128 u);
```

Copies a `fio_u128` into the canonical 128-bit fake x86 type.

#### `fio___fx86_to_u256`

```c
FIO_IFUNC fio_u256 fio___fx86_to_u256(fio_fx86_m256i v);
```

Copies a `fio_fx86_m256i` into a `fio_u256` using `fio_memcpy32`.

#### `fio___fx86_from_u256`

```c
FIO_IFUNC fio_fx86_m256i fio___fx86_from_u256(fio_u256 u);
```

Copies a `fio_u256` into the canonical 256-bit fake x86 type.

#### `fio_fx86_loadu_si128`, `fio_fx86_load_si128`, `fio_fx86_storeu_si128`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_loadu_si128(const void *p);
FIO_IFUNC fio_fx86_m128i fio_fx86_load_si128(const void *p);
FIO_IFUNC void fio_fx86_storeu_si128(void *p, fio_fx86_m128i a);
```

SSE2-style 128-bit load/store helpers. The fallback copies 16 bytes. The aligned load name mirrors `_mm_load_si128`; the fallback does not check alignment.

#### `fio_fx86_add_epi32`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_add_epi32(fio_fx86_m128i a,
                                            fio_fx86_m128i b);
```

Adds four unsigned 32-bit lanes with normal wraparound.

#### `fio_fx86_xor_si128`, `fio_fx86_or_si128`, `fio_fx86_and_si128`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_xor_si128(fio_fx86_m128i a,
                                            fio_fx86_m128i b);
FIO_IFUNC fio_fx86_m128i fio_fx86_or_si128(fio_fx86_m128i a,
                                           fio_fx86_m128i b);
FIO_IFUNC fio_fx86_m128i fio_fx86_and_si128(fio_fx86_m128i a,
                                            fio_fx86_m128i b);
```

Bitwise operations over the whole 128-bit value.

#### `fio_fx86_slli_si128`, `fio_fx86_srli_si128`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_slli_si128(fio_fx86_m128i a, int imm8);
FIO_IFUNC fio_fx86_m128i fio_fx86_srli_si128(fio_fx86_m128i a, int imm8);
```

Shifts the whole 128-bit register by bytes, filling with zero. Values greater than or equal to 16 return zero in the fallback.

#### `fio_fx86_slli_epi32`, `fio_fx86_srli_epi32`, `fio_fx86_slli_epi64`, `fio_fx86_srli_epi64`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_slli_epi32(fio_fx86_m128i a, int imm8);
FIO_IFUNC fio_fx86_m128i fio_fx86_srli_epi32(fio_fx86_m128i a, int imm8);
FIO_IFUNC fio_fx86_m128i fio_fx86_slli_epi64(fio_fx86_m128i a, int imm8);
FIO_IFUNC fio_fx86_m128i fio_fx86_srli_epi64(fio_fx86_m128i a, int imm8);
```

Per-lane logical shifts for 32-bit and 64-bit lanes. Fallback shifts at or past the lane width produce zero.

#### `fio_fx86_shuffle_epi32`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_shuffle_epi32(fio_fx86_m128i a, int imm8);
```

Selects each output 32-bit lane from `a` using two bits per lane from `imm8`.

#### `fio_fx86_cmpeq_epi8`, `fio_fx86_movemask_epi8`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_cmpeq_epi8(fio_fx86_m128i a,
                                             fio_fx86_m128i b);
FIO_IFUNC int fio_fx86_movemask_epi8(fio_fx86_m128i a);
```

Byte equality produces `0xFF` or `0x00` per byte. The movemask packs the high bit of each byte into a 16-bit integer result.

#### `fio_fx86_set_epi32`, `fio_fx86_set_epi64x`, `fio_fx86_set_epi8`, `fio_fx86_set1_epi8`, `fio_fx86_setzero_si128`, `fio_fx86_cvtsi32_si128`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_set_epi32(int e3, int e2, int e1, int e0);
FIO_IFUNC fio_fx86_m128i fio_fx86_set_epi64x(int64_t e1, int64_t e0);
FIO_IFUNC fio_fx86_m128i fio_fx86_set_epi8(char e15, char e14, char e13,
                                           char e12, char e11, char e10,
                                           char e9, char e8, char e7, char e6,
                                           char e5, char e4, char e3, char e2,
                                           char e1, char e0);
FIO_IFUNC fio_fx86_m128i fio_fx86_set1_epi8(char a);
FIO_IFUNC fio_fx86_m128i fio_fx86_setzero_si128(void);
FIO_IFUNC fio_fx86_m128i fio_fx86_cvtsi32_si128(int a);
```

SSE2 constructors. The `set` functions follow x86 argument order: high lanes first, low lanes last.

#### `fio_fx86_shuffle_epi8`, `fio_fx86_alignr_epi8`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_shuffle_epi8(fio_fx86_m128i a,
                                               fio_fx86_m128i b);
FIO_IFUNC fio_fx86_m128i fio_fx86_alignr_epi8(fio_fx86_m128i a,
                                              fio_fx86_m128i b,
                                              int imm8);
```

SSSE3 helpers. `shuffle_epi8` performs byte lookup, zeroing output bytes whose control byte has bit 7 set. `alignr_epi8` concatenates `b:a`, shifts right by `imm8` bytes, and returns 16 bytes.

#### `fio_fx86_blend_epi16`, `fio_fx86_extract_epi32`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_blend_epi16(fio_fx86_m128i a,
                                              fio_fx86_m128i b,
                                              int imm8);
FIO_IFUNC int fio_fx86_extract_epi32(fio_fx86_m128i a, int imm8);
```

SSE4.1 helpers. `blend_epi16` selects each 16-bit lane from `a` or `b` by `imm8` bits. `extract_epi32` returns lane `imm8 & 3`.

#### `fio_fx86_prefetch`

```c
FIO_IFUNC void fio_fx86_prefetch(const void *p, int hint);
```

Prefetches on native SSE2. It is a no-op in fake mode.

#### `fio_fx86_aesenc_si128`, `fio_fx86_aesenclast_si128`, `fio_fx86_aeskeygenassist_si128`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_aesenc_si128(fio_fx86_m128i a,
                                               fio_fx86_m128i roundkey);
FIO_IFUNC fio_fx86_m128i fio_fx86_aesenclast_si128(fio_fx86_m128i a,
                                                   fio_fx86_m128i roundkey);
FIO_IFUNC fio_fx86_m128i fio_fx86_aeskeygenassist_si128(fio_fx86_m128i a,
                                                        int imm8);
```

AES-NI helpers. The fallback implements AES round logic with an S-box table. It is correct for portability tests, but the table lookups are cache-dependent; do not treat fake AES-NI as constant-time production crypto.

#### `fio_fx86_clmulepi64_si128`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_clmulepi64_si128(fio_fx86_m128i a,
                                                   fio_fx86_m128i b,
                                                   int imm8);
```

PCLMULQDQ carry-less multiplication. `imm8 & 1` selects the 64-bit lane from `a`; `(imm8 >> 4) & 1` selects the lane from `b`. The result is a 128-bit GF(2) product.

#### `fio_fx86_sha256rnds2_epu32`, `fio_fx86_sha256msg1_epu32`, `fio_fx86_sha256msg2_epu32`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_sha256rnds2_epu32(fio_fx86_m128i cdgh,
                                                    fio_fx86_m128i abef,
                                                    fio_fx86_m128i msg);
FIO_IFUNC fio_fx86_m128i fio_fx86_sha256msg1_epu32(fio_fx86_m128i msg0,
                                                   fio_fx86_m128i msg1);
FIO_IFUNC fio_fx86_m128i fio_fx86_sha256msg2_epu32(fio_fx86_m128i msg4,
                                                   fio_fx86_m128i msg1);
```

SHA-NI SHA-256 helpers. The round function expects `cdgh`, `abef`, and message-plus-constant layout matching Intel SHA extensions.

#### `fio_fx86_sha1rnds4_epu32`, `fio_fx86_sha1nexte_epu32`, `fio_fx86_sha1msg1_epu32`, `fio_fx86_sha1msg2_epu32`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_sha1rnds4_epu32(fio_fx86_m128i abcd,
                                                  fio_fx86_m128i e0,
                                                  int func);
FIO_IFUNC fio_fx86_m128i fio_fx86_sha1nexte_epu32(fio_fx86_m128i a,
                                                  fio_fx86_m128i b);
FIO_IFUNC fio_fx86_m128i fio_fx86_sha1msg1_epu32(fio_fx86_m128i msg0,
                                                 fio_fx86_m128i msg1);
FIO_IFUNC fio_fx86_m128i fio_fx86_sha1msg2_epu32(fio_fx86_m128i msg5,
                                                 fio_fx86_m128i msg1);
```

SHA-NI SHA-1 helpers. `func & 3` selects Ch, Parity, Maj, or Parity for `sha1rnds4`.

#### `fio_fx86_256_loadu_si256`, `fio_fx86_256_load_si256`, `fio_fx86_256_storeu_si256`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_loadu_si256(const void *p);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_load_si256(const void *p);
FIO_IFUNC void fio_fx86_256_storeu_si256(void *p, fio_fx86_m256i a);
```

AVX2-style 256-bit load/store helpers. The fallback copies 32 bytes.

#### `fio_fx86_256_add_epi16`, `fio_fx86_256_sub_epi16`, `fio_fx86_256_mullo_epi16`, `fio_fx86_256_add_epi32`, `fio_fx86_256_sub_epi32`, `fio_fx86_256_mullo_epi32`, `fio_fx86_256_add_epi64`, `fio_fx86_256_sub_epi64`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_add_epi16(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_sub_epi16(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_mullo_epi16(fio_fx86_m256i a,
                                                  fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_add_epi32(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_sub_epi32(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_mullo_epi32(fio_fx86_m256i a,
                                                  fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_add_epi64(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_sub_epi64(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
```

AVX2 packed integer arithmetic. Operations wrap by lane width; `mullo` keeps the low half of each product.

#### `fio_fx86_256_slli_epi32`, `fio_fx86_256_srai_epi32`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_slli_epi32(fio_fx86_m256i a, int imm8);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_srai_epi32(fio_fx86_m256i a, int imm8);
```

Per-lane 32-bit shifts. `srai` is arithmetic right shift; fallback values at or past 32 bits become the sign bit extended value.

#### `fio_fx86_256_xor_si256`, `fio_fx86_256_and_si256`, `fio_fx86_256_or_si256`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_xor_si256(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_and_si256(fio_fx86_m256i a,
                                                fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_or_si256(fio_fx86_m256i a,
                                               fio_fx86_m256i b);
```

Bitwise operations over the whole 256-bit value.

#### `fio_fx86_256_cmpeq_epi8`, `fio_fx86_256_movemask_epi8`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_cmpeq_epi8(fio_fx86_m256i a,
                                                 fio_fx86_m256i b);
FIO_IFUNC int fio_fx86_256_movemask_epi8(fio_fx86_m256i a);
```

AVX2 byte equality and 32-bit byte movemask.

#### `fio_fx86_256_set1_epi8`, `fio_fx86_256_set1_epi16`, `fio_fx86_256_set1_epi32`, `fio_fx86_256_set1_epi64x`, `fio_fx86_256_set_epi64x`, `fio_fx86_256_set_epi8`, `fio_fx86_256_setzero_si256`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_set1_epi8(char a);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_set1_epi16(short a);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_set1_epi32(int a);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_set1_epi64x(long long a);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_set_epi64x(long long e3,
                                                 long long e2,
                                                 long long e1,
                                                 long long e0);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_set_epi8(char e31, char e30, char e29,
                                               char e28, char e27, char e26,
                                               char e25, char e24, char e23,
                                               char e22, char e21, char e20,
                                               char e19, char e18, char e17,
                                               char e16, char e15, char e14,
                                               char e13, char e12, char e11,
                                               char e10, char e9, char e8,
                                               char e7, char e6, char e5,
                                               char e4, char e3, char e2,
                                               char e1, char e0);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_setzero_si256(void);
```

AVX2 constructors. Like Intel intrinsics, the explicit `set` functions receive high lanes first and low lanes last.

#### `fio_fx86_256_shuffle_epi8`, `fio_fx86_256_alignr_epi8`, `fio_fx86_256_permute4x64_epi64`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_shuffle_epi8(fio_fx86_m256i a,
                                                   fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_alignr_epi8(fio_fx86_m256i a,
                                                  fio_fx86_m256i b,
                                                  int imm8);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_permute4x64_epi64(fio_fx86_m256i a,
                                                        int imm8);
```

AVX2 shuffle/permute helpers. `shuffle_epi8` and `alignr_epi8` operate inside each 128-bit lane. `permute4x64_epi64` selects among the four 64-bit lanes across the full 256-bit value.

#### `fio_fx86_256_packs_epi32`, `fio_fx86_256_cvtepi16_epi32`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_packs_epi32(fio_fx86_m256i a,
                                                  fio_fx86_m256i b);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_cvtepi16_epi32(fio_fx86_m128i a);
```

AVX2 pack/convert helpers. `packs_epi32` saturates 32-bit signed lanes to 16-bit signed lanes, per 128-bit lane. `cvtepi16_epi32` sign-extends eight 16-bit values from the low 128 bits into eight 32-bit lanes.

#### `fio_fx86_256_castsi256_si128`, `fio_fx86_256_extracti128_si256`

```c
FIO_IFUNC fio_fx86_m128i fio_fx86_256_castsi256_si128(fio_fx86_m256i a);
FIO_IFUNC fio_fx86_m128i fio_fx86_256_extracti128_si256(fio_fx86_m256i a,
                                                        int imm8);
```

Extract 128-bit halves from a 256-bit value. `extracti128` uses `imm8 & 1`.

#### `fio_fx86_256_sha512rnds2_epi64`, `fio_fx86_256_sha512msg1_epi64`, `fio_fx86_256_sha512msg2_epi64`

```c
FIO_IFUNC fio_fx86_m256i fio_fx86_256_sha512rnds2_epi64(fio_fx86_m256i state1,
                                                        fio_fx86_m256i state0,
                                                        fio_fx86_m256i wk);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_sha512msg1_epi64(fio_fx86_m256i w0,
                                                       fio_fx86_m256i w1);
FIO_IFUNC fio_fx86_m256i fio_fx86_256_sha512msg2_epi64(fio_fx86_m256i w0,
                                                       fio_fx86_m256i w1);
```

Intel SHA-512 extension helpers using four 64-bit lanes.

### Shadowed Intrinsic Families

#### `_mm_*`

With `FIO_FAKE_X86_SHADOW`, these SSE/SHA/AES names are mapped to the matching `fio_fx86_*` functions:

```c
_mm_loadu_si128, _mm_load_si128, _mm_storeu_si128,
_mm_add_epi32,
_mm_xor_si128, _mm_or_si128, _mm_and_si128,
_mm_slli_si128, _mm_srli_si128,
_mm_slli_epi32, _mm_srli_epi32,
_mm_slli_epi64, _mm_srli_epi64,
_mm_shuffle_epi32, _mm_shuffle_epi8, _mm_alignr_epi8,
_mm_cmpeq_epi8, _mm_movemask_epi8,
_mm_set_epi32, _mm_set_epi64x, _mm_set_epi8,
_mm_set1_epi8, _mm_setzero_si128, _mm_cvtsi32_si128,
_mm_blend_epi16, _mm_extract_epi32, _mm_prefetch,
_mm_aesenc_si128, _mm_aesenclast_si128, _mm_aeskeygenassist_si128,
_mm_clmulepi64_si128,
_mm_sha256rnds2_epu32, _mm_sha256msg1_epu32, _mm_sha256msg2_epu32,
_mm_sha1rnds4_epu32, _mm_sha1nexte_epu32,
_mm_sha1msg1_epu32, _mm_sha1msg2_epu32
```

#### `_mm256_*`

With `FIO_FAKE_X86_SHADOW`, these AVX2/SHA-512 names are mapped to the matching `fio_fx86_256_*` functions:

```c
_mm256_loadu_si256, _mm256_load_si256, _mm256_storeu_si256,
_mm256_add_epi16, _mm256_sub_epi16, _mm256_mullo_epi16,
_mm256_add_epi32, _mm256_sub_epi32, _mm256_mullo_epi32,
_mm256_add_epi64, _mm256_sub_epi64,
_mm256_slli_epi32, _mm256_srai_epi32,
_mm256_xor_si256, _mm256_and_si256, _mm256_or_si256,
_mm256_cmpeq_epi8, _mm256_movemask_epi8,
_mm256_set1_epi8, _mm256_set1_epi16, _mm256_set1_epi32,
_mm256_set1_epi64x, _mm256_set_epi64x, _mm256_set_epi8,
_mm256_setzero_si256,
_mm256_shuffle_epi8, _mm256_alignr_epi8, _mm256_permute4x64_epi64,
_mm256_packs_epi32, _mm256_cvtepi16_epi32,
_mm256_castsi256_si128, _mm256_extracti128_si256,
_mm256_sha512rnds2_epi64, _mm256_sha512msg1_epi64,
_mm256_sha512msg2_epi64
```

### Examples

#### 128-bit add

```c
#define FIO_FAKE_X86
#include "fio-stl.h"
#include <stdint.h>

int main(void) {
  uint32_t a_in[4] = {1, 2, 3, 4};
  uint32_t b_in[4] = {10, 20, 30, 40};
  uint32_t out[4];

  fio_fx86_m128i a = fio_fx86_loadu_si128(a_in);
  fio_fx86_m128i b = fio_fx86_loadu_si128(b_in);
  fio_fx86_m128i c = fio_fx86_add_epi32(a, b);
  fio_fx86_storeu_si128(out, c);

  return (out[0] == 11 && out[3] == 44) ? 0 : 1;
}
```

#### AVX2-style byte equality

```c
#define FIO_FAKE_X86
#include "fio-stl.h"
#include <stdint.h>

int main(void) {
  uint8_t a[32] = {0};
  uint8_t b[32] = {0};
  b[7] = 1;

  fio_fx86_m256i va = fio_fx86_256_loadu_si256(a);
  fio_fx86_m256i vb = fio_fx86_256_loadu_si256(b);
  fio_fx86_m256i eq = fio_fx86_256_cmpeq_epi8(va, vb);
  int mask = fio_fx86_256_movemask_epi8(eq);

  return ((mask & (1 << 7)) == 0) ? 0 : 1;
}
```

#### Shadowing x86 names

```c
#define FIO_FAKE_X86_SHADOW
#include "fio-stl.h"
#include <stdint.h>

int main(void) {
  uint32_t in[4] = {0x01020304, 0, 0, 0};
  uint32_t out[4];

  __m128i v = _mm_loadu_si128((const __m128i *)in);
  v = _mm_slli_epi32(v, 8);
  _mm_storeu_si128((__m128i *)out, v);

  return out[0] == 0x02030400 ? 0 : 1;
}
```

### Safety Notes

#### Memory ownership

The fake x86 API does not allocate or free memory. Load and store functions copy fixed-size byte ranges from caller-owned buffers.

#### Thread safety

Functions are value-based and use only caller-provided memory plus read-only tables. They are thread-safe when callers synchronize shared buffers as usual.

#### Constant-time behavior

The portable AES fallback uses lookup tables and is not constant-time. SHA and PCLMUL fallbacks are portable C implementations intended for correctness and test coverage, not a promise of native-intrinsic timing.

#### Macro expansion

`FIO_FOR_UNROLL` declares loop variables inside the macro and expands `action` many times. Shadow-mode `_mm*` names are macros that call the `fio_fx86*` functions; avoid side-effect-heavy arguments even when they appear to be evaluated once today.
