/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ARRAY_NAME ary     /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                            Dynamic Arrays



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */

#ifdef FIO_ARRAY_NAME

#ifdef FIO_ARRAY_TYPE_STR
#ifndef FIO_ARRAY_TYPE
#define FIO_ARRAY_TYPE fio_keystr_s
#endif
#ifndef FIO_ARRAY_TYPE_COPY
#define FIO_ARRAY_TYPE_COPY(dest, src) ((dest) = fio_keystr_copy((src)))
#endif
#ifndef FIO_ARRAY_TYPE_DESTROY
#define FIO_ARRAY_TYPE_DESTROY(obj) fio_keystr_destroy(&(obj));
#endif
#ifndef FIO_ARRAY_TYPE_CMP
#define FIO_ARRAY_TYPE_CMP(a, b) fio_keystr_is_eq((a), (b))
#endif
#undef FIO_ARRAY_DESTROY_AFTER_COPY
#define FIO_ARRAY_DESTROY_AFTER_COPY 1
#endif

#ifndef FIO_ARRAY_TYPE
/** The type for array elements (an array of FIO_ARRAY_TYPE) */
#define FIO_ARRAY_TYPE void *
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID        NULL
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#else
#ifndef FIO_ARRAY_TYPE_INVALID
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID        ((FIO_ARRAY_TYPE){0})
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#endif
#endif

#ifndef FIO_ARRAY_TYPE_INVALID_SIMPLE
/** Is the FIO_ARRAY_TYPE_INVALID object memory is all zero? (yes = 1) */
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 0
#endif

#ifndef FIO_ARRAY_TYPE_COPY
/** Handles a copy operation for an array's element. */
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_COPY_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_DESTROY
/** Handles a destroy / free operation for an array's element. */
#define FIO_ARRAY_TYPE_DESTROY(obj)
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_DESTROY_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_CMP
/** Handles a comparison operation for an array's element. */
#define FIO_ARRAY_TYPE_CMP(a, b) (a) == (b)
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_CMP_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_CONCAT_COPY
#define FIO_ARRAY_TYPE_CONCAT_COPY        FIO_ARRAY_TYPE_COPY
#define FIO_ARRAY_TYPE_CONCAT_COPY_SIMPLE FIO_ARRAY_TYPE_COPY_SIMPLE
#endif
/**
 * The FIO_ARRAY_DESTROY_AFTER_COPY macro should be set if
 * FIO_ARRAY_TYPE_DESTROY should be called after FIO_ARRAY_TYPE_COPY when an
 * object is removed from the array after being copied to an external container
 * (an `old` pointer)
 */
#ifndef FIO_ARRAY_DESTROY_AFTER_COPY
#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE && !FIO_ARRAY_TYPE_COPY_SIMPLE
#define FIO_ARRAY_DESTROY_AFTER_COPY 1
#else
#define FIO_ARRAY_DESTROY_AFTER_COPY 0
#endif
#endif

/* Extra empty slots when allocating memory. */
#ifndef FIO_ARRAY_PADDING
#define FIO_ARRAY_PADDING 4
#endif

/*
 * Uses the array structure to embed object, if there's space for them.
 *
 * This optimizes small arrays and specifically touplets. For `void *` type
 * arrays this allows for 2 objects to be embedded, resulting in faster access
 * due to cache locality and reduced pointer redirection.
 *
 * For large arrays, it is better to disable this feature.
 *
 * Note: values larger than 1 add a memory allocation cost to the array
 * container, adding enough room for at least `FIO_ARRAY_ENABLE_EMBEDDED - 1`
 * items.
 */
#ifndef FIO_ARRAY_ENABLE_EMBEDDED
#define FIO_ARRAY_ENABLE_EMBEDDED 1
#endif

/* Sets memory growth to exponentially increase. Consumes more memory. */
#ifndef FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_EXPONENTIAL 0
#endif

#undef FIO_ARRAY_SIZE2WORDS
#define FIO_ARRAY_SIZE2WORDS(size)                                             \
  ((sizeof(FIO_ARRAY_TYPE) & 1)   ? (((size) & (~15)) + 16)                    \
   : (sizeof(FIO_ARRAY_TYPE) & 2) ? (((size) & (~7)) + 8)                      \
   : (sizeof(FIO_ARRAY_TYPE) & 4) ? (((size) & (~3)) + 4)                      \
   : (sizeof(FIO_ARRAY_TYPE) & 8) ? (((size) & (~1)) + 2)                      \
                                  : (size))

/* *****************************************************************************
Dynamic Arrays - type
***************************************************************************** */

/** an Array type. */
typedef struct FIO_NAME(FIO_ARRAY_NAME, s) {
  /* start common header (with embedded array type) */
  /** the offset to the first item. */
  uint32_t start;
  /** The offset to the first empty location the array. */
  uint32_t end;
  /* end common header (with embedded array type) */
  /** The array's capacity - limited to 32bits, but we use the extra padding. */
  uintptr_t capa;
  /** a pointer to the array's memory (if not embedded) */
  FIO_ARRAY_TYPE *ary;
#if FIO_ARRAY_ENABLE_EMBEDDED > 1
  /** Do we wanted larger small-array optimizations? */
  FIO_ARRAY_TYPE
  extra_memory_for_embedded_arrays[(FIO_ARRAY_ENABLE_EMBEDDED - 1)]
#endif
} FIO_NAME(FIO_ARRAY_NAME, s);

#ifdef FIO_PTR_TAG_TYPE
#define FIO_ARRAY_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, s) *
#endif

/* *****************************************************************************
Dynamic Arrays - API
***************************************************************************** */

#ifndef FIO_ARRAY_INIT
/* Initialization macro. */
#define FIO_ARRAY_INIT                                                         \
  { 0 }
#endif

#ifndef FIO_REF_CONSTRUCTOR_ONLY

/* Allocates a new array object on the heap and initializes it's memory. */
SFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, new)(void);

/* Frees an array's internal data AND it's container! */
SFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Destroys any objects stored in the array and frees the internal state. */
SFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_PTR ary);

/** Returns the number of elements in the Array. */
FIO_IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_PTR ary);

/** Returns the current, temporary, array capacity (it's dynamic). */
FIO_IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_PTR ary);

/**
 * Returns 1 if the array is embedded, 0 if it has memory allocated and -1 on an
 * error.
 */
FIO_IFUNC int FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(FIO_ARRAY_PTR ary);

/**
 * Returns a pointer to the C array containing the objects.
 */
FIO_IFUNC FIO_ARRAY_TYPE *FIO_NAME2(FIO_ARRAY_NAME, ptr)(FIO_ARRAY_PTR ary);

/**
 * Reserves a minimal capacity for additional elements to be added to the array.
 *
 * If `capa` is negative, new memory will be allocated at the beginning of the
 * array rather then it's end.
 *
 * Returns the array's new capacity.
 */
SFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_PTR ary,
                                                 int32_t capa);

/**
 * Adds all the items in the `src` Array to the end of the `dest` Array.
 *
 * The `src` Array remain untouched.
 *
 * Always returns the destination array (`dest`).
 */
SFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, concat)(FIO_ARRAY_PTR dest,
                                                     FIO_ARRAY_PTR src);

/**
 * Sets `index` to the value in `data`.
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 *
 * If `old` isn't NULL, the existing data will be copied to the location pointed
 * to by `old` before the copy in the Array is destroyed.
 *
 * Returns a pointer to the new object, or NULL on error.
 */
SFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_PTR ary,
                                                    int32_t index,
                                                    FIO_ARRAY_TYPE data,
                                                    FIO_ARRAY_TYPE *old);

/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
FIO_IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_PTR ary,
                                                       int32_t index);

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
SFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_PTR ary,
                                             FIO_ARRAY_TYPE data,
                                             int32_t start_at);

