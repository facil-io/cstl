## Multi-Precision Math

```c
#define FIO_MATH
#include "fio-stl.h"
```

If `FIO_MATH` is defined, some building blocks for multi-precision math will be provided as well as some naive implementations of simple multi-precision operation that focus on constant time (security) rather than performance.

Note that this implementation assumes that the CPU performs MUL in constant time (which may or may not be true).

### Multi-Precision Math Building Blocks

The following simple operations can be used to build your own multi-precision implementation.

#### `fio_math_addc64`

```c
uint64_t fio_math_addc64(uint64_t a,
                         uint64_t b,
                         uint64_t carry_in,
                         uint64_t *carry_out);
```

Add with carry.

#### `fio_math_subc64`

```c
uint64_t fio_math_subc64(uint64_t a,
                         uint64_t b,
                         uint64_t carry_in,
                         uint64_t *carry_out);
```

Subtract with carry.

#### `fio_math_mulc64`
```c
uint64_t fio_math_mulc64(uint64_t a, uint64_t b, uint64_t *carry_out);
```

Multiply with carry out.

### Multi-Precision Helper Types

The following union types hold (little endian) arrays of unsigned 64 bit numbers that are accessible also as byte arrays or smaller numeral types.


#### `fio_u128`

```c
typedef union {
  uint8_t u8[16];
  uint16_t u16[8];
  uint32_t u32[4];
  uint64_t u64[2];
  __uint128_t u128[1]; /* if supported by the compiler */
} fio_u128;
```

An unsigned 128 bit union type.

#### `fio_u256`

```c
typedef union {
  uint8_t u8[32];
  uint16_t u16[16];
  uint32_t u32[8];
  uint64_t u64[4];
  __uint128_t u128[2]; /* if supported by the compiler */
  __uint256_t u256[1]; /* if supported by the compiler */
} fio_u256;
```

An unsigned 256 bit union type.

#### `fio_u512`

```c
typedef union {
  uint8_t u8[64];
  uint16_t u16[32];
  uint32_t u32[16];
  uint64_t u64[8];
  __uint128_t u128[4]; /* if supported by the compiler */
  __uint256_t u256[2]; /* if supported by the compiler */
} fio_u512;
```

An unsigned 512 bit union type.

### Multi-Precision Math with Little Endian arrays

The following, somewhat naive, multi-precision math implementation focuses on constant time. It assumes an array of local endian 64bit numbers ordered within the array in little endian (word `0` contains the least significant bits and word `n-1` contains the most significant bits).


#### `fio_math_add`

```c
uint64_t fio_math_add(uint64_t *dest,
                      const uint64_t *a,
                      const uint64_t *b,
                      const size_t number_array_length);
```

Multi-precision ADD for `len*64` bit long a + b. Returns the carry.

#### `fio_math_sub`

```c
uint64_t fio_math_sub(uint64_t *dest,
                      const uint64_t *a,
                      const uint64_t *b,
                      const size_t number_array_length);
```

Multi-precision SUB for `len*64` bit long a + b. Returns the carry.

#### `fio_math_mul`

```c
void fio_math_mul(uint64_t *restrict dest,
                  const uint64_t *a,
                  const uint64_t *b,
                  const size_t number_array_length);
```

Multi-precision MUL for `len*64` bit long a, b. `dest` must be `len*2` long or buffer overflows will occur.

#### `fio_math_div`
```c
void fio_math_div(uint64_t *dest,
                  uint64_t *reminder,
                  const uint64_t *a,
                  const uint64_t *b,
                  const size_t number_array_length);
```

Multi-precision DIV for `len*64` bit long a, b.

This is **NOT constant time**.

The algorithm might be slow, as my math isn't that good and I couldn't understand faster division algorithms (such as Newton–Raphson division)... so this is sort of a factorized variation on long division.

#### `fio_math_shr`

```c
void fio_math_shr(uint64_t *dest,
                  uint64_t *n,
                  const size_t right_shift_bits,
                  size_t number_array_length);
```

Multi-precision shift right for `len` word number `n`.

#### `fio_math_shl`

```c
void fio_math_shl(uint64_t *dest,
                  uint64_t *n,
                  const size_t left_shift_bits,
                  const size_t number_array_length);
```

Multi-precision shift left for `len*64` bit number `n`.

#### `fio_math_inv`

```c
void fio_math_inv(uint64_t *dest, uint64_t *n, size_t len);
````

Multi-precision Inverse for `len*64` bit number `n` (i.e., turns `1` into `-1`).

#### `fio_math_msb_index`

```c
size_t fio_math_msb_index(uint64_t *n, const size_t len);
````

Multi-precision - returns the index for the most significant bit or -1.

This can be used to collect a number's bit length.

#### `fio_math_lsb_index`

