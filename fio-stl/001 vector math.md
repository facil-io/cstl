# Vector Math

Wide integer unions and lane-wise arithmetic helpers defined in [`./000 core.h`](./000%20core.h). These types are used for SIMD-style processing, big-integer arithmetic, hashing, and cryptography.

## Wide Union Types

These unions expose the same data through arrays of different widths, plus platform-specific vector views when SIMD is available. They are sized and aligned as follows:

| Type | Size | Alignment |
|------|------|-----------|
| `fio_u128`  | 16 bytes  | 16 |
| `fio_u256`  | 32 bytes  | 32 |
| `fio_u512`  | 64 bytes  | 64 |
| `fio_u1024` | 128 bytes | 64 |
| `fio_u2048` | 256 bytes | 64 |
| `fio_u4096` | 512 bytes | 64 |

All types include these overlapping members (counts vary by element width):

```c
size_t      uz[N];    ssize_t     iz[N];
uint64_t    u64[N];   int64_t     i64[N];
uint32_t    u32[N];   int32_t     i32[N];
uint16_t    u16[N];   int16_t     i16[N];
uint8_t     u8[N];    int8_t      i8[N];
float       f[N];
double      d[N];
long double ld[total_bytes / sizeof(long double)];
```

`total_bytes` is the type size from the table above, and each `N` is `total_bytes / sizeof(element)`. Larger types also contain smaller ones, for example `fio_u1024` provides `u128[8]`, `u256[4]`, and `u512[2]` in addition to the generic arrays.

When the compiler supports GCC/Clang `vector_size` attributes or ARM NEON intrinsics, the unions also expose vector views named `x64`, `x32`, `x16`, and `x8`. The internal layout adapts to the platform:

- On GCC/Clang, `x64[1]` is a single full-width vector.
- On ARM NEON, `x64[]` is an array of 128-bit `uint64x2_t` vectors.
- On platforms without vector support, `x64[]` falls back to a plain `uint64_t` array.

## Initialization Macros

Initialize a wide union from a list of lane values. Unused lanes are zeroed by the compiler.

```c
fio_u128_init8(0x01, 0x02, ...)
fio_u128_init16(...)
fio_u128_init32(...)
fio_u128_init64(...)

fio_u256_init8(...)
fio_u256_init16(...)
fio_u256_init32(...)
fio_u256_init64(...)

fio_u512_init8(...)
fio_u512_init16(...)
fio_u512_init32(...)
fio_u512_init64(...)

fio_u1024_init8(...)
fio_u1024_init16(...)
fio_u1024_init32(...)
fio_u1024_init64(...)

fio_u2048_init8(...)
fio_u2048_init16(...)
fio_u2048_init32(...)
fio_u2048_init64(...)

fio_u4096_init8(...)
fio_u4096_init16(...)
fio_u4096_init32(...)
fio_u4096_init64(...)
```

## Load, Store, and Byte Swap

All loads and stores copy whole vectors to and from memory using the fixed-size `fio_memcpyNN` helpers.

```c
fio_u128  fio_u128_load(const void *buf);
void      fio_u128_store(void *buf, const fio_u128 a);

fio_u256  fio_u256_load(const void *buf);
void      fio_u256_store(void *buf, const fio_u256 a);

fio_u512  fio_u512_load(const void *buf);
void      fio_u512_store(void *buf, const fio_u512 a);

fio_u1024 fio_u1024_load(const void *buf);
void      fio_u1024_store(void *buf, const fio_u1024 a);

fio_u2048 fio_u2048_load(const void *buf);
void      fio_u2048_store(void *buf, const fio_u2048 a);

fio_u4096 fio_u4096_load(const void *buf);
void      fio_u4096_store(void *buf, const fio_u4096 a);
```

Endian-aware load helpers read each lane from the given byte order and convert it to local order. Store variants are not provided here; use the byte-swap helpers before `store` if needed.