/**
 * Removes an object from the array, MOVING all the other objects to prevent
 * "holes" in the data.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns 0 on success and -1 on error.
 *
 * This action is O(n) where n in the length of the array.
 * It could get expensive.
 */
SFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_PTR ary,
                                           int32_t index,
                                           FIO_ARRAY_TYPE *old);

/**
 * Removes all occurrences of an object from the array (if any), MOVING all the
 * existing objects to prevent "holes" in the data.
 *
 * Returns the number of items removed.
 *
 * This action is O(n) where n in the length of the array.
 * It could get expensive.
 */
SFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_PTR ary,
                                                 FIO_ARRAY_TYPE data);

/** Attempts to lower the array's memory consumption. */
SFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_PTR ary);

/**
 * Pushes an object to the end of the Array. Returns a pointer to the new object
 * or NULL on error.
 */
SFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_PTR ary,
                                                     FIO_ARRAY_TYPE data);

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
SFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE *old);

/**
 * Unshifts an object to the beginning of the Array. Returns a pointer to the
 * new object or NULL on error.
 *
 * This could be expensive, causing `memmove`.
 */
SFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_PTR ary,
                                                        FIO_ARRAY_TYPE data);

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
SFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_PTR ary,
                                          FIO_ARRAY_TYPE *old);

/** Iteration information structure passed to the callback. */
typedef struct FIO_NAME(FIO_ARRAY_NAME, each_s) {
  /** The array iterated. Once set, cannot be safely changed. */
  FIO_ARRAY_PTR const parent;
  /** The current object's index */
  uint64_t index;
  /** The callback / task called for each index, may be updated mid-cycle. */
  int (*task)(struct FIO_NAME(FIO_ARRAY_NAME, each_s) * info);
  /** Opaque user data. */
  void *udata;
  /** The object / value at the current index. */
  FIO_ARRAY_TYPE value;
  /* memory padding used for FIOBJ */
  uint64_t padding;
} FIO_NAME(FIO_ARRAY_NAME, each_s);

/**
 * Iteration using a callback for each entry in the array.
 *
 * The callback task function must accept an each_s pointer, see above.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME,
                        each)(FIO_ARRAY_PTR ary,
                              int (*task)(FIO_NAME(FIO_ARRAY_NAME, each_s) *
                                          info),
                              void *udata,
                              int32_t start_at);

#ifndef FIO_ARRAY_EACH
/**
 * Iterates through the array using a `for` loop.
 *
 * Access the object with the pointer `pos`. The `pos` variable can be named
 * however you please.
 *
 * Avoid editing the array during a FOR loop, although I hope it's possible, I
 * wouldn't count on it.
 *
 * **Note**: this variant supports automatic pointer tagging / untagging.
 */
#define FIO_ARRAY_EACH(array_name, array, pos)                                 \
  for (FIO_NAME(array_name, ____type_t)                                        \
           *first___ai = NULL,                                                 \
           *pos = FIO_NAME(array_name, each_next)((array), &first___ai, NULL); \
       pos;                                                                    \
       pos = FIO_NAME(array_name, each_next)((array), &first___ai, pos))
#endif

/**
 * Returns a pointer to the (next) object in the array.
 *
 * Returns a pointer to the first object if `pos == NULL` and there are objects
 * in the array.
 *
 * The first pointer is automatically set and it allows object insertions and
 * memory effecting functions to be called from within the loop.
 *
 * If the object in `pos` (or an object before it) were removed, consider
 * passing `pos-1` to the function, to avoid skipping any elements while
 * looping.
 *
 * Returns the next object if both `first` and `pos` are valid.
 *
 * Returns NULL if `pos` was the last object or no object exist.
 *
 * Returns the first object if either `first` or `pos` are invalid.
 *
 */
FIO_IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME,
                                   each_next)(FIO_ARRAY_PTR ary,
                                              FIO_ARRAY_TYPE **first,
                                              FIO_ARRAY_TYPE *pos);

/* *****************************************************************************
Dynamic Arrays - embedded arrays
***************************************************************************** */
typedef struct {
  /* start common header */
  /** the offset to the first item. */
  uint32_t start;
  /** The offset to the first empty location the array. */
  uint32_t end;
  /* end common header */
  FIO_ARRAY_TYPE embedded[];
} FIO_NAME(FIO_ARRAY_NAME, ___embedded_s);

#define FIO_ARRAY2EMBEDDED(a) ((FIO_NAME(FIO_ARRAY_NAME, ___embedded_s) *)(a))

#if FIO_ARRAY_ENABLE_EMBEDDED
#define FIO_ARRAY_IS_EMBEDDED(a)                                               \
  ((sizeof(FIO_ARRAY_TYPE) +                                                   \
    sizeof(FIO_NAME(FIO_ARRAY_NAME, ___embedded_s))) <=                        \
       sizeof(FIO_NAME(FIO_ARRAY_NAME, s)) &&                                  \
   (((a)->start > (a)->end) || !(a)->ary))
#define FIO_ARRAY_IS_EMBEDDED_PTR(ary, ptr)                                    \
  ((sizeof(FIO_ARRAY_TYPE) +                                                   \
    sizeof(FIO_NAME(FIO_ARRAY_NAME, ___embedded_s))) <=                        \
       sizeof(FIO_NAME(FIO_ARRAY_NAME, s)) &&                                  \
   (uintptr_t)(ptr) > (uintptr_t)(ary) &&                                      \
   (uintptr_t)(ptr) < (uintptr_t)((ary) + 1))
#define FIO_ARRAY_EMBEDDED_CAPA                                                \
  ((sizeof(FIO_ARRAY_TYPE) +                                                   \
    sizeof(FIO_NAME(FIO_ARRAY_NAME, ___embedded_s))) >                         \
           sizeof(FIO_NAME(FIO_ARRAY_NAME, s))                                 \
       ? 0                                                                     \
       : ((sizeof(FIO_NAME(FIO_ARRAY_NAME, s)) -                               \
           sizeof(FIO_NAME(FIO_ARRAY_NAME, ___embedded_s))) /                  \
          sizeof(FIO_ARRAY_TYPE)))

#else
#define FIO_ARRAY_IS_EMBEDDED(a)            0
#define FIO_ARRAY_IS_EMBEDDED_PTR(ary, ptr) 0
#define FIO_ARRAY_EMBEDDED_CAPA             0

#endif /* FIO_ARRAY_ENABLE_EMBEDDED */
/* *****************************************************************************
Inlined functions
***************************************************************************** */
/** Returns the number of elements in the Array. */
FIO_IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0: return ary->end - ary->start;
  case 1: return ary->start;
  }
  return 0;
}

/** Returns the current, temporary, array capacity (it's dynamic). */
FIO_IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0: return (uint32_t)ary->capa;
  case 1: return FIO_ARRAY_EMBEDDED_CAPA;
  }
  return 0;
}

/**
 * Returns a pointer to the C array containing the objects.
 */
FIO_IFUNC FIO_ARRAY_TYPE *FIO_NAME2(FIO_ARRAY_NAME, ptr)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, NULL);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0: return ary->ary + ary->start;
  case 1: return FIO_ARRAY2EMBEDDED(ary)->embedded;
  }
  return NULL;
}

/**
 * Returns 1 if the array is embedded, 0 if it has memory allocated and -1 on an
 * error.
 */
FIO_IFUNC int FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, -1);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  return FIO_ARRAY_IS_EMBEDDED(ary);
  (void)ary; /* if unused (never embedded) */
}

