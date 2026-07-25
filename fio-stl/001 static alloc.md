# Static Scratch Allocator

A tiny, statically-backed allocator for short-lived temporary buffers.

Use it when you need a quick buffer you can return to a caller that should not be freed by the caller, or as a safer alternative to `alloca` for data that only needs to stay valid until the next few function calls.

The implementation is in [`./000 core.h`](./000%20core.h).

## `FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX`

```c
#ifndef FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX
#define FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX 256
#endif
```

Default slot count for library allocators that explicitly pass it as the fourth argument to `FIO_STATIC_ALLOC_DEF`.

`FIO_STATIC_ALLOC_DEF` does not use this setting implicitly. For each allocator, choose its `max_concurrent_allocations` argument according to the number of short-lived slots that may be outstanding at once. Raise this setting only when an allocator uses it and its slots are being reused too early.

## `FIO_STATIC_ALLOC_DEF`

```c
#define FIO_STATIC_ALLOC_DEF(name, type_T, units_per_allocation, max_concurrent_allocations)
```

Defines a static allocator named `name`.

```c
static type_T *name(size_t count);
```

The generated function returns an aligned `type_T *` to a slot containing `units_per_allocation` elements. Its `count` argument advances the atomic round-robin position by that number of slots; it does not change the size of the returned slot. Passing `0` returns the first slot without advancing the position.

- `name` — the allocator function name.
- `type_T` — the element type of each slot.
- `units_per_allocation` — number of `type_T` elements in each slot.
- `max_concurrent_allocations` — number of round-robin slots available before reuse.

The logical arena capacity reported by `name##_size()` is:

```
max_concurrent_allocations * units_per_allocation
```

## Generated API

The macro produces two functions:

- **`type_T *name(size_t count)`** — returns a pointer to a slot selected from the static round-robin buffer. No matching free exists; the memory is static and reused automatically.
- **`size_t name##_size(void)`** — returns the logical arena capacity in `type_T` units.

There is no generated `name##_free`, `name##_reset`, or similar helper. The buffer is managed by an atomic position counter and wraps around after `max_concurrent_allocations` slots.

## Example

```c
FIO_STATIC_ALLOC_DEF(numer2hex_allocator,
                     char,
                     19,
                     FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX);

char *ntos16(uint16_t n) {
  char *buf = numer2hex_allocator(1);
  buf[0] = '0';
  buf[1] = 'x';
  fio_ltoa16u(buf + 2, n, 16);
  buf[18] = 0;
  return buf;
}
```

The returned string is valid only until the allocator wraps around. A similar pattern is used by `fiobj_num2cstr` for temporary conversions.

## Thread Safety

The allocator is only "good enough" thread-safe. The atomic position counter protects the round-robin index, but the safety window is bounded by the allocator's `max_concurrent_allocations` argument. Keep slots short-lived and do not hold a returned pointer across too many calls or threads.
