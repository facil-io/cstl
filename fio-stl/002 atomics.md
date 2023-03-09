## Atomic operations

```c
#define FIO_ATOMIC
#include "fio-stl.h"
```

If the `FIO_ATOMIC` macro is defined than the following macros will be defined.

In general, when a function returns a value, it is always the previous value - unless the function name ends with `fetch` or `load`.

#### `fio_atomic_load(p_obj)`

Atomically loads and returns the value stored in the object pointed to by `p_obj`.

#### `fio_atomic_exchange(p_obj, value)`

Atomically sets the object pointer to by `p_obj` to `value`, returning the
previous value.

#### `fio_atomic_add(p_obj, value)`

A MACRO / function that performs `add` atomically.

Returns the previous value.

#### `fio_atomic_sub(p_obj, value)`

A MACRO / function that performs `sub` atomically.

Returns the previous value.

#### `fio_atomic_and(p_obj, value)`

A MACRO / function that performs `and` atomically.

Returns the previous value.

#### `fio_atomic_xor(p_obj, value)`

A MACRO / function that performs `xor` atomically.

Returns the previous value.

#### `fio_atomic_or(p_obj, value)`

A MACRO / function that performs `or` atomically.

Returns the previous value.

#### `fio_atomic_nand(p_obj, value)`

A MACRO / function that performs `nand` atomically.

Returns the previous value.

#### `fio_atomic_add_fetch(p_obj, value)`

A MACRO / function that performs `add` atomically.

Returns the new value.

#### `fio_atomic_sub_fetch(p_obj, value)`

A MACRO / function that performs `sub` atomically.

Returns the new value.

#### `fio_atomic_and_fetch(p_obj, value)`

A MACRO / function that performs `and` atomically.

Returns the new value.

#### `fio_atomic_xor_fetch(p_obj, value)`

A MACRO / function that performs `xor` atomically.

Returns the new value.

#### `fio_atomic_or_fetch(p_obj, value)`

A MACRO / function that performs `or` atomically.

Returns the new value.

#### `fio_atomic_nand_fetch(p_obj, value)`

A MACRO / function that performs `nand` atomically.

Returns the new value.

#### `fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired)`

A MACRO / function that performs a system specific `fio_atomic_compare_exchange` using pointers.

The behavior of this instruction is compiler / CPU architecture specific, where `p_expected` **SHOULD** be overwritten with the latest value of `p_obj`, but **MAY NOT**, depending on system and compiler implementations.

Returns 1 for successful exchange or 0 for failure.

#### Atomic Bitmap helpers

- `fio_atomic_bit_get(void *map, size_t bit)`

- `fio_atomic_bit_set(void *map, size_t bit)`   (an **atomic** operation, thread-safe)

- `fio_atomic_bit_unset(void *map, size_t bit)` (an **atomic** operation, thread-safe)

Gets / Sets bits an atomic thread-safe way.

### a SpinLock style MultiLock

Atomic operations lend themselves easily to implementing spinlocks, so the facil.io STL includes one whenever atomic operations are defined (`FIO_ATOMIC`).

Spinlocks are effective for very short critical sections or when a a failure to acquire a lock allows the program to redirect itself to other pending tasks. 

However, in general, spinlocks should be avoided when a task might take a longer time to complete or when the program might need to wait for a high contention lock to become available.

#### `fio_lock_i`

A spinlock type based on a volatile unsigned char.

**Note**: the spinlock contains one main / default lock (`sub == 0`) and 7 sub-locks (`sub >= 1 && sub <= 7`), which could be managed:

- Separately: using the `fio_lock_sublock`, `fio_trylock_sublock` and `fio_unlock_sublock` functions.
- Jointly: using the `fio_trylock_group`, `fio_lock_group` and `fio_unlock_group` functions.
- Collectively: using the `fio_trylock_full`, `fio_lock_full` and `fio_unlock_full` functions.


#### `fio_lock(fio_lock_i *)`

Busy waits for the default lock (sub-lock `0`) to become available.

#### `fio_trylock(fio_lock_i *)`

Attempts to acquire the default lock (sub-lock `0`). Returns 0 on success and 1 on failure.

#### `fio_unlock(fio_lock_i *)`

Unlocks the default lock (sub-lock `0`), no matter which thread owns the lock.

#### `fio_is_locked(fio_lock_i *)`