/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
FIO_IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_PTR ary_,
                                                       int32_t index) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, FIO_ARRAY_TYPE_INVALID);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  FIO_ARRAY_TYPE *a;
  size_t count;
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0:
    a = ary->ary + ary->start;
    count = ary->end - ary->start;
    break;
  case 1:
    a = FIO_ARRAY2EMBEDDED(ary)->embedded;
    count = ary->start;
    break;
  default: return FIO_ARRAY_TYPE_INVALID;
  }

  if (index < 0) {
    index += count;
    if (index < 0)
      return FIO_ARRAY_TYPE_INVALID;
  }
  if ((uint32_t)index >= count)
    return FIO_ARRAY_TYPE_INVALID;
  return a[index];
}

/* Returns a pointer to the (next) object in the array. */
FIO_IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME,
                                   each_next)(FIO_ARRAY_PTR ary_,
                                              FIO_ARRAY_TYPE **first,
                                              FIO_ARRAY_TYPE *pos) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, NULL);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  int32_t count;
  FIO_ARRAY_TYPE *a;
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0:
    count = ary->end - ary->start;
    a = ary->ary + ary->start;
    break;
  case 1:
    count = ary->start;
    a = FIO_ARRAY2EMBEDDED(ary)->embedded;
    break;
  default: return NULL;
  }
  intptr_t i;
  if (!count || !first)
    return NULL;
  if (!pos || !(*first) || (*first) > pos) {
    i = -1;
  } else {
    i = (intptr_t)(pos - (*first));
  }
  *first = a;
  ++i;
  if (i >= count)
    return NULL;
  return i + a;
}

/** Used internally for the EACH macro */
typedef FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, ____type_t);

/* *****************************************************************************
Exported functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
/* *****************************************************************************
Helper macros
***************************************************************************** */
#if FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_ADD2CAPA(capa) (((capa) << 1) + FIO_ARRAY_PADDING)
#else
#define FIO_ARRAY_ADD2CAPA(capa) ((capa) + FIO_ARRAY_PADDING)
#endif

/* *****************************************************************************
Dynamic Arrays - internal helpers
***************************************************************************** */

#define FIO_ARRAY_POS2ABS(ary, pos)                                            \
  (pos >= 0 ? (ary->start + pos) : (ary->end - pos))

#define FIO_ARRAY_AB_CT(cond, a, b) ((b) ^ ((0 - ((cond)&1)) & ((a) ^ (b))))

FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_ARRAY_NAME, s))
FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_ARRAY_NAME, destroy))
/* *****************************************************************************
Dynamic Arrays - implementation
***************************************************************************** */

#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new array object on the heap and initializes it's memory. */
SFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, new)(void) {
  FIO_NAME(FIO_ARRAY_NAME, s) *a =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*a), 0);
  if (!FIO_MEM_REALLOC_IS_SAFE_ && a) {
    *a = (FIO_NAME(FIO_ARRAY_NAME, s))FIO_ARRAY_INIT;
  }
  FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_ARRAY_NAME, s));
  return (FIO_ARRAY_PTR)FIO_PTR_TAG(a);
}

/* Frees an array's internal data AND it's container! */
SFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  FIO_NAME(FIO_ARRAY_NAME, destroy)(ary_);
  FIO_MEM_FREE_(ary, sizeof(*ary));
  FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_ARRAY_NAME, s));
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Destroys any objects stored in the array and frees the internal state. */
SFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  union {
    FIO_NAME(FIO_ARRAY_NAME, s) a;
    FIO_NAME(FIO_ARRAY_NAME, ___embedded_s) e;
  } tmp = {.a = *ary};
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s))FIO_ARRAY_INIT;

  switch (
      FIO_NAME_BL(FIO_ARRAY_NAME, embedded)((FIO_ARRAY_PTR)FIO_PTR_TAG(&tmp))) {
  case 0:
#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE
    for (size_t i = tmp.a.start; i < tmp.a.end; ++i) {
      FIO_ARRAY_TYPE_DESTROY(tmp.a.ary[i]);
    }
#endif
    FIO_MEM_FREE_(tmp.a.ary, tmp.a.capa * sizeof(*tmp.a.ary));
    FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_ARRAY_NAME, destroy));
    return;
  case 1:
#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE
    while (tmp.e.start--) {
      FIO_ARRAY_TYPE_DESTROY((tmp.e.embedded[tmp.e.start]));
    }
#endif
    return;
  }
  return;
}

/** Reserves a minimal capacity for the array. */
SFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_PTR ary_,
                                                 int32_t capa_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  uint32_t abs_capa = ((capa_ >= 0) ? (uint32_t)capa_ : (uint32_t)(0 - capa_));
  uint32_t capa;
  FIO_ARRAY_TYPE *tmp;
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0:
    abs_capa += ary->end - ary->start;
    capa = FIO_ARRAY_SIZE2WORDS((abs_capa));
    if (abs_capa <= ary->capa)
      return (uint32_t)ary->capa;
    /* objects don't move, use only realloc */
    if ((capa_ >= 0) || (capa_ < 0 && ary->start > 0)) {
      tmp = (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(ary->ary,
                                               0,
                                               sizeof(*tmp) * capa,
                                               sizeof(*tmp) * ary->end);
      if (!tmp)
        return (uint32_t)ary->capa;
      if (!ary->ary)
        FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_ARRAY_NAME, destroy));
      ary->capa = capa;
      ary->ary = tmp;
      return capa;
    } else { /* moving objects, starting with a fresh piece of memory */
      tmp = (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*tmp) * capa, 0);
      const uint32_t count = ary->end - ary->start;
      if (!tmp)
        return (uint32_t)ary->capa;
      if (!ary->ary)
        FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_ARRAY_NAME, destroy));
      if (capa_ >= 0) { /* copy items at beginning of memory stack */
        if (count) {
          FIO_MEMCPY(tmp, ary->ary + ary->start, count * sizeof(*tmp));
        }
        FIO_MEM_FREE_(ary->ary, sizeof(*ary->ary) * ary->capa);
        *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
            .start = 0,
            .end = count,
            .capa = capa,
            .ary = tmp,
        };
        return capa;
      } else { /* copy items at ending of memory stack */
        if (count) {
          FIO_MEMCPY(tmp + (capa - count),
                     ary->ary + ary->start,
                     count * sizeof(*tmp));
        }
        FIO_MEM_FREE_(ary->ary, sizeof(*ary->ary) * ary->capa);
        *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
            .start = (capa - count),
            .end = capa,
            .capa = capa,
            .ary = tmp,
        };
      }
    }
    return capa;
  case 1:
    abs_capa += ary->start;
    capa = FIO_ARRAY_SIZE2WORDS((abs_capa));
    if (abs_capa <= FIO_ARRAY_EMBEDDED_CAPA)
      return FIO_ARRAY_EMBEDDED_CAPA;
    tmp = (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*tmp) * capa, 0);
    if (!tmp)
      return FIO_ARRAY_EMBEDDED_CAPA;
    FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_ARRAY_NAME, destroy));
    if (capa_ >= 0) {
      /* copy items at beginning of memory stack */
      if (ary->start) {
        FIO_MEMCPY(tmp,
                   FIO_ARRAY2EMBEDDED(ary)->embedded,
                   ary->start * sizeof(*tmp));
      }
      *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
          .start = 0,
          .end = ary->start,
          .capa = capa,
          .ary = tmp,
      };
      return capa;
    }
    /* copy items at ending of memory stack */
    if (ary->start) {
      FIO_MEMCPY(tmp + (capa - ary->start),
                 FIO_ARRAY2EMBEDDED(ary)->embedded,
                 ary->start * sizeof(*tmp));
    }
    *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
        .start = (capa - ary->start),
        .end = capa,
        .capa = capa,
        .ary = tmp,
    };
    return capa;
  default: return 0;
  }
}