```c
fio_u128  fio_u128_load_le16(const void *buf);
fio_u128  fio_u128_load_be16(const void *buf);
fio_u128  fio_u128_load_le32(const void *buf);
fio_u128  fio_u128_load_be32(const void *buf);
fio_u128  fio_u128_load_le64(const void *buf);
fio_u128  fio_u128_load_be64(const void *buf);

fio_u256  fio_u256_load_le16(...); fio_u256  fio_u256_load_be16(...);
fio_u256  fio_u256_load_le32(...); fio_u256  fio_u256_load_be32(...);
fio_u256  fio_u256_load_le64(...); fio_u256  fio_u256_load_be64(...);

fio_u512  fio_u512_load_le16(...); fio_u512  fio_u512_load_be16(...);
fio_u512  fio_u512_load_le32(...); fio_u512  fio_u512_load_be32(...);
fio_u512  fio_u512_load_le64(...); fio_u512  fio_u512_load_be64(...);

fio_u1024 fio_u1024_load_le16(...); fio_u1024 fio_u1024_load_be16(...);
fio_u1024 fio_u1024_load_le32(...); fio_u1024 fio_u1024_load_be32(...);
fio_u1024 fio_u1024_load_le64(...); fio_u1024 fio_u1024_load_be64(...);

fio_u2048 fio_u2048_load_le16(...); fio_u2048 fio_u2048_load_be16(...);
fio_u2048 fio_u2048_load_le32(...); fio_u2048 fio_u2048_load_be32(...);
fio_u2048 fio_u2048_load_le64(...); fio_u2048 fio_u2048_load_be64(...);

fio_u4096 fio_u4096_load_le16(...); fio_u4096 fio_u4096_load_be16(...);
fio_u4096 fio_u4096_load_le32(...); fio_u4096 fio_u4096_load_be32(...);
fio_u4096 fio_u4096_load_le64(...); fio_u4096 fio_u4096_load_be64(...);
```

Byte-swap every lane of a vector, returning the result by value:

```c
fio_u128  fio_u128_bswap16(fio_u128 a);
fio_u128  fio_u128_bswap32(fio_u128 a);
fio_u128  fio_u128_bswap64(fio_u128 a);

fio_u256  fio_u256_bswap16(...);  fio_u256  fio_u256_bswap32(...);  fio_u256  fio_u256_bswap64(...);
fio_u512  fio_u512_bswap16(...);  fio_u512  fio_u512_bswap32(...);  fio_u512  fio_u512_bswap64(...);
fio_u1024 fio_u1024_bswap16(...); fio_u1024 fio_u1024_bswap32(...); fio_u1024 fio_u1024_bswap64(...);
fio_u2048 fio_u2048_bswap16(...); fio_u2048 fio_u2048_bswap32(...); fio_u2048 fio_u2048_bswap64(...);
fio_u4096 fio_u4096_bswap16(...); fio_u4096 fio_u4096_bswap32(...); fio_u4096 fio_u4096_bswap64(...);
```

## Lane-Wise Arithmetic

The arithmetic functions write lane-wise results into `*target`. They work on any `fio_uXXX` type at 8, 16, 32, and 64-bit lane widths. The compiler vectorizes the loops when SIMD is available.

```c
void fio_u128_add8(fio_u128 *target, const fio_u128 *a, const fio_u128 *b);
void fio_u128_add16(fio_u128 *target, const fio_u128 *a, const fio_u128 *b);
void fio_u128_add32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b);
void fio_u128_add64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b);

void fio_u128_sub8(...); void fio_u128_sub16(...); void fio_u128_sub32(...); void fio_u128_sub64(...);
void fio_u128_mul8(...); void fio_u128_mul16(...); void fio_u128_mul32(...); void fio_u128_mul64(...);
void fio_u128_and8(...); void fio_u128_and16(...); void fio_u128_and32(...); void fio_u128_and64(...);
void fio_u128_or8(...);  void fio_u128_or16(...);  void fio_u128_or32(...);  void fio_u128_or64(...);
void fio_u128_xor8(...); void fio_u128_xor16(...); void fio_u128_xor32(...); void fio_u128_xor64(...);
```

These are generated for all wide types: `fio_u256`, `fio_u512`, `fio_u1024`, `fio_u2048`, and `fio_u4096`.

Constant-operand variants apply the same operation with a scalar in place of `*b`:

