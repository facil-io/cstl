## Multi-Precision Math

```c
#define FIO_MATH
#include "fio-stl.h"
```

When requiring more than the core multi-precision ADD, SUB and MUL operations, `FIO_MATH` provides commonly used (yet more advanced) operations such as division, bit shifting, and bit index detection.

Note that this implementation assumes that the CPU performs MUL in constant time (which may or may not be true).

**Note**: The core multi-precision math building blocks (`fio_math_add`, `fio_math_sub`, `fio_math_mul`, `fio_math_addc64`, etc.) and the vector helper types (`fio_u128`, `fio_u256`, etc.) are always available as part of the core module and do not require `FIO_MATH` to be defined. See the [Core Module documentation](000 core.md) for details on those functions.

-------------------------------------------------------------------------------

### Multi-Precision Math with Little Endian Arrays

The following multi-precision math implementation focuses on constant time operations (where possible). It assumes an array of local endian 64-bit numbers ordered within the array in little endian (word `0` contains the least significant bits and word `n-1` contains the most significant bits).

#### `fio_math_div`

```c
void fio_math_div(uint64_t *dest,
                  uint64_t *reminder,
                  const uint64_t *a,
                  const uint64_t *b,
                  const size_t number_array_length);
```

Multi-precision DIV for `len*64` bit long numbers `a` and `b`.

Computes `a / b`, storing the quotient in `dest` and the remainder in `reminder`.

**Parameters:**
- `dest` - destination array for the quotient (may be NULL if only remainder is needed)
- `reminder` - destination array for the remainder (may be NULL if only quotient is needed)
- `a` - the dividend (numerator)
- `b` - the divisor (denominator)
- `number_array_length` - the number of 64-bit words in each array

**Note**: This is **NOT** constant time.

**Note**: The algorithm might be slow, as it uses a factorized variation on long division rather than faster algorithms like Newton-Raphson division.

#### `fio_math_shr`

```c
void fio_math_shr(uint64_t *dest,
                  uint64_t *n,
                  const size_t right_shift_bits,
                  size_t number_array_length);
```

Multi-precision shift right for a `len` word number `n`.

Shifts the multi-precision number `n` right by `right_shift_bits` bits, storing the result in `dest`.

**Parameters:**
- `dest` - destination array for the result
- `n` - the number to shift
- `right_shift_bits` - number of bits to shift right
- `number_array_length` - the number of 64-bit words in the arrays

#### `fio_math_shl`

```c
void fio_math_shl(uint64_t *dest,
                  uint64_t *n,
                  const size_t left_shift_bits,
                  const size_t number_array_length);
```

Multi-precision shift left for a `len*64` bit number `n`.

Shifts the multi-precision number `n` left by `left_shift_bits` bits, storing the result in `dest`.

**Parameters:**
- `dest` - destination array for the result
- `n` - the number to shift
- `left_shift_bits` - number of bits to shift left
- `number_array_length` - the number of 64-bit words in the arrays

#### `fio_math_inv`

```c
void fio_math_inv(uint64_t *dest, uint64_t *n, size_t len);
```

Multi-precision two's complement inverse for a `len*64` bit number `n`.

Computes the two's complement negation (i.e., turns `1` into `-1` in two's complement representation).

**Parameters:**
- `dest` - destination array for the result
- `n` - the number to invert
- `len` - the number of 64-bit words in the arrays

#### `fio_math_msb_index`

```c
size_t fio_math_msb_index(uint64_t *n, const size_t len);
```

Multi-precision - returns the index for the most significant bit, or `(size_t)-1` if the number is zero.

This can be used to determine a number's bit length.

**Parameters:**
- `n` - the multi-precision number
- `len` - the number of 64-bit words in the array

**Returns:** The zero-based index of the most significant set bit, or `(size_t)-1` if all bits are zero.

#### `fio_math_lsb_index`

```c
size_t fio_math_lsb_index(uint64_t *n, const size_t len);
```

Multi-precision - returns the index for the least significant bit, or `(size_t)-1` if the number is zero.

This can be used to extract an exponent value in base 2.

**Parameters:**
- `n` - the multi-precision number
- `len` - the number of 64-bit words in the array

**Returns:** The zero-based index of the least significant set bit, or `(size_t)-1` if all bits are zero.

-------------------------------------------------------------------------------