/**
 * Adds all the items in the `src` Array to the end of the `dest` Array.
 *
 * The `src` Array remain untouched.
 *
 * Returns `dest` on success or NULL on error (i.e., no memory).
 */
SFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, concat)(FIO_ARRAY_PTR dest_,
                                                     FIO_ARRAY_PTR src_) {
  FIO_PTR_TAG_VALID_OR_RETURN(dest_, (FIO_ARRAY_PTR)NULL);
  FIO_PTR_TAG_VALID_OR_RETURN(src_, (FIO_ARRAY_PTR)NULL);
  FIO_NAME(FIO_ARRAY_NAME, s) *dest =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), dest_);
  FIO_NAME(FIO_ARRAY_NAME, s) *src =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), src_);
  if (!dest || !src)
    return dest_;
  const uint32_t offset = FIO_NAME(FIO_ARRAY_NAME, count)(dest_);
  const uint32_t added = FIO_NAME(FIO_ARRAY_NAME, count)(src_);
  const uint32_t total = offset + added;
  if (!added)
    return dest_;

  if (total < offset || total + offset < total)
    return NULL; /* item count overflow */

  const uint32_t capa = FIO_NAME(FIO_ARRAY_NAME, reserve)(dest_, added);

  if (!FIO_ARRAY_IS_EMBEDDED(dest) && dest->start + total > capa) {
    /* we need to move the existing items due to the offset */
    FIO_MEMMOVE(dest->ary,
                dest->ary + dest->start,
                (dest->end - dest->start) * sizeof(*dest->ary));
    dest->start = 0;
    dest->end = offset;
  }
#if FIO_ARRAY_TYPE_CONCAT_COPY_SIMPLE
  /* copy data */
  FIO_MEMCPY(FIO_NAME2(FIO_ARRAY_NAME, ptr)(dest_) + offset,
             FIO_NAME2(FIO_ARRAY_NAME, ptr)(src_),
             added);
#else
  {
    FIO_ARRAY_TYPE *const a1 = FIO_NAME2(FIO_ARRAY_NAME, ptr)(dest_);
    FIO_ARRAY_TYPE *const a2 = FIO_NAME2(FIO_ARRAY_NAME, ptr)(src_);
    for (uint32_t i = 0; i < added; ++i) {
      FIO_ARRAY_TYPE_CONCAT_COPY(a1[i + offset], a2[i]);
    }
  }
#endif /* FIO_ARRAY_TYPE_CONCAT_COPY_SIMPLE */
  /* update dest */
  if (!FIO_ARRAY_IS_EMBEDDED(dest)) {
    dest->end += added;
    return dest_;
  } else
    dest->start = total;
  return dest_;
}

/**
 * Sets `index` to the value in `data`.
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 *
 * If `old` isn't NULL, the existing data will be copied to the location pointed
 * to by `old` before the copy in the Array is destroyed.
 *
 * Returns a pointer to the new object, or NULL on error.
 */
SFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_PTR ary_,
                                                    int32_t index,
                                                    FIO_ARRAY_TYPE data,
                                                    FIO_ARRAY_TYPE *old) {
  FIO_ARRAY_TYPE *a = NULL;
  FIO_NAME(FIO_ARRAY_NAME, s) * ary;
  uint32_t count;
  uint8_t pre_existing = 1;

  FIO_PTR_TAG_VALID_OR_GOTO(ary_, invalid);

  ary = FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

  if (index < 0) {
    index += count;
    if (index < 0)
      goto negative_expansion;
  }

  if ((uint32_t)index >= count) {
    if ((uint32_t)index == count)
      FIO_NAME(FIO_ARRAY_NAME, reserve)(ary_, FIO_ARRAY_ADD2CAPA(index));
    else
      FIO_NAME(FIO_ARRAY_NAME, reserve)(ary_, (uint32_t)index + 1);
    if (FIO_ARRAY_IS_EMBEDDED(ary))
      goto expand_embedded;
    goto expansion;
  }

  a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);

done:

  /* copy / clear object */
  if (pre_existing) {
    if (old) {
      FIO_ARRAY_TYPE_COPY(old[0], a[index]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
      FIO_ARRAY_TYPE_DESTROY(a[index]);
#endif
    } else {
      FIO_ARRAY_TYPE_DESTROY(a[index]);
    }
  } else if (old) {
    FIO_ARRAY_TYPE_COPY(old[0], FIO_ARRAY_TYPE_INVALID);
  }
  FIO_ARRAY_TYPE_COPY(a[index], FIO_ARRAY_TYPE_INVALID);
  FIO_ARRAY_TYPE_COPY(a[index], data);
  return a + index;

expansion:

  pre_existing = 0;
  a = ary->ary;
  {
    uint8_t was_moved = 0;
    /* test if we need to move objects to make room at the end */
    if (ary->start + index >= ary->capa) {
      FIO_MEMMOVE(ary->ary, ary->ary + ary->start, (count) * sizeof(*ary->ary));
      ary->start = 0;
      ary->end = index + 1;
      was_moved = 1;
    }
    /* initialize memory in between objects */
    if (was_moved || !FIO_MEM_REALLOC_IS_SAFE_ ||
        !FIO_ARRAY_TYPE_INVALID_SIMPLE) {
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
      FIO_MEMSET(a + count, 0, (index - count) * sizeof(*ary->ary));
#else
      for (size_t i = count; i <= (size_t)index; ++i) {
        FIO_ARRAY_TYPE_COPY(a[i], FIO_ARRAY_TYPE_INVALID);
      }
#endif
    }
    ary->end = index + 1;
  }
  goto done;

expand_embedded:
  pre_existing = 0;
  ary->start = index + 1;
  a = FIO_ARRAY2EMBEDDED(ary)->embedded;
  goto done;

negative_expansion:
  pre_existing = 0;
  FIO_NAME(FIO_ARRAY_NAME, reserve)(ary_, (index - count));
  index = 0 - index;

  if ((FIO_ARRAY_IS_EMBEDDED(ary)))
    goto negative_expansion_embedded;
  a = ary->ary;
  if (index > (int32_t)ary->start) {
    FIO_MEMMOVE(a + index, a + ary->start, count * sizeof(*a));
    ary->end = index + count;
    ary->start = index;
  }
  index = ary->start - index;
  if ((uint32_t)(index + 1) < ary->start) {
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
    FIO_MEMSET(a + index, 0, (ary->start - index) * (sizeof(*a)));
#else
    for (size_t i = index; i < (size_t)ary->start; ++i) {
      FIO_ARRAY_TYPE_COPY(a[i], FIO_ARRAY_TYPE_INVALID);
    }
#endif
  }
  ary->start = index;
  goto done;

negative_expansion_embedded:
  a = FIO_ARRAY2EMBEDDED(ary)->embedded;
  FIO_MEMMOVE(a + index, a, count * count * sizeof(*a));
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
  FIO_MEMSET(a, 0, index * (sizeof(a)));
#else
  for (size_t i = 0; i < (size_t)index; ++i) {
    FIO_ARRAY_TYPE_COPY(a[i], FIO_ARRAY_TYPE_INVALID);
  }
#endif
  index = 0;
  goto done;

invalid:
  FIO_ARRAY_TYPE_DESTROY(data);
  if (old) {
    FIO_ARRAY_TYPE_COPY(old[0], FIO_ARRAY_TYPE_INVALID);
  }

  return a;
}

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
SFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_PTR ary_,
                                             FIO_ARRAY_TYPE data,
                                             int32_t start_at) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  if (!a)
    return -1;
  size_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  if (start_at >= 0) {
    /* seek forwards */
    if ((uint32_t)start_at >= count)
      start_at = (int32_t)count;
    while ((uint32_t)start_at < count) {
      if (FIO_ARRAY_TYPE_CMP(a[start_at], data))
        return start_at;
      ++start_at;
    }
  } else {
    /* seek backwards */
    if (start_at + (int32_t)count < 0)
      return -1;
    count += start_at;
    count += 1;
    while (count--) {
      if (FIO_ARRAY_TYPE_CMP(a[count], data))
        return count;
    }
  }
  return -1;
}

