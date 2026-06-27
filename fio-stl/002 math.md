# Multi-Precision Math

```c
#define FIO_MATH
#include "fio-stl.h"
```

Extra multi-precision operations on little-endian `uint64_t` arrays: division, shifts, two's-complement inverse, and bit-index queries. The core add/sub/mul helpers (`fio_math_add`, `fio_math_sub`, `fio_math_mul`, `fio_math_addc64`, etc.) and the wide union types (`fio_u128`, `fio_u256`, …) live in [`000 core.h`](./000%20core.md) and do not require `FIO_MATH`.

**Note:** this module assumes the CPU performs 64-bit multiply in constant time, which is not guaranteed on all hardware.

### API Functions

#### `fio_math_div`

```c
FIO_IFUNC void fio_math_div(uint64_t *dest,
                            uint64_t *reminder,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t number_array_length);
```

Divides the `number_array_length`-word number `a` by `b`, storing the quotient in `dest` and the remainder in `reminder`. Either output may be `NULL` if you only need the other one.

**Note:** not constant time. Uses binary long division, which is simple but not the fastest algorithm on earth.

If `b` is zero, logs an error and fills both outputs with `0xFF`.

#### `fio_math_shr`

```c
FIO_IFUNC void fio_math_shr(uint64_t *dest,
                            uint64_t *n,
                            const size_t right_shift_bits,
                            size_t number_array_length);
```

Shifts `n` right by `right_shift_bits` and stores the result in `dest`.

#### `fio_math_shl`

```c
FIO_IFUNC void fio_math_shl(uint64_t *dest,
                            uint64_t *n,
                            const size_t left_shift_bits,
                            const size_t number_array_length);
```

Shifts `n` left by `left_shift_bits` and stores the result in `dest`.

#### `fio_math_inv`

```c
FIO_IFUNC void fio_math_inv(uint64_t *dest, uint64_t *n, size_t len);
```

Two's-complement negation of the `len`-word number `n`.

#### `fio_math_msb_index`

```c
FIO_MIFN size_t fio_math_msb_index(uint64_t *n, const size_t len);
```

Returns the zero-based index of the most significant set bit, or `(size_t)-1` if the number is zero. Useful for measuring bit length.

#### `fio_math_lsb_index`

```c
FIO_MIFN size_t fio_math_lsb_index(uint64_t *n, const size_t len);
```

Returns the zero-based index of the least significant set bit, or `(size_t)-1` if the number is zero.

### Example

```c
#define FIO_MATH
#include "fio-stl.h"

int main(void) {
  uint64_t a[2] = {0, 1};          /* 2^64 */
  uint64_t b[2] = {3, 0};          /* 3 */
  uint64_t q[2] = {0};
  uint64_t r[2] = {0};

  fio_math_div(q, r, a, b, 2);
  printf("quot: %016llX%016llX\n",
         (unsigned long long)q[1], (unsigned long long)q[0]);
  printf("rem:  %016llX%016llX\n",
         (unsigned long long)r[1], (unsigned long long)r[0]);
  return 0;
}
```

------------------------------------------------------------
