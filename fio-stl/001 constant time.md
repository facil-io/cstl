# Constant-Time Helpers

Branchless, timing-aware primitives defined in [`./000 core.h`](./000%20core.h).

These helpers avoid data-dependent branches and early exits so that execution time does not leak information about the values being processed. They are used by the cryptographic modules, authentication code, and anywhere else a compiler might otherwise turn a secret into a branch.

## Boolean constants

```c
uintmax_t fio_ct_true(uintmax_t cond);
uintmax_t fio_ct_false(uintmax_t cond);
```

* `fio_ct_true(cond)` returns `1` if `cond` is non-zero, else `0`.
* `fio_ct_false(cond)` returns `1` if `cond` is zero, else `0`.

Both return only the value `0` or `1`, suitable for feeding into `fio_ct_if_bool`.

## Conditional selection

```c
uintmax_t fio_ct_if_bool(uintmax_t cond, uintmax_t a, uintmax_t b);
uintmax_t fio_ct_if(uintmax_t cond, uintmax_t a, uintmax_t b);
```

* `fio_ct_if_bool` returns `a` when `cond == 1`, otherwise `b`.
* `fio_ct_if` first normalizes `cond` with `fio_ct_true`, then returns `a` when `cond` is non-zero, otherwise `b`.

Both are branchless: they compute `b ^ (mask & (a ^ b))`.

## Min, max, and absolute value

```c
intmax_t  fio_ct_max(intmax_t a, intmax_t b);
intmax_t  fio_ct_min(intmax_t a, intmax_t b);
uintmax_t fio_ct_abs(intmax_t i);
```

* `fio_ct_max` / `fio_ct_min` perform signed comparisons without branches.
* `fio_ct_abs` returns the absolute value of `i`.

## ASCII case conversion

```c
char fio_ct_tolower(char c);
```

Returns the lowercase form of `c` when `c` is an ASCII uppercase letter (`A`–`Z`); otherwise returns `c` unchanged. Branchless and locale-independent.

## Bitwise mux, majority, and three-way XOR

```c
uint32_t fio_ct_mux32(uint32_t x, uint32_t y, uint32_t z);
uint64_t fio_ct_mux64(uint64_t x, uint64_t y, uint64_t z);

uint32_t fio_ct_maj32(uint32_t x, uint32_t y, uint32_t z);
uint64_t fio_ct_maj64(uint64_t x, uint64_t y, uint64_t z);

uint32_t fio_ct_xor3_32(uint32_t x, uint32_t y, uint32_t z);
uint64_t fio_ct_xor3_64(uint64_t x, uint64_t y, uint64_t z);
```

| Function | Operation | Formula |
|----------|-----------|---------|
| `fio_ct_mux32/64` | bitwise choose | `z ^ (x & (y ^ z))` |
| `fio_ct_maj32/64` | bitwise majority | `(x & y) \| (z & (x \| y))` |
| `fio_ct_xor3_32/64` | bitwise parity | `x ^ y ^ z` |

These are the SHA-style `Ch`, `Maj`, and parity functions, implemented without branches.

## Constant-time equality

```c
_Bool fio_ct_is_eq(const void *a, const void *b, size_t bytes);
```

Compares two memory regions and returns `1` if they are byte-for-byte identical, `0` otherwise. The comparison is timing-attack resistant: it always reads every byte and accumulates differences into a single flag before returning, regardless of where the first mismatch occurs.

## Secure zero and stack wipe

```c
void fio_secure_zero(void *a_, size_t bytes);
```

Zeros `bytes` starting at `a_`. The write is performed through a `volatile` pointer so the compiler cannot optimize it away. Use this for passwords, keys, and other sensitive material that must be erased from memory.

```c
#define FIO_MEM_STACK_WIPE(pages)
```

Allocates a volatile stack array of `(pages) * 4096` bytes and initializes it to zero. Useful for clearing sensitive stack scratch space; the `(void)stack_mem` use keeps the array alive through the scope.
