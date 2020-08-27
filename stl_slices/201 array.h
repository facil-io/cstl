/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_ARRAY_NAME ary          /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#define FIO_TEST_CSTL               /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                            Dynamic Arrays








Example:

```c
typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME ary
#define FIO_ARRAY_TYPE foo_s
#define FIO_ARRAY_TYPE_CMP(a,b) (a.i == b.i && a.f == b.f)
#include "fio_cstl.h"

void example(void) {
  ary_s a = FIO_ARRAY_INIT;
  foo_s *p = ary_push(&a, (foo_s){.i = 42});
  FIO_ARRAY_EACH(&a, pos) { // pos will be a pointer to the element
    fprintf(stderr, "* [%zu]: %p : %d\n", (size_t)(pos - ary2ptr(&a)), pos->i);
  }
  ary_destroy(&a);
}
```

***************************************************************************** */

#ifdef FIO_ARRAY_NAME

#ifndef FIO_ARRAY_TYPE
/** The type for array elements (an array of FIO_ARRAY_TYPE) */
#define FIO_ARRAY_TYPE void *
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID NULL
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#else
#ifndef FIO_ARRAY_TYPE_INVALID
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID ((FIO_ARRAY_TYPE){0})
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#endif
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
#define FIO_ARRAY_TYPE_CONCAT_COPY FIO_ARRAY_TYPE_COPY
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

/* Sets memory growth to exponentially increase. Consumes more memory. */
#ifndef FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_EXPONENTIAL 0
#endif

#undef FIO_ARRAY_SIZE2WORDS
#define FIO_ARRAY_SIZE2WORDS(size)                                             \
  ((sizeof(FIO_ARRAY_TYPE) & 1)                                                \
       ? (((size) & (~15)) + 16)                                               \
       : (sizeof(FIO_ARRAY_TYPE) & 2)                                          \
             ? (((size) & (~7)) + 8)                                           \
             : (sizeof(FIO_ARRAY_TYPE) & 4)                                    \
                   ? (((size) & (~3)) + 4)                                     \
                   : (sizeof(FIO_ARRAY_TYPE) & 8) ? (((size) & (~1)) + 2)      \
                                                  : (size))

/* *****************************************************************************
Dynamic Arrays - type
***************************************************************************** */

typedef struct {
  /* start common header */
  /** the offser to the first item. */
  uint32_t start;
  /** The offset to the first empty location the array. */
  uint32_t end;
  /* end common header */
  /** The attay's capacity only 32bits are valid */
  uintptr_t capa;
  /** a pointer to the array's memory (if not embedded) */
  FIO_ARRAY_TYPE *ary;
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

/* Allocates a new array object on the heap and initializes it's memory. */
IFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, new)(void);

#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Frees an array's internal data AND it's container! */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary);
#else
IFUNC int FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary);
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Destroys any objects stored in the array and frees the internal state. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_PTR ary);

/** Returns the number of elements in the Array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_PTR ary);

/** Returns the current, temporary, array capacity (it's dynamic). */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_PTR ary);

/**
 * Returns a pointer to the C array containing the objects.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME2(FIO_ARRAY_NAME, ptr)(FIO_ARRAY_PTR ary);

/**
 * Reserves a minimal capacity for the array.
 *
 * If `capa` is negative, new memory will be allocated at the beginning of the
 * array rather then it's end.
 *
 * Returns the array's new capacity.
 *
 * Note: the reserved capacity includes existing data. If the requested reserved
 * capacity is equal (or less) then the existing capacity, nothing will be done.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_PTR ary,
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
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_PTR ary,
                                                    int32_t index,
                                                    FIO_ARRAY_TYPE data,
                                                    FIO_ARRAY_TYPE *old);

/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_PTR ary,
                                                   int32_t index);

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
IFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_PTR ary,
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
IFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_PTR ary,
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
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_PTR ary,
                                                 FIO_ARRAY_TYPE data);

/** Attempts to lower the array's memory consumption. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_PTR ary);

/**
 * Pushes an object to the end of the Array. Returns a pointer to the new object
 * or NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_PTR ary,
                                                     FIO_ARRAY_TYPE data);

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE *old);

/**
 * Unshifts an object to the beginning of the Array. Returns a pointer to the
 * new object or NULL on error.
 *
 * This could be expensive, causing `memmove`.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_PTR ary,
                                                        FIO_ARRAY_TYPE data);

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_PTR ary,
                                          FIO_ARRAY_TYPE *old);

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
                        each)(FIO_ARRAY_PTR ary,
                              int32_t start_at,
                              int (*task)(FIO_ARRAY_TYPE obj, void *arg),
                              void *arg);

#ifndef FIO_ARRAY_EACH
/**
 * Iterates through the list using a `for` loop.
 *
 * Access the object with the pointer `pos`. The `pos` variable can be named
 * however you please.
 *
 * Avoid editing the array during a FOR loop, although I hope it's possible, I
 * wouldn't count on it.
 *
 * **Note**: doesn't support automatic pointer tagging / untagging.
 */