/**
 * Removes an object from the array, MOVING all the other objects to prevent
 * "holes" in the data.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns 0 on success and -1 on error.
 */
SFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_PTR ary_,
                                           int32_t index,
                                           FIO_ARRAY_TYPE *old) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  size_t count;
  if (!a)
    goto invalid;
  count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

  if (index < 0) {
    index += count;
    if (index < 0) {
      FIO_LOG_WARNING(
          FIO_MACRO2STR(FIO_NAME(FIO_ARRAY_NAME,
                                 remove)) " called with a negative index lower "
                                          "than the element count.");
      goto invalid;
    }
  }
  if ((uint32_t)index >= count)
    goto invalid;
  if (!index) {
    FIO_NAME(FIO_ARRAY_NAME, shift)(ary_, old);
    return 0;
  }
  if ((uint32_t)index + 1 == count) {
    FIO_NAME(FIO_ARRAY_NAME, pop)(ary_, old);
    return 0;
  }

  if (old) {
    FIO_ARRAY_TYPE_COPY(*old, a[index]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
    FIO_ARRAY_TYPE_DESTROY(a[index]);
#endif
  } else {
    FIO_ARRAY_TYPE_DESTROY(a[index]);
  }

  if ((uint32_t)(index + 1) < count) {
    FIO_MEMMOVE(a + index, a + index + 1, (count - (index + 1)) * sizeof(*a));
  }
  FIO_ARRAY_TYPE_COPY((a + (count - 1))[0], FIO_ARRAY_TYPE_INVALID);

  if (FIO_ARRAY_IS_EMBEDDED(ary))
    goto embedded;
  --ary->end;
  return 0;

embedded:
  --ary->start;
  return 0;

invalid:
  if (old) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
  }
  return -1;
}

/**
 * Removes all occurrences of an object from the array (if any), MOVING all the
 * existing objects to prevent "holes" in the data.
 *
 * Returns the number of items removed.
 */
SFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_PTR ary_,
                                                 FIO_ARRAY_TYPE data) {
  size_t c = 0;
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  size_t count;
  if (!a)
    return (uint32_t)c;
  count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

  size_t i = 0;
  while ((i + c) < count) {
    if (!(FIO_ARRAY_TYPE_CMP(a[i + c], data))) {
      a[i] = a[i + c];
      ++i;
      continue;
    }
    FIO_ARRAY_TYPE_DESTROY(a[i + c]);
    ++c;
  }
  if (c && FIO_MEM_REALLOC_IS_SAFE_) {
    /* keep memory zeroed out */
    FIO_MEMSET(a + i, 0, sizeof(*a) * c);
  }
  if (!FIO_ARRAY_IS_EMBEDDED_PTR(ary, a)) {
    ary->end = (uint32_t)(ary->start + i);
    return (uint32_t)c;
  }
  ary->start = (uint32_t)i;
  return (uint32_t)c;
}

/** Attempts to lower the array's memory consumption. */
SFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  size_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  FIO_ARRAY_TYPE *tmp = NULL;

  if (count <= FIO_ARRAY_EMBEDDED_CAPA)
    goto re_embed;

  tmp = (FIO_ARRAY_TYPE *)
      FIO_MEM_REALLOC_(NULL, 0, (ary->end - ary->start) * sizeof(*tmp), 0);
  if (!tmp)
    return;
  FIO_MEMCPY(tmp, ary->ary + ary->start, count * sizeof(*ary->ary));
  FIO_MEM_FREE_(ary->ary, ary->capa * sizeof(*ary->ary));
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
      .start = 0,
      .end = (ary->end - ary->start),
      .capa = (ary->end - ary->start),
      .ary = tmp,
  };
  return;

re_embed:
  if (!FIO_ARRAY_IS_EMBEDDED(ary)) {
    tmp = ary->ary;
    uint32_t offset = ary->start;
    size_t old_capa = ary->capa;
    *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
        .start = (uint32_t)count,
    };
    if (count) {
      FIO_MEMCPY(FIO_ARRAY2EMBEDDED(ary)->embedded,
                 tmp + offset,
                 count * sizeof(*tmp));
    }
    if (tmp) {
      FIO_MEM_FREE_(tmp, sizeof(*tmp) * old_capa);
      FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_ARRAY_NAME, destroy));
      (void)old_capa; /* if unused */
    }
  }
  return;
}
/**
 * Pushes an object to the end of the Array. Returns NULL on error.
 */
SFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_PTR ary_,
                                                     FIO_ARRAY_TYPE data) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0:
    if (ary->end == ary->capa) {
      if (!ary->start) {
        if (FIO_NAME(FIO_ARRAY_NAME,
                     reserve)(ary_, (uint32_t)FIO_ARRAY_ADD2CAPA(ary->capa)) ==
            ary->end)
          goto invalid;
      } else {
        const uint32_t new_start = (ary->start >> 2);
        const uint32_t count = ary->end - ary->start;
        if (count)
          FIO_MEMMOVE(ary->ary + new_start,
                      ary->ary + ary->start,
                      count * sizeof(*ary->ary));
        ary->end = count + new_start;
        ary->start = new_start;
      }
    }
    FIO_ARRAY_TYPE_COPY(ary->ary[ary->end], data);
    return ary->ary + (ary->end++);

  case 1:
    if (ary->start == FIO_ARRAY_EMBEDDED_CAPA)
      goto needs_memory_embedded;
    FIO_ARRAY_TYPE_COPY(FIO_ARRAY2EMBEDDED(ary)->embedded[ary->start], data);
    return FIO_ARRAY2EMBEDDED(ary)->embedded + (ary->start++);
  }
invalid:
  FIO_ARRAY_TYPE_DESTROY(data);
  return NULL;

needs_memory_embedded:
  if (FIO_NAME(FIO_ARRAY_NAME,
               reserve)(ary_, FIO_ARRAY_ADD2CAPA(FIO_ARRAY_EMBEDDED_CAPA)) ==
      FIO_ARRAY_EMBEDDED_CAPA)
    goto invalid;
  FIO_ARRAY_TYPE_COPY(ary->ary[ary->end], data);
  return ary->ary + (ary->end++);
}

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
SFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_PTR ary_,
                                        FIO_ARRAY_TYPE *old) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0:
    if (ary->end == ary->start)
      return -1;
    --ary->end;
    if (old) {
      FIO_ARRAY_TYPE_COPY(*old, ary->ary[ary->end]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
      FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->end]);
#endif
    } else {
      FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->end]);
    }
    return 0;
  case 1:
    if (!ary->start)
      return -1;
    --ary->start;
    if (old) {
      FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY2EMBEDDED(ary)->embedded[ary->start]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
      FIO_ARRAY_TYPE_DESTROY(FIO_ARRAY2EMBEDDED(ary)->embedded[ary->start]);
#endif
    } else {
      FIO_ARRAY_TYPE_DESTROY(FIO_ARRAY2EMBEDDED(ary)->embedded[ary->start]);
    }
    FIO_MEMSET(FIO_ARRAY2EMBEDDED(ary)->embedded + ary->start,
               0,
               sizeof(*ary->ary));
    return 0;
  }
  if (old)
    FIO_ARRAY_TYPE_COPY(old[0], FIO_ARRAY_TYPE_INVALID);
  return -1;
}

