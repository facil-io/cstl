/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#define FIO_LOCK2                   /* Development inclusion - ignore line */
#define FIO_ATOMIC                  /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                            Atomic Operations




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

// clang-format on

#else
#error Required builtin "__sync_add_and_fetch" not found.
#endif

#define FIO_LOCK_INIT         0
#define FIO_LOCK_SUBLOCK(sub) ((uint8_t)(1U) << ((sub)&7))
typedef volatile unsigned char fio_lock_i;

/** Tries to lock a specific sublock. Returns 0 on success and 1 on failure. */
FIO_IFUNC uint8_t fio_trylock_sublock(fio_lock_i *lock, uint8_t sub) {
  __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
  __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
  __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
Atomic operations - test
***************************************************************************** */
#if defined(FIO_TEST_CSTL)

FIO_SFUNC void FIO_NAME_TEST(stl, atomics)(void) {
  fprintf(stderr, "* Testing atomic operation macros.\n");
  struct fio___atomic_test_s {
    size_t w;
    unsigned long l;
    unsigned short s;
    unsigned char c;
  } s = {0}, r1 = {0}, r2 = {0};
  fio_lock_i lock = FIO_LOCK_INIT;

  r1.c = fio_atomic_add(&s.c, 1);
  r1.s = fio_atomic_add(&s.s, 1);
  r1.l = fio_atomic_add(&s.l, 1);
  r1.w = fio_atomic_add(&s.w, 1);
  FIO_ASSERT(r1.c == 0 && s.c == 1, "fio_atomic_add failed for c");
  FIO_ASSERT(r1.s == 0 && s.s == 1, "fio_atomic_add failed for s");
  FIO_ASSERT(r1.l == 0 && s.l == 1, "fio_atomic_add failed for l");
  FIO_ASSERT(r1.w == 0 && s.w == 1, "fio_atomic_add failed for w");
  r2.c = fio_atomic_add_fetch(&s.c, 1);
  r2.s = fio_atomic_add_fetch(&s.s, 1);
  r2.l = fio_atomic_add_fetch(&s.l, 1);
  r2.w = fio_atomic_add_fetch(&s.w, 1);
  FIO_ASSERT(r2.c == 2 && s.c == 2, "fio_atomic_add_fetch failed for c");
  FIO_ASSERT(r2.s == 2 && s.s == 2, "fio_atomic_add_fetch failed for s");
  FIO_ASSERT(r2.l == 2 && s.l == 2, "fio_atomic_add_fetch failed for l");
  FIO_ASSERT(r2.w == 2 && s.w == 2, "fio_atomic_add_fetch failed for w");
  r1.c = fio_atomic_sub(&s.c, 1);
  r1.s = fio_atomic_sub(&s.s, 1);
  r1.l = fio_atomic_sub(&s.l, 1);
  r1.w = fio_atomic_sub(&s.w, 1);
  FIO_ASSERT(r1.c == 2 && s.c == 1, "fio_atomic_sub failed for c");
  FIO_ASSERT(r1.s == 2 && s.s == 1, "fio_atomic_sub failed for s");
  FIO_ASSERT(r1.l == 2 && s.l == 1, "fio_atomic_sub failed for l");
  FIO_ASSERT(r1.w == 2 && s.w == 1, "fio_atomic_sub failed for w");
  r2.c = fio_atomic_sub_fetch(&s.c, 1);
  r2.s = fio_atomic_sub_fetch(&s.s, 1);
  r2.l = fio_atomic_sub_fetch(&s.l, 1);
  r2.w = fio_atomic_sub_fetch(&s.w, 1);
  FIO_ASSERT(r2.c == 0 && s.c == 0, "fio_atomic_sub_fetch failed for c");
  FIO_ASSERT(r2.s == 0 && s.s == 0, "fio_atomic_sub_fetch failed for s");
  FIO_ASSERT(r2.l == 0 && s.l == 0, "fio_atomic_sub_fetch failed for l");
  FIO_ASSERT(r2.w == 0 && s.w == 0, "fio_atomic_sub_fetch failed for w");
  fio_atomic_add(&s.c, 1);
  fio_atomic_add(&s.s, 1);
  fio_atomic_add(&s.l, 1);
  fio_atomic_add(&s.w, 1);
  r1.c = fio_atomic_exchange(&s.c, 99);
  r1.s = fio_atomic_exchange(&s.s, 99);
  r1.l = fio_atomic_exchange(&s.l, 99);
  r1.w = fio_atomic_exchange(&s.w, 99);
  FIO_ASSERT(r1.c == 1 && s.c == 99, "fio_atomic_exchange failed for c");
  FIO_ASSERT(r1.s == 1 && s.s == 99, "fio_atomic_exchange failed for s");
  FIO_ASSERT(r1.l == 1 && s.l == 99, "fio_atomic_exchange failed for l");
  FIO_ASSERT(r1.w == 1 && s.w == 99, "fio_atomic_exchange failed for w");
  // clang-format off
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.c, &r1.c, &r1.c), "fio_atomic_compare_exchange_p didn't fail for c");
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.s, &r1.s, &r1.s), "fio_atomic_compare_exchange_p didn't fail for s");
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.l, &r1.l, &r1.l), "fio_atomic_compare_exchange_p didn't fail for l");
  FIO_ASSERT(!fio_atomic_compare_exchange_p(&s.w, &r1.w, &r1.w), "fio_atomic_compare_exchange_p didn't fail for w");
  r1.c = 1;s.c = 99; r1.s = 1;s.s = 99; r1.l = 1;s.l = 99; r1.w = 1;s.w = 99; /* ignore system spefcific behavior. */
  r1.c = fio_atomic_compare_exchange_p(&s.c,&s.c, &r1.c);
  r1.s = fio_atomic_compare_exchange_p(&s.s,&s.s, &r1.s);
  r1.l = fio_atomic_compare_exchange_p(&s.l,&s.l, &r1.l);
  r1.w = fio_atomic_compare_exchange_p(&s.w,&s.w, &r1.w);
  FIO_ASSERT(r1.c == 1 && s.c == 1, "fio_atomic_compare_exchange_p failed for c");
  FIO_ASSERT(r1.s == 1 && s.s == 1, "fio_atomic_compare_exchange_p failed for s");
  FIO_ASSERT(r1.l == 1 && s.l == 1, "fio_atomic_compare_exchange_p failed for l");
  FIO_ASSERT(r1.w == 1 && s.w == 1, "fio_atomic_compare_exchange_p failed for w");
  // clang-format on

  uint64_t val = 1;
  FIO_ASSERT(fio_atomic_and(&val, 2) == 1,
             "fio_atomic_and should return old value");
  FIO_ASSERT(val == 0, "fio_atomic_and should update value");
  FIO_ASSERT(fio_atomic_xor(&val, 1) == 0,
             "fio_atomic_xor should return old value");
  FIO_ASSERT(val == 1, "fio_atomic_xor_fetch should update value");
  FIO_ASSERT(fio_atomic_xor_fetch(&val, 1) == 0,
             "fio_atomic_xor_fetch should return new value");
  FIO_ASSERT(val == 0, "fio_atomic_xor should update value");
  FIO_ASSERT(fio_atomic_or(&val, 2) == 0,
             "fio_atomic_or should return old value");
  FIO_ASSERT(val == 2, "fio_atomic_or should update value");
  FIO_ASSERT(fio_atomic_or_fetch(&val, 1) == 3,
             "fio_atomic_or_fetch should return new value");
  FIO_ASSERT(val == 3, "fio_atomic_or_fetch should update value");
  FIO_ASSERT(fio_atomic_nand_fetch(&val, 4) == ~0ULL,
             "fio_atomic_nand_fetch should return new value");
  FIO_ASSERT(val == ~0ULL, "fio_atomic_nand_fetch should update value");
  val = 3ULL;
  FIO_ASSERT(fio_atomic_nand(&val, 4) == 3ULL,
             "fio_atomic_nand should return old value");
  FIO_ASSERT(val == ~0ULL, "fio_atomic_nand_fetch should update value");

  FIO_ASSERT(!fio_is_locked(&lock),
             "lock should be initialized in unlocked state");
  FIO_ASSERT(!fio_trylock(&lock), "fio_trylock should succeed");
  FIO_ASSERT(fio_trylock(&lock), "fio_trylock should fail");
  FIO_ASSERT(fio_is_locked(&lock), "lock should be engaged");
  fio_unlock(&lock);
  FIO_ASSERT(!fio_is_locked(&lock), "lock should be released");
  fio_lock(&lock);
  FIO_ASSERT(fio_is_locked(&lock), "lock should be engaged (fio_lock)");
  for (uint8_t i = 1; i < 8; ++i) {
    FIO_ASSERT(!fio_is_sublocked(&lock, i),
               "sublock flagged, but wasn't engaged (%u - %p)",
               (unsigned int)i,
               (void *)(uintptr_t)lock);
  }
  fio_unlock(&lock);
  FIO_ASSERT(!fio_is_locked(&lock), "lock should be released");
  lock = FIO_LOCK_INIT;
  for (size_t i = 0; i < 8; ++i) {
    FIO_ASSERT(!fio_is_sublocked(&lock, i),
               "sublock should be initialized in unlocked state");
    FIO_ASSERT(!fio_trylock_sublock(&lock, i),
               "fio_trylock_sublock should succeed");
    FIO_ASSERT(fio_trylock_sublock(&lock, i), "fio_trylock should fail");
    FIO_ASSERT(fio_trylock_full(&lock), "fio_trylock_full should fail");
    FIO_ASSERT(fio_is_sublocked(&lock, i), "sub-lock %d should be engaged", i);
    {
      uint8_t g =
          fio_trylock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(3));
      FIO_ASSERT((i != 1 && i != 3 && !g) || ((i == 1 || i == 3) && g),
                 "fio_trylock_group should succeed / fail");
      if (!g)
        fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(3));
    }
    for (uint8_t j = 1; j < 8; ++j) {
      FIO_ASSERT(i == j || !fio_is_sublocked(&lock, j),
                 "another sublock was flagged, though it wasn't engaged");
    }
    FIO_ASSERT(fio_is_sublocked(&lock, i), "lock should remain engaged");
    fio_unlock_sublock(&lock, i);
    FIO_ASSERT(!fio_is_sublocked(&lock, i), "sublock should be released");
    FIO_ASSERT(!fio_trylock_full(&lock), "fio_trylock_full should succeed");
    fio_unlock_full(&lock);
    FIO_ASSERT(!lock, "fio_unlock_full should unlock all");
  }
}

