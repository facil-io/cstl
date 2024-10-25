## Multi-Precision Math

```c
#define FIO_MATH
#include "fio-stl.h"
```

When requiring more than multi-precision ADD, SUB and MUL, `FIO_MATH` attempts to provide some commonly used (yet more advanced) operations.

Note that this implementation assumes that the CPU performs MUL in constant time (which may or may not be true).

### Multi-Precision Math with Little Endian arrays

The following, somewhat naive, multi-precision math implementation focuses on constant time. It assumes an array of local endian 64bit numbers ordered within the array in little endian (word `0` contains the least significant bits and word `n-1` contains the most significant bits).


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

### Vector Math Helpers

Vector helper functions start with the type of the vector. i.e. `fio_u128`. Next the operation name and the bit grouping are stated, i.e. `fio_u256_mul64`.

When a vector operation is applied to a constant, the operation is prefixed with a `c`, i.e., `fio_u128_clrot32`.

`fio_uxxx_load`, `fio_uxxx_load_le32` and `fio_uxxx_load_le64` functions are also provided.

The following functions are available:

* `fio_uxxx_mul##(vec_a, vec_b)` - performs the `mul` operation (`*`).
* `fio_uxxx_add##(vec_a, vec_b)` - performs the `add` operation (`+`).
* `fio_uxxx_sub##(vec_a, vec_b)` - performs the `sub` operation (`-`).
* `fio_uxxx_div##(vec_a, vec_b)` - performs the `div` operation (`/`).
* `fio_uxxx_reminder##(vec_a, vec_b)` - performs the `reminder` operation (`%`).


* `fio_uxxx_cmul##(vec, single_element)` - performs the `mul` operation (`*`).
* `fio_uxxx_cadd##(vec, single_element)` - performs the `add` operation (`+`).
* `fio_uxxx_csub##(vec, single_element)` - performs the `sub` operation (`-`).
* `fio_uxxx_cdiv##(vec, single_element)` - performs the `div` operation (`/`).
* `fio_uxxx_creminder##(vec, single_element)` - performs the `reminder` operation (`%`).

* `fio_uxxx_and(vec_a, vec_b)` - performs the `and` operation (`&`) on the whole vector.
* `fio_uxxx_or(vec_a, vec_b)` - performs the `or` operation (`|`) on the whole vector.
* `fio_uxxx_xor(vec_a, vec_b)` - performs the `xor` operation (`^`) on the whole vector.

* `fio_uxxx_cand##(vec, single_element)` - performs the `and` operation (`&`) with an X bit constant.
* `fio_uxxx_cor##(vec, single_element)` - performs the `or` operation (`|`) with an X bit constant.
* `fio_uxxx_cxor##(vec, single_element)` - performs the `xor` operation (`^`) with an X bit constant.

* `fio_uxxx_flip(vec)` - performs the `flip` bit operation (`~`).

* `fio_uxxx_shuffle##(vec, index0, index1...)` - performs a limited `shuffle` operation on a single vector, reordering its members.

* `fio_uxxx_load(const void * buffer)` loads data from the buffer, returning a properly aligned vector.

* `fio_uxxx_load_le##(const void * buffer)` loads data from the buffer, returning a properly aligned vector. This variation performs a `bswap` operation on each bit group, so the data is loaded using little endian rather than local endian.

* `fio_uxxx_load_be##(const void * buffer)` loads data from the buffer, returning a properly aligned vector. This variation performs a `bswap` operation on each bit group, so the data is loaded using big endian rather than local endian.

Example use:

```c
fio_u256 const prime = {.u64 = {FIO_U64_HASH_PRIME0,
                                FIO_U64_HASH_PRIME1,
                                FIO_U64_HASH_PRIME2,
                                FIO_U64_HASH_PRIME3}};
fio_u256 u = fio_u256_load(buf), tmp;
u = fio_u256_mul64(u, prime);
tmp = fio_u256_clrot64(u, 31);
u = fio_u256_xor64(u, tmp);
// ...
```

**Note**: the implementation is portable and doesn't currently use any compiler vector builtins or types. We pray to the optimization gods instead (which don't always listen) and depend on compilation flags.

#### Numeral Array Shuffling

Numeral vector / array shuffling is available the numeral types `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, as well as the `float` and `double` types.

Vector / array shuffling is available for any combinations of up to 256 bytes (i.e., `8x256` or `64x32`).

The naming convention is `fio_PRxL_reshuffle` where `PR` is either `u8`, `u16`, `u32`, `u64`, `float` or `dbl` and `L` is the length of the array in number of elements.

**Note**: The use of **re**shuffle denotes that the shuffling occurs in-place, replacing current data (unlike the `fio_uxxx_shuffle##` math functions).

i.e.: 

```c
void fio_u64x4_reshuffle(uint64_t * v, uint8_t[4]);
void fio_u64x8_reshuffle(uint64_t * v, uint8_t[8]);
void fio_u64x16_reshuffle(uint64_t *v, uint8_t[16]);
#define fio_u64x4_reshuffle(v, ...)  fio_u64x4_reshuffle(v,  (uint8_t[4]){__VA_ARGS__})
#define fio_u64x8_reshuffle(v, ...)  fio_u64x8_reshuffle(v,  (uint8_t[8]){__VA_ARGS__})
#define fio_u64x16_reshuffle(v, ...) fio_u64x16_reshuffle(v, (uint8_t[16]){__VA_ARGS__})
```

#### Numeral Array Reduction

Numeral vector / array reduction is available the numeral types `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, as well as the `float` and `double` types.

Vector / array reduction is available for any combinations of up to 256 bytes (i.e., `8x256` or `64x32`).

The naming convention is `fio_PRxL_reduce_OP` where:

- `PR` is either `u8`, `u16`, `u32`, `u64`, `float` or `dbl`

-  `L` is the length of the array in number of elements.

- `OP` is the operation to be performed, one of: `add`, `mul` `xor`, `or`, `and`. Note that for `float` and `double` types, only `add` and `mul` are available.

i.e.: 

```c
uint64_t fio_u64x4_reduce_add(uint64_t * v);
uint64_t fio_u64x8_reduce_xor(uint64_t * v);
uint64_t fio_u64x16_reduce_and(uint64_t * v);
```

-------------------------------------------------------------------------------
