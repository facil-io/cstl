/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************
C++ extern start
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************




                          Common internal Macros



These are re-defined for ever `include` cycle
***************************************************************************** */

/* *****************************************************************************
Memory allocation macros
***************************************************************************** */

#if defined(FIO_MEM_RESET) || !defined(FIO_MEM_REALLOC) ||                     \
    !defined(FIO_MEM_FREE)

#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#undef FIO_MEM_RESET

/* if a global allocator was previously defined route macros to fio_malloc */
#if defined(H___FIO_MALLOC___H)
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio_realloc2((ptr), (new_size), (copy_len))
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))
/** Set to true of internall allocator is used (memory returned set to zero). */
#define FIO_MEM_REALLOC_IS_SAFE 1

#else /* H___FIO_MALLOC___H */
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  realloc((ptr), (new_size))
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) free((ptr))
/** Set to true of internall allocator is used (memory returned set to zero). */
#define FIO_MEM_REALLOC_IS_SAFE 0
#endif /* H___FIO_MALLOC___H */

#endif /* defined(FIO_MEM_REALLOC) */

/** FIO_MEMORY_DISABLE disables all custom memory allocators. */
#if defined(FIO_MEMORY_DISABLE)
#ifndef FIO_MALLOC_TMP_USE_SYSTEM
#define FIO_MALLOC_TMP_USE_SYSTEM 1
#endif
#endif

/* recursive? */
#if !defined(FIO_MEM_REALLOC_) || !defined(FIO_MEM_FREE_)
#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_
#undef FIO_MEM_REALLOC_IS_SAFE_

#ifdef FIO_MALLOC_TMP_USE_SYSTEM /* force malloc */
#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  realloc((ptr), (new_size))
#define FIO_MEM_FREE_(ptr, size) free((ptr))
#define FIO_MEM_REALLOC_IS_SAFE_ 0

#else /* FIO_MALLOC_TMP_USE_SYSTEM */
#define FIO_MEM_REALLOC_         FIO_MEM_REALLOC
#define FIO_MEM_FREE_            FIO_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIO_MEM_REALLOC_IS_SAFE
#endif /* FIO_MALLOC_TMP_USE_SYSTEM */

#endif /* !defined(FIO_MEM_REALLOC_)... */

/* *****************************************************************************
Locking selector
***************************************************************************** */

#ifndef FIO_USE_THREAD_MUTEX_TMP
#define FIO_USE_THREAD_MUTEX_TMP FIO_USE_THREAD_MUTEX
#endif

#if FIO_USE_THREAD_MUTEX_TMP
#define FIO_THREADS
#define FIO___LOCK_NAME          "OS mutex"
#define FIO___LOCK_TYPE          fio_thread_mutex_t
#define FIO___LOCK_INIT          ((FIO___LOCK_TYPE)FIO_THREAD_MUTEX_INIT)
#define FIO___LOCK_DESTROY(lock) fio_thread_mutex_destroy(&(lock))
#define FIO___LOCK_LOCK(lock)                                                  \
  do {                                                                         \
    if (fio_thread_mutex_lock(&(lock)))                                        \
      FIO_LOG_ERROR("Couldn't lock mutex @ %s:%d - error (%d): %s",            \
                    __FILE__,                                                  \
                    __LINE__,                                                  \
                    errno,                                                     \
                    strerror(errno));                                          \
  } while (0)
#define FIO___LOCK_TRYLOCK(lock) fio_thread_mutex_trylock(&(lock))
#define FIO___LOCK_UNLOCK(lock)                                                \
  do {                                                                         \
    if (fio_thread_mutex_unlock(&(lock))) {                                    \
      FIO_LOG_ERROR("Couldn't release mutex @ %s:%d - error (%d): %s",         \
                    __FILE__,                                                  \
                    __LINE__,                                                  \
                    errno,                                                     \
                    strerror(errno));                                          \
    }                                                                          \
  } while (0)

#else
#define FIO___LOCK_NAME          "facil.io spinlocks"
#define FIO___LOCK_TYPE          fio_lock_i
#define FIO___LOCK_INIT          ((FIO___LOCK_TYPE)FIO_LOCK_INIT)
#define FIO___LOCK_DESTROY(lock) ((lock) = FIO___LOCK_INIT)
#define FIO___LOCK_LOCK(lock)    fio_lock(&(lock))
#define FIO___LOCK_TRYLOCK(lock) fio_trylock(&(lock))
#define FIO___LOCK_UNLOCK(lock)  fio_unlock(&(lock))
#endif

/* *****************************************************************************
Recursive inclusion management
***************************************************************************** */
#ifndef SFUNC_ /* if we aren't in a recursive #include statement */

#ifdef FIO_EXTERN
#define SFUNC_
#define IFUNC_

#else /* !FIO_EXTERN */
#undef SFUNC
#undef IFUNC
#define SFUNC_ FIO_SFUNC
#define IFUNC_ FIO_IFUNC
#endif /* FIO_EXTERN */

#undef SFUNC
#undef IFUNC
#define SFUNC SFUNC_
#define IFUNC IFUNC_

