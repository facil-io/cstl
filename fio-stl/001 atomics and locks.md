# Atomics and Locks

Concurrency primitives live in [`./000 core.h`](./000%20core.h). The lock selector that chooses between spinlocks and OS mutexes is in [`./001 header.h`](./001%20header.h).

## Atomic operations

All atomic macros use sequentially consistent ordering. They work on a pointer to the object and return the **previous** value, unless the name ends in `_fetch`, in which case they return the **new** value.

### Load and exchange

| Macro | Returns | Notes |
|---|---|---|
| `fio_atomic_load(dest, p_obj)` | assigns value to `dest` | Atomic read of `*p_obj`. |
| `fio_atomic_exchange(p_obj, value)` | previous value | Atomically sets `*p_obj = value`. |
| `fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired)` | `1` on success, `0` on failure | System-specific compare-and-swap. `p_expected` may be overwritten with the current value. |

### Arithmetic and logic

| Macro | Operation | Returns |
|---|---|---|
| `fio_atomic_add(p_obj, value)` | `*p_obj += value` | previous value |
| `fio_atomic_sub(p_obj, value)` | `*p_obj -= value` | previous value |
| `fio_atomic_and(p_obj, value)` | `*p_obj &= value` | previous value |
| `fio_atomic_or(p_obj, value)` | `*p_obj \|= value` | previous value |
| `fio_atomic_xor(p_obj, value)` | `*p_obj ^= value` | previous value |
| `fio_atomic_nand(p_obj, value)` | `*p_obj = ~(prev & value)` | previous value |
| `fio_atomic_add_fetch(p_obj, value)` | `*p_obj += value` | new value |
| `fio_atomic_sub_fetch(p_obj, value)` | `*p_obj -= value` | new value |
| `fio_atomic_and_fetch(p_obj, value)` | `*p_obj &= value` | new value |
| `fio_atomic_or_fetch(p_obj, value)` | `*p_obj \|= value` | new value |
| `fio_atomic_xor_fetch(p_obj, value)` | `*p_obj ^= value` | new value |
| `fio_atomic_nand_fetch(p_obj, value)` | `*p_obj = ~(prev & value)` | new value |

The implementation prefers GCC/Clang `__atomic` builtins, falls back to legacy `__sync` builtins, then C11 `stdatomic.h`, then MSVC intrinsics.

## Spinlocks

The default lock is an 8-bit spinlock with one main lock and seven sub-locks packed into the same byte.

| Type / macro | Meaning |
|---|---|
| `fio_lock_i` | `volatile unsigned char` spinlock type. |
| `FIO_LOCK_INIT` | Initializer (`0`). |
| `FIO_LOCK_SUBLOCK(i)` | Bit mask for sub-lock `i` (`0` to `7`). Combine with `\|`. |

### Single lock API

| Function | Behavior |
|---|---|
| `uint8_t fio_trylock(fio_lock_i *lock)` | Try to acquire the main lock. Returns `0` on success, `1` on failure. |
| `void fio_lock(fio_lock_i *lock)` | Busy-wait until the main lock is acquired. Avoid long waits. |
| `void fio_unlock(fio_lock_i *lock)` | Release the main lock. |
| `uint8_t fio_is_locked(fio_lock_i *lock)` | Returns non-zero if the main lock is held. |

### Group lock API

| Function | Behavior |
|---|---|
| `uint8_t fio_trylock_group(fio_lock_i *lock, uint8_t group)` | Try to acquire a group of sub-locks. Returns `0` on success, `1` on failure. |
| `void fio_lock_group(fio_lock_i *lock, uint8_t group)` | Busy-wait until the whole group is acquired. |
| `void fio_unlock_group(fio_lock_i *lock, uint8_t group)` | Release a group of sub-locks. |
| `uint8_t fio_is_group_locked(fio_lock_i *lock, uint8_t group)` | Returns non-zero if any sub-lock in `group` is held. |
| `uint8_t fio_trylock_full(fio_lock_i *lock)` | Try to lock all sub-locks at once. |
| `void fio_lock_full(fio_lock_i *lock)` | Busy-wait until every sub-lock is acquired. |
| `void fio_unlock_full(fio_lock_i *lock)` | Release every sub-lock. |

Example:

```c
fio_lock_i lock = FIO_LOCK_INIT;
if (!fio_trylock_group(&lock,
                       FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2))) {
  /* critical section */
  fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2));
}
```

## Atomic bitmap access

Operate on a single bit inside a byte array. All four helpers are atomic at the byte level.

| Function | Behavior |
|---|---|
| `uint8_t fio_atomic_bit_get(void *map, size_t bit)` | Read bit `bit` from `map`. |
| `void fio_atomic_bit_set(void *map, size_t bit)` | Set bit `bit` to `1`. |
| `void fio_atomic_bit_unset(void *map, size_t bit)` | Set bit `bit` to `0`. |
| `void fio_atomic_bit_flip(void *map, size_t bit)` | Toggle bit `bit`. |

## Lock selector

[`./001 header.h`](./001%20header.h) chooses between facil.io spinlocks and OS mutexes. The default is spinlocks unless `FIO_USE_THREAD_MUTEX` is set to `1`.

| Macro | Purpose |
|---|---|
| `FIO_USE_THREAD_MUTEX_TMP` | Per-include override. Defaults to `FIO_USE_THREAD_MUTEX`. |
| `FIO_THREADS` | Defined when OS mutexes are selected. |
| `FIO___LOCK_NAME` | Human-readable name of the selected lock type. |
| `FIO___LOCK_TYPE` | The selected lock type (`fio_thread_mutex_t` or `fio_lock_i`). |
| `FIO___LOCK_INIT` | Initializer for the selected lock type. |
| `FIO___LOCK_DESTROY(lock)` | Destroy a lock instance. |
| `FIO___LOCK_LOCK(lock)` | Acquire the lock. |
| `FIO___LOCK_TRYLOCK(lock)` | Try to acquire the lock. |
| `FIO___LOCK_UNLOCK(lock)` | Release the lock. |

These macros are used by modules that need thread safety but do not care which lock type is active.

## Thread scheduling hints

Defined in [`./000 core.h`](./000%20core.h) and used by the spinlock back-off loops.

| Macro | Behavior |
|---|---|
| `FIO_THREAD_WAIT(nano_sec)` | Sleep the current thread for about `nano_sec` nanoseconds. POSIX uses `nanosleep`; Windows uses `Sleep` (rounded up to at least 1 ms). |
| `FIO_THREAD_YIELD()` | Hint to the CPU that the thread is spinning. Emits `pause` on x86, `yield` on ARM, calls `YieldProcessor()` on MSVC, or falls back to `sched_yield()` on POSIX. |
| `FIO_THREAD_RESCHEDULE()` | Short sleep via `FIO_THREAD_WAIT(4)`. Used by `fio_lock_group` to back off without fully suspending the thread. |
