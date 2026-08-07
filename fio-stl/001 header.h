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

/** FIO_MEMORY_DISABLE disables all custom memory allocators. */
#if defined(FIO_MEMORY_DISABLE)
#ifndef FIO_MALLOC_TMP_USE_SYSTEM
#define FIO_MALLOC_TMP_USE_SYSTEM 1
#endif
#endif

#if defined(FIO_MEM_RESET) || !defined(FIO_MEM_REALLOC) ||                     \
    !defined(FIO_MEM_FREE)

#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#undef FIO_MEM_ALIGNMENT_SIZE
#undef FIO_MEM_REALLOC_ALIGNED
#undef FIO_MEM_FREE_ALIGNED
#undef FIO_MEM_ALLOC_SIZE
#undef FIO_MEM_RESET

/* if a global allocator was previously defined route macros to fio_malloc */
#if defined(H___FIO_MALLOC___H)
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio_realloc2((ptr), (new_size), (copy_len))
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))
/** Set to true of internall allocator is used (memory returned set to zero). */
#define FIO_MEM_REALLOC_IS_SAFE fio_realloc_is_safe()
/** Detect allocator allignment dynamically. */
#define FIO_MEM_ALIGNMENT_SIZE fio_malloc_alignment()
/** Reallocates memory with an alignment requirement, assigning `ptr`. */
#define FIO_MEM_REALLOC_ALIGNED(ptr, old_size, new_size, copy_len, alignment)  \
  ((ptr) = fio_realloc_aligned((ptr), (new_size), (copy_len), (alignment)))
/** Frees memory allocated using FIO_MEM_REALLOC_ALIGNED. */
#if defined(FIO_MALLOC_TMP_USE_SYSTEM) && FIO_OS_WIN
#define FIO_MEM_FREE_ALIGNED(ptr, size) _aligned_free((ptr))
#elif defined(FIO_MALLOC_TMP_USE_SYSTEM) && !FIO_OS_POSIX
#define FIO_MEM_FREE_ALIGNED(ptr, size) fio___aligned_free_fallback((ptr))
#else
#define FIO_MEM_FREE_ALIGNED(ptr, size) fio_free((ptr))
#endif
/** Returns the usable size the allocator reserves for a `size` request. */
#define FIO_MEM_ALLOC_SIZE(size) fio_alloc_size((size))

#else /* H___FIO_MALLOC___H */
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  realloc((ptr), (new_size))
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) free((ptr))
/** Set to true of internall allocator is used (memory returned set to zero). */
#define FIO_MEM_REALLOC_IS_SAFE 0
/** Assume allocator allignment. */
#define FIO_MEM_ALIGNMENT_SIZE  sizeof(long double)
/** Reallocates memory with an alignment requirement, assigning `ptr`. */
#define FIO_MEM_REALLOC_ALIGNED(ptr, old_size, new_size, copy_len, alignment)  \
  ((ptr) = fio___aligned_realloc_fallback((ptr), (new_size), (copy_len),       \
                                          (alignment)))
/** Frees memory allocated using FIO_MEM_REALLOC_ALIGNED. */
#define FIO_MEM_FREE_ALIGNED(ptr, size) fio___aligned_free_fallback((ptr))
/** Returns the usable size the allocator reserves for a `size` request. */
#define FIO_MEM_ALLOC_SIZE(size) ((size_t)(size))
#endif /* H___FIO_MALLOC___H */

#endif /* defined(FIO_MEM_REALLOC) */

/* recursive? */
#if !defined(FIO_MEM_REALLOC_) || !defined(FIO_MEM_FREE_)
#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_
#undef FIO_MEM_REALLOC_IS_SAFE_
#undef FIO_MEM_ALIGNMENT_SIZE_
#undef FIO_MEM_REALLOC_ALIGNED_
#undef FIO_MEM_FREE_ALIGNED_