/**
 * Unshifts an object to the beginning of the Array. Returns -1 on error.
 *
 * This could be expensive, causing `memmove`.
 */
SFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_PTR ary_,
                                                        FIO_ARRAY_TYPE data) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);
  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0:
    if (!ary->start) {
      if (ary->end == ary->capa) {
        FIO_NAME(FIO_ARRAY_NAME, reserve)
        (ary_, (-1 - (int32_t)FIO_ARRAY_ADD2CAPA(ary->capa)));
        if (!ary->start)
          goto invalid;
      } else {
        const uint32_t new_end =
            (uint32_t)(ary->capa - ((ary->capa - ary->end) >> 2));
        const uint32_t count = (uint32_t)(ary->end - ary->start);
        const uint32_t new_start = (uint32_t)(new_end - count);
        if (count)
          FIO_MEMMOVE(ary->ary + new_start,
                      ary->ary + ary->start,
                      count * sizeof(*ary->ary));
        ary->end = new_end;
        ary->start = new_start;
      }
    }
    FIO_ARRAY_TYPE_COPY(ary->ary[--ary->start], data);
    return ary->ary + ary->start;

  case 1:
    if (ary->start == FIO_ARRAY_EMBEDDED_CAPA)
      goto needs_memory_embed;
    if (ary->start)
      FIO_MEMMOVE(FIO_ARRAY2EMBEDDED(ary)->embedded + 1,
                  FIO_ARRAY2EMBEDDED(ary)->embedded,
                  sizeof(*ary->ary) * ary->start);
    ++ary->start;
    FIO_ARRAY_TYPE_COPY(FIO_ARRAY2EMBEDDED(ary)->embedded[0], data);
    return FIO_ARRAY2EMBEDDED(ary)->embedded;
  }
invalid:
  FIO_ARRAY_TYPE_DESTROY(data);
  return NULL;

needs_memory_embed:
  if (FIO_NAME(FIO_ARRAY_NAME, reserve)(
          ary_,
          (-1 - (int32_t)FIO_ARRAY_ADD2CAPA(FIO_ARRAY_EMBEDDED_CAPA))) ==
      FIO_ARRAY_EMBEDDED_CAPA)
    goto invalid;
  FIO_ARRAY_TYPE_COPY(ary->ary[--ary->start], data);
  return ary->ary + ary->start;
}

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
SFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_PTR ary_,
                                          FIO_ARRAY_TYPE *old) {

  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_ARRAY_NAME, s), ary_);

  switch (FIO_NAME_BL(FIO_ARRAY_NAME, embedded)(ary_)) {
  case 0:
    if (ary->end == ary->start)
      return -1;
    if (old) {
      FIO_ARRAY_TYPE_COPY(*old, ary->ary[ary->start]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
      FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->start]);
#endif
    } else {
      FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->start]);
    }
    ++ary->start;
    return 0;
  case 1:
    if (!ary->start)
      return -1;
    if (old) {
      FIO_ARRAY_TYPE_COPY(old[0], FIO_ARRAY2EMBEDDED(ary)->embedded[0]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
      FIO_ARRAY_TYPE_DESTROY(FIO_ARRAY2EMBEDDED(ary)->embedded[0]);
#endif
    } else {
      FIO_ARRAY_TYPE_DESTROY(FIO_ARRAY2EMBEDDED(ary)->embedded[0]);
    }
    --ary->start;
    if (ary->start)
      FIO_MEMMOVE(FIO_ARRAY2EMBEDDED(ary)->embedded,
                  FIO_ARRAY2EMBEDDED(ary)->embedded +
                      FIO_ARRAY2EMBEDDED(ary)->start,
                  FIO_ARRAY2EMBEDDED(ary)->start *
                      sizeof(*FIO_ARRAY2EMBEDDED(ary)->embedded));
    FIO_MEMSET(FIO_ARRAY2EMBEDDED(ary)->embedded + ary->start,
               0,
               sizeof(*ary->ary));
    return 0;
  }
  if (old)
    FIO_ARRAY_TYPE_COPY(old[0], FIO_ARRAY_TYPE_INVALID);
  return -1;
}

/**
 * Iteration using a callback for each entry in the array.
 *
 * The callback task function must accept an the entry data as well as an opaque
 * user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME,
                        each)(FIO_ARRAY_PTR ary_,
                              int (*task)(FIO_NAME(FIO_ARRAY_NAME, each_s) *
                                          info),
                              void *udata,
                              int32_t start_at) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  if (!a)
    return (uint32_t)-1;

  uint32_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

  if (start_at < 0) {
    start_at = count - start_at;
    if (start_at < 0)
      start_at = 0;
  }

  if (!a || !task)
    return (uint32_t)-1;

  if ((uint32_t)start_at >= count)
    return (uint32_t)count;

  FIO_NAME(FIO_ARRAY_NAME, each_s)
  e = {
      .parent = ary_,
      .index = (uint64_t)start_at,
      .task = task,
      .udata = udata,
  };

  while ((uint32_t)e.index < FIO_NAME(FIO_ARRAY_NAME, count)(ary_)) {
    a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
    e.value = a[e.index];
    int r = e.task(&e);
    ++e.index;
    if (r == -1) {
      return (uint32_t)(e.index);
    }
  }
  return (uint32_t)e.index;
}

/* *****************************************************************************
Dynamic Arrays - test
***************************************************************************** */
#ifdef FIO_TEST_ALL

/* make suer the functions are defined for the testing */
#ifdef FIO_REF_CONSTRUCTOR_ONLY
IFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, new)(void);
IFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary);
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

#define FIO_ARRAY_TEST_OBJ_SET(dest, val)                                      \
  FIO_MEMSET(&(dest), (int)(val), sizeof(FIO_ARRAY_TYPE))
#define FIO_ARRAY_TEST_OBJ_IS(val)                                             \
  (!FIO_MEMCMP(&o,                                                             \
               FIO_MEMSET(&v, (int)(val), sizeof(v)),                          \
               sizeof(FIO_ARRAY_TYPE)))

FIO_SFUNC int FIO_NAME_TEST(stl, FIO_NAME(FIO_ARRAY_NAME, test_task))(
    FIO_NAME(FIO_ARRAY_NAME, each_s) * i) {
  struct data_s {
    int i;
    int va[];
  } *d = (struct data_s *)i->udata;
  FIO_ARRAY_TYPE v;

  FIO_ARRAY_TEST_OBJ_SET(v, d->va[d->i]);
  ++d->i;
  if (d->va[d->i + 1])
    return 0;
  return -1;
}

