# Endian and Byte Ordering

Macros and inline helpers for detecting and converting byte order. All are defined in [`./000 core.h`](./000%20core.h).

facil.io assumes one of the two common orderings: big-endian or little-endian. Mixed-endian systems are not supported.

## Compile-Time Detection

The library normalizes two common compiler-provided symbols so downstream code can test endianness with simple `#if` checks.

- **`__BIG_ENDIAN__`** — set to `1` on big-endian systems, `0` on little-endian systems.
- **`__LITTLE_ENDIAN__`** — set to `1` on little-endian systems, `0` on big-endian systems.

The library also checks `__BYTE_ORDER__` as an input hint when the compiler provides it. If the compiler already defines `__BIG_ENDIAN__`, `__LITTLE_ENDIAN__`, or `__BYTE_ORDER__`, those values are honored. Otherwise facil.io defines `FIO_ENDIAN_ORDER_TEST` (`'1234'`), `FIO_LITTLE_ENDIAN_TEST` (`0x31323334UL`), and `FIO_BIG_ENDIAN_TEST` (`0x34333231UL`), then compares the unprefixed names in `#if ENDIAN_ORDER_TEST == LITTLE_ENDIAN_TEST` and `#elif ENDIAN_ORDER_TEST == BIG_ENDIAN_TEST`. If neither matches, compilation fails with an error.

The comparison deliberately uses the unprefixed names `ENDIAN_ORDER_TEST`, `LITTLE_ENDIAN_TEST`, and `BIG_ENDIAN_TEST`; those exact identifiers are not defined by facil.io. This mirrors the source.

These are constants, not variables; use them in `#if __BIG_ENDIAN__` rather than runtime `if` statements when possible.

## Runtime Detection

```c
unsigned int fio_is_little_endian(void);
unsigned int fio_is_big_endian(void);
```

Runtime tests that return a non-zero value when the machine matches the named ordering. `fio_is_big_endian` is implemented as `!fio_is_little_endian()`. Compilers usually constant-fold the result, but the functions are provided for cases where a runtime check is clearer.

## Byte Swapping

Reverse the byte order of a fixed-width integer. Each macro expands to the corresponding compiler builtin when available; otherwise it falls back to a portable bit-shifting implementation.

- `fio_bswap8(i)` — identity, defined so generic code can use the `8`-bit width without special cases.
- `fio_bswap16(i)` — swap bytes of a 16-bit integer.
- `fio_bswap32(i)` — swap bytes of a 32-bit integer.
- `fio_bswap64(i)` — swap bytes of a 64-bit integer.
- `fio_bswap128(i)` — swap bytes of a 128-bit integer. Only available when the compiler supports `__int128`/`__uint128_t`.

## Host ↔ Network Conversions

Network byte order is big-endian. These macros convert between the local byte order and network byte order.

- `fio_lton16(i)`, `fio_lton32(i)`, `fio_lton64(i)` — local to network.
- `fio_ntol16(i)`, `fio_ntol32(i)`, `fio_ntol64(i)` — network to local.
- `fio_lton128(i)` — local to network, defined only on little-endian systems where the compiler supports `__int128`/`__uint128_t`.
- `fio_ntol128(i)` — network to local, defined only when the compiler supports `__int128`/`__uint128_t`.

On big-endian systems the 16/32/64-bit macros are no-ops. On little-endian systems they byte-swap.

`fio_lton8(i)` and `fio_ntol8(i)` are also defined as identities so width-generic code can treat 8-bit values uniformly.

## Host ↔ Little-Endian Conversions

These helpers force values into little-endian byte order, which is also the ordering used by several internal data structures.

- `fio_ltole16(i)`, `fio_ltole32(i)`, `fio_ltole64(i)` — local to little-endian.
- `fio_ltole128(i)` — local to little-endian, only available when the compiler supports `__int128`/`__uint128_t`.

On little-endian systems they are no-ops. On big-endian systems they byte-swap.

`fio_ltole8(i)` is defined as an identity for generic width handling.

## Endian-Aware Shifts

A pair of shift macros whose direction depends on the host endianness. They are used in code that processes multi-byte values as bit patterns where the conceptual "forward" or "backward" direction reverses with endianness.

On little-endian systems:

```c
#define FIO_SHIFT_FORWARDS(i, bits)  ((i) << (bits))
#define FIO_SHIFT_BACKWARDS(i, bits) ((i) >> (bits))
```

On big-endian systems they are defined only when the compiler supports `__int128`/`__uint128_t`:

```c
#define FIO_SHIFT_FORWARDS(i, bits)  ((i) >> (bits))
#define FIO_SHIFT_BACKWARDS(i, bits) ((i) << (bits))
```

## See Also

- [`./000 core.h`](./000%20core.h) — source of truth for all macros and functions above.
- [`./000 core.md`](./000%20core.md) — overview of core types and helpers.