```c
void fio_u128_cadd8(fio_u128 *target, const fio_u128 *a, uint8_t  b);
void fio_u128_cadd16(fio_u128 *target, const fio_u128 *a, uint16_t b);
void fio_u128_cadd32(fio_u128 *target, const fio_u128 *a, uint32_t b);
void fio_u128_cadd64(fio_u128 *target, const fio_u128 *a, uint64_t b);

void fio_u128_csub8(...); void fio_u128_csub16(...); void fio_u128_csub32(...); void fio_u128_csub64(...);
void fio_u128_cmul8(...); void fio_u128_cmul16(...); void fio_u128_cmul32(...); void fio_u128_cmul64(...);
void fio_u128_cand8(...); void fio_u128_cand16(...); void fio_u128_cand32(...); void fio_u128_cand64(...);
void fio_u128_cor8(...);  void fio_u128_cor16(...);  void fio_u128_cor32(...);  void fio_u128_cor64(...);
void fio_u128_cxor8(...); void fio_u128_cxor16(...); void fio_u128_cxor32(...); void fio_u128_cxor64(...);
```

Reduction functions fold all lanes of the given width into a single scalar:

```c
uint8_t  fio_u128_reduce_add8(const fio_u128 *a);
uint16_t fio_u128_reduce_add16(const fio_u128 *a);
uint32_t fio_u128_reduce_add32(const fio_u128 *a);
uint64_t fio_u128_reduce_add64(const fio_u128 *a);

uint8_t  fio_u128_reduce_sub8(...);  uint16_t fio_u128_reduce_sub16(...);
uint32_t fio_u128_reduce_sub32(...); uint64_t fio_u128_reduce_sub64(...);
uint8_t  fio_u128_reduce_mul8(...);  uint16_t fio_u128_reduce_mul16(...);
uint32_t fio_u128_reduce_mul32(...); uint64_t fio_u128_reduce_mul64(...);
uint8_t  fio_u128_reduce_and8(...);  uint16_t fio_u128_reduce_and16(...);
uint32_t fio_u128_reduce_and32(...); uint64_t fio_u128_reduce_and64(...);
uint8_t  fio_u128_reduce_or8(...);   uint16_t fio_u128_reduce_or16(...);
uint32_t fio_u128_reduce_or32(...);  uint64_t fio_u128_reduce_or64(...);
uint8_t  fio_u128_reduce_xor8(...);  uint16_t fio_u128_reduce_xor16(...);
uint32_t fio_u128_reduce_xor32(...); uint64_t fio_u128_reduce_xor64(...);
```

Whole-vector variants that operate on 64-bit lanes without a width suffix are also provided for bitwise operations:

```c
void fio_u128_and(fio_u128 *target, const fio_u128 *a, const fio_u128 *b);
void fio_u128_or (fio_u128 *target, const fio_u128 *a, const fio_u128 *b);
void fio_u128_xor(fio_u128 *target, const fio_u128 *a, const fio_u128 *b);
```

## Rotations

Lane-wise right and left rotations. Variable rotations read the per-lane shift from an array; constant rotations use a single `uint8_t` for all lanes.

```c
void fio_u128_rrot8 (fio_u128 *target, const fio_u128 *a, const uint8_t rotations[16]);
void fio_u128_rrot16(fio_u128 *target, const fio_u128 *a, const uint8_t rotations[8]);
void fio_u128_rrot32(fio_u128 *target, const fio_u128 *a, const uint8_t rotations[4]);
void fio_u128_rrot64(fio_u128 *target, const fio_u128 *a, const uint8_t rotations[2]);

void fio_u128_lrot8 (fio_u128 *target, const fio_u128 *a, const uint8_t rotations[16]);
void fio_u128_lrot16(fio_u128 *target, const fio_u128 *a, const uint8_t rotations[8]);
void fio_u128_lrot32(fio_u128 *target, const fio_u128 *a, const uint8_t rotations[4]);
void fio_u128_lrot64(fio_u128 *target, const fio_u128 *a, const uint8_t rotations[2]);

void fio_u128_crrot8 (fio_u128 *target, const fio_u128 *a, const uint8_t bts);
void fio_u128_crrot16(fio_u128 *target, const fio_u128 *a, const uint8_t bts);
void fio_u128_crrot32(fio_u128 *target, const fio_u128 *a, const uint8_t bts);
void fio_u128_crrot64(fio_u128 *target, const fio_u128 *a, const uint8_t bts);

void fio_u128_clrot8 (fio_u128 *target, const fio_u128 *a, const uint8_t bts);
void fio_u128_clrot16(fio_u128 *target, const fio_u128 *a, const uint8_t bts);
void fio_u128_clrot32(fio_u128 *target, const fio_u128 *a, const uint8_t bts);
void fio_u128_clrot64(fio_u128 *target, const fio_u128 *a, const uint8_t bts);
```

