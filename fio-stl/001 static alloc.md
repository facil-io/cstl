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

Maximum safe concurrent calls for all static allocators defined with `FIO_STATIC_ALLOC_DEF`.

The allocator is a round-robin buffer. Once the number of in-flight allocations exceeds this multiplier, new allocations may reuse memory that a previous caller still holds. Raise this value if you use many static allocator calls or many threads and see buffer reuse too early.

## `FIO_STATIC_ALLOC_DEF`

```c
#define FIO_STATIC_ALLOC_DEF(name, type_T, size_per_allocation, allocations_per_thread)
```

Defines a static allocator named `name`.

```c
static type_T *name(size_t count);
```

The generated function takes a `count` and returns a `type_T *` to a buffer of `sizeof(type_T) * count * size_per_allocation` bytes, aligned for `type_T`.

- `name` — the allocator function name.
- `type_T` — the element type the pointer is cast to.
- `size_per_allocation` — base size multiplier for each allocation unit.
- `allocations_per_thread` — extra headroom per logical caller.

The total safe buffer size before reuse is:

```
FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX * allocations_per_thread *
    sizeof(type_T) * size_per_allocation
```

## Generated API

The macro produces a single function:

- **`type_T *name(size_t count)`** — returns a pointer to the next slice of the static round-robin buffer. No matching free exists; the memory is static and reused automatically.

There is no generated `name##_free`, `name##_reset`, or similar helper. The buffer is managed automatically by advancing an atomic position counter and wrapping around.

## Example

```c
FIO_STATIC_ALLOC_DEF(numer2hex_allocator, char, 19, 1);

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

The allocator is only "good enough" thread-safe. The atomic position counter protects the round-robin index, but the safety window is bounded by `FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX`. Keep allocations short-lived and do not hold the returned pointer across too many other static allocator calls or across too many threads.
