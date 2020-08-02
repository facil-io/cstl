## Bit-Byte operations:

If the `FIO_BITWISE` macro is defined than the following macros will be
defined:

#### Byte Swapping

Returns a number of the indicated type with it's byte representation swapped.

- `fio_bswap16(i)`
- `fio_bswap32(i)`
- `fio_bswap64(i)`

#### Bit rotation (left / right)

Returns a number with it's bits left rotated (`lrot`) or right rotated (`rrot`) according to the type width specified (i.e., `fio_rrot64` indicates a **r**ight rotation for `uint64_t`).

- `fio_lrot32(i, bits)`

- `fio_rrot32(i, bits)`

- `fio_lrot64(i, bits)`

- `fio_rrot64(i, bits)`

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

#### Bytes to Numbers (native / reversed / network ordered)

Reads a number from an unaligned memory buffer. The number or bits read from the buffer is indicated by the name of the function.

**Big Endian (default)**:

- `fio_buf2u16(buffer)`
- `fio_buf2u32(buffer)`
- `fio_buf2u64(buffer)`

**Little Endian**:

- `fio_buf2u16_little(buffer)`
- `fio_buf2u32_little(buffer)`
- `fio_buf2u64_little(buffer)`

**Native Byte Order**:

- `fio_buf2u16_local(buffer)`
- `fio_buf2u32_local(buffer)`
- `fio_buf2u64_local(buffer)`

**Reversed Byte Order**:

- `fio_buf2u16_bswap(buffer)`
- `fio_buf2u32_bswap(buffer)`
- `fio_buf2u64_bswap(buffer)`

#### Numbers to Bytes (native / reversed / network ordered)

Writes a number to an unaligned memory buffer. The number or bits written to the buffer is indicated by the name of the function.

**Big Endian (default)**:

- `fio_u2buf16(buffer, i)`
- `fio_u2buf32(buffer, i)`
- `fio_u2buf64(buffer, i)`

**Little Endian**:

- `fio_u2buf16_little(buffer, i)`
- `fio_u2buf32_little(buffer, i)`
- `fio_u2buf64_little(buffer, i)`

**Native Byte Order**:

- `fio_u2buf16_local(buffer, i)`
- `fio_u2buf32_local(buffer, i)`
- `fio_u2buf64_local(buffer, i)`

**Reversed Byte Order**:

- `fio_u2buf16_bswap(buffer, i)`
- `fio_u2buf32_bswap(buffer, i)`
- `fio_u2buf64_bswap(buffer, i)`

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