#define FIO_ARRAY_EACH(array, pos)                                             \
  if ((array)->ary)                                                            \
    for (__typeof__((array)->ary) start__tmp__ = (array)->ary,                 \
                                  pos = ((array)->ary + (array)->start);       \
         pos < (array)->ary + (array)->end;                                    \
         (pos = (array)->ary + (pos - start__tmp__) + 1),                      \
                                  (start__tmp__ = (array)->ary))
#endif

#ifndef FIO_ARRAY_EACH
/**
 * Iterates through the list using a `for` loop.
 *
 * Access the object with the pointer `pos`. The `pos` variable can be named
 * however you please.
 *
 * Avoid editing the array during a FOR loop, although I hope it's possible, I
 * wouldn't count on it.
 *
 * **Note**: this variant supports automatic pointer tagging / untagging.
 */
#define FIO_ARRAY_EACH2(array_type, array, pos) TODO
#endif

#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
Helper macros
***************************************************************************** */
#if FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_ADD2CAPA(capa) (((capa) << 1) + FIO_ARRAY_PADDING)
#else
#define FIO_ARRAY_ADD2CAPA(capa) ((capa) + FIO_ARRAY_PADDING)
#endif

/* *****************************************************************************
Dynamic Arrays - embedded arrays (TODO)
***************************************************************************** */
#define FIO_ARRAY_IS_EMBEDED(a)                                                \
  (sizeof(FIO_ARRAY_TYPE) <= sizeof(void *) && ((a)->start > (a)->end))
#define FIO_ARRAY_IS_EMBEDDED_PTR(ary, ptr)                                    \
  ((uintptr_t)(ptr) > (uintptr_t)(ary) &&                                      \
   (uintptr_t)(ptr) < (uintptr_t)((ary) + 1))
#define FIO_ARRAY2EMBEDED(a) ((FIO_NAME(FIO_ARRAY_NAME, ___embedded_s) *)(a))
#define FIO_ARRAY_EMBEDED_CAPA                                                 \
  ((sizeof(FIO_NAME(FIO_ARRAY_NAME, s)) -                                      \
    sizeof(FIO_NAME(FIO_ARRAY_NAME, ___embedded_s))) /                         \
   sizeof(FIO_ARRAY_TYPE))
#define FIO_ARRAY_OFFSET(a) (FIO_ARRAY_IS_EMBEDED(a) ? 0 : a->start)

typedef struct {
  /* start common header */
  /** the offser to the first item. */
  uint32_t start;
  /** The offset to the first empty location the array. */
  uint32_t end;
  /* end common header */
  FIO_ARRAY_TYPE embded[];
} FIO_NAME(FIO_ARRAY_NAME, ___embedded_s);

/* *****************************************************************************
Dynamic Arrays - internal helpers
***************************************************************************** */

#define FIO_ARRAY_POS2ABS(ary, pos)                                            \
  (pos >= 0 ? (ary->start + pos) : (ary->end - pos))

#define FIO_ARRAY_AB_CT(cond, a, b) ((b) ^ ((0 - ((cond)&1)) & ((a) ^ (b))))

