/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_LOCK2              /* Development inclusion - ignore line */
#define FIO_ATOMIC             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                            Atomic Operations



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */

#if defined(FIO_ATOMIC) && !defined(H___FIO_ATOMIC___H)
#define H___FIO_ATOMIC___H 1

/* C11 Atomics are defined? */
#if defined(__ATOMIC_RELAXED)
/** An atomic load operation, returns value in pointer. */
#define fio_atomic_load(dest, p_obj)                                           \
  do {                                                                         \
    dest = __atomic_load_n((p_obj), __ATOMIC_SEQ_CST);                         \
  } while (0)

// clang-format off

/** An atomic compare and exchange operation, returns true if an exchange occured. `p_expected` MAY be overwritten with the existing value (system specific). */
#define fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired) __atomic_compare_exchange((p_obj), (p_expected), (p_desired), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
/** An atomic exchange operation, returns previous value */
#define fio_atomic_exchange(p_obj, value) __atomic_exchange_n((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic addition operation, returns previous value */
#define fio_atomic_add(p_obj, value) __atomic_fetch_add((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic subtraction operation, returns previous value */
#define fio_atomic_sub(p_obj, value) __atomic_fetch_sub((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic AND (&) operation, returns previous value */
#define fio_atomic_and(p_obj, value) __atomic_fetch_and((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic XOR (^) operation, returns previous value */
#define fio_atomic_xor(p_obj, value) __atomic_fetch_xor((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic OR (|) operation, returns previous value */
#define fio_atomic_or(p_obj, value) __atomic_fetch_or((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic NOT AND ((~)&) operation, returns previous value */
#define fio_atomic_nand(p_obj, value) __atomic_fetch_nand((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic addition operation, returns new value */
#define fio_atomic_add_fetch(p_obj, value) __atomic_add_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic subtraction operation, returns new value */
#define fio_atomic_sub_fetch(p_obj, value) __atomic_sub_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic AND (&) operation, returns new value */
#define fio_atomic_and_fetch(p_obj, value) __atomic_and_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic XOR (^) operation, returns new value */
#define fio_atomic_xor_fetch(p_obj, value) __atomic_xor_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic OR (|) operation, returns new value */
#define fio_atomic_or_fetch(p_obj, value) __atomic_or_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic NOT AND ((~)&) operation, returns new value */
#define fio_atomic_nand_fetch(p_obj, value) __atomic_nand_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/* note: __ATOMIC_SEQ_CST may be safer and __ATOMIC_ACQ_REL may be faster */

/* Select the correct compiler builtin method. */
#elif __has_builtin(__sync_add_and_fetch) || (__GNUC__ > 3)
/** An atomic load operation, returns value in pointer. */
#define fio_atomic_load(dest, p_obj)                                           \
  do {                                                                         \
    dest = *(p_obj);                                                           \
  } while (!__sync_bool_compare_and_swap((p_obj), dest, dest))


/** An atomic compare and exchange operation, returns true if an exchange occured. `p_expected` MAY be overwritten with the existing value (system specific). */
#define fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired) __sync_bool_compare_and_swap((p_obj), (p_expected), *(p_desired))
/** An atomic exchange operation, ruturns previous value */
#define fio_atomic_exchange(p_obj, value) __sync_val_compare_and_swap((p_obj), *(p_obj), (value))
/** An atomic addition operation, returns new value */
#define fio_atomic_add(p_obj, value) __sync_fetch_and_add((p_obj), (value))
/** An atomic subtraction operation, returns new value */
#define fio_atomic_sub(p_obj, value) __sync_fetch_and_sub((p_obj), (value))
/** An atomic AND (&) operation, returns new value */
#define fio_atomic_and(p_obj, value) __sync_fetch_and_and((p_obj), (value))
/** An atomic XOR (^) operation, returns new value */
#define fio_atomic_xor(p_obj, value) __sync_fetch_and_xor((p_obj), (value))
/** An atomic OR (|) operation, returns new value */
#define fio_atomic_or(p_obj, value) __sync_fetch_and_or((p_obj), (value))
/** An atomic NOT AND ((~)&) operation, returns new value */
#define fio_atomic_nand(p_obj, value) __sync_fetch_and_nand((p_obj), (value))
/** An atomic addition operation, returns previous value */
#define fio_atomic_add_fetch(p_obj, value) __sync_add_and_fetch((p_obj), (value))
/** An atomic subtraction operation, returns previous value */
#define fio_atomic_sub_fetch(p_obj, value) __sync_sub_and_fetch((p_obj), (value))
/** An atomic AND (&) operation, returns previous value */
#define fio_atomic_and_fetch(p_obj, value) __sync_and_and_fetch((p_obj), (value))
/** An atomic XOR (^) operation, returns previous value */
#define fio_atomic_xor_fetch(p_obj, value) __sync_xor_and_fetch((p_obj), (value))
/** An atomic OR (|) operation, returns previous value */
#define fio_atomic_or_fetch(p_obj, value) __sync_or_and_fetch((p_obj), (value))
/** An atomic NOT AND ((~)&) operation, returns previous value */
#define fio_atomic_nand_fetch(p_obj, value) __sync_nand_and_fetch((p_obj), (value))


