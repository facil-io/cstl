# Core Math Helpers

Low-level integer arithmetic, prime testing, and multi-precision primitives defined in [`./000 core.h`](./000%20core.h).

These functions work on plain `uint64_t` arrays. The higher-level division, shift, and inversion helpers live in [`./002 math.h`](./002%20math.h), and the wide-union wrappers are documented in [`./001 vector math.md`](./001%20vector%20math.md).

## Hash primes

Four families of pre-selected prime constants, indexed `0` through `31`:

```c
FIO_U8_HASH_PRIME0  ... FIO_U8_HASH_PRIME31
FIO_U16_HASH_PRIME0 ... FIO_U16_HASH_PRIME31
FIO_U32_HASH_PRIME0 ... FIO_U32_HASH_PRIME31
FIO_U64_HASH_PRIME0 ... FIO_U64_HASH_PRIME31
```

Each fits in the named unsigned type and has about half of its bits set (`3`–`5` bits for the 8-bit primes). They are useful as hash mixers and for picking small prime bases.

## Modular arithmetic

```c
uint64_t fio_math_mod_mul64(uint64_t a, uint64_t b, uint64_t mod);
```

Computes `(a * b) % mod` without overflowing 64 bits. Uses `__uint128_t` when available; otherwise falls back to a double-and-add loop.

```c
uint64_t fio_math_mod_expo64(uint64_t base, uint64_t exp, uint64_t mod);
```

Right-to-left binary modular exponentiation. Calls `fio_math_mod_mul64` for each set bit of `exp`.

## Primality tests

```c
bool fio_math_is_uprime(uint64_t n);
bool fio_math_is_iprime(int64_t n);
```

`fio_math_is_uprime` tests whether `n` is probably prime. `fio_math_is_iprime` tests the absolute value of `n`.

For `n < FIO_PRIME_TABLE_LIMIT` (default `1024`) the test is deterministic using a small sieve. Larger values use Miller–Rabin with `10` rounds. Internal helpers `fio___is_prime_table` and `fio___is_prime_maybe` implement the two paths.

## 64-bit add / sub / mul with carry

```c
uint64_t fio_math_addc64(uint64_t a, uint64_t b,
                         uint64_t carry_in, uint64_t *carry_out);

uint64_t fio_math_subc64(uint64_t a, uint64_t b,
                         uint64_t borrow_in, uint64_t *borrow_out);

uint64_t fio_math_mulc64(uint64_t a, uint64_t b,
                         uint64_t *carry_out);
```

Return the low 64 bits of `a + b + carry_in`, `a - b - borrow_in`, or `a * b`; the outgoing carry/borrow/high word is written to `*carry_out`. They use compiler builtins where available and fall back to portable word arithmetic.

## Multi-precision add / sub / mul

All multi-precision routines treat arrays as little-endian `uint64_t` words: index `0` holds the least-significant word. Inputs are expected to have the same `len` (pad shorter inputs with zero).

```c
bool     fio_math_add(uint64_t *dest,
                      const uint64_t *a,
                      const uint64_t *b,
                      size_t len);

uint64_t fio_math_sub(uint64_t *dest,
                      const uint64_t *a,
                      const uint64_t *b,
                      size_t len);

void     fio_math_mul(uint64_t *restrict dest,
                      const uint64_t *a,
                      const uint64_t *b,
                      size_t len);
```

* `fio_math_add` writes `a + b` to `dest` and returns the final carry.
* `fio_math_sub` writes `a - b` to `dest` and returns the final borrow.
* `fio_math_mul` writes the full `len * 2` word product of `a` and `b` to `dest`.

When `__uint128_t` is available, `fio_math_addc128` and `fio_math_add2` provide the same carry-style addition for 128-bit words.

## Internal multiplication helpers

`fio_math_mul` dispatches to specialized helpers depending on `len`:

* `fio___math_mul_long` — optimized schoolbook multiplication, loop-unrolled, with a `__uint128_t` fast path.
* `fio___math_mul_256` — fully unrolled 4-word (256-bit) multiply.
* `fio___math_mul_512` — semi-unrolled 8-word (512-bit) multiply.
* `fio___math_mul_1024` — semi-unrolled 16-word (1024-bit) multiply, mostly used as a fallback.

For large numbers:

```c
#define FIO___MATH_KARATSUBA_THRESHOLD 32
```

When `len` is at least this threshold and even, `fio_math_mul` calls `fio___math_mul_karatsuba`.

```c
void fio___math_mul_karatsuba(uint64_t *restrict dest,
                              const uint64_t *a,
                              const uint64_t *b,
                              size_t len);
```

`fio___math_mul_karatsuba` splits `a` and `b` into low/high halves, computes `z0`, `z1`, and `z2`, then combines them as `z2*B^2 + z1*B + z0`. It uses `fio___math_mul_norecurse` for the three half-sized sub-products and allocates scratch buffers on the stack. `len` must be even; on MSVC or pre-C++14 the stack buffers are fixed at 256 words, so inputs are limited accordingly.

## See also

* [`./000 core.h`](./000%20core.h) — source of truth.
* [`./002 math.h`](./002%20math.h) — higher-level division, shift, and inversion helpers.
* [`./001 vector math.md`](./001%20vector%20math.md) — wide-union `fio_uXXX` arithmetic built on these primitives.