All rotation families exist for `fio_u256`, `fio_u512`, `fio_u1024`, `fio_u2048`, and `fio_u4096` with appropriate array sizes.

## Ternary Lane-Wise Operations

These compute common three-input bit functions across each lane.

```c
// target[i] = z[i] ^ (x[i] & (y[i] ^ z[i]))
void fio_u128_mux32(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);
void fio_u128_mux64(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);

// target[i] = (x[i] & y[i]) | (z[i] & (x[i] | y[i]))
void fio_u128_maj32(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);
void fio_u128_maj64(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);

// target[i] = x[i] ^ y[i] ^ z[i]
void fio_u128_3xor32(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);
void fio_u128_3xor64(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);
```

Whole-vector 64-bit variants without a width suffix are also provided:

```c
void fio_u128_mux(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);
void fio_u128_maj(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);
void fio_u128_3xor(fio_u128 *target, const fio_u128 *x, const fio_u128 *y, const fio_u128 *z);
```

All ternary functions exist for the other wide types: `fio_u256`, `fio_u512`, `fio_u1024`, `fio_u2048`, and `fio_u4096`.

## Whole-Vector Value Operations

A small set of convenience functions return results by value instead of writing through a pointer. They operate on 64-bit lanes and are designed to let the compiler use SIMD directly.

```c
fio_u128 fio_u128_xorv(fio_u128 a, fio_u128 b);
fio_u128 fio_u128_andv(fio_u128 a, fio_u128 b);
fio_u128 fio_u128_orv (fio_u128 a, fio_u128 b);
fio_u128 fio_u128_addv64(fio_u128 a, fio_u128 b);

fio_u256 fio_u256_xorv(...);  fio_u256 fio_u256_andv(...);  fio_u256 fio_u256_orv(...);  fio_u256 fio_u256_addv64(...);
fio_u512 fio_u512_xorv(...);  fio_u512 fio_u512_andv(...);  fio_u512 fio_u512_orv(...);  fio_u512 fio_u512_addv64(...);
fio_u1024 fio_u1024_xorv(...); fio_u1024 fio_u1024_andv(...); fio_u1024 fio_u1024_orv(...); fio_u1024 fio_u1024_addv64(...);
fio_u2048 fio_u2048_xorv(...); fio_u2048 fio_u2048_andv(...); fio_u2048 fio_u2048_orv(...); fio_u2048 fio_u2048_addv64(...);
fio_u4096 fio_u4096_xorv(...); fio_u4096 fio_u4096_andv(...); fio_u4096 fio_u4096_orv(...); fio_u4096 fio_u4096_addv64(...);
```

## Constant-Time Helpers

```c
bool fio_u128_is_eq(const fio_u128 *a, const fio_u128 *b);
```

Returns `true` if every byte of `*a` equals the corresponding byte of `*b`, otherwise `false`. The comparison is branchless over the vector body.

```c
void fio_u128_inv(fio_u128 *target, const fio_u128 *a);
```

Bitwise NOT: `target[i] = ~a[i]` on 64-bit lanes.

```c
void fio_u128_ct_swap_if(bool cond, fio_u128 *a, fio_u128 *b);
```

Swaps `*a` and `*b` when `cond` is true, without branching on `cond`. Uses a mask derived from `(uint64_t)0 - cond`.

All three helpers exist for `fio_u256`, `fio_u512`, `fio_u1024`, `fio_u2048`, and `fio_u4096`.

## Multi-Precision Math

These functions treat the wide unions as unsigned big integers made of little-endian `u64` words.

```c
uint64_t fio_u128_add(fio_u128 *result, const fio_u128 *a, const fio_u128 *b);
uint64_t fio_u128_sub(fio_u128 *result, const fio_u128 *a, const fio_u128 *b);
int      fio_u128_cmp(const fio_u128 *a, const fio_u128 *b);

uint64_t fio_u256_add(...); uint64_t fio_u256_sub(...); int fio_u256_cmp(...);
uint64_t fio_u512_add(...); uint64_t fio_u512_sub(...); int fio_u512_cmp(...);
uint64_t fio_u1024_add(...); uint64_t fio_u1024_sub(...); int fio_u1024_cmp(...);
uint64_t fio_u2048_add(...); uint64_t fio_u2048_sub(...); int fio_u2048_cmp(...);
uint64_t fio_u4096_add(...); uint64_t fio_u4096_sub(...); int fio_u4096_cmp(...);
```