FIO_SFUNC void FIO_NAME_TEST(stl, FIO_ARRAY_NAME)(void) {
  FIO_ARRAY_TYPE o;
  FIO_ARRAY_TYPE v;
  FIO_NAME(FIO_ARRAY_NAME, s) a_on_stack = FIO_ARRAY_INIT;
  FIO_ARRAY_PTR a_array[2];
  a_array[0] = (FIO_ARRAY_PTR)FIO_PTR_TAG((&a_on_stack));
  a_array[1] = FIO_NAME(FIO_ARRAY_NAME, new)();
  FIO_ASSERT_ALLOC(a_array[1]);
  /* perform test twice, once for an array on the stack and once for allocate */
  for (int selector = 0; selector < 2; ++selector) {
    FIO_ARRAY_PTR a = a_array[selector];
    fprintf(stderr,
            "* Testing dynamic arrays on the %s (" FIO_MACRO2STR(
                FIO_NAME(FIO_ARRAY_NAME,
                         s)) ").\n"
                             "  This type supports %zu embedded items\n",
            (selector ? "heap" : "stack"),
            FIO_ARRAY_EMBEDDED_CAPA);
    /* Test start here */

    /* test push */
    for (int i = 0; i < (int)(FIO_ARRAY_EMBEDDED_CAPA) + 3; ++i) {
      FIO_ARRAY_TEST_OBJ_SET(o, (i + 1));
      o = *FIO_NAME(FIO_ARRAY_NAME, push)(a, o);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1), "push failed (%d)", i);
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, i);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1), "push-get cycle failed (%d)", i);
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, -1);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1),
                 "get with -1 returned wrong result (%d)",
                 i);
    }
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   FIO_ARRAY_EMBEDDED_CAPA + 3,
               "push didn't update count correctly (%d != %d)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a),
               (int)(FIO_ARRAY_EMBEDDED_CAPA) + 3);

    /* test pop */
    for (int i = (int)(FIO_ARRAY_EMBEDDED_CAPA) + 3; i--;) {
      FIO_NAME(FIO_ARRAY_NAME, pop)(a, &o);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS((i + 1)),
                 "pop value error failed (%d)",
                 i);
    }
    FIO_ASSERT(!FIO_NAME(FIO_ARRAY_NAME, count)(a),
               "pop didn't pop all elements?");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, pop)(a, &o),
               "pop for empty array should return an error.");

    /* test compact with zero elements */
    FIO_NAME(FIO_ARRAY_NAME, compact)(a);
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) == FIO_ARRAY_EMBEDDED_CAPA,
               "compact zero elementes didn't make array embedded?");

    /* test unshift */
    for (int i = (int)(FIO_ARRAY_EMBEDDED_CAPA) + 3; i--;) {
      FIO_ARRAY_TEST_OBJ_SET(o, (i + 1));
      o = *FIO_NAME(FIO_ARRAY_NAME, unshift)(a, o);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1), "shift failed (%d)", i);
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, 0);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1),
                 "unshift-get cycle failed (%d)",
                 i);
      int32_t negative_index = 0 - (((int)(FIO_ARRAY_EMBEDDED_CAPA) + 3) - i);
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, negative_index);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1),
                 "get with %d returned wrong result.",
                 negative_index);
    }
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   FIO_ARRAY_EMBEDDED_CAPA + 3,
               "unshift didn't update count correctly (%d != %d)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a),
               (int)(FIO_ARRAY_EMBEDDED_CAPA) + 3);

    /* test shift */
    for (int i = 0; i < (int)(FIO_ARRAY_EMBEDDED_CAPA) + 3; ++i) {
      FIO_NAME(FIO_ARRAY_NAME, shift)(a, &o);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS((i + 1)),
                 "shift value error failed (%d)",
                 i);
    }
    FIO_ASSERT(!FIO_NAME(FIO_ARRAY_NAME, count)(a),
               "shift didn't shift all elements?");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, shift)(a, &o),
               "shift for empty array should return an error.");

    /* test set from embedded? array */
    FIO_NAME(FIO_ARRAY_NAME, compact)(a);
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) == FIO_ARRAY_EMBEDDED_CAPA,
               "compact zero elementes didn't make array embedded (2)?");
    FIO_ARRAY_TEST_OBJ_SET(o, 1);
    FIO_NAME(FIO_ARRAY_NAME, push)(a, o);
    if (FIO_ARRAY_EMBEDDED_CAPA) {
      FIO_ARRAY_TEST_OBJ_SET(o, 1);
      FIO_NAME(FIO_ARRAY_NAME, set)(a, FIO_ARRAY_EMBEDDED_CAPA, o, &o);
      FIO_ASSERT(FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID),
                 "set overflow from embedded array should reset `old`");
      FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                     FIO_ARRAY_EMBEDDED_CAPA + 1,
                 "set didn't update count correctly from embedded "
                 "array (%d != %d)",
                 FIO_NAME(FIO_ARRAY_NAME, count)(a),
                 (int)FIO_ARRAY_EMBEDDED_CAPA);
    }

    /* test set from bigger array */
    FIO_ARRAY_TEST_OBJ_SET(o, 1);
    FIO_NAME(FIO_ARRAY_NAME, set)
    (a, ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4), o, &o);
    FIO_ASSERT(FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID),
               "set overflow should reset `old`");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4) + 1,
               "set didn't update count correctly (%d != %d)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a),
               (int)((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4));
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) >=
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4),
               "set capa should be above item count");
    if (FIO_ARRAY_EMBEDDED_CAPA) {
      FIO_ARRAY_TYPE_COPY(o, FIO_ARRAY_TYPE_INVALID);
      FIO_NAME(FIO_ARRAY_NAME, set)(a, FIO_ARRAY_EMBEDDED_CAPA, o, &o);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(1),
                 "set overflow lost last item while growing.");
    }
    o = FIO_NAME(FIO_ARRAY_NAME, get)(a, (FIO_ARRAY_EMBEDDED_CAPA + 1) * 2);
    FIO_ASSERT(FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID),
               "set overflow should have memory in the middle set to invalid "
               "objetcs.");
    FIO_ARRAY_TEST_OBJ_SET(o, 2);
    FIO_NAME(FIO_ARRAY_NAME, set)(a, 0, o, &o);
    FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(1),
               "set should set `old` to previous value");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4) + 1,
               "set item count error");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) >=
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4) + 1,
               "set capa should be above item count");

    /* test find TODO: test with uninitialized array */
    FIO_ARRAY_TEST_OBJ_SET(o, 99);
    if (FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID)) {
      FIO_ARRAY_TEST_OBJ_SET(o, 100);
    }
    int found = FIO_NAME(FIO_ARRAY_NAME, find)(a, o, 0);
    FIO_ASSERT(found == -1,
               "seeking for an object that doesn't exist should fail.");
    FIO_ARRAY_TEST_OBJ_SET(o, 1);
    found = FIO_NAME(FIO_ARRAY_NAME, find)(a, o, 1);
    FIO_ASSERT(found == ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4),
               "seeking for an object returned the wrong index.");
    FIO_ASSERT(found == FIO_NAME(FIO_ARRAY_NAME, find)(a, o, -1),
               "seeking for an object in reverse returned the wrong index.");
    FIO_ARRAY_TEST_OBJ_SET(o, 2);
    FIO_ASSERT(
        !FIO_NAME(FIO_ARRAY_NAME, find)(a, o, -2),
        "seeking for an object in reverse (2) returned the wrong index.");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4) + 1,
               "find should have side-effects - count error");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) >=
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4) + 1,
               "find should have side-effects - capa error");

    /* test remove */
    FIO_NAME(FIO_ARRAY_NAME, remove)(a, found, &o);
    FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(1), "remove didn't copy old data?");
    o = FIO_NAME(FIO_ARRAY_NAME, get)(a, 0);
    FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(2), "remove removed more?");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4),
               "remove with didn't update count correctly (%d != %s)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a),
               (int)((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4));
    o = FIO_NAME(FIO_ARRAY_NAME, get)(a, -1);

    /* test remove2 */
    FIO_ARRAY_TYPE_COPY(o, FIO_ARRAY_TYPE_INVALID);
    FIO_ASSERT((found = FIO_NAME(FIO_ARRAY_NAME, remove2)(a, o)) ==
                   ((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4) - 1,
               "remove2 result error, %d != %d items.",
               found,
               (int)((FIO_ARRAY_EMBEDDED_CAPA + 1) * 4) - 1);
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) == 1,
               "remove2 didn't update count correctly (%d != 1)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a));

    /* hopefuly these will end... or crash on error. */
    while (!FIO_NAME(FIO_ARRAY_NAME, pop)(a, NULL)) {
      ;
    }
    while (!FIO_NAME(FIO_ARRAY_NAME, shift)(a, NULL)) {
      ;
    }

    /* test push / unshift alternate */
    FIO_NAME(FIO_ARRAY_NAME, destroy)(a);
    for (int i = 0; i < 4096; ++i) {
      FIO_ARRAY_TEST_OBJ_SET(o, (i + 1));
      FIO_NAME(FIO_ARRAY_NAME, push)(a, o);
      FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) + 1 ==
                     ((uint32_t)(i + 1) << 1),
                 "push-unshift[%d.5] cycle count arror (%d != %d)",
                 i,
                 FIO_NAME(FIO_ARRAY_NAME, count)(a),
                 (((uint32_t)(i + 1) << 1)) - 1);
      FIO_ARRAY_TEST_OBJ_SET(o, (i + 4097));
      FIO_NAME(FIO_ARRAY_NAME, unshift)(a, o);
      FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) == ((uint32_t)(i + 1) << 1),
                 "push-unshift[%d] cycle count arror (%d != %d)",
                 i,
                 FIO_NAME(FIO_ARRAY_NAME, count)(a),
                 ((uint32_t)(i + 1) << 1));
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, 0);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 4097),
                 "unshift-push cycle failed (%d)",
                 i);
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, -1);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1),
                 "push-shift cycle failed (%d)",
                 i);
    }
    for (int i = 0; i < 4096; ++i) {
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, i);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS((4096 * 2) - i),
                 "item value error at index %d",
                 i);
    }
    for (int i = 0; i < 4096; ++i) {
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, i + 4096);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS((1 + i)),
                 "item value error at index %d",
                 i + 4096);
    }
