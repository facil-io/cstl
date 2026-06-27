# Bit Operations

Bit-twiddling helpers defined in [`./000 core.h`](./000%20core.h).

## Bit rotation

### Generic rotation macros

```c
#define FIO_LROT(i, bits)
#define FIO_RROT(i, bits)
```

Rotate the value `i` left or right by `bits`. The rotation count is masked to the width of `i`, so these macros work for any integer type.

### Typed rotation helpers

Each width has a left and right variant. When the compiler provides a rotate builtin it is used; otherwise a portable shift-or expression is emitted. All are marked `FIO_CONST`.

Note: the `fio_rrot8` builtin guard has a typo (`__has_builtin(__builtin_rotatrightt8)`), so the 8-bit right rotation always falls back to the portable implementation.

| Width | Left | Right |
|---|---|---|
| 8-bit | `fio_lrot8(i, bits)` | `fio_rrot8(i, bits)` |
| 16-bit | `fio_lrot16(i, bits)` | `fio_rrot16(i, bits)` |
| 32-bit | `fio_lrot32(i, bits)` | `fio_rrot32(i, bits)` |
| 64-bit | `fio_lrot64(i, bits)` | `fio_rrot64(i, bits)` |
| 128-bit | `fio_lrot128(i, bits)` | `fio_rrot128(i, bits)` |

The 128-bit variants are only available when the compiler supports `__int128`/`__uint128_t`.

### Endian-aware forward rotation

```c
fio_frot16(i, bits)
fio_frot32(i, bits)
fio_frot64(i, bits)
```

On little-endian systems `fio_frot*` maps to the right-rotation helpers; on big-endian systems it maps to the left-rotation helpers. These are macros, not functions.

## Combined rotation-XOR operations

Common SHA-2/BLAKE2-style combinations kept as separate operations so the compiler can optimize them as a unit.

```c
uint32_t fio_xor_rrot3_32(uint32_t x, uint8_t a, uint8_t b, uint8_t c);
uint64_t fio_xor_rrot3_64(uint64_t x, uint8_t a, uint8_t b, uint8_t c);
```

Computes `ROTR(x,a) ^ ROTR(x,b) ^ ROTR(x,c)`. Used by SHA-256/SHA-512 Sigma functions.

```c
uint32_t fio_xor_rrot2_shr_32(uint32_t x, uint8_t a, uint8_t b, uint8_t c);
uint64_t fio_xor_rrot2_shr_64(uint64_t x, uint8_t a, uint8_t b, uint8_t c);
```

Computes `ROTR(x,a) ^ ROTR(x,b) ^ (x >> c)`. Used by SHA-256/SHA-512 lowercase sigma functions for message scheduling.

## XOR masking

```c
void fio_xmask(void *buf, size_t len, uint64_t mask);
void fio_xmask_cpy(char *restrict dest,
                   const char *src,
                   size_t len,
                   uint64_t mask);
```

XOR every byte of a buffer with a repeating 64-bit `mask`. `fio_xmask` works in place; `fio_xmask_cpy` reads from `src` and writes to `dest`. If `dest == src`, `fio_xmask_cpy` behaves like `fio_xmask`.

Both functions process data in 64-bit chunks for speed and fall back to a byte-sized tail for the last `0`–`7` bytes. Performance is best when the buffer is aligned.

## Popcount and Hamming distance

```c
int fio_popcount(uint64_t n);
```

Returns the number of `1` bits in `n`. Uses `__builtin_popcountll` or MSVC `__popcnt64` when available; otherwise falls back to a portable parallel bit-count implementation.

```c
#define fio_hemming_dist(n1, n2) \
    fio_popcount(((uint64_t)(n1) ^ (uint64_t)(n2)))
```

Returns the Hamming distance between `n1` and `n2` — the number of bits that differ. The macro is named `fio_hemming_dist` in the source (the conventional spelling is "Hamming").

## Bit isolation and indexing

```c
uint64_t fio_bits_lsb(uint64_t i);
uint64_t fio_bits_msb(uint64_t i);
```

* `fio_bits_lsb(i)` isolates the least-significant set bit.
* `fio_bits_msb(i)` isolates the most-significant set bit.

```c
size_t fio_bits_lsb_index(uint64_t i);
size_t fio_bits_msb_index(uint64_t i);
```

Return the bit index of the least-significant or most-significant set bit. Both return `(size_t)-1` when `i` is `0`.

```c
size_t fio_lsb_index_unsafe(uint64_t i);
size_t fio_msb_index_unsafe(uint64_t i);
```

Same as above but assume `i != 0`; calling them with `0` is undefined behavior. They use `__builtin_ctzll`/`__builtin_clzll` when available and fall back to `fio___single_bit_index_unsafe` — a `switch` over 64 power-of-two constants — otherwise.

## Byte detection in vectors

These helpers detect special byte values inside a packed integer. In the result, a matching byte lane is set to `0x80`; all other lanes are `0x00`.

### 32-bit vectors

```c
uint32_t fio_has_zero_byte32(uint32_t row);
uint32_t fio_has_byte32(uint32_t row, uint8_t byte);
uint32_t fio_has_full_byte32(uint32_t row);
```

* `fio_has_zero_byte32` detects `0x00` bytes.
* `fio_has_byte32` detects an arbitrary byte value.
* `fio_has_full_byte32` detects `0x00` bytes — the source's inline comment claims `0xFF`, but the implementation calls `fio_has_zero_byte32(row)`.

### 64-bit vectors

```c
uint64_t fio_has_zero_byte64(uint64_t row);
uint64_t fio_has_zero_byte_alt64(uint64_t row);
uint64_t fio_has_byte64(uint64_t row, uint8_t byte);
uint64_t fio_has_full_byte64(uint64_t row);
uint64_t fio_has_byte2bitmap(uint64_t result);
```

* `fio_has_zero_byte64` detects `0x00` bytes.
* `fio_has_zero_byte_alt64` is an alternate entry point with the same behavior as `fio_has_zero_byte64`; the source's warning about bitmap safety may be stale.
* `fio_has_byte64` detects an arbitrary byte value.
* `fio_has_full_byte64` detects `0xFF` bytes.
* `fio_has_byte2bitmap` packs a `fio_has_byte*` result into a single 8-bit bitmap.

## Bitmap access

Operate on a single bit inside a byte array. These are **not** atomic; use the [`fio_atomic_bit_*`](./001%20atomics%20and%20locks.md#atomic-bitmap-access) helpers for shared mutable bitmaps.

```c
uint8_t fio_bit_get(void *map, size_t bit);
void    fio_bit_set(void *map, size_t bit);
void    fio_bit_unset(void *map, size_t bit);
void    fio_bit_flip(void *map, size_t bit);
```

* `fio_bit_get` returns `0` or `1`.
* `fio_bit_set` sets the bit to `1`.
* `fio_bit_unset` sets the bit to `0`.
* `fio_bit_flip` toggles the bit.

## See also

- [`./000 core.h`](./000%20core.h) — source of truth for all macros and functions above.
- [`./001 atomics and locks.md`](./001%20atomics%20and%20locks.md) — atomic bitmap helpers and locks.
- [`./001 constant time.md`](./001%20constant%20time.md) — branchless bitwise primitives.
- [`./001 endian.md`](./001%20endian.md) — byte ordering helpers.