#ifdef FIO_MALLOC_TMP_USE_SYSTEM /* force malloc */
#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  realloc((ptr), (new_size))
#define FIO_MEM_FREE_(ptr, size) free((ptr))
#define FIO_MEM_REALLOC_IS_SAFE_ 0
#define FIO_MEM_ALIGNMENT_SIZE_  sizeof(long double)
#define FIO_MEM_REALLOC_ALIGNED_(ptr, old_size, new_size, copy_len, alignment) \
  ((ptr) = fio___aligned_realloc_fallback((ptr), (new_size), (copy_len),       \
                                          (alignment)))
#define FIO_MEM_FREE_ALIGNED_(ptr, size) fio___aligned_free_fallback((ptr))

#else /* FIO_MALLOC_TMP_USE_SYSTEM */
#define FIO_MEM_REALLOC_         FIO_MEM_REALLOC
#define FIO_MEM_FREE_            FIO_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIO_MEM_REALLOC_IS_SAFE
#define FIO_MEM_ALIGNMENT_SIZE_  FIO_MEM_ALIGNMENT_SIZE
#define FIO_MEM_REALLOC_ALIGNED_ FIO_MEM_REALLOC_ALIGNED
#define FIO_MEM_FREE_ALIGNED_    FIO_MEM_FREE_ALIGNED
#endif /* FIO_MALLOC_TMP_USE_SYSTEM */

#endif /* !defined(FIO_MEM_REALLOC_)... */

/* *****************************************************************************
Aligned allocation fallback (system allocator backends)
***************************************************************************** */
#ifndef H___FIO_MEM_ALIGNED_FALLBACK___H
#define H___FIO_MEM_ALIGNED_FALLBACK___H

#if FIO_OS_WIN
#include <malloc.h>
#endif

#ifndef FIO_MEM_SYS_ALIGN_MAX_LOG
/** Mirrors the custom allocator's default FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG. */
#define FIO_MEM_SYS_ALIGN_MAX_LOG 21
#endif

/* SublimeText marker */
void fio___aligned_free_fallback__(void);
/**
 * Frees memory allocated by `fio___aligned_realloc_fallback` /
 * `fio___aligned_alloc_fallback`.
 *
 * NOTE: on Windows this maps to `_aligned_free` - do NOT interleave with
 * plain `malloc` / `free`.
 */
FIO_IFUNC void fio___aligned_free_fallback(void *ptr) {
  if (!ptr)
    return;
#if FIO_OS_WIN
  _aligned_free(ptr);
#elif FIO_OS_POSIX
  free(ptr);
#else /* the raw pointer was stashed before the aligned pointer */
  free(((void **)ptr)[-1]);
#endif
}

/* SublimeText marker */
void fio___aligned_alloc_fallback__(void);
/**
 * Aligned allocation fallback. `alignment` MUST be a normalized power of 2
 * (see fio___aligned_realloc_fallback).
 */
FIO_IFUNC void *fio___aligned_alloc_fallback(size_t size, size_t alignment) {
#if FIO_OS_WIN
  return _aligned_malloc(size, alignment);
#elif FIO_OS_POSIX
  void *r = NULL;
  if (posix_memalign(&r, alignment, size))
    return NULL;
  return r;
#else /* over-allocation with the raw pointer stashed before the result */
  void *raw = malloc(size + alignment + sizeof(void *));
  void *r;
  if (!raw)
    return NULL;
  r = (void *)(((uintptr_t)raw + sizeof(void *) + (alignment - 1)) &
               ~(uintptr_t)(alignment - 1));
  ((void **)r)[-1] = raw;
  return r;
#endif
}

/* SublimeText marker */
void fio___aligned_realloc_fallback__(void);
/**
 * Aligned reallocation fallback, used when no fio allocator is linked (or
 * `FIO_MALLOC_TMP_USE_SYSTEM` is enforced).
 *
 * The `alignment` argument is normalized: `0` selects the default
 * (`sizeof(long double)`), non power-of-2 values are rounded DOWN to the
 * nearest power of 2, and the result is never smaller than the current
 * alignment of `ptr`.
 */