#elif !defined(FIO___RECURSIVE_INCLUDE) || (FIO___RECURSIVE_INCLUDE + 1 != 100)
/* SFUNC_ - internal helper types are always `static` */
#undef SFUNC
#undef IFUNC
#define SFUNC static __attribute__((unused))
#define IFUNC static inline __attribute__((unused))
#endif /* SFUNC_ vs FIO___RECURSIVE_INCLUDE*/

/* *****************************************************************************
Leak Counter Helpers
***************************************************************************** */
#if (FIO_LEAK_COUNTER + 1) == 1
/* No leak counting defined */
#define FIO___LEAK_COUNTER_DEF(name)
#define FIO___LEAK_COUNTER_ON_ALLOC(name)
#define FIO___LEAK_COUNTER_ON_FREE(name)
#else
#undef FIO___LEAK_COUNTER_DEF
#undef FIO___LEAK_COUNTER_ON_ALLOC
#undef FIO___LEAK_COUNTER_ON_FREE
#define FIO___LEAK_COUNTER_DEF(name)                                           \
  FIO_IFUNC size_t FIO_NAME(fio___leak_counter, name)(size_t i) {              \
    static volatile size_t counter;                                            \
    size_t tmp = fio_atomic_add_fetch(&counter, i);                            \
    if (tmp == ((size_t)-1))                                                   \
      goto error_double_free;                                                  \
    return tmp;                                                                \
  error_double_free:                                                           \
    FIO_ASSERT(0, FIO_MACRO2STR(name) " `free` after `free` detected!");       \
  }                                                                            \
  static void FIO_NAME(fio___leak_counter_cleanup, name)(void *i) {            \
    size_t counter = FIO_NAME(fio___leak_counter, name)((size_t)(uintptr_t)i); \
    FIO_LOG_DDEBUG2("testing leaks for " FIO_MACRO2STR(name));                 \
    if (counter)                                                               \
      FIO_LOG_ERROR("%zu leaks detected for " FIO_MACRO2STR(name), counter);   \
  }                                                                            \
  FIO_CONSTRUCTOR(FIO_NAME(fio___leak_counter_const, name)) {                  \
    fio_state_callback_add(FIO_CALL_AT_EXIT,                                   \
                           FIO_NAME(fio___leak_counter_cleanup, name),         \
                           NULL);                                              \
  }
#define FIO___LEAK_COUNTER_ON_ALLOC(name) FIO_NAME(fio___leak_counter, name)(1)
#define FIO___LEAK_COUNTER_ON_FREE(name)                                       \
  FIO_NAME(fio___leak_counter, name)(((size_t)-1))
#endif

/* *****************************************************************************
Pointer Tagging
***************************************************************************** */
#ifndef FIO_PTR_TAG
/**
 * Supports embedded pointer tagging / untagging for the included types.
 *
 * Should resolve to a tagged pointer value. i.e.: ((uintptr_t)(p) | 1)
 */
#define FIO_PTR_TAG(p) (p)
#endif

#ifndef FIO_PTR_UNTAG
/**
 * Supports embedded pointer tagging / untagging for the included types.
 *
 * Should resolve to an untagged pointer value. i.e.: ((uintptr_t)(p) | ~1UL)
 */
#define FIO_PTR_UNTAG(p) (p)
#endif

/**
 * If FIO_PTR_TAG_TYPE is defined, then functions returning a type's pointer
 * will return a pointer of the specified type instead.
 */
#ifndef FIO_PTR_TAG_TYPE
#endif

#ifndef FIO_PTR_TAG_VALIDATE
/**
 * If FIO_PTR_TAG_VALIDATE is defined, tagging will be verified before executing
 * any code.
 *
 * FIO_PTR_TAG_VALIDATE must fail on NULL pointers.
 */
#define FIO_PTR_TAG_VALIDATE(ptr) ((ptr) != NULL)
#endif

#undef FIO_PTR_TAG_VALID_OR_RETURN
#define FIO_PTR_TAG_VALID_OR_RETURN(tagged_ptr, value)                         \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE((tagged_ptr)))) {                               \
      FIO_LOG_DEBUG("pointer tag (type) mismatch in function call.");          \
      return (value);                                                          \
    }                                                                          \
  } while (0)
#undef FIO_PTR_TAG_VALID_OR_RETURN_VOID
#define FIO_PTR_TAG_VALID_OR_RETURN_VOID(tagged_ptr)                           \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE((tagged_ptr)))) {                               \
      FIO_LOG_DEBUG("pointer tag (type) mismatch in function call.");          \
      return;                                                                  \
    }                                                                          \
  } while (0)
#undef FIO_PTR_TAG_VALID_OR_GOTO
#define FIO_PTR_TAG_VALID_OR_GOTO(tagged_ptr, lable)                           \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE((tagged_ptr)))) {                               \
      /* Log error since GOTO indicates cleanup or other side-effects. */      \
      FIO_LOG_ERROR("(" FIO__FILE__ ":" FIO_MACRO2STR(                         \
          __LINE__) ") pointer tag (type) mismatch in function call.");        \
      goto lable;                                                              \
    }                                                                          \
  } while (0)

#define FIO_PTR_TAG_GET_UNTAGGED(untagged_type, tagged_ptr)                    \
  ((untagged_type *)(FIO_PTR_UNTAG((tagged_ptr))))