```c
size_t fio_math_lsb_index(uint64_t *n, const size_t len);
````

Multi-precision - returns the index for the least significant bit or -1.

This can be used to extract an exponent value in base 2.

### Vector Helper Types

#### `fio_v128`

```c
typedef union fio_v128 {
#if __has_attribute(vector_size)
  uint64_t __attribute__((vector_size(16))) v64;
  uint32_t __attribute__((vector_size(16))) v32;
  uint16_t __attribute__((vector_size(16))) v16;
  uint8_t __attribute__((vector_size(16))) v8;
#else
  uint64_t v64[2];
  uint32_t v32[4];
  uint16_t v16[8];
  uint8_t v8[16];
#endif
} fio_v128;
```

This is a 128 bit vector type that can be used for portable vector operations (implemented as regular math if vector instructions are unavailable).

#### `fio_v256`

```c
typedef union fio_v256 {
#if __has_attribute(vector_size)
  uint64_t __attribute__((vector_size(32))) v64;
  uint32_t __attribute__((vector_size(32))) v32;
  uint16_t __attribute__((vector_size(32))) v16;
  uint8_t __attribute__((vector_size(32))) v8;
#else
  uint64_t v64[4];
  uint32_t v32[8];
  uint16_t v16[16];
  uint8_t v8[32];
#endif
} fio_v256;
```

This is a 256 bit vector type that can be used for portable vector operations (implemented as regular math if vector instructions are unavailable).

### Vector Helpers

Vector helper functions start with the type of the vector. i.e. `fio_v128`. Next the operation name and the bit grouping are stated, i.e. `_mul64`.

When a vector operation is applied to a constant, the operation is prefixed with a `c`, i.e., `_cllro32`.

For example, `fio_v128_clrot32` is a constant left rotation on 32 bit groups within a 128 bit vector.

`fio_vxxx_load`, `fio_vxxx_load_le32` and `fio_vxxx_load_le64` functions are also provided.

The following functions are available:

* `fio_vxxx_mul##(vec_a, vec_b)` - performs the `mul` operation (`*`).
* `fio_vxxx_add##(vec_a, vec_b)` - performs the `add` operation (`+`).
* `fio_vxxx_sub##(vec_a, vec_b)` - performs the `sub` operation (`-`).
* `fio_vxxx_div##(vec_a, vec_b)` - performs the `div` operation (`/`).
* `fio_vxxx_reminder##(vec_a, vec_b)` - performs the `reminder` operation (`%`).


* `fio_vxxx_cmul##(vec, single_element)` - performs the `mul` operation (`*`).
* `fio_vxxx_cadd##(vec, single_element)` - performs the `add` operation (`+`).
* `fio_vxxx_csub##(vec, single_element)` - performs the `sub` operation (`-`).
* `fio_vxxx_cdiv##(vec, single_element)` - performs the `div` operation (`/`).
* `fio_vxxx_creminder##(vec, single_element)` - performs the `reminder` operation (`%`).

* `fio_vxxx_and##(vec_a, vec_b)` - performs the `and` operation (`&`).
* `fio_vxxx_or##(vec_a, vec_b)` - performs the `or` operation (`|`).
* `fio_vxxx_xor##(vec_a, vec_b)` - performs the `xor` operation (`^`).

* `fio_vxxx_cand##(vec, single_element)` - performs the `and` operation (`&`).
* `fio_vxxx_cor##(vec, single_element)` - performs the `or` operation (`|`).
* `fio_vxxx_cxor##(vec, single_element)` - performs the `xor` operation (`^`).

* `fio_vxxx_flip##(vec)` - performs the `flip` bit operation (`~`).

* `fio_vxxx_shuffle##(vec, index0, index1...)` - performs a limited `shuffle` operation on a single vector, reordering its members.

* `fio_vxxx_load(const void * buffer)` loads data from the buffer, returning a properly aligned vector.

* `fio_vxxx_load_le32(const void * buffer)` loads data from the buffer, returning a properly aligned vector. This variation performs a 4 byte `bswap` operation on each 32 bit group, so the data is loaded using little endian rather than network endian.

* `fio_vxxx_load_le64(const void * buffer)` loads data from the buffer, returning a properly aligned vector. This variation performs an 4 byte `bswap` operation on each 64 bit group, so the data is loaded using little endian rather than network endian.


Example use:

```c
fio_v256 const prime = {.v64 = {FIO_STABLE_HASH_PRIME0,
                                FIO_STABLE_HASH_PRIME1,
                                FIO_STABLE_HASH_PRIME2,
                                FIO_STABLE_HASH_PRIME3}};
fio_v256 v = fio_v256_load(buf);
v = fio_v256_mul64(v, prime);
v = fio_v256_xor64(v, fio_v256_clrot64(v, 31));
// ...
```

**Note**: although `fio_v256` is defined, it isn't fully implemented and is missing both `shuffle` and `load` helpers.

-------------------------------------------------------------------------------