#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
Atomics - cleanup
***************************************************************************** */
#endif /* FIO_ATOMIC */
#undef FIO_ATOMIC

/* *****************************************************************************




                      Multi-Lock with Mutex Emulation




***************************************************************************** */
#if defined(FIO_LOCK2) && !defined(H___FIO_LOCK2___H)
#define H___FIO_LOCK2___H 1

#ifndef FIO_THREAD_T
#include <pthread.h>
#define FIO_THREAD_T pthread_t
#endif

#ifndef FIO_THREAD_ID
#define FIO_THREAD_ID() pthread_self()
#endif

#ifndef FIO_THREAD_PAUSE
#define FIO_THREAD_PAUSE(id)                                                   \
  do {                                                                         \
    sigset_t set___;                                                           \
    int got___sig;                                                             \
    sigemptyset(&set___);                                                      \
    sigaddset(&set___, SIGINT);                                                \
    sigaddset(&set___, SIGTERM);                                               \
    sigaddset(&set___, SIGCONT);                                               \
    sigwait(&set___, &got___sig);                                              \
  } while (0)
#endif

#ifndef FIO_THREAD_RESUME
#define FIO_THREAD_RESUME(id) pthread_kill((id), SIGCONT)
#endif

typedef struct fio___lock2_wait_s fio___lock2_wait_s;