/* *****************************************************************************
Dynamic Arrays - implementation
***************************************************************************** */

#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new array object on the heap and initializes it's memory. */
IFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, new)(void) {
  FIO_NAME(FIO_ARRAY_NAME, s) *a =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*a), 0);
  if (!FIO_MEM_REALLOC_IS_SAFE_ && a) {
    *a = (FIO_NAME(FIO_ARRAY_NAME, s))FIO_ARRAY_INIT;
  }
  return (FIO_ARRAY_PTR)FIO_PTR_TAG(a);
}

/* Frees an array's internal data AND it's container! */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  FIO_NAME(FIO_ARRAY_NAME, destroy)(ary_);
  FIO_MEM_FREE_(ary, sizeof(*ary));
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Destroys any objects stored in the array and frees the internal state. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  /* move array data to a temp var., protecting against recursive nesting */
  FIO_NAME(FIO_ARRAY_NAME, s) tmp = *ary;
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s))FIO_ARRAY_INIT;

  if (FIO_ARRAY_IS_EMBEDED(&tmp))
    goto embedded;

#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE
  for (size_t i = tmp.start; i < tmp.end; ++i) {
    FIO_ARRAY_TYPE_DESTROY(tmp.ary[i]);
  }
#endif
  FIO_MEM_FREE_(tmp.ary, tmp.capa * sizeof(*tmp.ary));
  return;
embedded:
#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE
  while (tmp.start--) {
    FIO_ARRAY_TYPE_DESTROY((FIO_ARRAY2EMBEDED(&tmp)->embded[tmp.start]));
  }
#endif
  return;
}

/** Returns the number of elements in the Array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (FIO_ARRAY_IS_EMBEDED(ary))
    return FIO_ARRAY2EMBEDED(ary)->start;
  return (ary->end - ary->start);
}

/** Returns the current, temporary, array capacity (it's dynamic). */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (FIO_ARRAY_IS_EMBEDED(ary) || !ary->ary)
    return FIO_ARRAY_EMBEDED_CAPA;
  return ary->capa;
}

