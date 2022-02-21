## Bit-Byte operations:

If the `FIO_BITWISE` macro is defined than the following macros will be
defined:

**Note**: the 128 bit helpers are only available with systems / compilers that support 128 bit types.

#### Byte Swapping

Returns a number of the indicated type with it's byte representation swapped.

- `fio_bswap16(i)`
- `fio_bswap32(i)`
- `fio_bswap64(i)`
- `fio_bswap128(i)`

#### Bit rotation (left / right)

Returns a number with it's bits left rotated (`lrot`) or right rotated (`rrot`) according to the type width specified (i.e., `fio_rrot64` indicates a **r**ight rotation for `uint64_t`).

- `fio_lrot8(i, bits)`
- `fio_rrot8(i, bits)`
- `fio_lrot16(i, bits)`
- `fio_rrot16(i, bits)`
- `fio_lrot32(i, bits)`
- `fio_rrot32(i, bits)`
- `fio_lrot64(i, bits)`
- `fio_rrot64(i, bits)`
- `fio_lrot128(i, bits)`
- `fio_rrot128(i, bits)`

- `FIO_LROT(i, bits)` (MACRO, can be used with any type size)
- `FIO_RROT(i, bits)` (MACRO, can be used with any type size)

#### Numbers to Numbers (network ordered)

On big-endian systems, these macros a NOOPs, whereas on little-endian systems these macros flip the byte order.

- `fio_lton16(i)`
- `fio_ntol16(i)`
- `fio_lton32(i)`
- `fio_ntol32(i)`
- `fio_lton64(i)`
- `fio_ntol64(i)`
- `fio_lton128(i)`
- `fio_ntol128(i)`

#### Numbers to Numbers (Little Endian)

Converts a local number to little-endian. On big-endian systems, these macros flip the byte order, whereas on little-endian systems these macros are a NOOP.

- `fio_ltole16(i)`
- `fio_ltole32(i)`
- `fio_ltole64(i)`
- `fio_ltole128(i)`

#### Bytes to Numbers (native / reversed / network ordered)

Reads a number from an unaligned memory buffer. The number or bits read from the buffer is indicated by the name of the function.

Note: The following functions might use `__builtin_memcpy` when available. To use a the facil.io C implementation, define `FIO_BITWISE_USE_MEMCPY` as `0` and consider enabling unaligned memory access if the platform allows for it, by setting `FIO_UNALIGNED_ACCESS` to `1`.

**Big Endian (default)**:

- `fio_buf2u16(buffer)`
- `fio_buf2u32(buffer)`
- `fio_buf2u64(buffer)`
- `fio_buf2u128(buffer)`

**Little Endian**:

- `fio_buf2u16_little(buffer)`
- `fio_buf2u32_little(buffer)`
- `fio_buf2u64_little(buffer)`
- `fio_buf2u128_little(buffer)`

**Native Byte Order**:

- `fio_buf2u16_local(buffer)`
- `fio_buf2u32_local(buffer)`
- `fio_buf2u64_local(buffer)`
- `fio_buf2u128_local(buffer)`

**Reversed Byte Order**:

- `fio_buf2u16_bswap(buffer)`
- `fio_buf2u32_bswap(buffer)`
- `fio_buf2u64_bswap(buffer)`
- `fio_buf2u128_bswap(buffer)`

#### Numbers to Bytes (native / reversed / network ordered)

Writes a number to an unaligned memory buffer. The number or bits written to the buffer is indicated by the name of the function.

**Big Endian (default)**:

- `fio_u2buf16(buffer, i)`
- `fio_u2buf32(buffer, i)`
- `fio_u2buf64(buffer, i)`
- `fio_u2buf128(buffer, i)`

**Little Endian**:

- `fio_u2buf16_little(buffer, i)`
- `fio_u2buf32_little(buffer, i)`
- `fio_u2buf64_little(buffer, i)`
- `fio_u2buf128_little(buffer, i)`

**Native Byte Order**:

- `fio_u2buf16_local(buffer, i)`
- `fio_u2buf32_local(buffer, i)`
- `fio_u2buf64_local(buffer, i)`
- `fio_u2buf128_local(buffer, i)`

**Reversed Byte Order**:

- `fio_u2buf16_bswap(buffer, i)`
- `fio_u2buf32_bswap(buffer, i)`
- `fio_u2buf64_bswap(buffer, i)`
- `fio_u2buf128_bswap(buffer, i)`

#### Constant Time Bit Operations

Performs the operation indicated in constant time.

- `fio_ct_true(condition)`

    Tests if `condition` is non-zero (returns `1` / `0`).

- `fio_ct_false(condition)`

    Tests if `condition` is zero (returns `1` / `0`).

- `fio_ct_if_bool(bool, a_if_true, b_if_false)`

    Tests if `bool == 1` (returns `a` / `b`).

- `fio_ct_if(condition, a_if_true, b_if_false)`

    Tests if `condition` is non-zero (returns `a` / `b`).

- `fio_ct_max(a, b)`

    Returns `a` if a >= `b` (performs a **signed** comparison).

#### Simulating SIMD instructions


- `fio_has_full_byte32(uint32_t row)`

	Detects a byte where all the bits are set (`255`) within a 4 byte vector.

- `fio_has_zero_byte32(uint32_t row)`

	Detects a byte where no bits are set (0) within a 4 byte vector.

- `fio_has_byte32(uint32_t row, uint8_t byte)`

	Detects if `byte` exists within a 4 byte vector.

- `fio_has_full_byte64(uint64_t row)`

	Detects a byte where all the bits are set (`255`) within an 8 byte vector.

- `fio_has_zero_byte64(uint64_t row)`

	Detects a byte where no bits are set (byte == 0) within an 8 byte vector.

- `fio_has_byte64(uint64_t row, uint8_t byte)`

	Detects if `byte` exists within an 8 byte vector.

- `fio_has_full_byte128(__uint128_t row)`

    Detects a byte where all the bits are set (`255`) within an 8 byte vector.

- `fio_has_zero_byte128(__uint128_t row)`

    Detects a byte where no bits are set (0) within an 8 byte vector.

- `fio_has_byte128(__uint128_t row, uint8_t byte)`

    Detects if `byte` exists within an 8 byte vector.

#### `fio_popcount` and Hemming 

```c
int fio_popcount(uint64_t n);
```

Returns the number of set bits in the number `n`.

#### `fio_hemming_dist`

```c
#define fio_hemming_dist(n1, n2) fio_popcount(((uint64_t)(n1) ^ (uint64_t)(n2)))
```

Returns the Hemming Distance between the number `n1` and the number `n2`.

Hemming Distance is the number of bits that need to be "flipped" in order for both numbers to be equal.

#### `fio_xmask`

```c
void fio_xmask(char *buf,
               size_t len,
               uint64_t mask);
```

Masks data using a 64 bit mask.

The function may perform significantly better when the buffer's memory is aligned.

#### `fio_xmask2`

```c
uint64_t fio_xmask2(char *buf,
                    size_t len,
                    uint64_t mask,
                    uint64_t nonce);
```

Masks data using a 64 bit mask and a counter mode nonce.

Returns the end state of the mask.

The function may perform significantly better when the buffer's memory is aligned.

**Note**: this function could be used to obfuscate data in locally stored buffers, mitigating risks such as data leaks that may occur when memory is swapped to disk. However, this function should **never** be used as an alternative to actual encryption.

-------------------------------------------------------------------------------

## Bitmap helpers

If the `FIO_BITMAP` macro is defined than the following macros will be
defined.

In addition, the `FIO_ATOMIC` will be assumed to be defined, as setting bits in
the bitmap is implemented using atomic operations.

#### Bitmap helpers
- `fio_bitmap_get(void *map, size_t bit)`
- `fio_bitmap_set(void *map, size_t bit)`   (an atomic operation, thread-safe)
- `fio_bitmap_unset(void *map, size_t bit)` (an atomic operation, thread-safe)

-------------------------------------------------------------------------------