/* *****************************************************************************
Public API
***************************************************************************** */

/**
 * The fio_lock2 variation is a Mutex style multi-lock.
 *
 * Thread functions and types are managed by the following macros:
 * * the `FIO_THREAD_T` macro should return a thread type, default: `pthread_t`
 * * the `FIO_THREAD_ID()` macro should return this thread's FIO_THREAD_T.
 * * the `FIO_THREAD_PAUSE(id)` macro should temporarily pause thread execution.
 * * the `FIO_THREAD_RESUME(id)` macro should resume thread execution.
 */
typedef struct {
  volatile size_t lock;
  fio___lock2_wait_s *volatile waiting;
} fio_lock2_s;

/**
 * Tries to lock a multilock.
 *
 * Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
 * macro. i.e.:
 *
 *      if(!fio_trylock2(&lock,
 *                            FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2))) {
 *         // act in lock
 *      }
 *
 * Returns 0 on success and non-zero on failure.
 */
FIO_IFUNC uint8_t fio_trylock2(fio_lock2_s *lock, size_t group);

/**
 * Locks a multilock, waiting as needed.
 *
 * Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
 * macro. i.e.:
 *
 *      fio_lock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2)));
 *
 * Doesn't return until a successful lock was acquired.
 */
SFUNC void fio_lock2(fio_lock2_s *lock, size_t group);

/**
 * Unlocks a multilock, regardless of who owns the locked group.
 *
 * Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
 * macro. i.e.:
 *
 *      fio_unlock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2));
 *
 */
SFUNC void fio_unlock2(fio_lock2_s *lock, size_t group);

/* *****************************************************************************
Implementation - Inline
***************************************************************************** */