- `fio_uXXX_add` returns the carry bit (`1` or `0`).
- `fio_uXXX_sub` returns the borrow bit (`1` or `0`).
- `fio_uXXX_cmp` returns `-1` if `a < b`, `0` if equal, and `1` if `a > b`.

Multiplication doubles the width of the result:

```c
void fio_u128_mul(fio_u256 *result, const fio_u128 *a, const fio_u128 *b);
void fio_u256_mul(fio_u512 *result, const fio_u256 *a, const fio_u256 *b);
void fio_u512_mul(fio_u1024 *result, const fio_u512 *a, const fio_u512 *b);
void fio_u1024_mul(fio_u2048 *result, const fio_u1024 *a, const fio_u1024 *b);
void fio_u2048_mul(fio_u4096 *result, const fio_u2048 *a, const fio_u2048 *b);
```

Montgomery multiplication is provided for RSA-like modular arithmetic:

```c
void fio_u128_montgomery_mul(fio_u128 *result, const fio_u128 *a, const fio_u128 *b,
                             const fio_u128 *N, const fio_u128 *N_dash);
void fio_u256_montgomery_mul(fio_u256 *result, const fio_u256 *a, const fio_u256 *b,
                             const fio_u256 *N, const fio_u256 *N_dash);
void fio_u512_montgomery_mul(fio_u512 *result, const fio_u512 *a, const fio_u512 *b,
                             const fio_u512 *N, const fio_u512 *N_dash);
void fio_u1024_montgomery_mul(fio_u1024 *result, const fio_u1024 *a, const fio_u1024 *b,
                              const fio_u1024 *N, const fio_u1024 *N_dash);
void fio_u2048_montgomery_mul(fio_u2048 *result, const fio_u2048 *a, const fio_u2048 *b,
                              const fio_u2048 *N, const fio_u2048 *N_dash);
```

The multiplication uses `fio_math_mul` from [`./002 math.h`](./002%20math.h).

## Shuffle and Reduce on Native Arrays

A separate family of helpers works on plain C arrays of fixed size. They are useful when the data is not wrapped in a `fio_uXXX` union.

### Shuffle

```c
void fio_u8x4_reshuffle  (uint8_t  *v, uint8_t indx[4]);
void fio_u8x8_reshuffle  (uint8_t  *v, uint8_t indx[8]);
void fio_u8x16_reshuffle (uint8_t  *v, uint8_t indx[16]);
void fio_u8x32_reshuffle (uint8_t  *v, uint8_t indx[32]);
void fio_u8x64_reshuffle (uint8_t  *v, uint8_t indx[64]);
void fio_u8x128_reshuffle(uint8_t  *v, uint8_t indx[128]);
void fio_u8x256_reshuffle(uint8_t  *v, uint8_t indx[256]);

void fio_u16x2_reshuffle (uint16_t *v, uint8_t indx[2]);
void fio_u16x4_reshuffle (uint16_t *v, uint8_t indx[4]);
void fio_u16x8_reshuffle (uint16_t *v, uint8_t indx[8]);
void fio_u16x16_reshuffle(uint16_t *v, uint8_t indx[16]);
void fio_u16x32_reshuffle(uint16_t *v, uint8_t indx[32]);
void fio_u16x64_reshuffle(uint16_t *v, uint8_t indx[64]);
void fio_u16x128_reshuffle(uint16_t *v, uint8_t indx[128]);

void fio_u32x2_reshuffle (uint32_t *v, uint8_t indx[2]);
void fio_u32x4_reshuffle (uint32_t *v, uint8_t indx[4]);
void fio_u32x8_reshuffle (uint32_t *v, uint8_t indx[8]);
void fio_u32x16_reshuffle(uint32_t *v, uint8_t indx[16]);
void fio_u32x32_reshuffle(uint32_t *v, uint8_t indx[32]);
void fio_u32x64_reshuffle(uint32_t *v, uint8_t indx[64]);

void fio_u64x2_reshuffle (uint64_t *v, uint8_t indx[2]);
void fio_u64x4_reshuffle (uint64_t *v, uint8_t indx[4]);
void fio_u64x8_reshuffle (uint64_t *v, uint8_t indx[8]);
void fio_u64x16_reshuffle(uint64_t *v, uint8_t indx[16]);
void fio_u64x32_reshuffle(uint64_t *v, uint8_t indx[32]);

void fio_floatx2_reshuffle (float  *v, uint8_t indx[2]);
void fio_floatx4_reshuffle (float  *v, uint8_t indx[4]);
void fio_floatx8_reshuffle (float  *v, uint8_t indx[8]);
void fio_floatx16_reshuffle(float  *v, uint8_t indx[16]);
void fio_floatx32_reshuffle(float  *v, uint8_t indx[32]);
void fio_floatx64_reshuffle(float  *v, uint8_t indx[64]);

void fio_dblx2_reshuffle  (double *v, uint8_t indx[2]);
void fio_dblx4_reshuffle  (double *v, uint8_t indx[4]);
void fio_dblx8_reshuffle  (double *v, uint8_t indx[8]);
void fio_dblx16_reshuffle (double *v, uint8_t indx[16]);
void fio_dblx32_reshuffle (double *v, uint8_t indx[32]);
```

