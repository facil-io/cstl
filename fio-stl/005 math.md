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

-------------------------------------------------------------------------------