#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#ifdef _MSC_VER
#pragma message ("Fallback to C11 atomic header, might be missing some features.")
#undef FIO_COMPILER_GUARD
#define FIO_COMPILER_GUARD atomic_thread_fence(memory_order_seq_cst)
#else
#warning Fallback to C11 atomic header, might be missing some features.
#endif /* _MSC_VER */
/** An atomic load operation, returns value in pointer. */
#define fio_atomic_load(dest, p_obj)  (dest = atomic_load(p_obj))

/** An atomic compare and exchange operation, returns true if an exchange occured. `p_expected` MAY be overwritten with the existing value (system specific). */
#define fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired) atomic_compare_exchange_strong((p_obj), (p_expected), (p_desired))
/** An atomic exchange operation, returns previous value */
#define fio_atomic_exchange(p_obj, value) atomic_exchange((p_obj), (value))
/** An atomic addition operation, returns previous value */
#define fio_atomic_add(p_obj, value) atomic_fetch_add((p_obj), (value))
/** An atomic subtraction operation, returns previous value */
#define fio_atomic_sub(p_obj, value) atomic_fetch_sub((p_obj), (value))
/** An atomic AND (&) operation, returns previous value */
#define fio_atomic_and(p_obj, value) atomic_fetch_and((p_obj), (value))
/** An atomic XOR (^) operation, returns previous value */
#define fio_atomic_xor(p_obj, value) atomic_fetch_xor((p_obj), (value))
/** An atomic OR (|) operation, returns previous value */
#define fio_atomic_or(p_obj, value) atomic_fetch_or((p_obj), (value))
/** An atomic NOT AND ((~)&) operation, returns previous value */
#define fio_atomic_nand(p_obj, value) atomic_fetch_nand((p_obj), (value))
/** An atomic addition operation, returns new value */
#define fio_atomic_add_fetch(p_obj, value) (atomic_fetch_add((p_obj), (value)), atomic_load((p_obj)))
/** An atomic subtraction operation, returns new value */
#define fio_atomic_sub_fetch(p_obj, value) (atomic_fetch_sub((p_obj), (value)), atomic_load((p_obj)))
/** An atomic AND (&) operation, returns new value */
#define fio_atomic_and_fetch(p_obj, value) (atomic_fetch_and((p_obj), (value)), atomic_load((p_obj)))
/** An atomic XOR (^) operation, returns new value */
#define fio_atomic_xor_fetch(p_obj, value) (atomic_fetch_xor((p_obj), (value)), atomic_load((p_obj)))
/** An atomic OR (|) operation, returns new value */
#define fio_atomic_or_fetch(p_obj, value) (atomic_fetch_or((p_obj), (value)), atomic_load((p_obj)))