/**
 * Returns a pointer to the C array containing the objects.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME2(FIO_ARRAY_NAME, ptr)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, (FIO_ARRAY_TYPE *)NULL);

  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (FIO_ARRAY_IS_EMBEDED(ary) || !ary->ary)
    return FIO_ARRAY2EMBEDED(ary)->embded;

  return ary->ary + ary->start;
}

/** Reserves a minimal capacity for the array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_PTR ary_,
                                                 int32_t capa) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (!ary)
    return 0;

  uint32_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  uint32_t abs_capa = (capa >= 0) ? capa : (uint32_t)(0 - capa);
  const uint32_t old_capa = FIO_NAME(FIO_ARRAY_NAME, capa)(ary_);
  FIO_ARRAY_TYPE *const a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  if (old_capa >= abs_capa) {
    return old_capa;
  }
  abs_capa = FIO_ARRAY_SIZE2WORDS(abs_capa);

  /* if we don't need to move objects, use the system's realloc algorithm */
  if (!FIO_ARRAY_IS_EMBEDDED_PTR(ary, a) &&
      ((capa >= 0 && ary->end < ary->capa) || (capa < 0 && ary->start > 0))) {
    FIO_ARRAY_TYPE *tmp = FIO_MEM_REALLOC_(
        ary->ary, 0, sizeof(*tmp) * abs_capa, sizeof(*tmp) * ary->end);
    if (!tmp)
      return old_capa;
    ary->capa = abs_capa;
    ary->ary = tmp;
    return abs_capa;
  }

  /* moving objects, starting with a fresh piece of memory */
  FIO_ARRAY_TYPE *tmp = FIO_MEM_REALLOC_(NULL, 0, sizeof(*tmp) * abs_capa, 0);
  if (!tmp)
    return old_capa;
  if (capa >= 0) {
    /* copy items at begining of memory stack */
    if (count)
      FIO___MEMCPY(tmp, a, count * sizeof(*tmp));
    *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
        .start = 0,
        .end = count,
        .capa = abs_capa,
        .ary = tmp,
    };
    return abs_capa;
  }
  /* copy items at ending of memory stack */
  if (count)
    FIO___MEMCPY(tmp + (abs_capa - count), a, count * sizeof(*tmp));
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
      .start = 0,
      .end = count,
      .capa = abs_capa,
      .ary = tmp,
  };
  return abs_capa;
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
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(dest_));
  FIO_NAME(FIO_ARRAY_NAME, s) *src =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(src_));
  if (!dest || !src || src->start == src->end)
    return dest_;

  const uint32_t total = FIO_NAME(FIO_ARRAY_NAME, count)(dest_) +
                         FIO_NAME(FIO_ARRAY_NAME, count)(src_);
  if (total < FIO_NAME(FIO_ARRAY_NAME, count)(dest_) ||
      total < FIO_NAME(FIO_ARRAY_NAME, count)(src_))
    return NULL; /* item count overflow */

  const uint32_t capa = FIO_NAME(FIO_ARRAY_NAME, reserve)(dest_, total);

  if (!FIO_ARRAY_IS_EMBEDED(dest) && dest->start + total > capa) {
    /* we need to move the existing items due to the offset */
    memmove(dest->ary,
            dest->ary + dest->start,
            (dest->end - dest->start) * sizeof(*dest->ary));
  }

  /* copy data */
  memcpy(FIO_NAME2(FIO_ARRAY_NAME, ptr)(dest_) +
             FIO_NAME(FIO_ARRAY_NAME, count)(dest_),
         FIO_NAME2(FIO_ARRAY_NAME, ptr)(src_),
         FIO_NAME(FIO_ARRAY_NAME, count)(src_));
  /* update dest */
  if (!FIO_ARRAY_IS_EMBEDED(dest)) {
    dest->end += src->end - src->start;
    return dest_;
  }
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
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_PTR ary_,
                                                    int32_t index,
                                                    FIO_ARRAY_TYPE data,
                                                    FIO_ARRAY_TYPE *old) {
  FIO_NAME(FIO_ARRAY_NAME, s) * ary;
  FIO_ARRAY_TYPE *a;
  uint32_t count;
  uint8_t pre_existing = 1;

  FIO_PTR_TAG_VALID_OR_GOTO(ary_, invalid);

  ary = (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

  if (index < 0) {
    index += count;
    if (index < 0) {
      FIO_LOG_ERROR(FIO_MACRO2STR(FIO_NAME(
          FIO_ARRAY_NAME, set)) " called with a negative index lower "
                                "than the element count (array underflow).");
      goto invalid;
    }
  }

  FIO_NAME(FIO_ARRAY_NAME, reserve)
  (ary_, FIO_ARRAY_SIZE2WORDS(((size_t)index + 1)));
  a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);

  if ((uint32_t)index >= count) {
    pre_existing = 0;
    uint8_t was_moved = 0;
    /* test if we need to move objects to make room at the end */
    if (!FIO_ARRAY_IS_EMBEDDED_PTR(ary, a) && ary->start + index >= ary->capa) {
      memmove(ary->ary, ary->ary + ary->start, (count) * sizeof(*ary->ary));
      ary->start = 0;
      ary->end = count;
      was_moved = 1;
      a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
    }
    /* initialize memory in between objects */
    if ((uint32_t)index > count && (was_moved || !FIO_MEM_REALLOC_IS_SAFE_)) {
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
      memset(a + count, 0, (index - count) * sizeof(*ary->ary));
#else
      for (size_t i = count; i <= (size_t)index; ++i) {
        FIO_ARRAY_TYPE_COPY(a[i], FIO_ARRAY_TYPE_INVALID);
      }
#endif
    }
    if (FIO_ARRAY_IS_EMBEDDED_PTR(ary, a))
      ary->start = index + 1;
    else
      ary->end = index + 1;
  }

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

  FIO_ARRAY_TYPE_COPY(a[index], data);
  return a + index;

invalid:
  FIO_ARRAY_TYPE_DESTROY(data);
  if (old) {
    FIO_ARRAY_TYPE_COPY(old[0], FIO_ARRAY_TYPE_INVALID);
  }

  return NULL;
}
/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_PTR ary_,
                                                   int32_t index) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  if (!a)
    return FIO_ARRAY_TYPE_INVALID;
  size_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  if (index < 0) {
    index += count;
    if (index < 0)
      return FIO_ARRAY_TYPE_INVALID;
  }
  if ((uint32_t)index >= count)
    return FIO_ARRAY_TYPE_INVALID;
  return a[index];
}

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
IFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_PTR ary_,
                                             FIO_ARRAY_TYPE data,
                                             int32_t start_at) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  if (!a)
    return -1;
  size_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  if (start_at >= 0) {
    /* seek forwards */
    if ((uint32_t)start_at >= count)
      start_at = count;
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
IFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_PTR ary_,
                                           int32_t index,
                                           FIO_ARRAY_TYPE *old) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) * ary;
  ary = (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  size_t count;
  if (!a)
    goto invalid;
  count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

  if (index < 0) {
    index += count;
    if (index < 0) {
      FIO_LOG_WARNING(FIO_MACRO2STR(FIO_NAME(
          FIO_ARRAY_NAME, remove)) " called with a negative index lower "
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
  FIO___MEMCPY(a + index, a + index + 1, (index - count) * sizeof(*a));
  FIO_ARRAY_TYPE_COPY((a + (count - 1))[0], FIO_ARRAY_TYPE_INVALID);

  if (FIO_ARRAY_IS_EMBEDED(ary))
    goto embedded;
  --ary->end;
  return 0;
embedded:
  /* TODO! */
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
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_PTR ary_,
                                                 FIO_ARRAY_TYPE data) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) * ary;
  ary = (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  size_t count;
  if (!a)
    return 0;
  count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

  size_t c = 0;
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
    memset(a + i, 0, sizeof(*a) * c);
  }
  if (!FIO_ARRAY_IS_EMBEDDED_PTR(ary, a)) {
    ary->end = ary->start + i;
    return c;
  }
  ary->start = i;
  return c;
}

/** Attempts to lower the array's memory consumption. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  size_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  FIO_ARRAY_TYPE *tmp = NULL;

  if (count <= FIO_ARRAY_EMBEDED_CAPA)
    goto re_embed;

  tmp = (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(
      NULL, 0, (ary->end - ary->start) * sizeof(*tmp), 0);
  if (!tmp)
    return;
  memcpy(tmp, ary->ary + ary->start, count * sizeof(*ary->ary));
  FIO_MEM_FREE_(ary->ary, ary->capa * sizeof(*ary->ary));
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
      .ary = tmp,
      .capa = (ary->end - ary->start),
      .start = 0,
      .end = (ary->end - ary->start),
  };
  return;

re_embed:
  if (!FIO_ARRAY_IS_EMBEDED(ary)) {
    tmp = ary->ary;
    uint32_t offset = ary->start;
    size_t old_capa = ary->capa;
    *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
        .start = count,
    };
    if (count) {
      FIO___MEMCPY(
          FIO_ARRAY2EMBEDED(ary)->embded, tmp + offset, count * sizeof(*tmp));
    }
    if (tmp) {
      FIO_MEM_FREE_(tmp, sizeof(*tmp) * old_capa);
      (void)old_capa; /* if unused */
    }
  }
  return;
}
/**
 * Pushes an object to the end of the Array. Returns NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_PTR ary_,
                                                     FIO_ARRAY_TYPE data) {

  FIO_NAME(FIO_ARRAY_NAME, s) * ary;
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  uint32_t count;
  if (!a)
    goto error;
  /* test memory state */
  count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  if (count >= FIO_NAME(FIO_ARRAY_NAME, capa)(ary_)) {
    if (count >=
        FIO_NAME(FIO_ARRAY_NAME, reserve)(
            ary_, FIO_ARRAY_ADD2CAPA(FIO_NAME(FIO_ARRAY_NAME, capa)(ary_))))
      goto error;
    a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  }
  /* update array */
  FIO_ARRAY_TYPE_COPY(a[count], data);
  /* update container */
  ary = (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if ((uintptr_t)a < (uintptr_t)ary || (uintptr_t)a >= (uintptr_t)(ary + 1))
    ++ary->end;
  else
    ++ary->start;
  return a + count;

error:
  FIO_ARRAY_TYPE_DESTROY(data);
  return NULL;
}

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_PTR ary_,
                                        FIO_ARRAY_TYPE *old) {
  uint32_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
  FIO_ARRAY_TYPE *p = NULL;
  if (!count)
    goto no_object;
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (FIO_ARRAY_IS_EMBEDED(ary)) {
    --ary->start;
    p = FIO_ARRAY2EMBEDED(ary)->embded + ary->start;
  } else {
    --ary->end;
    p = ary->ary + ary->end;
  }
  if (old) {
    FIO_ARRAY_TYPE_COPY(*old, p[0]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
    FIO_ARRAY_TYPE_DESTROY(p[0]);
#endif
  } else {
    FIO_ARRAY_TYPE_DESTROY(p[0]);
  }
  FIO_ARRAY_TYPE_COPY(p[0], FIO_ARRAY_TYPE_INVALID);
  return 0;

no_object:
  FIO_ARRAY_TYPE_COPY(old[0], FIO_ARRAY_TYPE_INVALID);
  return -1;
}

/**
 * Unshifts an object to the beginning of the Array. Returns -1 on error.
 *
 * This could be expensive, causing `memmove`.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_PTR ary_,
                                                        FIO_ARRAY_TYPE data) {
  FIO_PTR_TAG_VALID_OR_GOTO(ary_, invalid_ptr);

  {
    FIO_NAME(FIO_ARRAY_NAME, s) *ary =
        (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
    if (ary->start) {
      --ary->start;
      FIO_ARRAY_TYPE *pos = ary->ary + ary->start;
      *pos = FIO_ARRAY_TYPE_INVALID;
      FIO_ARRAY_TYPE_COPY(*pos, data);
      return pos;
    }
    return FIO_NAME(FIO_ARRAY_NAME, set)(ary_, -1 - ary->end, data, NULL);
  }
invalid_ptr:
  FIO_ARRAY_TYPE_DESTROY(data);
  return NULL;
}

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_PTR ary_,
                                          FIO_ARRAY_TYPE *old) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, -1);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (!ary || ary->start == ary->end) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
    return -1;
  }
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
                              int32_t start_at,
                              int (*task)(FIO_ARRAY_TYPE obj, void *arg),
                              void *arg) {
  FIO_ARRAY_TYPE *a = FIO_NAME2(FIO_ARRAY_NAME, ptr)(ary_);
  if (!a)
    return start_at;
  {
    uint32_t count = FIO_NAME(FIO_ARRAY_NAME, count)(ary_);

    if (!a || !task)
      return start_at;
    if ((uint32_t)start_at >= count)
      return count;
  }

  for (; (uint32_t)start_at < FIO_NAME(FIO_ARRAY_NAME, count)(ary_);
       ++start_at) {
    if (task(a[(uint32_t)start_at], arg) == -1) {
      return (uint32_t)(start_at + 1);
    }
  }
  return start_at;
}

/* *****************************************************************************
Dynamic Arrays - test
***************************************************************************** */
#ifdef FIO_TEST_CSTL

FIO_SFUNC void FIO_NAME_TEST(stl, FIO_NAME(FIO_ARRAY_NAME, test))(void) {
  FIO_ARRAY_TYPE o;
  FIO_ARRAY_TYPE v;
#define FIO_ARRAY_TEST_OBJ_SET(val) memset(&o, (int)(val), sizeof(o))
#define FIO_ARRAY_TEST_OBJ_IS(val)                                             \
  (memset(&v, (int)(val), sizeof(v)) && 0 == memcmp(&o, &v, sizeof(o)))
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
                FIO_NAME(FIO_ARRAY_NAME, s)) ").\n",
            (selector ? "heap" : "stack"));
    /* Test start here */

    /* test push */
    for (int i = 0; i < (int)(FIO_ARRAY_EMBEDED_CAPA) + 3; ++i) {
      FIO_ARRAY_TEST_OBJ_SET((i + 1));
      FIO_NAME(FIO_ARRAY_NAME, push)(a, o);
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, i);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1), "push-get cycle failed (%d)", i);
      o = FIO_NAME(FIO_ARRAY_NAME, get)(a, -1);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(i + 1),
                 "get with -1 returned wrong result (%d)",
                 i);
    }
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) == FIO_ARRAY_EMBEDED_CAPA + 3,
               "push didn't update count correctly (%d != %d)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a),
               (int)(FIO_ARRAY_EMBEDED_CAPA) + 3);

    /* test pop */
    for (int i = (int)(FIO_ARRAY_EMBEDED_CAPA) + 3; i--;) {
      FIO_NAME(FIO_ARRAY_NAME, pop)(a, &o);
      FIO_ASSERT(
          FIO_ARRAY_TEST_OBJ_IS((i + 1)), "pop value error failed (%d)", i);
    }
    FIO_ASSERT(!FIO_NAME(FIO_ARRAY_NAME, count)(a),
               "pop didn't pop all elements?");

    /* test compact with zero elements */
    FIO_NAME(FIO_ARRAY_NAME, compact)(a);
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) == FIO_ARRAY_EMBEDED_CAPA,
               "compact zero elementes didn't make array embedded?");

    /* test set from embedded? array */
    FIO_ARRAY_TEST_OBJ_SET(1);
    FIO_NAME(FIO_ARRAY_NAME, push)(a, o);
    if (FIO_ARRAY_EMBEDED_CAPA) {
      FIO_ARRAY_TEST_OBJ_IS(1);
      FIO_NAME(FIO_ARRAY_NAME, set)(a, FIO_ARRAY_EMBEDED_CAPA, o, &o);
      FIO_ASSERT(FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID),
                 "set overflow from embedded array should reset `old`");
      FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                     FIO_ARRAY_EMBEDED_CAPA + 1,
                 "set didn't update count correctly from embedded "
                 "array (%d != %d)",
                 FIO_NAME(FIO_ARRAY_NAME, count)(a),
                 (int)FIO_ARRAY_EMBEDED_CAPA);
    }

    /* test set from bigger array */
    FIO_ARRAY_TEST_OBJ_SET(1);
    FIO_NAME(FIO_ARRAY_NAME, set)(a, ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4), o, &o);
    FIO_ASSERT(FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID),
               "set overflow should reset `old`");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4) + 1,
               "set didn't update count correctly (%d != %d)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a),
               (int)((FIO_ARRAY_EMBEDED_CAPA + 1) * 4));
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) >=
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4),
               "set capa should be above item count");
    if (FIO_ARRAY_EMBEDED_CAPA) {
      FIO_ARRAY_TYPE_COPY(o, FIO_ARRAY_TYPE_INVALID);
      FIO_NAME(FIO_ARRAY_NAME, set)(a, FIO_ARRAY_EMBEDED_CAPA, o, &o);
      FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(1),
                 "set overflow lost last item while growing.");
    }
    o = FIO_NAME(FIO_ARRAY_NAME, get)(a, (FIO_ARRAY_EMBEDED_CAPA + 1) * 2);
    FIO_ASSERT(FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID),
               "set overflow should have memory in the middle set to invalid "
               "objetcs.");
    FIO_ARRAY_TEST_OBJ_SET(2);
    FIO_NAME(FIO_ARRAY_NAME, set)(a, 0, o, &o);
    FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(1),
               "set should set `old` to previous value");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4) + 1,
               "set item count error");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) >=
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4) + 1,
               "set capa should be above item count");

    /* test find TODO: test with uninitialized array */
    FIO_ARRAY_TEST_OBJ_SET(99);
    if (FIO_ARRAY_TYPE_CMP(o, FIO_ARRAY_TYPE_INVALID)) {
      FIO_ARRAY_TEST_OBJ_SET(100);
    }
    int found = FIO_NAME(FIO_ARRAY_NAME, find)(a, o, 0);
    FIO_ASSERT(found == -1,
               "seeking for an object that doesn't exist should fail.");
    FIO_ARRAY_TEST_OBJ_SET(1);
    found = FIO_NAME(FIO_ARRAY_NAME, find)(a, o, 1);
    FIO_ASSERT(found == ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4),
               "seeking for an object returned the wrong index.");
    FIO_ASSERT(found == FIO_NAME(FIO_ARRAY_NAME, find)(a, o, -1),
               "seeking for an object in reverse returned the wrong index.");
    FIO_ARRAY_TEST_OBJ_SET(2);
    FIO_ASSERT(
        !FIO_NAME(FIO_ARRAY_NAME, find)(a, o, -2),
        "seeking for an object in reverse (2) returned the wrong index.");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4) + 1,
               "find should have side-effects - count error");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) >=
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4) + 1,
               "find should have side-effects - capa error");

    /* test remove */
    FIO_NAME(FIO_ARRAY_NAME, remove)(a, found, &o);
    FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(1), "remove didn't copy old data?");
    o = FIO_NAME(FIO_ARRAY_NAME, get)(a, 0);
    FIO_ASSERT(FIO_ARRAY_TEST_OBJ_IS(2), "remove removed more?");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) ==
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4),
               "remove with didn't update count correctly (%d != %s)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a),
               (int)((FIO_ARRAY_EMBEDED_CAPA + 1) * 4));
    o = FIO_NAME(FIO_ARRAY_NAME, get)(a, -1);

    /* test remove2 */
    FIO_ARRAY_TYPE_COPY(o, FIO_ARRAY_TYPE_INVALID);
    FIO_ASSERT((found = FIO_NAME(FIO_ARRAY_NAME, remove2)(a, o)) ==
                   ((FIO_ARRAY_EMBEDED_CAPA + 1) * 4) - 1,
               "remove2 result error, %d != %d items.",
               found,
               (int)((FIO_ARRAY_EMBEDED_CAPA + 1) * 4) - 1);
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, count)(a) == 1,
               "remove2 didn't update count correctly (%d != 1)",
               FIO_NAME(FIO_ARRAY_NAME, count)(a));

    /* test destroy */
    FIO_NAME(FIO_ARRAY_NAME, destroy)(a);
    FIO_ASSERT(!FIO_NAME(FIO_ARRAY_NAME, count)(a),
               "destroy didn't clear count.");
    FIO_ASSERT(FIO_NAME(FIO_ARRAY_NAME, capa)(a) == FIO_ARRAY_EMBEDED_CAPA,
               "destroy capa error.");
    /* Test end here */
  }
  FIO_NAME(FIO_ARRAY_NAME, free)(a_array[1]);
}
#undef FIO_ARRAY_TEST_OBJ_SET
#undef FIO_ARRAY_TEST_OBJ_IS

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Dynamic Arrays - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_ARRAY_NAME
#undef FIO_ARRAY_TYPE
#undef FIO_ARRAY_TYPE_INVALID
#undef FIO_ARRAY_TYPE_INVALID_SIMPLE
#undef FIO_ARRAY_TYPE_COPY
#undef FIO_ARRAY_TYPE_COPY_SIMPLE
#undef FIO_ARRAY_TYPE_DESTROY
#undef FIO_ARRAY_TYPE_DESTROY_SIMPLE
#undef FIO_ARRAY_DESTROY_AFTER_COPY
#undef FIO_ARRAY_TYPE_CMP
#undef FIO_ARRAY_TYPE_CMP_SIMPLE
#undef FIO_ARRAY_TYPE_CONCAT_COPY
#undef FIO_ARRAY_PADDING
#undef FIO_ARRAY_SIZE2WORDS
#undef FIO_ARRAY_POS2ABS
#undef FIO_ARRAY_AB_CT
#undef FIO_ARRAY_PTR
#undef FIO_ARRAY_EXPONENTIAL
#undef FIO_ARRAY_ADD2CAPA
#endif /* FIO_ARRAY_NAME */