Returns 1 if the (main) lock is engaged. Otherwise returns 0.

#### `fio_lock_sublock(fio_lock_i *, uint8_t sub)`

Busy waits for a sub-lock to become available.

#### `fio_trylock_sublock(fio_lock_i *, uint8_t sub)`

Attempts to acquire the sub-lock. Returns 0 on success and 1 on failure.

#### `fio_unlock_sublock(fio_lock_i *, uint8_t sub)`

Unlocks the sub-lock, no matter which thread owns the lock.

#### `fio_is_sublocked(fio_lock_i *, uint8_t sub)`

Returns 1 if the specified sub-lock is engaged. Otherwise returns 0.

#### `uint8_t fio_trylock_group(fio_lock_i *lock, const uint8_t group)`

Tries to lock a group of sub-locks.

Combine a number of sub-locks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
macro. i.e.:

```c
if(fio_trylock_group(&lock,
                     FIO_LOCK_SUBLOCK(1) |
                     FIO_LOCK_SUBLOCK(2)) == 0) {
  // act in lock and then release the SAME lock with:
  fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2));
}
```

Returns 0 on success and 1 on failure.

#### `void fio_lock_group(fio_lock_i *lock, uint8_t group)`

Busy waits for a group lock to become available - not recommended.

See `fio_trylock_group` for details.

#### `void fio_unlock_group(fio_lock_i *lock, uint8_t group)`

Unlocks a sub-lock group, no matter which thread owns which sub-lock.

#### `fio_trylock_full(fio_lock_i *lock)`

Tries to lock all sub-locks. Returns 0 on success and 1 on failure.

#### `fio_lock_full(fio_lock_i *lock)`

Busy waits for all sub-locks to become available - not recommended.

#### `fio_unlock_full(fio_lock_i *lock)`

Unlocks all sub-locks, no matter which thread owns which lock.

-------------------------------------------------------------------------------

## MultiLock with Thread Suspension

```c
#define FIO_LOCK2
#include "fio-stl.h"
```

**BROKEN(!):** note that the `FIO_LOCK2` implementation currently does not work on all systems and assumes specific OS behavior.

If the `FIO_LOCK2` macro is defined than the multi-lock `fio_lock2_s` type and it's functions will be defined.

The `fio_lock2` locking mechanism follows a bitwise approach to multi-locking, allowing a single lock to contain up to 31 sublocks (on 32 bit machines) or 63 sublocks (on 64 bit machines).

This is a very powerful tool that allows simultaneous locking of multiple sublocks (similar to `fio_trylock_group`) while also supporting a thread "waitlist" where paused threads await their turn to access the lock and enter the critical section.

The default implementation uses `pthread` (POSIX Threads) to access the thread's "ID", pause the thread (using `sigwait`) and resume the thread (with `pthread_kill`).

The default behavior can be controlled using the following MACROS:

* the `FIO_THREAD_T` macro should return a thread type, default: `pthread_t`

* the `FIO_THREAD_ID()` macro should return this thread's FIO_THREAD_T.

* the `FIO_THREAD_PAUSE(id)` macro should temporarily pause thread execution.

* the `FIO_THREAD_RESUME(id)` macro should resume thread execution.

#### `fio_lock2_s`

```c
typedef struct {
  volatile size_t lock;
  fio___lock2_wait_s *waiting; /**/
} fio_lock2_s;
```

The `fio_lock2_s` type **must be considered opaque** and the struct's fields should **never** be accessed directly.

The `fio_lock2_s` type is the lock's type.

#### `fio_trylock2`

```c
uint8_t fio_trylock2(fio_lock2_s *lock, size_t group);
```

Tries to lock a multilock.

Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
macro. i.e.:

```c
if(!fio_trylock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2))) {
  // act in lock
  fio_unlock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2));
}
```

Returns 0 on success and non-zero on failure.

#### `fio_lock2`

```c
void fio_lock2(fio_lock2_s *lock, size_t group);
```

Locks a multilock, waiting as needed.

Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
macro. i.e.:

     fio_lock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2)));

Doesn't return until a successful lock was acquired.

#### `fio_unlock2`

```c
void fio_unlock2(fio_lock2_s *lock, size_t group);
```

Unlocks a multilock, regardless of who owns the locked group.

Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
macro. i.e.:

```c
fio_unlock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2));
````

-------------------------------------------------------------------------------