#elif _MSC_VER
#pragma message ("Warning: WinAPI atomics have less features, but this is what this compiler has, so...")
#include <intrin.h>
#define FIO___ATOMICS_FN_ROUTE(fn, ptr, ...)                                   \
  ((sizeof(*ptr) == 1)                                                         \
       ? fn##8((int8_t volatile *)(ptr), __VA_ARGS__)                          \
       : (sizeof(*ptr) == 2)                                                   \
             ? fn##16((int16_t volatile *)(ptr), __VA_ARGS__)                  \
             : (sizeof(*ptr) == 4)                                             \
                   ? fn((int32_t volatile *)(ptr), __VA_ARGS__)                \
                   : fn##64((int64_t volatile *)(ptr), __VA_ARGS__))

#ifndef _WIN64
#error Atomics on Windows require 64bit OS and compiler support.
#endif

/** An atomic load operation, returns value in pointer. */
#define fio_atomic_load(dest, p_obj) (dest = *(p_obj))

/** An atomic compare and exchange operation, returns true if an exchange occured. `p_expected` MAY be overwritten with the existing value (system specific). */
#define fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired) (FIO___ATOMICS_FN_ROUTE(_InterlockedCompareExchange, (p_obj),(*(p_desired)),(*(p_expected))), (*(p_obj) == *(p_desired)))
/** An atomic exchange operation, returns previous value */
#define fio_atomic_exchange(p_obj, value) FIO___ATOMICS_FN_ROUTE(_InterlockedExchange, (p_obj), (value))

/** An atomic addition operation, returns previous value */
#define fio_atomic_add(p_obj, value) FIO___ATOMICS_FN_ROUTE(_InterlockedExchangeAdd, (p_obj), (value))
/** An atomic subtraction operation, returns previous value */
#define fio_atomic_sub(p_obj, value) FIO___ATOMICS_FN_ROUTE(_InterlockedExchangeAdd, (p_obj), (0ULL - (value)))
/** An atomic AND (&) operation, returns previous value */
#define fio_atomic_and(p_obj, value) FIO___ATOMICS_FN_ROUTE(_InterlockedAnd, (p_obj), (value))
/** An atomic XOR (^) operation, returns previous value */
#define fio_atomic_xor(p_obj, value) FIO___ATOMICS_FN_ROUTE(_InterlockedXor, (p_obj), (value))
/** An atomic OR (|) operation, returns previous value */
#define fio_atomic_or(p_obj, value)  FIO___ATOMICS_FN_ROUTE(_InterlockedOr, (p_obj), (value))

/** An atomic addition operation, returns new value */
#define fio_atomic_add_fetch(p_obj, value) (fio_atomic_add((p_obj), (value)), (*(p_obj)))
/** An atomic subtraction operation, returns new value */
#define fio_atomic_sub_fetch(p_obj, value) (fio_atomic_sub((p_obj), (value)), (*(p_obj)))
/** An atomic AND (&) operation, returns new value */
#define fio_atomic_and_fetch(p_obj, value) (fio_atomic_and((p_obj), (value)), (*(p_obj)))
/** An atomic XOR (^) operation, returns new value */
#define fio_atomic_xor_fetch(p_obj, value) (fio_atomic_xor((p_obj), (value)), (*(p_obj)))
/** An atomic OR (|) operation, returns new value */
#define fio_atomic_or_fetch(p_obj, value) (fio_atomic_or((p_obj), (value)), (*(p_obj)))
#else
#error Required atomics not found (__STDC_NO_ATOMICS__) and older __sync_add_and_fetch is also missing.

#endif
// clang-format on

#define FIO_LOCK_INIT         0
#define FIO_LOCK_SUBLOCK(sub) ((uint8_t)(1U) << ((sub)&7))
typedef volatile unsigned char fio_lock_i;

/** Tries to lock a specific sublock. Returns 0 on success and 1 on failure. */
FIO_IFUNC uint8_t fio_trylock_sublock(fio_lock_i *lock, uint8_t sub) {
  FIO_COMPILER_GUARD;
  sub &= 7;
  uint8_t sub_ = 1U << sub;
  return ((fio_atomic_or(lock, sub_) & sub_) >> sub);
}

/** Busy waits for a specific sublock to become available - not recommended. */
FIO_IFUNC void fio_lock_sublock(fio_lock_i *lock, uint8_t sub) {
  while (fio_trylock_sublock(lock, sub)) {
    FIO_THREAD_RESCHEDULE();
  }
}

/** Unlocks the specific sublock, no matter which thread owns the lock. */
FIO_IFUNC void fio_unlock_sublock(fio_lock_i *lock, uint8_t sub) {
  sub = 1U << (sub & 7);
  fio_atomic_and(lock, (~(fio_lock_i)sub));
}

/**
 * Tries to lock a group of sublocks.
 *
 * Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
 * macro. i.e.:
 *
 *      if(!fio_trylock_group(&lock,
 *                            FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2))) {
 *         // act in lock
 *      }
 *
 * Returns 0 on success and non-zero on failure.
 */
FIO_IFUNC uint8_t fio_trylock_group(fio_lock_i *lock, uint8_t group) {
  if (!group)
    group = 1;
  FIO_COMPILER_GUARD;
  uint8_t state = fio_atomic_or(lock, group);
  if (!(state & group))
    return 0;
  /* release the locks we aquired, which are: ((~state) & group) */
  fio_atomic_and(lock, ~((~state) & group));
  return 1;
}

/**
 * Busy waits for a group lock to become available - not recommended.
 *
 * See `fio_trylock_group` for details.
 */
FIO_IFUNC void fio_lock_group(fio_lock_i *lock, uint8_t group) {
  while (fio_trylock_group(lock, group)) {
    FIO_THREAD_RESCHEDULE();
  }
}

/** Unlocks a sublock group, no matter which thread owns which sublock. */
FIO_IFUNC void fio_unlock_group(fio_lock_i *lock, uint8_t group) {
  if (!group)
    group = 1;
  fio_atomic_and(lock, (~group));
}

/** Tries to lock all sublocks. Returns 0 on success and 1 on failure. */
FIO_IFUNC uint8_t fio_trylock_full(fio_lock_i *lock) {
  FIO_COMPILER_GUARD;
  fio_lock_i old = fio_atomic_or(lock, ~(fio_lock_i)0);
  if (!old)
    return 0;
  fio_atomic_and(lock, old);
  return 1;
}

/** Busy waits for all sub lock to become available - not recommended. */
FIO_IFUNC void fio_lock_full(fio_lock_i *lock) {
  while (fio_trylock_full(lock)) {
    FIO_THREAD_RESCHEDULE();
  }
}

/** Unlocks all sub locks, no matter which thread owns the lock. */
FIO_IFUNC void fio_unlock_full(fio_lock_i *lock) { fio_atomic_and(lock, 0); }

/**
 * Tries to acquire the default lock (sublock 0).
 *
 * Returns 0 on success and 1 on failure.
 */
FIO_IFUNC uint8_t fio_trylock(fio_lock_i *lock) {
  return fio_trylock_sublock(lock, 0);
}

/** Busy waits for the default lock to become available - not recommended. */
FIO_IFUNC void fio_lock(fio_lock_i *lock) {
  while (fio_trylock(lock)) {
    FIO_THREAD_RESCHEDULE();
  }
}

/** Unlocks the default lock, no matter which thread owns the lock. */
FIO_IFUNC void fio_unlock(fio_lock_i *lock) { fio_unlock_sublock(lock, 0); }

/** Returns 1 if the lock is locked, 0 otherwise. */
FIO_IFUNC uint8_t FIO_NAME_BL(fio, locked)(fio_lock_i *lock) {
  return *lock & 1;
}

/** Returns 1 if the lock is locked, 0 otherwise. */
FIO_IFUNC uint8_t FIO_NAME_BL(fio, sublocked)(fio_lock_i *lock, uint8_t sub) {
  uint8_t bit = 1U << (sub & 7);
  return (((*lock) & bit) >> (sub & 7));
}

/* *****************************************************************************
Atomic Bit access / manipulation
***************************************************************************** */

/** Gets the state of a bit in a bitmap. */
FIO_IFUNC uint8_t fio_atomic_bit_get(void *map, size_t bit) {
  return ((((uint8_t *)(map))[(bit) >> 3] >> ((bit)&7)) & 1);
}

/** Sets the a bit in a bitmap (sets to 1). */
FIO_IFUNC void fio_atomic_bit_set(void *map, size_t bit) {
  fio_atomic_or((uint8_t *)(map) + ((bit) >> 3), (1UL << ((bit)&7)));
}

/** Unsets the a bit in a bitmap (sets to 0). */
FIO_IFUNC void fio_atomic_bit_unset(void *map, size_t bit) {
  fio_atomic_and((uint8_t *)(map) + ((bit) >> 3),
                 (uint8_t)(~(1UL << ((bit)&7))));
}

/** Flips the a bit in a bitmap (sets to 0 if 1, sets to 1 if 0). */
FIO_IFUNC void fio_atomic_bit_flip(void *map, size_t bit) {
  fio_atomic_xor((uint8_t *)(map) + ((bit) >> 3), (1UL << ((bit)&7)));
}

/* *****************************************************************************
Atomics - cleanup
***************************************************************************** */
#endif /* FIO_ATOMIC */
#undef FIO_ATOMIC