/**
 * Tries to lock a multilock.
 *
 * Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
 * macro. i.e.:
 *
 *      if(!fio_trylock2(&lock,
 *                            FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2))) {
 *         // act in lock
 *      }
 *
 * Returns 0 on success and non-zero on failure.
 */
FIO_IFUNC uint8_t fio_trylock2(fio_lock2_s *lock, size_t group) {
  if (!group)
    group = 1;
  __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
  size_t state = fio_atomic_or(&lock->lock, group);
  if (!(state & group))
    return 0;
  fio_atomic_and(&lock->lock, ~((~state) & group));
  return 1;
}

/* *****************************************************************************
Implementation - Extern
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE)

struct fio___lock2_wait_s {
  struct fio___lock2_wait_s *next;
  struct fio___lock2_wait_s *prev;
  FIO_THREAD_T t;
};

/**
 * Locks a multilock, waiting as needed.
 *
 * Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
 * macro. i.e.:
 *
 *      fio_lock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2)));
 *
 * Doesn't return until a successful lock was acquired.
 */
SFUNC void fio_lock2(fio_lock2_s *lock, size_t group) {
  if (!group)
    group = 1;
  __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
  size_t state = fio_atomic_or(&lock->lock, group);
  if (!(state & group))
    return;
  /* note, we now own the part of the lock */

  /* a lock-wide (all groups) lock ID for the waitlist */
  const size_t inner_lock = (sizeof(inner_lock) >= 8)
                                ? ((size_t)1ULL << 63)
                                : (sizeof(inner_lock) >= 4)
                                      ? ((size_t)1UL << 31)
                                      : (sizeof(inner_lock) >= 2)
                                            ? ((size_t)1UL << 15)
                                            : ((size_t)1UL << 7);

  /* initialize self-waiting node memory (using stack memory) */
  fio___lock2_wait_s self_thread = {
      .next = &self_thread,
      .prev = &self_thread,
      .t = FIO_THREAD_ID(),
  };

  /* enter waitlist spinlock */
  while ((fio_atomic_or(&lock->lock, inner_lock) & inner_lock)) {
    FIO_THREAD_RESCHEDULE();
  }

  /* add self-thread to end of waitlist */
  if (lock->waiting) {
    FIO_LIST_PUSH(lock->waiting, &self_thread);
  } else {
    lock->waiting = &self_thread;
  }

  /* release waitlist spinlock and unlock any locks we may have aquired */
  fio_atomic_xor(&lock->lock, (((~state) & group) | inner_lock));

  for (;;) {
    if (!fio_trylock2(lock, group))
      break;
    /* it's possible the next thread is waiting for a different group */
    if (self_thread.next != lock->waiting) {
      FIO_THREAD_RESUME(self_thread.next->t);
    }
    if (!fio_trylock2(lock, group))
      break;
    FIO_THREAD_PAUSE(self_thread.t);
  }

  /* remove self from waiting list */
  while ((fio_atomic_or(&lock->lock, inner_lock) & inner_lock)) {
    FIO_THREAD_RESCHEDULE();
  }
  if (self_thread.next != lock->waiting) {
    FIO_THREAD_RESUME(self_thread.next->t);
  }
  FIO_LIST_REMOVE(&self_thread);
  fio_atomic_and(&lock->lock, ~inner_lock);
}

/**
 * Unlocks a multilock, regardless of who owns the locked group.
 *
 * Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
 * macro. i.e.:
 *
 *      fio_unlock2(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2));
 *
 */
SFUNC void fio_unlock2(fio_lock2_s *lock, size_t group) {
  /* a lock-wide (all groups) lock ID for the waitlist */
  const size_t inner_lock = (sizeof(inner_lock) >= 8)
                                ? ((size_t)1ULL << 63)
                                : (sizeof(inner_lock) >= 4)
                                      ? ((size_t)1UL << 31)
                                      : (sizeof(inner_lock) >= 2)
                                            ? ((size_t)1UL << 15)
                                            : ((size_t)1UL << 7);
  fio___lock2_wait_s *waiting;
  if (!group)
    group = 1;
  /* spinlock for waitlist */
  while ((fio_atomic_or(&lock->lock, inner_lock) & inner_lock)) {
    FIO_THREAD_RESCHEDULE();
  }
  waiting = lock->waiting;
  /* unlock group & waitlist */
  fio_atomic_and(&lock->lock, ~(group | inner_lock));
  if (waiting) {
    FIO_THREAD_RESUME(waiting->t);
  }
}
#endif /* FIO_EXTERN_COMPLETE */

#endif /* FIO_LOCK2 */
#undef FIO_LOCK2
