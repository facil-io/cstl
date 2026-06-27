# Core Random PRNG

An optionally-deterministic 128-bit pseudo-random number generator defined by a macro. It is a low-level building block for hash functions, test harnesses, and other internals that need an optionally repeatable, fast stream of bits.

For the high-level random module — `fio_rand64`, secure bytes, hashing, etc. — see [`./002 random.h`](./002%20random.h).

The implementation is in [`./000 core.h`](./000%20core.h).

## `fio_cycle_counter`

```c
uint64_t fio_cycle_counter(void);
```

Returns a hardware cycle counter when one is available, or `0` otherwise.

- On x86/x86_64 it reads `rdtsc`.
- On AArch64 it reads `cntvct_el0`.
- On other platforms it returns `0`.

In non-deterministic mode, the PRNG uses this for timing-based jitter during reseeding.

## `FIO_DEFINE_RANDOM128_FN`

```c
#define FIO_DEFINE_RANDOM128_FN(extern, name, reseed_log, seed_offset)
```

Defines a named PRNG instance and its API. The most common deterministic form looks like:

```c
FIO_DEFINE_RANDOM128_FN(static, my_rng, 0, 0x1234)
```

Parameters:

- `extern` — linkage prefix, e.g. `static` or `FIO_SFUNC`.
- `name` — base name for all generated functions and internal state.
- `reseed_log` — automatic reseed interval, `2^reseed_log` calls. Set to `0` for a fully deterministic stream.
- `seed_offset` — arbitrary offset added to the default seed constants.

### Non-Deterministic Mode

```c
FIO_DEFINE_RANDOM128_FN(static, my_rng, 5, 0x1234)
```

When `reseed_log` is non-zero (and less than `64`), the generator calls `name_reseed()` automatically every `2^reseed_log` calls, mixing in timing data and the cycle counter. This makes the stream non-deterministic, and improves entropy.

In this example, reseeding is set to `5`, resulting in reseeding every 32 calls (collecting new entropy every 4,096 bytes).

## Generated API

For a definition `FIO_DEFINE_RANDOM128_FN(static, my_rng, 0, 0)` the following functions are generated:

### `my_rng128`

```c
fio_u128 my_rng128(void);
```

Returns 128 pseudo-random bits. The return type is [`fio_u128`](./001%20vector%20math.md), so the result can also be read through `.u64[0]`, `.u64[1]`, `.u8`, etc.

### `my_rng64`

```c
uint64_t my_rng64(void);
```

Returns 64 pseudo-random bits. Each call consumes half of a 128-bit result; a new 128-bit value is generated only every other call.

### `my_rng_bytes`

```c
void my_rng_bytes(void *dest, size_t len);
```

Fills `dest` with `len` pseudo-random bytes. Does nothing if `dest` is `NULL` or `len` is zero.

### `my_rng_reset`

```c
void my_rng_reset(void);
```

Resets the generator to its initial seeded state.

### `my_rng_reseed`

```c
void my_rng_reseed(void);
```

Re-seeds the generator using `CLOCK_MONOTONIC`, address/bits of local state, and `fio_cycle_counter()`. Useful after `fork()` or when you want to de-correlate separate processes.

### `my_rng_on_fork`

```c
void my_rng_on_fork(void *is_null);
```

Calls `my_rng_reseed()`. Fork-handler helper that takes an ignored `void *` argument.

## Example

```c
FIO_DEFINE_RANDOM128_FN(static, my_rng, 0, 0)

void roll_die(int n) {
  my_rng_reset();
  for (int i = 0; i < n; ++i) {
    uint64_t r = my_rng64();
    printf("roll %d: %llu\n", i, (unsigned long long)(r % 6 + 1));
  }
}
```

Because `reseed_log` is `0`, the same seed always produces the same sequence.

## Determinism and fork safety

- Set `reseed_log = 0` for deterministic, repeatable output.
- Set `reseed_log` to a practical value such as `31` for a non-deterministic stream that reseeds automatically.
- After `fork()`, call `name_reseed()` or register a wrapper that calls `name_on_fork()` with `pthread_atfork()` to avoid duplicated sequences in child processes.

## Testing

Test results for this generator are recorded in [`./721 rand128 macro.md`](./721%20rand128%20macro.md).

## See also

- [`./002 random.h`](./002%20random.h) — high-level random/hashing module.
- [`./001 vector math.md`](./001%20vector%20math.md) — the `fio_u128` return type.
- [`./000 core.h`](./000%20core.h) — source of truth.