#if DEBUG
    for (int i = 0; i < 2; ++i) {
      FIO_LOG_DEBUG2(
          "\t- " FIO_MACRO2STR(
              FIO_NAME(FIO_ARRAY_NAME, s)) " after push/unshit cycle%s:\n"
                                           "\t\t- item count: %d items\n"
                                           "\t\t- capacity:   %d items\n"
                                           "\t\t- memory:     %d bytes\n",
          (i ? " after compact" : ""),
          FIO_NAME(FIO_ARRAY_NAME, count)(a),
          FIO_NAME(FIO_ARRAY_NAME, capa)(a),
          FIO_NAME(FIO_ARRAY_NAME, capa)(a) * sizeof(FIO_ARRAY_TYPE));
      FIO_NAME(FIO_ARRAY_NAME, compact)(a);
    }
#endif /* DEBUG */

    FIO_ARRAY_TYPE_COPY(o, FIO_ARRAY_TYPE_INVALID);
/* test set with NULL, hopefully a bug will cause a crash */
#if FIO_ARRAY_TYPE_DESTROY_SIMPLE
    for (int i = 0; i < 4096; ++i) {
      FIO_NAME(FIO_ARRAY_NAME, set)(a, i, o, NULL);
    }
#else
    /*
     * we need to clear the memory to make sure a cleanup actions don't get
     * unexpected values.
     */
    for (int i = 0; i < (4096 * 2); ++i) {
      FIO_ARRAY_TYPE_COPY((FIO_NAME2(FIO_ARRAY_NAME, ptr)(a)[i]),
                          FIO_ARRAY_TYPE_INVALID);
    }

#endif

    /* TODO: test concat */

    /* test each */
    {
      struct data_s {
        int i;
        int va[10];
      } d = {1, {1, 8, 2, 7, 3, 6, 4, 5}};
      FIO_NAME(FIO_ARRAY_NAME, destroy)(a);
      for (int i = 0; d.va[i]; ++i) {
        FIO_ARRAY_TEST_OBJ_SET(o, d.va[i]);
        FIO_NAME(FIO_ARRAY_NAME, push)(a, o);
      }

      int index = FIO_NAME(FIO_ARRAY_NAME, each)(
          a,
          FIO_NAME_TEST(stl, FIO_NAME(FIO_ARRAY_NAME, test_task)),
          (void *)&d,
          d.i);
      FIO_ASSERT(index == d.i,
                 "index rerturned from each should match next object");
      FIO_ASSERT(*(char *)&d.va[d.i],
                 "array each error (didn't stop in time?).");
      FIO_ASSERT(!(*(char *)&d.va[d.i + 1]),
                 "array each error (didn't stop in time?).");
    }
#if FIO_ARRAY_TYPE_DESTROY_SIMPLE
    {
      FIO_NAME(FIO_ARRAY_NAME, destroy)(a);
      size_t max_items = 63;
      FIO_ARRAY_TYPE tmp[64];
      for (size_t i = 0; i < max_items; ++i) {
        FIO_MEMSET(tmp + i, i + 1, sizeof(*tmp));
      }
      for (size_t items = 0; items <= max_items; items = ((items << 1) | 1)) {
        FIO_LOG_DEBUG2("* testing the FIO_ARRAY_EACH macro with %zu items.",
                       items);
        size_t i = 0;
        for (i = 0; i < items; ++i)
          FIO_NAME(FIO_ARRAY_NAME, push)(a, tmp[i]);
        i = 0;
        FIO_ARRAY_EACH(FIO_ARRAY_NAME, a, pos) {
          FIO_ASSERT(!memcmp(tmp + i, pos, sizeof(*pos)),
                     "FIO_ARRAY_EACH pos is at wrong index %zu != %zu",
                     (size_t)(pos - FIO_NAME2(FIO_ARRAY_NAME, ptr)(a)),
                     i);
          ++i;
        }
        FIO_ASSERT(i == items,
                   "FIO_ARRAY_EACH macro count error - didn't review all "
                   "items? %zu != %zu ",
                   i,
                   items);
        FIO_NAME(FIO_ARRAY_NAME, destroy)(a);
      }
    }
#endif
    /* test destroy */
    FIO_NAME(FIO_ARRAY_NAME, destroy)(a);
    FIO_ASSERT(!FIO_NAME(FIO_ARRAY_NAME, count)(a),
               "destroy didn't clear count.");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) == FIO_ARRAY_EMBEDDED_CAPA,
               "destroy capa error.");
    /* Test end here */
  }
  FIO_NAME(FIO_ARRAY_NAME, free)(a_array[1]);
}
#undef FIO_ARRAY_TEST_OBJ_SET
#undef FIO_ARRAY_TEST_OBJ_IS

#endif /* FIO_TEST_ALL */
/* *****************************************************************************
Dynamic Arrays - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ARRAY_NAME */

#undef FIO_ARRAY_NAME
#undef FIO_ARRAY_TYPE
#undef FIO_ARRAY_ENABLE_EMBEDDED
#undef FIO_ARRAY_TYPE_INVALID
#undef FIO_ARRAY_TYPE_INVALID_SIMPLE
#undef FIO_ARRAY_TYPE_COPY
#undef FIO_ARRAY_TYPE_COPY_SIMPLE
#undef FIO_ARRAY_TYPE_CONCAT_COPY
#undef FIO_ARRAY_TYPE_CONCAT_COPY_SIMPLE
#undef FIO_ARRAY_TYPE_DESTROY
#undef FIO_ARRAY_TYPE_DESTROY_SIMPLE
#undef FIO_ARRAY_DESTROY_AFTER_COPY
#undef FIO_ARRAY_TYPE_CMP
#undef FIO_ARRAY_TYPE_CMP_SIMPLE
#undef FIO_ARRAY_PADDING
#undef FIO_ARRAY_SIZE2WORDS
#undef FIO_ARRAY_POS2ABS
#undef FIO_ARRAY_AB_CT
#undef FIO_ARRAY_PTR
#undef FIO_ARRAY_EXPONENTIAL
#undef FIO_ARRAY_ADD2CAPA
#undef FIO_ARRAY_IS_EMBEDDED
#undef FIO_ARRAY_IS_EMBEDDED_PTR
#undef FIO_ARRAY_EMBEDDED_CAPA
#undef FIO_ARRAY2EMBEDDED