FIO_IFUNC void *fio___aligned_realloc_fallback(void *ptr,
                                               size_t new_size,
                                               size_t copy_len,
                                               size_t alignment) {
  /* normalize: power-of-2 floor; floor at the pointer's own alignment */
  if (alignment)
    alignment = (size_t)1 << fio_msb_index_unsafe(alignment);
  if (ptr) {
    size_t ptr_alignment = (size_t)1 << fio_lsb_index_unsafe((uintptr_t)ptr);
    if (ptr_alignment > (((size_t)1) << FIO_MEM_SYS_ALIGN_MAX_LOG))
      ptr_alignment = (((size_t)1) << FIO_MEM_SYS_ALIGN_MAX_LOG);
    if (ptr_alignment > alignment)
      alignment = ptr_alignment;
  }
  if (alignment < sizeof(long double))
    alignment = sizeof(long double);
#if FIO_OS_WIN
  if (!ptr)
    return _aligned_malloc(new_size ? new_size : alignment, alignment);
  if (!new_size) {
    _aligned_free(ptr);
    return NULL;
  }
  {
    /* NOTE: _aligned_realloc cannot realign under Wine's MSVCRT (EINVAL).
     * Emulate: allocate-copy-free (always correct, also on real Windows). */
    void *r = _aligned_malloc(new_size, alignment);
    if (!r)
      return NULL;
    fio___memcpy_unsafe_x(r, ptr, (copy_len < new_size) ? copy_len : new_size);
    _aligned_free(ptr);
    return r;
  }
#else
  if (!new_size && ptr) {
    fio___aligned_free_fallback(ptr);
    return NULL;
  }
  {
    void *r = fio___aligned_alloc_fallback(new_size ? new_size : alignment,
                                           alignment);
    if (!r)
      return NULL;
    if (ptr) {
      /* core primitive copy (FIO_MEMCPY may resolve to a later SFUNC) */
      fio___memcpy_unsafe_x(r, ptr, (copy_len < new_size) ? copy_len : new_size);
      fio___aligned_free_fallback(ptr);
    }
    return r;
  }
#endif
}
#endif /* H___FIO_MEM_ALIGNED_FALLBACK___H */

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
#define SFUNC static FIO_MAYBE_UNUSED
#define IFUNC static inline FIO_MAYBE_UNUSED
#endif /* SFUNC_ vs FIO___RECURSIVE_INCLUDE*/

/* *****************************************************************************
Leak Counter Helpers
***************************************************************************** */
#undef FIO_LEAK_COUNTER_DEF
#undef FIO_LEAK_COUNTER_ON_ALLOC
#undef FIO_LEAK_COUNTER_ON_FREE
#undef FIO_LEAK_COUNTER_COUNT

#if ((FIO_LEAK_COUNTER + 1) == 1)
/* No leak counting defined */
#define FIO_LEAK_COUNTER_DEF(name)
#define FIO_LEAK_COUNTER_ON_ALLOC(name) ((void)0)
#define FIO_LEAK_COUNTER_ON_FREE(name)  ((void)0)
#define FIO_LEAK_COUNTER_COUNT(name)    ((size_t)0)
#else
#ifndef FIO_LEAK_COUNTER_SKIP_EXIT
#define FIO_LEAK_COUNTER_SKIP_EXIT 0
#endif
#define FIO_LEAK_COUNTER_DEF      FIO___LEAK_COUNTER_DEF
#define FIO_LEAK_COUNTER_COUNT    FIO___LEAK_COUNTER_COUNT
#define FIO_LEAK_COUNTER_ON_ALLOC FIO___LEAK_COUNTER_ON_ALLOC
#define FIO_LEAK_COUNTER_ON_FREE  FIO___LEAK_COUNTER_ON_FREE
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
      FIO_LOG_ERROR("(" FIO___FILE__ ":" FIO_MACRO2STR(                        \
          __LINE__) ") pointer tag (type) mismatch in function call.");        \
      goto lable;                                                              \
    }                                                                          \
  } while (0)

#define FIO_PTR_TAG_GET_UNTAGGED(untagged_type, tagged_ptr)                    \
  ((untagged_type *)(FIO_PTR_UNTAG((tagged_ptr))))