Indices are masked with `len - 1`, so out-of-range indices wrap safely.

Variadic macro wrappers let you pass the index list inline:

```c
fio_u8x4_reshuffle(v, 3, 2, 1, 0);
fio_u32x4_reshuffle(v, 0, 2, 1, 3);
```

### Reduce

Integer reductions provide add, subtract, multiply, bitwise and/or/xor, and min/max:

```c
uint8_t  fio_u8x4_reduce_add (uint8_t  *v);  uint8_t  fio_u8x4_reduce_sub (uint8_t  *v);
uint8_t  fio_u8x4_reduce_mul (uint8_t  *v);  uint8_t  fio_u8x4_reduce_and (uint8_t  *v);
uint8_t  fio_u8x4_reduce_or  (uint8_t  *v);  uint8_t  fio_u8x4_reduce_xor (uint8_t  *v);
uint8_t  fio_u8x4_reduce_max (uint8_t  *v);  uint8_t  fio_u8x4_reduce_min (uint8_t  *v);

// Same pattern for u8 x 8, 16, 32, 64, 128, 256
// Same pattern for u16 x 2, 4, 8, 16, 32, 64, 128
// Same pattern for u32 x 2, 4, 8, 16, 32, 64
// Same pattern for u64 x 2, 4, 8, 16, 32
```

Floating-point reductions provide add, multiply, and min/max (no bitwise operations):

```c
float fio_floatx2_reduce_add(float *v);   float fio_floatx2_reduce_mul(float *v);
float fio_floatx2_reduce_max(float *v);   float fio_floatx2_reduce_min(float *v);

// Same pattern for float x 4, 8, 16, 32, 64
// Same pattern for dbl x 2, 4, 8, 16, 32
```

### Lane-wise operations on native arrays

Each integer reduction family also defines matching in-place binary operators on the arrays themselves:

```c
void fio_u8x4_add (uint8_t  *dest, uint8_t  *a, uint8_t  *b);
void fio_u8x4_sub (uint8_t  *dest, uint8_t  *a, uint8_t  *b);
void fio_u8x4_mul (uint8_t  *dest, uint8_t  *a, uint8_t  *b);
void fio_u8x4_and (uint8_t  *dest, uint8_t  *a, uint8_t  *b);
void fio_u8x4_or  (uint8_t  *dest, uint8_t  *a, uint8_t  *b);
void fio_u8x4_xor (uint8_t  *dest, uint8_t  *a, uint8_t  *b);

// Same pattern for all u8/u16/u32/u64 widths and lengths.
```

Floating-point array families define add and multiply only:

```c
void fio_floatx2_add(float *dest, float *a, float *b);
void fio_floatx2_mul(float *dest, float *a, float *b);

// Same pattern for float x 4, 8, 16, 32, 64 and dbl x 2, 4, 8, 16, 32.
```

## See Also

- [`./000 core.h`](./000%20core.h) — source of truth for all vector definitions.
- [`./000 core.md`](./000%20core.md) — core documentation index.
- `001 fx86.h` — fake/portable x86 intrinsics built on these types.
- [`./002 math.h`](./002%20math.h) — multi-precision `fio_math_mul` and related helpers.
